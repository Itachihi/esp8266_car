/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_humiture.c
 *
 * Description: humiture demo's function realization
 *
 * Modification history:
 *     2014/5/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"

#if SENSOR_DEVICE
#include "user_sensor.h"

LOCAL os_timer_t sensor_sleep_timer;
LOCAL os_timer_t link_led_timer;
LOCAL uint8 link_led_level = 0;
LOCAL uint32 link_start_time;

#if HX711_SUB_DEVICE
#include "hx711.h"
#define GPIOx_Din	PERIPHS_IO_MUX_MTMS_U
#define NUM_Din 	14
#define FUNC_Din	FUNC_GPIO14
#define PINx_Din	GPIO_ID_PIN(NUM_Din)

#define GPIOx_Sck	PERIPHS_IO_MUX_MTDI_U
#define NUM_Sck 	12
#define FUNC_Sck	FUNC_GPIO12
#define PINx_Sck	GPIO_ID_PIN(NUM_Sck)
LOCAL ICACHE_FLASH_ATTR
void __sck_set(bool b) {
	GPIO_OUTPUT_SET(PINx_Sck, b);
}
LOCAL ICACHE_FLASH_ATTR
void __dio_set(bool b) {
	GPIO_OUTPUT_SET(PINx_Din, b);
}
LOCAL ICACHE_FLASH_ATTR
uint8 __dio_get(void) {
	return GPIO_INPUT_GET(PINx_Din);
}
LOCAL ICACHE_FLASH_ATTR
void __delayus(uint16 us) {
	os_delay_us(us);
}

LOCAL ETSTimer hx711_time;
LOCAL uint32 hx711_data = 0;
LOCAL HX711_t hx711 = {
	__sck_set,
	__dio_set,
	__dio_get,
	__delayus
};

ICACHE_FLASH_ATTR
void user_hx711_init(void) {
	HX711_t *hx = &hx711;

	PIN_FUNC_SELECT(GPIOx_Din, FUNC_Din);
	PIN_FUNC_SELECT(GPIOx_Sck, FUNC_Sck);

	GPIO_REG_WRITE(GPIO_PIN_ADDR(PINx_Din), GPIO_REG_READ(GPIO_PIN_ADDR(PINx_Din)) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)); //open drain;
	GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << NUM_Din));
	GPIO_REG_WRITE(GPIO_PIN_ADDR(PINx_Sck), GPIO_REG_READ(GPIO_PIN_ADDR(PINx_Sck)) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)); //open drain;
	GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << NUM_Sck));

	HX711_Start(hx);
	HX711_SetMode(hx, HX711_Mode_A_64);
	HX711_Mearsure(hx);
	os_delay_us(10);
	HX711_Read(hx);
}

ICACHE_FLASH_ATTR
uint32 user_hx711_read(void) {
	HX711_t *hx = &hx711;
	HX711_Mearsure(hx);
	os_delay_us(100);
	return HX711_Read(hx);
}

LOCAL ICACHE_FLASH_ATTR
void user_hx711_time_callback(void *arg) {
	static uint8 step = 0;
	HX711_t *hx = &hx711;
	if ((step++ & 0x01) == 0) {
		HX711_Mearsure(hx);
	} else {
		uint32 data = HX711_Read(hx);
		os_printf("hx: %d\n", data);
	}
}
#endif

LOCAL ICACHE_FLASH_ATTR
void user_link_led_init(void) {
	PIN_FUNC_SELECT(SENSOR_LINK_LED_IO_MUX, SENSOR_LINK_LED_IO_FUNC);
	PIN_FUNC_SELECT(SENSOR_UNUSED_LED_IO_MUX, SENSOR_UNUSED_LED_IO_FUNC);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(SENSOR_UNUSED_LED_IO_NUM), 0);
}

ICACHE_FLASH_ATTR
void user_link_led_output(uint8 level) {
	GPIO_OUTPUT_SET(GPIO_ID_PIN(SENSOR_LINK_LED_IO_NUM), level);
}

LOCAL ICACHE_FLASH_ATTR
void user_link_led_timer_cb(void) {
	link_led_level = (~link_led_level) & 0x01;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(SENSOR_LINK_LED_IO_NUM), link_led_level);
}

ICACHE_FLASH_ATTR
void user_link_led_timer_init(void) {
	link_start_time = system_get_time();

	os_timer_disarm(&link_led_timer);
	os_timer_setfn(&link_led_timer, (os_timer_func_t *)user_link_led_timer_cb, NULL);
	os_timer_arm(&link_led_timer, 50, 1);
	link_led_level = 0;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(SENSOR_LINK_LED_IO_NUM), link_led_level);
}

ICACHE_FLASH_ATTR
void user_link_led_timer_done(void) {
	os_timer_disarm(&link_led_timer);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(SENSOR_LINK_LED_IO_NUM), 0);
}

ICACHE_FLASH_ATTR
void user_sensor_deep_sleep_enter(void) {
	system_deep_sleep(SENSOR_DEEP_SLEEP_TIME > link_start_time \
					  ? SENSOR_DEEP_SLEEP_TIME - link_start_time : 30000000);
}

ICACHE_FLASH_ATTR
void user_sensor_deep_sleep_disable(void) {
	os_timer_disarm(&sensor_sleep_timer);
}

ICACHE_FLASH_ATTR
void user_sensor_deep_sleep_init(uint32 time) {
	os_timer_disarm(&sensor_sleep_timer);
	os_timer_setfn(&sensor_sleep_timer, (os_timer_func_t *)user_sensor_deep_sleep_enter, NULL);
	os_timer_arm(&sensor_sleep_timer, time, 0);
}

/******************************************************************************
 * FunctionName : user_humiture_init
 * Description  : init humiture function, include key and mvh3004
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
ICACHE_FLASH_ATTR
void user_sensor_init(uint8 active) {

#if HX711_SUB_DEVICE
	user_hx711_init();
//	os_timer_setfn(&hx711_time, user_hx711_time_callback, NULL);
//	os_timer_arm(&hx711_time, 500, TRUE);
#endif

#ifdef SENSOR_DEEP_SLEEP
	if (wifi_get_opmode() != STATIONAP_MODE) {
		if (active == 1) {
			user_sensor_deep_sleep_init(SENSOR_DEEP_SLEEP_TIME / 1000);
		} else {
			user_sensor_deep_sleep_init(SENSOR_DEEP_SLEEP_TIME / 1000 / 3 * 2);
		}
	}
#endif
}
#endif

