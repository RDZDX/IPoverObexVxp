//#include <vmsys.h>
//#include <vmio.h>
//#include <vmstdlib.h>
#include <vmbtcm.h>
#include <vmchset.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
//#include <console.h>
#include <string.h>
static int debug_printf(char const* const format, ...);
#include "opp.h"
#include "opp_types.h"

volatile VMBOOL disconnect_complete = TRUE;
volatile VMBOOL opp_connected_event = FALSE;
volatile VMBOOL opp_disconnected_event = FALSE;

extern VMUINT32 bt_obex_events_base;
#define FIX_OBEX_EVENT(x) ((x) + bt_obex_events_base)

static VMINT32 connc_id = -1;
static VMINT32 conns_id = -1;

static VMINT32 obexc_id = -1;
static VMINT32 obexs_id = -1;


static VMINT32 opcc_mtu = 0;
static VMINT32 opcs_mtu = 0;

static void* obex_send_buf = 0;
static void* send_buf = 0;
static void* receive_buf = 0;

static VMINT32 send_buf_pos = 0;
static VMINT32 receive_buf_pos = 0;

static VMBOOL wait_data_to_send = 0;
static VMBOOL wait_space_to_receive = 0;

static VMBOOL is_connected = 0;

static char buf[1024];

static const char* bt_pw_st[4] = {
	"On", "Off", "Switching on", "Switching off"
};

static const char* bt_obex_events_names[40] = {
	"GOEP_REGISTER_SERVER_REQ",
	"GOEP_REGISTER_SERVER_RSP",
	"GOEP_DEREGISTER_SERVER_REQ",
	"GOEP_DEREGISTER_SERVER_RSP",
	"GOEP_CONNECT_IND",
	"GOEP_CONNECT_RES",
	"GOEP_PUSH_IND",
	"GOEP_PUSH_RES",
	"GOEP_PULL_IND",
	"GOEP_PULL_RES",
	"GOEP_SET_FOLDER_IND",
	"GOEP_SET_FOLDER_RES",
	"GOEP_ABORT_IND",
	"GOEP_ABORT_RES",
	"GOEP_AUTH_REQ",
	"GOEP_AUTH_RSP",
	"GOEP_REGISTER_CLIENT_REQ",
	"GOEP_REGISTER_CLIENT_RSP",
	"GOEP_DEREGISTER_CLIENT_REQ",
	"GOEP_DEREGISTER_CLIENT_RSP",
	"GOEP_CONNECT_REQ",
	"GOEP_CONNECT_RSP",
	"GOEP_PUSH_REQ",
	"GOEP_PUSH_RSP",
	"GOEP_PULL_REQ",
	"GOEP_PULL_RSP",
	"GOEP_SET_FOLDER_REQ",
	"GOEP_SET_FOLDER_RSP",
	"GOEP_ABORT_REQ",
	"GOEP_ABORT_RSP",
	"GOEP_DISCONNECT_REQ",
	"GOEP_DISCONNECT_RSP",
	"GOEP_DISCONNECT_IND",
	"GOEP_DISCONNECT_RES",
	"GOEP_TPDISCONNECT_REQ",
	"GOEP_AUTH_IND",
	"GOEP_AUTH_RES",
	"GOEP_OPP_SUPPORTED_FORMATS_IND",
	"GOEP_AUTHORIZE_IND",
	"GOEP_AUTHORIZE_RES",
};

//void create_app_txt_filenamex(VMWSTR text, VMSTR extt);
//void log_debug(const char* fmt, ...);

void bt_mac_conv(VMUINT8* out, VMUINT8* in) { //TODO move to another place
	out[0] = in[5], out[1] = in[4], out[2] = in[3], out[3] = 0x00;
	out[4] = in[2], out[5] = 0x0F;  // 5th byte is supposedly an alignment byte.
	out[6] = in[1], out[7] = in[0];
}

