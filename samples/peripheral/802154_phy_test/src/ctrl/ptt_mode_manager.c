/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: mode manager implementation. */

#include "ptt.h"

#include "ctrl/ptt_trace.h"
#include "rf/ptt_rf.h"

#include "ptt_ctrl_internal.h"
#include "ptt_modes.h"

static enum ptt_ret ptt_mode_init(enum ptt_mode_t mode);

/* caller should take care about freeing shared resources */
enum ptt_ret ptt_mode_default_init(void)
{
	enum ptt_mode_t mode = ptt_ctrl_get_default_mode();

	return ptt_mode_init(mode);
}

enum ptt_ret ptt_mode_switch(enum ptt_mode_t new_mode)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;
	uint32_t mode_mask;

	if (new_mode >= PTT_MODE_N) {
		PTT_TRACE("%s: invalid new_mode %d\n", __func__, new_mode);
		ret = PTT_RET_INVALID_VALUE;
	} else {
		if (ptt_get_mode_mask_ext(&mode_mask)) {
			/* if new_mode isn't allowed */
			if ((mode_mask & (1u << new_mode)) == 0) {
				PTT_TRACE("%s: new_mode isn't allowed %d\n", __func__, new_mode);
				ret = PTT_RET_INVALID_VALUE;
			}
		}
	}

	if (ret == PTT_RET_SUCCESS) {
		ret = ptt_mode_uninit();
		if (ret == PTT_RET_SUCCESS) {
			ret = ptt_mode_init(new_mode);
		}
	}

	return ret;
}

enum ptt_ret ptt_mode_uninit(void)
{
	enum ptt_mode_t current_mode = ptt_ctrl_get_current_mode();
	enum ptt_ret ret = PTT_RET_SUCCESS;

	switch (current_mode) {
	case PTT_MODE_ZB_PERF_DUT:
		ret = ptt_zb_perf_dut_mode_uninit();
		break;

	case PTT_MODE_ZB_PERF_CMD:
		ret = ptt_zb_perf_cmd_mode_uninit();
		break;

	default:
		PTT_TRACE("%s: invalid current_mode %d\n", __func__, current_mode);
		ret = PTT_RET_INVALID_MODE;
		break;
	}

	if (ret == PTT_RET_SUCCESS) {
		ptt_timers_reset_all();
		ptt_events_reset_all();
		ptt_ctrl_handlers_reset_all();
		ptt_rf_reset();
		ptt_ctrl_set_current_mode(PTT_MODE_N);
	}

	return ret;
}

/* caller should take care about freeing shared resources */
static enum ptt_ret ptt_mode_init(enum ptt_mode_t mode)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	switch (mode) {
	case PTT_MODE_ZB_PERF_DUT:
		ret = ptt_zb_perf_dut_mode_init();
		break;

	case PTT_MODE_ZB_PERF_CMD:
		ret = ptt_zb_perf_cmd_mode_init();
		break;

	default:
		PTT_TRACE("%s: invalid mode %d\n", __func__, mode);
		ret = PTT_RET_INVALID_MODE;
		break;
	}

	if (ret == PTT_RET_SUCCESS) {
		ptt_ctrl_set_current_mode(mode);
	}

	return ret;
}
