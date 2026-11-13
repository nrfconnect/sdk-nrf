/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <unity.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>

#include <modem/trace_backend.h>

#include "fake_nrfx_uarte.h"

#define CHUNK_SZ      CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_NRFX_CHUNK_SZ
#define BOUNCE_BUF_SZ CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_NRFX_BOUNCE_BUF_SZ

#define MAX_CB_CALLS 32

extern int unity_main(void);

extern struct nrf_modem_lib_trace_backend trace_backend;

static uint8_t trace_data[128];

static struct {
	size_t calls;
	size_t lens[MAX_CB_CALLS];
	size_t total;
	/* Return fail_err on the given 1-based call. */
	size_t fail_at;
	int fail_err;
} processed;

static int processed_cb(size_t len)
{
	size_t call = ++processed.calls;

	if (call <= ARRAY_SIZE(processed.lens)) {
		processed.lens[call - 1] = len;
	}

	if (call == processed.fail_at) {
		return processed.fail_err;
	}

	processed.total += len;

	return 0;
}

static void backend_init(void)
{
	TEST_ASSERT_EQUAL(0, trace_backend.init(processed_cb));
}

/* The trace library deinitializes the backend after a write error. */
static void backend_reinit(void)
{
	TEST_ASSERT_EQUAL(0, trace_backend.deinit());
	backend_init();
}

static void reset_counters(void)
{
	memset(&processed, 0, sizeof(processed));
	fake_uarte.tx_calls = 0;
	fake_uarte.async_aborts = 0;
	fake_uarte.sync_aborts = 0;
	fake_uarte.events = 0;
	fake_uarte.sent_len = 0;
}

static void make_trace_data_inaccessible(void)
{
	fake_uarte.inaccessible = trace_data;
	fake_uarte.inaccessible_len = sizeof(trace_data);
}

/* Assert that a write of len bytes completed fully and in order, in chunks. */
static void assert_write_complete(int ret, size_t len, size_t chunk)
{
	TEST_ASSERT_EQUAL(len, ret);
	TEST_ASSERT_EQUAL(len, processed.total);
	TEST_ASSERT_EQUAL(DIV_ROUND_UP(len, chunk), processed.calls);
	for (size_t i = 0; i < processed.calls; i++) {
		TEST_ASSERT_EQUAL(MIN(chunk, len - i * chunk), processed.lens[i]);
	}
	TEST_ASSERT_EQUAL(len, fake_uarte.sent_len);
	TEST_ASSERT_EQUAL_MEMORY(trace_data, fake_uarte.sent, len);
}

/* Assert that a new write is not affected by leftovers of the previous one. */
static void assert_next_write_clean(void)
{
	int ret;

	fake_uarte.stalled = false;
	fake_uarte.abort_stuck = false;
	fake_uarte.complete_on_sync_abort = false;
	fake_uarte.complete_all = false;
	fake_uarte.tx_fail_at = 0;
	fake_uarte.link_cancel_count = 0;
	reset_counters();

	ret = trace_backend.write(trace_data, CHUNK_SZ + 4);
	assert_write_complete(ret, CHUNK_SZ + 4, CHUNK_SZ);
	TEST_ASSERT_EQUAL(2, fake_uarte.events);
}

void setUp(void)
{
	fake_uarte_reset();
	memset(&processed, 0, sizeof(processed));

	for (size_t i = 0; i < sizeof(trace_data); i++) {
		trace_data[i] = (uint8_t)(i * 7 + 3);
	}
}

void tearDown(void)
{
	(void)trace_backend.deinit();

	TEST_ASSERT_NULL_MESSAGE(fake_uarte.protocol_error, fake_uarte.protocol_error);
	TEST_ASSERT_FALSE(fake_uarte.overwrite_detected);
	TEST_ASSERT_LESS_OR_EQUAL(2, fake_uarte.max_queued);
}

void test_init_null_callback(void)
{
	TEST_ASSERT_EQUAL(-EFAULT, trace_backend.init(NULL));
	TEST_ASSERT_FALSE(fake_uarte.initialized);
}

