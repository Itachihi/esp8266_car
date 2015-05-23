/*
 * File	: at_ipCmd.c
 * This file is part of Espressif's AT+ command set program.
 * Copyright (C) 2013 - 2016, Espressif Systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "c_types.h"
#include "user_interface.h"
#include "at_version.h"
#include "espconn.h"
#include "mem.h"
#include "at.h"
#include "at_ipCmd.h"
#include "osapi.h"
#include "driver/uart.h"
#include <stdlib.h>
#include "upgrade.h"
#include "driver/pwm.h"
#include "gpio.h"
#include "eagle_soc.h"

extern at_mdStateType mdState;
extern BOOL specialAtState;
extern at_stateType at_state;
extern at_funcationType at_fun[];
extern uint8_t *pDataLine;
extern uint8_t at_dataLine[];
extern uint8_t at_wifiMode;
extern int8_t at_dataStrCpy(void *pDest, const void *pSrc, int8_t maxLen);
extern uint8_t BXSJ[23];

uint16_t at_sendLen; //now is 256
uint16_t at_tranLen; //now is 256
os_timer_t at_delayCheck;
BOOL IPMODE;
uint8_t ipDataSendFlag = 0;
extern BOOL at_ipMux ;
static BOOL disAllFlag = FALSE;
static at_linkConType pLink[at_linkMax];
static uint8_t sendingID;
static BOOL serverEn = FALSE;
static at_linkNum = 0;
static uint16_t server_timeover = 180;
static struct espconn *pTcpServer;
static void at_tcpclient_discon_cb(void *arg);

void ICACHE_FLASH_ATTR
at_testCmdCifsr(uint8_t id) {
	at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCifsr(uint8_t id, char *pPara) {
	struct ip_info pTempIp;
	int8_t len;
	char ipTemp[64];
//  char temp[64];

	if (at_wifiMode == STATION_MODE) {
		at_backError;
		return;
	}
	pPara = strchr(pPara, '\"');
	len = at_dataStrCpy(ipTemp, pPara, 32);
	if (len == -1) {
		uart0_sendStr("IP ERROR\r\n");
		return;
	}

	wifi_get_ip_info(0x01, &pTempIp);
	pTempIp.ip.addr = ipaddr_addr(ipTemp);

	os_printf("%d.%d.%d.%d\r\n",
			  IP2STR(&pTempIp.ip));

	if (!wifi_set_ip_info(0x01, &pTempIp)) {
		at_backError;
		return;
	}
	at_backOk;
	return;
}

/**
  * @brief  Execution commad of get module ip.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdCifsr(uint8_t id) { //add get station ip and ap ip
	struct ip_info pTempIp;
	char temp[64];
	uint8 bssid[6];

	if ((at_wifiMode == SOFTAP_MODE) || (at_wifiMode == STATIONAP_MODE)) {
		wifi_get_ip_info(0x01, &pTempIp);
		os_sprintf(temp, "%s:APIP,", at_fun[id].at_cmdName);
		uart0_sendStr(temp);

		os_sprintf(temp, "\"%d.%d.%d.%d\"\r\n",
				   IP2STR(&pTempIp.ip));
		uart0_sendStr(temp);

		os_sprintf(temp, "%s:APMAC,", at_fun[id].at_cmdName);
		uart0_sendStr(temp);

		wifi_get_macaddr(SOFTAP_IF, bssid);
		os_sprintf(temp, "\""MACSTR"\"\r\n",
				   MAC2STR(bssid));
		uart0_sendStr(temp);
//    mdState = m_gotip; /////////
	}
	if ((at_wifiMode == STATION_MODE) || (at_wifiMode == STATIONAP_MODE)) {
		wifi_get_ip_info(0x00, &pTempIp);
		os_sprintf(temp, "%s:STAIP,", at_fun[id].at_cmdName);
		uart0_sendStr(temp);

		os_sprintf(temp, "\"%d.%d.%d.%d\"\r\n",
				   IP2STR(&pTempIp.ip));
		uart0_sendStr(temp);

		os_sprintf(temp, "%s:STAMAC,", at_fun[id].at_cmdName);
		uart0_sendStr(temp);

		wifi_get_macaddr(STATION_IF, bssid);
		os_sprintf(temp, "\""MACSTR"\"\r\n",
				   MAC2STR(bssid));
		uart0_sendStr(temp);
//    mdState = m_gotip; /////////
	}
	mdState = m_gotip;
	at_backOk;
}

/**
  * @brief  Test commad of get link status.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_testCmdCipstatus(uint8_t id) {
	at_backOk;
}

/**
  * @brief  Execution commad of get link status.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdCipstatus(uint8_t id) {
	char temp[64];
	uint8_t i;

	os_sprintf(temp, "STATUS:%d\r\n",
			   mdState);
	uart0_sendStr(temp);
	if (serverEn) {

	}
	for (i = 0; i < at_linkMax; i++) {
		if (pLink[i].linkEn) {
			if (pLink[i].pCon->type == ESPCONN_TCP) {
				os_sprintf(temp, "%s:%d,\"TCP\",\"%d.%d.%d.%d\",%d,%d\r\n",
						   at_fun[id].at_cmdName,
						   pLink[i].linkId,
						   IP2STR(pLink[i].pCon->proto.tcp->remote_ip),
						   pLink[i].pCon->proto.tcp->remote_port,
						   pLink[i].teType);
				uart0_sendStr(temp);
			} else {
				os_sprintf(temp, "%s:%d,\"UDP\",\"%d.%d.%d.%d\",%d,%d,%d\r\n",
						   at_fun[id].at_cmdName,
						   pLink[i].linkId,
						   IP2STR(pLink[i].pCon->proto.udp->remote_ip),
						   pLink[i].pCon->proto.udp->remote_port,
						   pLink[i].pCon->proto.udp->local_port,
						   pLink[i].teType);
				uart0_sendStr(temp);
			}
		}
	}
	at_backOk;
}

/**
  * @brief  Test commad of start client.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_testCmdCipstart(uint8_t id) {
	char temp[64];

	if (at_ipMux) {
		os_sprintf(temp, "%s:(\"type\"),(\"ip address\"),(port)\r\n",
				   at_fun[id].at_cmdName);
		uart0_sendStr(temp);
		os_sprintf(temp, "%s:(\"type\"),(\"domain name\"),(port)\r\n",
				   at_fun[id].at_cmdName);
		uart0_sendStr(temp);
	} else {
		os_sprintf(temp, "%s:(id)(\"type\"),(\"ip address\"),(port)\r\n",
				   at_fun[id].at_cmdName);
		uart0_sendStr(temp);
		os_sprintf(temp, "%s:((id)\"type\"),(\"domain name\"),(port)\r\n",
				   at_fun[id].at_cmdName);
		uart0_sendStr(temp);
	}
	at_backOk;
}

/**
  * @brief  Client received callback function.
  * @param  arg: contain the ip link information
  * @param  pdata: received data
  * @param  len: the lenght of received data
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_tcpclient_recv(void *arg, char *pdata, unsigned short len) {
	struct espconn *pespconn = (struct espconn *)arg;
	at_linkConType *linkTemp = (at_linkConType *)pespconn->reverse;
	char temp[32];
	os_printf("recv\r\n");
	if (at_ipMux) {
		os_sprintf(temp, "\r\n+IPD,%d,%d:",
				   linkTemp->linkId, len);
		uart0_sendStr(temp);
		uart0_tx_buffer(pdata, len);
		//GPIO0=0;
		if (pdata[0] == 'G' && pdata[1] == 'P' && pdata[2] == 'I' && pdata[3] == 'O' && pdata[4] == '0' && pdata[5] == '=' && pdata[6] == '0' && pdata[7] == ';') {
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
			GPIO_OUTPUT_SET(GPIO_ID_PIN(0), 0);
		}
		//GPIO0=1;
		if (pdata[0] == 'G' && pdata[1] == 'P' && pdata[2] == 'I' && pdata[3] == 'O' && pdata[4] == '0' && pdata[5] == '=' && pdata[6] == '1' && pdata[7] == ';') {
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
			GPIO_OUTPUT_SET(GPIO_ID_PIN(0), 1);
		}
	} else if (IPMODE == FALSE) {
		os_sprintf(temp, "\r\n+IPD,%d:", len);
		uart0_sendStr(temp);
		uart0_tx_buffer(pdata, len);
	} else {
		uart0_tx_buffer(pdata, len);
		return;
	}
	at_backOk;
}

/**
  * @brief  Client received callback function.
  * @param  arg: contain the ip link information
  * @param  pdata: received data
  * @param  len: the lenght of received data
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_udpclient_recv(void *arg, char *pdata, unsigned short len) {
	struct espconn *pespconn = (struct espconn *)arg;
	at_linkConType *linkTemp = (at_linkConType *)pespconn->reverse;
	char temp[32];
	if (at_ipMux) {
		//os_sprintf(temp, "\r\n+IPD,%d,%d:",
		//          linkTemp->linkId, len);
		//uart0_sendStr(temp);
		//uart0_tx_buffer(pdata, len);
	} else if (IPMODE == FALSE) {
		//os_sprintf(temp, "\r\n+IPD,%d:", len);
		//uart0_sendStr(temp);
		//uart0_tx_buffer(pdata, len);
	} else {
		//uart0_tx_buffer(pdata, len);
		return;
	}
	//at_backOk;
}

/**
  * @brief  Client send over callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_tcpclient_sent_cb(void *arg) {
//	os_free(at_dataLine);
//  os_printf("send_cb\r\n");
	if (IPMODE == TRUE) {
		ipDataSendFlag = 0;
		os_timer_disarm(&at_delayCheck);
		os_timer_arm(&at_delayCheck, 20, 0);
		system_os_post(at_recvTaskPrio, 0, 0); ////
		ETS_UART_INTR_ENABLE();
		return;
	}
	specialAtState = TRUE;
	at_state = at_statIdle;
	uart0_sendStr("\r\nSEND OK\r\n");
}
/**
  * @brief  Tcp client connect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_tcpclient_connect_cb(void *arg) {
	struct espconn *pespconn = (struct espconn *)arg;
	at_linkConType *linkTemp = (at_linkConType *)pespconn->reverse;
	char temp[16];

	//os_printf("tcp client connect\r\n");//������
	os_printf("pespconn %p\r\n", pespconn);

	linkTemp->linkEn = TRUE;
	linkTemp->teType = teClient;
	linkTemp->repeaTime = 0;
	espconn_regist_disconcb(pespconn, at_tcpclient_discon_cb);
	espconn_regist_recvcb(pespconn, at_tcpclient_recv);////////
	espconn_regist_sentcb(pespconn, at_tcpclient_sent_cb);///////

	mdState = m_linked;
//  at_linkNum++;
	if (at_state == at_statIpTraning) {
		return;
	}
	if (at_ipMux) {
		os_sprintf(temp, "%d,CONNECT\r\n", linkTemp->linkId);
		uart0_sendStr(temp);
	} else {
		uart0_sendStr("CONNECT\r\n");
	}
	at_backOk;
//  uart0_sendStr("Linked\r\n");//////////////////

	specialAtState = TRUE;
	at_state = at_statIdle;
}

/**
  * @brief  Tcp client connect repeat callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_tcpclient_recon_cb(void *arg, sint8 errType) {
	struct espconn *pespconn = (struct espconn *)arg;
	at_linkConType *linkTemp = (at_linkConType *)pespconn->reverse;
	struct ip_info ipconfig;
	os_timer_t sta_timer;
	char temp[16];

//  os_printf("at_tcpclient_recon_cb %p\r\n", arg);

	if (at_state == at_statIpTraning) {
		linkTemp->repeaTime++;
		ETS_UART_INTR_ENABLE(); ///
		os_printf("Traning recon\r\n");
		if (linkTemp->repeaTime > 10) {
			linkTemp->repeaTime = 10;
		}
		os_delay_us(linkTemp->repeaTime * 10000);
		pespconn->proto.tcp->local_port = espconn_port();
		espconn_connect(pespconn);
		return;
	}
	//os_sprintf(temp,"%d,CLOSED\r\n", linkTemp->linkId);//�ѶϿ�
	//uart0_sendStr(temp);

	if (linkTemp->teToff == TRUE) {
		linkTemp->teToff = FALSE;
		linkTemp->repeaTime = 0;
		if (pespconn->proto.tcp != NULL) {
			os_free(pespconn->proto.tcp);
		}
		os_free(pespconn);
		linkTemp->linkEn = false;
		at_linkNum--;
		if (at_linkNum == 0) {
			at_backOk;
			mdState = m_unlink; //////////////////////
//      uart0_sendStr("Unlink\r\n");
			disAllFlag = false;
			specialAtState = TRUE;
			at_state = at_statIdle;
		}
	} else {
		linkTemp->repeaTime++;
		if (linkTemp->repeaTime >= 1) {
			os_printf("repeat over %d\r\n", linkTemp->repeaTime);
//      specialAtState = TRUE;
//      at_state = at_statIdle;
			linkTemp->repeaTime = 0;
//      os_printf("err %d\r\n", errType);
			if (errType == ESPCONN_CLSD) {
				at_backOk;
			} else {
				at_backError;
			}
			if (pespconn->proto.tcp != NULL) {
				os_free(pespconn->proto.tcp);
			}
			os_free(pespconn);
			linkTemp->linkEn = false;
			os_printf("disconnect\r\n");
			//  os_printf("con EN? %d\r\n", pLink[0].linkEn);
			at_linkNum--;
			if (at_linkNum == 0) {
				mdState = m_unlink; //////////////////////
				disAllFlag = false;
			}
			ETS_UART_INTR_ENABLE(); ///
			specialAtState = true;
			at_state = at_statIdle;
			return;
		}

		specialAtState = true;
		at_state = at_statIdle;
		os_printf("link repeat %d\r\n", linkTemp->repeaTime);
		pespconn->proto.tcp->local_port = espconn_port();
		espconn_connect(pespconn);
	}
}


static ip_addr_t host_ip;
/******************************************************************************
 * FunctionName : user_esp_platform_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
at_dns_found(const char *name, ip_addr_t *ipaddr, void *arg) {
	struct espconn *pespconn = (struct espconn *) arg;
	at_linkConType *linkTemp = (at_linkConType *) pespconn->reverse;
	char temp[16];

	if (ipaddr == NULL) {
		linkTemp->linkEn = FALSE;
		uart0_sendStr("DNS Fail\r\n");
		specialAtState = TRUE;
		at_state = at_statIdle;
//    device_status = DEVICE_CONNECT_SERVER_FAIL;
		return;
	}

	os_printf("DNS found: %d.%d.%d.%d\n",
			  *((uint8 *) &ipaddr->addr),
			  *((uint8 *) &ipaddr->addr + 1),
			  *((uint8 *) &ipaddr->addr + 2),
			  *((uint8 *) &ipaddr->addr + 3));

	if (host_ip.addr == 0 && ipaddr->addr != 0) {
		if (pespconn->type == ESPCONN_TCP) {
			os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);
			espconn_connect(pespconn);
			at_linkNum++;
		} else {
			os_memcpy(pespconn->proto.udp->remote_ip, &ipaddr->addr, 4);
			os_memcpy(linkTemp->remoteIp, &ipaddr->addr, 4);
			espconn_connect(pespconn);
			specialAtState = TRUE;
			at_state = at_statIdle;
			at_linkNum++;
			os_sprintf(temp, "%d,CONNECT\r\n", linkTemp->linkId);
			uart0_sendStr(temp);
			at_backOk;
		}
	}
}

/**
  * @brief  Setup commad of start client.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipstart(uint8_t id, char *pPara) {
	char temp[64];
//  enum espconn_type linkType;
	int8_t len;
	enum espconn_type linkType = ESPCONN_INVALID;
	uint32_t ip = 0;
	char ipTemp[128];
	int32_t remotePort, localPort;
	uint8_t linkID;
	uint8_t changType;

	char ret;
	remotePort = 0;
	localPort = 0;
	if (at_wifiMode == 1) {
		if (wifi_station_get_connect_status() != STATION_GOT_IP) {
			uart0_sendStr("no ip\r\n");
			return;
		}
	}
	pPara++;
	if (at_ipMux) {
		linkID = atoi(pPara);
		pPara++;
		pPara = strchr(pPara, '\"');
	} else {
		linkID = 0;
	}
	if (linkID >= at_linkMax) {
		uart0_sendStr("ID ERROR\r\n");
		return;
	}
	len = at_dataStrCpy(temp, pPara, 6);
	if (len == -1) {
		uart0_sendStr("Link typ ERROR\r\n");
		return;
	}
	if (os_strcmp(temp, "TCP") == 0) {
		linkType = ESPCONN_TCP;
	} else if (os_strcmp(temp, "UDP") == 0) {
		linkType = ESPCONN_UDP;
	} else {
		uart0_sendStr("Link typ ERROR\r\n");
		return;
	}
	pPara += (len + 3);
	len = at_dataStrCpy(ipTemp, pPara, 64);
	os_printf("%s\r\n", ipTemp);
	if (len == -1) {
		uart0_sendStr("IP ERROR\r\n");
		return;
	}
	pPara += (len + 2);
	if (*pPara != ',') {
		uart0_sendStr("ENTRY ERROR\r\n");
		return;
	}
	pPara += (1);
	remotePort = atoi(pPara);

	if (linkType == ESPCONN_UDP) {
		os_printf("remote port:%d\r\n", remotePort);
		pPara = strchr(pPara, ',');
		if (pPara == NULL) {
			if ((remotePort == 0) | (ipTemp[0] == 0)) {
				uart0_sendStr("Miss param\r\n");
				return;
			}
		} else {
			pPara += 1;
			localPort = atoi(pPara);
			if (localPort == 0) {
				uart0_sendStr("Miss param2\r\n");
				return;
			}
			os_printf("local port:%d\r\n", localPort);

			pPara = strchr(pPara, ',');
			if (pPara == NULL) {
				changType = 0;
			} else {
				pPara += 1;
				changType = atoi(pPara);
			}
			os_printf("change type:%d\r\n", changType);
		}
	}

	if (pLink[linkID].linkEn) {
		uart0_sendStr("ALREAY CONNECT\r\n");
		return;
	}
	pLink[linkID].pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));
	if (pLink[linkID].pCon == NULL) {
		uart0_sendStr("CONNECT FAIL\r\n");
		return;
	}
	pLink[linkID].pCon->type = linkType;
	pLink[linkID].pCon->state = ESPCONN_NONE;
	pLink[linkID].linkId = linkID;

	switch (linkType) {
	case ESPCONN_TCP:
		ip = ipaddr_addr(ipTemp);
		pLink[linkID].pCon->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
		pLink[linkID].pCon->proto.tcp->local_port = espconn_port();
		pLink[linkID].pCon->proto.tcp->remote_port = remotePort;

		os_memcpy(pLink[linkID].pCon->proto.tcp->remote_ip, &ip, 4);

		pLink[linkID].pCon->reverse = &pLink[linkID];

		espconn_regist_connectcb(pLink[linkID].pCon, at_tcpclient_connect_cb);
		espconn_regist_reconcb(pLink[linkID].pCon, at_tcpclient_recon_cb);
		specialAtState = FALSE;
		if ((ip == 0xffffffff) && (os_memcmp(ipTemp, "255.255.255.255", 16) != 0)) {
			espconn_gethostbyname(pLink[linkID].pCon, ipTemp, &host_ip, at_dns_found);
		} else {
			espconn_connect(pLink[linkID].pCon);
			at_linkNum++;
		}
		break;

	case ESPCONN_UDP:
		pLink[linkID].pCon->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
		if (localPort == 0) {
			pLink[linkID].pCon->proto.udp->local_port = espconn_port();
		} else {
			pLink[linkID].pCon->proto.udp->local_port = localPort;
		}
		if (remotePort == 0) {
			pLink[linkID].pCon->proto.udp->remote_port = espconn_port();
		} else {
			pLink[linkID].pCon->proto.udp->remote_port = remotePort;
		}

		pLink[linkID].changType = changType;
		pLink[linkID].remotePort = pLink[linkID].pCon->proto.udp->remote_port;

		pLink[linkID].pCon->reverse = &pLink[linkID];
//    os_printf("%d\r\n",pLink[linkID].pCon->proto.udp->local_port);///

		pLink[linkID].linkId = linkID;
		pLink[linkID].linkEn = TRUE;
		pLink[linkID].teType = teClient;
		espconn_regist_recvcb(pLink[linkID].pCon, at_udpclient_recv);
		espconn_regist_sentcb(pLink[linkID].pCon, at_tcpclient_sent_cb);
		if (ipTemp[0] == 0) {
			ip = 0xffffffff;
			os_memcpy(ipTemp, "255.255.255.255", 16);
		} else {
			ip = ipaddr_addr(ipTemp);
		}
		os_memcpy(pLink[linkID].pCon->proto.udp->remote_ip, &ip, 4);
		os_memcpy(pLink[linkID].remoteIp, &ip, 4);
		if ((ip == 0xffffffff) && (os_memcmp(ipTemp, "255.255.255.255", 16) != 0)) {
			specialAtState = FALSE;
			espconn_gethostbyname(pLink[linkID].pCon, ipTemp, &host_ip, at_dns_found);
		} else {
			ret = espconn_create(pLink[linkID].pCon);
//      os_printf("udp create ret:%d\r\n", ret);
			os_sprintf(temp, "%d,CONNECT\r\n", linkID);
			uart0_sendStr(temp);
			at_linkNum++;
			at_backOk;
		}
		break;

	default:
		break;
	}
}

/**
  * @brief  Tcp client disconnect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_tcpclient_discon_cb(void *arg) {
	struct espconn *pespconn = (struct espconn *)arg;
	at_linkConType *linkTemp = (at_linkConType *)pespconn->reverse;
	uint8_t idTemp;
	char temp[16];

	if (pespconn == NULL) {
		return;
	}
	if (at_state == at_statIpTraning) {
		ETS_UART_INTR_ENABLE(); ///
		os_printf("Traning nodiscon\r\n");
		pespconn->proto.tcp->local_port = espconn_port();
		espconn_connect(pespconn);
		return;
	}
	if (pespconn->proto.tcp != NULL) {
		os_free(pespconn->proto.tcp);
	}
	os_free(pespconn);

	linkTemp->linkEn = FALSE;
//  os_printf("disconnect\r\n");
	if (at_ipMux) {
		os_sprintf(temp, "%d,CLOSED\r\n", linkTemp->linkId);
		uart0_sendStr(temp);
	} else {
		uart0_sendStr("CLOSED\r\n");
	}

//  os_printf("con EN? %d\r\n", pLink[0].linkEn);
	at_linkNum--;

	if (disAllFlag == FALSE) {
		at_backOk;
	}
	if (at_linkNum == 0) {
		mdState = m_unlink;//////////////////////
		if (disAllFlag) {
			at_backOk;
		}
		disAllFlag = FALSE;
	}

	if (disAllFlag) {
		idTemp = linkTemp->linkId + 1;
		for (; idTemp < at_linkMax; idTemp++) {
			if (pLink[idTemp].linkEn) {
				if (pLink[idTemp].teType == teServer) {
					continue;
				}
				if (pLink[idTemp].pCon->type == ESPCONN_TCP) {
					specialAtState = FALSE;
					espconn_disconnect(pLink[idTemp].pCon);
					break;
				} else {
					os_sprintf(temp, "%d,CLOSED\r\n", pLink[idTemp].linkId);
					uart0_sendStr(temp);
					pLink[idTemp].linkEn = FALSE;
					espconn_delete(pLink[idTemp].pCon);
					os_free(pLink[idTemp].pCon->proto.udp);
					os_free(pLink[idTemp].pCon);
					at_linkNum--;
					if (at_linkNum == 0) {
						mdState = m_unlink;
						at_backOk;
						disAllFlag = FALSE;
					}
				}
			}
		}
	}
	ETS_UART_INTR_ENABLE();
//  IPMODE = FALSE;
	specialAtState = TRUE;
	at_state = at_statIdle;
}

/**
  * @brief  Test commad of close ip link.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_testCmdCipclose(uint8_t id) {
	at_backOk;
}

/**
  * @brief  Setup commad of close ip link.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipclose(uint8_t id, char *pPara) {
	char temp[64];
	uint8_t linkID;
	uint8_t i;
	pPara++;
	if (at_ipMux == 0) {
		uart0_sendStr("MUX=0\r\n");
		return;
	}
	linkID = atoi(pPara);
	if (linkID > at_linkMax) {
		at_backError;
		return;
	}
	if (linkID == at_linkMax) {
		if (serverEn) {
			/* restart */
			uart0_sendStr("we must restart\r\n");
			return;
		}
		for (linkID = 0; linkID < at_linkMax; linkID++) {
			if (pLink[linkID].linkEn) {
				if (pLink[linkID].pCon->type == ESPCONN_TCP) {
					pLink[linkID].teToff = TRUE;
					specialAtState = FALSE;
					espconn_disconnect(pLink[linkID].pCon);
					disAllFlag = TRUE;
					break;
				} else {
					pLink[linkID].linkEn = FALSE;
					os_sprintf(temp, "%d,CLOSED\r\n", linkID);
					uart0_sendStr(temp);
					espconn_delete(pLink[linkID].pCon);
					os_free(pLink[linkID].pCon->proto.udp);///
					os_free(pLink[linkID].pCon);
					at_linkNum--;
					if (at_linkNum == 0) {
						mdState = m_unlink;
						at_backOk;
//            uart0_sendStr("Unlink\r\n");
					}
				}
			}
		}
	} else {
		if (pLink[linkID].linkEn == FALSE) {
			uart0_sendStr("link is not\r\n");
			return;
		}
		if (pLink[linkID].teType == teServer) {
			if (pLink[linkID].pCon->type == ESPCONN_TCP) {
				pLink[linkID].teToff = TRUE;
				specialAtState = FALSE;
				espconn_disconnect(pLink[linkID].pCon);
			} else {
				pLink[linkID].linkEn = FALSE;
				os_sprintf(temp, "%d,CLOSED\r\n", linkID);
				uart0_sendStr(temp);
				espconn_delete(pLink[linkID].pCon);
				at_linkNum--;
				at_backOk;
				if (at_linkNum == 0) {
					mdState = m_unlink;
//          uart0_sendStr("Unlink\r\n");
				}
			}
		} else {
			if (pLink[linkID].pCon->type == ESPCONN_TCP) {
				pLink[linkID].teToff = TRUE;
				specialAtState = FALSE;
				espconn_disconnect(pLink[linkID].pCon);
			} else {
				pLink[linkID].linkEn = FALSE;
				os_sprintf(temp, "%d,CLOSED\r\n", linkID);
				uart0_sendStr(temp);
				espconn_delete(pLink[linkID].pCon);
				os_free(pLink[linkID].pCon->proto.udp);
				os_free(pLink[linkID].pCon);
				at_linkNum--;
				at_backOk;
				if (at_linkNum == 0) {
					mdState = m_unlink;
				}
			}
		}
	}
}

