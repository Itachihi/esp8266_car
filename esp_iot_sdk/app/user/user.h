/*
 * user.h
 *
 *  Created on: 2015-2-28
 *      Author: Administrator
 */

#ifndef __USER_H__
#define __USER_H__

#include <stdbool.h>
#include "driver/key.h"


#define KEY_ONEKEY_MUX		PERIPHS_IO_MUX_GPIO2_U
#define KEY_ONEKEY_NUM		2
#define KEY_ONEKEY_FUNC		FUNC_GPIO2

#define LED_ONEKEY_MUX		PERIPHS_IO_MUX_GPIO0_U
#define LED_ONEKEY_NUM		0
#define LED_ONEKEY_FUNC		FUNC_GPIO0


#define LED_ON              0
#define LED_OFF             1

void sc_init(void);
void sc_enable(void);
void sc_disable(void);
void webserver_init(void);

#endif /* USER_H_ */
