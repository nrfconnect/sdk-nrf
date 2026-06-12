/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Zephyr-side public API of the app-domain RT framework.
 *
 * This is the control-plane interface that Zephyr-managed code uses to
 * configure the RT component. All calls are non-blocking: they only write to
 * the shared control block and never touch the RT-owned peripherals directly.
 *
 * Thread-safety: the rt_fw_*() functions serialize internally with a mutex, so
 * they may be called from multiple Zephyr threads. They must NOT be called from
 * interrupt context (they take a mutex and are not part of the RT fast path).
 *
 * Command latency: a command becomes effective on the next RT tick. Because the
 * RT period is also the control-plane polling interval, the worst-case latency
 * is one period, which is why the accepted period range is bounded (see
 * rt_fw_set_period_us()).
 */

#ifndef RT_FRAMEWORK_H_
#define RT_FRAMEWORK_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Accepted RT toggle period range, in microseconds. Values outside this range
 * are rejected by rt_fw_set_period_us().
 */
#define RT_FW_MIN_PERIOD_US CONFIG_RT_FW_MIN_PERIOD_US
#define RT_FW_MAX_PERIOD_US CONFIG_RT_FW_MAX_PERIOD_US

/* Snapshot of the RT component state, as observed by Zephyr.
 *
 * The "applied"/effective fields (enabled, period_*) are owned and updated by
 * the RT ISR and reflect the hardware state. The "req_" fields are what Zephyr
 * last requested. While pending is true the request has not been applied yet.
 */
struct rt_fw_status {
	/* Applied / in effect (RT-owned). */
	bool     enabled;
	uint32_t period_us;
	uint32_t period_ticks;

	/* Requested by Zephyr (may differ from applied while pending). */
	bool     req_enabled;
	uint32_t req_period_us;
	uint32_t req_period_ticks;

	/* Handshake / telemetry. */
	bool     pending;        /* command_seq != ack_seq */
	uint32_t command_seq;
	uint32_t ack_seq;
	uint32_t tick_count;
	uint32_t dropped_events; /* data-plane ring overflows */
	uint32_t last_error;
};

/*
 * Initialize the RT component: program TIMER, install the zero-latency ISR
 * and (optionally) set up the EGU data plane. The RT execution context is
 * alive after this call. GPIO toggling is gated by rt_fw_enable().
 */
int rt_fw_init(void);

/* Enable/disable GPIO toggling in the RT path (control plane). */
int rt_fw_enable(bool enable);

/*
 * Set the RT toggle period in microseconds (control plane). The value must be
 * within [RT_FW_MIN_PERIOD_US, RT_FW_MAX_PERIOD_US]. Out-of-range values are
 * rejected with -EINVAL. The change takes effect within one RT period.
 */
int rt_fw_set_period_us(uint32_t period_us);

/* Read back the current RT status/telemetry. */
int rt_fw_get_status(struct rt_fw_status *status);

#if defined(CONFIG_RT_FW_DATA_PLANE)
/* Data-plane callback, invoked from the Zephyr system workqueue. */
typedef void (*rt_fw_event_cb_t)(uint32_t type, uint32_t value, uint32_t timestamp);

/* Register a callback that receives events produced by the RT path. */
int rt_fw_register_event_cb(rt_fw_event_cb_t cb);
#endif

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_H_ */
