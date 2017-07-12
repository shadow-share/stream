#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEBUG 1
#include "stream.h"


int main() {
    stream_init();

    Packet packet = packet_create();
    for (int i = 0; i < 2048; ++i) {
        packet_put_string(packet, "abc\n");
    }
    printf("NodeSize: %d\nDataSize: %d\n", packet_get_node_size(packet), packet_get_data_size(packet));

    char *cat_string = packet_cat_string(packet, 4104);
    puts(cat_string);
    free(cat_string);

    packet_free(&packet);
    return 0;
}
