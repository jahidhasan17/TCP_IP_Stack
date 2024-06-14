#include "graph.h"
#include <stdio.h>
#include "layer2.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "comm.h"

extern void layer3_pkt_recv(node_t *node, interface_t *interface, 
                char *pkt, unsigned int pkt_size);

static void promote_pkt_to_layer3(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size) {
    layer3_pkt_recv(node, interface, pkt, pkt_size);
}

void send_arp_broadcast_request(node_t *node, interface_t *oif, char *ip_addr) {

    /*Take memory which can accomodate Ethernet hdr + ARP hdr*/
    ethernet_hdr_t *ethernet_hdr = calloc(1, sizeof(ethernet_hdr_t) + sizeof(arp_hdr_t));

    if (!oif) {
        oif = node_get_matching_subnet_interface(node, ip_addr);

        if (!oif) {
           printf("Error : %s : No eligible subnet for ARP resolution for Ip-address : %s",
                    node->node_name, ip_addr);
            return; 
        }
    }

    printf("%s : ARP Broadcast Request for IP %s on Node %s, Outgoing Interface %s\n",
            __FUNCTION__, ip_addr, node->node_name, oif->if_name);

    /*STEP 1 : Prepare ethernet hdr*/
    layer2_fill_with_broadcast_mac(ethernet_hdr->dst_mac.mac);
    memcpy(ethernet_hdr->src_mac.mac, IF_MAC(oif), sizeof(mac_add_t));
    ethernet_hdr->type = ARP_MSG;

    /*Step 2 : Prepare ARP Broadcast Request Msg out of oif*/
    arp_hdr_t *arp_hdr = (arp_hdr_t *)ethernet_hdr->payload;
    
    arp_hdr->hw_type = 1;
    arp_hdr->proto_type = 0x0800;
    arp_hdr->hw_addr_len = 6;
    arp_hdr->proto_addr_len = 4;

    arp_hdr->op_code = ARP_BROAD_REQ;

    memcpy(arp_hdr->src_mac.mac, IF_MAC(oif), sizeof(mac_add_t));

    inet_pton(AF_INET, IF_IP(oif), &arp_hdr->src_ip);
    arp_hdr->src_ip = htonl(arp_hdr->src_ip);

    memset(arp_hdr->dst_mac.mac, 0, sizeof(mac_add_t));

    inet_pton(AF_INET, ip_addr, &arp_hdr->dst_ip);
    arp_hdr->dst_ip = htonl(arp_hdr->dst_ip);

    ethernet_hdr->FCS = 0;  /*Not used*/

    printf("ARP Broadcast Request : Src IP : %s, Dst IP : %s\n", 
            IF_IP(oif), ip_addr);
    printf("ARP Broadcast Request : Src MAC : %u:%u:%u:%u:%u:%u, Dst MAC : %u:%u:%u:%u:%u:%u\n",
            ethernet_hdr->src_mac.mac[0], ethernet_hdr->src_mac.mac[1], ethernet_hdr->src_mac.mac[2],
            ethernet_hdr->src_mac.mac[3], ethernet_hdr->src_mac.mac[4], ethernet_hdr->src_mac.mac[5],
            ethernet_hdr->dst_mac.mac[0], ethernet_hdr->dst_mac.mac[1], ethernet_hdr->dst_mac.mac[2],
            ethernet_hdr->dst_mac.mac[3], ethernet_hdr->dst_mac.mac[4], ethernet_hdr->dst_mac.mac[5]);

    /*STEP 3 : Now dispatch the ARP Broadcast Request Packet out of interface*/
    send_pkt_out((char *)ethernet_hdr, sizeof(ethernet_hdr_t) + sizeof(arp_hdr_t), oif);

    free(ethernet_hdr);
}

