#include <Console.h>
#include <IPtoStream.h>
#include <opp.h>
#include <string.h>
#include <vmchset.h>
#include <vmgraph.h>
#include <vmio.h>
#include <vmstdlib.h>
#include <vmsys.h>
#include <vmtimer.h>

#include <array>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Console_io.h"
#include "T2Input.h"
#include "main.h"
#include "thread.h"

int scr_w = 0, scr_h = 0;
VMUINT8* layer_bufs[2] = {0, 0};
VMINT layer_hdls[2] = {-1, -1};

volatile bool key_pending = false;
volatile VMINT pending_event = 0;
volatile VMINT pending_keycode = 0;

VMCHAR command[100] = {};
VMCHAR portx[100] = {};
VMCHAR login[100] = {};
VMCHAR password[100] = {};

VMBOOL missingConfigFile = VM_TRUE;
VMBOOL flightMode = VM_FALSE;
VMBOOL startup = VM_FALSE;
VMCHAR text[512] = {};
//VMCHAR text11[100] = {}; //remove !!!!!!!!!!!!!!!!!!!

VMCHAR text_pcr[100] = {};

VMCHAR text22[100] = {};
VMCHAR text33[100] = {};
VMCHAR text44[100] = {};
VMCHAR text55[100] = {};

VMCHAR text222[100] = {};
VMCHAR text333[100] = {};
VMCHAR text444[100] = {};
//VMCHAR text555[] = "\n";

//VMCHAR my_path[100] = {};
//VMCHAR my_file_name[100] = {};
//VMINT lenght_555;
//VMINT lenght_666;

VMCHAR text6[100];
VMCHAR text3[41] = "IPoverObexTest\n\n";

VMCHAR text4[250] =  "about      - show program information\nhelp       - list available commands\nlistbt     - list paired BT connections\ndisconnect - disconnect BT\nconnect    - connect to BT by index\n";
VMCHAR text5[250] =  "export     - save buffer to txt file\ncls        - clear screen\nexit       - quits the program\n\n";

//VMCHAR text4[150] =
//    "about        - show program information\nhelp         - list available "
//    "commands\necho         - print string\ndir          - list directory "
//    "content\n";
//VMCHAR text5[172] =
//    "cd           - change current directory\npath         - sets a "
//    "path\nmkdir        - creates a directory\nrmdir        - deletes a "
//    "directory\ncopy         - copies a file\n";
//VMCHAR text10[232] =
//    "append       - joins two text files\nrename       - renames a file\nmove  "
//    "       - move a file\ndisconnect   - disconnect BT \nconnect      - "
//    "connect to BT by index\ncls          - clear screen\nexit         - quits "
//    "the program\n\n";
VMCHAR text88[44] = "The syntax of the command is incorrect.\n\n";
VMINT hnd;
VMINT test;
VMINT rrrrrrr = 0;
VMINT page11 = 0;
VMINT plus_line = 29;  // dirtyhack: compensate 29 lines of ignored first display
struct vm_fileinfo_ext fileInfo;
VMWCHAR fullPath1[100];
VMWCHAR fullPath2[100];
VMWCHAR fullPath3[100];
VMWCHAR fullPath4[100];
VMUINT nread;
VMFILE f_read;
VMFILE f_write;
VMCHAR new_data[2000];

VMINT network_timer_id = -1;

extern void console_execute_command(const char* cmd);

extern int myScroll;

//char input_buffer[256] = {}; //remove!!!!!!!!!!
//int input_length = 0; //remove!!!!!!!!!!!!
bool remote_mode = false; 
//bool socket_output = true; //remove !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

static char buf[1024];

Console console;
T2Input t2input;

int main_timer_id = -1;

int timeout_timer_id = -1;  // Timer for waiting telnet to connect
int timeout = 0;            // Timeout counter

char ip[BUF_SIZE];

char port1[BUF_SIZE];

VMINT timer_id1 = -1;
VMINT num = 0;

vm_srv_bt_cm_dev_struct bt_info;

bool connected = false;
bool bt_off = false;
bool bt_empty = false;
bool show_msg = false;

bool bt_initialized = false;

VMUINT8 my_mac[6];

struct CommandContext {
    const char* arg1;
    const char* arg2;
    const char* arg3;
};

struct CommandHandler {
    const char* name;
    void (*handler)(const CommandContext&);
};

struct ParsedCommand {
    char cmd[64];
    char arg1[128];
    char arg2[128];
    char arg3[128];
};

ConnectionState connState = ConnectionState::Disconnected;

enum class ChatUiState {
    SetName,
    StartupMenu,
    ConnectSelection,
    Listening,
    Chat,
    ReconnectMenu
};

ChatUiState chatUiState = ChatUiState::SetName;
int selected_peer_index = -1;
int last_peer_index = -1;
char active_peer_name[80] = "Peer";
char last_peer_name[80] = "";
char local_sender_name[80] = "Me";
char incoming_line_buffer[512] = {};
size_t incoming_line_length = 0;

//enum {
//    OPP_STATE_IDLE,
//    OPP_STATE_CONNECTING,
//    OPP_STATE_CONNECTED,
//    OPP_STATE_DISCONNECTED
//};

//volatile int opp_state;

bool remote_disconnect_message = false;

#ifndef WIN32
extern "C" void _sbrk() {}
extern "C" void _write() {}
extern "C" void _close() {}
extern "C" void _lseek() {}
extern "C" void _open() {}
extern "C" void _read() {}
extern "C" void _exit() {}
extern "C" void _getpid() {}
extern "C" void _kill() {}
extern "C" void _fstat() {}
extern "C" void _isatty() {}
#endif

void handle_sysevt(VMINT message, VMINT param);
void handle_keyevt(VMINT event, VMINT keycode);

void key_handler(VMINT event, VMINT keycode);

