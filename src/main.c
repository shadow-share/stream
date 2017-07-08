#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEBUG 1
#include "stream.h"

Packet packet = NULL;

int main() {
    stream_init();

    packet = packet_create();
    packet_put_uint8(packet, 0xff);
    packet_put_uint16(packet, 0x0102);
    packet_put_uint32(packet, 0x01020304);
    packet_put_uint64(packet, 0x1122334455667788ULL);
    packet_put_string(packet, "GET / HTTP/1.1\r\n\r\n");
    packet_put_uint64(packet, 0x1122334455667788ULL);
    packet_put_uint32(packet, 0x01020304);
    packet_put_uint16(packet, 0x0102);
    packet_put_uint8(packet, 0xff);

    printf("Packet node_size = %u, data size = %u\n",
           packet_get_node_size(packet),
           packet_get_size(packet));

    uint8_t pop_1 = packet_pop_uint8(packet);
    printf("pop_uint8 = %#x\n", pop_1);

    uint16_t pop_2 = packet_pop_uint16(packet);
    printf("pop_uint16 = %#x\n", pop_2);

    uint32_t pop_3 = packet_pop_uint32(packet);
    printf("pop_uint32 = %#x\n", pop_3);

    uint64_t pop_4 = packet_pop_uint64(packet);
    uint32_t low, hgh;
    low = (uint32_t)(pop_4 & 0xffffffff);
    hgh = (uint32_t)(pop_4 >> 32);
    printf("pop_uint64 = %#x %#x\n", hgh, low);

    uint8_t shift_1 = packet_shift_uint8(packet);
    printf("shift_uint8 = %#x\n", shift_1);

    uint16_t shift_2 = packet_shift_uint16(packet);
    printf("shift_uint16 = %#x\n", shift_2);

    uint32_t shift_3 = packet_shift_uint32(packet);
    printf("shift_uint32 = %#x\n", shift_3);

    uint64_t shift_4 = packet_shift_uint64(packet);
    low = (uint32_t)(shift_4 & 0xffffffff);
    hgh = (uint32_t)(shift_4 >> 32);
    printf("shift_uint64 = %#x %#x\n", hgh, low);

    packet_free(&packet);
    return 0;
}
