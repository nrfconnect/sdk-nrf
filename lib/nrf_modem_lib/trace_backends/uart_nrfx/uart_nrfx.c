/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <modem/trace_backend.h>
#include <nrfx_uarte.h>
#include <soc_nrf_common.h>
#include <dmm.h>

LOG_MODULE_REGISTER(modem_trace_backend, CONFIG_MODEM_TRACE_BACKEND_LOG_LEVEL);

#define UARTE_NODE DT_CHOSEN(nordic_modem_trace_uart)

BUILD_ASSERT(DT_NODE_HAS_COMPAT(UARTE_NODE, nordic_nrf_uarte),
	     "nordic,modem-trace-uart must point to a nordic,nrf-uarte node");
BUILD_ASSERT(DT_NODE_HAS_STATUS(UARTE_NODE, reserved),
	     "nordic,modem-trace-uart node must have status \"reserved\" so that it is not "
	     "claimed by the Zephyr UART driver");
BUILD_ASSERT(DT_NODE_HAS_PROP(UARTE_NODE, current_speed),
	     "nordic,modem-trace-uart node must have the current-speed property");

#define CHUNK_SZ      CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_NRFX_CHUNK_SZ
#define BOUNCE_BUF_SZ CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_NRFX_BOUNCE_BUF_SZ
#define TX_TIMEOUT    K_MSEC(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_NRFX_TX_TIMEOUT_MS)

#if defined(UARTE_DMA_TX_MAXCNT_MAXCNT_Msk)
#define DMA_MAX_LEN (UARTE_DMA_TX_MAXCNT_MAXCNT_Msk >> UARTE_DMA_TX_MAXCNT_MAXCNT_Pos)
#else
#define DMA_MAX_LEN (UARTE_TXD_MAXCNT_MAXCNT_Msk >> UARTE_TXD_MAXCNT_MAXCNT_Pos)
#endif

BUILD_ASSERT(CHUNK_SZ <= DMA_MAX_LEN, "Trace chunk size exceeds the UARTE EasyDMA MAXCNT");
BUILD_ASSERT(BOUNCE_BUF_SZ <= DMA_MAX_LEN, "Bounce buffer size exceeds the UARTE EasyDMA MAXCNT");

/* BAUDRATE register value: 2^12 * floor(2^20 / round(f_PCLK / baudrate)). */
#define BAUDRATE     DT_PROP(UARTE_NODE, current_speed)
#define PCLK_FREQ    NRF_PERIPH_GET_FREQUENCY(UARTE_NODE)
#define BAUD_DIV     DIV_ROUND_CLOSEST(PCLK_FREQ, BAUDRATE)
#define BAUDRATE_REG ((BIT(20) / BAUD_DIV) << 12)

BUILD_ASSERT(BAUD_DIV > 0, "Baud rate is too high for the UARTE clock");

/* A chunk of 10-bit frames must be sent within the TX timeout. */
BUILD_ASSERT((uint64_t)CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_NRFX_TX_TIMEOUT_MS * BAUDRATE >
		     (uint64_t)CHUNK_SZ * 10 * MSEC_PER_SEC,
	     "TX timeout is shorter than the transmission time of one trace chunk");

/* One transfer on the wire and one linked transfer waiting in the buffered DMA registers. */
#define MAX_IN_FLIGHT 2

struct tx_done_evt {
	size_t len;
	bool aborted;
};

PINCTRL_DT_DEFINE(UARTE_NODE);

static nrfx_uarte_t uarte = NRFX_UARTE_INSTANCE(DT_REG_ADDR(UARTE_NODE));

K_MSGQ_DEFINE(tx_done_msgq, sizeof(struct tx_done_evt), 2 * MAX_IN_FLIGHT, 4);
static K_MUTEX_DEFINE(tx_lock);

static trace_backend_processed_cb trace_processed_callback;
static bool initialized;
static bool suspended;

#if BOUNCE_BUF_SZ > 0
static uint8_t bounce_buf[MAX_IN_FLIGHT][BOUNCE_BUF_SZ] DMM_MEMORY_SECTION(UARTE_NODE);
static uint8_t bounce_idx;
#endif

static void uarte_evt_handler(nrfx_uarte_event_t const *evt, void *ctx)
{
	ARG_UNUSED(ctx);

	if (evt->type != NRFX_UARTE_EVT_TX_DONE) {
		return;
	}

	struct tx_done_evt done = {
		.len = evt->data.tx.length,
		.aborted = (evt->data.tx.flags & NRFX_UARTE_TX_DONE_ABORTED) != 0,
	};

	if (k_msgq_put(&tx_done_msgq, &done, K_NO_WAIT)) {
		__ASSERT(0, "TX done queue overflow");
	}
}