void create_app_txt_path(VMWSTR text, VMSTR extt);
void checkFileExist(void);
VMINT parseText(VMSTR text);
VMINT parseText1(VMSTR text);
VMINT parseText2(VMSTR text);
void timer1(int a);
void trim(char* result_data, size_t result_size, const char* input_data);
void create_app_txt_path1(void);
void create_supdir_path(VMWSTR result, VMWSTR source);
void create_search_path(VMWSTR result, VMWSTR source, VMSTR text);
VMINT cb(VMINT act, VMUINT32 total, VMUINT32 completed, VMINT hdl);
void trim_left_symbols(char* result_data, const char* input_data);
void extract_path(char* result_data, const char* input_data);
void stringReverse(char* str);
void trim_single_spec_symb(char* result, const char* input);
int cprintf(char const* const format, ...);

void cmd_help(const CommandContext& ctx);
void cmd_about(const CommandContext& ctx);
void cmd_cls(const CommandContext& ctx);
void cmd_exit(const CommandContext& ctx);
void cmd_line(const CommandContext& ctx);
void execute_command(const ParsedCommand& cmd);
void cmd_dir(const CommandContext& ctx);
void cmd_cd(const CommandContext& ctx);
void cmd_echo(const CommandContext& ctx);
ParsedCommand parse_command(const char* input);
void cmd_remote(const CommandContext&);
void cmd_local(const CommandContext&);
static void watchdog_timer(VMINT tid);
void cleanup_resources();

void prompt_tick();
void process_local_command(const char*);
void print_bt_address(int num);
void set_bt_address(int num_index);
void print_bt_address1(int num);
void show_start_menu();
void show_connect_menu();
void show_reconnect_menu();
void handle_chat_line(const char* input);
void flush_incoming_line();
void consume_incoming_bytes(const char* data, size_t size);
bool ensure_bt_ready();
void start_network_loop();
void cmd_start_network(const CommandContext& ctx);
void cmd_stop_network(const CommandContext& ctx);
void cmd_listbt(const CommandContext& ctx);
void cmd_export(const CommandContext& ctx);
void create_root_full_path_name(VMWSTR result, const VMCHAR* extt);

CommandHandler commands[] = {
    {"help", cmd_help},
    {"about", cmd_about},
    {"cls", cmd_cls},
    {"exit", cmd_exit},
    {"line", cmd_line},
    {"cd", cmd_cd},
    {"dir", cmd_dir},
    {"echo", cmd_echo},
    {"remote", cmd_remote},
    {"local", cmd_local},
    {"connect", cmd_start_network},
    {"disconnect", cmd_stop_network},
    {"listbt", cmd_listbt},
    {"export", cmd_export},
};

void vm_main(void) {

    scr_w = vm_graphic_get_screen_width();
    scr_h = vm_graphic_get_screen_height();
    console.init();
    t2input.init();
    vm_reg_sysevt_callback(handle_sysevt);
    vm_reg_keyboard_callback(handle_keyevt);
    checkFileExist();
    create_app_txt_path1();
}

void draw() {
    vm_graphic_fill_rect(layer_bufs[1], 0, 0, scr_w, scr_h, tr_color, tr_color);

    if (myScroll == 0) {
        vm_graphic_line(layer_bufs[1], console.cursor_x * char_width,
                        (console.cursor_y + 1) * char_height,
                        (console.cursor_x + 1) * char_width,
                        (console.cursor_y + 1) * char_height,
                        console.cur_textcolor);
    }

    t2input.draw();
    if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
        vm_graphic_flush_layer(layer_hdls, 2);
    }
}

void timer(int tid) { draw(); }

void timeout_f(int tid) {
    timeout++;  // Increase the timeout counter

    if (timeout > 22 && timeout < 24) {
        console_str_in("\nTimed out, exiting...");
    }

    if (timeout > 25) {
        vm_exit_app();  // Exit
    }
}

void handle_sysevt(VMINT message, VMINT param) {
    switch (message) {
        case VM_MSG_CREATE:
        case VM_MSG_ACTIVE:
            layer_hdls[0] = vm_graphic_create_layer(0, 0, scr_w, scr_h, -1);
            layer_hdls[1] = vm_graphic_create_layer(0, 0, scr_w, scr_h, tr_color);
            vm_graphic_set_clip(0, 0, scr_w, scr_h);
            layer_bufs[0] = vm_graphic_get_layer_buffer(layer_hdls[0]);
            layer_bufs[1] = vm_graphic_get_layer_buffer(layer_hdls[1]);

            vm_switch_power_saving_mode(turn_off_mode);

            console.scr_buf = layer_bufs[0];
            console.draw_all();

            t2input.scr_buf = layer_bufs[1];
            t2input.layer_handle = layer_hdls[1];

            t2input.input_mode = 1;  // Get input from keyboard to buffer

            if (startup == VM_FALSE) {
                startup = VM_TRUE;
                console_str_in("IPoverObex Bluetooth chat\n\n");
                cprintf("Enter your sender name:\n\n");
            }

            if (main_timer_id == -1) {
                main_timer_id = vm_create_timer(1000 / 15, timer);  // 15 fps
            }

            break;

        case VM_MSG_PAINT:
            if (show_msg == true) {
                show_msg = false;
                console_str_in(text_pcr);
            }

            draw();
            break;

        case VM_MSG_INACTIVE:
            vm_switch_power_saving_mode(turn_on_mode);
            cleanup_resources();
            break;

        case VM_MSG_QUIT:
            cleanup_resources();
            if (bt_off == false) {
                ipts.quit();
            }
            break;
    }
}

void handle_keyevt(VMINT event, VMINT keycode) {
    t2input.handle_keyevt(event, keycode);
}

void key_handler(VMINT event, VMINT keycode) {
    pending_event = event;
    pending_keycode = keycode;
    key_pending = true;
}

