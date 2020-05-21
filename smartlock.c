#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "bsp.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "smartlock.h"
#include "ble_m.h"

smart_lock_info sl_info;
smart_lock_code_date sl_code_date;
bool ble_lock_handle = false;
uint8_t tmp_dev_id[SL_DEV_ID_LEN] = {0};

void smart_lock_active_beep(bool enable)
{
	if (sl_info.beep == true && enable == true)
		return;

	if (sl_info.beep == false && enable == false)
		return;

	if (enable)
		sl_info.beep = true;
	else
		sl_info.beep = false;

	nrf_gpio_pin_toggle(BEEP_PIN);
}

void smart_lock_pulse_led(void)
{
	for (int i = 0; i < 2; i++)
	{
		nrf_gpio_pin_clear(LED_PIN);
		nrf_delay_ms(100);
    nrf_gpio_pin_set(LED_PIN);
		nrf_delay_ms(100);
		nrf_gpio_pin_clear(LED_PIN);
	}
}

int smart_lock_read_locker(void)
{
	return nrf_gpio_pin_read(LOCKER_PIN);
}

void smart_lock_locked(void)
{
	NRF_LOG_INFO("lock status = %d", sl_info.lock_status);
	if (sl_info.lock_status != SL_LOCK)
	{
		NRF_LOG_INFO("active relay to lock");
		sl_info.lock_status = SL_LOCK;
		nrf_gpio_pin_toggle(RELAY_PIN);
	}
}

void smart_lock_unlocked(void)
{
  NRF_LOG_INFO("lock status = %d", sl_info.lock_status);
  if (sl_info.lock_status != SL_UNLOCK)
  {
		NRF_LOG_INFO("active relay to unlock");
		sl_info.lock_status = SL_UNLOCK;
		nrf_gpio_pin_toggle(RELAY_PIN);
	}
}

void smart_lock_add_info(uint8_t *buf, int len, int code_len)
{
	memcpy(sl_info.access_code, buf + 2, code_len);

	memcpy(sl_info.dev_id, buf + 5, SL_DEV_ID_LEN);

	memcpy(sl_info.code_date, buf + 21, SL_CODE_TIME_LEN);

	sl_code_date.years = sl_info.code_date[0] * 10 + sl_info.code_date[1];
	sl_code_date.months = sl_info.code_date[2] * 10 + sl_info.code_date[3];
	sl_code_date.days = sl_info.code_date[4] * 10 + sl_info.code_date[5];
	sl_code_date.hours = sl_info.code_date[6] * 10 + sl_info.code_date[7];
	sl_code_date.minutes = sl_info.code_date[8] * 10 + sl_info.code_date[9];

	sl_info.code_valid = code_len;
	sl_info.update = true;
}

int smart_lock_compare_date(uint8_t *buf)
{
	int years = 0, months = 0, days = 0, hours = 0, minutes = 0;
	uint8_t tmp[SL_CODE_TIME_LEN] = {0};

	memcpy(tmp, buf + 19, SL_CODE_TIME_LEN);

	years = tmp[0] * 10 + tmp[1];
	months = tmp[2] * 10 + tmp[3];
	days = tmp[4] * 10 + tmp[5];
	hours = tmp[6] * 10 + tmp[7];
	minutes = tmp[8] * 10 + tmp[9];

	NRF_LOG_INFO("code_year = %d, code_months = %d, code_days = %d, code_hours = %d, code_minutes = %d",
							sl_code_date.years, sl_code_date.months, sl_code_date.days, sl_code_date.hours, sl_code_date.minutes);
	NRF_LOG_INFO("tmp_year = %d, tmp_months = %d, tmp_days = %d, tmp_hours = %d, tmp_minutes = %d",
							years, months, days, hours, minutes);

	if (years > sl_code_date.years || months > sl_code_date.months)
	{
		return -1;
	}

	if (days > sl_code_date.days)
	{
		return -1;
	}

	if (hours > sl_code_date.hours)
	{
		return -1;
	}

	if (minutes > sl_code_date.minutes + 1)
	{
		return -1;
	}

	return 0;
}

int smart_lock_compare_dev_id(uint8_t *buf)
{
	int i;

	if (sl_info.dev_lock)
	{
		for (i = 0; i < SL_DEV_ID_LEN; i++)
		{
			NRF_LOG_INFO("[%d]: tmp dev id = %x, buf = %x", i, tmp_dev_id[i], buf[3 + i]);
			if (tmp_dev_id[i] != buf[3 + i])
				break;
		}

		if (i != SL_DEV_ID_LEN)
			return -1;
	}
	else
	{
		for (i = 0; i < SL_DEV_ID_LEN; i++)
		{
			tmp_dev_id[i] = buf[3 + i];
			NRF_LOG_INFO("store tmp_dev_id = %x", tmp_dev_id[i]);
		}
	}
	return 0;
}

void smart_lock_clear_code(void)
{
	NRF_LOG_INFO("clear codes");

	sl_info.code_valid = 0;
	//sl_info.dev_lock = false;

	for (int i = 0; i <= SL_CODE_NUM; i++)
	{
		sl_info.access_code[i] = 0xFF;
	}
}

void smart_lock_read_status(void)
{
	int ret = smart_lock_read_locker();

	if (smart_lock_read_locker() == LOCKER_CONNECT)
		sl_info.lock_status = SL_LOCK;
	else
		sl_info.lock_status = SL_UNLOCK;
}

