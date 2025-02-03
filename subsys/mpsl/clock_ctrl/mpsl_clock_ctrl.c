/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_CLOCK_CONTROL_NRF)
#include <nrfx_clock.h>
#endif /* CONFIG_CLOCK_CONTROL_NRF */

#include <mpsl_clock.h>
#include "mpsl_clock_ctrl.h"

LOG_MODULE_REGISTER(mpsl_clock_ctrl, CONFIG_MPSL_LOG_LEVEL);

/* Variable shared for nrf and nrf2 clock control */
static atomic_t m_hfclk_refcnt;

/* Type use to get information about a clock request status */
struct clock_onoff_state {
	struct onoff_client cli;
	atomic_t m_clk_ready;
	atomic_t m_clk_refcnt;
	struct k_sem sem;
	int clk_req_rsp;
};

static struct clock_onoff_state m_lfclk_state;

static int32_t m_lfclk_release(void);

#if defined(CONFIG_CLOCK_CONTROL_NRF2_NRFS_CLOCK_TIMEOUT_MS)
#define MPSL_LFCLK_REQUEST_WAIT_TIMEOUT_MS CONFIG_CLOCK_CONTROL_NRF2_NRFS_CLOCK_TIMEOUT_MS
#else
#define MPSL_LFCLK_REQUEST_WAIT_TIMEOUT_MS 1000
#endif /* CONFIG_CLOCK_CONTROL_NRF2_NRFS_CLOCK_TIMEOUT_MS */

/** @brief LFCLK request callback.
 *
 * The callback function provided to clock control to notify about LFCLK request being finished.
 */
static void lfclk_request_cb(struct onoff_manager *mgr, struct onoff_client *cli, uint32_t state,
			     int res)
{
	struct clock_onoff_state *clock_state = CONTAINER_OF(cli, struct clock_onoff_state, cli);

	clock_state->clk_req_rsp = res;
	k_sem_give(&clock_state->sem);
}

/** @brief Wait for LFCLK to be ready.
 *
 * The function can time out if there is no response from clock control drvier until
 * MPSL_LFCLK_REQUEST_WAIT_TIMEOUT_MS.
 *
 * @note For nRF54H SoC series waiting for LFCLK can't block the system work queue. The nrf2 clock
 *       control driver can return -TIMEDOUT due not handled response from sysctrl.
 *
 * @retval 0 LFCLK is ready.
 * @retval -NRF_EINVAL There were no LFCLK request.
 * @retval -NRF_EFAULT LFCLK request failed.
 */
static int32_t m_lfclk_wait(void)
{
	int32_t err;

	if (atomic_get(&m_lfclk_state.m_clk_ready) == (atomic_val_t) true) {
		return 0;
	}

	/* Check if lfclk has been requested */
	if (atomic_get(&m_lfclk_state.m_clk_refcnt) <= (atomic_val_t)0) {
		return -NRF_EINVAL;
	}

	/* Wait for response from clock control */
	err = k_sem_take(&m_lfclk_state.sem, K_MSEC(MPSL_LFCLK_REQUEST_WAIT_TIMEOUT_MS));
	if (err < 0) {
		/* Do a gracefull cancel of the request, the function release does this
		 * as well as and relase.
		 */
		(void)m_lfclk_release();

		return -NRF_EFAULT;
	}

	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF2) && m_lfclk_state.clk_req_rsp == -ETIMEDOUT) {
		/* Due to NCSDK-31169, temporarily allow for LFCLK request to timeout.
		 * That doens't break anything now because the LFCLK requested clock is
		 * 500PPM and such LFCLK should be running from boot of the radio core.
		 */
		LOG_WRN("LFCLK could not be started: %d", m_lfclk_state.clk_req_rsp);
		return 0;
	} else if (m_lfclk_state.clk_req_rsp < 0) {
		__ASSERT(false, "LFCLK could not be started, reason: %d",
			 m_lfclk_state.clk_req_rsp);
		/* Possible failure reasons:
		 *  # -ERRTIMEDOUT - nRFS service timeout
		 *  # -EIO - nRFS service error
		 *  # -ENXIO - request rejected
		 * All these mean failure for MPSL.
		 */
		return -NRF_EFAULT;
	}

	atomic_set(&m_lfclk_state.m_clk_ready, (atomic_val_t) true);

	return 0;
}

