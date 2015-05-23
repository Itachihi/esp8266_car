#include "os_type.h"
#include "osapi.h"
#include "json/jsontree.h"
#include "user_iot_version.h"
#include "user.h"
#include "driver/pwm.h"

/******************************************************************************
 * FunctionName : version_get
 * Description  : set up the device version paramer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
version_get(struct jsontree_context *js_ctx) {
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);
	char string[32];

	if (os_strncmp(path, "hardware", 8) == 0) {
#if SENSOR_DEVICE
		os_sprintf(string, "0.3");
#else
		os_sprintf(string, "0.1");
#endif
	} else if (os_strncmp(path, "sdk_version", 11) == 0) {
		os_sprintf(string, "%s", system_get_sdk_version());
	} else if (os_strncmp(path, "iot_version", 11) == 0) {
		os_sprintf(string, "%s%d.%d.%dt%d(%s)", VERSION_TYPE, IOT_VERSION_MAJOR, \
				   IOT_VERSION_MINOR, IOT_VERSION_REVISION, device_type, UPGRADE_FALG);
	}

	os_printf("version get \n");
	jsontree_write_string(js_ctx, string);

	return 0;
}
/******************************************************************************
 * FunctionName : device_get
 * Description  : set up the device information parmer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
device_get(struct jsontree_context *js_ctx) {
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);

	if (os_strncmp(path, "manufacture", 11) == 0) {
		jsontree_write_string(js_ctx, "Espressif Systems");
	} else if (os_strncmp(path, "product", 7) == 0) {
#if SENSOR_DEVICE
#if HUMITURE_SUB_DEVICE
		jsontree_write_string(js_ctx, "Humiture");
#elif FLAMMABLE_GAS_SUB_DEVICE
		jsontree_write_string(js_ctx, "Flammable Gas");
#elif HX711_SUB_DEVICE
		jsontree_write_string(js_ctx, "Electronic Weigher");
#endif
#endif
#if PLUG_DEVICE
		jsontree_write_string(js_ctx, "Plug");
#endif
#if LIGHT_DEVICE
		jsontree_write_string(js_ctx, "Light");
#endif
	}

	return 0;
}

LOCAL struct jsontree_callback version_callback =
	JSONTREE_CALLBACK(version_get, NULL);
LOCAL struct jsontree_callback device_callback =
	JSONTREE_CALLBACK(device_get, NULL);
JSONTREE_OBJECT(device_tree,
				JSONTREE_PAIR("product", &device_callback),
				JSONTREE_PAIR("manufacturer", &device_callback));
JSONTREE_OBJECT(version_tree,
				JSONTREE_PAIR("hardware", &version_callback),
				JSONTREE_PAIR("sdk_version", &version_callback),
				JSONTREE_PAIR("iot_version", &version_callback),
			   );
JSONTREE_OBJECT(info_tree,
				JSONTREE_PAIR("Version", &version_tree),
				JSONTREE_PAIR("Device", &device_tree));

JSONTREE_OBJECT(INFOTree,
				JSONTREE_PAIR("info", &info_tree));

struct jsontree_value *getINFOTree(void) {
	return (struct jsontree_value *)&INFOTree;
}



/**********    set   light **********************************************************/
LOCAL int ICACHE_FLASH_ATTR
light_set(struct jsontree_context *js_ctx) {

	uint8_t data;
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 2);

	if (os_strncmp(path, "on", 2) == 0) {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_ONEKEY_NUM), LED_ON);
	} else if (os_strncmp(path, "off", 3) == 0) {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_ONEKEY_NUM), LED_OFF);
	}

	data = GPIO_INPUT_GET(GPIO_ID_PIN(LED_ONEKEY_NUM));
	if (data) {
		jsontree_write_string(js_ctx, "OFF");
	} else {
		jsontree_write_string(js_ctx, "ON");
		//jsontree_write_int(js_ctx,data);
	}

	return 0;
}


