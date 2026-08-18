#pragma once
#include <stdint.h>
#include <stddef.h>

class IPtoStream {
public:
	void init();
	void connectBT(uint8_t mac[6]);

	size_t write(const void* buf, size_t size);
	size_t read(void* buf, size_t size);

	bool is_connected();
	void update();
	void disconnect();
	void quit();
};

extern IPtoStream ipts;