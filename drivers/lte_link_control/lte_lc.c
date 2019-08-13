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
#include <at_cmd.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

#define LC_MAX_READ_LENGTH 128
#define AT_CMD_SIZE(x) (sizeof(x) - 1)

/* Subscribes to notifications with level 2 */
static const char subscribe[] = "AT+CEREG=2";

#if defined(CONFIG_LTE_LOCK_BANDS)
/* Lock LTE bands 3, 4, 13 and 20 (volatile setting) */
static const char lock_bands[] = "AT%XBANDLOCK=2,\""CONFIG_LTE_LOCK_BAND_MASK
				 "\"";
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
#if defined(CONFIG_LTE_NETWORK_MODE_NBIOT)
/* Set network mode to Narrowband-IoT */
static const char network_mode[] = "AT%XSYSTEMMODE=0,1,1,0";
#elif defined(CONFIG_LTE_NETWORK_MODE_LTE_M)
/* Set network mode to LTE-M */
static const char network_mode[] = "AT%XSYSTEMMODE=1,0,1,0";
#endif
/* Accepted network statuses read from modem */
static const char   status1[] = "+CEREG: 1";
static const char   status2[] = "+CEREG:1";
static const char   status3[] = "+CEREG: 5";
static const char   status4[] = "+CEREG:5";
static struct k_sem link;

#if defined(CONFIG_LTE_PDP_CMD) && defined(CONFIG_LTE_PDP_CONTEXT)
static const char cgdcont[] = "AT+CGDCONT="CONFIG_LTE_PDP_CONTEXT;
#endif
#if defined(CONFIG_LTE_LEGACY_PCO_MODE)
static const char legacy_pco[] = "AT%XEPCO=0";
#endif

void at_handler(char *response)
{
	LOG_DBG("recv: %s", log_strdup(response));

	if (!memcmp(status1, response, AT_CMD_SIZE(status1)) ||
	    !memcmp(status2, response, AT_CMD_SIZE(status2)) ||
	    !memcmp(status3, response, AT_CMD_SIZE(status3)) ||
	    !memcmp(status4, response, AT_CMD_SIZE(status4))) {
		k_sem_give(&link);
	}
}

static int w_lte_lc_init(void)
{
#if defined(CONFIG_LTE_EDRX_REQ)
	/* Request configured eDRX settings to save power */
	if (at_cmd_write(edrx_req, NULL, 0, NULL) != 0) {
		return -EIO;
	}
#endif
	if (at_cmd_write(subscribe, NULL, 0, NULL) != 0) {
		return -EIO;
	}

#if defined(CONFIG_LTE_LOCK_BANDS)
	/* Set LTE band lock (volatile setting).
	 * Has to be done every time before activating the modem.
	 */
	if (at_cmd_write(lock_bands, NULL, 0, NULL) != 0) {
		return -EIO;
	}
#endif
#if defined(CONFIG_LTE_LEGACY_PCO_MODE)
	if (at_cmd_write(legacy_pco, NULL, 0, NULL) != 0) {
		return -EIO;
	}
	LOG_INF("Using legacy LTE PCO mode...");
#endif
#if defined(CONFIG_LTE_PDP_CMD)
	if (at_cmd_write(cgdcont, NULL, 0, NULL) != 0) {
		return -EIO;
	}
	LOG_INF("PDP Context: %s", cgdcont);
#endif

	return 0;
}

static int w_lte_lc_connect(void)
{
	if (at_cmd_write(network_mode, NULL, 0, NULL) != 0) {
		return -EIO;
	}

	k_sem_init(&link, 0, 1);

	at_cmd_set_notification_handler(at_handler);

	if (at_cmd_write(normal, NULL, 0, NULL) != 0) {
		return -EIO;
	}

	k_sem_take(&link, K_FOREVER);

	at_cmd_set_notification_handler(NULL);

	return 0;
}

static int w_lte_lc_init_and_connect(struct device *unused)
{
	int ret;

	ret = w_lte_lc_init();
	if (ret) {
		return ret;
	}

	return w_lte_lc_connect();
}

/* lte lc Init wrapper */
int lte_lc_init(void)
{
	return w_lte_lc_init();
}

/* lte lc Connect wrapper */
int lte_lc_connect(void)
{
	return w_lte_lc_connect();
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
	if (at_cmd_write(offline, NULL, 0, NULL) != 0) {
		return -EIO;
	}

	return 0;
}

int lte_lc_power_off(void)
{
	if (at_cmd_write(power_off, NULL, 0, NULL) != 0) {
		return -EIO;
	}

	return 0;
}

int lte_lc_normal(void)
{
	if (at_cmd_write(normal, NULL, 0, NULL) != 0) {
		return -EIO;
	}

	return 0;
}

int lte_lc_psm_req(bool enable)
{
	if (at_cmd_write(enable ? psm_req : psm_disable,
			 NULL, 0, NULL) != 0) {
		return -EIO;
	}

	return 0;
}

int lte_lc_edrx_req(bool enable)
{
	if (at_cmd_write(enable ? edrx_req : edrx_disable,
			 NULL, 0, NULL) != 0) {
		return -EIO;
	}

	return 0;
}

#if defined(CONFIG_LTE_AUTO_INIT_AND_CONNECT)
DEVICE_DECLARE(lte_link_control);
DEVICE_AND_API_INIT(lte_link_control, "LTE_LINK_CONTROL",
		    w_lte_lc_init_and_connect, NULL, NULL, APPLICATION,
		    CONFIG_APPLICATION_INIT_PRIORITY, NULL);
#endif /* CONFIG_LTE_AUTO_INIT_AND_CONNECT */
