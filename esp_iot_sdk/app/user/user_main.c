/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "user_interface.h"
#include "driver/uart.h"
#include "driver/key.h"
#include "user.h"
//#include "Gpio.h"
#include "driver/pwm.h"
#include "espconn.h"
#include "user_webserver.h"
#include "webserver_process.h"
#include "driver/i2c_master.h"
#include "at_wifiCmd.h"

#define KEY_NUM		1
LOCAL struct keys_param keys;
LOCAL struct single_key_param *single_key[KEY_NUM];


struct motor_pwm_param {
	uint16 pwm_freq;
	uint8  pwm_duty[PWM_CHANNEL];
};

LOCAL struct motor_pwm_param motor_param;
LOCAL os_timer_t led_timer;

LOCAL void ICACHE_FLASH_ATTR
smartrobot_init(void) {
	//uint8 du;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(0), 0);

	motor_param.pwm_freq    = 100;   //���Ϊ500
	motor_param.pwm_duty[0] = 0;     //0-255
	motor_param.pwm_duty[1] = 0;
	motor_param.pwm_duty[2] = 0;
	motor_param.pwm_duty[3] = 0;
	pwm_init(motor_param.pwm_freq, motor_param.pwm_duty);
	pwm_start();
}


LOCAL void ICACHE_FLASH_ATTR
key_ok_short_press(void) {
	sc_enable();  //����smartconfig���ù���
}
LOCAL void ICACHE_FLASH_ATTR
key_ok_long_press(void) {
	system_restore();
	system_restart();
}

LOCAL void ICACHE_FLASH_ATTR
user_key_init(void) {
	single_key[0] = key_init_single(KEY_ONEKEY_NUM, KEY_ONEKEY_MUX, KEY_ONEKEY_FUNC,
									key_ok_long_press, key_ok_short_press);//ָ�������жϴ�����

	keys.key_num = KEY_NUM;
	keys.single_key = single_key;

	key_init(&keys);
}

LOCAL void ICACHE_FLASH_ATTR
gpio(void) {
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(0), 0);
}

LOCAL void ICACHE_FLASH_ATTR
init_done_cb(void) {
	sc_init();             //wifi��ʼ��
//    user_key_init();       //����GPIO2�ж�����
	user_devicefind_init();//espconn�ṹ�������ʼ��
	webserver_init();      //webserver ��ʼ��
	smartrobot_init();
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*****************************************************************************/
uint8_t BXSJ[23];
BOOL at_ipMux;
struct ip_info pTempIp;

void user_init(void) {  //��ں���
	uart_init(BIT_RATE_115200, BIT_RATE_115200); //���ڳ�ʼ��uart0�����շ�����
	system_set_os_print(1);    //�򿪴�ӡ����
	os_printf("SDK version:%s\n", system_get_sdk_version());
	uart0_sendStr("hello world!\n");
#if 1
	wifi_set_opmode(STATION_MODE);           //����wifiģʽΪstation
//	wifi_set_opmode(SOFTAP_MODE);
//	os_timer_setfn(&led_timer,led,NULL);
//	os_timer_arm(&led_timer,1000,1);

	system_init_done_cb(init_done_cb);       // ע��ϵͳ��ɳ�ʼ���Ļص�����

#else
	struct softap_config config;
	char macaddr[6];
	char cmdbuf[50];

	wifi_set_opmode(SOFTAP_MODE);//����ģʽSOFTAP
	os_printf("softap mode\n");
	wifi_get_macaddr(SOFTAP_IF, macaddr);
	os_sprintf(config.ssid, "%s_" MACSTR3, AP_SSID, MAC2STR3(macaddr));
	os_sprintf(cmdbuf, "=\"%s\",\"%s\",11,4", config.ssid, PASSWORD);
	at_setupCmdCwsap(0, cmdbuf);
	at_ipMux = TRUE; // ����Ϊ������
	os_memset(cmdbuf, 0, 50);
	os_sprintf(cmdbuf, "=1,%d", TCP_SERVER_PORT);
	at_setupCmdCipserver(0, cmdbuf); //����ΪTCP������
	wifi_softap_get_config(&config);
	os_printf("ssid %s password %s \n", config.ssid, config.password);
//	spi_flash_read(0x3c*4096,(uint32*)&BXSJ,sizeof(BXSJ));//���洢����BXSJ������
//	os_printf(BXSJ);os_printf(BXSJ);os_printf(BXSJ);

	at_init();

#endif

}

