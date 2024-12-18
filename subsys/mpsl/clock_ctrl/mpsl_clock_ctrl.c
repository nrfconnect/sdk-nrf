/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#include <mpsl_clock.h>
#include "mpsl_clock_ctrl.h"

/* Variable shared for nrf and nrf2 clock control */
static atomic_val_t m_hfclk_refcnt;

/* Type use to get information about a clock request status */
struct clock_onoff_state {
	struct onoff_client cli;
	atomic_t m_clk_ready;
	atomic_val_t m_clk_refcnt;
};

static struct clock_onoff_state m_lfclk_state;

/** @brief Wait for LFCLK to be ready.
 *
 * In case of nrf clock control driver the function does blcoking wait until the LFLCK is ready.
 * In case of nrf2 clock control driver the function can time out in case there is no response
 * from remote clock controller.
 *
 * @note For nRF54H SoC series waiting for LFCLK can't block the system work queue. In such case
 *       the nrf2 clock control driver could return -TIMEDOUT due to lack of response from sysctrl.
 *
 * @retval 0 LFCLK is ready.
 * @retval -NRF_EINVAL There were no LFCLK request.
 * @retval -NRF_ETIMEDOUT LFCLK wait timed out.
 */
static int32_t m_lfclk_wait(void)
{
	int32_t err;
	int32_t res;

	if (m_lfclk_state.m_clk_ready == true) {
		return 0;
	}

	/* Check if lfclk was already requested */
	if (m_lfclk_state.m_clk_refcnt <= 0) {
		return -NRF_EINVAL;
	}

	do {
		err = sys_notify_fetch_result(&m_lfclk_state.cli.notify, &res);
		if (!err && res) {
			__ASSERT(false, "LFCLK could not be started: %d", res);
			/* Translate -ERRTIMEDOUT to NRF counterpart */
			return -NRF_ETIMEDOUT;
		}
		/* The function is executed in POST_KERNEL hence sleep is allowed */
		k_msleep(1);
	} while (err == -EAGAIN);

	/* In case there is some other error. */
	if (err < 0) {
		return err;
	}

	atomic_set(&m_lfclk_state.m_clk_ready, true);

	return 0;
}

#if defined(CONFIG_CLOCK_CONTROL_NRF)

static void m_hfclk_request(void)
{
	/* The z_nrf_clock_bt_ctlr_hf_request doesn't count references to HFCLK,
	 * it is caller responsibility handle requests and releases counting.
	 */
	if (atomic_inc(&m_hfclk_refcnt) > 0) {
		return;
	}

	z_nrf_clock_bt_ctlr_hf_request();
}

static void m_hfclk_release(void)
{
	/* The z_nrf_clock_bt_ctlr_hf_request doesn't count references to HFCLK,
	 * it is caller responsibility to do not release the clock if there is
	 * other request pending.
	 */
	if (m_hfclk_refcnt < 1) {
		return;
	}

	if (atomic_dec(&m_hfclk_refcnt) > 1) {
		return;
	}

	z_nrf_clock_bt_ctlr_hf_release();
}

static bool m_hfclk_is_running(void)
{
	/* As of now assume the HFCLK is runnig after the request was put */
	return ((m_hfclk_refcnt > 0) ? true : false);
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
		return z_nrf_clock_calibration_is_in_progress();
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

	sys_notify_init_spinwait(&m_lfclk_state.cli.notify);
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

	/* In case there is other ongoing request, cancell it. */
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
 * for nRF54H SoC series. What more there is no API to retrieve the information about accuracy of
 * configured LFCLK.
 */
#define MPSL_LFCLK_ACCURACY_PPM 500

static const struct nrf_clock_spec m_lfclk_specs = {
	.frequency = 32768,
	.accuracy = MPSL_LFCLK_ACCURACY_PPM,
	/* This affects selected LFCLK source. It doesn't switch to higher accuracy but selects more
	 * current hungry lfclk source.
	 */
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

static void m_lfclk_calibration_start(void)
{
	/* This function should not be called from MPSL if CONFIG_CLOCK_CONTROL_NRF2 is set */
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

	sys_notify_init_spinwait(&m_lfclk_state.cli.notify);

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
	if (atomic_inc(&m_hfclk_refcnt) > 0) {
		return;
	}

	nrf_clock_control_hfxo_request();
}

static void m_hfclk_release(void)
{
	if (m_hfclk_refcnt < 1) {
		return;
	}

	if (atomic_dec(&m_hfclk_refcnt) > 1) {
		return;
	}

	nrf_clock_control_hfxo_release();
}

static bool m_hfclk_is_running(void)
{
	if (m_hfclk_refcnt < 1) {
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
	.lfclk_release = m_lfclk_release
};

static mpsl_clock_hfclk_ctrl_source_t m_nrf_hfclk_ctrl_data = {
	.hfclk_request = m_hfclk_request,
	.hfclk_release = m_hfclk_release,
	.hfclk_is_running = m_hfclk_is_running
};

int32_t mpsl_clock_ctrl_init(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF_ACCURACY)
	m_nrf_lfclk_ctrl_data.accuracy_ppm = CONFIG_CLOCK_CONTROL_NRF_ACCURACY;
#else
	m_nrf_lfclk_ctrl_data.accuracy_ppm = MPSL_LFCLK_ACCURACY_PPM;
#endif /* CONFIG_CLOCK_CONTROL_NRF_ACCURACY */
	m_nrf_lfclk_ctrl_data.skip_wait_lfclk_started = IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT);
	m_nrf_hfclk_ctrl_data.startup_time_us = CONFIG_MPSL_HFCLK_LATENCY;

	return mpsl_clock_ctrl_source_register(&m_nrf_lfclk_ctrl_data, &m_nrf_hfclk_ctrl_data);
}

int32_t mpsl_clock_ctrl_uninit(void)
{
	return mpsl_clock_ctrl_source_unregister();
}