void smart_lock_parse_data(uint8_t *buf, int len)
{
	int i = 0, sl_len = 0, user_code = -1;
	uint8_t cmd = 0x0;

	cmd = buf[0];
	sl_len = buf[1];

	NRF_LOG_INFO("CMD = 0x%x, st_len = %d, len = %d", cmd, sl_len, len);

	ble_lock_handle = true;

	switch (cmd)
	{
		case SL_UNLOCK_CMD:
//			if (sl_info.update == false)
//			{
//				NRF_LOG_INFO("SmartLocker is not updated yet");
//				uarts_ble_send_data(SL_DEV_NEED_UPDATE);
//				goto out;
//			}

			user_code = buf[2];
		  NRF_LOG_INFO("User access code = %x", user_code);
      NRF_LOG_INFO("valid code left = %d", sl_info.code_valid);

			if (sl_info.code_valid <= 1)
			{
				NRF_LOG_INFO("Lock needs new codes");
				uarts_ble_send_data(SL_CODE_RUN_OUT);
				goto out;
			}

			if (smart_lock_compare_dev_id(buf) != 0)
			{
				NRF_LOG_INFO("Dev ID not match");
				uarts_ble_send_data(SL_DEV_ID_FAIL);
				goto out;
			}

      if (smart_lock_compare_date(buf) != 0)
			{
				NRF_LOG_INFO("Code is outdated");
				uarts_ble_send_data(SL_CODE_OUT_OF_DATE);
				goto out;
			}

			for (i = 0; i < SL_CODE_NUM; i++)
		  {
				if (user_code == sl_info.access_code[i])
				{
					sl_info.code_valid--;
					sl_info.access_code[i] = 0xFF;
					sl_info.dev_lock = true;
					smart_lock_unlocked();
					smart_lock_active_beep(true);
					nrf_delay_ms(200);
					smart_lock_active_beep(false);
					uarts_ble_send_data(SL_UNLOCK_SUCCESS);
					goto out;
				}
			}

			NRF_LOG_INFO("Access code is not matched");
			uarts_ble_send_data(SL_UNLOCK_FAIL);
			break;
		case SL_UPDATE_CODE_CMD:
			smart_lock_clear_code();
			smart_lock_add_info(buf, len, sl_len);
			NRF_LOG_INFO("Update access codes successfully!");
		  uarts_ble_send_data(SL_UPDATE_SUCCESS);
		  break;
		case SL_LOCK_CMD:
			smart_lock_locked();

		  if (smart_lock_read_locker() == LOCKER_CONNECT)
			{
				NRF_LOG_INFO("Lock successful");
				smart_lock_active_beep(false);
				nrf_delay_ms(100);
				smart_lock_active_beep(true);
				nrf_delay_ms(200);
				smart_lock_active_beep(false);
				sl_info.lock_status = SL_LOCK;
				uarts_ble_send_data(SL_LOCK_SUCCESS);
			}
			else
			{
				sl_info.lock_status = SL_UNUSABLE;
			}
		  break;
		case SL_APP_READY_CMD:
			break;
		case SL_RESET_CMD:
			smart_lock_init();
		  uarts_ble_send_data(SL_RESET_SUCCESS);
			break;
		default:
      NRF_LOG_INFO("Unknown CMD !");
		  break;
	}

out:
	ble_lock_handle = false;
}

void smart_lock_detect(void)
{
	int actual_lock;

	if (ble_lock_handle)
		return;

	nrf_delay_ms(100);

	actual_lock = smart_lock_read_locker();

	switch (sl_info.lock_status)
	{
		case SL_UNLOCK:
			break;
		case SL_LOCK:
			break;
		case SL_UNUSABLE:
			if (actual_lock == LOCKER_CONNECT)
			{
				NRF_LOG_INFO("Lock successful");
				sl_info.alarm_cnt = 0;
				smart_lock_active_beep(false);
				nrf_delay_ms(100);
				smart_lock_active_beep(true);
				nrf_delay_ms(200);
				smart_lock_active_beep(false);
				sl_info.lock_status = SL_LOCK;
				uarts_ble_send_data(SL_LOCK_SUCCESS);
			}
			else
			{
				NRF_LOG_INFO("Lock failed");
				sl_info.lock_status = SL_UNUSABLE;
				uarts_ble_send_data(SL_LOCK_FAIL);

				if (sl_info.alarm_cnt > 20)
				{
					smart_lock_active_beep(true);
					return;
				}
				sl_info.alarm_cnt += 1;
			}
			break;
	}
}

void smart_lock_init(void)
{
	memset(&sl_info, 0x0, sizeof(sl_info));
	memset(&sl_code_date, 0x0, sizeof(sl_code_date));

	sl_info.lock_status = SL_LOCK;
	sl_info.lock_connect = LOCKER_CONNECT;
	sl_info.code_valid = 0;
	sl_info.alarm_cnt = 0;
	sl_info.update = false;
	sl_info.beep = false;
	sl_info.dev_lock = false;

	for (int i = 0; i <= SL_CODE_NUM; i++)
	{
		sl_info.access_code[i] = 0xFF;
	}

	//nrf_gpio_cfg_output(LED_PIN);
	//nrf_gpio_pin_set(LED_PIN);

	nrf_gpio_cfg_output(BEEP_PIN);
	nrf_gpio_pin_clear(BEEP_PIN);

	nrf_gpio_cfg_output(RELAY_PIN);
	nrf_gpio_pin_clear(RELAY_PIN);

	nrf_gpio_cfg_input(LOCKER_PIN, NRF_GPIO_PIN_PULLUP);

	if (smart_lock_read_locker() == LOCKER_CONNECT)
		sl_info.lock_status = SL_LOCK;
	else
		sl_info.lock_status = SL_UNLOCK;

	NRF_LOG_INFO("Init Lock Status = %x", sl_info.lock_status);
}