void create_app_txt_path(VMWSTR text, VMSTR extt) {
    VMWCHAR fullPath[100];
    VMWCHAR wfile_extension[8];

    vm_get_exec_filename(fullPath);
    vm_ascii_to_ucs2(wfile_extension, 8, extt);
    vm_wstrncpy(text, fullPath, vm_wstrlen(fullPath) - 3);
    vm_wstrcat(text, wfile_extension);
}

void checkFileExist(void) {
    VMFILE f_read;
    VMUINT nread;
    VMWCHAR file_pathw[100];
    VMCHAR new_data[500];

    create_app_txt_path(file_pathw, (char*)"txt");

    f_read = vm_file_open(file_pathw, MODE_READ, FALSE);

    if (f_read < 0) {
        missingConfigFile = VM_TRUE;
    } else {
        vm_file_read(f_read, new_data, sizeof(new_data) - 1, &nread);
        new_data[nread] = '\0';
        vm_file_close(f_read);
        if (strlen(new_data) > 1) {
            parseText(new_data);
            missingConfigFile = VM_FALSE;
        } else {
            missingConfigFile = VM_TRUE;
        }
    }
}

VMINT parseText(VMSTR text) {
    VMCHAR vns_simbl[2] = {};
    VMCHAR nauj_strng[100] = {};
    VMINT counter = 0;
    VMINT counter1 = 0;
    VMCHAR* ptr;

    ptr = text;

    while (*ptr != '\0' && counter1 != 5) {
        if (*ptr == '\r') {
            ptr++;
        }
        if (*ptr == '\n') {
            counter = counter + 1;

            if (counter == 1) {
                strcpy(command, nauj_strng);
            }
            if (counter == 2) {
                strcpy(ip, nauj_strng);
            }
            if (counter == 3) {
                strcpy(portx, nauj_strng);
            }
            if (counter == 4) {
                strcpy(login, nauj_strng);
            }
            if (counter == 5) {
                strcpy(password, nauj_strng);
            }

            counter1 = counter;

            strcpy(nauj_strng, "");
            ptr++;
        }

        vns_simbl[0] = *ptr;
        vns_simbl[1] = '\0';

        if (strlen(nauj_strng) < sizeof(nauj_strng) - 1) {
            strncat(nauj_strng, vns_simbl, 1);
        }
        ptr++;
    }

    if (counter == 0) {
        strcpy(command, nauj_strng);
    }
    if (counter == 1) {
        strcpy(ip, nauj_strng);
    }
    if (counter == 2) {
        strcpy(portx, nauj_strng);
    }
    if (counter == 3) {
        strcpy(login, nauj_strng);
    }
    if (counter == 4) {
        strcpy(password, nauj_strng);
    }

    return 0;
}

VMINT parseText1(VMSTR text) {
    VMCHAR vns_simbl[2] = {};
    VMCHAR nauj_strng[100] = {};
    VMINT counter = 0;
    VMCHAR* ptr;

    ptr = text;

    while (*ptr != '\0') {
        if (*ptr == ' ') {
            ++counter;

            if (counter == 1)
                strcpy(text22, nauj_strng);
            else if (counter == 2)
                strcpy(text33, nauj_strng);
            else if (counter == 3)
                strcpy(text44, nauj_strng);

            if (counter < 4) nauj_strng[0] = '\0';

            ++ptr;
            continue;
        }

        vns_simbl[0] = *ptr;
        vns_simbl[1] = '\0';

        if (strlen(nauj_strng) < sizeof(nauj_strng) - 1)
            strncat(nauj_strng, vns_simbl, 1);

        ++ptr;
    }

    if (counter == 0) {
        strcpy(text22, nauj_strng);
    }
    if (counter == 1) {
        strcpy(text33, nauj_strng);
    }
    if (counter == 2) {
        strcpy(text44, nauj_strng);
    }
    if (counter > 2) {
        strcpy(text55, nauj_strng);
    }

    return 0;
}

VMINT parseText2(VMSTR text) {
    VMCHAR textx[220] = {};
    VMCHAR vns_simbl[2] = {};
    VMCHAR nauj_strng[100] = {};
    VMINT counter = 0;
    VMCHAR text22X[100] = {};
    VMCHAR text33X[100] = {};
    VMCHAR text44X[100] = {};
    VMCHAR text44Y[100] = {};
    VMCHAR text44Z[100] = {};
    VMCHAR text44Q[100] = {};
    VMCHAR text44K[100] = {};

    VMCHAR* ptr;
    VMCHAR* ptr1;
    VMCHAR* ptr2;
    VMCHAR* ptr3;

    strcpy(textx, text);

    ptr = textx;

    stringReverse(textx);

    while (*ptr != '\0') {
        if (*ptr == ' ' && counter != 2) {
            counter = counter + 1;

            if (counter == 1) {
                strcpy(text22X, nauj_strng);
            }
            if (counter == 2) {
                strcpy(text33X, nauj_strng);
            }
            if (counter < 3) {
                strcpy(nauj_strng, "");
            }
            ptr++;
        }

        vns_simbl[0] = *ptr;
        vns_simbl[1] = '\0';

        if (strlen(nauj_strng) < sizeof(nauj_strng) - 1) {
            strncat(nauj_strng, vns_simbl, 1);
        }
        ptr++;
    }

    if (counter == 0) {
        strcpy(text22X, nauj_strng);
    }
    if (counter == 1) {
        strcpy(text33X, nauj_strng);
    }
    if (counter > 1) {
        strcpy(text44X, nauj_strng);
    }

    if (strlen(text22X) > 0) {
        ptr1 = text22X;
        stringReverse(ptr1);
        snprintf(text222, sizeof(text222), "%s", ptr1);
    }

    if (strlen(text33X) > 0) {
        ptr2 = text33X;
        stringReverse(ptr2);
        sprintf(text333, "%s", ptr2);
    }

    if (strlen(text44X) > 0) {
        trim(text44Y, sizeof(text44Y), text44X);

        if (strlen(text44Y) != strlen(text22)) {
            size_t lenY = strlen(text44Y);
            size_t len22 = strlen(text22);
            if (lenY <= len22 + 1) return -1;
            size_t copy_len = strlen(text44Y) - (strlen(text22) + 1);  //!!!!
            if (copy_len >= sizeof(text44Z)) copy_len = sizeof(text44Z) - 1;
            memcpy(text44Z, text44Y, copy_len);
            text44Z[copy_len] = '\0';

            if (strlen(text44Z) > 1) {
                ptr3 = text44Z;
                stringReverse(ptr3);
                sprintf(text44Q, "%s", ptr3);
                trim_single_spec_symb(text44K, text44Q);
            } else if (strlen(text44Z) == 1) {
                ptr3 = text44Z;
                sprintf(text44Q, "%s", ptr3);
                trim_single_spec_symb(text44K, text44Q);
            } else {
            }

            if (strlen(text44K) > 0) {
                sprintf(text444, "%s", text44K);
            }
        }
    }

    return 0;
}

