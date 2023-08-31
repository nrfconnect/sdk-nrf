/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/sntp.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socketutils.h>
#if defined(CONFIG_LTE_LINK_CONTROL)
#include <modem/lte_lc.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(date_time, CONFIG_DATE_TIME_LOG_LEVEL);

#define UIO_NTP    "ntp.uio.no"
#define GOOGLE_NTP "time.google.com"

#define NTP_PORT 123

static const char * const servers[] = {
	UIO_NTP,
	GOOGLE_NTP
};

static struct sntp_time sntp_time;

static int sntp_time_request(const char *server, uint32_t timeout, struct sntp_time *time)
{
	int err;
	struct zsock_addrinfo *addrinfo;
	struct sntp_ctx sntp_ctx;

	struct zsock_addrinfo hints = {
		.ai_flags = AI_NUMERICSERV,
		.ai_family = AF_UNSPEC /* Allow both IPv4 and IPv6 addresses */
	};

	err = zsock_getaddrinfo(server, STRINGIFY(NTP_PORT), &hints, &addrinfo);
	if (err) {
		LOG_WRN("getaddrinfo, error: %d", err);
		return err;
	}

	err = sntp_init(&sntp_ctx, addrinfo->ai_addr, addrinfo->ai_addrlen);
	if (err) {
		LOG_WRN("sntp_init, error: %d", err);
		goto socket_close;
	}

	err = sntp_query(&sntp_ctx, timeout, time);
	if (err) {
		LOG_WRN("sntp_query, error: %d", err);
	}

socket_close:
	zsock_freeaddrinfo(addrinfo);

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

	if (reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
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
		err =  sntp_time_request(servers[i],
			MSEC_PER_SEC * CONFIG_DATE_TIME_NTP_QUERY_TIME_SECONDS,
			&sntp_time);
		if (err) {
			LOG_DBG("Did not get time from NTP server %s, error %d", servers[i], err);
			continue;
		}
		LOG_DBG("Time obtained from NTP server %s", servers[i]);
		*date_time_ms = (int64_t)sntp_time.seconds * 1000;
		return 0;
	}

	LOG_WRN("Did not get time from any NTP server");

	return -ENODATA;
}
