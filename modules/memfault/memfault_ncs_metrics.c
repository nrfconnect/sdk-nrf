/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>
#include <memfault/metrics/metrics.h>
#include <memfault/core/platform/overrides.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(memfault_ncs_metrics, CONFIG_MEMFAULT_NCS_LOG_LEVEL);

#ifdef CONFIG_MEMFAULT_NCS_STACK_METRICS
static bool connected;
static bool connect_timer_started;
#endif /* CONFIG_MEMFAULT_NCS_STACK_METRICS */

static void stack_check(const struct k_thread *cthread, void *user_data)
{
#ifdef CONFIG_MEMFAULT_NCS_STACK_METRICS
	struct k_thread *thread = (struct k_thread *)cthread;
	char hexname[11];
	const char *name;
	size_t unused;
	int err;
	enum stack_type {
		STACK_NONE,
		STACK_AT_CMD,
		STACK_CONNECTION_POLL
	} stack_type;

	ARG_UNUSED(user_data);

	name = k_thread_name_get((k_tid_t)thread);
	if (!name || name[0] == '\0') {
		name = hexname;

		snprintk(hexname, sizeof(hexname), "%p", (void *)thread);
		LOG_DBG("No thread name registered for %s", name);
		return;
	}

	if (strncmp("at_cmd_socket_thread", name, sizeof("at_cmd_socket_thread")) == 0) {
		stack_type = STACK_AT_CMD;
	} else if (strncmp("connection_poll_thread", name,
			   sizeof("connection_poll_thread")) == 0) {
		stack_type = STACK_CONNECTION_POLL;
	} else {
		LOG_DBG("Not relevant stack: %s", name);
		return;
	}

	err = k_thread_stack_space_get(thread, &unused);
	if (err) {
		LOG_WRN(" %-20s: unable to get stack space (%d)", name, err);
		return;
	}

	if (stack_type == STACK_AT_CMD) {
		LOG_DBG("Unused at_cmd_socket_thread stack size: %d", unused);

		err = memfault_metrics_heartbeat_set_unsigned(
			MEMFAULT_METRICS_KEY(Ncs_AtCmdUnusedStack), unused);
		if (err) {
			LOG_ERR("Failed to set Ncs_AtCmdUnusedStack");
		}
	} else if (stack_type == STACK_CONNECTION_POLL) {
		LOG_DBG("Unused connection_poll_thread stack size: %d", unused);

		err = memfault_metrics_heartbeat_set_unsigned(
			MEMFAULT_METRICS_KEY(Ncs_ConnectionPollUnusedStack), unused);
		if (err) {
			LOG_ERR("Failed to set Ncs_ConnectionPollUnusedStack");
		}
	}
#endif /* CONFIG_MEMFAULT_NCS_STACK_METRICS */
}