/**
  * @brief  Execution commad of close ip link.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdCipclose(uint8_t id) {
	char temp[64];
	if (at_ipMux) {
		uart0_sendStr("MUX=1\r\n");
		return;
	}
	if (pLink[0].linkEn) {
		if (serverEn) {
			/* restart */
			uart0_sendStr("we must restart\r\n");
			return;
		} else {
			if (pLink[0].pCon->type == ESPCONN_TCP) {
				specialAtState = FALSE;
				pLink[0].teToff = TRUE;
				espconn_disconnect(pLink[0].pCon);
			} else {
				pLink[0].linkEn = FALSE;
				uart0_sendStr("CLOSED\r\n");
				espconn_delete(pLink[0].pCon);
				os_free(pLink[0].pCon->proto.udp);
				os_free(pLink[0].pCon);
				at_linkNum--;
				if (at_linkNum == 0) {
					mdState = m_unlink;
					at_backOk;
				}
			}
		}
	} else {
		at_backError;
	}
}

/**
  * @brief  Test commad of send ip data.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_testCmdCipsend(uint8_t id) {
	at_backOk;
}

char *ICACHE_FLASH_ATTR
at_checkLastNum(char *pPara, uint8_t maxLen) {
	int8_t ret = -1;
	char *pTemp;
	uint8_t i;

	pTemp = pPara;
	for (i = 0; i < maxLen; i++) {
		if ((*pTemp > '9') || (*pTemp < '0')) {
			break;
		}
		pTemp++;
	}
	if (i == maxLen) {
		return NULL;
	} else {
		return pTemp;
	}
}
/**
  * @brief  Setup commad of send ip data.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipsend(uint8_t id, char *pPara) {
	if (IPMODE == TRUE) {
		uart0_sendStr("IPMODE=1\r\n");
		at_backError;
		return;
	}
	pPara++;
	if (at_ipMux) {
		sendingID = atoi(pPara);
		if (sendingID >= at_linkMax) {
			at_backError;
			return;
		}
		pPara++;
		if (*pPara != ',') { //ID must less 10
			at_backError;
			return;
		}
		pPara++;
	} else {
		sendingID = 0;
	}
	if (pLink[sendingID].linkEn == FALSE) {
		uart0_sendStr("link is not\r\n");
		return;
	}
	at_sendLen = atoi(pPara);
	if (at_sendLen > 2048) {
		uart0_sendStr("too long\r\n");
		return;
	}
	pPara = at_checkLastNum(pPara, 5);
	if ((pPara == NULL) || (*pPara != '\r')) {
		uart0_sendStr("type error\r\n");
		return;
	}
	pDataLine = at_dataLine;
//  pDataLine = UartDev.rcv_buff.pRcvMsgBuff;
	specialAtState = FALSE;
	at_state = at_statIpSending;
	uart0_sendStr("> "); //uart0_sendStr("\r\n>");
}

/**
  * @brief  Send data through ip.
  * @param  pAtRcvData: point to data
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_ipDataSending(uint8_t *pAtRcvData) {
	espconn_sent(pLink[sendingID].pCon, pAtRcvData, at_sendLen);
	os_printf("id:%d,Len:%d,dp:%p\r\n", sendingID, at_sendLen, pAtRcvData);
}

/**
  * @brief  Transparent data through ip.
  * @param  arg: no used
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_ipDataTransparent(void *arg) {
	if (at_state != at_statIpTraning) {
		return;
	}
	os_timer_disarm(&at_delayCheck);
	if ((at_tranLen == 3) && (os_memcmp(at_dataLine, "+++", 3) == 0)) { //UartDev.rcv_buff.pRcvMsgBuff
		specialAtState = TRUE;
		at_state = at_statIdle;
		return;
	} else if (at_tranLen) {
		ETS_UART_INTR_DISABLE(); //
		espconn_sent(pLink[0].pCon, at_dataLine, at_tranLen); //UartDev.rcv_buff.pRcvMsgBuff ////
		ipDataSendFlag = 1;
		pDataLine = at_dataLine;
		at_tranLen = 0;
		return;
	}
	os_timer_arm(&at_delayCheck, 20, 0);
}

/**
  * @brief  Send data through ip.
  * @param  pAtRcvData: point to data
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_ipDataSendNow(void) {
	espconn_sent(pLink[0].pCon, at_dataLine, at_tranLen); //UartDev.rcv_buff.pRcvMsgBuff ////
	ipDataSendFlag = 1;
	pDataLine = at_dataLine;
	at_tranLen = 0;
}

/**
  * @brief  Setup commad of send ip data.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdCipsend(uint8_t id) {
	if ((serverEn) || (IPMODE == FALSE)) {
		at_backError;
		return;
	}
	if (pLink[0].linkEn == FALSE) {
		at_backError;
		return;
	}
	pDataLine = at_dataLine;//UartDev.rcv_buff.pRcvMsgBuff;
	at_tranLen = 0;
	specialAtState = FALSE;
	at_state = at_statIpTraning;
	os_timer_disarm(&at_delayCheck);
	os_timer_setfn(&at_delayCheck, (os_timer_func_t *)at_ipDataTransparent, NULL);
	os_timer_arm(&at_delayCheck, 20, 0);
//  IPMODE = TRUE;
	uart0_sendStr("\r\n>");
}

/**
  * @brief  Query commad of set multilink mode.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_queryCmdCipmux(uint8_t id) {
	char temp[32];
	os_sprintf(temp, "%s:%d\r\n",
			   at_fun[id].at_cmdName, at_ipMux);
	uart0_sendStr(temp);
	at_backOk;
}

/**
  * @brief  Setup commad of set multilink mode.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipmux(uint8_t id, char *pPara) {
	uint8_t muxTemp;
	if (mdState == m_linked) {
		uart0_sendStr("link is builded\r\n");
		return;
	}
	pPara++;
	muxTemp = atoi(pPara);
	if (muxTemp == 1) {
		at_ipMux = TRUE;
	} else if (muxTemp == 0) {
		at_ipMux = FALSE;
	} else {
		at_backError;
		return;
	}
	at_backOk;
}

/**
  * @brief  Tcp server disconnect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_tcpserver_discon_cb(void *arg) {
	struct espconn *pespconn = (struct espconn *) arg;
	at_linkConType *linkTemp = (at_linkConType *) pespconn->reverse;
	char temp[16];

	os_printf("S conect C: %p\r\n", arg);

	linkTemp->linkEn = FALSE;
	linkTemp->pCon = NULL;
	if (at_ipMux) {
		os_sprintf(temp, "%d,CLOSED\r\n", linkTemp->linkId);
		uart0_sendStr(temp);
	} else {
		uart0_sendStr("CLOSED\r\n");
	}
	if (linkTemp->teToff == TRUE) {
		linkTemp->teToff = FALSE;
		at_backOk;
	}
	at_linkNum--;
	if (at_linkNum == 0) {
		mdState = m_unlink;
		disAllFlag = false;
	}
	ETS_UART_INTR_ENABLE();
	specialAtState = true;
	at_state = at_statIdle;
}

/**
  * @brief  Tcp server connect repeat callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_tcpserver_recon_cb(void *arg, sint8 errType) {
	struct espconn *pespconn = (struct espconn *)arg;
	at_linkConType *linkTemp = (at_linkConType *)pespconn->reverse;
	char temp[16];

	os_printf("S conect C: %p\r\n", arg);

	if (pespconn == NULL) {
		return;
	}

	linkTemp->linkEn = false;
	linkTemp->pCon = NULL;
	os_printf("con EN? %d\r\n", linkTemp->linkId);
	at_linkNum--;
	if (at_linkNum == 0) {
		mdState = m_unlink; //////////////////////
	}

	if (at_ipMux) {
		os_sprintf(temp, "%d,CONNECT\r\n", linkTemp->linkId);
		uart0_sendStr(temp);
	} else {
		uart0_sendStr("CONNECT\r\n");
	}
	disAllFlag = false;

	if (linkTemp->teToff == TRUE) {
		linkTemp->teToff = FALSE;
		at_backOk;
	}
	ETS_UART_INTR_ENABLE();
	specialAtState = true;
	at_state = at_statIdle;
}

/**
  * @brief  Tcp server listend callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
LOCAL void ICACHE_FLASH_ATTR
at_tcpserver_listen(void *arg) {
	struct espconn *pespconn = (struct espconn *)arg;
	uint8_t i;
	char temp[16];

	os_printf("get tcpClient:\r\n");
	for (i = 0; i < at_linkMax; i++) {
		if (pLink[i].linkEn == FALSE) {
			pLink[i].linkEn = TRUE;
			break;
		}
	}
	if (i >= 5) {
		return;
	}
	pLink[i].teToff = FALSE;
	pLink[i].linkId = i;
	pLink[i].teType = teServer;
	pLink[i].repeaTime = 0;
	pLink[i].pCon = pespconn;
	mdState = m_linked;
	at_linkNum++;
	pespconn->reverse = &pLink[i];
	espconn_regist_recvcb(pespconn, at_tcpclient_recv);
	espconn_regist_reconcb(pespconn, at_tcpserver_recon_cb);
	espconn_regist_disconcb(pespconn, at_tcpserver_discon_cb);
	espconn_regist_sentcb(pespconn, at_tcpclient_sent_cb);///////
	if (at_ipMux) {
		os_sprintf(temp, "%d,CONNECT\r\n", i);
		uart0_sendStr(temp);
	} else {
		uart0_sendStr("CONNECT\r\n");
	}
}
/**
  * @brief  Setup commad of module as server.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipserver(uint8_t id, char *pPara) {
	BOOL serverEnTemp;
	int32_t port;
	char temp[32];

	if (at_ipMux == FALSE) {
		at_backError;
		return;
	}
	pPara++;
	serverEnTemp = atoi(pPara);
	pPara++;
	if (serverEnTemp == 0) {
		if (*pPara != '\r') {

			at_backError;
			return;
		}
	} else if (serverEnTemp == 1) {
		if (*pPara == ',') {
			pPara++;
			port = atoi(pPara);
		} else {
			port = 333;
		}
	} else {

		at_backError;
		return;
	}
	if (serverEnTemp == serverEn) {
		uart0_sendStr("no change\r\n");
		return;
	}

	if (serverEnTemp) {
		pTcpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
		if (pTcpServer == NULL) {
			uart0_sendStr("TcpServer Failure\r\n");
			return;
		}
		pTcpServer->type = ESPCONN_TCP;
		pTcpServer->state = ESPCONN_NONE;
		pTcpServer->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
		pTcpServer->proto.tcp->local_port = port;
		espconn_regist_connectcb(pTcpServer, at_tcpserver_listen);
		espconn_accept(pTcpServer);
		espconn_regist_time(pTcpServer, server_timeover, 0);
		os_printf("local port:%d", pTcpServer->proto.tcp->local_port);
	} else {
		/* restart */
		uart0_sendStr("we must restart\r\n");
		return;
	}
	serverEn = serverEnTemp;
	at_backOk;
}

