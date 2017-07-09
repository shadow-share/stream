#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>

#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

//#include <Python.h>
#include "stream.h"


/* Global Variables */
bool g_stream_init = false;


/* Static Typedef */
typedef int Sock;


/* Static Methods */
Sock socket_create(const char *host, const int port);


/* Common Macro */
#define DE_PTR(Ptr)                 (*(Ptr))
#define ZERO_INIT(Ptr, Size)        (memset(Ptr, 0, Size))
#define UINT8(value)                ((uint8_t)(value))


/* Static ByteNode Structure */
#define BYTE_NODE_DATA_SIZE         4099
typedef struct _byte_node_t {
    uint8_t values[BYTE_NODE_DATA_SIZE]; // 1-byte
    uint16_t header_index;               // 2-bytes
    uint16_t current_index;              // 2-bytes
    struct _byte_node_t *prev;           // 4-bytes
    struct _byte_node_t *next;           // 4-bytes // 15-bytes
} byte_node_t;
typedef byte_node_t *ByteNode;
/* ByteNode methods */
static ByteNode byte_node_create(void);
static bool byte_node_put(ByteNode node, uint8_t value);
static uint8_t byte_node_pop(ByteNode node);
static uint8_t byte_node_shift(ByteNode node);


/* Static ByteArray Structure */
typedef struct _byte_array_t {
    ByteNode header;
    ByteNode footer;
    uint32_t node_size;
} byte_array_t;
typedef byte_array_t *ByteArray;
/* Static Methods for ByteArray */
static ByteArray byte_array_create(void);
static void byte_array_put(ByteArray byte_array, const uint8_t value);
static uint8_t byte_array_pop(ByteArray byte_array);
static uint8_t byte_array_shift(ByteArray byte_array);
static void byte_array_free(ByteArray *byte_array);


/* Packet Structure*/
struct _packet_t {
    ByteArray byte_array;
    uint32_t packet_size;
};


/* Packet Methods */
Packet packet_create() {
    Packet packet = (packet_t*)malloc(sizeof(packet_t));
    packet->byte_array = byte_array_create();
    packet->packet_size = 0;

#ifdef DEBUG
    Dbg("create new Packet Instance, address = %#p", packet);
#endif

    return packet;
}

void packet_put_uint8(Packet packet, const uint8_t value) {
    byte_array_put(packet->byte_array, value);
    packet->packet_size += 1;
}

void packet_put_uint16(Packet packet, const uint16_t value) {
    packet_put_uint8(packet, (uint8_t)((value & 0xff00) >> 8));
    packet_put_uint8(packet, (uint8_t)(value & 0xff));
}

void packet_put_uint32(Packet packet, const uint32_t value) {
    packet_put_uint16(packet, (uint16_t)((value & 0xffff0000) >> 16));
    packet_put_uint16(packet, (uint16_t)(value & 0xffff));
}

void packet_put_uint64(Packet packet, const uint64_t value) {
    packet_put_uint32(
            packet, (uint32_t)((value & (uint64_t)0xffffffff00000000) >> 32));
    packet_put_uint32(packet, (uint32_t)(value & (uint64_t)0xffffffff));
}

void packet_put_string(Packet packet, const char *string) {
    for (int i = 0, e = strlen(string); i < e; ++i) {
        packet_put_uint8(packet, (uint8_t)string[i]);
    }
}

uint8_t packet_pop_uint8(Packet packet) {
    uint8_t ret = byte_array_pop(packet->byte_array);

    --packet->packet_size;
    return ret;
}

uint8_t packet_shift_uint8(Packet packet) {
    uint8_t ret = byte_array_shift(packet->byte_array);

    --packet->packet_size;
    return ret;
}

uint16_t packet_pop_uint16(Packet packet) {
    uint16_t low, hgh;

    low = packet_pop_uint8(packet);
    hgh = packet_pop_uint8(packet);

    return (hgh << 8) | low;
}

uint16_t packet_shift_uint16(Packet packet) {
    uint16_t hgh, low;

    hgh = packet_shift_uint8(packet);
    low = packet_shift_uint8(packet);

    return (hgh << 8) | low;
}

uint32_t packet_pop_uint32(Packet packet) {
    uint32_t low, hgh;

    low = packet_pop_uint16(packet);
    hgh = packet_pop_uint16(packet);

    return (hgh << 16) | low;
}

uint32_t packet_shift_uint32(Packet packet) {
    uint32_t hgh, low;

    hgh = packet_shift_uint16(packet);
    low = packet_shift_uint16(packet);

    return (hgh << 16) | low;
}

uint64_t packet_pop_uint64(Packet packet) {
    uint64_t low, hgh;

    low = packet_pop_uint32(packet);
    hgh = packet_pop_uint32(packet);

    return (hgh << 32) | low;
}

uint64_t packet_shift_uint64(Packet packet) {
    uint64_t hgh, low;

    hgh = packet_shift_uint32(packet);
    low = packet_shift_uint32(packet);

    return (hgh << 32) | low;
}

