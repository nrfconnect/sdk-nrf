/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <date_time.h>
#include <zephyr/kernel.h>
#include <zephyr/posix/time.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/logging/log.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#if defined(CONFIG_DATE_TIME_AUTO_UPDATE)
#include <modem/at_monitor.h>
#endif

#include "date_time_core.h"

LOG_MODULE_DECLARE(date_time, CONFIG_DATE_TIME_LOG_LEVEL);

extern struct k_work_q date_time_work_q;

#if defined(CONFIG_DATE_TIME_AUTO_UPDATE)
/* AT monitor for %XTIME notifications */
AT_MONITOR(xtime, "%XTIME", date_time_at_xtime_handler);

void date_time_modem_xtime_subscribe_work_fn(struct k_work *work_item);
K_WORK_DEFINE(date_time_modem_xtime_subscribe_work, date_time_modem_xtime_subscribe_work_fn);

/* Indicates whether modem network time has been received with XTIME notification. */
static bool modem_valid_network_time;
#else
/* As we won't be getting XTIME notifications, we'll fake that modem time is available so that
 * we won't block requesting of modem time.
 */
static bool modem_valid_network_time = true;
#endif /* defined(CONFIG_DATE_TIME_AUTO_UPDATE) */

int date_time_modem_get(int64_t *date_time_ms, int *date_time_tz)
{
	int rc;
	struct tm date_time;
	int tz = DATE_TIME_TZ_INVALID;

#if defined(CONFIG_DATE_TIME_NTP)
	if (!modem_valid_network_time) {
		/* We will get here whenever we haven't received XTIME notification meaning
		 * that we don't want to query modem time when it has been set with AT+CCLK.
		 * However, if we don't have other time sources (NTP in practice),
		 * then we are OK to check modem time as modem clock is more accurate than
		 * application side clock.
		 */
		LOG_DBG("Modem has not got time from LTE network, so not using it");
		return -ENODATA;
	}
#endif
	/* Example of modem time response:
	 * "+CCLK: \"20/02/25,17:15:02+04\"\r\n\000{"
	 */
	rc = nrf_modem_at_scanf("AT+CCLK?",
		"+CCLK: \"%u/%u/%u,%u:%u:%u%d",
		&date_time.tm_year,
		&date_time.tm_mon,
		&date_time.tm_mday,
		&date_time.tm_hour,
		&date_time.tm_min,
		&date_time.tm_sec,
		&tz
	);

	/* Want to match 6 or 7 args */
	if (rc != 6 && rc != 7) {
		LOG_WRN("Did not get time from cellular network (error: %d). "
			"This is normal as some cellular networks don't provide it or "
			"time may not be available yet.", rc);
		return -ENODATA;
	}

	/* Relative to 1900, as per POSIX */
	date_time.tm_year = date_time.tm_year + 2000 - 1900;
	/* Range is 0-11, as per POSIX */
	date_time.tm_mon = date_time.tm_mon - 1;

	*date_time_ms = (int64_t)timeutil_timegm64(&date_time) * 1000;
	*date_time_tz = tz;

	LOG_DBG("Time obtained from cellular network");

	return 0;
}

#if defined(CONFIG_DATE_TIME_AUTO_UPDATE)
/**
 * @brief Converts an octet having two semi-octets into a decimal.
 *
 * @details Semi-octet representation is explained in 3GPP TS 23.040 Section 9.1.2.3.
 * An octet has semi-octets in the following order:
 *   semi-octet-digit2, semi-octet-digit1
 * Octet for decimal number '21' is hence represented as semi-octet bits:
 *   00010010
 *
 * @param[in] value Octet to be converted.
 *
 * @return Decimal value.
 */
static uint8_t semioctet_to_dec(uint8_t value)
{
	/* 4 LSBs represent decimal that should be multiplied by 10. */
	/* 4 MSBs represent decimal that should be added as is. */
	return ((value & 0xf0) >> 4) + ((value & 0x0f) * 10);
}

