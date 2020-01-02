/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <nrf9160_timestamp.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <at_cmd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <net/sntp.h>
#include <net/socketutils.h>
#include <sys/timeutil.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(nrf9160_timestamp, CONFIG_NRF9160_TIMESTAMP_LOG_LEVEL);

#define AT_CMD_MODEM_DATE_TIME			"AT+CCLK?"
#define AT_CMD_MODEM_DATE_TIME_RESPONSE_LEN	28

#define UIO_IP      "129.240.2.6"
#define GOOGLE_IP   "216.239.35.0"
#define GOOGLE_IP_2 "216.239.35.4"
#define GOOGLE_IP_3 "216.239.35.8"
#define GOOGLE_IP_4 "216.239.35.12"

K_SEM_DEFINE(fetch_time_sem, 0, 1);

static struct k_delayed_work time_work;

static struct time_aux {
	s64_t date_time_utc;
	int last_date_time_update;
} time_aux;

static struct sntp_time sntp_time;
static bool initial_valid_time;

struct ntp_servers {
	const char *server;
};

struct ntp_servers servers[] = {
	{.server = UIO_IP},
	{.server = GOOGLE_IP},
	{.server = GOOGLE_IP_2},
	{.server = GOOGLE_IP_3},
	{.server = GOOGLE_IP_4}
};

#if defined(CONFIG_NRF9160_TIMESTAMP_TIME_FROM_MODEM)
static int parse_time_entries(char *datetime_string, int min, int max)
{
	char buf[50];

	for (int i = min; i < max + 1; i++) {
		buf[i - min] = datetime_string[i];
	}

	return strtol(buf, NULL, 10);
}

static int get_time_cellular_network(void)
{
	int err;
	char buf[AT_CMD_MODEM_DATE_TIME_RESPONSE_LEN + 5];
	struct tm date_time;

	err = at_cmd_write(AT_CMD_MODEM_DATE_TIME, buf, sizeof(buf), NULL);
	if (err) {
		LOG_DBG("Could not get cellular network time, error: %d", err);
		return err;
	}
	buf[AT_CMD_MODEM_DATE_TIME_RESPONSE_LEN] = 0;

	LOG_DBG("Response from modem: %s", log_strdup(buf));

	date_time.tm_year = parse_time_entries(buf, 8, 9) + 2000 - 1900;
	date_time.tm_mon  = parse_time_entries(buf, 11, 12) - 1;
	date_time.tm_mday = parse_time_entries(buf, 14, 15);
	date_time.tm_hour = parse_time_entries(buf, 17, 18);
	date_time.tm_min  = parse_time_entries(buf, 20, 21);
	date_time.tm_sec  = parse_time_entries(buf, 23, 24);

	if (date_time.tm_year == 115 || date_time.tm_year == 0) {
		LOG_DBG("Modem giving old cellular network time");
		return -ENODATA;
	}

	time_aux.date_time_utc =
		(s64_t)timeutil_timegm64(&date_time) * 1000 - k_uptime_get();
	time_aux.last_date_time_update = k_uptime_get();

	return 0;
}
#endif

static int sntp_request_time(const char *server, u32_t timeout,
			     struct sntp_time *time)
{
	int err;
	static struct addrinfo hints;
	struct addrinfo *addr;
	struct sntp_ctx sntp_ctx;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;

	err = net_getaddrinfo_addr_str(server, "123", &hints, &addr);
	if (err) {
		LOG_ERR("net_getaddrinfo_addr_str, error: %d", err);
		return err;
	}

	err = sntp_init(&sntp_ctx, addr->ai_addr, addr->ai_addrlen);
	if (err) {
		goto freeaddr;
	}

	err = sntp_query(&sntp_ctx, timeout, time);
	if (err) {
		sntp_close(&sntp_ctx);
		goto freeaddr;
	}

freeaddr:
	freeaddrinfo(addr);
	return err;
}