void packet_free(Packet *ptr_packet) {
    byte_array_free(&(DE_PTR(ptr_packet)->byte_array));

    free(DE_PTR(ptr_packet));
    DE_PTR(ptr_packet) = NULL;
}

unsigned packet_get_node_size(const Packet packet) {
    return packet->byte_array->node_size;
}

unsigned packet_get_data_size(const Packet packet) {
    return packet->packet_size;
}


/* Methods of Stream */
void stream_init(void) {
    extern bool g_stream_init;

#ifdef _WIN32
    WSADATA win_sock;
    if (WSAStartup(MAKEWORD(2, 2), &win_sock) != 0) {
        ERROR_EXIT("Windows Socket environment initializing failure");
    }
#else
    // cannot do anything on *nix
#endif

    g_stream_init = true;
}


/* Static Methods for ByteNode */
static ByteNode byte_node_create(void) {
    byte_node_t *byte_node = (byte_node_t*)malloc(sizeof(byte_node_t));
    ZERO_INIT(byte_node, sizeof(byte_node_t));

    return byte_node;
}

static bool byte_node_put(ByteNode node, uint8_t value) {
    if (node->current_index == BYTE_NODE_DATA_SIZE) {
        return false;
    }

    // reset node state
    if (node->header_index == node->current_index) {
        node->header_index = 0;
        node->current_index = 0;
    }

    node->values[node->current_index++] = UINT8(value);
    return true;
}

static uint8_t byte_node_pop(ByteNode node) {
    if (node->current_index == node->header_index) {
        ERROR_EXIT("cannot pop item from empty ByteNode");
    }
    return node->values[--node->current_index];
}

static uint8_t byte_node_shift(ByteNode node) {
    if (node->header_index == node->current_index) {
        ERROR_EXIT("cannot shift item from empty ByteNode");
    }
    return node->values[node->header_index++];
}


/* Static Methods for ByteArray */
static ByteArray byte_array_create(void) {
    byte_array_t *byte_array = (byte_array_t*)malloc(sizeof(byte_array_t));
    ZERO_INIT(byte_array, sizeof(byte_array_t));

    return byte_array;
}

static void byte_array_put(ByteArray byte_array, const uint8_t value) {
    if (byte_array->header == NULL) {
        byte_array->header = byte_node_create();
        byte_array->footer = byte_array->header;
        byte_array->node_size = 1;

        Dbg("initial ByteNode, address is %#p", byte_array->header);
    }

    if (!byte_node_put(byte_array->footer, value)) {
        ByteNode new_node = byte_node_create();
        Dbg("last node[%d] is full, address = %#p",
            byte_array->node_size - 1, byte_array->footer);
        new_node->prev = byte_array->footer;

        byte_array->footer->next = new_node;
        byte_array->footer = byte_array->footer->next;
        ++byte_array->node_size;
        if (!byte_node_put(byte_array->footer, value)) {
            ERROR_EXIT("cannot create new ByteNode");
        }
    }
}

static uint8_t byte_array_pop(ByteArray byte_array) {
    ByteNode footer = byte_array->footer;
    // check footer-node is empty
    if (footer->current_index == footer->header_index) {
        if (byte_array->node_size == 1) {
            // cannot pop item from empty ByteArray
            ERROR_EXIT("cannot pop item from empty ByteArray");
        }
        // update byte-array footer
        byte_array->footer = byte_array->footer->prev;
        // update footer next pointer to NULL
        byte_array->footer->next = NULL;
        // free empty-node
        Dbg("the last empty node will be free, address = %#p", footer);
        free(footer);
        // re-assign footer
        footer = byte_array->footer;
        // update node-size
        --byte_array->node_size;
    }
    // return last Byte
    return byte_node_pop(footer);
}

static uint8_t byte_array_shift(ByteArray byte_array) {
    ByteNode header = byte_array->header;
    // check header-node is empty
    if (header->header_index == header->current_index) {
        if (byte_array->node_size == 1) {
            // single node and empty
            ERROR_EXIT("cannot pop item from empty ByteArray");
        }
        // update byte-array state
        byte_array->header = byte_array->header->next;
        // update header `prev` pointer
        byte_array->header->prev = NULL;
        // free empty header-node
        Dbg("the first empty node will be free, address = %#p", header);
        free(header);
        // re-assign header
        header = byte_array->header;
        // update node-size
        --byte_array->node_size;
    }
    // return value
    return byte_node_shift(header);
}

static void byte_array_free(ByteArray *byte_array) {
    ByteNode curr = DE_PTR(byte_array)->header;
    for (ByteNode next = NULL; curr != NULL; curr = next) {
        next = curr->next;
        free(curr);
    }

    free(DE_PTR(byte_array));
    DE_PTR(byte_array) = NULL;
}


/* Static Methods */
Sock socket_create(const char *host, const int port) {
    Sock sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        ERROR_EXIT("cannot create socket");
    }

    return sock;
}
