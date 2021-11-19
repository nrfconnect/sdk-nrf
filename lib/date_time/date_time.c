/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <date_time.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <device.h>
#include <stdio.h>
#include <posix/time.h>

#if defined(CONFIG_DATE_TIME_MODEM)
#include <nrf_modem_at.h>
#if defined(CONFIG_DATE_TIME_AUTO_UPDATE)
#include <modem/at_monitor.h>
#endif
#endif
#if defined(CONFIG_LTE_LINK_CONTROL)
#include <modem/lte_lc.h>
#endif
#if defined(CONFIG_DATE_TIME_NTP)
#include <net/sntp.h>
#include <net/socketutils.h>
#endif

#include <errno.h>
#include <string.h>
#include <sys/timeutil.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(date_time, CONFIG_DATE_TIME_LOG_LEVEL);

#if defined(CONFIG_DATE_TIME_MODEM)
#if defined(CONFIG_DATE_TIME_AUTO_UPDATE)
/* AT monitor for %XTIME notifications */
AT_MONITOR(xtime, "%XTIME", date_time_at_xtime_handler);

/* Indicates whether modem network time has been received with XTIME notification. */
static bool modem_valid_network_time;
#else
static bool modem_valid_network_time = true;
#endif /* defined(CONFIG_DATE_TIME_AUTO_UPDATE) */
#endif /* defined(CONFIG_DATE_TIME_MODEM) */

#if defined(CONFIG_DATE_TIME_NTP)
#define UIO_IP      "ntp.uio.no"
#define GOOGLE_IP_1 "time1.google.com"
#define GOOGLE_IP_2 "time2.google.com"
#define GOOGLE_IP_3 "time3.google.com"
#define GOOGLE_IP_4 "time4.google.com"

#define NTP_DEFAULT_PORT "123"

struct ntp_servers {
	const char *server_str;
	struct sockaddr addr;
	socklen_t addrlen;
};

struct ntp_servers servers[] = {
	{.server_str = UIO_IP},
	{.server_str = GOOGLE_IP_1},
	{.server_str = GOOGLE_IP_2},
	{.server_str = GOOGLE_IP_3},
	{.server_str = GOOGLE_IP_4}
};

static struct sntp_time sntp_time;
#endif

K_SEM_DEFINE(time_fetch_sem, 0, 1);

static void date_time_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(time_work, date_time_handler);

static struct time_aux {
	int64_t date_time_utc;
	int64_t last_date_time_update;
} time_aux;

static bool initial_valid_time;
static date_time_evt_handler_t app_evt_handler;

static struct date_time_evt evt;

static void date_time_notify_event(const struct date_time_evt *evt)
{
	__ASSERT(evt != NULL, "Library event not found");

	if (app_evt_handler != NULL) {
		app_evt_handler(evt);
	}
}

static void date_time_schedule_update(void)
{
	/* If periodic updates are requested in the configuration
	 * and next update hasn't been already scheduled.
	 */
	if (CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS > 0 &&
	    !k_work_delayable_is_pending(&time_work)) {

		LOG_DBG("New date time update in: %d seconds",
			CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS);

		k_work_schedule(&time_work, K_SECONDS(CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS));
	}
}

static int current_time_check(void)
{
	if (time_aux.last_date_time_update == 0 ||
	    time_aux.date_time_utc == 0) {
		LOG_DBG("Date time never set");
		return -ENODATA;
	}

	if ((k_uptime_get() - time_aux.last_date_time_update) >=
	    CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS * MSEC_PER_SEC) {
		LOG_DBG("Current date time too old");
		return -ENODATA;
	}

	return 0;
}

