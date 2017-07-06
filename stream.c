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


/* Common Macro */
#define DE_PTR(Ptr)                 (*(Ptr))
#define PTR_SIZE                    (sizeof(void*))
#define ZERO_INIT(Ptr, Size)        (memset(Ptr, 0, Size))
#define UINT8(value)                ((uint8_t)(value))

/* Error Message */
#define ERROR_EXIT(Fmt, Arg...) do {\
    fprintf(stderr, "[ERROR] %24s:%-4u %s: " Fmt "\n",\
        __FILE__, __LINE__,\
        __PRETTY_FUNCTION__, ##Arg\
    );\
    exit(-1);\
} while (0)



/* Static ByteNode Structure */
#define BYTE_NODE_SIZE 4096
#define BYTE_NODE_DATA_SIZE BYTE_NODE_SIZE - PTR_SIZE - 4
typedef struct _byte_node_t {
    uint8_t values[BYTE_NODE_DATA_SIZE];
    uint16_t header_index; // 2-Byte
    uint16_t data_size; // 2-Byte
    struct _byte_node_t *next; // 4(8)-bytes
} byte_node_t;
typedef byte_node_t *ByteNode;
// Create new ByteNode
#define New_ByteNode() ({\
    ByteNode _byte_node = (byte_node_t*)malloc(sizeof(byte_node_t));\
    ZERO_INIT(_byte_node, sizeof(byte_node_t));\
    _byte_node->header_index = 0;\
    _byte_node->data_size = 0;\
    _byte_node;\
})
/* ByteNode methods */
static bool byte_node_put(ByteNode node, uint8_t value);


/* Static ByteArray Structure */
typedef struct _byte_array_t {
    ByteNode header;
    ByteNode *footer;
    uint32_t node_size;
} byte_array_t;
typedef byte_array_t *ByteArray;
// Create New ByteArray
#define New_ByteArray() ({\
    ByteArray _array = (byte_array_t*)malloc(sizeof(byte_array_t));\
    _array->header = NULL;\
    _array->footer = NULL;\
    _array->node_size = 0;\
    _array;\
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
    byte_array_free(&(DE_PTR(ptr_packet)->byte_array));

    free(DE_PTR(ptr_packet));
    DE_PTR(ptr_packet) = NULL;
}

unsigned packet_get_node_size(const Packet packet) {
    return packet->byte_array->node_size;
}

unsigned packet_get_size(const Packet packet) {
    return packet->packet_size;
}


/* Methods of Stream */
void stream_init(void) {
#ifdef _WIN32
    WSADATA win_sock;

    if (WSAStartup(MAKEWORD(2, 2), &win_sock) != 0) {
        ERROR_EXIT("Windows Socket environment initializing failure");
    }
#else
    // In Linux cannot do anything
#endif

    g_stream_init = true;
}


/* Static Methods for ByteNode */
static bool byte_node_put(ByteNode node, uint8_t value) {
    if (node->data_size == BYTE_NODE_DATA_SIZE) {
        return false;
    }
    node->values[node->header_index++] = UINT8(value);
    ++node->data_size;
    return true;
}


/* Static Methods for ByteArray */
static void byte_array_put(ByteArray byte_array, const uint8_t value) {
    if (byte_array->header == NULL) {
        byte_array->header = New_ByteNode();
        byte_array->footer = &(byte_array->header);
        byte_array->node_size = 1;
    }

    if (!byte_node_put(DE_PTR(byte_array->footer), value)) {
        Dbg("The %d node is full, alloc new node", byte_array->node_size);

        DE_PTR(byte_array->footer)->next = New_ByteNode();
        byte_array->footer = &(DE_PTR(byte_array->footer)->next);
        ++byte_array->node_size;
        if (!byte_node_put(DE_PTR(byte_array->footer), value)) {
            ERROR_EXIT("cannot create new ByteNode");
        }
    }
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
