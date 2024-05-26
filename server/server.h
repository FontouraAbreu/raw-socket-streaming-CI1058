#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

struct kermit_frame_t {
    unsigned start;
    unsigned size;
    unsigned seq;
    unsigned type;
    unsigned data[64];
    unsigned crc_8;
} kermit_frame_t;
    


#endif // __SERVER_H__
