#include "Console_io.h"
#include <IPtoStream.h>
#include "main.h"

extern Console console;
//extern Telnet telnet;
extern int myScroll;
extern int myScrollx;
extern T2Input t2input;

extern bool socket_output;
//extern int tcp_id;
//extern IPtoStream ipts;
//extern ConnectionState connState;
//extern bool connected1;


extern "C" void console_char_in(char ch){
	console.put_c(ch);
}

extern "C" void console_str_in(const char* str){
	console.putstr(str);
//        console.putstr(str, sizeof(str));
}

extern "C" void console_str_with_length_in(const char* str, int length){
	console.putstr(str, length);
}

//extern "C" void console_char_out(char ch){
//    if (t2input.input_mode == 0) {
//        // Output to telnet socket
//        char temp_str[2] = {ch, 0};
//        //telnet.send_data(temp_str);

//        ipts.tcp.write(tcp_id, text11, strlen(text11));
//        ipts.tcp.write(tcp_id, "\r\n", 2);

//    } else {
//        // Įvestis leidžiama tik kai nesame peržiūros režime
//        if (myScroll == 0) {
//            int len = strlen(t2input.str_buf);

//            if (len < BUF_SIZE) {
//                if (ch == '\177') { // Backspace
//                    if (len > 0) {
//                        t2input.str_buf[len - 1] = (char)'\0';
//                        console_str_in("\b \b");
//                    }
//                } else {
//                    if (t2input.input_done == 0)
//                        t2input.str_buf[len] = ch;

//                    console_char_in(ch);
//                }
//            }
//        } else {
//            // Jei vartotojas paspaudė simbolį peržiūros režime:
//            // grįžtame į darbinę būseną (paskutinę eilutę)
//            while (myScroll > 0) {
//	          myScrollx = console.scroll(1);
//	          myScroll--;
//            }
//            myScroll = 0;

//            int len = strlen(t2input.str_buf);

//            if (len < BUF_SIZE) {
//                if (ch == '\177') { // Backspace
//                    if (len > 0) {
//                        t2input.str_buf[len - 1] = (char)'\0';
//                        console_str_in("\b \b");
//                    }
//                } else {
//                    if (t2input.input_done == 0)
//                        t2input.str_buf[len] = ch;

//                    console_char_in(ch);
//                }
//            }
//        }
//    }
//}


extern "C" void console_char_out(char ch)
{
    // send console output to BT socket
//    if (socket_output &&
//        connState == ConnectionState::Connected)
//    if (socket_output && connected1)   //----------------------------------------
//    {//---------------------------------------------------------------------------
//        ipts.tcp.write(tcp_id, &ch, 1);//------------------------------------------
//    }//----------------------------------------------------------------------------

    // always keep local console active
    if (myScroll != 0)
    {
        while (myScroll > 0)
        {
            myScrollx = console.scroll(1);
            myScroll--;
        }

        myScroll = 0;
    }

    int len = strlen(t2input.str_buf);

    if (len < BUF_SIZE)
    {
        if (ch == '\177')
        {
            if (len > 0)
            {
                t2input.str_buf[len - 1] = '\0';
                console_str_in("\b \b");
            }
        }
        else
        {
            if (t2input.input_done == 0)
            {
                t2input.str_buf[len] = ch;
                t2input.str_buf[len + 1] = '\0';
            }

            console_char_in(ch);
        }
    }
}


extern "C" void console_str_out(const char* str){
	for(unsigned int i = 0; i < strlen(str); i++){
		console_char_out(str[i]);
	}
}

extern "C" void console_str_with_length_out(const char* str, int length){
	if(t2input.input_mode == 1 && str == "\r\n")
		t2input.input_done = 1;

	console_str_out((char*)str);
}
