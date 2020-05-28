#include <stdint.h>
#include <stdbool.h>
#include "bsp.h"

#ifndef SMARTLOCK_H__
#define SMARTLOCK_H__

#define BEEP_PIN    NRF_GPIO_PIN_MAP(1, 7)
#define RELAY_PIN   NRF_GPIO_PIN_MAP(1, 11)
#define LOCKER_PIN  NRF_GPIO_PIN_MAP(1, 13)
#define LED_PIN     LED_1

#define LOCKER_DISCONNECT 1
#define LOCKER_CONNECT    0

//SmartLock protocol
#define SL_DEV_ID_LEN                   16
#define SL_CODE_NUM                     10
#define SL_CODE_TIME_LEN                10 //yymmddhhmm

#define SL_UNLOCK_CMD                   0x0
#define SL_LOCK_CMD                     0x1
#define SL_UPDATE_CODE_CMD              0x2
#define SL_APP_READY_CMD                0x3
#define SL_RESET_CMD                    0x4

#define SL_LOCK_SUCCESS                 0x10
#define SL_UNLOCK_SUCCESS               0x11
#define SL_UPDATE_SUCCESS               0x12
#define SL_RESET_SUCCESS                0x13
#define SL_DEV_NEED_UPDATE              0x14
#define SL_CODE_RUN_OUT                 0x15
#define SL_UNLOCK_FAIL                  0x16
#define SL_LOCK_FAIL                    0x17
#define SL_CODE_OUT_OF_DATE             0x18
#define SL_CODE_INVALID                 0x19
#define SL_DEV_ID_FAIL                  0x1A
#define SL_LOCK                         0xD0
#define SL_UNLOCK                       0xD1
#define SL_UNUSABLE                     0xD2

typedef struct
{
	int years;
	int months;
	int days;
	int hours;
	int minutes;
} smart_lock_code_date;

typedef struct
{
	int lock_status;
	int lock_connect;
	int alarm_cnt;
	int code_valid;
	uint8_t access_code[SL_CODE_NUM];
	uint8_t code_date[SL_CODE_TIME_LEN];
	uint8_t dev_id[SL_DEV_ID_LEN];
	bool update;
	bool beep;
	bool dev_lock;
} smart_lock_info;

extern smart_lock_info sl_info;

void smart_lock_pulse_led(void);

void smart_lock_parse_data(uint8_t *buf, int len);

void smart_lock_detect(void);

void smart_lock_init(void);

#endif // SMARTLOCK_H__