static int get_time_NTP_server(void)
{
	int err;
	int i = 0;

	while (i < ARRAY_SIZE(servers)) {
		err =  sntp_request_time(servers[i].server,
					 K_SECONDS(5), &sntp_time);
		if (err) {
			LOG_DBG("Not getting time from NTP server %s, error %d",
				log_strdup(servers[i].server), err);
			LOG_DBG("Trying another address...");
			i++;
			continue;
		}

		LOG_DBG("Got time response from NTP server %s",
			log_strdup(servers[i].server));
		time_aux.date_time_utc =
			(s64_t)sntp_time.seconds * 1000 - k_uptime_get();
		time_aux.last_date_time_update = k_uptime_get();
		return 0;
	}

	LOG_ERR("Not getting time from any NTP server");
	return -ENODATA;
}

static int check_current_time(void)
{
	int age_last_time_update =
		k_uptime_get() - time_aux.last_date_time_update;
	int current_update_time_interval_ms =
		CONFIG_NRF9160_TIMESTAMP_TIME_UPDATE_INTERVAL * 1000;

	if (time_aux.last_date_time_update == 0 ||
	    time_aux.date_time_utc == 0) {
		LOG_DBG("Date time never set");
		return -ENODATA;
	}

	if (age_last_time_update > current_update_time_interval_ms) {
		LOG_DBG("Current date time too old");
		return -ENODATA;
	}

	return 0;
}

static void update_new_date_time(void)
{
	int err;

	while (true) {
		k_sem_take(&fetch_time_sem, K_FOREVER);

		LOG_DBG("Updating date time UTC...");

		err = check_current_time();
		if (err == 0) {
			LOG_DBG("Time successfully obtained");
			initial_valid_time = true;
			continue;
		}

		LOG_DBG("Current time not valid");

#if defined(CONFIG_NRF9160_TIMESTAMP_TIME_FROM_MODEM)
		LOG_DBG("Fallback on cellular network time");

		err = get_time_cellular_network();
		if (err == 0) {
			LOG_DBG("Time from cellular network obtained");
			initial_valid_time = true;
			continue;
		}

		LOG_DBG("Not getting cellular network time");
#endif

		LOG_DBG("Fallback on NTP server");

		err = get_time_NTP_server();
		if (err == 0) {
			LOG_DBG("Time from NTP server obtained");
			initial_valid_time = true;
			continue;
		}

		LOG_DBG("Not getting time from NTP server");
	}
}

K_THREAD_DEFINE(time_thread, CONFIG_NRF9160_TIMESTAMP_NTP_THREAD_SIZE,
		update_new_date_time, NULL, NULL, NULL,
		K_HIGHEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

static void nrf9160_time_handler(struct k_work *work)
{
	k_sem_give(&fetch_time_sem);

	LOG_DBG("New date time update in: %d seconds",
		CONFIG_NRF9160_TIMESTAMP_TIME_UPDATE_INTERVAL);

	k_delayed_work_submit(
		&time_work,
		K_SECONDS(CONFIG_NRF9160_TIMESTAMP_TIME_UPDATE_INTERVAL));
}

void nrf9160_timestamp_init(void)
{
	k_delayed_work_init(&time_work, nrf9160_time_handler);
	k_delayed_work_submit(&time_work, K_NO_WAIT);
}

void date_time_set(const struct tm *new_date_time)
{
	time_aux.last_date_time_update = k_uptime_get();
	time_aux.date_time_utc =
		(s64_t)timeutil_timegm64(new_date_time) * 1000 - k_uptime_get();
}

int date_time_get(s64_t *unix_timestamp_ms)
{
	if (!initial_valid_time || unix_timestamp_ms == 0) {
		k_sem_give(&fetch_time_sem);
		return -ENODATA;
	}

	*unix_timestamp_ms += time_aux.date_time_utc;

	return 0;
}
