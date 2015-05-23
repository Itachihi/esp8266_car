#include "user_interface.h"
#include "smartconfig.h"
#include "user.h"

static u8_t flag = 0;

#define LED_TIMER 50

static os_timer_t led_timer;
static
void led_timer_cb(void) {
	static int status;
	status++;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_ONEKEY_NUM), status & 0x01);
}
static
void led_enable(void) {
	os_timer_disarm(&led_timer);//关闭定时器
	os_timer_setfn(&led_timer, led_timer_cb, NULL); //设置定时器中断函数
	os_timer_arm(&led_timer, LED_TIMER, 1);//初始化定时器
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_ONEKEY_NUM), 0);
}
static
void led_disable(void) {
	os_timer_disarm(&led_timer);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_ONEKEY_NUM), 1);
}

static
void wifi_connect(struct station_config *cfg) {
	struct station_config conf;
	os_memcpy(&conf, cfg, sizeof(struct station_config));
	wifi_station_set_config(&conf);
	wifi_station_disconnect();
	wifi_station_connect();
}

static
void sc_done_cb(void *dat) {
	wifi_connect((struct station_config *)dat);
	sc_disable();
}

static
void sc_softap_mode(void) {
	wifi_set_opmode(STATIONAP_MODE);//设置wifi模式为station+AP
	os_printf("station+ap mode \n");
	struct softap_config config;
	char password[33];
	char macaddr[6];
	wifi_softap_get_config(&config); //获得wifi的softAP接口参数
	wifi_get_macaddr(SOFTAP_IF, macaddr);//获取wifi的接口MAC地址
	config.ssid_len = os_sprintf(config.ssid, "%s_" MACSTR3, AP_SSID, MAC2STR3(macaddr));
	os_sprintf(config.password, "%s", PASSWORD);
	os_printf("ssid %s password %s\n", config.ssid, config.password);
	config.authmode = AUTH_WPA_WPA2_PSK;//加密方式

	wifi_softap_set_config(&config); //设置wifi的softAP参数
}

void sc_enable(void) {
	if (flag == 0) {
		flag = 1;
		os_printf("smartconfig start\n");
		smartconfig_start(SC_TYPE_AIRKISS, sc_done_cb);//sc开启后调用sc_done_cb
		led_enable();  //灯闪烁
	}
}

void sc_disable(void) {
	if (flag != 0) {
		led_disable();
		smartconfig_stop();
		flag = 0;
	}
	os_printf("sc_disable\n");
}

void sc_init(void) {
	struct station_config conf;
	wifi_station_get_config(&conf);                 //获得wifi的station接口参数
	if (conf.ssid[0] != 0 && conf.password[0] != 0) {
		os_printf("ssid: %s\n", conf.ssid);
		os_printf("pwd:  %s\n", conf.password);
		wifi_station_disconnect();                  //断开连接所有的路由
		wifi_station_connect();                     //连接路由
	} else {
		//	sc_enable();
		sc_softap_mode();                           //设置station+ap模式
	}
	PIN_FUNC_SELECT(LED_ONEKEY_MUX, LED_ONEKEY_FUNC);//管脚功能选择
}

