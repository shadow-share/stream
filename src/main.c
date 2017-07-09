#include <stdio.h>
#include <time.h>

#define DEBUG 1
#include "stream.h"


int main() {
    Packet packet = packet_create();
    stream_init();

    Performance_Start(1)
    for (int i = 0; i < 99999999; ++i) {
        packet_put_uint8(packet, 0x00);
    }

    for (int i = 0; i < 99999999; ++i) {
        packet_put_uint8(packet, 0xff);
    }

    for (int i = 0; i < 99999999; ++i) {
        packet_pop_uint8(packet);
        packet_shift_uint8(packet);
//        if (packet_pop_uint8(packet) != 0xff) {
//            ERROR_EXIT("pop item invalid");
//        }
//
//        if (packet_shift_uint8(packet) != 0x00) {
//            ERROR_EXIT("shift item invalid");
//        }
    }
    Performance_End;


    printf("\nNodeSize: %d\nDataSize: %d\n", packet_get_node_size(packet),
           packet_get_data_size(packet));
    packet_free(&packet);
    return 0;
}
