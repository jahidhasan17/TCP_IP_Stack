#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include "../common.h"

#define DEST_PORT 2000
#define SERVER_IP_ADDRESS "127.0.0.1"

void setup_tcp_communication(){
    /*structure to store the server*/
    struct sockaddr_in server_addr;

    /*Ipv4 sockets, Other values are IPv6*/
    server_addr.sin_family = AF_INET;

    /*Client wants to send data to server process which is running on server machine, and listening on 
     * port on DEST_PORT, server IP address SERVER_IP_ADDRESS.
     * Inform client about which server to send data to : All we need is port number, and server ip address. Pls note that
     * there can be many processes running on the server listening on different no of ports, 
     * our client is interested in sending data to server process which is lisetning on PORT = DEST_PORT*/ 
    server_addr.sin_port = DEST_PORT;

    struct hostent *host = (struct hostent *)gethostbyname(SERVER_IP_ADDRESS);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr_list[0]);

    int socket_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    connect(socket_descriptor, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));

    input_struct_t client_data;
    result_struct_t result;

    PROMPT_USER:
        /*prompt the user to enter data*/
        printf("Enter a : ?\n");
        scanf("%u", &client_data.a);
        printf("Enter b : ?\n");
        scanf("%u", &client_data.b);

        socklen_t socket_address_length = sizeof(struct sockaddr);

        int sent_recv_bytes = sendto(socket_descriptor, 
           &client_data,
           sizeof(input_struct_t), 
           0, 
           (struct sockaddr *)&server_addr, 
           socket_address_length);

        printf("No of bytes sent = %d\n", sent_recv_bytes);

        /*recvfrom is a blocking system call, meaning the client program will not run past this point
        * untill the data arrives on the socket from server*/
        sent_recv_bytes =  recvfrom(socket_descriptor, (char *)&result, sizeof(result_struct_t), 0,
                        (struct sockaddr *)&server_addr, &socket_address_length);
        
        printf("No of bytes recvd = %d\n", sent_recv_bytes);
    
        printf("Result recvd = %u\n", result.c);

        goto PROMPT_USER;
}

int main(int argc, char const *argv[])
{
    setup_tcp_communication();

    return 0;
}
