#ifndef STREAM_STREAM_H
#define STREAM_STREAM_H

#include <time.h>
#include <inttypes.h>

/* Debug Utility Macro */
#ifdef DEBUG
#define Dbg(Fmt, Arg...) do {\
        fprintf(stderr, "[DEBUG] %24s:%-4u %s: " Fmt "\n",\
            __FILE__, __LINE__,\
			__PRETTY_FUNCTION__, ##Arg\
        );\
    } while (0)
#else
#define Dbg(Fmt, Arg...) do {\
    } while (0)
#endif

/* Performance Testing */
#ifdef DEBUG
    #define Performance_Start(times) do {\
        double __ts[3] = {0};\
        clock_t __s, __e = 0;\
        for (int __i = 0; __i < 3; ++__i) {\
            __s = clock();\
            for (unsigned long __j = 0UL; __j < times##UL; ++__j) {
    #define Performance_End \
            }\
            __e = clock();\
            __ts[__i] = (__e - __s) / (double)CLOCKS_PER_SEC;\
            Dbg("%d running, time = %lf", __i + 1, __ts[__i]);\
        }\
        Dbg("Running for 3 times, average time = %lf", (__ts[0] + __ts[1] + __ts[2]) / 3.);\
    } while (0)
#else
    #define Performance_Start(times)
    #define Performance_End
#endif


struct _packet_t;
typedef struct _packet_t packet_t;
typedef packet_t *Packet;

/* Packet Methods */
Packet packet_create();
void packet_put_uint8(Packet packet, const uint8_t value);
unsigned packet_get_size(const Packet packet);
void packet_free(Packet *ptr_packet);

#endif