static void print_hex(const char* s, unsigned char* h, int l) {
	DEBUG_PRINTF(s);
	for (int i = 0; i < l; ++i)
		DEBUG_PRINTF("%02X ", h[i]);
	DEBUG_PRINTF("\n");
}

static VMUINT16 put_name[128];
static VMSTR put_mime[80] = { 0 };
static void oppc_connect_rsp_handler(void* msg) {

//DEBUG_PRINTFZ("CONNECT_RSP id=%d\n", rsp->goep_conn_id);
//log_debug("CONNECT_RSP id=%d\n", rsp->goep_conn_id);

	goep_connect_rsp_struct* rsp = (goep_connect_rsp_struct*)msg;

	if (rsp->rsp_code == GOEP_STATUS_SUCCESS)
	{
		if (rsp->peer_mru != 0 && rsp->peer_mru < OBEX_SEND_BUF)
			opcc_mtu = rsp->peer_mru;
		else
			opcc_mtu = OBEX_SEND_BUF;

		//cprintf("opcc_mtu: %d\n", opcc_mtu);

#ifdef REGISTER_CONN
		srv_bt_cm_connect_ind(rsp->req_id);
#endif // REGISTER_CONN

		obexc_id = rsp->goep_conn_id;

		srv_oppc_send_push_req(obexc_id, GOEP_FIRST_PKT, 0x7FFFFFFF, put_name, put_mime, 0, 0);
	}
	else
	{
#ifdef REGISTER_CONN
		srv_bt_cm_stop_conn(rsp->req_id);
#endif // REGISTER_CONN
	}

//log_debug("CONNECT_RSP id=%d\n", rsp->goep_conn_id);
}

static void send_from_buf() {

    if (obexc_id < 0)
    {
//        log_debug("send_from_buf aborted obexc_id=%d\n", obexc_id);
        return;
    }

 //   log_debug(
 //       "send_from_buf conn=%d size=%d\n",
 //       obexc_id,
 //       send_buf_pos
 //   );

	VMUINT32 size = send_buf_pos;
	if (size > opcc_mtu)
		size = opcc_mtu;

	memcpy(obex_send_buf, send_buf, size);
	//memmove(send_buf, (char*)send_buf + size, size);
        memmove(send_buf, (char*)send_buf + size, send_buf_pos - size);
	send_buf_pos -= size;

	srv_oppc_send_push_req(obexc_id, GOEP_NORMAL_PKT, 0x7FFFFFFF, 0, put_name, obex_send_buf, size);

	wait_data_to_send = FALSE;
}

static void oppc_push_rsp_handler(void* msg) {
	goep_push_rsp_struct* rsp = (goep_push_rsp_struct*)msg;

	if (rsp->rsp_code != GOEP_STATUS_SUCCESS)
	{
		srv_oppc_send_disconnect_req(rsp->goep_conn_id, TRUE);
		return;
	}


	if (send_buf_pos) {
		send_from_buf();
	}
	else
		wait_data_to_send = TRUE;
}

static void opps_general_rsp(VMUINT32 msg_id, VMUINT8 conn_id, VMUINT8 rsp_code) {
	goep_res_struct* res;
	res = (goep_res_struct*)construct_local_para(sizeof(*res), 0);

	res->goep_conn_id = conn_id;
	res->rsp_code = rsp_code;

	srv_opp_send_ilm(FIX_OBEX_EVENT(msg_id), res);
}

static void opps_authorize_ind_hdler(void* msg) {
	goep_authorize_ind_struct* ind = (goep_authorize_ind_struct*)msg;

	obexs_id = ind->goep_conn_id;

#ifdef REGISTER_CONN
	conns_id = srv_bt_cm_start_conn(TRUE, 0x1105, &(ind->bd_addr), (VMINT8*)ind->dev_name);
#endif // REGISTER_CONN

	opps_general_rsp(GOEP_AUTHORIZE_RES, ind->goep_conn_id, GOEP_STATUS_SUCCESS);
}