void test_init_configures_uarte(void)
{
	backend_init();

	TEST_ASSERT_TRUE(fake_uarte.initialized);
	TEST_ASSERT_EQUAL(PINCTRL_STATE_DEFAULT, fake_uarte.pinctrl_state);
	/* 1 Mbaud from a 16 MHz clock: 2^12 * floor(2^20 / 16). */
	TEST_ASSERT_EQUAL_HEX32(0x10000000, fake_uarte.config.baudrate);
	TEST_ASSERT_EQUAL(NRF_UARTE_HWFC_DISABLED, fake_uarte.config.config.hwfc);
	TEST_ASSERT_TRUE(fake_uarte.config.skip_psel_cfg);
	TEST_ASSERT_TRUE(fake_uarte.config.skip_gpio_cfg);
	TEST_ASSERT_EQUAL(1, fake_uarte.config.interrupt_priority);

	/* Initializing again only updates the callback. */
	backend_init();
	TEST_ASSERT_EQUAL(1, fake_uarte.init_calls);
}

void test_init_pinctrl_error(void)
{
	fake_uarte.pinctrl_err = -EIO;

	TEST_ASSERT_EQUAL(-EIO, trace_backend.init(processed_cb));
	TEST_ASSERT_FALSE(fake_uarte.initialized);
	TEST_ASSERT_EQUAL(-EPERM, trace_backend.write(trace_data, 4));
}

void test_write_not_initialized(void)
{
	TEST_ASSERT_EQUAL(-EPERM, trace_backend.write(trace_data, 4));
	TEST_ASSERT_EQUAL(0, fake_uarte.tx_calls);
}

void test_deinit(void)
{
	backend_init();

	TEST_ASSERT_EQUAL(0, trace_backend.deinit());
	TEST_ASSERT_FALSE(fake_uarte.initialized);
	TEST_ASSERT_EQUAL(PINCTRL_STATE_SLEEP, fake_uarte.pinctrl_state);
	TEST_ASSERT_EQUAL(-EPERM, trace_backend.write(trace_data, 4));
}

void test_write_single_chunk(void)
{
	int ret;

	backend_init();

	ret = trace_backend.write(trace_data, 10);

	assert_write_complete(ret, 10, CHUNK_SZ);
	TEST_ASSERT_EQUAL(1, fake_uarte.tx_calls);
	TEST_ASSERT_EQUAL(0, fake_uarte.tx_log[0].flags);
	TEST_ASSERT_EQUAL_PTR(trace_data, fake_uarte.tx_log[0].buf);
}

void test_write_links_chunks(void)
{
	const size_t len = 3 * CHUNK_SZ + 5;
	int ret;

	backend_init();

	ret = trace_backend.write(trace_data, len);

	assert_write_complete(ret, len, CHUNK_SZ);
	TEST_ASSERT_EQUAL(4, fake_uarte.tx_calls);
	TEST_ASSERT_EQUAL(2, fake_uarte.max_queued);
	TEST_ASSERT_EQUAL(0, fake_uarte.tx_log[0].flags);
	for (size_t i = 1; i < fake_uarte.tx_calls; i++) {
		TEST_ASSERT_EQUAL(NRFX_UARTE_TX_LINK, fake_uarte.tx_log[i].flags);
		TEST_ASSERT_EQUAL_PTR(&trace_data[i * CHUNK_SZ], fake_uarte.tx_log[i].buf);
	}
	/* BAUDRATE is restored only when starting, not when linking. */
	TEST_ASSERT_EQUAL(1, fake_uarte.baudrate_restores);
}

void test_write_restores_baudrate_on_each_start(void)
{
	int ret;

	backend_init();
	TEST_ASSERT_EQUAL_HEX32(0, fake_uarte.baudrate);

	ret = trace_backend.write(trace_data, 4);
	assert_write_complete(ret, 4, CHUNK_SZ);

	reset_counters();
	ret = trace_backend.write(trace_data, 4);
	assert_write_complete(ret, 4, CHUNK_SZ);

	TEST_ASSERT_EQUAL(2, fake_uarte.baudrate_restores);
}

