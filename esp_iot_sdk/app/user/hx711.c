#include "hx711.h"

#define RETRY 10

void HX711_Start(HX711_t *hx) {
	hx->dio_set(true);
	hx->sck_set(true);
}

void HX711_Stop(HX711_t *hx) {
	//	SCK:	_/-----
	hx->sck_set(true);
	hx->delayus(60);
}

int HX711_SetMode(HX711_t *hx, HX711_Mode mode) {
	if (mode == hx->mode) {
		return -1;
	}
	switch (mode) {
	case HX711_Mode_A_128:
	case HX711_Mode_A_64:
	case HX711_Mode_B_32:
		hx->mode = mode;
		break;
	default:
		if (hx->mode == HX711_Mode_A_128) {
			return -1;
		}
		hx->mode = HX711_Mode_A_128;
		break;
	}
	return 0;
}

int HX711_GetMode(HX711_t *hx) {
	return hx->mode;
}

void HX711_Mearsure(HX711_t *hx) {
	//	DO: --\____
	hx->dio_set(true);
	hx->sck_set(false);
}

unsigned int HX711_Read(HX711_t *hx) {
	int retry = RETRY;
	int i;
	int dat = 0;
	do {
		retry--;
		if (retry == 0) {
			return -1;
		}
		hx->delayus(100);
	} while (hx->dio_get());
	for (i = 23; i >= 0; i--) {
		hx->sck_set(true);
		hx->delayus(1);
		hx->sck_set(false);
		if (hx->dio_get()) {
			dat |= (1 << i);
		}
		hx->delayus(1);
	}
	i = hx->mode;
	do {
		hx->sck_set(true);
		hx->delayus(2);
		hx->sck_set(false);
		hx->delayus(2);
	} while (i--);
	hx->dio_set(true);
	if (dat & 0x800000) {
		dat |= 0xFF000000;
	}
	return dat;
}

