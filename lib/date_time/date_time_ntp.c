/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/sntp.h>
#include <net/socketutils.h>
#if defined(CONFIG_LTE_LINK_CONTROL)
#include <modem/lte_lc.h>
#endif

#include <logging/log.h>

LOG_MODULE_DECLARE(date_time, CONFIG_DATE_TIME_LOG_LEVEL);

#define UIO_IP      "ntp.uio.no"
#define GOOGLE_IP_1 "time1.google.com"
#define GOOGLE_IP_2 "time2.google.com"
#define GOOGLE_IP_3 "time3.google.com"
#define GOOGLE_IP_4 "time4.google.com"

#define NTP_PORT "123"

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

static int sntp_time_request(struct ntp_servers *server, uint32_t timeout,
			     struct sntp_time *time)
{
	int err;
	static struct addrinfo hints;
	struct addrinfo *addrinfo;
	struct sntp_ctx sntp_ctx;

	if (server->addrlen == 0) {
		if (IS_ENABLED(CONFIG_DATE_TIME_IPV6)) {
			hints.ai_family = AF_INET6;
		} else {
			hints.ai_family = AF_INET;
		}
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = 0;

		err = getaddrinfo(server->server_str, NTP_PORT, &hints,
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

int date_time_ntp_get(int64_t *date_time_ms)
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
		*date_time_ms = (int64_t)sntp_time.seconds * 1000;
		return 0;
	}

	LOG_WRN("Did not get time from any NTP server");

	return -ENODATA;
}