static void opps_connect_ind_handler(void* msg) {
	goep_connect_ind_struct* ind = (goep_connect_ind_struct*)msg;

//    log_debug("CONNECT_IND id=%d peer_mru=%d\n", ind->goep_conn_id, ind->peer_mru);

	opcs_mtu = ind->peer_mru;

	//cprintf("opcs_mtu: %d\n", opcs_mtu);

	is_connected = TRUE;
        opp_connected_event = TRUE;

    wait_data_to_send = TRUE;

	opps_general_rsp(GOEP_CONNECT_RES, ind->goep_conn_id, GOEP_STATUS_SUCCESS);

#ifdef REGISTER_CONN
	srv_bt_cm_connect_ind(conns_id);
#endif // REGISTER_CONN
}


static void opps_push_ind_handler(void* msg) {
	goep_push_ind_struct* ind = (goep_push_ind_struct*)msg;

	DEBUG_PRINTF("opps_push_ind_handler(%d)\n", ind->frag_len);

	{
		memcpy((char*)receive_buf + receive_buf_pos, ind->frag_ptr, ind->frag_len);
		receive_buf_pos += ind->frag_len;

		if (RECEIVE_BUF - receive_buf_pos < opcs_mtu)
			wait_space_to_receive = TRUE;
		else
			opps_general_rsp(GOEP_PUSH_RES, ind->goep_conn_id, GOEP_STATUS_SUCCESS);
	}
}

static void opp_disconnect_ind_handler(void* msg)
{

disconnect_complete = TRUE;

//log_debug("DISCONNECT_IND\n");

	goep_disconnect_ind_struct* goep_disconnect_ind = (goep_disconnect_ind_struct*)msg;

    is_connected = FALSE;
    opp_disconnected_event = TRUE;

    obexc_id = -1;
    obexs_id = -1;

    send_buf_pos = 0;
    receive_buf_pos = 0;

    wait_data_to_send = FALSE;
    wait_space_to_receive = FALSE;

}

//static VMUINT8 opp_event_handler(int msg_id, void* msg) {

//    log_debug(
//        "EVENT %d\n",
//        msg_id - bt_obex_events_base
//    );

//	DEBUG_PRINTF("event(%d, %s)\n", msg_id, bt_obex_events_names[msg_id - bt_obex_events_base]);
//	switch (msg_id - bt_obex_events_base) {
//	case GOEP_CONNECT_RSP:
//		oppc_connect_rsp_handler(msg);
//		break;
//	case GOEP_PUSH_RSP:
//		oppc_push_rsp_handler(msg);
//		break;
//	case GOEP_AUTHORIZE_IND:
//		opps_authorize_ind_hdler(msg);
//		break;
//	case GOEP_CONNECT_IND:
//		opps_connect_ind_handler(msg);
//		break;
//	case GOEP_PUSH_IND:
//		opps_push_ind_handler(msg);
//		break;
//	case GOEP_DISCONNECT_IND:
//		opp_disconnect_ind_handler(msg);
//		break;
//	}
//}

static VMUINT8 opp_event_handler(int msg_id, void* msg)
{
    int evt = msg_id - bt_obex_events_base;

//    log_debug("EVENT %d (%s)\n", evt, bt_obex_events_names[evt]);

    switch (evt)
    {
        case GOEP_CONNECT_RSP:
            oppc_connect_rsp_handler(msg);
            break;

        case GOEP_PUSH_RSP:
            oppc_push_rsp_handler(msg);
            break;

        case GOEP_AUTHORIZE_IND:
            opps_authorize_ind_hdler(msg);
            break;

        case GOEP_CONNECT_IND:
            opps_connect_ind_handler(msg);
            break;

        case GOEP_PUSH_IND:
            opps_push_ind_handler(msg);
            break;

        case GOEP_DISCONNECT_IND:
            opp_disconnect_ind_handler(msg);
            break;
    }
}

static VMUINT8 opp_msg_handler(void* msg, int src_mod, ilm_struct* ilm) {

//    log_debug("MSG id=%d\n", ilm->msg_id);

	opp_event_handler(ilm->msg_id, msg);
}

