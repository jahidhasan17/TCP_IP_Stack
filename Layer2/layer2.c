#include "graph.h"
#include <stdio.h>

extern void layer3_pkt_recv(node_t *node, interface_t *interface, 
                char *pkt, unsigned int pkt_size);

static void promote_pkt_to_layer3(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size) {
    layer3_pkt_recv(node, interface, pkt, pkt_size);
}

void layer2_frame_recv(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size) {
    
    printf("Layer 2 Frame Recvd : Rcv Node %s, Intf : %s, data recvd : %s, pkt size : %u\n",
            node->node_name, interface->if_name, pkt, pkt_size);

    promote_pkt_to_layer3(node, interface, pkt, pkt_size);
}