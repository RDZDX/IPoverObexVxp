#pragma once
#include "vmsys.h"
#include "vmio.h"
#include "vmgraph.h"
#include "vmchset.h"
#include "vmstdlib.h"
//#include "ResID.h"
#include "string.h"
#include <ctype.h>
#include "vmmm.h"
#include <IPtoStream.h>

#define BUF_SIZE 253 // Buffer size for output mode 1, set to 253 if you want to connect to longest domain name :)

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Error
};

extern bool remote_mode;

extern IPtoStream ipts;

extern int tcp_id;
extern int tcpl_id;

extern bool connected;
extern bool connected1;

//extern bool socket_output;

extern ConnectionState connState;





extern int scr_w, scr_h;
extern VMUINT8 *layer_bufs[2];
extern VMINT layer_hdls[2];

const unsigned short tr_color = VM_COLOR_888_TO_565(0, 255, 255);

#ifdef WIN32
#define my_printf(...) printf(__VA_ARGS__)
#else
#define my_printf(...)
#endif