static void send_arp_reply_msg(ethernet_hdr_t *ethernet_hdr_in, interface_t *oif) {
    ethernet_hdr_t *ethernet_hdr = calloc(1, sizeof(ethernet_hdr_t) + sizeof(arp_hdr_t));

    /*Prepare ethernet hdr*/
    memcpy(ethernet_hdr->dst_mac.mac, ethernet_hdr_in->src_mac.mac, sizeof(mac_add_t));
    memcpy(ethernet_hdr->src_mac.mac, IF_MAC(oif), sizeof(mac_add_t));
    ethernet_hdr->type = ARP_MSG;

    /*Prepare ARP Reply Msg*/
    arp_hdr_t *arp_hdr = (arp_hdr_t *)ethernet_hdr->payload;

    arp_hdr->hw_type = 1;
    arp_hdr->proto_type = 0x0800;
    arp_hdr->hw_addr_len = 6;
    arp_hdr->proto_addr_len = 4;
    arp_hdr->op_code = ARP_REPLY;

    memcpy(arp_hdr->src_mac.mac, IF_MAC(oif), sizeof(mac_add_t));

    inet_pton(AF_INET, IF_IP(oif), &arp_hdr->src_ip);
    arp_hdr->src_ip = htonl(arp_hdr->src_ip);

    memcpy(arp_hdr->dst_mac.mac, ethernet_hdr_in->src_mac.mac, sizeof(mac_add_t));
    arp_hdr->dst_ip = arp_hdr->src_ip;

    ethernet_hdr->FCS = 0;

    send_pkt_out((char *)ethernet_hdr, sizeof(ethernet_hdr_t) + sizeof(arp_hdr_t), oif);

    free(ethernet_hdr);
}

static void process_arp_broadcast_request(node_t *node, interface_t *iif, ethernet_hdr_t *ethernet_hdr) {
    printf("%s : ARP Broadcast msg recvd on interface %s of node %s\n", 
                __FUNCTION__, iif->if_name , iif->att_node->node_name);

    char ip_addr[16];
    arp_hdr_t *arp_hdr = (arp_hdr_t *)ethernet_hdr->payload;

    unsigned int arp_dst_ip = htonl(arp_hdr->dst_ip);
    inet_ntop(AF_INET, &arp_dst_ip, ip_addr, 16);
    ip_addr[15] = '\0';

    if (strncmp(IF_IP(iif), ip_addr, 16)) {
        printf("%s : ARP Broadcast req msg dropped, Dst IP address did not match", 
                node->node_name );
        return;
    }

    send_arp_reply_msg(ethernet_hdr, iif);
}

static void process_arp_reply_msg(node_t *node, interface_t *iif, ethernet_hdr_t *ethernet_hdr) {
    printf("%s : ARP reply msg recvd on interface %s of node %s\n",
             __FUNCTION__, iif->if_name , iif->att_node->node_name);

    arp_hdr_t *arp_hdr = (arp_hdr_t *)ethernet_hdr->payload;

    arp_table_update_from_arp_reply(NODE_ARP_TABLE(node), arp_hdr, iif);
}

void layer2_frame_recv(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size) {
    
    printf("L2 Frame Recvd on Node %s, Interface %s\n", node->node_name, interface->if_name);

    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)pkt;

    if (l2_frame_recv_qualify_on_interface(interface, ethernet_hdr) == FALSE) {
        printf("L2 Frame Rejected");

        return;
    }

    printf("L2 Frame Accepted\n");

    switch (ethernet_hdr->type)
    {
        case ARP_MSG:
            /*Can be ARP Broadcast or ARP reply*/
            arp_hdr_t *arp_hdr = (arp_hdr_t *)ethernet_hdr->payload;

            switch (arp_hdr->op_code)
            {
                case ARP_BROAD_REQ:
                    process_arp_broadcast_request(node, interface, ethernet_hdr);
                    break;
                case ARP_REPLY:
                    process_arp_reply_msg(node, interface, ethernet_hdr);
                    break;
                default:
                    break;
            }

            break;
        
        default:
            promote_pkt_to_layer3(node, interface, pkt, pkt_size);
            break;
    }
}