static void date_time_store(int64_t curr_time_ms)
{
	struct timespec tp = { 0 };
	struct tm ltm = { 0 };
	int ret;

	tp.tv_sec = curr_time_ms / 1000;
	tp.tv_nsec = (curr_time_ms % 1000) * 1000000;

	ret = clock_settime(CLOCK_REALTIME, &tp);
	if (ret != 0) {
		LOG_ERR("Could not set system time, %d", ret);
		return;
	}
	gmtime_r(&tp.tv_sec, &ltm);
	LOG_DBG("System time updated: %04u-%02u-%02u %02u:%02u:%02u",
		ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
		ltm.tm_hour, ltm.tm_min, ltm.tm_sec);

#if defined(CONFIG_DATE_TIME_MODEM)
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

		/* Time zone is not known and it's mandatory so setting to zero.
		 * POSIX year is relative to 1900 which doesn't affect as last two digits are taken
		 * with modulo 100.
		 * POSIX month is in range 0-11 so adding 1.
		 */
		ret = nrf_modem_at_printf("AT+CCLK=\"%02u/%02u/%02u,%02u:%02u:%02u+%02u\"",
			ltm.tm_year % 100, ltm.tm_mon + 1, ltm.tm_mday,
			ltm.tm_hour, ltm.tm_min, ltm.tm_sec, 0);
		if (ret) {
			LOG_ERR("Setting modem time failed, %d", ret);
			return;
		}

		LOG_DBG("Modem time updated");
	}
#endif /* defined(CONFIG_DATE_TIME_MODEM) */
}

#if defined(CONFIG_DATE_TIME_MODEM)
static int time_modem_get(void)
{
	int rc;
	struct tm date_time;

	if (!modem_valid_network_time) {
		/* We will get here whenever we haven't received XTIME notification meaning
		 * that we don't want to query modem time when it has been set with AT+CCLK.
		 */
		LOG_DBG("Modem has not got time from LTE network, so not using it");
		return -ENODATA;
	}

	/* Example of modem time response:
	 * "+CCLK: \"20/02/25,17:15:02+04\"\r\n\000{"
	 */
	rc = nrf_modem_at_scanf("AT+CCLK?",
		"+CCLK: \"%u/%u/%u,%u:%u:%u",
		&date_time.tm_year,
		&date_time.tm_mon,
		&date_time.tm_mday,
		&date_time.tm_hour,
		&date_time.tm_min,
		&date_time.tm_sec
	);

	/* Want to match 6 args */
	if (rc != 6) {
		LOG_WRN("Did not get time from cellular network (error: %d). "
			"This is normal as some cellular networks don't provide it or "
			"time may not be available yet.", rc);
		return -ENODATA;
	}

	/* Relative to 1900, as per POSIX */
	date_time.tm_year = date_time.tm_year + 2000 - 1900;
	/* Range is 0-11, as per POSIX */
	date_time.tm_mon = date_time.tm_mon - 1;

	time_aux.date_time_utc = (int64_t)timeutil_timegm64(&date_time) * 1000;
	time_aux.last_date_time_update = k_uptime_get();

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
	uint8_t time_buf[6];
	size_t time_buf_len;
	char *time_str_start;
	int err;

	if (notif == NULL) {
		return;
	}
	modem_valid_network_time = true;

	/* Check if current time is valid */
	err = current_time_check();
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
	time_buf_len = hex2bin(time_str_start, 12, time_buf, sizeof(time_buf));

	if (time_buf_len < sizeof(time_buf)) {
		LOG_ERR("%%XTIME notification decoding failed (ret=%d): %s", time_buf_len, notif);
	}

	date_time.tm_year = semioctet_to_dec(time_buf[0]);
	date_time.tm_mon  = semioctet_to_dec(time_buf[1]);
	date_time.tm_mday = semioctet_to_dec(time_buf[2]);
	date_time.tm_hour = semioctet_to_dec(time_buf[3]);
	date_time.tm_min  = semioctet_to_dec(time_buf[4]);
	date_time.tm_sec  = semioctet_to_dec(time_buf[5]);

	/* Relative to 1900, as per POSIX */
	date_time.tm_year = date_time.tm_year + 2000 - 1900;
	/* Range is 0-11, as per POSIX */
	date_time.tm_mon = date_time.tm_mon - 1;

	time_aux.date_time_utc = (int64_t)timeutil_timegm64(&date_time) * 1000;
	time_aux.last_date_time_update = k_uptime_get();

	LOG_DBG("Time obtained from cellular network (XTIME notification)");

	initial_valid_time = true;
	evt.type = DATE_TIME_OBTAINED_MODEM;
	date_time_store(time_aux.date_time_utc);
	date_time_schedule_update();
	date_time_notify_event(&evt);
}
#endif /* defined(CONFIG_DATE_TIME_AUTO_UPDATE) */
#endif /* defined(CONFIG_DATE_TIME_MODEM) */

