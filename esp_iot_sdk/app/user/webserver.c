/*
 * webserver.c
 *
 *  Created on: 2015-2-28
 *      Author: Administrator
 */

#include "user_interface.h"
#include "user_webserver.h"
#include "webserver_process.h"
#include "espconn.h"
#include "mem.h"
#include "user.h"
#include "Gpio.h"
#include "osapi.h"


LOCAL os_timer_t client_timer, led_timer;
#define DATA_BUF_SIZE 1024

#define  CHECKIP_TIMER  100

LOCAL ICACHE_FLASH_ATTR
void checkip_timer_cb(uint8 flag) {
	struct ip_info ipconfig;
	uint8_t status;

	os_timer_disarm(&client_timer);

	wifi_get_ip_info(STATION_IF, &ipconfig);
	status = wifi_station_get_connect_status();

	if ((status == STATION_GOT_IP) && (ipconfig.ip.addr != 0)) {
		os_printf("got station ip\n");
	} else if ((status == STATION_WRONG_PASSWORD) ||
			   (status == STATION_NO_AP_FOUND) ||
			   (status == STATION_CONNECT_FAIL)) {
		os_printf("station connect error\n");
	} else {
		os_timer_setfn(&client_timer, (os_timer_func_t *)checkip_timer_cb, NULL);
		os_timer_arm(&client_timer, CHECKIP_TIMER, 0);
	}
}

/******************************************************************************
 * FunctionName : parse_url
 * Description  : parse the received data from the server
 * Parameters   : precv -- the received data
 *                purl_frame -- the result of parsing the url
 * Returns      : none
*******************************************************************************/

LOCAL ICACHE_FLASH_ATTR
void parse_url(char *precv, URL_Frame *purl_frame) {
	char *str = NULL;
	uint8 length = 0;
	char *pbuffer = NULL;
	char *pbufer = NULL;

	if (purl_frame == NULL || precv == NULL) {
		return;
	}

	pbuffer = (char *)os_strstr(precv, "Host:");

	if (pbuffer != NULL) {
		length = pbuffer - precv;
		pbufer = (char *)os_zalloc(length + 1);
		pbuffer = pbufer;
		os_memcpy(pbuffer, precv, length);
		os_memset(purl_frame->pSelect, 0, URLSize);
		os_memset(purl_frame->pCommand, 0, URLSize);
		os_memset(purl_frame->pFilename, 0, URLSize);

		if (os_strncmp(pbuffer, "GET ", 4) == 0) {
			purl_frame->Type = GET;
			pbuffer += 4;
		} else if (os_strncmp(pbuffer, "POST ", 5) == 0) {
			purl_frame->Type = POST;
			pbuffer += 5;
		}

		pbuffer ++;
		str = (char *)os_strstr(pbuffer, "?");

		if (str != NULL) {
			length = str - pbuffer;
			os_memcpy(purl_frame->pSelect, pbuffer, length);
			str ++;
			pbuffer = (char *)os_strstr(str, "=");

			if (pbuffer != NULL) {
				length = pbuffer - str;
				os_memcpy(purl_frame->pCommand, str, length);
				pbuffer ++;
				str = (char *)os_strstr(pbuffer, "&");

				if (str != NULL) {
					length = str - pbuffer;
					os_memcpy(purl_frame->pFilename, pbuffer, length);
				} else {
					str = (char *)os_strstr(pbuffer, " HTTP");

					if (str != NULL) {
						length = str - pbuffer;
						os_memcpy(purl_frame->pFilename, pbuffer, length);
					}
				}
			}
		}

		os_free(pbufer);
	} else {
		return;
	}
}

LOCAL ICACHE_FLASH_ATTR
char *save_data(char *precv, uint16 length) {
	LOCAL char *precvbuffer;
	LOCAL uint32 dat_sumlength = 0;
	LOCAL uint32 totallength = 0;

	bool flag = false;
	char length_buf[10] = {0};
	char *ptemp = NULL;
	char *pdata = NULL;
	uint16 headlength = 0;

	ptemp = (char *)os_strstr(precv, "\r\n\r\n");

	if (ptemp != NULL) {
		length -= ptemp - precv;
		length -= 4;
		totallength += length;
		headlength = ptemp - precv + 4;
		pdata = (char *)os_strstr(precv, "Content-Length: ");

		if (pdata != NULL) {
			pdata += 16;
			precvbuffer = (char *)os_strstr(pdata, "\r\n");

			if (precvbuffer != NULL) {
				os_memcpy(length_buf, pdata, precvbuffer - pdata);
				dat_sumlength = atoi(length_buf);
			}
		} else {
			if (totallength != 0x00) {
				totallength = 0;
				dat_sumlength = 0;
				return NULL;
			}
		}
		if ((dat_sumlength + headlength) >= 1024) {
			precvbuffer = (char *)os_zalloc(headlength + 1);
			os_memcpy(precvbuffer, precv, headlength + 1);
		} else {
			precvbuffer = (char *)os_zalloc(dat_sumlength + headlength + 1);
			os_memcpy(precvbuffer, precv, os_strlen(precv));
		}
	} else {
		if (precvbuffer != NULL) {
			totallength += length;
			os_memcpy(precvbuffer + os_strlen(precvbuffer), precv, length);
		} else {
			totallength = 0;
			dat_sumlength = 0;
			return NULL;
		}
	}

	if (totallength == dat_sumlength) {
		totallength = 0;
		dat_sumlength = 0;
		return precvbuffer;
	} else {
		return NULL;
	}
}

