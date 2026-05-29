#pragma once
#include "main.h"
//#include "Telnet.h"

//extern bool remote_input_mode;

class T2Input
{
public:

        typedef void (*InputCompleteCallback)(
            const char* text
        );

char str_buf[BUF_SIZE]; //---------------------------------

char line_buffer[256];  //----------------------------------
int line_length; //-----------------------------------------
int remote_length = 0; //-----------------------------------

	enum Input_mode{
		SMALL,
		FIRST_BIG,
		BIG,
		NUM,
	};

	enum State{
		MAIN,
		SECOND_CLICK,
		F_NUM,
		SET_MENU,
		CTRL,
		CTRL_SECOND_CLICK
	};

        //bool remote_input = true; //???????????????????????

	Input_mode cur_input_mode;
	int last_input_time;
	int last_imput_key;

	State state;
	
	VMUINT8* scr_buf;
	int layer_handle;
	
	int key_w, key_h;
	int keyboard_h;

	bool draw_kb;

	int current_key;

	VMINT16 squares[8][2];

        // line input callback
        InputCompleteCallback input_callback;

	int input_mode; // 0 -> output to telnet socket, 1 -> output to buffer
	int input_done;
//	char str_buf[BUF_SIZE];  // temp buf for storing input if input_mode is 1

	int get_keycode(int x, int y);

	void send_c(const char*str);

	void numpad_input(int keycode);

	void show_current_pressed_key();

	//void handle_penevt(VMINT event, VMINT x, VMINT y);
	void handle_keyevt(VMINT event, VMINT keycode);

	void draw_xy_char(int x, int y, const char*str);
	void draw_xy_str(int x, int y, const char*str);
	void draw_xy_str_color(int x, int y, unsigned short textcolor,  unsigned short backcolor, const char*str);

	void draw();
	void init();
        void set_input_callback(
           InputCompleteCallback cb
        );

        void free_buffer();

//        void flush_pending_input();

	~T2Input(void);

};