void init_arp_table(arp_table_t **arp_table) {
    *arp_table = calloc(1, sizeof(arp_table_t));
    init_glthread(&(*arp_table)->arp_entries);
}

arp_entry_t *arp_table_lookup(arp_table_t *arp_table, char *ip_addr) {
    arp_entry_t *arp_entry;
    glthread_t *curr;

    ITERATE_GLTHREAD_BEGIN(&arp_table->arp_entries, curr) {
        arp_entry = arp_glue_to_arp_entry(curr);
        if (strncmp(arp_entry->ip_addr.ip_addr, ip_addr, 16) == 0) {
            return arp_entry;
        }
    } ITERATE_GLTHREAD_END(&arp_table->arp_entries, curr);

    return NULL;
}

void clear_arp_table(arp_table_t *arp_table) {
    glthread_t *curr;
    arp_entry_t *arp_entry;

    ITERATE_GLTHREAD_BEGIN(&arp_table->arp_entries, curr) {
        arp_entry = arp_glue_to_arp_entry(curr);
        remove_glthread(&arp_entry->arp_glue);
        free(arp_entry);
    } ITERATE_GLTHREAD_END(&arp_table->arp_entries, curr);

    free(arp_table);
}

void delete_arp_table_entry(arp_table_t *arp_table, char *ip_addr) {
    arp_entry_t *arp_entry = arp_table_lookup(arp_table, ip_addr);

    if (!arp_entry) {
        return;
    }

    remove_glthread(&arp_entry->arp_glue);
    free(arp_entry);
}

bool_t arp_table_entry_add(arp_table_t *arp_table, arp_entry_t *arp_entry) {
    arp_entry_t *arp_entry_old = arp_table_lookup(arp_table, arp_entry->ip_addr.ip_addr);

    if (arp_entry_old && memcmp(arp_entry_old, arp_entry, sizeof(arp_entry_t)) == 0) {
        return FALSE;
    }

    if (arp_entry_old) {
        delete_arp_table_entry(arp_table, arp_entry->ip_addr.ip_addr);
    }

    init_glthread(&arp_entry->arp_glue);
    glthread_add_next(&arp_table->arp_entries, &arp_entry->arp_glue);

    return TRUE;
}

void arp_table_update_from_arp_reply(arp_table_t *arp_table, arp_hdr_t *arp_hdr, interface_t *iif) {
    arp_entry_t *arp_entry = calloc(1, sizeof(arp_entry_t));

    unsigned int src_ip = htonl(arp_hdr->src_ip);
    inet_ntop(AF_INET, &src_ip, arp_entry->ip_addr.ip_addr, 16);
    arp_entry->ip_addr.ip_addr[15] = '\0';

    memcpy(arp_entry->mac_addr.mac, arp_hdr->src_mac.mac, sizeof(mac_add_t));
    strncpy(arp_entry->oif_name, iif->if_name, IF_NAME_SIZE);

    bool_t rc = arp_table_entry_add(arp_table, arp_entry);

    if (rc == FALSE) {
        free(arp_entry);
    }
}

void dump_arp_table(arp_table_t *arp_table) {
    glthread_t *curr;
    arp_entry_t *arp_entry;

    ITERATE_GLTHREAD_BEGIN(&arp_table->arp_entries, curr){

        arp_entry = arp_glue_to_arp_entry(curr);
        printf("IP : %s, MAC : %u:%u:%u:%u:%u:%u, OIF = %s\n", 
            arp_entry->ip_addr.ip_addr, 
            arp_entry->mac_addr.mac[0], 
            arp_entry->mac_addr.mac[1], 
            arp_entry->mac_addr.mac[2], 
            arp_entry->mac_addr.mac[3], 
            arp_entry->mac_addr.mac[4], 
            arp_entry->mac_addr.mac[5], 
            arp_entry->oif_name);
    } ITERATE_GLTHREAD_END(&arp_table->arp_entries, curr);
}