LOCAL struct jsontree_callback lightset_callback =
	JSONTREE_CALLBACK(light_set, NULL);


JSONTREE_OBJECT(lighton_tree,
				JSONTREE_PAIR("light", &lightset_callback));

JSONTREE_OBJECT(LightOnTree,
				JSONTREE_PAIR("on", &lighton_tree));

struct jsontree_value *getSetLightOnTree(void) {
	return (struct jsontree_value *)&LightOnTree;
}
JSONTREE_OBJECT(lightoff_tree,
				JSONTREE_PAIR("light", &lightset_callback));

JSONTREE_OBJECT(LightOffTree,
				JSONTREE_PAIR("off", &lightoff_tree));

struct jsontree_value *getSetLightOffTree(void) {
	return (struct jsontree_value *)&LightOffTree;
}

/**************   car control   *****************************************/
LOCAL unsigned char TxBuf1[] = "PL0\r\n";
LOCAL unsigned char TxBuf2[] = "PL0 SQ0 ONCE100\r\n ";
LOCAL unsigned char TxBuf3[] = "PL0 SQ1 ONCE100\r\n";
LOCAL os_timer_t pwm_timer;
LOCAL unsigned char flag = 0;
LOCAL uint8_t stop_flag = 1;
#define  PWM_TIMER  10

LOCAL void ICACHE_FLASH_ATTR
pwm_timer_set(void) {
	static unsigned int  i = 0, j = 255;
	unsigned int duty;
	if (flag == 0) {
		if (stop_flag == 1) {
			i++;
			pwm_set_duty(i, 0);
			pwm_set_duty(i, 1);
			pwm_set_duty(i, 2);
			pwm_set_duty(i, 3);
			pwm_start();
			//	duty=pwm_get_duty(0);
			//	os_printf("pwm is:%d\n",duty);
			if (i >= 255) {
				os_timer_disarm(&pwm_timer);
				i = 0;
			}
		} else {
			pwm_set_duty(255, 0);
			pwm_set_duty(255, 1);
			pwm_set_duty(255, 2);
			pwm_set_duty(255, 3);
			pwm_start();
			os_timer_disarm(&pwm_timer);
		}
//		duty=pwm_get_duty(0);
//		os_printf("pwm:%d\n",duty);
	} else if (flag == 1) {
		if (stop_flag == 0) {
			j--;
			pwm_set_duty(j,  0);
			pwm_set_duty(255, 1);
			pwm_set_duty(j,  2);
			pwm_set_duty(255, 3);
			pwm_start();
			if (j == 0) {
				os_timer_disarm(&pwm_timer);
				j = 255;
			}
		} else {
			i++;
			pwm_set_duty(0,  0);
			pwm_set_duty(i, 1);
			pwm_set_duty(0,  2);
			pwm_set_duty(i, 3);
			pwm_start();
			if (i >= 255) {
				os_timer_disarm(&pwm_timer);
				i = 100;
			}
		}
	} else if (flag == 2) {
		if (stop_flag == 0) {
			j--;
			pwm_set_duty(255, 0);
			pwm_set_duty(j, 1);
			pwm_set_duty(255, 2);
			pwm_set_duty(j, 3);
			pwm_start();
			if (j == 0) {
				os_timer_disarm(&pwm_timer);
				j = 255;
			}
		} else {
			i++;
			pwm_set_duty(i, 0);
			pwm_set_duty(0, 1);
			pwm_set_duty(i, 2);
			pwm_set_duty(0, 3);
			pwm_start();
			if (i >= 255) {
				os_timer_disarm(&pwm_timer);
				i = 100;
			}
		}

	} else if (flag == 3) {
		if (stop_flag == 0) {
			j--;
			pwm_set_duty(j, 0);
			pwm_set_duty(j, 1);
			pwm_set_duty(j, 2);
			pwm_set_duty(j, 3);
			pwm_start();
			if (j == 0) {
				os_timer_disarm(&pwm_timer);
				j = 255;
			}
		} else {
			pwm_set_duty(0, 0);
			pwm_set_duty(0, 1);
			pwm_set_duty(0, 2);
			pwm_set_duty(0, 3);
			pwm_start();
			os_timer_disarm(&pwm_timer);
		}
	}
}