void timer1(int a) {
    vm_delete_timer_ex(a);
    vm_exit_app();
}

void trim(char* result_data, size_t result_size, const char* input_data)
{
    while (isspace((unsigned char)*input_data))
        ++input_data;

    size_t len = strlen(input_data);

    while (len > 0 &&
           isspace((unsigned char)input_data[len - 1]))
        --len;

    if (len >= result_size)
        len = result_size - 1;

    memcpy(result_data, input_data, len);
    result_data[len] = '\0';
}

void create_app_txt_path1(void) {
    VMWCHAR fullPath[100];
    VMWCHAR wfile_extension[10];

    vm_get_exec_filename(fullPath);
    vm_get_path(fullPath, fullPath1);
}

void create_supdir_path(VMWSTR result, VMWSTR source) {
    VMINT addrBefo = 3;
    VMINT addrLast = 0;
    VMINT count1 = 0;
    VMWCHAR fullPathx2[100];
    VMWCHAR* ptr;

    if (wstrlen(source) < 4) {
        wstrcpy(fullPathx2, source);
        wstrcpy(result, fullPathx2);
        return;
    }

    ptr = source;

    while (*ptr != L'\0') {
        addrBefo = addrLast;

        if (*ptr == L'\\') {
            addrLast = count1 + 1;
        }

        count1 = count1 + 1;

        ptr++;
    }

    vm_wstrncpy(fullPathx2, source, addrBefo);
    wstrcpy(result, fullPathx2);
}

void create_search_path(VMWSTR result, VMWSTR source, VMSTR text) {
    VMWCHAR fullPathx1[100];
    VMWCHAR wtext[100];

    vm_ascii_to_ucs2(wtext, (strlen(text) + 1) * 2, text);  //"*.*"
    vm_wstrcpy(fullPathx1, source);
    vm_wstrcat(fullPathx1, wtext);
    vm_wstrcpy(result, fullPathx1);
}

VMINT cb(VMINT act, VMUINT32 total, VMUINT32 completed, VMINT hdl) { return 0; }

void trim_left_symbols(char* result_data, const char* input_data) {
    strcpy(result_data, input_data);

    size_t len = strlen(result_data);

    while (len > 0 && result_data[len - 1] == '\\') {
        result_data[len - 1] = '\0';
        --len;
    }
}

void extract_path(char* result_data, const char* input_data) {
    strcpy(result_data, input_data);

    char* last = strrchr(result_data, '\\');

    if (last != nullptr) {
        *(last + 1) = '\0';
    } else {
        result_data[0] = '\0';
    }
}

void stringReverse(char* str) {
    int len = strlen(str);

    if (len > 1) {
        // pointers to start and end
        char* start = str;
        char* end = str + len - 1;

        while (start < end) {
            char temp = *start;
            *start = *end;
            *end = temp;
            start++;
            end--;
        }
    }
}

void trim_single_spec_symb(char* result, const char* input) {
    size_t len = strlen(input);

    if (len >= 2 && input[0] == '\'' && input[len - 1] == '\'') {
        strncpy(result, input + 1, len - 2);
        result[len - 2] = '\0';
    } else {
        strcpy(result, input);
    }
}

void cmd_help(const CommandContext& ctx) {
    console_str_in(text4);
    console_str_in(text5);
}

void cmd_about(const CommandContext& ctx) { console_str_in(text3); }

void cmd_cls(const CommandContext& ctx) {
    if (strlen(ctx.arg1) != 0) {
        console_str_in(text88);
        return;
    }

    if (my_intx > 471) {
        console.clean_history();
        my_intx = 0;
    }

    console.erase_display(2);
    console.reset();
    rrrrrrr = 0;
}

void cmd_exit(const CommandContext& ctx) { vm_create_timer_ex(10, timer1); }

void cmd_line(const CommandContext& ctx) {
    snprintf(text, sizeof(text), "Line: %d\n\n", my_intx + plus_line);

    console_str_in(text);
}

void execute_command(const ParsedCommand& cmd) {
    CommandContext ctx = {cmd.arg1, cmd.arg2, cmd.arg3};

    const size_t count = sizeof(commands) / sizeof(commands[0]);

    for (size_t i = 0; i < count; ++i) {
        if (vm_string_equals_ignore_case(cmd.cmd, commands[i].name) == 0) {
            commands[i].handler(ctx);
            return;
        }
    }

    snprintf(text, sizeof(text),
             "%s is not recognized as an internal or external command.\n\n",
             cmd.cmd);

    console_str_in(text);
}

