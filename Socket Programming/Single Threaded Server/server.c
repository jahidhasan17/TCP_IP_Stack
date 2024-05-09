#include <stdio.h>
#include <stdlib.h>
#include "../common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <memory.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 2000


char data_buffer[1024];


void setup_tcp_server_communication()
{
    int master_socket_tcp_file_descriptor = 0;

    /*client specific communication socket file descriptor,
     * used for only data exchange/communication between client and server*/
    int client_socket_tcp_file_descriptor = 0;

    /*structure to store the server and client info*/
    struct sockaddr_in server_addr, client_addr;

    socklen_t socket_address_length = sizeof(struct sockaddr);

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

    /*master socket started listening where it have a queue of size 2 for client pending connections.
    It's not related to maximum client connection. Maximum client connection depends of system resources.
    For example sql server can have more then 10K active client.*/
    if (listen(master_socket_tcp_file_descriptor, 2) < 0)
    {
        printf("Server listen failed\n");
        return;
    }

    /*Server is waiting for client connection at the master socket.*/
    client_socket_tcp_file_descriptor = accept(master_socket_tcp_file_descriptor,
                                               (struct sockaddr *)&client_addr, &socket_address_length);

    if (client_socket_tcp_file_descriptor < 0)
    {
        /* if accept failed to return a socket descriptor, display error and exit */
        printf("accept error : errno = %d\n", errno);
    }

    printf("Connection accepted from client : %s:%u\n",
        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));


    /* Server infinite loop for servicing the client. */    
    while (1)
    {
        printf("Server is ready to service client messages.\n");

        /*Drain to store client info (ip and port) when data arrives from client, sometimes, server would want to find the identity of the client sending msgs*/
        memset(data_buffer, 0, sizeof(data_buffer));

        int received_bytes = recvfrom(client_socket_tcp_file_descriptor,
                    (char *)data_buffer,
                    sizeof(data_buffer), 0,
                    (struct sockaddr *)&client_addr, &socket_address_length);

        printf("Server received %d bytes from client %s:%u\n", received_bytes,
                                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (received_bytes == 0) {
            close(client_socket_tcp_file_descriptor);
            break;
        }

        input_struct_t *client_data = (input_struct_t *)data_buffer;

        if (client_data->a == 0 && client_data->b == 0) {
            close(client_socket_tcp_file_descriptor);
            printf("Server closes connection with client : %s:%u\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            break;
        }

        result_struct_t result;
        result.c = client_data->a + client_data->b;

        int sent_recv_bytes = sendto(client_socket_tcp_file_descriptor, (char *)&result, sizeof(result_struct_t), 0,
                (struct sockaddr *)&client_addr, socket_address_length);

        printf("Server sent %d bytes in reply to client\n", sent_recv_bytes);
    }
}

int main(int argc, char const *argv[])
{
    setup_tcp_server_communication();

    return 0;
}
