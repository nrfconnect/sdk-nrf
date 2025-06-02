/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/net/socket.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <nrf_errno.h>
#include <nrf_modem_at.h>
#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>

#include "modules/cereg.h"
#include "modules/cfun.h"
#include "modules/coneval.h"
#include "modules/cscon.h"
#include "modules/edrx.h"
#include "modules/mdmev.h"
#include "modules/ncellmeas.h"
#include "modules/periodicsearchconf.h"
#include "modules/psm.h"
#include "modules/xmodemsleep.h"
#include "modules/xsystemmode.h"
#include "modules/xt3412.h"

#include "common/work_q.h"
#include "common/event_handler_list.h"

LOG_MODULE_REGISTER(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* Public API */

void lte_lc_register_handler(lte_lc_evt_handler_t handler)
{
	if (handler == NULL) {
		LOG_INF("NULL as a handler received: Nothing to be done.\n"
			"The handler can be deregistered using lte_lc_deregister_handler()");
		return;
	}

	event_handler_list_handler_append(handler);
}

int lte_lc_deregister_handler(lte_lc_evt_handler_t handler)
{
	if (handler == NULL) {
		LOG_ERR("Invalid handler (handler=0x%08X)", (uint32_t)handler);
		return -EINVAL;
	}

	return event_handler_list_handler_remove(handler);
}

int lte_lc_connect(void)
{
	LOG_DBG("Connecting synchronously");
	return cereg_lte_connect(true);
}

int lte_lc_connect_async(lte_lc_evt_handler_t handler)
{
	LOG_DBG("Connecting asynchronously");
	if (handler) {
		event_handler_list_handler_append(handler);
	} else if (event_handler_list_is_empty()) {
		LOG_ERR("No handler registered");
		return -EINVAL;
	}

	return cereg_lte_connect(false);
}

int lte_lc_normal(void)
{
	return cfun_mode_set(LTE_LC_FUNC_MODE_NORMAL) ? -EFAULT : 0;
}

int lte_lc_offline(void)
{
	return cfun_mode_set(LTE_LC_FUNC_MODE_OFFLINE) ? -EFAULT : 0;
}

int lte_lc_power_off(void)
{
	return cfun_mode_set(LTE_LC_FUNC_MODE_POWER_OFF) ? -EFAULT : 0;
}

int lte_lc_psm_param_set(const char *rptau, const char *rat)
{
	return psm_param_set(rptau, rat);
}

int lte_lc_psm_param_set_seconds(int rptau, int rat)
{
	return psm_param_set_seconds(rptau, rat);
}

int lte_lc_psm_req(bool enable)
{
	return psm_req(enable);
}

int lte_lc_psm_get(int *tau, int *active_time)
{
	return psm_get(tau, active_time);
}

int lte_lc_proprietary_psm_req(bool enable)
{
	return psm_proprietary_req(enable);
}

int lte_lc_edrx_param_set(enum lte_lc_lte_mode mode, const char *edrx)
{
	return edrx_param_set(mode, edrx);
}

int lte_lc_ptw_set(enum lte_lc_lte_mode mode, const char *ptw)
{
	return edrx_ptw_set(mode, ptw);
}

int lte_lc_edrx_req(bool enable)
{
	return edrx_request(enable);
}

int lte_lc_edrx_get(struct lte_lc_edrx_cfg *edrx_cfg)
{
	return edrx_cfg_get(edrx_cfg);
}

int lte_lc_nw_reg_status_get(enum lte_lc_nw_reg_status *status)
{
	return cereg_status_get(status);
}

int lte_lc_system_mode_set(enum lte_lc_system_mode mode,
			   enum lte_lc_system_mode_preference preference)
{
	return xsystemmode_mode_set(mode, preference);
}

int lte_lc_system_mode_get(enum lte_lc_system_mode *mode,
			   enum lte_lc_system_mode_preference *preference)
{
	return xsystemmode_mode_get(mode, preference);
}

int lte_lc_func_mode_get(enum lte_lc_func_mode *mode)
{
	return cfun_mode_get(mode);
}

int lte_lc_func_mode_set(enum lte_lc_func_mode mode)
{
	return cfun_mode_set(mode);
}

int lte_lc_lte_mode_get(enum lte_lc_lte_mode *mode)
{
	return cereg_mode_get(mode);
}

int lte_lc_neighbor_cell_measurement(struct lte_lc_ncellmeas_params *params)
{
	return ncellmeas_start(params);
}

int lte_lc_neighbor_cell_measurement_cancel(void)
{
	return ncellmeas_cancel();
}

int lte_lc_conn_eval_params_get(struct lte_lc_conn_eval_params *params)
{
	return coneval_params_get(params);
}

int lte_lc_modem_events_enable(void)
{
	return mdmev_enable();
}

int lte_lc_modem_events_disable(void)
{
	return mdmev_disable();
}

int lte_lc_periodic_search_set(const struct lte_lc_periodic_search_cfg *const cfg)
{
	return periodicsearchconf_set(cfg);
}

int lte_lc_periodic_search_clear(void)
{
	return periodicsearchconf_clear();
}

int lte_lc_periodic_search_request(void)
{
	return periodicsearchconf_request();
}

int lte_lc_periodic_search_get(struct lte_lc_periodic_search_cfg *const cfg)
{
	return periodicsearchconf_get(cfg);
}

static int lte_lc_sys_init(void)
{
	work_q_start();

	return 0;
}

SYS_INIT(lte_lc_sys_init, APPLICATION, 0);