static void date_time_at_xtime_handler(const char *notif)
{
	struct tm date_time;
	int64_t date_time_ms;
	uint8_t time_buf[7];
	size_t time_buf_len;
	char *time_str_start;
	int err;
	int tz;

	if (notif == NULL) {
		return;
	}
	modem_valid_network_time = true;

	/* Check if current time is valid */
	err = date_time_core_current_check();
	if (err == 0) {
		LOG_DBG("Previously obtained time is not too old. Discarding XTIME notification.");
		return;
	}

	/* %XTIME: [<local_time_zone>],[<universal_time>],[<daylight_saving_time>]"
	 * <unversal_time> string in hex format, seven bytes long optional field for
	 *                 universal time as specified in 3GPP TS 24.008 ch 10.5.3.9
	 *                 and received from network.
	 * Examples of modem time response:
	 * %XTIME: "08","81109251714208","01"
	 * %XTIME: ,"81109251714208",
	 */
	time_str_start = strchr(notif, ',');
	if (time_str_start == NULL) {
		LOG_ERR("%%XTIME notification doesn't contain ',': %s", notif);
		return;
	}
	if (strlen(time_str_start) < 17) {
		LOG_ERR("%%XTIME notification too short: %s", notif);
		return;
	}
	if (*(time_str_start + 1) != '"') {
		LOG_ERR("%%XTIME notification doesn't contain '\"' after ',': %s", notif);
		return;
	}

	time_str_start += 2;
	time_buf_len = hex2bin(time_str_start, 14, time_buf, sizeof(time_buf));

	if (time_buf_len < sizeof(time_buf)) {
		LOG_ERR("%%XTIME notification decoding failed (ret=%d): %s", time_buf_len, notif);
		return;
	}

	date_time.tm_year = semioctet_to_dec(time_buf[0]);
	date_time.tm_mon  = semioctet_to_dec(time_buf[1]);
	date_time.tm_mday = semioctet_to_dec(time_buf[2]);
	date_time.tm_hour = semioctet_to_dec(time_buf[3]);
	date_time.tm_min  = semioctet_to_dec(time_buf[4]);
	date_time.tm_sec  = semioctet_to_dec(time_buf[5]);

	tz = semioctet_to_dec(time_buf[6] & 0xF7);
	if (time_buf[6] & 0x08) {
		tz = -tz;
	}

	/* Relative to 1900, as per POSIX */
	date_time.tm_year = date_time.tm_year + 2000 - 1900;
	/* Range is 0-11, as per POSIX */
	date_time.tm_mon = date_time.tm_mon - 1;

	date_time_ms = (int64_t)timeutil_timegm64(&date_time) * 1000;

	LOG_DBG("Time obtained from cellular network (XTIME notification)");

	date_time_core_store_tz(date_time_ms, DATE_TIME_OBTAINED_MODEM, tz);
}
#endif /* defined(CONFIG_DATE_TIME_AUTO_UPDATE) */

void date_time_modem_store(struct tm *ltm, int tz)
{
	int ret;

	/* Set modem time if modem has not got it from the LTE network */
	if (!modem_valid_network_time) {
		/* AT+CCLK=<time>
		 * "<time> sring type value; format is ""yy/MM/dd,hh:mm:ss±zz"",
		 *         where characters indicate year (two last digits),
		 *         month, day, hour, minutes, seconds and time zone
		 *         (indicates the difference, expressed in quarters of an hour,
		 *         between the local time and GMT; range -48...+48)."
		 * Example command:
		 *   "AT+CCLK="18/12/06,22:10:00+08"
		 */

		/* POSIX year is relative to 1900 which doesn't affect as last two digits are taken
		 * with modulo 100.
		 * POSIX month is in range 0-11 so adding 1.
		 */
		ret = nrf_modem_at_printf("AT+CCLK=\"%02u/%02u/%02u,%02u:%02u:%02u%+03d\"",
			ltm->tm_year % 100, ltm->tm_mon + 1, ltm->tm_mday,
			ltm->tm_hour, ltm->tm_min, ltm->tm_sec, tz);
		if (ret) {
			LOG_ERR("Setting modem time failed, %d", ret);
			return;
		}

		LOG_DBG("Modem time updated");
	}
}

#if defined(CONFIG_DATE_TIME_AUTO_UPDATE)

void date_time_modem_xtime_subscribe_work_fn(struct k_work *work_item)
{
	/* Subscribe to modem time notifications */
	int err = nrf_modem_at_printf("AT%%XTIME=1");

	if (err) {
		LOG_ERR("Subscribing to modem AT%%XTIME notifications failed, err=%d", err);
	} else {
		LOG_DBG("Subscribing to modem AT%%XTIME notifications succeeded");
	}
}

NRF_MODEM_LIB_ON_CFUN(date_time_cfun_hook, date_time_modem_on_cfun, NULL);

static void date_time_modem_on_cfun(int mode, void *ctx)
{
	ARG_UNUSED(ctx);

	if (mode == LTE_LC_FUNC_MODE_NORMAL || mode == LTE_LC_FUNC_MODE_ACTIVATE_LTE) {
		k_work_submit_to_queue(&date_time_work_q, &date_time_modem_xtime_subscribe_work);
	}
}

#endif /* defined(CONFIG_DATE_TIME_AUTO_UPDATE) */
