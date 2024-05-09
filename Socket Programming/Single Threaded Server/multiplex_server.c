#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include "../common.h"
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>

#define SERVER_PORT 2000
#define MAX_CLIENT_SUPPORTED 3


char data_buffer[1024];

int monitor_fd_set[MAX_CLIENT_SUPPORTED];

static void initialize_monitor_fd_set() {
    for(int i = 0; i < MAX_CLIENT_SUPPORTED; i++) {
        monitor_fd_set[i] = -1;
    }
}

static void re_init_readfds(fd_set *fd_set_ptr) {
    FD_ZERO(fd_set_ptr);

    for(int i = 0; i < MAX_CLIENT_SUPPORTED; i++) {
        if (monitor_fd_set[i] != -1) {
            FD_SET(monitor_fd_set[i], fd_set_ptr);
        }
    }
}

static void add_to_monitor_fd_set(int new_fd) {
    for(int i = 0; i < MAX_CLIENT_SUPPORTED; i++) {
        if (monitor_fd_set[i] == -1) {
            monitor_fd_set[i] = new_fd;
            break;
        }
    }
}

static void remove_from_monitor_fd_set(int current_fd){
    for(int i = 0; i < MAX_CLIENT_SUPPORTED; i++) {
        if (monitor_fd_set[i] == current_fd) {
            monitor_fd_set[i] = -1;
            break;
        }
    }
}

static int get_max_fd() {
    int max = -1;
    for(int i = 0; i < MAX_CLIENT_SUPPORTED; i++) {
        if (monitor_fd_set[i] > max) {
            max = monitor_fd_set[i];
        }
    }

    return max;
}

static int get_total_connected_client() {
    int count = 0;
    for(int i = 0; i < MAX_CLIENT_SUPPORTED; i++) {
        if (monitor_fd_set[i] != -1) {
            count++;
        }
    }

    return count;
}

void setup_tcp_server_communication()
{
    /*Server socket descriptor, used for accepting client connection.*/
    int master_socket_tcp_file_descriptor = 0;

    /*structure to store the server and client info*/
    struct sockaddr_in server_addr, client_addr;

    socklen_t socket_address_length = sizeof(struct sockaddr);

    initialize_monitor_fd_set();

    /*Set of file descriptor on which select() polls. Select() unblocks whever data arrives on any fd present in this set*/
    /*Fd_set is a data structure which store the file descriptor(fd) and keeps truck of file descriptor, which fd is active now and allow us to many operator on FD*/
    fd_set readfds;

    /* Creating Server Socket.*/
    if ((master_socket_tcp_file_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        printf("Server socket creation failed\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = SERVER_PORT;

    /*3232249957; //( = 192.168.56.101); Server's IP address,
    //means, Linux will send all data whose destination address = address of any local interface
    //of this machine, in this case it is 192.168.56.101*/
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* Bind the server. Binding means, we are telling kernel(OS) that any data
     * you recieve with dest ip address = 192.168.56.101, and tcp port no = 2000, pls send that data to this process
     * bind() is a mechnism to tell OS what kind of data server process is interested in to recieve. Remember, server machine
     * can run multiple server processes to process different data and service different clients. Note that, bind() is
     * used on server side, not on client side*/
    if (bind(master_socket_tcp_file_descriptor, (struct sockaddr *)&server_addr, socket_address_length) == -1)
    {
        printf("Socket bind Failed\n");
        return;
    }


    /*listen() : Tell the Linux OS to maintain the queue of max length to Queue incoming client connections.*/
    /*master socket started listening where it have a queue of size 2 for client pending connections.
    It's not related to maximum client connection. Maximum client connection depends of system resources.
    For example sql server can have more then 10K active client.*/
    if (listen(master_socket_tcp_file_descriptor, 2) < 0)
    {
        printf("Server listen failed\n");
        return;
    }

    printf("Server is listening on port : %d\n", SERVER_PORT);

    /*Add master socket to Monitored set of FDs*/
    add_to_monitor_fd_set(master_socket_tcp_file_descriptor);

    while(1) {
        re_init_readfds(&readfds);

        printf("Server is waiting for any connected from a new client or data to process from existing client...\n");
        printf("Current total connected client : %d\n", get_total_connected_client());

        /*Call the select system cal, server process blocks here. Linux OS keeps this process blocked untill the data arrives on any of the file Drscriptors in the 'readfds' set*/
        select(get_max_fd() + 1, &readfds, NULL, NULL, NULL);

        /*If Data arrives on master socket FD, which means a new connection arrived to connect with server*/
        if (FD_ISSET(master_socket_tcp_file_descriptor, &readfds)){
            /*Data arrives on Master socket only when new client connects with the server (that is, 'connect' call is invoked on client side)*/
            printf("New connection recieved recvd, accept the connection. Client and Server completes TCP-3 way handshake at this point\n");

            /* accept() returns a new temporary file desriptor(fd). Server uses this 'client_socket_fd' fd for the rest of the
             * life of connection with this client to send and recieve msg. Master socket is used only for accepting
             * new client's connection and not for data exchange with the client*/
            int client_socket_fd = accept(master_socket_tcp_file_descriptor, (struct sockaddr *)&client_addr, &socket_address_length);
            
            if(client_socket_fd < 0){
                /* if accept failed to return a socket descriptor, display error and exit */
                printf("accept error : errno = %d\n", errno);
                exit(0);
            }

            add_to_monitor_fd_set(client_socket_fd);
            printf("Connection accepted from client : %s:%u\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        }
        else{
            int current_client_socket_fd = -1;

            for(int i = 0; i < MAX_CLIENT_SUPPORTED; i++) {
                if (FD_ISSET(monitor_fd_set[i], &readfds)) { /*Find the clinet FD on which Data has arrived*/
                    current_client_socket_fd = monitor_fd_set[i];

                    memset(data_buffer, 0, sizeof(data_buffer));

                    int received_bytes = recvfrom(current_client_socket_fd,
                    (char *)data_buffer,
                    sizeof(data_buffer), 0,
                    (struct sockaddr *)&client_addr, &socket_address_length);

                    printf("Server received %d bytes from client %s:%u\n", received_bytes,
                                            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    if (received_bytes == 0) {
                        close(current_client_socket_fd);
                        remove_from_monitor_fd_set(current_client_socket_fd);
                        break;
                    }

                    input_struct_t *client_data = (input_struct_t *)data_buffer;

                    if (client_data->a == 0 && client_data->b == 0) {
                        close(current_client_socket_fd);
                        remove_from_monitor_fd_set(current_client_socket_fd);
                        printf("Server closes connection with client : %s:%u\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                        break;
                    }

                    result_struct_t result;
                    result.c = client_data->a + client_data->b;

                    int sent_recv_bytes = sendto(current_client_socket_fd, (char *)&result, sizeof(result_struct_t), 0,
                            (struct sockaddr *)&client_addr, socket_address_length);

                    printf("Server sent %d bytes in reply to client\n", sent_recv_bytes);
                }
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    setup_tcp_server_communication();

    return 0;
}