static void oppc_send_connect_req(VMINT conn_id, VMUINT8 buf, VMUINT16 buf_size, vm_srv_bt_cm_bt_addr* addr) {

//log_debug("CONNECT_REQ\n");

	goep_connect_req_struct* req;

	req = (goep_connect_req_struct*)construct_local_para(sizeof(*req), 0);

	req->uuid[0] = '\0';
	req->uuid_len = 0;
	req->req_id = conn_id;
	req->bd_addr = *addr;
	req->tp_type = 0;
	req->buf_ptr = buf;
	req->buf_size = buf_size;
	req->auth_use = 0;
	req->passwd_len = 0;
	req->realm_len = 0;

	srv_opp_send_ilm(FIX_OBEX_EVENT(GOEP_CONNECT_REQ), req);
}

#define MALLOC_ASSERT(x) if (!x) return FALSE;

VMBOOL bt_opp_init() {

//log_debug("bt_obex_events_base=%d\n", bt_obex_events_base);

	vm_ascii_to_ucs2(put_name, 128 * 2, "IpOverObex.txt");

	obex_send_buf = vm_malloc(OBEX_SEND_BUF);
	MALLOC_ASSERT(obex_send_buf);
	send_buf = vm_malloc(SEND_BUF);
	MALLOC_ASSERT(send_buf);
	receive_buf = vm_malloc(RECEIVE_BUF);
	MALLOC_ASSERT(receive_buf);
	PLATFORM_ASSERT();


	send_buf_pos = 0;
	receive_buf_pos = 0;

	DEBUG_PRINTF("BT statud: %s\n", bt_pw_st[vm_btcm_get_power_status()]);

	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_REGISTER_SERVER_RSP), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_DEREGISTER_SERVER_RSP), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_CONNECT_IND), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_PUSH_IND), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_PULL_IND), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_SET_FOLDER_IND), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_ABORT_IND), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_AUTHORIZE_IND), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_CONNECT_RSP), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_PUSH_RSP), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_PULL_RSP), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_SET_FOLDER_RSP), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_ABORT_RSP), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_DISCONNECT_IND), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_AUTH_IND), opp_msg_handler, 0);
	mmi_frm_set_protocol_event_handler(FIX_OBEX_EVENT(GOEP_OPP_SUPPORTED_FORMATS_IND), opp_msg_handler, 0);

	return TRUE;
}

VMBOOL bt_opp_connect(VMUINT8* mac) {

//log_debug("bt_obex_events_base=%d\n", bt_obex_events_base);

    if (!disconnect_complete)
    {
//        log_debug("CONNECT BLOCKED\n");
        return FALSE;
    }

//log_debug("bt_opp_connect old obexc_id=%d\n", obexc_id);

    is_connected = FALSE;

//    obexc_id = -1;
//    obexs_id = -1;

//    send_buf_pos = 0;
//    receive_buf_pos = 0;

	VMUINT8 mac8[8];
	bt_mac_conv(mac8, mac);
	print_hex("Mac:  ", mac, 6);
	print_hex("Mac8: ", mac8, 8);

	PLATFORM_ASSERT();

#ifdef REGISTER_CONN
	connc_id = srv_bt_cm_start_conn(FALSE, 0xfffd, mac8, NULL);
	DEBUG_PRINTF("connc_id = %d\n", connc_id);
#endif // REGISTER_CONN

//log_debug("re-register handlers\n");

mmi_frm_set_protocol_event_handler(
    FIX_OBEX_EVENT(GOEP_CONNECT_RSP),
    opp_msg_handler,
    0
);

mmi_frm_set_protocol_event_handler(
    FIX_OBEX_EVENT(GOEP_PUSH_RSP),
    opp_msg_handler,
    0
);

mmi_frm_set_protocol_event_handler(
    FIX_OBEX_EVENT(GOEP_CONNECT_IND),
    opp_msg_handler,
    0
);

mmi_frm_set_protocol_event_handler(
    FIX_OBEX_EVENT(GOEP_DISCONNECT_IND),
    opp_msg_handler,
    0
);

	oppc_send_connect_req(connc_id, obex_send_buf, OBEX_SEND_BUF, mac8);
}

