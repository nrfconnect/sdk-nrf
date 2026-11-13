/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>

#include "fake_nrfx_uarte.h"

#define MAX_QUEUED 2

/* A transfer owned by the fake hardware, with a copy of the data taken when it was started. */
struct xfer {
	const uint8_t *buf;
	size_t len;
	uint8_t snapshot[CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_NRFX_CHUNK_SZ];
};

struct fake_uarte fake_uarte;

static struct xfer queue[MAX_QUEUED];
static size_t queued;
static bool abort_requested;
static bool enabled;
/* The driver flags left behind by a synchronous abort, which only uninit clears. */
static bool stale_state;
static nrfx_uarte_event_handler_t evt_handler;
static void *evt_ctx;

static void hw_tick(struct k_timer *timer);
static K_TIMER_DEFINE(hw_timer, hw_tick, NULL);

static void protocol_error(const char *msg)
{
	if (!fake_uarte.protocol_error) {
		fake_uarte.protocol_error = msg;
	}
}

static void check_overwrite(const struct xfer *x)
{
	if (memcmp(x->buf, x->snapshot, x->len) != 0) {
		fake_uarte.overwrite_detected = true;
	}
}

/* The driver disables the UARTE when TX stops. */
static void hw_disable(void)
{
	enabled = false;
	fake_uarte.baudrate = 0;
}

static void enqueue(const uint8_t *buf, size_t len)
{
	struct xfer *x = &queue[queued++];

	x->buf = buf;
	x->len = len;
	memcpy(x->snapshot, buf, len);
	fake_uarte.max_queued = MAX(fake_uarte.max_queued, queued);
}

static void complete_head(size_t len, bool aborted)
{
	struct xfer x = queue[0];
	nrfx_uarte_event_t evt = {
		.type = NRFX_UARTE_EVT_TX_DONE,
		.data.tx.p_buffer = x.buf,
		.data.tx.length = len,
		.data.tx.flags = aborted ? NRFX_UARTE_TX_DONE_ABORTED : 0,
	};

	check_overwrite(&x);

	if (fake_uarte.sent_len + len <= sizeof(fake_uarte.sent)) {
		memcpy(&fake_uarte.sent[fake_uarte.sent_len], x.buf, len);
		fake_uarte.sent_len += len;
	} else {
		protocol_error("sent log overflow");
	}

	queue[0] = queue[1];
	queued--;
	fake_uarte.events++;

	if (queued == 0) {
		hw_disable();
	}

	evt_handler(&evt, evt_ctx);
}

static void hw_tick(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	if (abort_requested) {
		if (fake_uarte.abort_stuck) {
			return;
		}

		abort_requested = false;

		/* The active transfer reports what was sent, a linked one reports 0 bytes. */
		if (queued > 0) {
			complete_head(MIN(fake_uarte.abort_partial_len, queue[0].len), true);
		}
		while (queued > 0) {
			complete_head(0, true);
		}
		return;
	}

	if (fake_uarte.stalled) {
		return;
	}

	do {
		if (queued > 0) {
			complete_head(queue[0].len, false);
		}
	} while (fake_uarte.complete_all && queued > 0);
}

void fake_uarte_reset(void)
{
	k_timer_stop(&hw_timer);
	memset(&fake_uarte, 0, sizeof(fake_uarte));
	queued = 0;
	abort_requested = false;
	enabled = false;
	stale_state = false;
	evt_handler = NULL;
	evt_ctx = NULL;
}

int nrfx_uarte_init(nrfx_uarte_t *p_instance, nrfx_uarte_config_t const *p_config,
		    nrfx_uarte_event_handler_t event_handler)
{
	ARG_UNUSED(p_instance);

	fake_uarte.init_calls++;

	if (fake_uarte.initialized) {
		return -EALREADY;
	}

	if (event_handler == NULL) {
		protocol_error("blocking mode is not supported");
		return -EINVAL;
	}

	fake_uarte.config = *p_config;
	fake_uarte.initialized = true;
	evt_handler = event_handler;
	evt_ctx = p_config->p_context;
	queued = 0;
	abort_requested = false;
	stale_state = false;
	/* BAUDRATE is written while the UARTE is disabled, so it is lost. */
	hw_disable();

	k_timer_start(&hw_timer, K_MSEC(1), K_MSEC(1));

	return 0;
}

void nrfx_uarte_uninit(nrfx_uarte_t *p_instance)
{
	ARG_UNUSED(p_instance);

	k_timer_stop(&hw_timer);
	fake_uarte.initialized = false;
	queued = 0;
	abort_requested = false;
	stale_state = false;
	hw_disable();
}

