/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <date_time.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <device.h>
#include <stdio.h>
#include <stdlib.h>
#include <modem/at_cmd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <net/sntp.h>
#include <net/socketutils.h>
#include <sys/timeutil.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(date_time, CONFIG_DATE_TIME_LOG_LEVEL);

#define AT_CMD_MODEM_DATE_TIME			"AT+CCLK?"
#define AT_CMD_MODEM_DATE_TIME_RESPONSE_LEN	32

/* The magic number 115 corresponds to the year 2015 which is the default
 * year given by the modem when the cellular network has not pushed time
 * the modem.
 */
#define MODEM_TIME_DEFAULT 115

#define UIO_IP      "ntp.uio.no"
#define GOOGLE_IP_1 "time1.google.com"
#define GOOGLE_IP_2 "time2.google.com"
#define GOOGLE_IP_3 "time3.google.com"
#define GOOGLE_IP_4 "time4.google.com"

#define NTP_DEFAULT_PORT "123"

K_SEM_DEFINE(time_fetch_sem, 0, 1);

static struct k_delayed_work time_work;

static struct time_aux {
	s64_t date_time_utc;
	int last_date_time_update;
} time_aux;

static struct sntp_time sntp_time;
static bool initial_valid_time;

struct ntp_servers {
	const char *server_str;
	struct addrinfo *addr;
};

struct ntp_servers servers[] = {
	{.server_str = UIO_IP},
	{.server_str = GOOGLE_IP_1},
	{.server_str = GOOGLE_IP_2},
	{.server_str = GOOGLE_IP_3},
	{.server_str = GOOGLE_IP_4}
};

#if defined(CONFIG_DATE_TIME_MODEM)
static int time_modem_get(void)
{
	int err;
	char buf[AT_CMD_MODEM_DATE_TIME_RESPONSE_LEN + 1];
	struct tm date_time;

	err = at_cmd_write(AT_CMD_MODEM_DATE_TIME, buf, sizeof(buf), NULL);
	if (err) {
		LOG_DBG("Could not get cellular network time, error: %d", err);
		return err;
	}
	/* This line zero indexes the buffer at the desired length
	 * This ensures clean printing of the modem response.
	 */
	buf[AT_CMD_MODEM_DATE_TIME_RESPONSE_LEN - 4] = '\0';

	/* Example of modem time response:
	 * "+CCLK: \"20/02/25,17:15:02+04\"\r\n\000{"
	 */
	LOG_DBG("Response from modem: %s", log_strdup(buf));

	/* Replace '/' ',' and ':' with whitespace for easier parsing by strtol.
	 * strtol skips over whitespace.
	 */
	for (int i = 0; i < AT_CMD_MODEM_DATE_TIME_RESPONSE_LEN; i++) {
		if (buf[i] == '/' || buf[i] == ',' || buf[i] == ':') {
			buf[i] = ' ';
		}
	}

	/* The year starts at index 8. */
	char *ptr_index = &buf[8];
	int base = 10;

	date_time.tm_year = strtol(ptr_index, &ptr_index, base) + 2000 - 1900;
	date_time.tm_mon = strtol(ptr_index, &ptr_index, base) - 1;
	date_time.tm_mday = strtol(ptr_index, &ptr_index, base);
	date_time.tm_hour = strtol(ptr_index, &ptr_index, base);
	date_time.tm_min = strtol(ptr_index, &ptr_index, base);
	date_time.tm_sec = strtol(ptr_index, &ptr_index, base);

	if (date_time.tm_year == MODEM_TIME_DEFAULT) {
		LOG_DBG("Modem time never set");
		return -ENODATA;
	}

	time_aux.date_time_utc = (s64_t)timeutil_timegm64(&date_time) * 1000;
	time_aux.last_date_time_update = k_uptime_get();

	return 0;
}
#endif

static int sntp_time_request(struct ntp_servers *server, u32_t timeout,
			     struct sntp_time *time)
{
	int err;
	static struct addrinfo hints;
	struct sntp_ctx sntp_ctx;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;

	if (server->addr == NULL) {
		err = getaddrinfo(server->server_str, NTP_DEFAULT_PORT, &hints,
				  &server->addr);
		if (err) {
			LOG_ERR("getaddrinfo, error: %d", err);
			return err;
		}
	} else {
		LOG_DBG("Server address already obtained, skipping DNS lookup");
	}