void cmd_dir(const CommandContext& ctx) {
    create_search_path(fullPath2, fullPath1, (char*)"*.*");

    test = 0;
    hnd = vm_find_first_ext(fullPath2, &fileInfo);

    if (hnd < 0) {
        console_str_in("Directory read error\n\n");
        return;
    }

    do {
        vm_ucs2_to_ascii(text6, wstrlen(fileInfo.filefullname) + 1,
                         fileInfo.filefullname);

        if (fileInfo.attributes & VM_FS_ATTR_DIR) {
            strcpy(text, "<DIR> ");
        } else {
            strcpy(text, "-r--- ");
        }

        strncat(text, text6, sizeof(text) - strlen(text) - 1);

        strncat(text, "\n", sizeof(text) - strlen(text) - 1);

        console_str_in(text);

    } while (vm_find_next_ext(hnd, &fileInfo) == 0);

    vm_find_close_ext(hnd);

    console_str_in("\n");
}

void cmd_cd(const CommandContext& ctx) {
    if (strlen(ctx.arg1) == 0 ||
        vm_string_equals_ignore_case(ctx.arg1, ".") == 0) {
        vm_ucs2_to_ascii(text, wstrlen(fullPath1) + 1, fullPath1);

        strncat(text, "\n\n", sizeof(text) - strlen(text) - 1);

        console_str_in(text);
        return;
    }

    if (vm_string_equals_ignore_case(ctx.arg1, "..") == 0) {
        create_supdir_path(fullPath1, fullPath1);

        vm_ucs2_to_ascii(text, wstrlen(fullPath1) + 1, fullPath1);

        strncat(text, "\n\n", sizeof(text) - strlen(text) - 1);

        console_str_in(text);

        return;
    }

    trim_left_symbols(text6, ctx.arg1);

    if (strlen(text6) < sizeof(text6) - 1)
        strncat(text6, "\\", sizeof(text6) - strlen(text6) - 1);

    vm_ascii_to_ucs2(fullPath3, (strlen(text6) + 1) * 2, text6);

    vm_wstrcpy(fullPath2, fullPath1);
    vm_wstrcat(fullPath2, fullPath3);

    if (vm_file_get_attributes(fullPath2) == VM_FS_ATTR_DIR) {
        wstrcpy(fullPath1, fullPath2);

        vm_ucs2_to_ascii(text, wstrlen(fullPath1) + 1, fullPath1);
    } else {
        strcpy(text, "The system cannot find the path specified.");
    }

    strncat(text, "\n\n", sizeof(text) - strlen(text) - 1);

    console_str_in(text);
}

void cmd_echo(const CommandContext& ctx) {
    if (strlen(ctx.arg1) == 0) {
        console_str_in("ECHO is on.\n\n");
        return;
    }

    console_str_in(ctx.arg1);

    if (strlen(ctx.arg2)) {
        console_str_in(" ");
        console_str_in(ctx.arg2);
    }

    if (strlen(ctx.arg3)) {
        console_str_in(" ");
        console_str_in(ctx.arg3);
    }

    console_str_in("\n\n");
}

ParsedCommand parse_command(const char* input) {
    ParsedCommand out = {};

    sscanf(input, "%63s %127s %127s %127[^\n]", out.cmd, out.arg1, out.arg2,
           out.arg3);

    return out;
}

void cmd_remote(const CommandContext&) {
    remote_mode = true;
    console_str_in("Remote mode ON\n\n");
}

void cmd_local(const CommandContext&) {
    remote_mode = false;
    console_str_in("Local mode ON\n\n");
}

static void watchdog_timer(VMINT tid)
{
    // Timer has fired
    timer_id1 = -1;

    // Ignore if connection already completed
    if (connState != ConnectionState::Connecting)
    {
        return;
    }

    cprintf("Connection timed out.\n\n");

    ipts.disconnect();

    connected = false;
    remote_mode = false;
    connState = ConnectionState::Disconnected;

    t2input.free_buffer();

    show_start_menu();
}

void cleanup_resources() {
    if (layer_hdls[0] != -1) {
        vm_graphic_delete_layer(layer_hdls[0]);
        layer_hdls[0] = -1;
    }

    if (layer_hdls[1] != -1) {
        vm_graphic_delete_layer(layer_hdls[1]);
        layer_hdls[1] = -1;
    }

    if (main_timer_id != -1) {
        vm_delete_timer(main_timer_id);
        main_timer_id = -1;
    }
}

int cprintf(char const* const format, ...) {
    va_list aptr;

    va_start(aptr, format);
    int ret = vsnprintf(buf, sizeof(buf), format, aptr);
    va_end(aptr);

    console_str_in(buf);
    return ret;
}

bool ensure_bt_ready() {
    if (vm_btcm_get_power_status() == VM_SRV_BT_CM_POWER_OFF) {
        cprintf("Please turn on bluetooth first.\n\n");
        return false;
    }

    if (!bt_initialized) {
        ipts.init();
        bt_initialized = true;
    }

    start_network_loop();

    return true;
}

void show_start_menu() {
    cprintf("Select mode:\n");
    cprintf("1. Listen\n");
    cprintf("2. Connect\n\n");
    chatUiState = ChatUiState::StartupMenu;
}

void show_connect_menu() {
    num = vm_btcm_get_dev_num(VM_SRV_BT_CM_RECENT_USED_DEV);
    if (num <= 0) {
        cprintf("No paired devices found.\n\n");
        show_start_menu();
        return;
    }

    cprintf("Select paired device number:\n");
    for (VMINT d = 0; d < num; d++) {
        cprintf("%d: ", d + 1);
        print_bt_address(d);
    }
    cprintf("\n");
    chatUiState = ChatUiState::ConnectSelection;
}

void show_reconnect_menu() {
    cprintf("\nConnection closed.\n");
    if (last_peer_index >= 0) {
        cprintf("Last peer: %s\n", last_peer_name[0] ? last_peer_name : "Peer");
        cprintf("1. Reconnect\n");
    } else {
        cprintf("1. Listen\n");
    }
    cprintf("2. New connection\n\n");
    chatUiState = ChatUiState::ReconnectMenu;
}

