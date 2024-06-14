#include "utils.h"
#include <sys/socket.h>
#include <arpa/inet.h>

/*Apply mask on prefix, and store result in 'str_prefix'
 *For eg : prefix = 122.1.1.1, mask 24, then str_prefix
  will store 122.1.1.0
 * */
void apply_mask(char *prefix, char mask, char *str_prefix) {
    unsigned int binary_prefix = 0, i = 0;
    inet_pton(AF_INET, prefix, &binary_prefix);
    binary_prefix = htonl(binary_prefix);
    for(; i < (32 - mask); i++)
        UNSET_BIT(binary_prefix, i);
    binary_prefix = htonl(binary_prefix);
    inet_ntop(AF_INET, &binary_prefix, str_prefix, 16);
    str_prefix[15] = '\0';
}


void layer2_fill_with_broadcast_mac(char *mac_array) {
    mac_array[0] = 0xFF;
    mac_array[1] = 0xFF;
    mac_array[2] = 0xFF;
    mac_array[3] = 0xFF;
    mac_array[4] = 0xFF;
    mac_array[5] = 0xFF;
}