LOCAL int ICACHE_FLASH_ATTR
controlcar(struct jsontree_context *js_ctx) {

	uint8_t data, i = 255;

	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 2);

	//	os_printf("%s \n",path);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(0), 0);

	if (0 == os_strncmp(path, "arm1", 4)) {
		uart0_sendStr(TxBuf1);
		jsontree_write_string(js_ctx, "arm1");
	} else if (0 == os_strncmp(path, "arm2", 4)) {
		uart0_sendStr(TxBuf2);
		jsontree_write_string(js_ctx, "arm2");
	} else if (0 == os_strncmp(path, "arm3", 4)) {
		uart0_sendStr(TxBuf3);
		jsontree_write_string(js_ctx, "arm3");
	}

	if (0 == os_strncmp(path, "forward", 7)) {
		flag = 0;
		os_timer_setfn(&pwm_timer, pwm_timer_set, NULL);
		os_timer_arm(&pwm_timer, PWM_TIMER, 1);
		stop_flag = 0;
		jsontree_write_string(js_ctx, "forward");
	} else if (0 == os_strncmp(path, "left", 4)) {
		flag = 1;
		os_timer_setfn(&pwm_timer, pwm_timer_set, NULL);
		os_timer_arm(&pwm_timer, PWM_TIMER, 1);
		stop_flag = 0;
		jsontree_write_string(js_ctx, "left");
	} else if (0 == os_strncmp(path, "right", 5)) {
		flag == 2;
		os_timer_setfn(&pwm_timer, pwm_timer_set, NULL);
		os_timer_arm(&pwm_timer, PWM_TIMER, 1);
		stop_flag = 0;
		jsontree_write_string(js_ctx, "right");
	} else if (0 == os_strncmp(path, "stop", 4)) {
		flag = 3;
		os_timer_setfn(&pwm_timer, pwm_timer_set, NULL);
		os_timer_arm(&pwm_timer, PWM_TIMER, 1);
		stop_flag = 1;
		jsontree_write_string(js_ctx, "stop");
	} else if (0 == os_strncmp(path, "back", 4)) {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(0), 1);
		pwm_set_duty(0, 0);
		pwm_set_duty(0, 1);
		pwm_set_duty(0, 2);
		pwm_set_duty(0, 3);
		pwm_start();
		jsontree_write_string(js_ctx, "back");
	}

	return 0;
}


LOCAL struct jsontree_callback carcontrol_callback =
	JSONTREE_CALLBACK(controlcar, NULL);

JSONTREE_OBJECT(gofoward_tree,
				JSONTREE_PAIR("car", &carcontrol_callback));

JSONTREE_OBJECT(GoForwardTree,
				JSONTREE_PAIR("forward", &gofoward_tree));

struct jsontree_value *getControlCarForwardTree(void) {
	return (struct jsontree_value *)&GoForwardTree;
}

JSONTREE_OBJECT(goleft_tree,
				JSONTREE_PAIR("car", &carcontrol_callback));

JSONTREE_OBJECT(GoLeftTree,
				JSONTREE_PAIR("left", &goleft_tree));

struct jsontree_value *getControlCarLeftTree(void) {
	return (struct jsontree_value *)&GoLeftTree;
}

JSONTREE_OBJECT(goright_tree,
				JSONTREE_PAIR("car", &carcontrol_callback));

JSONTREE_OBJECT(GoRightTree,
				JSONTREE_PAIR("right", &goright_tree));

struct jsontree_value *getControlCarRightTree(void) {
	return (struct jsontree_value *)&GoRightTree;
}

JSONTREE_OBJECT(stop_tree,
				JSONTREE_PAIR("car", &carcontrol_callback));

