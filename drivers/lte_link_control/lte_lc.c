/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <errno.h>
#include <net/socket.h>
#include <string.h>
#include <stdio.h>
#include <device.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

#define LC_MAX_READ_LENGTH 128
#define AT_CMD_SIZE(x) (sizeof(x) - 1)

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
/* Accepted network statuses read from modem */
static const char status1[] = "+CEREG: 1";
static const char status2[] = "+CEREG:1";
static const char status3[] = "+CEREG: 5";
static const char status4[] = "+CEREG:5";

#if defined(CONFIG_LTE_PDP_CMD) && defined(CONFIG_LTE_PDP_CONTEXT)
static const char cgdcont[] = "AT+CGDCONT="CONFIG_LTE_PDP_CONTEXT;
#endif
#if defined(CONFIG_LTE_LEGACY_PCO_MODE)
static const char legacy_pco[] = "AT%XEPCO=0";
#endif

static int at_cmd(int fd, const char *cmd, size_t size)
{
	int len;
	u8_t buffer[LC_MAX_READ_LENGTH];

	LOG_DBG("send: %s", cmd);
	len = send(fd, cmd, size, 0);
	if (len != size) {
		LOG_ERR("send: failed");
		return -EIO;
	}

	len = recv(fd, buffer, LC_MAX_READ_LENGTH, 0);
	if ((len < AT_CMD_SIZE(success)) ||
	    (memcmp(success, buffer, AT_CMD_SIZE(success)) != 0)) {
		LOG_ERR("recv: %s", buffer);
		return -EIO;
	}

	return 0;
}

static int w_lte_lc_init_and_connect(struct device *unused)
{
	int err;
	int at_socket_fd;

	u8_t buffer[LC_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}

#if defined(CONFIG_LTE_EDRX_REQ)
	/* Request configured eDRX settings to save power */
	err = at_cmd(at_socket_fd, edrx_req, AT_CMD_SIZE(edrx_req));
	if (err) {
		close(at_socket_fd);
		return err;
	}
#endif
	err = at_cmd(at_socket_fd, subscribe, AT_CMD_SIZE(subscribe));
	if (err) {
		close(at_socket_fd);
		return err;
	}

#if defined(CONFIG_LTE_LOCK_BANDS)
	/* Set LTE band lock (volatile setting).
	 * Has to be done every time before activating the modem.
	 */
	err = at_cmd(at_socket_fd, lock_bands, AT_CMD_SIZE(lock_bands));
	if (err) {
		close(at_socket_fd);
		return err;
	}
#endif
#if defined(CONFIG_LTE_LEGACY_PCO_MODE)
	err = at_cmd(at_socket_fd, legacy_pco, AT_CMD_SIZE(legacy_pco));
	if (err) {
		close(at_socket_fd);
		return err;
	}
	LOG_INF("Using legacy LTE PCO mode...");
#endif
#if defined(CONFIG_LTE_PDP_CMD)
	err = at_cmd(at_socket_fd, cgdcont, AT_CMD_SIZE(cgdcont));
	if (err) {
		close(at_socket_fd);
		return err;
	}
	LOG_INF("PDP Context: %s", cgdcont);
#endif
	err = at_cmd(at_socket_fd, normal, AT_CMD_SIZE(normal));
	if (err) {
		close(at_socket_fd);
		return err;
	}

	while (true) {
		int bytes;

		bytes = recv(at_socket_fd, buffer, LC_MAX_READ_LENGTH, 0);
		if (bytes) {
			LOG_DBG("recv: %s", buffer);
			if (!memcmp(status1, buffer, AT_CMD_SIZE(status1)) ||
			    !memcmp(status2, buffer, AT_CMD_SIZE(status2)) ||
			    !memcmp(status3, buffer, AT_CMD_SIZE(status3)) ||
			    !memcmp(status4, buffer, AT_CMD_SIZE(status4))) {
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
	int err;
	int at_socket_fd;

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}

	err = at_cmd(at_socket_fd, offline, AT_CMD_SIZE(offline));
	close(at_socket_fd);

	return err;
}

int lte_lc_power_off(void)
{
	int err;
	int at_socket_fd;

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}

	err = at_cmd(at_socket_fd, power_off, AT_CMD_SIZE(power_off));
	close(at_socket_fd);

	return err;
}

int lte_lc_normal(void)
{
	int err;
	int at_socket_fd;

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}

	err = at_cmd(at_socket_fd, normal, AT_CMD_SIZE(normal));
	close(at_socket_fd);

	return err;
}

int lte_lc_psm_req(bool enable)
{
	int err;
	int at_socket_fd;
	const char *cmd;
	size_t size;

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}
	if (enable) {
		cmd = psm_req;
		size = AT_CMD_SIZE(psm_req);
	} else {
		cmd = psm_disable;
		size = AT_CMD_SIZE(psm_disable);
	}

	err = at_cmd(at_socket_fd, cmd, size);
	close(at_socket_fd);

	return err;
}

int lte_lc_edrx_req(bool enable)
{
	int err;
	int at_socket_fd;
	const char *cmd;
	size_t size;

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}
	if (enable) {
		cmd = edrx_req;
		size = AT_CMD_SIZE(edrx_req);
	} else {
		cmd = edrx_disable;
		size = AT_CMD_SIZE(edrx_disable);
	}

	err = at_cmd(at_socket_fd, cmd, size);
	close(at_socket_fd);

	return err;
}

#if defined(CONFIG_LTE_AUTO_INIT_AND_CONNECT)
DEVICE_DECLARE(lte_link_control);
DEVICE_AND_API_INIT(lte_link_control, "LTE_LINK_CONTROL",
		    w_lte_lc_init_and_connect, NULL, NULL, APPLICATION,
		    CONFIG_APPLICATION_INIT_PRIORITY, NULL);
#endif /* CONFIG_LTE_AUTO_INIT_AND_CONNECT */
