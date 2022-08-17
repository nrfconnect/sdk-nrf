/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/nrf_modem_lib_trace.h>
#include <modem/trace_backend.h>
#include <nrf_modem.h>
#include <nrf_modem_at.h>
#include <nrf_modem_os.h>
#include <nrf_modem_trace.h>
#include <nrf_errno.h>

LOG_MODULE_REGISTER(nrf_modem_lib_trace, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

NRF_MODEM_LIB_ON_INIT(trace_init, trace_init_callback, NULL);

K_SEM_DEFINE(trace_sem, 0, 1);
K_SEM_DEFINE(trace_done_sem, 1, 1);

#define TRACE_THREAD_PRIORITY                                                                      \
	COND_CODE_1(CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PRIO_OVERRIDE,                               \
		    (CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PRIO), (K_LOWEST_APPLICATION_THREAD_PRIO))

static int trace_init(void);
static int trace_deinit(void);

#if CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE
static uint32_t backend_bps_avg;
static uint32_t backend_bps_tot;
static uint32_t backend_bps_samples;
static int64_t backend_measurement_start;

#define BACKEND_BPS_AVG_UPDATE_PERIOD K_MSEC(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_PERIOD_MS)

static void backend_bps_reset(void)
{
	backend_bps_tot = 0;
	backend_bps_samples = 0;
}

static void backend_bps_update(int bps)
{
	backend_bps_tot += bps;
	backend_bps_samples++;
}

static void backend_bps_avg_update(struct k_work *item);

K_WORK_DELAYABLE_DEFINE(backend_bps_avg_update_work, backend_bps_avg_update);

static void backend_bps_avg_update(struct k_work *item)
{
	if (backend_bps_samples != 0) {
		backend_bps_avg = backend_bps_tot / backend_bps_samples;
	} else {
		/* With 0 samples the average is 0 */
		backend_bps_avg = 0;
	}

	backend_bps_reset();

	k_work_schedule(&backend_bps_avg_update_work, BACKEND_BPS_AVG_UPDATE_PERIOD);
}

uint32_t nrf_modem_lib_trace_backend_bitrate_get(void)
{
	return backend_bps_avg;
}

static void trace_backend_bitrate_perf_start(void)
{
	backend_measurement_start = k_uptime_ticks();
}

static void trace_backend_bitrate_perf_end(int size)
{
	int64_t delta;
	uint32_t bps;

	delta = k_uptime_ticks() - backend_measurement_start;

	if (size > 0 && delta > 0) {
		bps = size * 8 * CONFIG_SYS_CLOCK_TICKS_PER_SEC / delta;
		backend_bps_update(bps);
	}
}

#define PERF_START trace_backend_bitrate_perf_start
#define PERF_END(size) trace_backend_bitrate_perf_end(size)
#else
#define PERF_START(...)
#define PERF_END(...)
#endif

#if CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_LOG
#define BACKEND_BPS_LOG_PERIOD K_MSEC(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_LOG_PERIOD_MS)

static void backend_bps_log(struct k_work *item);

K_WORK_DELAYABLE_DEFINE(backend_bps_log_work, backend_bps_log);

static void backend_bps_log(struct k_work *item)
{
	LOG_INF("Trace backend bitrate (bps): %u", backend_bps_avg);

	k_work_schedule(&backend_bps_log_work, BACKEND_BPS_LOG_PERIOD);
}
#endif

#if CONFIG_NRF_MODEM_LIB_TRACE_BITRATE_LOG
#define BPS_LOG_PERIOD_MS CONFIG_NRF_MODEM_LIB_TRACE_BITRATE_LOG_PERIOD_MS
#define BPS_LOG_PERIOD K_MSEC(BPS_LOG_PERIOD_MS)

static uint32_t trace_bytes_received;

static void update_trace_bytes_received(struct nrf_modem_trace_data *frags, size_t n_frags)
{
	for (size_t i = 0; i < n_frags; i++) {
		trace_bytes_received += frags[i].len;
	}
}

static void bps_log(struct k_work *item);

K_WORK_DELAYABLE_DEFINE(bps_log_work, bps_log);

static void bps_log(struct k_work *item)
{
	uint32_t data_bits = trace_bytes_received * 8;
	uint32_t trace_data_bps_avg = data_bits * 1000 / BPS_LOG_PERIOD_MS;

	trace_bytes_received = 0;

	LOG_INF("Trace bitrate (bps): %u", trace_data_bps_avg);

	k_work_schedule(&bps_log_work, BPS_LOG_PERIOD);
}

#define UPDATE_TRACE_BYTES_RECEIVED(frags, n_frags) update_trace_bytes_received(frags, n_frags)
#else
#define UPDATE_TRACE_BYTES_RECEIVED(...)
#endif

int nrf_modem_lib_trace_processing_done_wait(k_timeout_t timeout)
{
	int err;

	err = k_sem_take(&trace_done_sem, timeout);
	if (err) {
		return err;
	}

	k_sem_give(&trace_done_sem);

	return 0;
}

static int trace_fragment_write(struct nrf_modem_trace_data *frag)
{
	int ret;
	size_t remaining = frag->len;

	while (remaining) {
		PERF_START();

		ret = trace_backend_write((void *)((uint8_t *)frag->data + frag->len - remaining),
					  remaining);

		PERF_END(ret);

		if (ret < 0) {
			LOG_ERR("trace_backend_write failed with err: %d", ret);

			return ret;
		}

		if (ret == 0) {
			LOG_WRN("trace_backend_write wrote 0 bytes.");
		}

		remaining -= ret;
	}

	return 0;
}

void trace_thread_handler(void)
{
	int err;
	struct nrf_modem_trace_data *frags;
	size_t n_frags;

trace_reset:
	k_sem_take(&trace_sem, K_FOREVER);

	while (true) {
		err = nrf_modem_trace_get(&frags, &n_frags, NRF_MODEM_OS_FOREVER);
		switch (err) {
		case 0:
			/* Success */
			UPDATE_TRACE_BYTES_RECEIVED(frags, n_frags);
			break;
		case -NRF_ESHUTDOWN:
			LOG_INF("Modem was turned off, no more traces");
			goto out;
		case -NRF_ENODATA:
			LOG_INF("No more trace data");
			goto out;
		case -NRF_EINPROGRESS:
			__ASSERT(0, "Error in transport backend");
			goto out;
		default:
			__ASSERT(0, "Unhandled err %d", err);
			goto out;
		}

		for (size_t i = 0; i < n_frags; i++) {
			err = trace_fragment_write(&frags[i]);
			if (err) {
				goto out;
			}
		}
	}

out:
	err = trace_deinit();
	if (err) {
		LOG_ERR("trace_deinit failed with err: %d", err);
	}

	goto trace_reset;
}

static int trace_init(void)
{
	int err;

	k_sem_take(&trace_done_sem, K_FOREVER);

	err = trace_backend_init(nrf_modem_trace_processed);
	if (err) {
		LOG_ERR("trace_backend_init failed with err: %d", err);

		return err;
	}

	LOG_INF("Trace thread ready");
	k_sem_give(&trace_sem);

#if CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE
	k_work_schedule(&backend_bps_avg_update_work, BACKEND_BPS_AVG_UPDATE_PERIOD);
#endif

#if CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_LOG
	k_work_schedule(&backend_bps_log_work, BACKEND_BPS_LOG_PERIOD);
#endif

#if CONFIG_NRF_MODEM_LIB_TRACE_BITRATE_LOG
	k_work_schedule(&bps_log_work, BPS_LOG_PERIOD);
#endif

	return 0;
}

static void trace_init_callback(int err, void *ctx)
{
	if (err) {
		return;
	}

	err = trace_init();
	if (err) {
		LOG_ERR("Failed to initialize trace backend, err: %d", err);
		return;
	}

#if CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_OVERRIDE
	err = nrf_modem_lib_trace_level_set(CONFIG_NRF_MODEM_LIB_TRACE_LEVEL);
	LOG_INF("Trace level override: %d", CONFIG_NRF_MODEM_LIB_TRACE_LEVEL);
	if (err) {
		LOG_ERR("Failed to start tracing, err %d", err);
	}
#else
	LOG_INF("Trace level untouched");
#endif
}

static int trace_deinit(void)
{
	int err;

	err = trace_backend_deinit();
	if (err) {
		LOG_ERR("trace_backend_deinit failed with err: %d", err);

		return err;
	}

	k_sem_give(&trace_done_sem);

	return 0;
}

int nrf_modem_lib_trace_level_set(enum nrf_modem_lib_trace_level trace_level)
{
	int err;
	/* Casting to integer to remove any assumptions on the type of the enum
	 * (could be `char` or `int`) when `printf` expects exactly `int`.
	 */
	int tl = trace_level;

	if (tl) {
		err = nrf_modem_at_printf("AT%%XMODEMTRACE=1,%d", tl);
	} else {
		err = nrf_modem_at_printf("AT%%XMODEMTRACE=0");
	}

	if (err) {
		LOG_ERR("Failed to set trace level, err: %d", err);
		return -ENOEXEC;
	}

	return 0;
}

K_THREAD_DEFINE(trace_thread, CONFIG_NRF_MODEM_LIB_TRACE_STACK_SIZE, trace_thread_handler,
	       NULL, NULL, NULL, TRACE_THREAD_PRIORITY, 0, 0);