void test_write_link_after_transfers_done_starts_new(void)
{
	const size_t len = 5 * CHUNK_SZ;
	int ret;

	backend_init();
	fake_uarte.complete_all = true;

	/* Both transfers are done before the next one is linked, so the driver starts a new
	 * transfer which requires BAUDRATE to be restored.
	 */
	ret = trace_backend.write(trace_data, len);

	assert_write_complete(ret, len, CHUNK_SZ);
	TEST_ASSERT_EQUAL(0, fake_uarte.async_aborts);
	TEST_ASSERT_EQUAL(0, fake_uarte.sync_aborts);
	TEST_ASSERT_EQUAL(3, fake_uarte.baudrate_restores);
}

void test_write_link_canceled_is_fatal(void)
{
	backend_init();
	fake_uarte.link_cancel_count = 1;

	TEST_ASSERT_EQUAL(-ECANCELED, trace_backend.write(trace_data, 3 * CHUNK_SZ));
	TEST_ASSERT_EQUAL(2, fake_uarte.tx_calls);
	TEST_ASSERT_EQUAL(NRFX_UARTE_TX_LINK, fake_uarte.tx_log[1].flags);
	TEST_ASSERT_EQUAL(0, fake_uarte.async_aborts);
	TEST_ASSERT_EQUAL(1, fake_uarte.sync_aborts);
	TEST_ASSERT_EQUAL(0, processed.calls);

	backend_reinit();
	assert_next_write_clean();
}

void test_write_tx_error(void)
{
	backend_init();
	fake_uarte.tx_fail_at = 1;
	fake_uarte.tx_fail_err = -EIO;

	TEST_ASSERT_EQUAL(-EIO, trace_backend.write(trace_data, 2 * CHUNK_SZ));
	TEST_ASSERT_EQUAL(1, fake_uarte.tx_calls);
	TEST_ASSERT_EQUAL(0, processed.calls);
	TEST_ASSERT_EQUAL(0, fake_uarte.async_aborts);

	assert_next_write_clean();
}

void test_write_link_error_aborts_active_transfer(void)
{
	backend_init();
	fake_uarte.tx_fail_at = 2;
	fake_uarte.tx_fail_err = -EIO;

	TEST_ASSERT_EQUAL(-EIO, trace_backend.write(trace_data, 2 * CHUNK_SZ));
	TEST_ASSERT_EQUAL(2, fake_uarte.tx_calls);
	TEST_ASSERT_EQUAL(0, fake_uarte.async_aborts);
	TEST_ASSERT_EQUAL(1, fake_uarte.sync_aborts);
	TEST_ASSERT_EQUAL(0, fake_uarte.events);
	TEST_ASSERT_EQUAL(0, processed.calls);

	backend_reinit();
	assert_next_write_clean();
}

void test_write_processed_cb_error_aborts_linked_transfer(void)
{
	backend_init();
	processed.fail_at = 1;
	processed.fail_err = -ENOMEM;
	fake_uarte.abort_partial_len = 4;

	TEST_ASSERT_EQUAL(-ENOMEM, trace_backend.write(trace_data, 3 * CHUNK_SZ));

	/* The third chunk is never sent, and the aborted second chunk is not processed. */
	TEST_ASSERT_EQUAL(2, fake_uarte.tx_calls);
	TEST_ASSERT_EQUAL(1, fake_uarte.async_aborts);
	TEST_ASSERT_EQUAL(0, fake_uarte.sync_aborts);
	TEST_ASSERT_EQUAL(2, fake_uarte.events);
	TEST_ASSERT_EQUAL(1, processed.calls);
	TEST_ASSERT_EQUAL(CHUNK_SZ + 4, fake_uarte.sent_len);

	assert_next_write_clean();
}

void test_write_processed_cb_error_after_all_transfers_done(void)
{
	backend_init();
	processed.fail_at = 1;
	processed.fail_err = -ENOMEM;
	fake_uarte.complete_all = true;

	/* Both transfers are done when the first completion is handled, so the abort finds no
	 * active transfer and the second completion is already queued.
	 */
	TEST_ASSERT_EQUAL(-ENOMEM, trace_backend.write(trace_data, 2 * CHUNK_SZ));
	TEST_ASSERT_EQUAL(1, fake_uarte.async_aborts);
	TEST_ASSERT_EQUAL(0, fake_uarte.sync_aborts);
	TEST_ASSERT_EQUAL(2, fake_uarte.events);
	TEST_ASSERT_EQUAL(1, processed.calls);

	assert_next_write_clean();
}

