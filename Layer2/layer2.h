#ifndef __LAYER2__
#define __LAYER2__

#include "../net.h"
#include "../gluethread/glthread.h"
#include "graph.h"

#define ARP_BROAD_REQ   1
#define ARP_REPLY       2
#define ARP_MSG         806
#define BROADCAST_MAC   0xFFFFFFFFFFFF

#pragma pack (push,1)
typedef struct arp_hdr_ {
    short hw_type;          /*1 for ethernet cable*/
    short proto_type;       /*0x0800 for IPV4*/
    char hw_addr_len;       /*6 for MAC*/
    char proto_addr_len;    /*4 for IPV4*/
    short op_code;          /*Opcode of ARP message Opcode=1 is ARP broadcast, Opcode=2 is ARP reply.*/
    mac_add_t src_mac;      /*MAC of OIF interface*/
    unsigned int src_ip;    /*IP of OIF*/
    mac_add_t dst_mac;      /*?*/
    unsigned int dst_ip;    /*IP for which ARP is being resolved*/
} arp_hdr_t;

typedef struct ethernet_hdr_ {
    mac_add_t dst_mac;
    mac_add_t src_mac;
    short type;
    char payload[248];
    unsigned int FCS;
} ethernet_hdr_t;
#pragma pack(pop)


typedef struct arp_table_ {
    glthread_t arp_entries;
} arp_table_t;

typedef struct arp_entry_ {
    ip_add_t ip_addr;
    mac_add_t mac_addr;
    char oif_name[IF_NAME_SIZE];
    glthread_t arp_glue;
} arp_entry_t;

static inline bool_t l2_frame_recv_qualify_on_interface(
    interface_t *interface,
    ethernet_hdr_t *ethernet_hdr) {

    /* Presence of IP address on interface makes it work in L3 mode,
     * while absence of IP-address automatically make it work in
     * L2 mode. For interfaces working in L2 mode (L2 switch interfaces),
     * We should accept all frames. L2 switch never discards the frames
     * based on MAC addresses*/
    if (!interface->intf_nw_props.is_ipadd_config) {
        return TRUE;
    }

    /*Return TRUE if receiving machine must accept the frame*/
    if (memcmp(IF_MAC(interface), ethernet_hdr->dst_mac.mac, sizeof(mac_add_t)) == 0) {
        return TRUE;
    }

    if (IS_MAC_BROADCAST_ADDR(ethernet_hdr->dst_mac.mac)) {
        return TRUE;
    }

    return FALSE;
}

GLTHREAD_TO_STRUCT(arp_glue_to_arp_entry, arp_entry_t, arp_glue);

void send_arp_broadcast_request(node_t *node, interface_t *oif, char *ip_addr);

void init_arp_table(arp_table_t **arp_table);

arp_entry_t *arp_table_lookup(arp_table_t *arp_table, char *ip_addr);

bool_t arp_table_entry_add(arp_table_t *arp_table, arp_entry_t *arp_entry);

void clear_arp_table(arp_table_t *arp_table);

void delete_arp_table_entry(arp_table_t *arp_table, char *ip_addr);

void dump_arp_table(arp_table_t *arp_table);


void arp_table_update_from_arp_reply(arp_table_t *arp_table, arp_hdr_t *arp_hdr, interface_t *iif);

#endif /* __LAYER2__ */