#include "comm.h"
#include "graph.h"
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h> /*for struct hostent*/
#include <sys/select.h>

static unsigned int udp_port_number = 400000;
static char recv_buffer[MAX_PACKET_BUFFER_SIZE];
static char send_buffer[MAX_PACKET_BUFFER_SIZE];

static unsigned int get_next_udp_port_number() {
    return udp_port_number++;
}

void init_udp_socket(node_t *node) {
    
    if (node->udp_port_number) {
        return;
    }

    int udp_sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (udp_sock_fd == -1) {
        printf("Socket Creation Failed for node %s\n", node->node_name);
        return;
    }

    struct sockaddr_in node_addr;
    node_addr.sin_family = AF_INET;
    node_addr.sin_port = node->udp_port_number;
    node_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_sock_fd, (struct sockaddr *)&node_addr, sizeof(struct sockaddr)) == -1) {
        printf("Error : socket bind failed for Node %s\n", node->node_name);
        return;
    }

    node->udp_sock_fd = udp_sock_fd;
}

void _network_start_pkt_receiver_thread(void *arg) {

    node_t *node;
    glthread_t *curr;

    fd_set active_sock_fd_set, backup_sock_fd_set;

    int sock_max_fd = 0;
    int bytes_recvd = 0;

    graph_t *topo = (void *)arg;

    int addr_len = sizeof(struct sockaddr);

    FD_ZERO(&active_sock_fd_set);
    FD_ZERO(&backup_sock_fd_set);

    ITERATE_GLTHREAD_BEGIN(&topo->node_list, curr) {
        node = graph_glue_to_node(curr);

        if (!node->udp_sock_fd) {
            continue;
        }

        if(node->udp_sock_fd > sock_max_fd)
            sock_max_fd = node->udp_sock_fd;

        FD_SET(node->udp_sock_fd, &backup_sock_fd_set);
    } ITERATE_GLTHREAD_END(&topo->node_list, curr);

    struct sockaddr_in sender_addr;

    while(1) {
        memcpy(&active_sock_fd_set, &backup_sock_fd_set, sizeof(fd_set));

        select(sock_max_fd + 1, &active_sock_fd_set, NULL, NULL, NULL);

        ITERATE_GLTHREAD_BEGIN(&topo->node_list, curr){
            node = graph_glue_to_node(curr);

            if (FD_ISSET(node->udp_sock_fd, &active_sock_fd_set)) {
                memset(recv_buffer, 0, MAX_PACKET_BUFFER_SIZE);

                bytes_recvd = recvfrom(node->udp_sock_fd, (char *)recv_buffer,
                                        MAX_PACKET_BUFFER_SIZE, 0, (struct sockaddr *)&sender_addr, &addr_len);

                _pkt_receive(node, recv_buffer, bytes_recvd);
            }
        }
    }

}

static void _pkt_receive(node_t *receving_node, char *pkt_with_aux_data, unsigned int pkt_size) {
    char *recv_intf_name = pkt_with_aux_data;
    interface_t *recv_intf = get_node_if_by_name(receving_node, recv_intf_name);

    if(!recv_intf) {
        printf("Error : Pkt recvd on unknown interface %s on node %s\n", 
                    recv_intf->if_name, receving_node->node_name);
        return;
    }

    pkt_receive(receving_node, recv_intf, pkt_with_aux_data + IF_NAME_SIZE, pkt_size - IF_NAME_SIZE);
}

int pkt_receive(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size){
    
    /*Do further processing of the pkt here*/
    layer2_frame_recv(node, interface, pkt, pkt_size );
    
    return 0;
}

void network_start_pkt_receiver_thread(graph_t *topo) {

    pthread_attr_t attr;
    pthread_t recv_pkt_thread;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&recv_pkt_thread, &attr,
                    _network_start_pkt_receiver_thread,
                    (void *)topo);

}