JSONTREE_OBJECT(StopTree,
				JSONTREE_PAIR("stop", &stop_tree));

struct jsontree_value *getControlCarStopTree(void) {
	return (struct jsontree_value *)&StopTree;
}

JSONTREE_OBJECT(back_tree,
				JSONTREE_PAIR("car", &carcontrol_callback));

JSONTREE_OBJECT(BackTree,
				JSONTREE_PAIR("back", &back_tree));

struct jsontree_value *getControlCarBackTree(void) {
	return (struct jsontree_value *)&BackTree;
}

JSONTREE_OBJECT(arm1_tree,
				JSONTREE_PAIR("car", &carcontrol_callback));

JSONTREE_OBJECT(Arm1Tree,
				JSONTREE_PAIR("arm1", &arm1_tree));

struct jsontree_value *getArm1Tree(void) {
	return (struct jsontree_value *)&Arm1Tree;
}
JSONTREE_OBJECT(arm2_tree,
				JSONTREE_PAIR("car", &carcontrol_callback));

JSONTREE_OBJECT(Arm2Tree,
				JSONTREE_PAIR("arm2", &arm2_tree));

struct jsontree_value *getArm2Tree(void) {
	return (struct jsontree_value *)&Arm2Tree;
}

JSONTREE_OBJECT(arm3_tree,
				JSONTREE_PAIR("car", &carcontrol_callback));

JSONTREE_OBJECT(Arm3Tree,
				JSONTREE_PAIR("arm3", &arm3_tree));

struct jsontree_value *getArm3Tree(void) {
	return (struct jsontree_value *)&Arm3Tree;
}

/*****************************************************************************************/

#if HX711_SUB_DEVICE
static int weight_data;
LOCAL int ICACHE_FLASH_ATTR
weight_get_raw(struct jsontree_context *js_ctx) {
	weight_data = user_hx711_read();
	jsontree_write_int(js_ctx, weight_data);
	return 0;
}
LOCAL int ICACHE_FLASH_ATTR
weight_get_result(struct jsontree_context *js_ctx) {
	int ratio = 105;
	int data = (weight_data * ratio) / 1000 + (-12054);
	jsontree_write_int(js_ctx, data);
	return 0;
}

LOCAL struct jsontree_callback weight_raw_callback =
	JSONTREE_CALLBACK(weight_get_raw, NULL);
LOCAL struct jsontree_callback weight_result_callback =
	JSONTREE_CALLBACK(weight_get_result, NULL);
JSONTREE_OBJECT(WeightRaw,
				JSONTREE_PAIR("raw", &weight_raw_callback),
				JSONTREE_PAIR("result", &weight_result_callback));
JSONTREE_OBJECT(WeightInfo,
				JSONTREE_PAIR("weight", &WeightRaw));

struct jsontree_value *getWeightInfo(void) {
	return (struct jsontree_value *)&WeightInfo;
}
/*****************************************************************************************/
#endif

LOCAL int ICACHE_FLASH_ATTR
connect_status_get(struct jsontree_context *js_ctx) {
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);

	if (os_strncmp(path, "status", 8) == 0) {
		jsontree_write_int(js_ctx, wifi_station_get_connect_status());
	}

	return 0;
}
LOCAL struct jsontree_callback connect_status_callback =
	JSONTREE_CALLBACK(connect_status_get, NULL);
JSONTREE_OBJECT(status_sub_tree,
				JSONTREE_PAIR("status", &connect_status_callback));
JSONTREE_OBJECT(connect_status_tree,
				JSONTREE_PAIR("Status", &status_sub_tree));
JSONTREE_OBJECT(con_status_tree,
				JSONTREE_PAIR("info", &connect_status_tree));

struct jsontree_value *getConStatusTree(void) {
	return (struct jsontree_value *)&con_status_tree;
}
/*****************************************************************************************/