static void lte_trace_cb(enum lte_lc_trace_type type)
{
#ifdef CONFIG_MEMFAULT_NCS_LTE_METRICS
	int err;

	LOG_DBG("LTE trace: %d", type);

	switch (type) {
	case LTE_LC_TRACE_FUNC_MODE_NORMAL:
	case LTE_LC_TRACE_FUNC_MODE_ACTIVATE_LTE:
		if (connect_timer_started) {
			break;
		}

		err = memfault_metrics_heartbeat_timer_start(
			MEMFAULT_METRICS_KEY(Ncs_LteTimeToConnect));
		if (err) {
			LOG_WRN("LTE connection time tracking was not started, error: %d", err);
		} else {
			connect_timer_started = true;
		}

		break;
	default:
		break;
	}
#endif /* CONFIG_MEMFAULT_NCS_LTE_METRICS */
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
#ifdef CONFIG_MEMFAULT_NCS_LTE_METRICS
	int err;

	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		switch (evt->nw_reg_status) {

		case LTE_LC_NW_REG_REGISTERED_HOME:
		case LTE_LC_NW_REG_REGISTERED_ROAMING:
			connected = true;

			if (!connect_timer_started) {
				LOG_WRN("Ncs_LteTimeToConnect was not started");
				break;
			}

			err = memfault_metrics_heartbeat_timer_stop(
				MEMFAULT_METRICS_KEY(Ncs_LteTimeToConnect));
			if (err) {
				LOG_WRN("Failed to stop LTE connection timer, error: %d", err);
			} else {
				LOG_DBG("Ncs_LteTimeToConnect stopped");
				connect_timer_started = false;
			}

			break;
		case LTE_LC_NW_REG_NOT_REGISTERED:
		case LTE_LC_NW_REG_SEARCHING:
		case LTE_LC_NW_REG_REGISTRATION_DENIED:
		case LTE_LC_NW_REG_UNKNOWN:
		case LTE_LC_NW_REG_UICC_FAIL:
			if (connected) {
				err = memfault_metrics_heartbeat_add(
					MEMFAULT_METRICS_KEY(Ncs_LteConnectionLossCount), 1);
				if (err) {
					LOG_ERR("Failed to increment Ncs_LteConnectionLossCount");
				}

				if (connect_timer_started) {
					break;
				}

				err = memfault_metrics_heartbeat_timer_start(
					MEMFAULT_METRICS_KEY(Ncs_LteTimeToConnect));
				if (err) {
					LOG_WRN("Failed to start LTE connection timer, error: %d",
						err);
				} else {
					LOG_DBG("Ncs_LteTimeToConnect started");
					connect_timer_started = true;
				}
			}

			connected = false;
			break;
		default:
			break;
		}
	case LTE_LC_EVT_PSM_UPDATE:
		err = memfault_metrics_heartbeat_set_signed(
			MEMFAULT_METRICS_KEY(Ncs_LtePsmTauSec), evt->psm_cfg.tau);
		if (err) {
			LOG_ERR("Failed to set Ncs_LtePsmTau");
		}

		err = memfault_metrics_heartbeat_set_signed(
			MEMFAULT_METRICS_KEY(Ncs_LtePsmActiveTimeSec), evt->psm_cfg.active_time);
		if (err) {
			LOG_ERR("Failed to set Ncs_LtePsmActiveTime");
		}

		break;
	case LTE_LC_EVT_EDRX_UPDATE:
		err = memfault_metrics_heartbeat_set_unsigned(
			MEMFAULT_METRICS_KEY(Ncs_LteEdrxIntervalMsec),
					     (uint32_t)(evt->edrx_cfg.edrx * MSEC_PER_SEC));
		if (err) {
			LOG_ERR("Failed to set Ncs_LteEdrxInterval");
		}

		err = memfault_metrics_heartbeat_set_unsigned(
			MEMFAULT_METRICS_KEY(Ncs_LteEdrxPtwMsec),
					     (uint32_t)(evt->edrx_cfg.ptw * MSEC_PER_SEC));
		if (err) {
			LOG_ERR("Failed to set Ncs_LteEdrxPtw");
		}

		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		err = memfault_metrics_heartbeat_set_unsigned(
			MEMFAULT_METRICS_KEY(Ncs_LteMode), evt->lte_mode);
		if (err) {
			LOG_ERR("Failed to set Ncs_LteMode");
		}

		break;
	default:
		break;
	}

#endif /* CONFIG_MEMFAULT_NCS_LTE_METRICS */
}

/* Overriding weak implementation in Memfault SDK.
 * The function is run on every heartbeat and can be used to get fresh updates
 * of metrics.
 */
void memfault_metrics_heartbeat_collect_data(void)
{
	if (IS_ENABLED(CONFIG_MEMFAULT_NCS_STACK_METRICS)) {
		k_thread_foreach_unlocked(stack_check, NULL);
	}
}

void memfault_ncs_metrcics_init(void)
{
	if (IS_ENABLED(CONFIG_MEMFAULT_NCS_LTE_METRICS)) {
		lte_lc_trace_handler_set(lte_trace_cb);
		lte_lc_register_handler(lte_handler);
	}
}