#if defined(CONFIG_CLOCK_CONTROL_NRF)

static void m_hfclk_request(void)
{
	/* The z_nrf_clock_bt_ctlr_hf_request doesn't count references to HFCLK,
	 * it is caller responsibility handle requests and releases counting.
	 */
	if (atomic_inc(&m_hfclk_refcnt) > (atomic_val_t)0) {
		return;
	}

	z_nrf_clock_bt_ctlr_hf_request();
}

static void m_hfclk_release(void)
{
	/* The z_nrf_clock_bt_ctlr_hf_request doesn't count references to HFCLK,
	 * it is caller responsibility to not release the clock if there is
	 * other request pending.
	 */
	if (atomic_get(&m_hfclk_refcnt) < (atomic_val_t)1) {
		LOG_WRN("Mismatch between HFCLK request/release");
		return;
	}

	if (atomic_dec(&m_hfclk_refcnt) > (atomic_val_t)1) {
		return;
	}

	z_nrf_clock_bt_ctlr_hf_release();
}

static bool m_hfclk_is_running(void)
{
	if (atomic_get(&m_hfclk_refcnt) > (atomic_val_t)0) {
		nrf_clock_hfclk_t type;

		unsigned int key = irq_lock();

		(void)nrfx_clock_is_running(NRF_CLOCK_DOMAIN_HFCLK, &type);

		irq_unlock(key);

		return ((type == NRF_CLOCK_HFCLK_HIGH_ACCURACY) ? true : false);
	}

	return false;
}

static void m_lfclk_calibration_start(void)
{
	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_DRIVER_CALIBRATION)) {
		z_nrf_clock_calibration_force_start();
	}
}

static bool m_lfclk_calibration_is_enabled(void)
{
	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_DRIVER_CALIBRATION)) {
		return true;
	} else {
		return false;
	}
}

static int32_t m_lfclk_request(void)
{
	struct onoff_manager *mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);
	int32_t err;

	/* Workaround for NRFX-6865. The nrf clock control as well as nrfx_clock doesn't enable
	 * HFXO when LFSYNTH is selected as LFCLK source. Remove the code when nrfx is fixed.
	 */
	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH)) {
		m_hfclk_request();
	}

	sys_notify_init_callback(&m_lfclk_state.cli.notify, lfclk_request_cb);
	(void)k_sem_init(&m_lfclk_state.sem, 0, 1);

	err = onoff_request(mgr, &m_lfclk_state.cli);
	if (err < 0) {
		return err;
	}

	atomic_inc(&m_lfclk_state.m_clk_refcnt);

	return 0;
}

static int32_t m_lfclk_release(void)
{
	struct onoff_manager *mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);
	int32_t err;

	/* In case there is other ongoing request, cancel it. */
	err = onoff_cancel_or_release(mgr, &m_lfclk_state.cli);
	if (err < 0) {
		return err;
	}

	/* Workaround for NRFX-6865. The nrf clock control as well as nrfx_clock doesn't enable
	 * HFXO when LFSYNTH is selected as LFCLK source. Remove the code when nrfx is fixed.
	 */
	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH)) {
		m_hfclk_release();
	}

	atomic_dec(&m_lfclk_state.m_clk_refcnt);

	return 0;
}

#elif defined(CONFIG_CLOCK_CONTROL_NRF2)

/* Temporary macro because there is no system level configuration of LFCLK source and its accuracy
 * for nRF54H SoC series. What more, there is no API to retrieve the information about accuracy of
 * available LFCLK.
 */
#define MPSL_LFCLK_ACCURACY_PPM 500

