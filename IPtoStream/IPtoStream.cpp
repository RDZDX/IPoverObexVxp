//#include <vmsys.h>
//#include <vmio.h>
//#include <vmstdlib.h>
//#include <vmchset.h>
//#include <stdio.h>
//#include <stdarg.h>
//#include <stdlib.h>
//#include <string.h>

#include "IPtoStream.h"
#include <opp.h>
#include <cstdio>
//#include <console.h>

#undef ipts

IPtoStream ipts;

void create_app_txt_filenamex(VMWSTR text, VMSTR extt);
void log_debug(const char* fmt, ...);

void IPtoStream::init() {
	bt_opp_preinit();
	bt_opp_init();
}

void IPtoStream::connectBT(uint8_t mac[6]) {
	bt_opp_connect(mac);
}

size_t IPtoStream::write(const void* buf, size_t size) {

//    char dbg[64];
//    sprintf(dbg, "\nIPTS WRITE %d\n", (int)size);
//    console_str_in(dbg);
//log_debug(dbg);

	return (size_t)bt_opp_write(buf, (VMUINT32)size);
}

size_t IPtoStream::read(void* buf, size_t size) {
	VMUINT32 avail = bt_opp_get_receive_size();
	if (avail == 0) return 0;
	if ((VMUINT32)size > avail) size = (size_t)avail;
	const void* src = bt_opp_get_receive_buf();
	if (!src) return 0;
	for (size_t i = 0; i < size; i++)
		((char*)buf)[i] = ((const char*)src)[i];
	bt_opp_set_as_readed((VMUINT32)size);
	return size;
}

bool IPtoStream::is_connected() {
	return bt_opp_is_connected() != 0;
}

void IPtoStream::update() {
	bt_opp_flush();
}

void IPtoStream::disconnect() {
	bt_opp_disconnect();
}

//void IPtoStream::disconnect() {
//    bt_opp_disconnect();

//    for(int i = 0; i < 20; i++)
//    {
//        bt_opp_flush();
//    }
//}

void IPtoStream::quit() {
	bt_opp_disconnect();
	bt_opp_deinit();
}

//void create_app_txt_filenamex(VMWSTR text, VMSTR extt) {

//VMINT drv;
//VMWCHAR fullPath[100] = {0};
//VMWCHAR appName[100] = {0};
//VMWCHAR wfile_extension[8] = {0};
//VMCHAR fAutoFileName[100] = {0};
//VMWCHAR wAutoFileName[100] = {0};
//VMWCHAR wProduct[100] = {0};

//vm_ascii_to_ucs2(wfile_extension, 8, extt); // txt

//if ((drv = vm_get_removable_driver()) < 0) {
//   drv = vm_get_system_driver();
//}

//sprintf(fAutoFileName, "%c:\\", drv);
//vm_ascii_to_ucs2(wProduct, (strlen(fAutoFileName) + 1) * 2, fAutoFileName); // e:\

//vm_get_exec_filename(fullPath); // e:\home\program.vxp
//vm_get_filename(fullPath, appName); // program.vxp

//vm_wstrncpy(wAutoFileName, appName, vm_wstrlen(appName) - 3); //program.
//vm_wstrcat(wAutoFileName, wfile_extension); // program. + txt
//vm_wstrcat(wProduct, wAutoFileName); //e:\ + program.txt
//vm_wstrcpy(text, wProduct);

//}

//void log_debug(const char* fmt, ...)
//{
//    VMWCHAR file_pathw[256];
//    create_app_txt_filenamex(file_pathw, "txt");

//    VMFILE f = vm_file_open(file_pathw, MODE_APPEND, FALSE);
//    if (f < 0)
//        f = vm_file_open(file_pathw, MODE_CREATE_ALWAYS_WRITE, FALSE);

//    if (f < 0)
//        return;

//    char buf[256];

//    va_list ap;
//    va_start(ap, fmt);
//    vsnprintf(buf, sizeof(buf), fmt, ap);
//    va_end(ap);

//    VMUINT nwrite;
//    vm_file_write(f, buf, strlen(buf), &nwrite);

//    vm_file_close(f);
//}
