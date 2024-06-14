#include "graph.h"
#include <memory.h>
#include "utils.h"
#include <stdio.h>

/*Just some Random number generator*/
static unsigned int hash_code(void *ptr, unsigned int size){
    unsigned int value=0, i =0;
    char *str = (char*)ptr;
    while(i < size)
    {
        value += *str;
        value*=97;
        str++;
        i++;
    }
    return value;
}

/*Heuristics, Assign a unique mac address to interface*/
void interface_assign_mac_address(interface_t *interface) {

    unsigned int hash_code_val = hash_code(interface, sizeof(interface_t));
    memset(IF_MAC(interface), 0, sizeof(IF_MAC(interface)));
    memcpy(IF_MAC(interface), (char *)&hash_code_val, sizeof(unsigned int));
}

bool_t node_set_device_type(node_t *node, unsigned int F) {
    SET_BIT(node->node_nw_prop.flags, F);

    return TRUE;
}

bool_t node_set_loopback_address(node_t *node, char *ip_addr) {

    assert(ip_addr);

    if (IS_BIT_SET(node->node_nw_prop.flags, HUB)) {
        assert(0); /*Wrong Config : A HUB do not have any IP addresses*/
    }

    if (!IS_BIT_SET(node->node_nw_prop.flags, L3_ROUTER)) {
        assert(0); /*You must enable L3 routing on device first*/
    }

    node->node_nw_prop.is_lb_addr_config = TRUE;
    strncpy(NODE_LO_ADDR(node), ip_addr, 16);
    NODE_LO_ADDR(node)[16] = '\0';

    return TRUE;
}

bool_t node_set_intf_ip_address(node_t *node, char *local_if, char *ip_addr, char mask) {
    interface_t *interface = get_node_if_by_name(node, local_if);

    if (!interface) {
        assert(0);
    }

    strncpy(IF_IP(interface), ip_addr, 16);
    IF_IP(interface)[16] = '\0';
    interface->intf_nw_props.mask = mask;
    interface->intf_nw_props.is_ipadd_config = TRUE;

    return TRUE;
}

bool_t node_unset_intf_ip_address(node_t *node, char *local_if){

    return TRUE;
}


void dump_node_nw_props(node_t *node){

    printf("\nNode Name = %s, udp_port_no = %u\n", node->node_name, node->udp_port_number);
    printf("\t node flags : %u", node->node_nw_prop.flags);
    if(node->node_nw_prop.is_lb_addr_config){
        printf("\t  lo addr : %s/32\n", NODE_LO_ADDR(node));
    }
}

void dump_intf_props(interface_t *interface){

    dump_interface(interface);

    if(interface->intf_nw_props.is_ipadd_config){
        printf("\t IP Addr = %s/%u", IF_IP(interface), interface->intf_nw_props.mask);
    }
    else{
         printf("\t IP Addr = %s/%u", "Nil", 0);
    }

    printf("\t MAC : %u:%u:%u:%u:%u:%u\n", 
        IF_MAC(interface)[0], IF_MAC(interface)[1],
        IF_MAC(interface)[2], IF_MAC(interface)[3],
        IF_MAC(interface)[4], IF_MAC(interface)[5]);
}

void dump_nw_graph(graph_t *graph){

    node_t *node;
    glthread_t *curr;
    interface_t *interface;
    unsigned int i;
    
    printf("Topology Name = %s\n", graph->topology_name);

    ITERATE_GLTHREAD_BEGIN(&graph->node_list, curr){

        node = graph_glue_to_node(curr);
        dump_node_nw_props(node);
        for( i = 0; i < MAX_INTF_PER_NODE; i++){
            interface = node->intf[i];
            if(!interface) break;
            dump_intf_props(interface);
        }
    } ITERATE_GLTHREAD_END(&graph->node_list, curr);

}

interface_t *node_get_matching_subnet_interface(node_t *node, char *ip_addr) {
    
    interface_t *interface;
    char ip_network[16];
    char ip_mask = 0;
    char ip_addr_subnet[16];

    for (int i = 0; i < MAX_INTF_PER_NODE; i++) {
        interface = node->intf[i];

        if (!interface) {
            return NULL;
        }

        if (!interface->intf_nw_props.is_ipadd_config) {
            continue;
        }

        apply_mask(IF_IP(interface), interface->intf_nw_props.mask, ip_network);
        apply_mask(ip_addr, interface->intf_nw_props.mask, ip_addr_subnet);

        if (strncmp(ip_network, ip_addr_subnet, 16) == 0) {
            return interface;
        }
    }

    return NULL;
}