static const struct nrf_clock_spec m_lfclk_specs = {
	.frequency = 32768,
	.accuracy = MPSL_LFCLK_ACCURACY_PPM,
	/* This affects selected LFCLK source. It doesn't switch to higher accuracy but selects more
	 * precise but current hungry lfclk source.
	 */
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

static void m_lfclk_calibration_start(void)
{
	/* This function is not supported when CONFIG_CLOCK_CONTROL_NRF2 is set.
	 * As of now MPSL does not use this API in this configuration.
	 */
	__ASSERT_NO_MSG(false);
}

static bool m_lfclk_calibration_is_enabled(void)
{
	/* This function should not be called from MPSL if CONFIG_CLOCK_CONTROL_NRF2 is set */
	__ASSERT_NO_MSG(false);
	return false;
}

static int32_t m_lfclk_request(void)
{
	const struct device *lfclk_dev = DEVICE_DT_GET(DT_NODELABEL(lfclk));
	int err;

	sys_notify_init_callback(&m_lfclk_state.cli.notify, lfclk_request_cb);
	k_sem_init(&m_lfclk_state.sem, 0, 1);

	err = nrf_clock_control_request(lfclk_dev, &m_lfclk_specs, &m_lfclk_state.cli);
	if (err < 0) {
		return err;
	}

	atomic_inc(&m_lfclk_state.m_clk_refcnt);

	return 0;
}

static int32_t m_lfclk_release(void)
{
	const struct device *lfclk_dev = DEVICE_DT_GET(DT_NODELABEL(lfclk));
	int err;

	err = nrf_clock_control_cancel_or_release(lfclk_dev, &m_lfclk_specs, &m_lfclk_state.cli);
	if (err < 0) {
		return err;
	}

	atomic_dec(&m_lfclk_state.m_clk_refcnt);

	return 0;
}

static void m_hfclk_request(void)
{
	if (atomic_inc(&m_hfclk_refcnt) > (atomic_val_t)0) {
		return;
	}

	nrf_clock_control_hfxo_request();
}

static void m_hfclk_release(void)
{
	if (atomic_get(&m_hfclk_refcnt) < (atomic_val_t)1) {
		LOG_WRN("Mismatch between HFCLK request/release");
		return;
	}

	if (atomic_dec(&m_hfclk_refcnt) > (atomic_val_t)1) {
		return;
	}

	nrf_clock_control_hfxo_release();
}

static bool m_hfclk_is_running(void)
{
	/* As of now assume the HFCLK is running after the request was put.
	 * This puts the responsibility to the user to check if the time
	 * since last request is larger than the HFXO rampup time.
	 */
	if (atomic_get(&m_hfclk_refcnt) < (atomic_val_t)1) {
		return false;
	}

	return true;
}
#else
#error "Unsupported clock control"
#endif /* CONFIG_CLOCK_CONTROL_NRF */

static mpsl_clock_lfclk_ctrl_source_t m_nrf_lfclk_ctrl_data = {
	.lfclk_wait = m_lfclk_wait,
	.lfclk_calibration_start = m_lfclk_calibration_start,
	.lfclk_calibration_is_enabled = m_lfclk_calibration_is_enabled,
	.lfclk_request = m_lfclk_request,
	.lfclk_release = m_lfclk_release,
#if defined(CONFIG_CLOCK_CONTROL_NRF_ACCURACY)
	.accuracy_ppm = CONFIG_CLOCK_CONTROL_NRF_ACCURACY,
#else
	.accuracy_ppm = MPSL_LFCLK_ACCURACY_PPM,
#endif /* CONFIG_CLOCK_CONTROL_NRF_ACCURACY */
	.skip_wait_lfclk_started = IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT)
};

static mpsl_clock_hfclk_ctrl_source_t m_nrf_hfclk_ctrl_data = {
	.hfclk_request = m_hfclk_request,
	.hfclk_release = m_hfclk_release,
	.hfclk_is_running = m_hfclk_is_running,
	.startup_time_us = CONFIG_MPSL_HFCLK_LATENCY
};

int32_t mpsl_clock_ctrl_init(void)
{
	return mpsl_clock_ctrl_source_register(&m_nrf_lfclk_ctrl_data, &m_nrf_hfclk_ctrl_data);
}

int32_t mpsl_clock_ctrl_uninit(void)
{
	return mpsl_clock_ctrl_source_unregister();
}