//LOCAL struct jsontree_callback wifi_softap_callback =
//    JSONTREE_CALLBACK(wifi_softap_get, wifi_softap_set);
//
//JSONTREE_OBJECT(softap_config_tree,
//                JSONTREE_PAIR("authmode", &wifi_softap_callback),
//                JSONTREE_PAIR("channel", &wifi_softap_callback),
//                JSONTREE_PAIR("ssid", &wifi_softap_callback),
//                JSONTREE_PAIR("password", &wifi_softap_callback));
//JSONTREE_OBJECT(softap_ip_tree,
//                JSONTREE_PAIR("ip", &wifi_softap_callback),
//                JSONTREE_PAIR("mask", &wifi_softap_callback),
//                JSONTREE_PAIR("gw", &wifi_softap_callback));
//JSONTREE_OBJECT(get_softap_tree,
//                JSONTREE_PAIR("Connect_Softap", &softap_config_tree),
//                JSONTREE_PAIR("Ipinfo_Softap", &softap_ip_tree));
//JSONTREE_OBJECT(set_softap_tree,
//                JSONTREE_PAIR("Ipinfo_Softap", &softap_config_tree));
//
//JSONTREE_OBJECT(get_wifi_tree,
//                JSONTREE_PAIR("Station", &get_station_tree),
//                JSONTREE_PAIR("Softap", &get_softap_tree));
//JSONTREE_OBJECT(set_wifi_tree,
//                JSONTREE_PAIR("Station", &set_station_tree),
//                JSONTREE_PAIR("Softap", &set_softap_tree));
//
//JSONTREE_OBJECT(wifi_response_tree,
//                JSONTREE_PAIR("Response", &get_wifi_tree));
//JSONTREE_OBJECT(wifi_request_tree,
//                JSONTREE_PAIR("Request", &set_wifi_tree));
//
//JSONTREE_OBJECT(wifi_info_tree,
//                JSONTREE_PAIR("wifi", &wifi_response_tree));
//JSONTREE_OBJECT(wifi_req_tree,
//                JSONTREE_PAIR("wifi", &wifi_request_tree));

/*****************************************************************************************/

#if LIGHT_DEVICE
#include "user_light.h"
LOCAL int ICACHE_FLASH_ATTR
light_status_get(struct jsontree_context *js_ctx) {
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);

	if (os_strncmp(path, "red", 3) == 0) {
		jsontree_write_int(js_ctx, user_light_get_duty(LIGHT_RED));
	} else if (os_strncmp(path, "green", 5) == 0) {
		jsontree_write_int(js_ctx, user_light_get_duty(LIGHT_GREEN));
	} else if (os_strncmp(path, "blue", 4) == 0) {
		jsontree_write_int(js_ctx, user_light_get_duty(LIGHT_BLUE));
	} else if (os_strncmp(path, "freq", 4) == 0) {
		jsontree_write_int(js_ctx, user_light_get_freq());
	}

	return 0;
}