#if defined(CONFIG_DATE_TIME_AUTO_UPDATE) && defined(CONFIG_LTE_LINK_CONTROL)

void date_time_lte_ind_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:

		switch (evt->nw_reg_status) {
		case LTE_LC_NW_REG_REGISTERED_EMERGENCY:
		case LTE_LC_NW_REG_REGISTERED_HOME:
		case LTE_LC_NW_REG_REGISTERED_ROAMING:
			if (!initial_valid_time && !k_work_delayable_is_pending(&time_work)) {
				k_work_schedule(&time_work, K_SECONDS(1));
			}
			break;
#if defined(CONFIG_DATE_TIME_MODEM)
		case LTE_LC_NW_REG_SEARCHING: {
			/* Subscribe to modem time notifications */
			int err = nrf_modem_at_printf("AT%%XTIME=1");

			if (err) {
				LOG_ERR("Subscribing to modem AT%%XTIME notifications failed, "
					"err=%d", err);
			}
			break;
		}
#endif
		default:
			break;
		}
		break;
	default:
		break;
	}
}
#endif /* defined(CONFIG_DATE_TIME_AUTO_UPDATE) && defined(CONFIG_LTE_LINK_CONTROL) */

#if defined(CONFIG_DATE_TIME_NTP)
static int sntp_time_request(struct ntp_servers *server, uint32_t timeout,
			     struct sntp_time *time)
{
	int err;
	static struct addrinfo hints;
	struct addrinfo *addrinfo;
	struct sntp_ctx sntp_ctx;

	if (IS_ENABLED(CONFIG_DATE_TIME_IPV6)) {
		hints.ai_family = AF_INET6;
	} else {
		hints.ai_family = AF_INET;
	}
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;

