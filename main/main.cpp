#include "vmeditor.h"
#include "stdint.h"
#include <time.h>
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
#include "main.h"
#include "thread.h"

#define VM_CHSET_DEST_BYTES 4096                      /* vm_chset_convert target max bytes (<=4096 per spec) */
#define UCS2_CHUNK_WORDS    (VM_CHSET_DEST_BYTES / 2) /* 2048 words */

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

VMCHAR text_pcr[100] = {};

VMCHAR text22[100] = {};
VMCHAR text33[100] = {};
VMCHAR text44[100] = {};
VMCHAR text55[100] = {};

VMCHAR text222[100] = {};
VMCHAR text333[100] = {};
VMCHAR text444[100] = {};

VMCHAR text6[100];
VMCHAR text3[41] = "IPoverObexTest\n\n";

VMCHAR text4[250] =  "about      - show program information\nhelp       - list available commands\nlistbt     - list paired BT connections\ndisconnect - disconnect BT\nconnect    - connect to BT by index\n";
VMCHAR text5[250] =  "export     - save buffer to txt file\ncls        - clear screen\nexit       - quits the program\n\n";

VMCHAR text88[44] = "The syntax of the command is incorrect.\n";
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

bool remote_mode = false; 

static char buf[1024];

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

ChatUiState chatUiState = ChatUiState::StartupMenu;
int selected_peer_index = -1;
int last_peer_index = -1;
char active_peer_name[80] = "Peer";
char last_peer_name[80] = "";
char local_sender_name[80] = "Me";

bool remote_disconnect_message = false;

static VMINT layer_hdl[1];
static VMWCHAR left_label[32];
static VMWCHAR right_label[32];
static VMWCHAR center_label[32];
static VMWCHAR center_label1[32];
VMINT s_width = 0;
VMINT s_height = 0;

vm_editor_font_attribute my_font = {0, 0, 0, 16}; //16 18

static vm_input_mode_enum my_input_modes_lower_first[] = {VM_INPUT_MODE_MULTITAP_FIRST_UPPERCASE_ABC, VM_INPUT_MODE_123, VM_INPUT_MODE_123_SYMBOLS, VM_INPUT_MODE_NONE};

static VMINT32 history_editor = 0;
static VMINT32 input_editor   = 0;
static VMUWSTR history_buf = NULL;
static VMINT history_buf_bytes = 4096;     // 32 KB 32768
static VMUWSTR input_buf = NULL;
static VMINT input_buf_bytes = 512; //2048
static VMINT history_h;
static VMINT input_h = 64;
VMWCHAR outfile[100] = {0};

VMWCHAR menuw1[128];
VMCHAR menu1[128] = "Enter mode # from the list:\n1. Listen\n2. Connect to device from list\n3. Connect last used\n";
VMWCHAR menuw2[128];
VMCHAR menu2[128] = "Enter device # from the list:\n";
VMWCHAR menuw3[128];
VMCHAR menu3[128] = "No paired devices found.\n\n";
VMWCHAR menuw4[128];
VMCHAR menu4[128] = "Please turn on bluetooth first.\n\n";
VMWCHAR menuw5[128];
VMCHAR menu5[128] = "Connection timed out.\n\n";

VMWCHAR local_sender_namew[80];

static VMUINT16 rx_len = 0;
static VMUINT16 rx_pos = 0;

static VMUINT8 len_buf[2];
static VMUINT8 len_pos = 0;

static VMWCHAR rx_buf[512];

static void init_local_sender_name_from_bt() {
    snprintf(local_sender_name, sizeof(local_sender_name), "%s", "Device");

    vm_srv_bt_cm_dev_struct host_info = {};
    if (vm_btcm_get_host_dev_info(&host_info) < 0) {
        return;
    }

    size_t name_length = 0;
    const size_t max_host_name_length = sizeof(host_info.name);
    const size_t max_local_name_length = sizeof(local_sender_name) - 1;

    while (name_length < max_host_name_length && host_info.name[name_length] != '\0') {
        ++name_length;
    }

    if (name_length > max_local_name_length) {
        name_length = max_local_name_length;
    }

    if (name_length > 0) {
        memcpy(local_sender_name, host_info.name, name_length);
        local_sender_name[name_length] = '\0';
    }
}

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
void allocate_buffers(void);
VMUINT32 ime_cb(VMINT32 h, vm_editor_message_struct_p m);
void append_history(VMWSTR msg);
void send_message(void);
void clear_history(void);
void close_and_exit(void);
void activate_input(void);
void activate_history(void);
void create_auto_filename(VMWSTR text);
static VMINT save_history_to_utf8_chunked(VMWSTR outfile_path);
void save_history(void);
void append_history_ascii(const char* text);
int gui_printf(const char* format, ...);
void send_ucs2_packet(VMWSTR msg);
void process_ucs2_rx(void);