LOCAL ICACHE_FLASH_ATTR
void webserver_recv(void *arg, char *pusrdata, unsigned short length) {
	LOCAL uint8 upgrade_lock = 0;
	if (upgrade_lock) {
		return;
	}

	struct espconn *ptrespconn = arg;

	char *precvbuffer = NULL, *p = "command";
	os_printf("webserver recv \n");
	URL_Frame *pURL_Frame = (URL_Frame *)os_zalloc(sizeof(URL_Frame));

	do {
		void (*handler)(struct espconn * ctx, const char *const parm) = NULL;

		precvbuffer = save_data(pusrdata, length); //数据保存到precvbuffer
		if (precvbuffer == NULL) {
			break;
		}
		os_printf("%s\n", precvbuffer);
		parse_url(precvbuffer, pURL_Frame);
//		os_printf("select is %s\nfilename is %s\n",pURL_Frame->pSelect,pURL_Frame->pFilename);

		if (pURL_Frame->Type == GET) {
			handler = getCommandHandler(pURL_Frame->pSelect, pURL_Frame->pFilename);//指定处理函数
			if (handler != NULL) {
				char *s = (char *)os_strstr(pusrdata, "&");
				handler(ptrespconn, s);//处理函数
			}
		} else if (pURL_Frame->Type == POST) {
			if (os_strstr(precvbuffer, p)) {
				os_printf("has command\n");
			}
		}
	} while (0);
	os_free(pURL_Frame);
}

LOCAL ICACHE_FLASH_ATTR
void webserver_recon(void *arg, sint8 err) {
	struct espconn *pesp_conn = arg;
	uint8_t *p = pesp_conn->proto.tcp->remote_ip;
	os_printf("webserver's %d.%d.%d.%d:%d err %d reconnect\n", p[0], p[1], p[2], p[3], pesp_conn->proto.tcp->remote_port, err);
}
LOCAL ICACHE_FLASH_ATTR
void webserver_discon(void *arg) {
	struct espconn *pesp_conn = arg;
	uint8_t *p = pesp_conn->proto.tcp->remote_ip;
	os_printf("webserver's %d.%d.%d.%d:%d disconnect\n",  p[0], p[1], p[2], p[3], pesp_conn->proto.tcp->remote_port);
}

LOCAL ICACHE_FLASH_ATTR
void webserver_listen(void *arg) {
	struct espconn *pesp_conn = arg;

	espconn_regist_recvcb(pesp_conn, webserver_recv);//接收到数据调用webserver_recv
	espconn_regist_reconcb(pesp_conn, webserver_recon);//
	espconn_regist_disconcb(pesp_conn, webserver_discon);//未接收到数据调用webserver_discon

}
#if 0
void sentcb(void) {

	os_printf("sentcb\n");
}

LOCAL void yeelinkget(void) {
//    struct espconn ptrespconn;
//	LOCAL esp_tcp esptcp;
	static u8_t i = 0;
	static u16_t length;
//	static char buf[8]={"led"};
	char str_tmp[128] = {0};
	char remote_server[] = "api.yeelink.net";
	static char apikey[] = "fa93b17ce5d3b8bfae0988b503d0b4fa";
	static char http_request[DATA_BUF_SIZE] = {0};	//声明为静态变量，防止堆栈溢出
//http://api.yeelink.net/v1.0/device/20857/sensor/36576/datapoints

	os_sprintf(str_tmp, "/v1.1/device/20857/sensor/36576/datapoints");
	os_sprintf(http_request , "POST %s HTTP/1.1\r\n", str_tmp);
	os_sprintf(str_tmp , "Host:%s\r\n" , remote_server);
	os_strcat(http_request , str_tmp);
	os_strcat(http_request , "Accept: */*\r\n");
	os_sprintf(str_tmp , "U-ApiKey:%s\r\n" , apikey); //需要替换为自己的APIKey
	os_strcat(http_request , str_tmp);
	os_strcat(http_request , "Content-Length: 12\r\n");
	os_strcat(http_request , "Content-Type: application/x-www-form-urlencoded\r\n");
	os_strcat(http_request , "Connection: close\r\n");
	os_strcat(http_request , "\r\n");

	os_strcat(http_request , "{\"value\":31}");

	length = os_strlen(http_request);
	os_printf("%s", http_request);
	espconn_regist_recvcb(&esp_conn, webserver_recv);
//	espconn_regist_sentcb(&esp_conn, sentcb);
	espconn_connect(&esp_conn);
	if (0 == espconn_sent(&esp_conn, http_request, length)) {
		os_printf("sent ok\n");
	}

	if ((i++) & 0x01) {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(0), LED_ON);
	} else {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(0), LED_OFF);
	}

}
#endif

LOCAL ICACHE_FLASH_ATTR
void webserver_start(int port) {
	LOCAL struct espconn esp_conn;
	LOCAL esp_tcp esptcp;

	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = port;
	os_printf("tcp port:%d\n", port);
	espconn_regist_connectcb(&esp_conn, webserver_listen);//web有连接时调用webserver_listen
	espconn_accept(&esp_conn);

//	os_timer_setfn(&led_timer,yeelinkget,NULL);
//	os_timer_arm(&led_timer,15000,1);
}


ICACHE_FLASH_ATTR
void webserver_init(void) {

	os_printf("webserver init\n");
	/*	if(wifi_get_opmode() == SOFTAP_MODE){
			os_printf("wifi mode is error\n");
			return;
		}
	*/
	/* 定时检测station模式的ip信息*/
	os_timer_disarm(&client_timer); //定时器关闭
	os_timer_setfn(&client_timer, (os_timer_func_t *)checkip_timer_cb, 1);//设置定时器中断函数
	os_timer_arm(&client_timer, CHECKIP_TIMER, 0); //初始化定时器

	os_printf("wifi mode is %d\n", wifi_get_opmode());
	webserver_start(SERVER_PORT);  //webserver开启80端口


}
