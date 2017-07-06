#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEBUG 1
#include "stream.h"

Packet packet = NULL;

int main() {
    packet = packet_create();

    Performance_Start(999999)
        packet_put_uint8(packet, 0x12);
    Performance_End;

    printf("Packet node_size = %u, data size = %u\n",
           packet_get_node_size(packet),
           packet_get_size(packet));
    getc(stdin);
    getc(stdin);
    packet_free(&packet);
    getc(stdin);
    getc(stdin);
    getc(stdin);
    getc(stdin);
    return 0;
}
