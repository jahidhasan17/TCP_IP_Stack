#include "graph.h"
#include <stdio.h>
#include "layer2.h"
#include "utils.h"

extern void layer3_pkt_recv(node_t *node, interface_t *interface, 
                char *pkt, unsigned int pkt_size);

static void promote_pkt_to_layer3(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size) {
    layer3_pkt_recv(node, interface, pkt, pkt_size);
}

void send_arp_broadcast_request(node_t *node, interface_t *oif, char *ip_addr) {

    /*Take memory which can accomodate Ethernet hdr + ARP hdr*/
    ethernet_hdr_t *ethernet_hdr = calloc(1, sizeof(ethernet_hdr_t) + sizeof(arp_hdr_t));

    if (!oif) {

    }

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
}

static void process_arp_broadcast_request(node_t *node, interface_t *iif, ethernet_hdr_t *ethernet_hdr) {

}

void layer2_frame_recv(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size) {
    
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
                    
                    break;
                case ARP_REPLY:
                    break;
                default:
                    break;
            }

            break;
        
        default:
            promote_pkt_to_layer3(node, interface, pkt, pkt_size);
            break;
    }


    printf("Layer 2 Frame Recvd : Rcv Node %s, Intf : %s, data recvd : %s, pkt size : %u\n",
            node->node_name, interface->if_name, pkt, pkt_size);

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