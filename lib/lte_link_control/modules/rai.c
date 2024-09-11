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
#include <modem/at_monitor.h>
#include <modem/at_parser.h>
#include <nrf_modem_at.h>
#include <nrf_errno.h>

#include "common/event_handler_list.h"
#include "common/helpers.h"
#include "modules/rai.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* RAI notification parameters */
#define AT_RAI_RESPONSE_PREFIX	"%RAI"
#define AT_RAI_PARAMS_COUNT_MAX 5
#define AT_RAI_CELL_ID_INDEX	1
#define AT_RAI_PLMN_INDEX	2
#define AT_RAI_AS_INDEX		3
#define AT_RAI_CP_INDEX		4

AT_MONITOR(ltelc_atmon_rai, "%RAI", at_handler_rai);

static int parse_rai(const char *at_response, struct lte_lc_rai_cfg *rai_cfg)
{
	struct at_parser parser;
	int err;
	int tmp_int;

	err = at_parser_init(&parser, at_response);
	__ASSERT_NO_MSG(err == 0);

	/* Cell Id */
	err = string_param_to_int(&parser, AT_RAI_CELL_ID_INDEX, &tmp_int, 16);
	if (err) {
		LOG_ERR("Could not parse cell_id, error: %d", err);
		goto clean_exit;
	}

	if (tmp_int > LTE_LC_CELL_EUTRAN_ID_MAX) {
		LOG_ERR("Invalid cell ID: %d", tmp_int);
		err = -EBADMSG;
		goto clean_exit;
	}
	rai_cfg->cell_id = tmp_int;

	/* PLMN, that is, MCC and MNC */
	err = plmn_param_string_to_mcc_mnc(&parser, AT_RAI_PLMN_INDEX, &rai_cfg->mcc,
					   &rai_cfg->mnc);
	if (err) {
		goto clean_exit;
	}

	/* AS RAI configuration */
	err = at_parser_num_get(&parser, AT_RAI_AS_INDEX, &tmp_int);
	if (err) {
		LOG_ERR("Could not get AS RAI, error: %d", err);
		goto clean_exit;
	}
	rai_cfg->as_rai = tmp_int;

	/* CP RAI configuration */
	err = at_parser_num_get(&parser, AT_RAI_CP_INDEX, &tmp_int);
	if (err) {
		LOG_ERR("Could not get CP RAI, error: %d", err);
		goto clean_exit;
	}
	rai_cfg->cp_rai = tmp_int;

clean_exit:
	return err;
}

static void at_handler_rai(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("%%RAI notification");

	err = parse_rai(response, &evt.rai_cfg);
	if (err) {
		LOG_ERR("Can't parse RAI notification, error: %d", err);
		return;
	}

	evt.type = LTE_LC_EVT_RAI_UPDATE;

	event_handler_list_dispatch(&evt);
}

int rai_set(void)
{
	int err;

	if (IS_ENABLED(CONFIG_LTE_RAI_REQ)) {
		LOG_DBG("Enabling RAI with notifications");
	} else {
		LOG_DBG("Disabling RAI");
	}

	err = nrf_modem_at_printf("AT%%RAI=%d", IS_ENABLED(CONFIG_LTE_RAI_REQ) ? 2 : 0);
	if (err) {
		if (IS_ENABLED(CONFIG_LTE_RAI_REQ)) {
			LOG_DBG("Failed to enable RAI with notifications so trying without them");
			/* If AT%RAI=2 failed, modem might not support it so using older API */
			err = nrf_modem_at_printf("AT%%RAI=1");
		}
		if (err) {
			LOG_ERR("Failed to configure RAI, err %d", err);
			return -EFAULT;
		}
	}

	return err;
}