void test_write_timeout_partial_abort(void)
{
	int ret;

	backend_init();
	fake_uarte.stalled = true;
	fake_uarte.abort_partial_len = 5;

	ret = trace_backend.write(trace_data, 2 * CHUNK_SZ);

	/* Bytes sent before the abort are processed, the linked transfer reports 0 bytes. */
	TEST_ASSERT_EQUAL(5, ret);
	TEST_ASSERT_EQUAL(1, processed.calls);
	TEST_ASSERT_EQUAL(5, processed.total);
	TEST_ASSERT_EQUAL(2, fake_uarte.events);
	TEST_ASSERT_EQUAL(1, fake_uarte.async_aborts);
	TEST_ASSERT_EQUAL(0, fake_uarte.sync_aborts);
	TEST_ASSERT_EQUAL(5, fake_uarte.sent_len);
	TEST_ASSERT_EQUAL_MEMORY(trace_data, fake_uarte.sent, 5);

	assert_next_write_clean();
}

void test_write_timeout_zero_byte_abort(void)
{
	backend_init();
	fake_uarte.stalled = true;
	fake_uarte.abort_partial_len = 0;

	TEST_ASSERT_EQUAL(-EAGAIN, trace_backend.write(trace_data, 2 * CHUNK_SZ));
	TEST_ASSERT_EQUAL(0, processed.calls);
	TEST_ASSERT_EQUAL(2, fake_uarte.events);
	TEST_ASSERT_EQUAL(1, fake_uarte.async_aborts);
	TEST_ASSERT_EQUAL(0, fake_uarte.sync_aborts);

	assert_next_write_clean();
}

void test_write_timeout_escalates_to_sync_abort(void)
{
	backend_init();
	fake_uarte.stalled = true;
	fake_uarte.abort_stuck = true;

	TEST_ASSERT_EQUAL(-ETIMEDOUT, trace_backend.write(trace_data, 2 * CHUNK_SZ));
	TEST_ASSERT_EQUAL(0, processed.calls);
	TEST_ASSERT_EQUAL(0, fake_uarte.events);
	TEST_ASSERT_EQUAL(1, fake_uarte.async_aborts);
	TEST_ASSERT_EQUAL(1, fake_uarte.sync_aborts);

	backend_reinit();
	assert_next_write_clean();
}

void test_write_sync_abort_discards_late_completion(void)
{
	backend_init();
	fake_uarte.stalled = true;
	fake_uarte.abort_stuck = true;
	fake_uarte.complete_on_sync_abort = true;

	/* A completion that races with the synchronous abort is discarded with the write. */
	TEST_ASSERT_EQUAL(-ETIMEDOUT, trace_backend.write(trace_data, 2 * CHUNK_SZ));
	TEST_ASSERT_EQUAL(0, processed.calls);
	TEST_ASSERT_EQUAL(1, fake_uarte.events);
	TEST_ASSERT_EQUAL(1, fake_uarte.sync_aborts);

	backend_reinit();
	assert_next_write_clean();
}

void test_suspend_resume(void)
{
	int ret;

	backend_init();

	TEST_ASSERT_EQUAL(0, trace_backend.suspend());
	TEST_ASSERT_EQUAL(PINCTRL_STATE_SLEEP, fake_uarte.pinctrl_state);
	TEST_ASSERT_EQUAL(-EPERM, trace_backend.write(trace_data, 4));
	TEST_ASSERT_EQUAL(0, fake_uarte.tx_calls);

	TEST_ASSERT_EQUAL(0, trace_backend.resume());
	TEST_ASSERT_EQUAL(PINCTRL_STATE_DEFAULT, fake_uarte.pinctrl_state);

	ret = trace_backend.write(trace_data, 4);
	assert_write_complete(ret, 4, CHUNK_SZ);
}