void flush_incoming_line() {
    incoming_line_buffer[incoming_line_length] = '\0';

    if (incoming_line_length == 0) {
        return;
    }

    const char* message = incoming_line_buffer;
    char sender[80] = "Peer";
    const char* separator = strstr(incoming_line_buffer, "]:");

    if (incoming_line_buffer[0] == '[' && separator != nullptr &&
        separator > incoming_line_buffer + 1) {
        size_t sender_len = (size_t)(separator - incoming_line_buffer - 1);
        if (sender_len >= sizeof(sender)) {
            sender_len = sizeof(sender) - 1;
        }
        memcpy(sender, incoming_line_buffer + 1, sender_len);
        sender[sender_len] = '\0';

        message = separator + 2;
        if (*message == ' ') {
            ++message;
        }
    } else if (active_peer_name[0] != '\0') {
        snprintf(sender, sizeof(sender), "%s", active_peer_name);
    }

    char display[640];
    snprintf(display, sizeof(display), "[%s]: %s\n", sender, message);
    console_str_in(display);
}

void consume_incoming_bytes(const char* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        char c = data[i];

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            flush_incoming_line();
            incoming_line_length = 0;
            continue;
        }

        if (incoming_line_length + 1 < sizeof(incoming_line_buffer)) {
            incoming_line_buffer[incoming_line_length++] = c;
        }
    }
}

void handle_chat_line(const char* input) {
    if (!input || !*input) {
        return;
    }

    if (connState != ConnectionState::Connected) {
        cprintf("Not connected.\n\n");
        return;
    }

    char payload[640];
    snprintf(payload, sizeof(payload), "[%s]: %s\n", local_sender_name, input);
    ipts.write(payload, strlen(payload));

    char display[640];
    snprintf(display, sizeof(display), "[%s]: %s\n", local_sender_name, input);
    console_str_in(display);
}

void start_network_loop() {
    if (network_timer_id != -1) {
        return;
    }

    network_timer_id = vm_create_timer(33, [](int tid) {
        if (!bt_initialized) {
            return;
        }

        ipts.update();
        thread_next();

        if (opp_connected_event) {
            opp_connected_event = FALSE;

            if (timer_id1 != -1) {
                vm_delete_timer_ex(timer_id1);
                timer_id1 = -1;
            }

            connected = true;
            connState = ConnectionState::Connected;
            remote_mode = true;
            chatUiState = ChatUiState::Chat;

            if (selected_peer_index < 0) {
                const VMCHAR* peer_from_stack = bt_opp_get_last_peer_name();
                if (peer_from_stack && peer_from_stack[0] != '\0') {
                    snprintf(active_peer_name, sizeof(active_peer_name), "%s", peer_from_stack);
                    snprintf(last_peer_name, sizeof(last_peer_name), "%s", peer_from_stack);
                }
            }

            cprintf("Connected with %s.\n", active_peer_name);
        }

        if (opp_disconnected_event) {
            opp_disconnected_event = FALSE;

            connected = false;
            connState = ConnectionState::Disconnected;
            remote_mode = false;
            selected_peer_index = -1;
            incoming_line_length = 0;

            t2input.free_buffer();

            if (remote_disconnect_message) {
                remote_disconnect_message = false;
            } else {
                cprintf("Disconnected from %s.\n", active_peer_name);
            }

            show_reconnect_menu();
        }

        if (key_pending) {
            key_pending = false;
            t2input.handle_keyevt(pending_event, pending_keycode);
        }

        char rbuf[101];
        size_t size;
        do {
            size = ipts.read(rbuf, 100);
            if (size > 0) {
                consume_incoming_bytes(rbuf, size);
            }
        } while (size > 0);

        if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
            vm_graphic_flush_layer(layer_hdls, 2);
        }
    });
}

void process_local_command(const char* input) {
    if (input == nullptr) {
        return;
    }

    char line[256];
    snprintf(line, sizeof(line), "%s", input);

    size_t start = 0;
    while (line[start] != '\0' && isspace((unsigned char)line[start])) {
        ++start;
    }
    size_t end = strlen(line);
    while (end > start && isspace((unsigned char)line[end - 1])) {
        --end;
    }
    line[end] = '\0';

    const char* text_line = line + start;
    if (*text_line == '\0') {
        return;
    }

    if (chatUiState == ChatUiState::SetName) {
        snprintf(local_sender_name, sizeof(local_sender_name), "%s", text_line);
        cprintf("Sender set to: %s\n\n", local_sender_name);
        show_start_menu();
        return;
    }

    if (chatUiState == ChatUiState::Chat || chatUiState == ChatUiState::Listening) {
        handle_chat_line(text_line);
        return;
    }

    if (chatUiState == ChatUiState::StartupMenu) {
        if (strcmp(text_line, "1") == 0) {
            if (!ensure_bt_ready()) {
                return;
            }
            cprintf("Listening for peer connection...\n\n");
            chatUiState = ChatUiState::Listening;
            return;
        }

        if (strcmp(text_line, "2") == 0) {
            if (!ensure_bt_ready()) {
                return;
            }
            show_connect_menu();
            return;
        }

        cprintf("Unknown option.\n\n");
        show_start_menu();
        return;
    }

    if (chatUiState == ChatUiState::ConnectSelection) {
        for (size_t i = 0; text_line[i] != '\0'; ++i) {
            if (!isdigit((unsigned char)text_line[i])) {
                cprintf("Enter a device number.\n\n");
                return;
            }
        }

        int choice = atoi(text_line);
        if (choice <= 0 || choice > num) {
            cprintf("Invalid device number.\n\n");
            return;
        }

        selected_peer_index = choice - 1;
        last_peer_index = selected_peer_index;

        vm_btcm_get_dev_info_by_index(selected_peer_index, VM_SRV_BT_CM_RECENT_USED_DEV, &bt_info);
        snprintf(active_peer_name, sizeof(active_peer_name), "%s", bt_info.name);
        snprintf(last_peer_name, sizeof(last_peer_name), "%s", bt_info.name);

        char index_text[12];
        snprintf(index_text, sizeof(index_text), "%d", choice);
        chatUiState = ChatUiState::Listening;
        cmd_start_network(CommandContext{index_text, "", ""});
        return;
    }

    if (chatUiState == ChatUiState::ReconnectMenu) {
        if (strcmp(text_line, "1") == 0) {
            if (!ensure_bt_ready()) {
                return;
            }

            if (last_peer_index >= 0) {
                char index_text[12];
                snprintf(index_text, sizeof(index_text), "%d", last_peer_index + 1);
                chatUiState = ChatUiState::Listening;
                cmd_start_network(CommandContext{index_text, "", ""});
                return;
            }

            cprintf("Listening for peer connection...\n\n");
            chatUiState = ChatUiState::Listening;
            return;
        }

        if (strcmp(text_line, "2") == 0) {
            show_start_menu();
            return;
        }

        cprintf("Unknown option.\n\n");
        show_reconnect_menu();
    }
}