VMBOOL bt_opp_disconnect() {

disconnect_complete = FALSE;

//log_debug("bt_opp_disconnect obexc_id=%d obexs_id=%d\n", obexc_id, obexs_id);


    is_connected = FALSE;

	if (obexc_id != -1)
		srv_oppc_send_disconnect_req(obexc_id, FALSE);
	if (obexs_id != -1)
		srv_oppc_send_disconnect_req(obexs_id, FALSE);

    return TRUE;

}

VMBOOL bt_opp_deinit() {
	PLATFORM_ASSERT();

	mmi_bt_obex_event_hdlr_init(); // return callbacks to standart

#ifdef REGISTER_CONN
	if (connc_id >= 0)
		srv_bt_cm_stop_conn(connc_id);

	if (conns_id >= 0)
		srv_bt_cm_stop_conn(conns_id);
#endif // REGISTER_CONN
}

VMBOOL bt_opp_is_connected() {
//	return is_connected;
    return (is_connected && obexc_id >= 0);
}

void bt_opp_flush() {
//	if (wait_data_to_send && RECEIVE_BUF - receive_buf_pos > opcs_mtu)
//		send_from_buf();

    if (wait_data_to_send && send_buf_pos > 0)
    {
        send_from_buf();
    }

	if (wait_space_to_receive && SEND_BUF - send_buf_pos > opcs_mtu) {
		wait_space_to_receive = FALSE;
		opps_general_rsp(GOEP_PUSH_RES, obexs_id, GOEP_STATUS_SUCCESS);
	}
}

VMUINT32 bt_opp_get_free_size() {
	return SEND_BUF - send_buf_pos;
}

VMUINT32 bt_opp_write(const void* buf, VMUINT32 size) {

//    log_debug("bt_opp_write size=%d obexc_id=%d send_buf_pos=%d\n", size, obexc_id, send_buf_pos);

	VMUINT32 free_size = bt_opp_get_free_size();

	if (size > free_size)
		size = free_size;

	memcpy((char*)send_buf + send_buf_pos, buf, size);
	send_buf_pos += size;

//	if (send_buf_pos > opcc_mtu)
//		bt_opp_flush();

    bt_opp_flush();

	return size;
}


VMUINT32 bt_opp_read(void* buf, VMUINT32 size) {
	VMUINT32 used_size = receive_buf_pos;
	if (size > used_size)
		size = used_size;

	memcpy(buf, receive_buf, size);
	if (used_size - size)
		memmove(receive_buf, (char*)receive_buf + size, used_size - size);

	receive_buf_pos -= size;

	if (RECEIVE_BUF - receive_buf_pos > opcs_mtu)
		bt_opp_flush();

	return size;
}

void bt_opp_set_as_readed(VMUINT32 size) {
	VMUINT32 used_size = receive_buf_pos;
	if (size > used_size)
		abort();

	if (used_size - size)
		memmove(receive_buf, (char*)receive_buf + size, used_size - size);

	receive_buf_pos -= size;

	if (RECEIVE_BUF - receive_buf_pos > opcs_mtu)
		bt_opp_flush();
}

void* bt_opp_get_receive_buf() {
	return receive_buf;
}

VMUINT32 bt_opp_get_receive_size() {
	return receive_buf_pos;
}

//static char buf[1024];

static int debug_printf(const char *format, ...)
{
    va_list aptr;

    va_start(aptr, format);
    int ret = vsnprintf(buf, sizeof(buf), format, aptr);
    va_end(aptr);

    return ret;
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
////    vsprintf(buf, fmt, ap);
//    vsnprintf(buf, sizeof(buf), fmt, ap);
//    va_end(ap);

//    VMUINT nwrite;
//    vm_file_write(f, buf, strlen(buf), &nwrite);

//    vm_file_close(f);
//}

