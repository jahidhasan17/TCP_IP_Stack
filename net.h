#ifndef __NET__
#define __NET__

#include "utils.h"
#include <memory.h>

/*Device IDS*/
#define L3_ROUTER (1 << 0)
#define L2_SWITCH (1 << 1)
#define HUB       (1 << 2)

typedef struct graph_ graph_t;
typedef struct interface_ interface_t;
typedef struct node_ node_t;

#pragma pack (push,1)
typedef struct ip_add_ {
    unsigned char ip_addr[16];
} ip_add_t;

typedef struct mac_add_ {
    unsigned char mac[48];
} mac_add_t;
#pragma pack(pop)

/*Forward Declaration*/
typedef struct arp_table_ arp_table_t;

typedef struct node_nw_prop_ {

    /* Used to find various device types capabilities of
     * the node and other features*/
    unsigned int flags;

    /*L2 Properties*/
    arp_table_t *arp_table;

    /*L3 properties*/
    bool_t is_lb_addr_config; /*is loopback address is configured to this node or not.*/
    ip_add_t lb_addr; /*loopback address of node*/
} node_nw_prop_t;

typedef struct intf_nw_props_ {

    /*L2 properties*/
    mac_add_t mac_add; /*Mac are hard burnt in interface NIC*/

    /*L3 properties*/
    bool_t is_ipadd_config;   /*Set to TRUE if ip add is configured, intf operates in L3 mode if ip address is configured on it*/
    ip_add_t ip_add;

    char mask;

} inft_nw_props_t;

/*GET shorthand Macros*/
#define IF_MAC(intf_ptr)   ((intf_ptr)->intf_nw_props.mac_add.mac)
#define IF_IP(intf_ptr)    ((intf_ptr)->intf_nw_props.ip_add.ip_addr)

#define NODE_LO_ADDR(node_ptr) (node_ptr->node_nw_prop.lb_addr.ip_addr)
#define NODE_ARP_TABLE(node_ptr) (node_ptr->node_nw_prop.arp_table)

extern void
init_arp_table(arp_table_t **arp_table);

static inline void init_node_nw_prop(node_nw_prop_t *node_nw_prop) {
    node_nw_prop->flags = 0;
    node_nw_prop->is_lb_addr_config = FALSE;
    memset(node_nw_prop->lb_addr.ip_addr, 0, 16);
    init_arp_table(&node_nw_prop->arp_table);
}

static inline void init_intf_nw_prop(inft_nw_props_t *intf_nw_props) {

    memset(intf_nw_props->mac_add.mac, 0, sizeof(intf_nw_props->mac_add.mac));
    intf_nw_props->is_ipadd_config = FALSE;
    memset(intf_nw_props->ip_add.ip_addr, 0, 16);
    intf_nw_props->mask = 0;
}

void interface_assign_mac_address(interface_t *interface);

/*APIs to set Network Node properties*/
bool_t node_set_device_type(node_t *node, unsigned int F);
bool_t node_set_loopback_address(node_t *node, char *ip_addr);
bool_t node_set_intf_ip_address(node_t *node, char *local_if, char *ip_addr, char mask);
bool_t node_unset_intf_ip_address(node_t *node, char *local_if);

/*Dumping Functions to dump network information
 * on nodes and interfaces*/
void dump_nw_graph(graph_t *graph);
void dump_node_nw_props(node_t *node);
void dump_intf_props(interface_t *interface);

/*Helper Routines*/
interface_t *node_get_matching_subnet_interface(node_t *node, char *ip_addr);


#endif