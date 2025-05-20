/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: PTT interface part */

#include <assert.h>
#include <stddef.h>

#include "ptt.h"

#include "ctrl/ptt_trace.h"
#include "rf/ptt_rf.h"

#include "ptt_ctrl_internal.h"
#include "ptt_events_internal.h"
#include "ptt_modes.h"
#include "ptt_timers_internal.h"

#ifdef TESTS
#include "test_ctrl_conf.h"
#endif /* TESTS */

/** default timeout for waiting for any response */
#define PTT_DEFAULT_TIMEOUT (500u)

static struct ptt_ctrl_s ptt_ctrl_ctx;

void ptt_init(ptt_call_me_cb_t call_me_cb, ptt_time_t max_time)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	assert(call_me_cb != NULL);

	ptt_ctrl_handlers_reset_all();
	ptt_ctrl_ctx.call_me_cb = call_me_cb;
	ptt_ctrl_ctx.rsp_timeout = PTT_DEFAULT_TIMEOUT;
	ptt_ctrl_ctx.max_time = max_time;

	ptt_events_init();
	ptt_timers_init();
	ptt_rf_init();

	ret = ptt_mode_default_init();

	assert(ret == PTT_RET_SUCCESS);

	PTT_UNUSED(ret);
}

void ptt_uninit(void)
{
	ptt_mode_uninit();
	ptt_events_uninit();
	ptt_timers_uninit();
	ptt_rf_uninit();
}

void ptt_process(ptt_time_t current_time)
{
	ptt_timers_process(current_time);
}

struct ptt_timer_ctx_s *ptt_ctrl_get_timer_ctx(void)
{
	return &ptt_ctrl_ctx.timer_ctx;
}

void ptt_ctrl_call_me_cb(ptt_time_t timeout)
{
	if (ptt_ctrl_ctx.call_me_cb != NULL) {
		ptt_ctrl_ctx.call_me_cb(timeout);
	}
}

struct ptt_evt_ctx_s *ptt_ctrl_get_evt_ctx(void)
{
	return &ptt_ctrl_ctx.evt_ctx;
}

ptt_time_t ptt_ctrl_get_rsp_timeout(void)
{
	return ptt_ctrl_ctx.rsp_timeout;
}

void ptt_ctrl_set_rsp_timeout(ptt_time_t timeout)
{
	ptt_ctrl_ctx.rsp_timeout = timeout;
}

ptt_time_t ptt_ctrl_get_max_time(void)
{
	return ptt_ctrl_ctx.max_time;
}

enum ptt_mode_t ptt_ctrl_get_current_mode(void)
{
	return ptt_ctrl_ctx.current_mode;
}

void ptt_ctrl_set_current_mode(enum ptt_mode_t mode)
{
	ptt_ctrl_ctx.current_mode = mode;
}

enum ptt_mode_t ptt_ctrl_get_default_mode(void)
{
	uint32_t mode_mask;
	enum ptt_mode_t mode = PTT_MODE_ZB_PERF_DUT;

	if (ptt_get_mode_mask_ext(&mode_mask)) {
		for (uint8_t i = 0; i <= sizeof(mode_mask) * 8; i++) {
			if (((mode_mask >> i) & 1u) == 1u) {
				mode = i;
				break;
			}
		}
	}

	return mode;
}

uint8_t ptt_ctrl_get_hw_version(void)
{
	uint8_t hw_version = 0xFF;

	ptt_get_hw_version_ext(&hw_version);
	return hw_version;
}

uint8_t ptt_ctrl_get_sw_version(void)
{
	uint8_t sw_version = 0xFF;

	ptt_get_sw_version_ext(&sw_version);
	return sw_version;
}

void ptt_ctrl_set_dcdc(bool activate)
{
	ptt_set_dcdc_ext(activate);
}

bool ptt_ctrl_get_dcdc(void)
{
	return ptt_get_dcdc_ext();
}

enum ptt_ret ptt_ctrl_get_temp(int32_t *temp)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (!ptt_get_temp_ext(temp)) {
		ret = PTT_RET_INVALID_VALUE;
	}

	return ret;
}

struct ptt_ext_evts_handlers_s *ptt_ctrl_get_handlers(void)
{
	return &ptt_ctrl_ctx.mode_handlers;
}

void ptt_random_vector_generate(uint8_t *buff, uint8_t requested_size)
{
	for (int i = 0; i < requested_size; ++i) {
		buff[i] = ptt_random_get_ext() % UINT8_MAX;
	}
}

void ptt_ctrl_set_icache(bool enable)
{
	ptt_set_icache_ext(enable);
}

bool ptt_ctrl_get_icache(void)
{
	return ptt_get_icache_ext();
}

void ptt_ctrl_led_indication_on(void)
{
	ptt_ctrl_led_indication_on_ext();
}

void ptt_ctrl_led_indication_off(void)
{
	ptt_ctrl_led_indication_off_ext();
}

void ptt_ctrl_handlers_reset_all(void)
{
	ptt_ctrl_ctx.mode_handlers = (struct ptt_ext_evts_handlers_s){ 0 };
}

ptt_ext_evt_handler ptt_ctrl_get_handler_uart_pkt_received(void)
{
	return ptt_ctrl_ctx.mode_handlers.uart_pkt_received;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_tx_finished(void)
{
	return ptt_ctrl_ctx.mode_handlers.rf_tx_finished;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_tx_failed(void)
{
	return ptt_ctrl_ctx.mode_handlers.rf_tx_failed;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_rx_failed(void)
{
	return ptt_ctrl_ctx.mode_handlers.rf_rx_failed;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_tx_started(void)
{
	return ptt_ctrl_ctx.mode_handlers.rf_tx_started;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_rx_done(void)
{
	return ptt_ctrl_ctx.mode_handlers.rf_rx_done;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_ed_detected(void)
{
	return ptt_ctrl_ctx.mode_handlers.rf_ed_detected;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_ed_failed(void)
{
	return ptt_ctrl_ctx.mode_handlers.rf_ed_failed;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_cca_done(void)
{
	return ptt_ctrl_ctx.mode_handlers.rf_cca_done;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_cca_failed(void)
{
	return ptt_ctrl_ctx.mode_handlers.rf_cca_failed;
}

#ifdef TESTS
#include "test_ctrl_wrappers.c"
#endif /* TESTS */
