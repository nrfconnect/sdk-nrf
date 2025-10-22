/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>
#include <modem/at_parser.h>
#include <nrf_modem_at.h>

#include "common/event_handler_list.h"
#include "modules/cereg.h"
#include "modules/cfun.h"
#include "modules/cscon.h"
#include "modules/xmodemsleep.h"
#include "modules/xt3412.h"
#include "modules/mdmev.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

#define AT_CFUN_READ "AT+CFUN?"

static int enable_notifications(void)
{
	int err;

	err = cereg_notifications_enable();
	if (err) {
		return err;
	}

	err = cscon_notifications_enable();
	if (err) {
		return err;
	}

#if defined(CONFIG_LTE_LC_MODEM_SLEEP_MODULE)
	err = xmodemsleep_notifications_enable();
	if (err) {
		return err;
	}
#endif

#if defined(CONFIG_LTE_LC_TAU_PRE_WARNING_MODULE)
	err = xt3412_notifications_enable();
	if (err) {
		return err;
	}
#endif

	if (mdmev_enabled) {
		/* Modem events have been enabled by the application, so the notifications need
		 * to be subscribed to again. This is done using the same function which is used
		 * to enable modem events through the library API.
		 */
		err = mdmev_enable();
		if (err) {
			return err;
		}
	}

	return 0;
}

int cfun_mode_get(enum lte_lc_func_mode *mode)
{
	int err;
	uint16_t mode_tmp;

	if (mode == NULL) {
		return -EINVAL;
	}

	/* Exactly one parameter is expected to match. */
	err = nrf_modem_at_scanf(AT_CFUN_READ, "+CFUN: %hu", &mode_tmp);
	if (err != 1) {
		LOG_ERR("AT command failed, nrf_modem_at_scanf() returned error: %d", err);
		return -EFAULT;
	}

	*mode = mode_tmp;

	return 0;
}

int cfun_mode_set(enum lte_lc_func_mode mode)
{
	int err;

	switch (mode) {
	case LTE_LC_FUNC_MODE_ACTIVATE_LTE:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_ACTIVATE_LTE);

		err = enable_notifications();
		if (err) {
			LOG_ERR("Failed to enable notifications, error: %d", err);
			return -EFAULT;
		}
		break;
	case LTE_LC_FUNC_MODE_NORMAL:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_NORMAL);

		err = enable_notifications();
		if (err) {
			LOG_ERR("Failed to enable notifications, error: %d", err);
			return -EFAULT;
		}
		break;
	case LTE_LC_FUNC_MODE_POWER_OFF:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_POWER_OFF);
		break;
	case LTE_LC_FUNC_MODE_RX_ONLY:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_RX_ONLY);

		err = enable_notifications();
		if (err) {
			LOG_ERR("Failed to enable notifications, error: %d", err);
			return -EFAULT;
		}
		break;
	case LTE_LC_FUNC_MODE_OFFLINE:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_OFFLINE);
		break;
	case LTE_LC_FUNC_MODE_DEACTIVATE_LTE:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_DEACTIVATE_LTE);
		break;
	case LTE_LC_FUNC_MODE_DEACTIVATE_GNSS:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_DEACTIVATE_GNSS);
		break;
	case LTE_LC_FUNC_MODE_ACTIVATE_GNSS:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_ACTIVATE_GNSS);
		break;
	case LTE_LC_FUNC_MODE_DEACTIVATE_UICC:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_DEACTIVATE_UICC);
		break;
	case LTE_LC_FUNC_MODE_ACTIVATE_UICC:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_ACTIVATE_UICC);
		break;
	case LTE_LC_FUNC_MODE_OFFLINE_UICC_ON:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_OFFLINE_UICC_ON);
		break;
	default:
		LOG_ERR("Invalid functional mode: %d", mode);
		return -EINVAL;
	}

	err = nrf_modem_at_printf("AT+CFUN=%d", mode);
	if (err) {
		LOG_ERR("Failed to set functional mode. Please check XSYSTEMMODE.");
		return -EFAULT;
	}

	LOG_DBG("Functional mode set to %d", mode);

	return 0;
}
