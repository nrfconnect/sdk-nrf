/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdbool.h>
#include <sys/types.h>
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

K_SEM_DEFINE(trace_sem, 0, 1);
K_SEM_DEFINE(trace_clear_sem, 0, 1);
K_SEM_DEFINE(trace_done_sem, 1, 1);

extern struct nrf_modem_lib_trace_backend trace_backend;
static bool has_space = true;

#define TRACE_THREAD_PRIORITY                                                                      \
	COND_CODE_1(CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PRIO_OVERRIDE,                               \
		    (CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PRIO), (K_LOWEST_APPLICATION_THREAD_PRIO))

static int trace_init(void);
static int trace_deinit(void);

static bool backend_suspended;
static void backend_suspend_handle(struct k_work *item);
K_WORK_DELAYABLE_DEFINE(backend_suspend_work, backend_suspend_handle);
#define BACKEND_SUSPEND_DELAY K_MSEC(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_SUSPEND_DELAY_MS)

static void backend_suspend(void)
{
	int err;

	if (backend_suspended || !trace_backend.suspend) {
		return;
	}

	err = trace_backend.suspend();
	if (err) {
		LOG_ERR("Could not suspend trace backend");
	}

	backend_suspended = true;
}


static void backend_resume(void)
{
	int err;

	if (!backend_suspended || !trace_backend.resume) {
		return;
	}

	err = trace_backend.resume();
	if (err) {
		LOG_ERR("Could not resume trace backend");
	}

	backend_suspended = false;
}

static void backend_suspend_handle(struct k_work *item)
{
	backend_suspend();
}


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

__weak void nrf_modem_lib_trace_callback(enum nrf_modem_lib_trace_event evt)
{
	LOG_ERR("%s was called with event %d but no callback is set", __func__, evt);
}

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

static size_t trace_bytes_received;
static size_t trace_bytes_received_total;
static size_t trace_bytes_read_total;

static void update_trace_bytes_received(struct nrf_modem_trace_data *frags, size_t n_frags)
{
	for (size_t i = 0; i < n_frags; i++) {
		trace_bytes_received += frags[i].len;
		trace_bytes_received_total += frags[i].len;
	}
}

static void update_trace_bytes_read(size_t bytes)
{
	trace_bytes_read_total += bytes;
}

static void bps_log(struct k_work *item);

K_WORK_DELAYABLE_DEFINE(bps_log_work, bps_log);

static void bps_log(struct k_work *item)
{
	uint32_t data_bits = trace_bytes_received * 8;
	uint32_t trace_data_bps_avg = data_bits * 1000 / BPS_LOG_PERIOD_MS;

	trace_bytes_received = 0;

	LOG_INF("Written: %d, read: %d", trace_bytes_received_total, trace_bytes_read_total);
	LOG_INF("Trace bitrate (bps): %u", trace_data_bps_avg);

	k_work_schedule(&bps_log_work, BPS_LOG_PERIOD);
}

#define UPDATE_TRACE_BYTES_RECEIVED(frags, n_frags) update_trace_bytes_received(frags, n_frags)
#define UPDATE_TRACE_BYTES_READ(bytes) update_trace_bytes_read(bytes)
#else
#define UPDATE_TRACE_BYTES_RECEIVED(...)
#define UPDATE_TRACE_BYTES_READ(...)
#endif

int nrf_modem_lib_trace_processing_done_wait(k_timeout_t timeout)
{
	int err;

	if (!has_space) {
		return -ENOSPC;
	}

	err = k_sem_take(&trace_done_sem, timeout);
	if (err) {
		return err;
	}

	k_sem_give(&trace_done_sem);

	if (!has_space) {
		return -ENOSPC;
	}

	return 0;
}