void print_bt_address(int num) {
    if (num < 0) {
        return;
    }

    vm_btcm_get_dev_info_by_index(num, VM_SRV_BT_CM_RECENT_USED_DEV, &bt_info);

    VMUINT8 lap0 = (VMUINT8)(bt_info.bd_addr.lap & 0xFF);
    VMUINT8 lap1 = (VMUINT8)((bt_info.bd_addr.lap >> 8) & 0xFF);
    VMUINT8 lap2 = (VMUINT8)((bt_info.bd_addr.lap >> 16) & 0xFF);
    VMUINT8 uap = bt_info.bd_addr.uap;
    VMUINT8 nap0 = (VMUINT8)(bt_info.bd_addr.nap & 0xFF);
    VMUINT8 nap1 = (VMUINT8)((bt_info.bd_addr.nap >> 8) & 0xFF);

    if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
       cprintf("%02X:%02X:%02X:%02X:%02X:%02X %s\n", nap1, nap0, uap, lap2, lap1, lap0, bt_info.name);
    }
}

void set_bt_address(int num_index) {
    if (num_index < 0) {
        return;
    }

    vm_btcm_get_dev_info_by_index(num_index, VM_SRV_BT_CM_RECENT_USED_DEV,
                                  &bt_info);

    my_mac[0] = (VMUINT8)((bt_info.bd_addr.nap >> 8) & 0xFF);   // NAP high byte
    my_mac[1] = (VMUINT8)(bt_info.bd_addr.nap & 0xFF);          // NAP low byte
    my_mac[2] = bt_info.bd_addr.uap;                            // UAP
    my_mac[3] = (VMUINT8)((bt_info.bd_addr.lap >> 16) & 0xFF);  // LAP high byte
    my_mac[4] = (VMUINT8)((bt_info.bd_addr.lap >> 8) & 0xFF);   // LAP mid byte
    my_mac[5] = (VMUINT8)(bt_info.bd_addr.lap & 0xFF);          // LAP low byte
}

void print_bt_address1(int num) {
    if (num < 0) {
        return;
    }

    vm_btcm_get_dev_info_by_index(num, VM_SRV_BT_CM_RECENT_USED_DEV, &bt_info);

    VMUINT8 lap0 = (VMUINT8)(bt_info.bd_addr.lap & 0xFF);
    VMUINT8 lap1 = (VMUINT8)((bt_info.bd_addr.lap >> 8) & 0xFF);
    VMUINT8 lap2 = (VMUINT8)((bt_info.bd_addr.lap >> 16) & 0xFF);
    VMUINT8 uap = bt_info.bd_addr.uap;
    VMUINT8 nap0 = (VMUINT8)(bt_info.bd_addr.nap & 0xFF);
    VMUINT8 nap1 = (VMUINT8)((bt_info.bd_addr.nap >> 8) & 0xFF);

    sprintf(text_pcr, "Connect: %02X:%02X:%02X:%02X:%02X:%02X %s\n", nap1, nap0, uap, lap2, lap1, lap0, bt_info.name);
    show_msg = true;
    if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
       cprintf("Connect: %02X:%02X:%02X:%02X:%02X:%02X %s\n", nap1, nap0, uap, lap2, lap1, lap0, bt_info.name);
    }

}

void cmd_start_network(const CommandContext& ctx) {

    const char* arg1 = ctx.arg1 ? ctx.arg1 : "";
    const char* arg2 = ctx.arg2 ? ctx.arg2 : "";
//    const char* arg3 = ctx.arg3 ? ctx.arg3 : ""; //delete ??????????

if (connState == ConnectionState::Disconnecting)
{
    cprintf("Please wait for disconnect to complete.\n\n");
    return;
}

if (connState == ConnectionState::Connecting)
{
    cprintf("Already connecting.\n\n");
    return;
}


    if (connState == ConnectionState::Connected) {
        show_msg = true;
        sprintf(text_pcr, "%s", "Disconnect first !\n\n");
        if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
           cprintf("Disconnect first !\n\n");
        }
        return;
    }

    num = vm_btcm_get_dev_num(VM_SRV_BT_CM_RECENT_USED_DEV);

    if (num <= 0) {
        bt_empty = true;
        show_msg = true;
        sprintf(text_pcr, "%s", "Need initialy pair with BT host !\n\n");
        if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
           cprintf("Need initialy pair with BT host !\n\n");
        }
        return;

    } else if (vm_btcm_get_power_status() == VM_SRV_BT_CM_POWER_OFF) {
        bt_off = true;
        show_msg = true;
        sprintf(text_pcr, "%s", "Please turn on bluetooth !\n\n");
        if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
           cprintf("Please turn on bluetooth !\n\n");
        }
        return;

    } else if (strlen(arg1) != 0 && strlen(arg2) != 0) {
        show_msg = true;
        sprintf(text_pcr, "%s", "Incorrect connection number !\n\n");
        if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
           cprintf("Incorrect connection number !\n\n");
        }
        return;

    } else {
        int conn_num = 1;

if (arg1[0] != '\0') {

    for (size_t i = 0; arg1[i] != '\0'; ++i) {
        if (!isdigit((unsigned char)arg1[i])) {
            cprintf("Incorrect connection number !\n\n");
            return;
        }
    }

    conn_num = atoi(arg1);
}

        if (conn_num <= 0 || conn_num > num) {
           if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
              cprintf("Incorrect connection number !\n\n");
           }
            return;
        }

        set_bt_address(conn_num - 1);
        print_bt_address1(conn_num - 1);

    }