	err = sntp_init(&sntp_ctx, server->addr->ai_addr,
			server->addr->ai_addrlen);
	if (err) {
		LOG_ERR("sntp_init, error: %d", err);
		goto socket_close;
	}

	err = sntp_query(&sntp_ctx, timeout, time);
	if (err) {
		LOG_ERR("sntp_query, error: %d", err);
	}

socket_close:
	sntp_close(&sntp_ctx);
	return err;
}

static int time_NTP_server_get(void)
{
	int err;

	for (int i = 0; i < ARRAY_SIZE(servers); i++) {
		err =  sntp_time_request(&servers[i],
			K_SECONDS(CONFIG_DATE_TIME_NTP_QUERY_TIME_SECONDS),
			&sntp_time);
		if (err) {
			LOG_DBG("Not getting time from NTP server %s, error %d",
				log_strdup(servers[i].server_str), err);
			LOG_DBG("Trying another address...");
			continue;
		}

		LOG_DBG("Got time response from NTP server %s",
			log_strdup(servers[i].server_str));
		time_aux.date_time_utc = (s64_t)sntp_time.seconds * 1000;
		time_aux.last_date_time_update = k_uptime_get();
		return 0;
	}

	LOG_ERR("Not getting time from any NTP server");

	return -ENODATA;
}

static int current_time_check(void)
{
	if (time_aux.last_date_time_update == 0 ||
	    time_aux.date_time_utc == 0) {
		LOG_DBG("Date time never set");
		return -ENODATA;
	}

	if ((k_uptime_get() - time_aux.last_date_time_update) >
	    CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS * 1000) {
		LOG_DBG("Current date time too old");
		return -ENODATA;
	}

	return 0;
}

static void new_date_time_get(void)
{
	int err;

	while (true) {
		k_sem_take(&time_fetch_sem, K_FOREVER);

		LOG_DBG("Updating date time UTC...");

		err = current_time_check();
		if (err == 0) {
			LOG_DBG("Time successfully obtained");
			initial_valid_time = true;
			continue;
		}

		LOG_DBG("Current time not valid");

#if defined(CONFIG_DATE_TIME_MODEM)
		LOG_DBG("Fallback on cellular network time");

		err = time_modem_get();
		if (err == 0) {
			LOG_DBG("Time from cellular network obtained");
			initial_valid_time = true;
			continue;
		}

		LOG_DBG("Not getting cellular network time");
#endif
		LOG_DBG("Fallback on NTP server");

		err = time_NTP_server_get();
		if (err == 0) {
			LOG_DBG("Time from NTP server obtained");
			initial_valid_time = true;
			continue;
		}

		LOG_DBG("Not getting time from NTP server");
	}
}

K_THREAD_DEFINE(time_thread, CONFIG_DATE_TIME_THREAD_SIZE,
		new_date_time_get, NULL, NULL, NULL,
		K_HIGHEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

static void date_time_handler(struct k_work *work)
{
	k_sem_give(&time_fetch_sem);

	LOG_DBG("New date time update in: %d seconds",
		CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS);

	k_delayed_work_submit(&time_work,
			K_SECONDS(CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS));
}

static int date_time_init(struct device *unused)
{
	k_delayed_work_init(&time_work, date_time_handler);
	k_delayed_work_submit(&time_work,
			K_SECONDS(CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS));

	return 0;
}

void date_time_set(const struct tm *new_date_time)
{
	time_aux.last_date_time_update = k_uptime_get();
	time_aux.date_time_utc = (s64_t)timeutil_timegm64(new_date_time) * 1000;
}

int date_time_uptime_to_unix_time_ms(s64_t *uptime)
{
	if (!initial_valid_time) {
		LOG_ERR("Valid time not currently available, requesting time");
		k_sem_give(&time_fetch_sem);
		return -ENODATA;
	}

	*uptime += time_aux.date_time_utc - time_aux.last_date_time_update;

	return 0;
}

int date_time_now(s64_t *unix_time_ms)
{
	*unix_time_ms = k_uptime_get();

	return date_time_uptime_to_unix_time_ms(unix_time_ms);
}

void date_time_update(void)
{
	k_sem_give(&time_fetch_sem);
}

DEVICE_INIT(date_time, "DATE_TIME",
	    date_time_init, NULL, NULL, APPLICATION,
	    CONFIG_APPLICATION_INIT_PRIORITY);
