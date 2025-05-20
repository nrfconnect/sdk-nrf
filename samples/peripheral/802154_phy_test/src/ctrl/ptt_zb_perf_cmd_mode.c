/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: implementation of Zigbee RF Performance Test Plan CMD mode */

#include "ctrl/ptt_trace.h"

#include "ptt_ctrl_internal.h"
#include "ptt_modes.h"
#include "ptt_zb_perf_cmd_mode.h"

enum ptt_ret ptt_zb_perf_cmd_mode_init(void)
{
	PTT_TRACE("%s ->\n", __func__);

	cmd_ota_cmd_init();
	cmd_uart_cmd_init();

	struct ptt_ext_evts_handlers_s *handlers = ptt_ctrl_get_handlers();

	handlers->rf_tx_finished = cmd_rf_tx_finished;
	handlers->rf_tx_failed = cmd_rf_tx_failed;
	handlers->rf_rx_done = cmd_rf_rx_done;
	handlers->rf_rx_failed = cmd_rf_rx_failed;
	handlers->uart_pkt_received = cmd_uart_pkt_received;
	handlers->rf_cca_done = cmd_rf_cca_done;
	handlers->rf_cca_failed = cmd_rf_cca_failed;
	handlers->rf_ed_detected = cmd_rf_ed_detected;
	handlers->rf_ed_failed = cmd_rf_ed_failed;

	PTT_TRACE("%s -<\n", __func__);
	return PTT_RET_SUCCESS;
}

enum ptt_ret ptt_zb_perf_cmd_mode_uninit(void)
{
	PTT_TRACE("%s ->\n", __func__);

	cmd_ota_cmd_uninit();
	cmd_uart_cmd_uninit();

	PTT_TRACE("%s -<\n", __func__);
	return PTT_RET_SUCCESS;
}