static bool is_dma_accessible(const uint8_t *buf, size_t len)
{
	return nrf_dma_accessible_check(uarte.p_reg, buf) &&
	       nrf_dma_accessible_check(uarte.p_reg, buf + len - 1);
}

/* Start a DMA transfer of up to @p len bytes from @p src, linking it to the ongoing transfer
 * if @p link is set. On success, the number of bytes submitted is stored in @p submitted.
 */
static int tx_submit(const uint8_t *src, size_t len, bool link, size_t *submitted)
{
	const uint8_t *dma_buf = src;
	size_t n = MIN(len, CHUNK_SZ);
	unsigned int key;
	int err;

	if (!is_dma_accessible(src, n)) {
#if BOUNCE_BUF_SZ > 0
		n = MIN(n, BOUNCE_BUF_SZ);
		memcpy(bounce_buf[bounce_idx], src, n);
		dma_buf = bounce_buf[bounce_idx];
#else
		LOG_ERR("Trace data at %p is not DMA accessible", (void *)src);
		return -EFAULT;
#endif
	}

	/* The CPU may have written the data (e.g. the trace header), make it visible to DMA. */
	(void)sys_cache_data_flush_range((void *)dma_buf, n);

	/* Keep the driver TX state from changing between the check and the request. */
	key = irq_lock();
	if (!nrfx_uarte_tx_in_progress(&uarte)) {
		/* Some instances, such as UARTE120, do not retain BAUDRATE while disabled, and
		 * nrfx disables the UARTE after each transfer. Restore it after enabling.
		 */
		nrf_uarte_enable(uarte.p_reg);
		nrf_uarte_baudrate_set(uarte.p_reg, (nrf_uarte_baudrate_t)BAUDRATE_REG);
	}
	err = nrfx_uarte_tx(&uarte, dma_buf, n, link ? NRFX_UARTE_TX_LINK : 0);
	irq_unlock(key);
	if (err) {
		return err;
	}

#if BOUNCE_BUF_SZ > 0
	if (dma_buf != src) {
		bounce_idx = (bounce_idx + 1) % MAX_IN_FLIGHT;
	}
#endif

	*submitted = n;

	return 0;
}

/* Abort all transfers. The returned number of transfers is still to be reported by events.
 * A forced abort leaves the driver state inconsistent until nrfx_uarte_uninit(), so the write
 * must then fail for the trace library to deinitialize the backend.
 */
static uint8_t tx_abort(uint8_t in_flight, bool force)
{
	int err;

	if (force) {
		/* Synchronous abort generates no events. */
		(void)nrfx_uarte_tx_abort(&uarte, true);
		k_msgq_purge(&tx_done_msgq);
		return 0;
	}

	err = nrfx_uarte_tx_abort(&uarte, false);
	if (err == -EINPROGRESS) {
		/* No active transfer, all events have been reported already. */
		return k_msgq_num_used_get(&tx_done_msgq) ? in_flight : 0;
	}

	return in_flight;
}

int trace_backend_init(trace_backend_processed_cb trace_processed_cb)
{
	int err;
	nrfx_uarte_config_t cfg =
		NRFX_UARTE_DEFAULT_CONFIG(NRF_UARTE_PSEL_DISCONNECTED, NRF_UARTE_PSEL_DISCONNECTED);

	if (trace_processed_cb == NULL) {
		return -EFAULT;
	}

	k_mutex_lock(&tx_lock, K_FOREVER);

	trace_processed_callback = trace_processed_cb;

	if (initialized) {
		err = 0;
		goto out;
	}

	IRQ_CONNECT(DT_IRQN(UARTE_NODE), DT_IRQ(UARTE_NODE, priority), nrfx_uarte_irq_handler,
		    &uarte, 0);

	cfg.baudrate = (nrf_uarte_baudrate_t)BAUDRATE_REG;
	cfg.config.hwfc = DT_PROP(UARTE_NODE, hw_flow_control) ? NRF_UARTE_HWFC_ENABLED
							       : NRF_UARTE_HWFC_DISABLED;
	cfg.skip_psel_cfg = true;
	cfg.skip_gpio_cfg = true;
	cfg.interrupt_priority = DT_IRQ(UARTE_NODE, priority);

	err = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(UARTE_NODE), PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to apply pinctrl state, err %d", err);
		goto out;
	}

	err = nrfx_uarte_init(&uarte, &cfg, uarte_evt_handler);
	if (err) {
		LOG_ERR("Failed to initialize UARTE, err %d", err);
		(void)pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(UARTE_NODE),
					  PINCTRL_STATE_SLEEP);
		goto out;
	}

	k_msgq_purge(&tx_done_msgq);
	initialized = true;
	suspended = false;

	LOG_DBG("Trace UARTE at %p, %u baud (actual %u)", (void *)uarte.p_reg, BAUDRATE,
		(uint32_t)(((uint64_t)PCLK_FREQ * (BIT(20) / BAUD_DIV)) >> 20));

