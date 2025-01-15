/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/at_cmd_custom.h>
#include <modem/lte_lc.h>

#include "slm_at_notif.h"
#include "slm_util.h"

LOG_MODULE_REGISTER(slm_notif, CONFIG_SLM_LOG_LEVEL);

bool slm_fwd_cgev_notif;

/* We need to receive CGEV notifications at all times.
 * CGEREP AT commands are intercepted to prevent the user
 * from unsubcribing us and make that behavior invisible.
 */
AT_CMD_CUSTOM(at_cgerep_interceptor, "AT+CGEREP", at_cgerep_callback);

static int at_cgerep_callback(char *buf, size_t len, char *at_cmd)
{
	int ret;
	unsigned int subscribe;
	const bool set_cmd = (sscanf(at_cmd, "AT+CGEREP=%u", &subscribe) == 1);

	/* The modem interprets AT+CGEREP and AT+CGEREP= as AT+CGEREP=0.
	 * Prevent those forms, only allowing AT+CGEREP=0, for simplicty.
	 */
	if (!set_cmd && (!strcmp(at_cmd, "AT+CGEREP") || !strcmp(at_cmd, "AT+CGEREP="))) {
		LOG_ERR("The syntax %s is disallowed. Use AT+CGEREP=0 instead.", at_cmd);
		return -EINVAL;
	}
	if (!set_cmd || subscribe) {
		/* Forward the command to the modem only if not unsubscribing. */
		ret = slm_util_at_cmd_no_intercept(buf, len, at_cmd);
		if (ret) {
			return ret;
		}
		/* Modify the output of the read command to reflect the user's
		 * subscription status, not that of the SLM.
		 */
		if (at_cmd[strlen("AT+CGEREP")] == '?') {
			const size_t mode_idx = strlen("+CGEREP: ");

			if (mode_idx < len) {
				/* +CGEREP: <mode>,<bfr> */
				buf[mode_idx] = '0' + slm_fwd_cgev_notif;
			}
		}
	} else { /* AT+CGEREP=0 */
		snprintf(buf, len, "%s", "OK\r\n");
	}

	if (set_cmd) {
		slm_fwd_cgev_notif = subscribe;
	}
	return 0;
}

static void subscribe_cgev_notifications(void)
{
	char buf[sizeof("\r\nOK")];

	/* Bypass the CGEREP interception above as it is meant for commands received externally. */
	const int ret = slm_util_at_cmd_no_intercept(buf, sizeof(buf), "AT+CGEREP=1");

	if (ret) {
		LOG_ERR("Failed to subscribe to +CGEV notifications (%d).", ret);
	}
}

/* Notification subscriptions are reset on CFUN=0.
 * We intercept CFUN set commands to automatically subscribe.
 */
AT_CMD_CUSTOM(at_cfun_set_interceptor, "AT+CFUN=", at_cfun_set_callback);

static int at_cfun_set_callback(char *buf, size_t len, char *at_cmd)
{
	unsigned int mode;
	const int ret = slm_util_at_cmd_no_intercept(buf, len, at_cmd);

	/* sscanf() doesn't match if this is a test command (it also gets intercepted). */
	if (ret || sscanf(at_cmd, "AT+CFUN=%u", &mode) != 1) {
		/* The functional mode cannot have changed. */
		return ret;
	}

	if (mode == LTE_LC_FUNC_MODE_NORMAL || mode == LTE_LC_FUNC_MODE_ACTIVATE_LTE) {
		subscribe_cgev_notifications();
	} else if (mode == LTE_LC_FUNC_MODE_POWER_OFF) {
		/* Unsubscribe the user as would normally happen. */
		slm_fwd_cgev_notif = false;
	}
	return 0;
}