void test_suspend_pinctrl_error(void)
{
	int ret;

	backend_init();
	fake_uarte.pinctrl_err = -EIO;

	TEST_ASSERT_EQUAL(-EIO, trace_backend.suspend());

	/* The backend is not suspended. */
	fake_uarte.pinctrl_err = 0;
	ret = trace_backend.write(trace_data, 4);
	assert_write_complete(ret, 4, CHUNK_SZ);
}

void test_write_dma_accessible_data_sent_in_place(void)
{
#if BOUNCE_BUF_SZ > 0
	int ret;

	backend_init();

	ret = trace_backend.write(trace_data, 2 * CHUNK_SZ);

	assert_write_complete(ret, 2 * CHUNK_SZ, CHUNK_SZ);
	TEST_ASSERT_EQUAL_PTR(&trace_data[0], fake_uarte.tx_log[0].buf);
	TEST_ASSERT_EQUAL_PTR(&trace_data[CHUNK_SZ], fake_uarte.tx_log[1].buf);
#else
	TEST_IGNORE_MESSAGE("Bounce buffers are disabled");
#endif
}

void test_write_bounce_buffers_alternate(void)
{
#if BOUNCE_BUF_SZ > 0
	const size_t len = 5 * BOUNCE_BUF_SZ;
	int ret;

	backend_init();
	make_trace_data_inaccessible();

	ret = trace_backend.write(trace_data, len);

	assert_write_complete(ret, len, BOUNCE_BUF_SZ);
	TEST_ASSERT_EQUAL(5, fake_uarte.tx_calls);
	TEST_ASSERT_EQUAL(2, fake_uarte.max_queued);
	for (size_t i = 0; i < fake_uarte.tx_calls; i++) {
		TEST_ASSERT_EQUAL(BOUNCE_BUF_SZ, fake_uarte.tx_log[i].len);
		TEST_ASSERT_TRUE(nrf_dma_accessible_check(NULL, fake_uarte.tx_log[i].buf));
	}
	/* Consecutive transfers, which may be in flight at the same time, use different
	 * bounce buffers.
	 */
	for (size_t i = 1; i < fake_uarte.tx_calls; i++) {
		TEST_ASSERT_TRUE(fake_uarte.tx_log[i - 1].buf != fake_uarte.tx_log[i].buf);
	}
	TEST_ASSERT_EQUAL_PTR(fake_uarte.tx_log[0].buf, fake_uarte.tx_log[2].buf);
#else
	TEST_IGNORE_MESSAGE("Bounce buffers are disabled");
#endif
}

void test_write_bounce_buffers_after_abort(void)
{
#if BOUNCE_BUF_SZ > 0
	int ret;

	backend_init();
	make_trace_data_inaccessible();
	fake_uarte.stalled = true;
	fake_uarte.abort_stuck = true;

	TEST_ASSERT_EQUAL(-ETIMEDOUT, trace_backend.write(trace_data, 2 * BOUNCE_BUF_SZ));
	TEST_ASSERT_EQUAL(1, fake_uarte.sync_aborts);

	fake_uarte.stalled = false;
	fake_uarte.abort_stuck = false;
	backend_reinit();
	reset_counters();

	ret = trace_backend.write(trace_data, 3 * BOUNCE_BUF_SZ);
	assert_write_complete(ret, 3 * BOUNCE_BUF_SZ, BOUNCE_BUF_SZ);
#else
	TEST_IGNORE_MESSAGE("Bounce buffers are disabled");
#endif
}

void test_write_not_dma_accessible(void)
{
#if BOUNCE_BUF_SZ == 0
	backend_init();
	make_trace_data_inaccessible();

	TEST_ASSERT_EQUAL(-EFAULT, trace_backend.write(trace_data, CHUNK_SZ));
	TEST_ASSERT_EQUAL(0, fake_uarte.tx_calls);
	TEST_ASSERT_EQUAL(0, processed.calls);
#else
	TEST_IGNORE_MESSAGE("Bounce buffers are enabled");
#endif
}

int main(void)
{
	(void)unity_main();
	return 0;
}
