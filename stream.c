#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/* Common Macro */
#define De_Ptr(Ptr) (*(Ptr))


/* Static ByteNode Structure */
typedef struct _byte_node_t {
    uint8_t value; // 1-byte
    struct _byte_node_t *next; // 4(8)-bytes
} byte_node_t;
typedef byte_node_t *ByteNode;
// Create new ByteNode
#define New_ByteNode(value) ({\
    ByteNode _byte_node = (byte_node_t*)malloc(sizeof(byte_node_t));\
    _byte_node->value = ((uint8_t)(value) & 0xff);\
    _byte_node->next = NULL;\
    _byte_node;\
})

/* Static ByteArray Structure */
typedef struct _byte_array_t {
    ByteNode header;
    ByteNode *footer;
} byte_array_t;
typedef byte_array_t *ByteArray;
// Create New ByteArray
#define New_ByteArray() ({\
    ByteArray array = (byte_array_t*)malloc(sizeof(byte_array_t));\
    array->header = NULL;\
    array->footer = NULL;\
    array;\
})
/* Static Methods for ByteArray */
static void byte_array_put(ByteArray byte_array, const uint8_t value);
static void byte_array_free(ByteArray *byte_array);



/* Packet Structure*/
struct _packet_t {
    ByteArray byte_array;
    uint32_t packet_size;
};
// Create New Packet
#define New_Packet() ({\
    Packet packet = (packet_t*)malloc(sizeof(packet_t));\
    packet->byte_array = New_ByteArray();\
    packet->packet_size = 0;\
    packet;\
})


/* Packet Methods */
Packet packet_create() {
#ifdef DEBUG
    Packet packet = New_Packet();
    Dbg("Create an address is %#p Packet object", packet);
    return packet;
#else
    return New_Packet();
#endif
}

void packet_put_uint8(Packet packet, const uint8_t value) {
    byte_array_put(packet->byte_array, value);
    packet->packet_size += 1;
}

void packet_free(Packet *ptr_packet) {
    byte_array_free(&(De_Ptr(ptr_packet)->byte_array));
    free(De_Ptr(ptr_packet));
    De_Ptr(ptr_packet) = NULL;
}

/* Static Methods for ByteArray */
static void byte_array_put(ByteArray byte_array, const uint8_t value) {
    if (byte_array->header == NULL) {
        byte_array->header = New_ByteNode(value);
        byte_array->footer = &(byte_array->header->next);
        return;
    }
    De_Ptr(byte_array->footer) = New_ByteNode(value);
    byte_array->footer = &(De_Ptr(byte_array->footer)->next);
}

unsigned packet_get_size(const Packet packet) {
    return packet->packet_size;
}

static void byte_array_free(ByteArray *byte_array) {
    ByteNode curr = De_Ptr(byte_array)->header;
    for (ByteNode next; curr != NULL; curr = next) {
        next = curr->next;
        free(curr);
    }

    De_Ptr(byte_array) = NULL;
}
