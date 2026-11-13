/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FAKE_NRFX_UARTE_H__
#define FAKE_NRFX_UARTE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <nrfx_uarte.h>

#define FAKE_UARTE_TX_LOG_LEN 32
#define FAKE_UARTE_SENT_LEN   256

struct fake_uarte_tx_call {
	const uint8_t *buf;
	size_t len;
	uint32_t flags;
	int ret;
};

/* The fake UARTE completes one transfer per millisecond tick, unless configured otherwise. */
struct fake_uarte {
	/* Behavior controls, set by the test. */

	/* Transfers do not complete on their own. */
	bool stalled;
	/* Asynchronous aborts never complete. */
	bool abort_stuck;
	/* The active transfer completes right before a synchronous abort, as if the interrupt
	 * fired just before the abort was requested.
	 */
	bool complete_on_sync_abort;
	/* Number of bytes reported as sent for the active transfer when it is aborted. */
	size_t abort_partial_len;
	/* Complete all queued transfers on each tick instead of one. */
	bool complete_all;
	/* Fail the next linked transfers with -ECANCELED, as when TXSTARTED is not seen in time.
	 * Like the real driver, the transfer stays linked and is sent after the active one.
	 */
	int link_cancel_count;
	/* Fail the transfer request with the given 1-based call index. */
	size_t tx_fail_at;
	int tx_fail_err;
	/* Memory that is not accessible by EasyDMA. */
	const uint8_t *inaccessible;
	size_t inaccessible_len;
	/* Error returned by pinctrl_apply_state(). */
	int pinctrl_err;

	/* Observations. */
	bool initialized;
	size_t init_calls;
	nrfx_uarte_config_t config;
	struct fake_uarte_tx_call tx_log[FAKE_UARTE_TX_LOG_LEN];
	size_t tx_calls;
	size_t max_queued;
	size_t async_aborts;
	size_t sync_aborts;
	/* BAUDRATE register, which is lost while the UARTE is disabled, as on UARTE120. */
	uint32_t baudrate;
	size_t baudrate_restores;
	size_t events;
	uint8_t sent[FAKE_UARTE_SENT_LEN];
	size_t sent_len;
	bool overwrite_detected;
	const char *protocol_error;
	uint8_t pinctrl_state;
	size_t pinctrl_calls;
};

extern struct fake_uarte fake_uarte;

void fake_uarte_reset(void);

#endif /* FAKE_NRFX_UARTE_H__ */