out:
	k_mutex_unlock(&tx_lock);

	return err;
}

int trace_backend_deinit(void)
{
	k_mutex_lock(&tx_lock, K_FOREVER);

	if (initialized) {
		(void)tx_abort(0, true);
		nrfx_uarte_uninit(&uarte);
		(void)pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(UARTE_NODE),
					  PINCTRL_STATE_SLEEP);
		initialized = false;
	}

	k_mutex_unlock(&tx_lock);

	return 0;
}

int trace_backend_write(const void *data, size_t len)
{
	const uint8_t *buf = data;
	size_t queued = 0;
	size_t processed = 0;
	uint8_t in_flight = 0;
	bool stop = false;
	bool abort_pending = false;
	int err = 0;

	k_mutex_lock(&tx_lock, K_FOREVER);

	if (!initialized || suspended) {
		k_mutex_unlock(&tx_lock);
		return -EPERM;
	}

	/* Keep the transmitter busy by linking the next chunk while the current one is sent.
	 * Each chunk is reported as processed once its transfer is done, so that the modem can
	 * reuse the trace memory. All transfers are completed before returning, as the trace
	 * library does not allow fetching new trace data while there is unprocessed data.
	 */
	while (true) {
		while (!stop && in_flight < MAX_IN_FLIGHT && queued < len) {
			size_t n;

			err = tx_submit(&buf[queued], len - queued, in_flight > 0, &n);
			if (err) {
				LOG_ERR("UARTE TX failed, err %d", err);
				stop = true;
				/* A failed link may leave a transfer queued in the driver. */
				if (in_flight > 0) {
					in_flight = tx_abort(in_flight, true);
				}
				break;
			}

			queued += n;
			in_flight++;
		}

		if (in_flight == 0) {
			break;
		}

		struct tx_done_evt done;

		if (k_msgq_get(&tx_done_msgq, &done, TX_TIMEOUT)) {
			stop = true;
			if (abort_pending) {
				LOG_ERR("UARTE TX abort timed out");
				in_flight = tx_abort(in_flight, true);
				if (!err) {
					err = -ETIMEDOUT;
				}
			} else {
				LOG_WRN("UARTE TX timed out, aborting");
				in_flight = tx_abort(in_flight, false);
				abort_pending = true;
			}
			continue;
		}

		in_flight--;

		if (done.aborted) {
			stop = true;
		}

		if (done.len == 0 || err) {
			continue;
		}

		/* Bytes that were sent are processed, even if the transfer was aborted. */
		err = trace_processed_callback(done.len);
		if (err) {
			LOG_ERR("Failed to process trace data, err %d", err);
			stop = true;
			if (in_flight > 0 && !abort_pending) {
				in_flight = tx_abort(in_flight, false);
				abort_pending = true;
			}
			continue;
		}

		processed += done.len;
	}

	k_mutex_unlock(&tx_lock);

	if (err) {
		return err;
	}

	return processed ? processed : -EAGAIN;
}

int trace_backend_suspend(void)
{
	int err = 0;

	k_mutex_lock(&tx_lock, K_FOREVER);

	if (initialized && !suspended) {
		/* No transfer is ongoing as writes are serialized with the lock, and the UARTE
		 * is disabled by the nrfx driver once TX is stopped. Release the pins.
		 */
		err = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(UARTE_NODE),
					  PINCTRL_STATE_SLEEP);
		if (err) {
			LOG_ERR("Failed to apply sleep pinctrl state, err %d", err);
		} else {
			suspended = true;
			LOG_DBG("Trace backend suspended");
		}
	}

	k_mutex_unlock(&tx_lock);

	return err;
}

int trace_backend_resume(void)
{
	int err = 0;

	k_mutex_lock(&tx_lock, K_FOREVER);

	if (initialized && suspended) {
		err = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(UARTE_NODE),
					  PINCTRL_STATE_DEFAULT);
		if (err) {
			LOG_ERR("Failed to apply default pinctrl state, err %d", err);
		} else {
			suspended = false;
			LOG_DBG("Trace backend resumed");
		}
	}

	k_mutex_unlock(&tx_lock);

	return err;
}

struct nrf_modem_lib_trace_backend trace_backend = {
	.init = trace_backend_init,
	.deinit = trace_backend_deinit,
	.write = trace_backend_write,
	.suspend = trace_backend_suspend,
	.resume = trace_backend_resume,
};
