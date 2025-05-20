/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: implementation of Zigbee RF Performance Test Plan DUT mode */

#ifndef PTT_MODE_ZB_PERF_DUT_H__
#define PTT_MODE_ZB_PERF_DUT_H__

#include <assert.h>

#include "ctrl/ptt_ctrl.h"
#include "ctrl/ptt_events.h"
#include "ctrl/ptt_timers.h"
#include "ctrl/ptt_uart.h"

#include "ptt_errors.h"

/** possible states of library in DUT mode */
enum dut_state_t {
	DUT_STATE_IDLE = 0, /**< waiting for next command */
	DUT_STATE_ACK_SENDING, /**< rf module send ack command */
	DUT_STATE_POWER_SENDING, /**< DUT sends power */
	DUT_STATE_STREAM_EMITTING, /**< DUT emits modulated stream */
	DUT_STATE_RX_TEST_WAIT_FOR_END, /**< rf module wait for end_rx_test command */
	DUT_STATE_RX_TEST_REPORT_SENDING, /**< rf module send end_rx_test command */
	DUT_STATE_HW_VERSION_SENDING, /**< DUT sends hardware version */
	DUT_STATE_SW_VERSION_SENDING, /**< DUT sends software version */
	DUT_STATE_ANTENNA_SENDING, /**< DUT sends antenna */
	DUT_STATE_N /**< states count */
};

#endif /* PTT_MODE_ZB_PERF_DUT_H__ */
