/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_devicefind.c
 *
 * Description: Find your hardware's information while working any mode.
 *
 * Modification history:
 *     2014/3/12, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"

#include "espconn.h"
#include "user_json.h"
#include "user_devicefind.h"

static const char *const KEY_Action		= "action";
static const char *const KEY_Type		= "type";
static const char *const KEY_Ip			= "ip";
static const char *const KEY_Mac		= "mac";

static const char *const ACTION_Find	= "find";
static const char *const TYPE_Weight	= "weight";

LOCAL ICACHE_FLASH_ATTR
int json_get_string(struct jsonparse_state *state, char *name, int size) {
	jsonparse_next(state);
	jsonparse_next(state);
	return jsonparse_copy_value(state, name, size);
}

LOCAL ICACHE_FLASH_ATTR
bool is_call_me(const char *dat, unsigned short len) {
	bool flag = false;
	int type;
	struct jsonparse_state state;
	char *buf = (char *)os_malloc(len);
	jsonparse_setup(&state, dat, len);
	while ((type = jsonparse_next(&state)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			if (jsonparse_strcmp_value(&state, KEY_Action) == 0) {
				json_get_string(&state, buf, len);
				if (os_strcmp(buf, ACTION_Find) == 0) {
					flag = true;
				}
			} else if (jsonparse_strcmp_value(&state, KEY_Type) == 0) {
				json_get_string(&state, buf, len);
				if (os_strcmp(buf, TYPE_Weight) != 0) {
					flag = false;
				}
			}
		}
	}
	os_free(buf);
	return flag;
}

/*---------------------------------------------------------------------------*/
LOCAL struct espconn ptrespconn;

/******************************************************************************
 * FunctionName : user_devicefind_recv
 * Description  : Processing the received data from the host
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_devicefind_recv(void *arg, char *pusrdata, unsigned short length) {
	char DeviceBuffer[64] = {0};
	char Device_mac_buffer[60] = {0};
	char hwaddr[6];
	struct espconn *pconn = arg;

	struct ip_info ipconfig;
	if (pusrdata == NULL) {
		return;
	}
	do {
		if (wifi_get_opmode() != STATION_MODE) {
			wifi_get_ip_info(SOFTAP_IF, &ipconfig);
			wifi_get_macaddr(SOFTAP_IF, hwaddr);
			if (ip_addr_netcmp((struct ip_addr *)pconn->proto.udp->remote_ip, &ipconfig.ip, &ipconfig.netmask)) {
				break;
			}
		}
		wifi_get_ip_info(STATION_IF, &ipconfig);
		wifi_get_macaddr(STATION_IF, hwaddr);
	} while (0);

	if (is_call_me(pusrdata, length)) {
		char *buffer = (char *)os_malloc(jsonSize);
		int len = os_sprintf(buffer, "{\"%s\":\"%s\",\"%s\":\"" IPSTR "\",\"%s\":\"" MACSTR "\"}",
							 KEY_Type, TYPE_Weight,
							 KEY_Ip, IP2STR(&ipconfig.ip),
							 KEY_Mac, MAC2STR(hwaddr));
		espconn_sent(pconn, buffer, len);
		os_free(buffer);
	}
}

/******************************************************************************
 * FunctionName : user_devicefind_init
 * Description  : the espconn struct parame init
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_devicefind_init(void) {
	ptrespconn.type = ESPCONN_UDP;
	ptrespconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	ptrespconn.proto.udp->local_port = 1025;
	espconn_regist_recvcb(&ptrespconn, user_devicefind_recv);//接收到数据时回调user_devicefind_recv函数
	espconn_create(&ptrespconn);
}
