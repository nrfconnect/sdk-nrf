/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <zephyr/net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <net/lwm2m_client_utils_location.h>
#include <lwm2m_engine.h>

#include <modem/lte_lc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lwm2m_lte_notification, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);
static void lte_notify_handler(const struct lte_lc_evt *const evt);

#if defined(CONFIG_LWM2M_CLIENT_UTILS_NEIGHBOUR_CELL_LISTENER)
#define MAX_INSTANCE_COUNT CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_INSTANCE_COUNT

static K_SEM_DEFINE(rrc_idle, 0, 1);
static bool measurement_scheduled;

void lwm2m_ncell_schedule_measurement(void)
{
	if (measurement_scheduled) {
		LOG_WRN("Measurement already scheduled, waiting for RRC idle");
		return;
	}

	if (k_sem_take(&rrc_idle, K_NO_WAIT) == -EBUSY) {
		LOG_INF("RRC connected, measure when idle");
		measurement_scheduled = true;
		return;
	}

	lte_lc_neighbor_cell_measurement(NULL);
	measurement_scheduled = false;
	k_sem_give(&rrc_idle);
}
#endif /* CONFIG_LWM2M_CLIENT_UTILS_NEIGHBOUR_CELL_LISTENER */

int lwm2m_ncell_handler_register(void)
{
	LOG_INF("Registering ncell notification handler");
	lte_lc_register_handler(lte_notify_handler);
	return 0;
}

void lte_notify_handler(const struct lte_lc_evt *const evt)
{
	int err;
	struct lte_lc_psm_cfg psm_cfg = {0};

	switch (evt->type) {
#if defined(CONFIG_LWM2M_CLIENT_UTILS_NEIGHBOUR_CELL_LISTENER)
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS: {
		err = lwm2m_update_signal_meas_objects(&evt->cells_info);

		if (err == -ENODATA) {
			LOG_DBG("No neighboring cells available");
		} else if (err) {
			LOG_ERR("lwm2m_update_signal_meas_objects, error: %d", err);
		}
	};
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		if (evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED) {
			k_sem_reset(&rrc_idle);
		} else if (evt->rrc_mode == LTE_LC_RRC_MODE_IDLE) {
			if (measurement_scheduled) {
				lte_lc_neighbor_cell_measurement(NULL);
				measurement_scheduled = false;
			}
			k_sem_give(&rrc_idle);
		}
		break;
#endif /* CONFIG_LWM2M_CLIENT_UTILS_NEIGHBOUR_CELL_LISTENER */

#if defined(CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS)
	case LTE_LC_EVT_TAU_PRE_WARNING:
		if (!lwm2m_rd_client_ctx()) {
			LOG_DBG("No lwm2m context");
			return;
		}
		LOG_INF("TAU pre-warning notification. Triggering LwM2M RD client update");
		lwm2m_rd_client_update();
		break;
#endif /* CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS */
	case LTE_LC_EVT_PSM_UPDATE:
		err = lte_lc_psm_get(&psm_cfg.tau, &psm_cfg.active_time);

		if (err) {
			LOG_WRN("Failed to get PSM information");
			return;
		}
		LOG_DBG("Tau period %d seconds", psm_cfg.tau);
		LOG_DBG("Active time %d seconds", psm_cfg.active_time);
		break;
	default:
		break;
	}
}
