#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>


//#define DEBUG 1

#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
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
#define DE_PTR(Ptr)                 (*(Ptr))
#define ZERO_INIT(Ptr, Size)        (memset(Ptr, 0, Size))


/* Static ByteNode Structure */
#define BYTE_NODE_DATA_SIZE         4084
typedef struct _byte_node_t {
    uint8_t values[BYTE_NODE_DATA_SIZE];
    uint16_t header_index;               // 2-bytes
    uint16_t current_index;              // 2-bytes
    struct _byte_node_t *prev;           // 4-bytes
    struct _byte_node_t *next;           // 4-bytes // 12-bytes
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
};



/* Stream Structure */
struct _socket_stream_t {
    Packet buffer;
    char *host;
    char *ip_address;
    Sock sock_fd;
    unsigned short port;
};

struct _string_stream_t {
    Packet buffer;
};


/* Static Socket Methods */
static Sock socket_create();
static void socket_connect(Sock sock, const char *name, int port);
static struct sockaddr_storage socket_get_addr(const char *name, int port);


/* Packet Methods */
Packet packet_create() {
    Packet packet = (packet_t*)malloc(sizeof(packet_t));
    packet->byte_array = byte_array_create();

#ifdef DEBUG
    Dbg("create new Packet Instance, address = %#p", packet);
#endif

    return packet;
}

void packet_put_uint8(Packet packet, const uint8_t value) {
    byte_array_put(packet->byte_array, value);
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
    return byte_array_pop(packet->byte_array);
}

uint8_t packet_shift_uint8(Packet packet) {
    return byte_array_shift(packet->byte_array);
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
    unsigned node_size = 0;
    for (ByteNode node = packet->byte_array->header; node != NULL; node = node->next) {
        node_size += 1;
    }
    return node_size;
}

unsigned packet_get_data_size(const Packet packet) {
    unsigned packet_size = 0;

    for (ByteNode node = packet->byte_array->header; node != NULL; node = node->next) {
        packet_size += node->current_index - node->header_index;
    }

    return packet_size;
}


/* Methods of Stream */
void stream_init(void) {
    static bool g_stream_init = false;

    if (g_stream_init == false) {
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
}

SocketStream socket_stream_create(const char *host, unsigned short port) {
    SocketStream stream = (socket_stream_t*)malloc(sizeof(socket_stream_t));
    stream->buffer = packet_create();

    stream->host = (char*)malloc(strlen(host) * sizeof(char));
    strncpy(stream->host, host, strlen(host));

    stream->port = port; // short -> 2-bytes -> 65535
    stream->ip_address = NULL;
    stream->sock_fd = 0;

    return stream;
}

StringStream string_stream_create(const char *string) {
    StringStream stream = (string_stream_t*)malloc(sizeof(string_stream_t));
    stream->buffer = packet_create();

    // initializing buffer
    packet_put_string(stream->buffer, string);

    return stream;
}


/* Static Methods for ByteNode */
static ByteNode byte_node_create(void) {
    byte_node_t *byte_node = (byte_node_t*)malloc(sizeof(byte_node_t));
    byte_node->header_index = 0;
    byte_node->current_index = 0;
    byte_node->prev = NULL;
    byte_node->next = NULL;

    return byte_node;
}

static bool byte_node_put(ByteNode node, uint8_t value) {
    if (node->current_index == BYTE_NODE_DATA_SIZE) {
        return false;
    }

    node->values[node->current_index++] = value;
    return true;
}

static uint8_t byte_node_pop(ByteNode node) {
    return node->values[--node->current_index];
}

static uint8_t byte_node_shift(ByteNode node) {
    return node->values[node->header_index++];
}


/* Static Methods for ByteArray */
static ByteArray byte_array_create(void) {
    byte_array_t *byte_array = (byte_array_t*)malloc(sizeof(byte_array_t));
    ZERO_INIT(byte_array, sizeof(byte_array_t));

//    byte_array->header = byte_node_create();
//    byte_array->footer = byte_array->header;
//    Dbg("initial ByteNode, address is %#p", byte_array->header);

    return byte_array;
}

static void byte_array_put(ByteArray byte_array, const uint8_t value) {
    if (byte_array->header == NULL) {
        byte_array->header = byte_node_create();
        byte_array->footer = byte_array->header;
        Dbg("initial ByteNode, address is %#p", byte_array->header);
    }

    if (!byte_node_put(byte_array->footer, value)) {
        ByteNode new_node = byte_node_create();

        byte_array->footer->next = new_node;
        new_node->prev = byte_array->footer;
        byte_array->footer = byte_array->footer->next;

        byte_node_put(byte_array->footer, value); // no qa
    }
}

static uint8_t byte_array_pop(ByteArray byte_array) {
    ByteNode footer = byte_array->footer;
    // check footer-node is empty
    if (footer->current_index == footer->header_index) {
        if (byte_array->header == byte_array->footer) {
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
    }
    // return last Byte
    return byte_node_pop(footer);
}

static uint8_t byte_array_shift(ByteArray byte_array) {
    ByteNode header = byte_array->header;
    // check header-node is empty
    if (header->header_index == header->current_index) {
        if (byte_array->header == byte_array->footer) {
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


/* Static Socket Methods */
static Sock socket_create(void) {
    Sock sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        ERROR_EXIT("cannot create socket");
    }

    return sock;
}

static void socket_connect(Sock sock, const char *name, int port) {
    struct sockaddr_storage addr = socket_get_addr(name, port);
    connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr));
}

static struct sockaddr_storage socket_get_addr(const char *name, int port) {
    union {
        struct sockaddr_storage ss;
        struct sockaddr_in s4;
        struct sockaddr_in6 s6;
    } sock_addr;
    ZERO_INIT(&sock_addr, sizeof(sock_addr));

    struct hostent *he = gethostbyname(name);
    if (he == NULL) {
        ERROR_EXIT("cannot resolve %s", name);
    }

    if (he->h_addrtype == AF_INET) {
        sock_addr.s4.sin_family = AF_INET;
        memcpy(&sock_addr.s4.sin_addr, he->h_addr_list[0], (size_t)he->h_length);
    } else if (he->h_addrtype == AF_INET6){
        sock_addr.s6.sin6_family = AF_INET6;
        memcpy(&sock_addr.s6.sin6_addr, he->h_addr_list[0], (size_t)he->h_length);
    } else {
        ERROR_EXIT("undefined sock_addr type for %d", he->h_addrtype);
    }

    sock_addr.s4.sin_port = htons((unsigned short)port);
    return sock_addr.ss;
}
