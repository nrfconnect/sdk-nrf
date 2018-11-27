/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/* TODO A workaround for intrusive behavior of networking subsystem logging.
 *      Upstream issue #11659.
 */
#include <logging/log.h>
#define LOG_LEVEL CONFIG_LTE_LINK_CONTROL_LOG_LEVEL
LOG_MODULE_REGISTER(lte_lc);

#include <zephyr.h>
#include <zephyr/types.h>
#include <errno.h>
#include <net/socket.h>
#include <string.h>
#include <stdio.h>
#include <device.h>

#define LC_MAX_READ_LENGTH 128
#if defined(CONFIG_LTE_AUTO_INIT_AND_CONNECT)
DEVICE_DECLARE(lte_link_control);
#endif

/* Subscribes to notifications with level 2 */
static const char subscribe[] = "AT+CEREG=2";

#if defined(CONFIG_LTE_LOCK_BANDS)
/* Lock LTE bands 3, 4, 13 and 20 (volatile setting) */
static const char lock_bands[] = "AT%XBANDLOCK=2,\"10000001000000001100\"";
#endif
/* Request eDRX settings to be used */
static const char edrx_req[] = "AT+CEDRXS=1,"CONFIG_LTE_EDRX_REQ_ACTT_TYPE
	",\""CONFIG_LTE_EDRX_REQ_VALUE"\"";
/* Request eDRX to be disabled */
static const char edrx_disable[] = "AT+CEDRXS=3";
/* Request modem to go to power saving mode */
static const char psm_req[] = "AT+CPSMS=1,,,\""CONFIG_LTE_PSM_REQ_RPTAU
			      "\",\""CONFIG_LTE_PSM_REQ_RAT"\"";
/* Request PSM to be disabled */
static const char psm_disable[] = "AT+CPSMS=";
/* Set the modem to power off mode */
static const char power_off[] = "AT+CFUN=0";
/* Set the modem to Normal mode */
static const char normal[] = "AT+CFUN=1";
/* Set the modem to Offline mode */
static const char offline[] = "AT+CFUN=4";
/* Successful return from modem */
static const char success[] = "OK";
/* Network status read from modem */
static const char *status[4] = {
	"+CEREG: 1",
	"+CEREG:1",
	"+CEREG: 5",
	"+CEREG:5",
};

static int w_lte_lc_init_and_connect(struct device *unused)
{
	int at_socket_fd;
	int buffer;
	u8_t read_buffer[LC_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}

#if defined(CONFIG_LTE_EDRX_REQ)
	/* Request configured eDRX settings to save power */
	LOG_DBG("send: %s", edrx_req);
	buffer = send(at_socket_fd, edrx_req, strlen(edrx_req), 0);
	if (buffer != strlen(edrx_req)) {
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, sizeof(read_buffer), 0);
	if ((buffer < strlen(success)) ||
	    (memcmp(success, read_buffer, strlen(success)) != 0)) {
		close(at_socket_fd);
		LOG_ERR("recv: %s", read_buffer);
		return -EIO;
	}
#endif
	LOG_DBG("send: %s", subscribe);
	buffer = send(at_socket_fd, subscribe, strlen(subscribe), 0);
	if (buffer != strlen(subscribe)) {
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, sizeof(read_buffer), 0);
	if ((buffer < strlen(success)) ||
	    (memcmp(success, read_buffer, strlen(success)) != 0)) {
		close(at_socket_fd);
		LOG_ERR("recv: %s", read_buffer);
		return -EIO;
	}

#if defined(CONFIG_LTE_LOCK_BANDS)
	/* Set LTE band lock (volatile setting).
	   Has to be done every time before activating the modem. */
	LOG_DBG("send: %s", lock_bands);
	buffer = send(at_socket_fd, lock_bands, strlen(lock_bands), 0);
	if (buffer != strlen(lock_bands)) {
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, sizeof(read_buffer), 0);
	if ((buffer < strlen(success)) ||
	    (memcmp(success, read_buffer, strlen(success)) != 0)) {
		close(at_socket_fd);
		LOG_ERR("recv: %s", read_buffer);
		return -EIO;
	}
#endif

	LOG_DBG("send: %s", normal);
	buffer = send(at_socket_fd, normal, strlen(normal), 0);
	if (buffer != strlen(normal)) {
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, sizeof(read_buffer), 0);
	if ((buffer < strlen(success)) ||
	    (memcmp(success, read_buffer, strlen(success)) != 0)) {
		close(at_socket_fd);
		LOG_ERR("recv: %s", read_buffer);
		return -EIO;
	}

	while (true) {
		buffer = recv(at_socket_fd, read_buffer,
				  sizeof(read_buffer), 0);
		if (buffer) {
			LOG_DBG("recv: %s", read_buffer);
			if (
			(memcmp(status[0], read_buffer, strlen(status[0])) == 0) ||
			(memcmp(status[1], read_buffer, strlen(status[1])) == 0) ||
			(memcmp(status[2], read_buffer, strlen(status[2])) == 0) ||
			(memcmp(status[3], read_buffer, strlen(status[3])) == 0)) {
				break;
			}
		}
	}
	close(at_socket_fd);
	return 0;
}