if (!bt_initialized) {
    ipts.init();
    bt_initialized = true;
}

opp_connected_event = FALSE;
opp_disconnected_event = FALSE;

//connState = ConnectionState::Disconnected;
connState = ConnectionState::Connecting;
connected = false;

ipts.connectBT(my_mac);

if (timer_id1 == -1)
{
    timer_id1 = vm_create_timer_ex(10000, watchdog_timer); // 10 sec
}
//thread_create(2048, connect_wait_thread);
//timer_id1 = vm_create_timer_ex(5000, watchdog_timer);

    start_network_loop();
}


//void cmd_stop_network(const CommandContext& ctx)
//{

//if (connState != ConnectionState::Connected) {
//    cprintf("Not connected.\n\n");
//    return;
//}

//    connState = ConnectionState::Disconnected;
//    remote_mode = false;
//    connected = false;

//    // stop watchdog timer
//    if (timer_id1 != -1) {
//        vm_delete_timer_ex(timer_id1);
//        timer_id1 = -1;
//    }

//    ipts.disconnect();

//t2input.free_buffer();
//    cprintf("Disconnected.\n\n");
//}

void cmd_stop_network(const CommandContext&)
{

if (connState == ConnectionState::Disconnecting)
{
    cprintf("Already disconnecting.\n\n");
    return;
}

    if (connState != ConnectionState::Connected)
    {
        cprintf("Not connected.\n\n");
        return;
    }

    connState = ConnectionState::Disconnecting;

//remote_disconnect_message = true;

    ipts.disconnect();

    cprintf("Disconnecting...\n\n");
}

void cmd_listbt(const CommandContext& ctx) {

    if (strlen(ctx.arg1) == 0) {
        cprintf("Available connections:\n");
        VMINT d;

        num = vm_btcm_get_dev_num(VM_SRV_BT_CM_RECENT_USED_DEV);
        for (d = 0; d < num; d++) {
            cprintf("%d: ", d + 1);
            print_bt_address(d);
        } 
        cprintf("\n");
    } else {
        cprintf(text88);
    }

}

void cmd_export(const CommandContext& ctx) {

    if (strlen(ctx.arg1) == 0) {

        VMWCHAR file_pathw[128];
        create_root_full_path_name(file_pathw, "txt");
        console.save_to_file(file_pathw);
        cprintf("\n");
    } else {
        cprintf(text88);
    }

}

void create_root_full_path_name(VMWSTR result, const VMCHAR* extt)
{
    VMINT drv;
    VMINT len;

    VMCHAR fAutoFileName[100];
    VMCHAR ext_with_dot[16];

    VMWCHAR wAutoFileName[100];
    VMWCHAR fullPath[100];
    VMWCHAR exeFileName[100];
    VMWCHAR wfile_extension[16];
    VMWCHAR text[100];

    if ((drv = vm_get_removable_driver()) < 0)
    {
        drv = vm_get_system_driver();
    }

    sprintf(fAutoFileName, "%c:\\", drv);

    vm_ascii_to_ucs2(wAutoFileName, (strlen(fAutoFileName) + 1) * 2, fAutoFileName);

    vm_get_exec_filename(fullPath);
    vm_get_filename(fullPath, exeFileName);

    sprintf(ext_with_dot, ".%s", extt);

    vm_ascii_to_ucs2(wfile_extension, (strlen(ext_with_dot) + 1) * 2, ext_with_dot);

    len = vm_wstrlen(exeFileName);

    if (len > 4)
    {
        vm_wstrncpy(text, exeFileName, len - 4);
        text[len - 4] = 0;
    }
    else
    {
        vm_wstrcpy(text, exeFileName);
    }

    vm_wstrcat(text, wfile_extension);

    vm_wstrcat(wAutoFileName, text);

    vm_wstrcpy(result, wAutoFileName);
}

//void connect_wait_thread()
//{
//    while (!opp_connected_event)
//    {
//        thread_next();
//    }

//    opp_connected_event = FALSE;

//    connected = true;
//    connState = ConnectionState::Connected;
//    remote_mode = true;

//    cprintf("Connected.\n");

//    thread_end();
//}

//void connect_wait_thread()
//{
//    while (opp_state == OPP_STATE_CONNECTING)
//        thread_next();

//    if (opp_state == OPP_STATE_CONNECTED)
//    {
//        connected = true;
//        connState = ConnectionState::Connected;
//        remote_mode = true;
//    }

//    thread_end();
//}

//void disconnect_wait_thread()
//{
//    while (!opp_disconnected_event)
//    {
//        thread_next();
//    }

//    opp_disconnected_event = FALSE;

//    connected = false;
//    connState = ConnectionState::Disconnected;
//    remote_mode = false;

//    cprintf("Disconnected.\n");

//    thread_end();
//}