/**
  * @brief  Query commad of set transparent mode.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_queryCmdCipmode(uint8_t id) {
	char temp[32];

	os_sprintf(temp, "%s:%d\r\n", at_fun[id].at_cmdName, IPMODE);
	uart0_sendStr(temp);
	at_backOk;
}

/**
  * @brief  Setup commad of transparent.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipmode(uint8_t id, char *pPara) {
	uint8_t mode;
	char temp[32];

	pPara++;
	if ((at_ipMux) || (serverEn)) {
		at_backError;
		return;
	}
	mode = atoi(pPara);
	if (mode > 1) {
		at_backError;
		return;
	}
	IPMODE = mode;
	at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCipsto(uint8_t id) {
	char temp[32];
	os_sprintf(temp, "%s:%d\r\n",
			   at_fun[id].at_cmdName, server_timeover);
	uart0_sendStr(temp);
	at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCipsto(uint8_t id, char *pPara) {
	char temp[64];
	uint16_t timeOver;

	if (serverEn == FALSE) {
		at_backError;
		return;
	}
	pPara++;
	timeOver = atoi(pPara);
	if (timeOver > 28800) {
		at_backError;
		return;
	}
	if (timeOver != server_timeover) {
		server_timeover = timeOver;
		espconn_regist_time(pTcpServer, server_timeover, 0);
	}
	at_backOk;
	return;
}

#define ESP_PARAM_SAVE_SEC_0    1
#define ESP_PARAM_SAVE_SEC_1    2
#define ESP_PARAM_SEC_FLAG      3
#define UPGRADE_FRAME  "{\"path\": \"/v1/messages/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"},\
\"get\":{\"action\":\"%s\"},\"body\":{\"pre_rom_version\":\"%s\",\"rom_version\":\"%s\"}}\n"
#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Authorization: token %s\r\n\
Accept-Encoding: gzip,deflate,sdch\r\n\
Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"
#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Authorization: token %s\r\n\
Accept-Encoding: gzip,deflate,sdch\r\n\
Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"

#define test
#ifdef test
#define KEY "39cdfe29a1863489e788efc339f514d78b78f0de"
#else
#define KEY "4ec90c1abbd5ffc0b339f34560a2eb8d71733861"
#endif


struct espconn *pespconn;
struct upgrade_server_info *upServer = NULL;
struct esp_platform_saved_param {
	uint8 devkey[40];
	uint8 token[40];
	uint8 activeflag;
	uint8 pad[3];
};
struct esp_platform_sec_flag_param {
	uint8 flag;
	uint8 pad[3];
};
//static struct esp_platform_saved_param esp_param;

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_cb
 * Description  : Processing the downloaded data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
at_upDate_rsp(void *arg) {
	struct upgrade_server_info *server = arg;
	if (server->upgrade_flag == true) {
		os_printf("device_upgrade_success\r\n");
		at_backOk;
		system_upgrade_reboot();
	} else {
		os_printf("device_upgrade_failed\r\n");
		at_backError;
	}

	os_free(server->url);
	server->url = NULL;
	os_free(server);
	server = NULL;
	specialAtState = TRUE;
	at_state = at_statIdle;
}

/**
  * @brief  Tcp client disconnect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_upDate_discon_cb(void *arg) {
	struct espconn *pespconn = (struct espconn *)arg;
	uint8_t idTemp;

	if (pespconn->proto.tcp != NULL) {
		os_free(pespconn->proto.tcp);
	}
	if (pespconn != NULL) {
		os_free(pespconn);
	}

	os_printf("disconnect\r\n");

	if (system_upgrade_start(upServer) == false) {
		at_backError;
		specialAtState = TRUE;
		at_state = at_statIdle;
	} else {
		uart0_sendStr("+CIPUPDATE:4\r\n");
	}
}

/**
  * @brief  Udp server receive data callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
LOCAL void ICACHE_FLASH_ATTR
at_upDate_recv(void *arg, char *pusrdata, unsigned short len) {
	struct espconn *pespconn = (struct espconn *)arg;
	char temp[32];
	char *pTemp;
	uint8_t user_bin[9] = {0};
	uint8_t i;

	os_timer_disarm(&at_delayCheck);
	uart0_sendStr("+CIPUPDATE:3\r\n");
	pTemp = (char *)os_strstr(pusrdata, "rom_version\": ");
	if (pTemp == NULL) {
		return;
	}
	pTemp += sizeof("rom_version\": ");

	upServer = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));
	os_memcpy(upServer->upgrade_version, pTemp, 5);
	upServer->upgrade_version[5] = '\0';
	os_sprintf(upServer->pre_version, "v%d.%d", AT_VERSION_main, AT_VERSION_sub);

	upServer->pespconn = pespconn;
	os_memcpy(upServer->ip, pespconn->proto.tcp->remote_ip, 4);

	upServer->port = 80;

	upServer->check_cb = at_upDate_rsp;
	upServer->check_times = 60000;

	if (upServer->url == NULL) {
		upServer->url = (uint8 *) os_zalloc(512);
	}

	if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1) {
		os_memcpy(user_bin, "user2.bin", 10);
	} else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2) {
		os_memcpy(user_bin, "user1.bin", 10);
	}

	os_sprintf(upServer->url,
			   "GET /v1/device/rom/?action=download_rom&version=%s&filename=%s HTTP/1.1\r\nHost: "IPSTR":%d\r\n"pheadbuffer"",
			   upServer->upgrade_version, user_bin, IP2STR(upServer->ip),
			   upServer->port, KEY);
}

LOCAL void ICACHE_FLASH_ATTR
at_upDate_wait(void *arg) {
	struct espconn *pespconn = arg;
	os_timer_disarm(&at_delayCheck);
	if (pespconn != NULL) {
		espconn_disconnect(pespconn);
	} else {
		at_backError;
		specialAtState = TRUE;
		at_state = at_statIdle;
	}
}

/******************************************************************************
 * FunctionName : user_esp_platform_sent_cb
 * Description  : Data has been sent successfully and acknowledged by the remote host.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
at_upDate_sent_cb(void *arg) {
	struct espconn *pespconn = arg;
	os_timer_disarm(&at_delayCheck);
	os_timer_setfn(&at_delayCheck, (os_timer_func_t *)at_upDate_wait, pespconn);
	os_timer_arm(&at_delayCheck, 5000, 0);
	os_printf("at_upDate_sent_cb\r\n");
}

/**
  * @brief  Tcp client connect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_upDate_connect_cb(void *arg) {
	struct espconn *pespconn = (struct espconn *)arg;
	uint8_t user_bin[9] = {0};
	char *temp;
	uart0_sendStr("+CIPUPDATE:2\r\n");
	espconn_regist_disconcb(pespconn, at_upDate_discon_cb);
	espconn_regist_recvcb(pespconn, at_upDate_recv);////////
	espconn_regist_sentcb(pespconn, at_upDate_sent_cb);
	temp = (uint8 *) os_zalloc(512);
	os_sprintf(temp, "GET /v1/device/rom/?is_format_simple=true HTTP/1.0\r\nHost: "IPSTR":%d\r\n"pheadbuffer"",
			   IP2STR(pespconn->proto.tcp->remote_ip),
			   80, KEY);
	espconn_sent(pespconn, temp, os_strlen(temp));
	os_free(temp);
}

/**
  * @brief  Tcp client connect repeat callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_upDate_recon_cb(void *arg, sint8 errType) {
	struct espconn *pespconn = (struct espconn *)arg;
	at_backError;
	if (pespconn->proto.tcp != NULL) {
		os_free(pespconn->proto.tcp);
	}
	os_free(pespconn);
	os_printf("disconnect\r\n");

	if (upServer != NULL) {
		os_free(upServer);
		upServer = NULL;
	}
	at_backError;
	specialAtState = TRUE;
	at_state = at_statIdle;
}

/******************************************************************************
 * FunctionName : upServer_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
upServer_dns_found(const char *name, ip_addr_t *ipaddr, void *arg) {
	struct espconn *pespconn = (struct espconn *) arg;
	if (ipaddr == NULL) {
		at_backError;
		specialAtState = TRUE;
		at_state = at_statIdle;
		return;
	}
	uart0_sendStr("+CIPUPDATE:1\r\n");
	if (host_ip.addr == 0 && ipaddr->addr != 0) {
		if (pespconn->type == ESPCONN_TCP) {
			os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);
			espconn_regist_connectcb(pespconn, at_upDate_connect_cb);
			espconn_regist_reconcb(pespconn, at_upDate_recon_cb);
			espconn_connect(pespconn);
		}
	}
}

void ICACHE_FLASH_ATTR
at_exeCmdCiupdate(uint8_t id) {
	pespconn = (struct espconn *)os_zalloc(sizeof(struct espconn));
	pespconn->type = ESPCONN_TCP;
	pespconn->state = ESPCONN_NONE;
	pespconn->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
	pespconn->proto.tcp->local_port = espconn_port();
	pespconn->proto.tcp->remote_port = 80;

	specialAtState = FALSE;
	espconn_gethostbyname(pespconn, "iot.espressif.cn", &host_ip, upServer_dns_found);
}

void ICACHE_FLASH_ATTR
at_exeCmdCiping(uint8_t id) {
	at_backOk;
}

void ICACHE_FLASH_ATTR
at_exeCmdCipappup(uint8_t id) {

}
/**
  * @}
  */