VMINT parseText(VMSTR text);
VMINT parseText1(VMSTR text);
VMINT parseText2(VMSTR text);
void timer1(int a);
void trim(char* result_data, size_t result_size, const char* input_data);
VMINT cb(VMINT act, VMUINT32 total, VMUINT32 completed, VMINT hdl);
void trim_left_symbols(char* result_data, const char* input_data);
void stringReverse(char* str);
void trim_single_spec_symb(char* result, const char* input);
//int cprintf(char const* const format, ...);

void cmd_exit(const CommandContext& ctx);
void execute_command(const ParsedCommand& cmd);
ParsedCommand parse_command(const char* input);
static void watchdog_timer(VMINT tid);
void cleanup_resources();

void prompt_tick();
void process_local_command(const char*);
void print_bt_address(int num);
void set_bt_address(int num_index);
void print_bt_address1(int num);
void show_start_menu();
void show_reconnect_menu();
bool ensure_bt_ready();
void start_network_loop();
void cmd_start_network(const CommandContext& ctx);
void cmd_stop_network(const CommandContext& ctx);
void cmd_listbt(const CommandContext& ctx);

CommandHandler commands[] = {
//    {"help", cmd_help},
//    {"about", cmd_about},
//    {"cls", cmd_cls},
    {"exit", cmd_exit},
//    {"line", cmd_line},
//    {"cd", cmd_cd},
//    {"dir", cmd_dir},
//    {"echo", cmd_echo},
//    {"remote", cmd_remote},
//    {"local", cmd_local},
    {"connect", cmd_start_network},
    {"disconnect", cmd_stop_network},
    {"listbt", cmd_listbt},
//    {"export", cmd_export},
};


void vm_main(void)
{
    layer_hdl[0] = -1;

    vm_reg_sysevt_callback(handle_sysevt);

    allocate_buffers();

    if(!history_buf || !input_buf)
    {
        vm_exit_app();
        return;
    }

    s_width = vm_graphic_get_screen_width();
    s_height = vm_graphic_get_screen_height();

    history_h = s_height - input_h - vm_editor_get_softkey_height();

    vm_ascii_to_ucs2(left_label, sizeof(left_label), (VMSTR)"Swich");
    vm_ascii_to_ucs2(center_label, sizeof(center_label), (VMSTR)"Send");
    vm_ascii_to_ucs2(center_label1, sizeof(center_label1), (VMSTR)"Save");
    vm_ascii_to_ucs2(right_label, sizeof(right_label), (VMSTR)"Clear");

    vm_ascii_to_ucs2(menuw1, sizeof(menuw1), menu1);
    vm_ascii_to_ucs2(menuw2, sizeof(menuw2), menu2);
    vm_ascii_to_ucs2(menuw3, sizeof(menuw3), menu3);
    vm_ascii_to_ucs2(menuw4, sizeof(menuw4), menu4);
    vm_ascii_to_ucs2(menuw5, sizeof(menuw5), menu5);
    vm_ascii_to_ucs2(local_sender_namew, sizeof(local_sender_namew), local_sender_name);

   if (vm_btcm_get_power_status() == VM_SRV_BT_CM_POWER_OFF) {
       bt_off = true;
   }

}

void timeout_f(int tid) {
    timeout++;  // Increase the timeout counter

    if (timeout > 22 && timeout < 24) {
//        console_str_in("\nTimed out, exiting...");
        append_history_ascii("\nTimed out, exiting...");
    }

    if (timeout > 25) {
        vm_exit_app();  // Exit
    }
}

