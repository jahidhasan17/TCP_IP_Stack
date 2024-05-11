#include "graph.h"
#include <stdio.h>

void layer3_pkt_recv(node_t *node, interface_t *interface
                        char *pkt, unsigned int pkt_size) {
    printf("Layer 3 Packet Recvd : Rcv Node %s, Intf : %s, data recvd : %s, pkt size : %u\n",
            node->node_name, interface->if_name, pkt, pkt_size);
}