LOCAL int ICACHE_FLASH_ATTR
light_status_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser) {
	int type;

	while ((type = jsonparse_next(parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			if (jsonparse_strcmp_value(parser, "red") == 0) {
				uint8 status;
				jsonparse_next(parser);
				jsonparse_next(parser);
				status = jsonparse_get_value_as_int(parser);
				//                os_printf("R: %d \n",status);
				user_light_set_duty(status, LIGHT_RED);
			} else if (jsonparse_strcmp_value(parser, "green") == 0) {
				uint8 status;
				jsonparse_next(parser);
				jsonparse_next(parser);
				status = jsonparse_get_value_as_int(parser);
				//                os_printf("G: %d \n",status);
				user_light_set_duty(status, LIGHT_GREEN);
			} else if (jsonparse_strcmp_value(parser, "blue") == 0) {
				uint8 status;
				jsonparse_next(parser);
				jsonparse_next(parser);
				status = jsonparse_get_value_as_int(parser);
				//                os_printf("B: %d \n",status);
				user_light_set_duty(status, LIGHT_BLUE);
			} else if (jsonparse_strcmp_value(parser, "freq") == 0) {
				uint16 status;
				jsonparse_next(parser);
				jsonparse_next(parser);
				status = jsonparse_get_value_as_int(parser);
				//                os_printf("FREQ: %d \n",status);
				user_light_set_freq(status);
			}
		}
	}

	user_light_restart();

	return 0;
}

LOCAL struct jsontree_callback light_callback =
	JSONTREE_CALLBACK(light_status_get, light_status_set);

JSONTREE_OBJECT(rgb_tree,
				JSONTREE_PAIR("red", &light_callback),
				JSONTREE_PAIR("green", &light_callback),
				JSONTREE_PAIR("blue", &light_callback));
JSONTREE_OBJECT(sta_tree,
				JSONTREE_PAIR("freq", &light_callback),
				JSONTREE_PAIR("rgb", &rgb_tree));
JSONTREE_OBJECT(PwmTree,
				JSONTREE_PAIR("light", &sta_tree));

struct jsontree_value *getPwmTree(void) {
	return (struct jsontree_value *)&PwmTree;
}
#endif
/*****************************************************************************************/


#if PLUG_DEVICE
/******************************************************************************
 * FunctionName : status_get
 * Description  : set up the device status as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
status_get(struct jsontree_context *js_ctx) {
	if (user_plug_get_status() == 1) {
		jsontree_write_int(js_ctx, 1);
	} else {
		jsontree_write_int(js_ctx, 0);
	}

	return 0;
}

/******************************************************************************
 * FunctionName : status_set
 * Description  : parse the device status parmer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 *                parser -- A pointer to a JSON parser state
 * Returns      : result
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
status_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser) {
	int type;

	while ((type = jsonparse_next(parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {
			if (jsonparse_strcmp_value(parser, "status") == 0) {
				uint8 status;
				jsonparse_next(parser);
				jsonparse_next(parser);
				status = jsonparse_get_value_as_int(parser);
				user_plug_set_status(status);
			}
		}
	}

	return 0;
}

LOCAL struct jsontree_callback status_callback =
	JSONTREE_CALLBACK(status_get, status_set);

JSONTREE_OBJECT(status_tree,
				JSONTREE_PAIR("status", &status_callback));
JSONTREE_OBJECT(response_tree,
				JSONTREE_PAIR("Response", &status_tree));
JSONTREE_OBJECT(StatusTree,
				JSONTREE_PAIR("switch", &response_tree));

struct jsontree_value *getStatusTree(void) {
	return (struct jsontree_value *)&StatusTree;
}
#endif
/*****************************************************************************************/

/******************************************************************************
 * FunctionName : userbin_get
 * Description  : get up the user bin paramer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
userbin_get(struct jsontree_context *js_ctx) {
	const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);
	char string[32];

	if (os_strncmp(path, "status", 8) == 0) {
		os_sprintf(string, "200");
	} else if (os_strncmp(path, "user_bin", 8) == 0) {
		if (system_upgrade_userbin_check() == 0x00) {
			os_sprintf(string, "user1.bin");
		} else if (system_upgrade_userbin_check() == 0x01) {
			os_sprintf(string, "user2.bin");
		} else {
			return 0;
		}
	}

	jsontree_write_string(js_ctx, string);

	return 0;
}

LOCAL struct jsontree_callback userbin_callback =
	JSONTREE_CALLBACK(userbin_get, NULL);

JSONTREE_OBJECT(userbin_tree,
				JSONTREE_PAIR("status", &userbin_callback),
				JSONTREE_PAIR("user_bin", &userbin_callback));
JSONTREE_OBJECT(userinfo_tree, JSONTREE_PAIR("user_info", &userbin_tree));

struct jsontree_value *getUserInfo(void) {
	return (struct jsontree_value *)&userinfo_tree;
}
/*****************************************************************************************/
