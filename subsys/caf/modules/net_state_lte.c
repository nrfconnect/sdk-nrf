/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <modem/lte_lc.h>

#define MODULE net_state
#include <caf/events/module_state_event.h>
#include <caf/events/power_manager_event.h>
#include <caf/events/net_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_NET_STATE_LTE_LOG_LEVEL);

static struct k_work_delayable connecting_work;
static enum net_state net_state;
static bool registered;
static uint32_t cell_id = UINT32_MAX;

#if IS_ENABLED(CONFIG_CAF_LOG_NET_STATE_WAITING)
#define STATE_WAITING_PERIOD CONFIG_CAF_LOG_NET_STATE_WAITING_PERIOD
#else
#define STATE_WAITING_PERIOD 0
#endif


static void connecting_work_handler(struct k_work *work)
{
	LOG_INF("Waiting for LTE connection...");
	k_work_reschedule(&connecting_work, K_SECONDS(STATE_WAITING_PERIOD));
}

static void send_net_state_event(enum net_state state)
{
	struct net_state_event *event = new_net_state_event();

	event->id = MODULE_ID(MODULE);
	event->state = state;
	APP_EVENT_SUBMIT(event);
}

static void set_net_state(enum net_state state)
{
	if (state != net_state) {
		net_state = state;
		send_net_state_event(net_state);
	}
}

static void update_net_state(void)
{
	if (registered && (cell_id != UINT32_MAX)) {
		set_net_state(NET_STATE_CONNECTED);
		if (IS_ENABLED(CONFIG_CAF_POWER_MANAGER_EVENTS)) {
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_ALIVE);
		}
	} else {
		set_net_state(NET_STATE_DISCONNECTED);
		if (IS_ENABLED(CONFIG_CAF_POWER_MANAGER_EVENTS)) {
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_MAX);
		}
	}
}

static void lte_lc_evt_nw_reg_status_handler(enum lte_lc_nw_reg_status evt)
{
	bool roaming = false;

	switch (evt) {
	case LTE_LC_NW_REG_UICC_FAIL:
	case LTE_LC_NW_REG_UNKNOWN:
		LOG_WRN("LTE offline");
		registered = false;
		break;

	case LTE_LC_NW_REG_REGISTERED_ROAMING:
		roaming = true;
		/* Fall-through */

	case LTE_LC_NW_REG_REGISTERED_HOME:
		LOG_INF("LTE registered %s", (roaming)?("roaming"):("home"));
		registered = true;
		break;

	case LTE_LC_NW_REG_REGISTRATION_DENIED:
		LOG_WRN("LTE registration denied");
		registered = false;
		break;

	default:
		break;
	}

	update_net_state();
}

static void lte_lc_evt_edrx_update_handler(const struct lte_lc_edrx_cfg *edrx_cfg)
{
	char log_buf[60];
	int err;

	err = snprintf(log_buf, sizeof(log_buf),
		       "eDRX parameter update: eDRX: %0.2f, PTW: %0.2f",
		       edrx_cfg->edrx, edrx_cfg->ptw);
	if ((err > 0) && ((size_t)err < sizeof(log_buf))) {
		LOG_INF("%s", log_buf);
	}
}

static void lte_lc_evt_handler(const struct lte_lc_evt * const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		lte_lc_evt_nw_reg_status_handler(evt->nw_reg_status);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_INF("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE:
		lte_lc_evt_edrx_update_handler(&evt->edrx_cfg);
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_INF("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		cell_id = evt->cell.id;
		update_net_state();
		break;
	default:
		break;
	}
}

static int connect_lte(void)
{
	int err = lte_lc_init();

	if (err) {
		LOG_ERR("Failed to initialize LTE");
		return err;
	}

	err = lte_lc_connect_async(lte_lc_evt_handler);
	if (err) {
		LOG_ERR("LTE connection request failed");
		return err;
	}

	LOG_INF("LTE connection requested");

	return 0;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (IS_ENABLED(CONFIG_CAF_LOG_NET_STATE_WAITING)) {
				k_work_init_delayable(&connecting_work, connecting_work_handler);
			}

			update_net_state();

			if (connect_lte()) {
				LOG_ERR("Cannot initialize");
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_LOG_NET_STATE_WAITING) && is_net_state_event(aeh)) {
		if (net_state == NET_STATE_DISCONNECTED) {
			k_work_reschedule(&connecting_work, K_NO_WAIT);
		} else {
			/* Cancel cannot fail if executed from another work's context. */
			(void)k_work_cancel_delayable(&connecting_work);
		}

		return false;
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_CAF_LOG_NET_STATE_WAITING
	APP_EVENT_SUBSCRIBE(MODULE, net_state_event);
#endif