void handle_sysevt(VMINT message, VMINT param)
{
    switch(message)
    {
    case VM_MSG_CREATE:
    case VM_MSG_ACTIVE:

        if(layer_hdl[0] < 0)
        {
            layer_hdl[0] = vm_graphic_create_layer(0, 0, s_width, s_height, -1);
            vm_graphic_set_clip(0, 0, s_width, s_height);
        }

        if(history_editor == 0)
        {
            history_editor = vm_editor_create(VM_EDITOR_MULTILINE, 0, 0, s_width, history_h, history_buf, history_buf_bytes, VM_FALSE, layer_hdl[0]);
//            vm_editor_set_bg_border_style(history_editor, VM_EDITOR_SINGLE_BORDER, 0x001F, 0x001F);
            vm_editor_set_bg_border_style(history_editor, VM_EDITOR_NO_BORDER, 0x001F, 0x001F);
            vm_editor_set_multiline_text_font(history_editor, my_font);
            vm_editor_set_IME(history_editor, VM_INPUT_TYPE_SENTENCE, my_input_modes_lower_first, VM_INPUT_MODE_MULTITAP_FIRST_UPPERCASE_ABC, ime_cb);
            vm_editor_set_softkey(history_editor, (VMUWSTR)left_label, VM_LEFT_SOFTKEY, activate_input);
//            vm_editor_set_softkey(history_editor, (VMUWSTR)center_label1, VM_CENTER_SOFTKEY, save_history);
            vm_editor_set_softkey(history_editor, (VMUWSTR)center_label, VM_CENTER_SOFTKEY, send_message);
            vm_editor_set_softkey(history_editor, (VMUWSTR)right_label, VM_RIGHT_SOFTKEY, clear_history);
            vm_editor_show(history_editor);
        }

        if(input_editor == 0)
        {
            input_editor = vm_editor_create(VM_EDITOR_MULTILINE, 0, history_h, s_width, input_h, input_buf, input_buf_bytes, VM_FALSE, layer_hdl[0]);
            vm_editor_set_bg_border_style(input_editor, VM_EDITOR_DOUBLE_BORDER, 0x07E0, 0x07E0);
            vm_editor_set_multiline_text_font(input_editor, my_font);
            vm_editor_set_IME(input_editor, VM_INPUT_TYPE_SENTENCE, my_input_modes_lower_first, VM_INPUT_MODE_MULTITAP_FIRST_UPPERCASE_ABC, ime_cb);
            vm_editor_set_softkey(input_editor, (VMUWSTR)left_label, VM_LEFT_SOFTKEY, activate_history);
            vm_editor_set_softkey(input_editor, (VMUWSTR)center_label, VM_CENTER_SOFTKEY, send_message);
            vm_editor_set_softkey(input_editor, (VMUWSTR)right_label, VM_RIGHT_SOFTKEY, clear_history);
            vm_editor_activate(input_editor, VM_FALSE);
        }
        vm_switch_power_saving_mode(turn_off_mode);


            if (startup == VM_FALSE) {
                startup = VM_TRUE;
                if (bt_off == true) {
//                    console_str_in("Please turn on bluetooth !\n");
                    append_history_ascii("Please turn on bluetooth !\n");
                } else {
//                    console_str_in("IPoverObex Bluetooth chat\n\n");
                    append_history_ascii("IPoverObex Bluetooth chat\n\n");
                    init_local_sender_name_from_bt();
                    vm_ascii_to_ucs2(local_sender_namew, sizeof(local_sender_namew), local_sender_name);
//                    cprintf("Using device name: %s\n\n", local_sender_name);
                    gui_printf("Using device name: %s\n\n", local_sender_name);
                    show_start_menu();
                }
            }

        break;

    case VM_MSG_PAINT:

        if(history_editor)
            vm_editor_show(history_editor);

        if(input_editor)
            vm_editor_show(input_editor);

        break;

    case VM_MSG_INACTIVE:

        vm_switch_power_saving_mode(turn_on_mode);

        cleanup_resources();

        if(input_editor)
            vm_editor_deactivate(input_editor);

        if(history_editor)
        {
            vm_editor_close(history_editor);
            history_editor = 0;
        }

        if(input_editor)
        {
            vm_editor_close(input_editor);
            input_editor = 0;
        }

        if(layer_hdl[0] != -1)
        {
            vm_graphic_delete_layer(layer_hdl[0]);
            layer_hdl[0] = -1;
        }

        break;

    case VM_MSG_QUIT:

        cleanup_resources();
        if (bt_initialized) {
            ipts.quit();
        }

        close_and_exit();
        break;
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

VMINT cb(VMINT act, VMUINT32 total, VMUINT32 completed, VMINT hdl) { return 0; }

void trim_left_symbols(char* result_data, const char* input_data) {
    strcpy(result_data, input_data);

    size_t len = strlen(result_data);

    while (len > 0 && result_data[len - 1] == '\\') {
        result_data[len - 1] = '\0';
        --len;
    }
}

void stringReverse(char* str) {
    int len = strlen(str);

    if (len > 1) {
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

void cmd_exit(const CommandContext& ctx) { vm_create_timer_ex(10, timer1); }

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

//    console_str_in(text);
    append_history_ascii(text);
}

ParsedCommand parse_command(const char* input) {
    ParsedCommand out = {};

    sscanf(input, "%63s %127s %127s %127[^\n]", out.cmd, out.arg1, out.arg2,
           out.arg3);

    return out;
}

static void watchdog_timer(VMINT tid)
{
    timer_id1 = -1;

    if (connState != ConnectionState::Connecting)
    {
        return;
    }

//    cprintf("Connection timed out.\n\n");
    append_history_ascii("Connection timed out.\n");
    append_history(menuw5);
    ipts.disconnect();

    connected = false;
    remote_mode = false;
    connState = ConnectionState::Disconnected;
    show_start_menu();
}

void cleanup_resources() {

    if (network_timer_id != -1) {
        vm_delete_timer_ex(network_timer_id);
        network_timer_id = -1;
    }

    if (timer_id1 != -1) {
        vm_delete_timer_ex(timer_id1);
        timer_id1 = -1;
    }

    if (layer_hdls[0] != -1) {
        vm_graphic_delete_layer(layer_hdls[0]);
        layer_hdls[0] = -1;
    }

    if (layer_hdls[1] != -1) {
        vm_graphic_delete_layer(layer_hdls[1]);
        layer_hdls[1] = -1;
    }

}

bool ensure_bt_ready() {
    if (vm_btcm_get_power_status() == VM_SRV_BT_CM_POWER_OFF) {
//        cprintf("Please turn on bluetooth first.\n\n");
        append_history_ascii("Please turn on bluetooth first.\n");
        append_history(menuw4);
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

    chatUiState = ChatUiState::StartupMenu;
    append_history(menuw1);

}

void show_connect_menu() {
    num = vm_btcm_get_dev_num(VM_SRV_BT_CM_RECENT_USED_DEV);
    if (num <= 0) {
//        cprintf("No paired devices found.\n\n");
        append_history_ascii("No paired devices found.\n");
        append_history(menuw3);
        show_start_menu();
        return;
    }

//    cprintf("Enter device # from the list:\n");
    append_history_ascii("Enter device # from the list:");
    append_history(menuw2);
    for (VMINT d = 0; d < num; d++) {
//        cprintf("%d: ", d + 1);
        gui_printf("%d: ", d + 1);
        print_bt_address(d);
    }
//    cprintf("\n");
    chatUiState = ChatUiState::ConnectSelection;
}

void start_network_loop() {
    if (network_timer_id != -1) {
        return;
    }

    network_timer_id = vm_create_timer_ex(33, [](int tid) {
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

//            cprintf("Connected with %s.\n", active_peer_name);
            gui_printf("Connected with %s.\n", active_peer_name);
        }

        if (opp_disconnected_event) {
            opp_disconnected_event = FALSE;

            connected = false;
            connState = ConnectionState::Disconnected;
            remote_mode = false;
            selected_peer_index = -1;

            if (remote_disconnect_message) {
                remote_disconnect_message = false;
            } else {
//                cprintf("Disconnected from %s.\n", active_peer_name);
                gui_printf("Disconnected from %s.\n", active_peer_name);
            }


//chatUiState = ChatUiState::StartupMenu;
//show_start_menu();


            show_reconnect_menu();
        }

        if (key_pending) {
            key_pending = false;
//            t2input.handle_keyevt(pending_event, pending_keycode);
        }

//        flush_outgoing_messages(); //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//        char rbuf[101];
//        size_t size;
//        do {
//            size = ipts.read(rbuf, 100); //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            if (size > 0) {
////                consume_incoming_bytes(rbuf, size); //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            }
//        } while (size > 0);

process_ucs2_rx();

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
//        cprintf("Sender set to: %s\n\n", local_sender_name);
        gui_printf("Sender set to: %s\n\n", local_sender_name);
        show_start_menu();
        return;
    }

    if (chatUiState == ChatUiState::Chat || chatUiState == ChatUiState::Listening) {
//        handle_chat_line(text_line); //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        return;
    }

    if (chatUiState == ChatUiState::StartupMenu) {
        if (strcmp(text_line, "1") == 0) {
            if (!ensure_bt_ready()) {
                return;
            }
//            cprintf("Listening for peer connection...\n");
            append_history_ascii("Listening for peer connection...\n");
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

        if (strcmp(text_line, "3") == 0) {
            if (!ensure_bt_ready()) {
                return;
            }

            num = vm_btcm_get_dev_num(VM_SRV_BT_CM_RECENT_USED_DEV);

            if (num <= 0) {
//                cprintf("No paired devices found.\n\n");
                append_history_ascii("No paired devices found.\n\n");
                return;
            }

            chatUiState = ChatUiState::Listening;
            cmd_start_network(CommandContext{"1", "", ""});
            return;
        }
 
//        cprintf("Unknown option.\n\n");
        append_history_ascii("Unknown option.\n\n");
        show_start_menu();
        return;
    }

    if (chatUiState == ChatUiState::ConnectSelection) {
        for (size_t i = 0; text_line[i] != '\0'; ++i) {
            if (!isdigit((unsigned char)text_line[i])) {
//                cprintf("Enter a device number.\n\n");
                append_history_ascii("Enter a device number.\n");
                return;
            }
        }

        int choice = atoi(text_line);
        if (choice <= 0 || choice > num) {
//            cprintf("Invalid device number.\n\n");
            append_history_ascii("Invalid device number.\n");
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

//            cprintf("Listening for peer connection...\n\n");
            append_history_ascii("Listening for peer connection...\n");
            chatUiState = ChatUiState::Listening;
            return;
        }

        if (strcmp(text_line, "2") == 0) {
            show_start_menu();
            return;
        }

//        cprintf("Unknown option.\n\n");
        append_history_ascii("Unknown option.\n");
//        show_reconnect_menu();
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
//       cprintf("%02X:%02X:%02X:%02X:%02X:%02X %s\n", nap1, nap0, uap, lap2, lap1, lap0, bt_info.name);
       gui_printf("%02X:%02X:%02X:%02X:%02X:%02X %s\n", nap1, nap0, uap, lap2, lap1, lap0, bt_info.name);
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
//       cprintf("Connect: %02X:%02X:%02X:%02X:%02X:%02X %s\n", nap1, nap0, uap, lap2, lap1, lap0, bt_info.name);
       gui_printf("Connect: %02X:%02X:%02X:%02X:%02X:%02X %s\n", nap1, nap0, uap, lap2, lap1, lap0, bt_info.name);
    }

}

void cmd_start_network(const CommandContext& ctx) {

    const char* arg1 = ctx.arg1 ? ctx.arg1 : "";
    const char* arg2 = ctx.arg2 ? ctx.arg2 : "";

if (connState == ConnectionState::Disconnecting)
{
//    cprintf("Please wait for disconnect to complete.\n\n");
    append_history_ascii("Please wait for disconnect to complete.\n");
    return;
}

if (connState == ConnectionState::Connecting)
{
//    cprintf("Already connecting.\n\n");
    append_history_ascii("Already connecting.\n");
    return;
}


    if (connState == ConnectionState::Connected) {
        show_msg = true;
        sprintf(text_pcr, "%s", "Disconnect first !\n\n"); // ?????????????????????????????????????????????
        if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
//           cprintf("Disconnect first !\n\n");
           append_history_ascii("Disconnect first !\n");
        }
        return;
    }

    num = vm_btcm_get_dev_num(VM_SRV_BT_CM_RECENT_USED_DEV);

    if (num <= 0) {
        bt_empty = true;
        show_msg = true;
        sprintf(text_pcr, "%s", "Need initialy pair with BT host !\n\n"); //????????????????????????????????????
        if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
//           cprintf("Need initialy pair with BT host !\n\n");
           append_history_ascii("Need initialy pair with BT host !\n");
        }
        return;

    } else if (vm_btcm_get_power_status() == VM_SRV_BT_CM_POWER_OFF) {
        bt_off = true;
        show_msg = true;
        sprintf(text_pcr, "%s", "Please turn on bluetooth !\n\n"); //?????????????????????????????????
        if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
//           cprintf("Please turn on bluetooth !\n\n");
           append_history_ascii("Please turn on bluetooth !\n");
        }
        return;

    } else if (strlen(arg1) != 0 && strlen(arg2) != 0) {
        show_msg = true;
        sprintf(text_pcr, "%s", "Incorrect connection number !\n\n"); //??????????????????????????????
        if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
//           cprintf("Incorrect connection number !\n\n");
           append_history_ascii("Incorrect connection number !\n");
        }
        return;

    } else {
        int conn_num = 1;

if (arg1[0] != '\0') {

    for (size_t i = 0; arg1[i] != '\0'; ++i) {
        if (!isdigit((unsigned char)arg1[i])) {
//            cprintf("Incorrect connection number !\n\n");
            append_history_ascii("Incorrect connection number !\n");
            return;
        }
    }

    conn_num = atoi(arg1);
}

        if (conn_num <= 0 || conn_num > num) {
           if (layer_hdls[0] != -1 && layer_hdls[1] != -1) {
//              cprintf("Incorrect connection number !\n\n");
               append_history_ascii("Incorrect connection number !\n");
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

connState = ConnectionState::Connecting;
connected = false;

ipts.connectBT(my_mac);

if (timer_id1 == -1)
{
    timer_id1 = vm_create_timer_ex(10000, watchdog_timer); // 10 sec
}

    start_network_loop();
}


void cmd_stop_network(const CommandContext&)
{

if (connState == ConnectionState::Disconnecting)
{
//    cprintf("Already disconnecting.\n\n");
    append_history_ascii("Already disconnecting.\n\n");
    return;
}

    if (connState != ConnectionState::Connected)
    {
//        cprintf("Not connected.\n\n");
        append_history_ascii("Not connected.\n\n");
        return;
    }

    connState = ConnectionState::Disconnecting;

    ipts.disconnect();

//    cprintf("Disconnecting...\n\n");
    append_history_ascii("Disconnecting...\n\n");
}

void cmd_listbt(const CommandContext& ctx) {

    if (strlen(ctx.arg1) == 0) {
//        cprintf("Available connections:\n");
        append_history_ascii("Available connections:");
        VMINT d;

        num = vm_btcm_get_dev_num(VM_SRV_BT_CM_RECENT_USED_DEV);
        for (d = 0; d < num; d++) {
//            cprintf("%d: ", d + 1);
            gui_printf("%d: ", d + 1);
            print_bt_address(d);
        } 
//        cprintf("\n");
    } else {
//        cprintf(text88);
        append_history_ascii(text88);
    }

}

void allocate_buffers(void)
{
    history_buf = (VMUWSTR)vm_malloc(history_buf_bytes);
    input_buf = (VMUWSTR)vm_malloc(input_buf_bytes);

    if(history_buf)
        history_buf[0] = 0;

    if(input_buf)
        input_buf[0] = 0;
}

void append_history(VMWSTR msg)
{

    if (!history_buf || !msg || msg[0] == 0)
        return;

    VMWCHAR nl[2];
    VMINT used = vm_wstrlen((VMWSTR)history_buf);
    VMINT add  = vm_wstrlen(msg);

    if ((used + add + 2) * sizeof(VMWCHAR) >= history_buf_bytes)
    {
        return;
    }

//    vm_wstrcat((VMWSTR)history_buf, (VMWSTR)msg);
    vm_wstrcat((VMWSTR)history_buf, msg);

    nl[0] = '\n';
    nl[1] = 0;

//    vm_wstrcat((VMWSTR)history_buf, (VMWSTR)nl);
    vm_wstrcat((VMWSTR)history_buf, nl);

    //if(history_editor)
    //{
       //vm_editor_show(history_editor);
    vm_editor_set_text(history_editor, history_buf, history_buf_bytes);
    vm_editor_show(history_editor);
    vm_editor_show(input_editor);
   //}

}

void send_message(void) {

    VMWCHAR wtext[256];
    VMWCHAR packet[512];

    vm_editor_get_text(input_editor, (VMUWSTR)wtext, sizeof(wtext));

    if (vm_wstrlen(wtext) == 0) 
        return;

    input_buf[0] = 0;
    vm_editor_set_text(input_editor, input_buf, input_buf_bytes);

//    vm_editor_show(input_editor);
    vm_editor_show(history_editor);

    if (chatUiState != ChatUiState::Chat) {
        char text[256];

        vm_ucs2_to_ascii(text, sizeof(text), wtext);

        process_local_command(text);
    } else {
        packet[0] = 0;

        VMWCHAR wtmp[32];

        if (vm_wstrlen(local_sender_namew) + vm_wstrlen(wtext) + 8 >= 512) {
            return;
        }

        vm_ascii_to_ucs2(wtmp, sizeof(wtmp), (VMSTR) "[");
        vm_wstrcat(packet, wtmp);
        vm_wstrcat(packet, local_sender_namew);
        vm_ascii_to_ucs2(wtmp, sizeof(wtmp), (VMSTR) "]: ");
        vm_wstrcat(packet, wtmp);
        vm_wstrcat(packet, wtext);

        if (connState != ConnectionState::Connected) {
            append_history_ascii("Not connected");
            return;
        }

        send_ucs2_packet(packet);
        append_history(packet);
    }

}

void clear_history(void){}

void close_and_exit(void)
{

    if(history_editor)
    {
        vm_editor_close(history_editor);
        history_editor = 0;
    }

    if(input_editor)
    {
        vm_editor_close(input_editor);
        input_editor = 0;
    }

    if(history_buf)
    {
        vm_free(history_buf);
        history_buf = NULL;
    }

    if(input_buf)
    {
        vm_free(input_buf);
        input_buf = NULL;
    }

    if(layer_hdl[0] != -1)
    {
       vm_graphic_delete_layer(layer_hdl[0]);
       layer_hdl[0] = -1;
    }

    vm_exit_app();
}

VMUINT32 ime_cb(VMINT32 h, vm_editor_message_struct_p m) {

    if (!m) return 0;
    switch (m->message_id) {
        case VM_EDITOR_MESSAGE_ACTIVATE:
        case VM_EDITOR_MESSAGE_REDRAW_FLOATING_UI:
        case VM_EDITOR_MESSAGE_REDRAW_IMUI_RECTANGLE:
            //update_center_label_from_buffer();
            //if(history_editor)
            //    vm_editor_show(history_editor);
            //    vm_editor_redraw_ime_screen();
            //}

            //if(input_editor)
            //    vm_editor_show(input_editor);
            //    vm_editor_redraw_ime_screen();
            //}
            break;
        case VM_EDITOR_MESSAGE_DEACTIVATE:
            break;
        default:
            break;
    }
    return 0;
}

void activate_input(void)
{
//    vm_editor_set_softkey(history_editor, (VMUWSTR)center_label, VM_CENTER_SOFTKEY, send_message);
    vm_editor_activate(input_editor, VM_FALSE);
    vm_editor_set_pos(history_editor, 0, 0);
    vm_editor_set_size(history_editor, s_width, history_h);
    vm_editor_set_pos(input_editor, 0, history_h);
    vm_editor_set_size(input_editor, s_width,input_h);
    vm_editor_show(history_editor);
    vm_editor_show(input_editor);
}

void activate_history(void)
{
//    vm_editor_set_softkey(history_editor, (VMUWSTR)center_label1, VM_CENTER_SOFTKEY, save_history);
    vm_editor_activate(history_editor, VM_FALSE);
    vm_editor_set_pos(history_editor, 0, 0);
    vm_editor_set_size(history_editor, s_width, s_height - vm_editor_get_softkey_height());
    vm_editor_set_pos(input_editor, 0, s_height);
    vm_editor_set_size(input_editor, 0, 0);
    vm_editor_show(history_editor);
}

void create_auto_filename(VMWSTR text) {

    VMINT drv;
    VMCHAR fAutoFileName[100] = {0};
    VMCHAR fData_text[100] = {0};
    struct vm_time_t curr_time;

    vm_get_time(&curr_time);

    if ((drv = vm_get_removable_driver()) < 0) {
       drv = vm_get_system_driver();
    }

    sprintf(fAutoFileName, "%c:\\", drv);
    sprintf(fData_text, "%02d%02d%02d%02d%02d.txt", curr_time.mon, curr_time.day, curr_time.hour, curr_time.min, curr_time.sec);
    strcat(fAutoFileName, fData_text);
    vm_ascii_to_ucs2(text, (strlen(fAutoFileName) + 1) * 2, fAutoFileName);
}

static VMINT save_history_to_utf8_chunked(VMWSTR outfile_path)
{
    if (!outfile_path)
        return -1;

    VMFILE fh = vm_file_open(outfile_path, MODE_CREATE_ALWAYS_WRITE, FALSE);

    if (fh < 0)
        return -2;

    VMWCHAR *u_chunk = (VMWCHAR *)vm_calloc((UCS2_CHUNK_WORDS + 4) * sizeof(VMWCHAR));

    VMCHAR *out_utf8 = (VMCHAR *)vm_calloc(VM_CHSET_DEST_BYTES + 8);

    if (!u_chunk || !out_utf8)
    {
        if (u_chunk)
            vm_free(u_chunk);

        if (out_utf8)
            vm_free(out_utf8);

        vm_file_close(fh);

        return -3;
    }

    VMINT history_len = vm_wstrlen((VMWSTR)history_buf);

    VMINT remaining = history_len;
    VMINT pos = 0;

    while (remaining > 0)
    {
        VMINT take = (remaining > UCS2_CHUNK_WORDS) ? UCS2_CHUNK_WORDS : remaining;

        vm_wstrncpy(u_chunk, (VMWSTR)history_buf + pos, take);

        u_chunk[take] = 0;

        VMINT conv = vm_chset_convert(VM_CHSET_UCS2, VM_CHSET_UTF8, (VMCHAR *)u_chunk, (VMCHAR *)out_utf8, VM_CHSET_DEST_BYTES);

        if (conv != VM_CHSET_CONVERT_SUCCESS &&
            conv != 0)
        {
            vm_free(u_chunk);
            vm_free(out_utf8);
            vm_file_close(fh);

            return -4;
        }

        VMUINT written = 0;

        vm_file_write(fh, out_utf8, (VMUINT)strlen((VMSTR)out_utf8), &written);

        pos += take;
        remaining -= take;
    }

    vm_free(u_chunk);
    vm_free(out_utf8);

    vm_file_close(fh);

    return 0;
}

void save_history(void)
{
    create_auto_filename(outfile);
    save_history_to_utf8_chunked(outfile);
}

void append_history_ascii(const char* text)
{
    if (!text)
        return;

    VMWCHAR wbuf[512];

    vm_ascii_to_ucs2(wbuf, sizeof(wbuf), (VMSTR)text);

    append_history(wbuf);
}

int gui_printf(const char* format, ...)
{
    char buf[512];

    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    append_history_ascii(buf);

    return ret;
}

void send_ucs2_packet(VMWSTR msg)
{
    if (!msg)
        return;

    VMUINT16 payload_len = (vm_wstrlen(msg) + 1) * sizeof(VMWCHAR);

    ipts.write((char *)&payload_len, sizeof(payload_len));

    ipts.write((char *)msg, payload_len);


//VMUINT8 hdr[2];  //Change to because. send_ucs2_packet() endian issue. This witout works only if both phones use the same endian format.
//hdr[0] = payload_len & 0xFF;
//hdr[1] = (payload_len >> 8) & 0xFF;
//ipts.write((char*)hdr, 2);

}

void process_ucs2_rx(void)
{
    char tmp[128];

    size_t n;

    while ((n = ipts.read(tmp, sizeof(tmp))) > 0)
    {
        size_t p = 0;

        while (p < n)
        {
            /* receive 2-byte length field */
            if (rx_len == 0)
            {
                while (len_pos < 2 && p < n)
                {
                    len_buf[len_pos++] = (VMUINT8)tmp[p++];
                }

                if (len_pos < 2)
                {
                    break;
                }

                memcpy(&rx_len, len_buf, 2);
//rx_len = ((VMUINT16)len_buf[0]) | (((VMUINT16)len_buf[1]) << 8); //Change to because. send_ucs2_packet() endian issue. This witout works only if both phones use the same endian format.

                len_pos = 0;
                rx_pos = 0;

                if (rx_len == 0 ||
                    rx_len > sizeof(rx_buf))
                {
                    rx_len = 0;
                    continue;
                }
            }

            /* receive payload */
            VMUINT16 need  = rx_len - rx_pos;
            VMUINT16 avail = (VMUINT16)(n - p);

            VMUINT16 copy = (need < avail) ? need : avail;

            memcpy(((char*)rx_buf) + rx_pos, tmp + p, copy);

            rx_pos += copy;
            p += copy;

            if (rx_pos == rx_len)
            {

if (rx_len >= 2)
{
    ((char*)rx_buf)[rx_len - 1] = 0;
    ((char*)rx_buf)[rx_len - 2] = 0;
}

                append_history((VMWSTR)rx_buf);

                rx_len = 0;
                rx_pos = 0;
            }
        }
    }
}

void show_reconnect_menu() {
    append_history_ascii("\nConnection closed.\n");
    if (last_peer_index >= 0) {
        gui_printf("Last peer: %s\n", last_peer_name[0] ? last_peer_name : "Peer");
        append_history_ascii("1. Reconnect\n");
    } else {
        append_history_ascii("1. Listen\n");
    }
    append_history_ascii("2. New connection\n\n");
    chatUiState = ChatUiState::ReconnectMenu;
}
