#include "IPtoStream.h"
#include <opp.h>

#undef ipts

IPtoStream ipts;

void IPtoStream::init() {
	bt_opp_preinit();
	bt_opp_init();
}

void IPtoStream::connectBT(uint8_t mac[6]) {
	bt_opp_connect(mac);
}

size_t IPtoStream::write(const void* buf, size_t size) {
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

void IPtoStream::quit() {
	bt_opp_disconnect();
	bt_opp_deinit();
}
