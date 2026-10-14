/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <modem/at_monitor.h>
#include <modem/lte_lc.h>
#include <modem/at_parser.h>
#include <nrf_modem_at.h>

#include "common/event_handler_list.h"
#include "modules/xsim.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* XSIM notification parameters. */
#define AT_XSIM_STATE_INDEX 1
#define AT_XSIM_CAUSE_INDEX 2

/* Subscribe to UICC state notifications. */
static const char xsim[] = "AT%%XSIM=1";

AT_MONITOR(ltelc_atmon_xsim, "%XSIM", at_handler_xsim);

static int parse_xsim(const char *at_response, struct lte_lc_uicc_state_evt *uicc_state)
{
	int err;
	struct at_parser parser;
	uint16_t state;
	uint16_t cause = LTE_LC_UICC_STATE_CAUSE_NONE;
	size_t count = 0;

	__ASSERT_NO_MSG(at_response != NULL);
	__ASSERT_NO_MSG(uicc_state != NULL);

	err = at_parser_init(&parser, at_response);
	__ASSERT_NO_MSG(err == 0);

	err = at_parser_num_get(&parser, AT_XSIM_STATE_INDEX, &state);
	if (err) {
		LOG_ERR("Could not get UICC state, error: %d", err);

		return err;
	}

	if (state != LTE_LC_UICC_STATE_NOT_INITIALIZED && state != LTE_LC_UICC_STATE_INITIALIZED) {
		LOG_ERR("Invalid UICC state: %u", state);

		return -EINVAL;
	}

	err = at_parser_cmd_count_get(&parser, &count);
	if (err) {
		LOG_ERR("Could not get %%XSIM param count, error: %d", err);

		return err;
	}

	/* The cause is only included if it has a non-zero value. */
	if (count == (AT_XSIM_CAUSE_INDEX + 1)) {
		err = at_parser_num_get(&parser, AT_XSIM_CAUSE_INDEX, &cause);
		if (err) {
			LOG_ERR("Could not get UICC state cause, error: %d", err);

			return err;
		}

		if (cause > LTE_LC_UICC_STATE_CAUSE_USIM_PROFILE_CHANGED) {
			LOG_WRN("Unknown UICC state cause: %u", cause);
		}
	}

	uicc_state->state = state;
	uicc_state->cause = cause;

	return 0;
}

static void at_handler_xsim(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("%%XSIM notification");

	err = parse_xsim(response, &evt.uicc_state);
	if (err) {
		LOG_ERR("Can't parse %%XSIM notification, error: %d", err);

		return;
	}

	evt.type = LTE_LC_EVT_UICC_STATE;

	event_handler_list_dispatch(&evt);
}

void xsim_notifications_enable(void)
{
	int err;

	err = nrf_modem_at_printf(xsim);
	if (err) {
		LOG_WRN("Failed to enable UICC state notifications (%%XSIM), error %d, "
			"check if the modem firmware supports it", err);
	}
}