static int trace_fragment_write(struct nrf_modem_trace_data *frag)
{
	int ret;
	size_t remaining = frag->len;

	while (remaining) {
		PERF_START();

		ret = trace_backend.write((void *)((uint8_t *)frag->data + frag->len - remaining),
					  remaining);

		PERF_END(ret);

		__ASSERT(ret != 0, "Trace backend wrote 0 bytes");

		/* We handle this here and not in the trace_thread_handler as we might not write the
		 * entire trace fragment in the same write operation. If we get EAGAIN on the
		 * second, third, ..., we would repeat sending the first section of the trace
		 * fragment.
		 */
		if (ret == -EAGAIN) {
			/* We don't allow retrying if the modem is shut down as that can block
			 * a new modem init.
			 */
			if (!nrf_modem_is_initialized()) {
				return -ESHUTDOWN;
			}
			continue;
		}

		if (ret < 0) {
			LOG_ERR("trace_backend.write failed with err: %d", ret);
			return ret;
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
		if (trace_backend.suspend) {
			k_work_schedule(&backend_suspend_work, BACKEND_SUSPEND_DELAY);
		}

		err = nrf_modem_trace_get(&frags, &n_frags, NRF_MODEM_OS_FOREVER);
		if (trace_backend.suspend) {
			k_work_cancel_delayable(&backend_suspend_work);
		}
		switch (err) {
		case 0:
			/* Success */
			UPDATE_TRACE_BYTES_RECEIVED(frags, n_frags);
			break;
		case -NRF_ESHUTDOWN:
			LOG_INF("Modem was turned off, no more traces");
			goto deinit;
		case -NRF_ENODATA:
			LOG_INF("No more trace data");
			goto deinit;
		case -NRF_EINPROGRESS:
			__ASSERT(0, "Error in transport backend");
			goto deinit;
		default:
			__ASSERT(0, "Unhandled err %d", err);
			goto deinit;
		}

		if (backend_suspended) {
			backend_resume();
		}

		for (int i = 0; i < n_frags; i++) {
			err = trace_fragment_write(&frags[i]);
			switch (err) {
			case 0:
				break;
			case -ENOSPC:
				nrf_modem_lib_trace_callback(NRF_MODEM_LIB_TRACE_EVT_FULL);

				if (!trace_backend.clear) {
					goto deinit;
				}

				has_space = false;
				k_sem_give(&trace_done_sem);
				k_sem_take(&trace_clear_sem, K_FOREVER);
				/* Try the same fragment again */
				i--;
				continue;
			default:
				/* Irrecoverable error */
				goto deinit;
			}
		}
	}

deinit:
	err = trace_deinit();
	if (err) {
		LOG_ERR("trace_deinit failed with err: %d", err);
	}

	goto trace_reset;
}

static int trace_init(void)
{
	int err;

	if (!trace_backend.init || !trace_backend.deinit || !trace_backend.write) {
		LOG_ERR("trace backend must implement init, deinit and write");
		return -ENOTSUP;
	}

	k_sem_take(&trace_done_sem, K_FOREVER);

	err = trace_backend.init(nrf_modem_trace_processed);
	if (err) {
		LOG_ERR("trace_backend: init failed with err: %d", err);
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

void nrf_modem_lib_trace_init(void)
{
	int err;

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

	err = trace_backend.deinit();
	if (err) {
		LOG_ERR("trace_backend: deinit failed with err: %d", err);
		return err;
	}


#if CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE
	k_work_cancel(&backend_bps_avg_update_work);
#endif

#if CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_LOG
	k_work_cancel(&backend_bps_log_work);
#endif

#if CONFIG_NRF_MODEM_LIB_TRACE_BITRATE_LOG
	k_work_cancel(&bps_log_work);
#endif

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

size_t nrf_modem_lib_trace_data_size(void)
{
	if (!trace_backend.data_size) {
		return -ENOTSUP;
	}

	return trace_backend.data_size();
}

int nrf_modem_lib_trace_read(uint8_t *buf, size_t len)
{
	int read;

	if (!trace_backend.read) {
		return -ENOTSUP;
	}

	read = trace_backend.read(buf, len);
	if (read > 0) {
		UPDATE_TRACE_BYTES_READ(read);
	}

	return read;
}

int nrf_modem_lib_trace_clear(void)
{
	int err;

	if (!trace_backend.clear) {
		return -ENOTSUP;
	}

	err = trace_backend.clear();
	if (err) {
		return err;
	}

	if (!has_space) {
		k_sem_take(&trace_done_sem, K_FOREVER);
		has_space = true;
		k_sem_give(&trace_clear_sem);
	}

	return 0;
}

K_THREAD_DEFINE(trace_thread, CONFIG_NRF_MODEM_LIB_TRACE_STACK_SIZE, trace_thread_handler,
	       NULL, NULL, NULL, TRACE_THREAD_PRIORITY, 0, 0);