int nrfx_uarte_tx(nrfx_uarte_t *p_instance, uint8_t const *p_data, size_t length, uint32_t flags)
{
	struct fake_uarte_tx_call *call = NULL;
	size_t idx = fake_uarte.tx_calls++;
	int ret = 0;

	ARG_UNUSED(p_instance);

	if (idx < ARRAY_SIZE(fake_uarte.tx_log)) {
		call = &fake_uarte.tx_log[idx];
		call->buf = p_data;
		call->len = length;
		call->flags = flags;
	} else {
		protocol_error("tx log overflow");
	}

	if (!fake_uarte.initialized) {
		protocol_error("tx while not initialized");
		ret = -EPERM;
	} else if (stale_state) {
		protocol_error("tx after synchronous abort without uninit");
		ret = -EBUSY;
	} else if (length == 0 || length > sizeof(queue[0].snapshot)) {
		protocol_error("invalid tx length");
		ret = -EINVAL;
	} else if (!nrf_dma_accessible_check(NULL, p_data) ||
		   !nrf_dma_accessible_check(NULL, p_data + length - 1)) {
		protocol_error("tx from memory that is not DMA accessible");
		ret = -EACCES;
	} else if (abort_requested) {
		protocol_error("tx while abort is pending");
		ret = -EBUSY;
	} else if (fake_uarte.tx_fail_at == idx + 1) {
		ret = fake_uarte.tx_fail_err;
	} else if ((flags & NRFX_UARTE_TX_LINK) && queued == 1 &&
		   fake_uarte.link_cancel_count > 0) {
		/* The driver keeps the transfer linked even though it reports a failure. */
		fake_uarte.link_cancel_count--;
		enqueue(p_data, length);
		ret = -ECANCELED;
	} else if (queued == MAX_QUEUED) {
		protocol_error("more than two transfers in flight");
		ret = -EBUSY;
	} else if (queued > 0 && !(flags & NRFX_UARTE_TX_LINK)) {
		protocol_error("tx without link while busy");
		ret = -EBUSY;
	} else {
		if (queued == 0 && fake_uarte.baudrate == 0) {
			protocol_error("tx started without restoring BAUDRATE");
		}
		enqueue(p_data, length);
	}

	if (call) {
		call->ret = ret;
	}

	return ret;
}

int nrfx_uarte_tx_abort(nrfx_uarte_t *p_instance, bool sync)
{
	ARG_UNUSED(p_instance);

	if (sync) {
		fake_uarte.sync_aborts++;
		if (fake_uarte.complete_on_sync_abort && queued > 0) {
			complete_head(queue[0].len, false);
		}
		if (queued == 0) {
			return -EINPROGRESS;
		}
		for (size_t i = 0; i < queued; i++) {
			check_overwrite(&queue[i]);
		}
		/* Only the active transfer is cleared, the aborted and linked flags remain. */
		stale_state = true;
		queued = 0;
		abort_requested = false;
		hw_disable();
		return 0;
	}

	fake_uarte.async_aborts++;

	if (queued == 0) {
		return -EINPROGRESS;
	}

	abort_requested = true;

	return 0;
}

void nrfx_uarte_irq_handler(const void *p_instance)
{
	ARG_UNUSED(p_instance);
}

bool nrfx_uarte_tx_in_progress(nrfx_uarte_t const *p_instance)
{
	ARG_UNUSED(p_instance);

	return queued > 0;
}

void nrf_uarte_enable(NRF_UARTE_Type *p_reg)
{
	ARG_UNUSED(p_reg);

	enabled = true;
}

void nrf_uarte_baudrate_set(NRF_UARTE_Type *p_reg, nrf_uarte_baudrate_t baudrate)
{
	ARG_UNUSED(p_reg);

	if (enabled) {
		fake_uarte.baudrate = baudrate;
		fake_uarte.baudrate_restores++;
	}
}

bool nrf_dma_accessible_check(void const *p_reg, void const *p_object)
{
	const uint8_t *addr = p_object;

	ARG_UNUSED(p_reg);

	return !(fake_uarte.inaccessible && addr >= fake_uarte.inaccessible &&
		 addr < fake_uarte.inaccessible + fake_uarte.inaccessible_len);
}

int pinctrl_apply_state(const struct pinctrl_dev_config *config, uint8_t id)
{
	ARG_UNUSED(config);

	fake_uarte.pinctrl_calls++;

	if (fake_uarte.pinctrl_err) {
		return fake_uarte.pinctrl_err;
	}

	fake_uarte.pinctrl_state = id;

	return 0;
}
