/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <modem/at_monitor.h>
#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>
#include <modem/at_parser.h>
#include <nrf_modem_at.h>

#include "common/event_handler_list.h"
#include "modules/cscon.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* CSCON command parameters */
#define AT_CSCON_RRC_MODE_INDEX	     1
#define AT_CSCON_READ_RRC_MODE_INDEX 2

/* Enable CSCON (RRC mode) notifications */
static const char cscon[] = "AT+CSCON=1";

AT_MONITOR(ltelc_atmon_cscon, "+CSCON", at_handler_cscon);

/**@brief Parses an AT command response, and returns the current RRC mode.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param mode Pointer to where the RRC mode is stored.
 * @param mode_index Parameter index for mode.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
#if defined(CONFIG_UNITY)
int parse_rrc_mode(const char *at_response, enum lte_lc_rrc_mode *mode, size_t mode_index)
#else
static int parse_rrc_mode(const char *at_response, enum lte_lc_rrc_mode *mode, size_t mode_index)
#endif /* CONFIG_UNITY */
{
	int err, temp_mode;
	struct at_parser parser;

	__ASSERT_NO_MSG(at_response != NULL);
	__ASSERT_NO_MSG(mode != NULL);

	err = at_parser_init(&parser, at_response);
	__ASSERT_NO_MSG(err == 0);

	/* Get the RRC mode from the response */
	err = at_parser_num_get(&parser, mode_index, &temp_mode);
	if (err) {
		LOG_ERR("Could not get signalling mode, error: %d", err);
		goto clean_exit;
	}

	/* Check if the parsed value maps to a valid registration status */
	if (temp_mode == 0) {
		*mode = LTE_LC_RRC_MODE_IDLE;
	} else if (temp_mode == 1) {
		*mode = LTE_LC_RRC_MODE_CONNECTED;
	} else {
		LOG_ERR("Invalid signalling mode: %d", temp_mode);
		err = -EINVAL;
	}

clean_exit:
	return err;
}

static void at_handler_cscon(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("+CSCON notification");

	err = parse_rrc_mode(response, &evt.rrc_mode, AT_CSCON_RRC_MODE_INDEX);
	if (err) {
		LOG_ERR("Can't parse signalling mode, error: %d", err);
		return;
	}

	if (evt.rrc_mode == LTE_LC_RRC_MODE_IDLE) {
		LTE_LC_TRACE(LTE_LC_TRACE_RRC_IDLE);
	} else if (evt.rrc_mode == LTE_LC_RRC_MODE_CONNECTED) {
		LTE_LC_TRACE(LTE_LC_TRACE_RRC_CONNECTED);
	}

	evt.type = LTE_LC_EVT_RRC_UPDATE;

	event_handler_list_dispatch(&evt);
}

int cscon_notifications_enable(void)
{
	int err;

	err = nrf_modem_at_printf(cscon);
	if (err) {
		LOG_WRN("Failed to enable RRC notifications (+CSCON), error %d", err);
		return -EFAULT;
	}

	return 0;
}