/* lte lc Init and connect wrapper */
int lte_lc_init_and_connect(void)
{
	struct device *x = 0;

	int err = w_lte_lc_init_and_connect(x);

	return err;
}

int lte_lc_offline(void)
{
	int at_socket_fd;
	int buffer;
	u8_t read_buffer[LC_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}
	LOG_DBG("send: %s", offline);
	buffer = send(at_socket_fd, offline, strlen(offline), 0);
	if (buffer != strlen(offline)) {
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, LC_MAX_READ_LENGTH, 0);
	if ((buffer < strlen(success)) ||
	    (memcmp(success, read_buffer, strlen(success) != 0))) {
		close(at_socket_fd);
		LOG_ERR("recv: %s", read_buffer);
		return -EIO;
	}

	close(at_socket_fd);
	return 0;
}

int lte_lc_power_off(void)
{
	int at_socket_fd;
	int buffer;
	u8_t read_buffer[LC_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}
	LOG_DBG("send: %s", power_off);
	buffer = send(at_socket_fd, power_off, strlen(power_off), 0);
	if (buffer != strlen(power_off)) {
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, LC_MAX_READ_LENGTH, 0);
	if ((buffer < strlen(success)) ||
	    (memcmp(success, read_buffer, strlen(success)) != 0)) {
		close(at_socket_fd);
		LOG_ERR("recv: %s", read_buffer);
		return -EIO;
	}
	close(at_socket_fd);
	return 0;
}

int lte_lc_normal(void)
{
	int at_socket_fd;
	int buffer;
	u8_t read_buffer[LC_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}
	LOG_DBG("send: %s", normal);
	buffer = send(at_socket_fd, normal, strlen(normal), 0);
	if (buffer != strlen(normal)) {
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, LC_MAX_READ_LENGTH, 0);
	if ((buffer < strlen(success)) ||
	    (memcmp(success, read_buffer, strlen(success)) != 0)) {
		close(at_socket_fd);
		LOG_ERR("recv: %s", read_buffer);
		return -EIO;
	}
	close(at_socket_fd);
	return 0;
}

int lte_lc_psm_req(bool enable)
{
	int at_socket_fd;
	int buffer;
	u8_t read_buffer[LC_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}
	if (enable) {
		LOG_DBG("send: %s", psm_req);
		buffer = send(at_socket_fd, psm_req, strlen(psm_req), 0);
		if (buffer != strlen(psm_req)) {
			close(at_socket_fd);
			return -EIO;
		}
	} else {
		LOG_DBG("send: %s", psm_disable);
		buffer = send(at_socket_fd, psm_disable, strlen(psm_disable), 0);
		if (buffer != strlen(psm_disable)) {
			close(at_socket_fd);
			return -EIO;
		}
	}

	buffer = recv(at_socket_fd, read_buffer, LC_MAX_READ_LENGTH, 0);
	if ((buffer < strlen(success)) ||
	    (memcmp(success, read_buffer, strlen(success)) != 0)) {
		close(at_socket_fd);
		LOG_ERR("recv: %s", read_buffer);
		return -EIO;
	}
	close(at_socket_fd);
	return 0;
}

int lte_lc_edrx_req(bool enable)
{
	int at_socket_fd;
	int buffer;
	u8_t read_buffer[LC_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}
	if (enable) {
		LOG_DBG("send: %s", edrx_req);
		buffer = send(at_socket_fd, edrx_req, strlen(edrx_req), 0);
		if (buffer != strlen(edrx_req)) {
			close(at_socket_fd);
			return -EIO;
		}
	} else {
		LOG_DBG("send: %s", edrx_disable);
		buffer = send(at_socket_fd, edrx_disable, strlen(edrx_disable), 0);
		if (buffer != strlen(edrx_disable)) {
			close(at_socket_fd);
			return -EIO;
		}
	}

	buffer = recv(at_socket_fd, read_buffer, sizeof(read_buffer), 0);
	if ((buffer < strlen(success)) ||
	    (memcmp(success, read_buffer, strlen(success)) != 0)) {
		close(at_socket_fd);
		LOG_ERR("recv: %s", read_buffer);
		return -EIO;
	}
	close(at_socket_fd);
	return 0;
}

#if defined(CONFIG_LTE_AUTO_INIT_AND_CONNECT)
DEVICE_AND_API_INIT(lte_link_control, "LTE_LINK_CONTROL",
		    w_lte_lc_init_and_connect, NULL, NULL, APPLICATION,
		    CONFIG_APPLICATION_INIT_PRIORITY, NULL);
#endif /* CONFIG_LTE_AUTO_INIT_AND_CONNECT */