	if (server->addrlen == 0) {
		err = getaddrinfo(server->server_str, NTP_DEFAULT_PORT, &hints,
				  &addrinfo);
		if (err) {
			LOG_WRN("getaddrinfo, error: %d", err);
			return err;
		}

		if (addrinfo->ai_addrlen > sizeof(server->addr)) {
			LOG_WRN("getaddrinfo, addrlen: %d > %d",
				addrinfo->ai_addrlen, sizeof(server->addr));
			freeaddrinfo(addrinfo);
			return -ENOMEM;
		}

		memcpy(&server->addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
		server->addrlen = addrinfo->ai_addrlen;
		freeaddrinfo(addrinfo);
	} else {
		LOG_DBG("Server address already obtained, skipping DNS lookup");
	}

	err = sntp_init(&sntp_ctx, &server->addr, server->addrlen);
	if (err) {
		LOG_WRN("sntp_init, error: %d", err);
		goto socket_close;
	}

	err = sntp_query(&sntp_ctx, timeout, time);
	if (err) {
		LOG_WRN("sntp_query, error: %d", err);
	}

socket_close:
	sntp_close(&sntp_ctx);
	return err;
}

#if defined(CONFIG_LTE_LINK_CONTROL)
static bool is_connected_to_lte(void)
{
	int err;
	enum lte_lc_nw_reg_status reg_status;

	err = lte_lc_nw_reg_status_get(&reg_status);
	if (err) {
		LOG_WRN("Failed getting LTE network registration status, error: %d", err);
		return false;
	}

	if (reg_status == LTE_LC_NW_REG_REGISTERED_EMERGENCY ||
	    reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
	    reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING) {
		return true;
	}

	return false;
}
#endif /* defined(CONFIG_LTE_LINK_CONTROL) */

static int time_NTP_server_get(void)
{
	int err;

#if defined(CONFIG_LTE_LINK_CONTROL)
	if (!is_connected_to_lte()) {
		LOG_DBG("Not connected to LTE, skipping NTP UTC time update");
		return -ENODATA;
	}

	LOG_DBG("Connected to LTE, performing NTP UTC time update");
#endif

	for (int i = 0; i < ARRAY_SIZE(servers); i++) {
		err =  sntp_time_request(&servers[i],
			MSEC_PER_SEC * CONFIG_DATE_TIME_NTP_QUERY_TIME_SECONDS,
			&sntp_time);
		if (err) {
			LOG_DBG("Did not get time from NTP server %s, error %d",
				log_strdup(servers[i].server_str), err);
			LOG_DBG("Trying another address...");
			continue;
		}
		LOG_DBG("Time obtained from NTP server %s",
			log_strdup(servers[i].server_str));
		time_aux.date_time_utc = (int64_t)sntp_time.seconds * 1000;
		time_aux.last_date_time_update = k_uptime_get();
		return 0;
	}

	LOG_WRN("Did not get time from any NTP server");

	return -ENODATA;
}
#endif

static void new_date_time_get(void)
{
	int err;

	while (true) {
		k_sem_take(&time_fetch_sem, K_FOREVER);

		LOG_DBG("Updating date time UTC...");

		err = current_time_check();
		if (err == 0) {
			LOG_DBG("Using previously obtained time");
			initial_valid_time = true;
			date_time_schedule_update();
			date_time_notify_event(&evt);
			continue;
		}

		LOG_DBG("Current time not valid");

#if defined(CONFIG_DATE_TIME_MODEM)
		LOG_DBG("Getting time from cellular network");

		err = time_modem_get();
		if (err == 0) {
			initial_valid_time = true;
			evt.type = DATE_TIME_OBTAINED_MODEM;
			date_time_store(time_aux.date_time_utc);
			date_time_schedule_update();
			date_time_notify_event(&evt);
			continue;
		}
#endif
#if defined(CONFIG_DATE_TIME_NTP)
		LOG_DBG("Getting time from NTP server");

		err = time_NTP_server_get();
		if (err == 0) {
			initial_valid_time = true;
			evt.type = DATE_TIME_OBTAINED_NTP;
			date_time_store(time_aux.date_time_utc);
			date_time_schedule_update();
			date_time_notify_event(&evt);
			continue;
		}
#endif
		LOG_DBG("Did not get time from any time source");

		evt.type = DATE_TIME_NOT_OBTAINED;
		date_time_schedule_update();
		date_time_notify_event(&evt);
	}
}

K_THREAD_DEFINE(time_thread, CONFIG_DATE_TIME_THREAD_STACK_SIZE,
		new_date_time_get, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

static void date_time_handler(struct k_work *work)
{
	k_sem_give(&time_fetch_sem);
}

static int date_time_init(const struct device *unused)
{
#if (defined(CONFIG_DATE_TIME_MODEM) && \
	defined(CONFIG_DATE_TIME_AUTO_UPDATE) && \
	!defined(CONFIG_AT_MONITOR_SYS_INIT))
	at_monitor_init();
#endif
#if defined(CONFIG_DATE_TIME_AUTO_UPDATE) && defined(CONFIG_LTE_LINK_CONTROL)
	lte_lc_register_handler(date_time_lte_ind_handler);
#endif

#if !defined(CONFIG_DATE_TIME_AUTO_UPDATE)
	if (CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS > 0) {
		k_work_schedule(&time_work, K_SECONDS(CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS));
	}
#endif
	return 0;
}

int date_time_set(const struct tm *new_date_time)
{
	int err = 0;

	if (new_date_time == NULL) {
		LOG_ERR("The passed in pointer cannot be NULL");
		return -EINVAL;
	}

	/** Seconds after the minute. tm_sec is generally 0-59.
	 *  The extra range is to accommodate for leap seconds
	 *  in certain systems.
	 */
	if (new_date_time->tm_sec < 0 || new_date_time->tm_sec > 61) {
		LOG_ERR("Seconds in time structure not in correct format");
		err = -EINVAL;
	}

	/** Minutes after the hour. */
	if (new_date_time->tm_min < 0 || new_date_time->tm_min > 59) {
		LOG_ERR("Minutes in time structure not in correct format");
		err = -EINVAL;
	}

	/** Hours since midnight. */
	if (new_date_time->tm_hour < 0 || new_date_time->tm_hour > 23) {
		LOG_ERR("Hours in time structure not in correct format");
		err = -EINVAL;
	}

	/** Day of the month. */
	if (new_date_time->tm_mday < 1 || new_date_time->tm_mday > 31) {
		LOG_ERR("Day in time structure not in correct format");
		err = -EINVAL;
	}

	/** Months since January. */
	if (new_date_time->tm_mon < 0 || new_date_time->tm_mon > 11) {
		LOG_ERR("Month in time structure not in correct format");
		err = -EINVAL;
	}

	/** Years since 1900. 115 corresponds to the year 2015. */
	if (new_date_time->tm_year < 115 || new_date_time->tm_year > 1900) {
		LOG_ERR("Year in time structure not in correct format");
		err = -EINVAL;
	}

	if (err) {
		return err;
	}

	initial_valid_time = true;
	time_aux.last_date_time_update = k_uptime_get();
	time_aux.date_time_utc = (int64_t)timeutil_timegm64(new_date_time) * 1000;

	date_time_store(time_aux.date_time_utc);
	evt.type = DATE_TIME_OBTAINED_EXT;
	date_time_notify_event(&evt);

	return 0;
}

int date_time_uptime_to_unix_time_ms(int64_t *uptime)
{
	int64_t uptime_prev;

	if (uptime == NULL) {
		LOG_ERR("The passed in pointer cannot be NULL");
		return -EINVAL;
	}

	if (*uptime < 0) {
		LOG_ERR("Uptime cannot be negative");
		return -EINVAL;
	}

	uptime_prev = *uptime;

	if (!initial_valid_time) {
		LOG_WRN("Valid time not currently available");
		return -ENODATA;
	}

	*uptime += time_aux.date_time_utc - time_aux.last_date_time_update;

	/* Check if the passed in uptime was already converted,
	 * meaning that after a second conversion it is greater than the
	 * current date time UTC.
	 */
	if (*uptime > time_aux.date_time_utc +
	    (k_uptime_get() - time_aux.last_date_time_update)) {
		LOG_WRN("Uptime too large or previously converted");
		LOG_WRN("Clear variable or set a new uptime");
		*uptime = uptime_prev;
		return -EINVAL;
	}

	return 0;
}

int date_time_now(int64_t *unix_time_ms)
{
	int err;
	int64_t unix_time_ms_prev;

	if (unix_time_ms == NULL) {
		LOG_ERR("The passed in pointer cannot be NULL");
		return -EINVAL;
	}

	unix_time_ms_prev = *unix_time_ms;

	*unix_time_ms = k_uptime_get();

	err = date_time_uptime_to_unix_time_ms(unix_time_ms);
	if (err) {
		LOG_WRN("date_time_uptime_to_unix_time_ms, error: %d", err);
		*unix_time_ms = unix_time_ms_prev;
	}

	return err;
}

bool date_time_is_valid(void)
{
	return initial_valid_time;
}

void date_time_register_handler(date_time_evt_handler_t evt_handler)
{
	if (evt_handler == NULL) {
		app_evt_handler = NULL;

		LOG_DBG("Previously registered handler %p de-registered",
			app_evt_handler);

		return;
	}

	LOG_DBG("Registering handler %p", evt_handler);

	app_evt_handler = evt_handler;
}

int date_time_update_async(date_time_evt_handler_t evt_handler)
{
	if (evt_handler) {
		app_evt_handler = evt_handler;
	} else if (app_evt_handler == NULL) {
		LOG_DBG("No handler registered");
	}

	k_sem_give(&time_fetch_sem);

	return 0;
}

int date_time_clear(void)
{
	time_aux.date_time_utc = 0;
	time_aux.last_date_time_update = 0;
	initial_valid_time = false;

	return 0;
}

int date_time_timestamp_clear(int64_t *unix_timestamp)
{
	if (unix_timestamp == NULL) {
		LOG_ERR("The passed in pointer cannot be NULL");
		return -EINVAL;
	}

	*unix_timestamp = 0;

	return 0;
}

SYS_INIT(date_time_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
