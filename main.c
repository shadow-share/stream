#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEBUG 1
#include "stream.h"

Packet packet = NULL;

int main() {
    packet = packet_create();

    Performance_Start(33333333)
    packet_put_uint8(packet, 0x12);
    Performance_End;

    printf("Packet size = %u\n", packet_get_size(packet));
    packet_free(&packet);
    return 0;
}
