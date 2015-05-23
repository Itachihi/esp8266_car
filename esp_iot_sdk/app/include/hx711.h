#ifndef __HX711_H__
#define __HX711_H__

#include "c_types.h"

typedef enum {
	HX711_Mode_A_128 = 0,
	HX711_Mode_B_32,
	HX711_Mode_A_64
} HX711_Mode;

typedef struct {
	void (*sck_set)(bool b);
	void (*dio_set)(bool b);
	unsigned char (*dio_get)(void);
	void (*delayus)(unsigned short t);
	unsigned char mode;
} HX711_t;

void HX711_Start(HX711_t *hx);
void HX711_Stop(HX711_t *hx);
void HX711_Mearsure(HX711_t *hx);
int HX711_SetMode(HX711_t *hx, HX711_Mode mode);
int HX711_GetMode(HX711_t *hx);
unsigned int HX711_Read(HX711_t *hx);

#endif
