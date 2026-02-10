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
#include "nrf_errno.h"

LOG_MODULE_REGISTER(mpsl_clock_ctrl, CONFIG_MPSL_LOG_LEVEL);

static int32_t m_lfclk_release(void);

/* Variable shared for nrf and nrf2 clock control */
static atomic_t m_hfclk_refcnt;

/* Type use to get information about a clock request status */
struct clock_onoff_state {
	struct onoff_client cli;
	atomic_t m_clk_ready;
	atomic_t m_clk_refcnt;
	struct k_sem sem;
	int clk_req_rsp;
	int32_t (*m_clock_request_release)(void);
};

static struct clock_onoff_state m_lfclk_state = {
	.m_clock_request_release = m_lfclk_release
};
#if defined(CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW)
/* Variable used temporarily until the NCSDK-31169 is fixed */
static atomic_t m_lfclk_req_timeout;
#endif /* CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW */

#if defined(CONFIG_MPSL_EXT_CLK_CTRL_NVM_CLOCK_REQUEST)
#define NVM_CLOCK_DT_LABEL DT_NODELABEL(hsfll120)

#define NVM_CLOCK_FREQUENCY_HIGHEST \
	DT_PROP_LAST(NVM_CLOCK_DT_LABEL, supported_clock_frequencies)

static const struct nrf_clock_spec m_nvm_clk_spec = {
	.frequency = NVM_CLOCK_FREQUENCY_HIGHEST,
};

static int32_t m_nvm_clock_release(void);

static struct clock_onoff_state m_nvm_clock_state = {
	.m_clock_request_release = m_nvm_clock_release
};
#endif /* CONFIG_MPSL_EXT_CLK_CTRL_NVM_CLOCK_REQUEST */

/** @brief A clock request callback.
 *
 * The callback function provided to clock control to notify about a clock request being finished.
 */
static void m_clock_request_cb(struct onoff_manager *mgr, struct onoff_client *cli, uint32_t state,
			       int res)
{
	struct clock_onoff_state *clock_state = CONTAINER_OF(cli, struct clock_onoff_state, cli);

	clock_state->clk_req_rsp = res;
	k_sem_give(&clock_state->sem);
}

/** @brief Wait for clock request to be completed to be ready.
 *
 * The function can time out if there is no response from clock control driver until provided
 * time out value.
 *
 * @note For nRF54H SoC series waiting for a clock can't block the system work queue. The nrf2 clock
 *       control driver can return -TIMEDOUT due not handled response from sysctrl.
 *
 * @retval 0 the clock is ready.
 * @retval -NRF_EINVAL There were no the clock request.
 * @retval -NRF_EFAULT the clock request failed.
 */
static int32_t m_clock_request_wait(struct clock_onoff_state *clock_state, uint32_t timeout_ms)
{
	int32_t err;

	if (atomic_get(&clock_state->m_clk_ready) == (atomic_val_t) true) {
		return 0;
	}

	/* Check if the clock has been requested */
	if (atomic_get(&clock_state->m_clk_refcnt) <= (atomic_val_t)0) {
		return -NRF_EINVAL;
	}

	/* Wait for response from clock control */
	err = k_sem_take(&clock_state->sem, K_MSEC(timeout_ms));
	if (err < 0) {
		/* Do a gracefull cancel of the request, the function release does this
		 * as well as and relase.
		 */
		(void)clock_state->m_clock_request_release();

		return -NRF_EFAULT;
	}

	if (clock_state->clk_req_rsp == -ETIMEDOUT) {
		/* Change of error code to stay consistent with other APIs. */
		return -NRF_ETIMEDOUT;
	} else if (clock_state->clk_req_rsp < 0) {
		__ASSERT(false, "The requested clock could not be started, reason: %d",
			 clock_state->clk_req_rsp);
		/* Possible failure reasons:
		 *  # -ERRTIMEDOUT - nRFS service timeout
		 *  # -EIO - nRFS service error
		 *  # -ENXIO - request rejected
		 * All these mean failure for MPSL.
		 */
		return -NRF_EFAULT;
	}

	atomic_set(&clock_state->m_clk_ready, (atomic_val_t) true);

	return 0;
}

/** @brief Wait for LFCLK to be ready.
 *
 * The function can time out if there is no response from clock control driver until
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
	int err;

#if defined(CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW)
	/* Do not attempt to wait for the LFCLK if there was a timeout reported
	 * for former requests. Due to NCSDK-31169, we allow for such case,
	 * assuming there is a LFCLK source selected by sysctrl with accuracy
	 * that meets radio protocols requirements.
	 */
	if (atomic_get(&m_lfclk_req_timeout) == (atomic_val_t) true) {
		return 0;
	}
#endif /* CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW */

	err = m_clock_request_wait(&m_lfclk_state,
				   CONFIG_MPSL_EXT_CLK_CTRL_CLOCK_REQUEST_WAIT_TIMEOUT_MS);
#if defined(CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW)
	if (err == -NRF_ETIMEDOUT) {
		/* Due to NCSDK-31169, temporarily allow for LFCLK request to timeout.
		 * That doens't break anything now because the LFCLK requested clock is
		 * 500PPM and such LFCLK should be running from boot of the radio core.
		 */
		atomic_set(&m_lfclk_req_timeout, (atomic_val_t) true);

		/* Decrement the internal reference counter because the m_lfclk_req_timeout
		 * takes priority in handling of future request and release calls until
		 * mpsl_clock_ctrl_uninit() is called.
		 */
		atomic_dec(&m_lfclk_state.m_clk_refcnt);

		LOG_WRN("LFCLK could not be started: %d", m_lfclk_state.clk_req_rsp);
		return 0;
	}
#endif /* CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW */

	return err;
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

	sys_notify_init_callback(&m_lfclk_state.cli.notify, m_clock_request_cb);
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

/* Minimum accuracy of LFCLK that is required by Bluetooth Core Specification Version 6.1, Vol 6,
 * Part B, Section 4.2.2.
 */
#define MPSL_LFCLK_ACCURACY_PPM 500
/* The variable holds actual LFCLK specification that is use in the system.
 * Enabled LFCLK at least matches the minimum Bluetooth sleep clock accuracy,
 * but it might be better.
 */
static struct nrf_clock_spec m_lfclk_specs;

/* Minimum allowed HFXO startup time in microseconds. Use conservative value that comes from BICR.
 *
 * The time is used by MPSL to request the clock before radio event start. If the time is too short
 * there will be no time run low priority thread to handle Zephyr PM subsystem configuration of
 * no-latency event and request the MRAM to be always on.
 */
#define MPSL_PM_HFCLK_MINIMUM_ALLOWED_STARTUP_TIME_US 850

#define HFCLK_LABEL DT_NODELABEL(hfxo)

#if DT_NODE_HAS_STATUS(HFCLK_LABEL, okay) && DT_NODE_HAS_COMPAT(HFCLK_LABEL, nordic_nrf54h_hfxo)

static const struct nrf_clock_spec m_hfclk_specs = {
	.frequency = DT_PROP(HFCLK_LABEL, clock_frequency),
	.accuracy = DT_PROP(HFCLK_LABEL, accuracy_ppm),
	/* In case of HFCLK the driver assumes the request is always for high precision. */
	.precision = NRF_CLOCK_CONTROL_PRECISION_HIGH,
};
#else
#error "A HFXO DTS instance not found"
#endif /* DT_NODE_HAS_STATUS(HFCLK_LABEL, okay) &&
	* DT_NODE_HAS_COMPAT(HFCLK_LABEL, nordic_nrf54h_hfxo) \
	*/

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

#if defined(CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW)
	/* Do not attempt request LFCLK again if there was a timeout resposne
	 * for past request. Due to NCSDK-31169, we allow for such case
	 * assuming there is a LFCLK source selected by sysctrl with accuracy
	 * that meets radio protocols requirements.
	 */
	if (atomic_get(&m_lfclk_req_timeout) == (atomic_val_t) true) {
		return 0;
	}
#endif /* CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW */

	sys_notify_init_callback(&m_lfclk_state.cli.notify, m_clock_request_cb);
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

#if defined(CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW)
	/* Do not attempt to release the LFCLK if there was a timeout reported
	 * for a request. The clock driver hasn't got a request registered
	 * because of the error.
	 * Due to NCSDK-31169, we allow for such case assuming there is
	 * a LFCLK source selected by sysctrl with accuracy that meets radio
	 * protocols requirements.
	 */
	if (atomic_get(&m_lfclk_req_timeout) == (atomic_val_t) true) {
		return 0;
	}
#endif /* CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW */

	err = nrf_clock_control_cancel_or_release(lfclk_dev, &m_lfclk_specs, &m_lfclk_state.cli);
	if (err < 0) {
		return err;
#if defined(CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW)
	} else if (err == ONOFF_STATE_OFF) {
		/* Due to NCSDK-31169, clear the variable only in case all request are released.
		 * This allows for full reinitialization.
		 */
		atomic_set(&m_lfclk_req_timeout, (atomic_val_t) false);
#endif /* CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW */
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

#if defined(CONFIG_MPSL_EXT_CLK_CTRL_NVM_CLOCK_REQUEST)

static int32_t m_nvm_clock_request(void)
{
	int err;

	/* The MRAM DTS node does not provide:
	 * - clock property that can be used to get the clock node,
	 * - power-domain property that could provide reference to the clock node.
	 *
	 * The clock must be referred explicitly. It makes the code less portable.
	 */
	const struct device *nvm_clock_dev = DEVICE_DT_GET(NVM_CLOCK_DT_LABEL);

	sys_notify_init_callback(&m_nvm_clock_state.cli.notify, m_clock_request_cb);
	k_sem_init(&m_nvm_clock_state.sem, 0, 1);

	LOG_DBG("Requested frequency [Hz]: %d\n", m_nvm_clk_spec.frequency);
	err = nrf_clock_control_request(nvm_clock_dev, &m_nvm_clk_spec, &m_nvm_clock_state.cli);
	if (err < 0) {
		LOG_ERR("MRAM clock request failed: %d", err);
		return err;
	}

	atomic_inc(&m_nvm_clock_state.m_clk_refcnt);

	return 0;
}

static int32_t m_nvm_clock_release(void)
{
	const struct device *nvm_clock_dev = DEVICE_DT_GET(NVM_CLOCK_DT_LABEL);
	int err;

	err = nrf_clock_control_cancel_or_release(nvm_clock_dev, &m_nvm_clk_spec,
						  &m_nvm_clock_state.cli);
	if (err < 0) {
		return err;
	}

	atomic_dec(&m_nvm_clock_state.m_clk_refcnt);

	return 0;
}

int32_t mpsl_clock_ctrl_nvm_clock_wait(void)
{
	return m_clock_request_wait(&m_nvm_clock_state,
				    CONFIG_MPSL_EXT_CLK_CTRL_CLOCK_REQUEST_WAIT_TIMEOUT_MS);
}
#endif /* CONFIG_MPSL_EXT_CLK_CTRL_NVM_CLOCK_REQUEST */

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
};

#if defined(CONFIG_CLOCK_CONTROL_NRF2)
static int m_lfclk_accuracy_get(void)
{
	int err;
	static const struct nrf_clock_spec lfclk_specs_req = {
		/* LFCLK frequency [Hz] */
		.frequency = 32768,
		.accuracy = MPSL_LFCLK_ACCURACY_PPM,
		/* This affects selected LFCLK source. It doesn't switch to higher accuracy
		 * but selects more precise but current hungry lfclk source.
		 */
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	};

	err = nrf_clock_control_resolve(DEVICE_DT_GET(DT_NODELABEL(lfclk)), &lfclk_specs_req,
					&m_lfclk_specs);
	if (err < 0) {
		LOG_ERR("Failed to resolve LFCLK spec: %d", err);
		return err;
	}

	LOG_DBG("LF Clock accuracy: %d", m_lfclk_specs.accuracy);

	return m_lfclk_specs.accuracy;
}
#endif /* CONFIG_CLOCK_CONTROL_NRF2 */

int32_t mpsl_clock_ctrl_init(void)
{
	int err;

#if defined(CONFIG_MPSL_EXT_CLK_CTRL_NVM_CLOCK_REQUEST)
	err = m_nvm_clock_request();
	if (err) {
		return err;
	}
#endif /* CONFIG_MPSL_EXT_CLK_CTRL_NVM_CLOCK_REQUEST */

#if defined(CONFIG_CLOCK_CONTROL_NRF)
#if DT_NODE_EXISTS(DT_NODELABEL(hfxo))
	m_nrf_hfclk_ctrl_data.startup_time_us = z_nrf_clock_bt_ctlr_hf_get_startup_time_us();
#else
	m_nrf_hfclk_ctrl_data.startup_time_us = CONFIG_MPSL_HFCLK_LATENCY;
#endif /* DT_NODE_EXISTS(DT_NODELABEL(hfxo)) */
#elif defined(CONFIG_CLOCK_CONTROL_NRF2)
#if DT_NODE_HAS_STATUS(HFCLK_LABEL, okay) && DT_NODE_HAS_COMPAT(HFCLK_LABEL, nordic_nrf54h_hfxo)
	uint32_t startup_time_us;

	err = nrf_clock_control_get_startup_time(DEVICE_DT_GET(HFCLK_LABEL), &m_hfclk_specs,
						 &startup_time_us);
	if (err < 0) {
		LOG_ERR("Failed to get HFCLK startup time: %d", err);
		return err;
	}

	if (startup_time_us > UINT16_MAX) {
		LOG_ERR("HFCLK startup time is too large: %d [us]", startup_time_us);
		return -NRF_EFAULT;
	}
#if defined(CONFIG_SOC_SERIES_NRF54H) && defined(CONFIG_MPSL_USE_ZEPHYR_PM)
	else if (startup_time_us < MPSL_PM_HFCLK_MINIMUM_ALLOWED_STARTUP_TIME_US) {
		/* Override the startup time to the minimum allowed value. */
		startup_time_us = MPSL_PM_HFCLK_MINIMUM_ALLOWED_STARTUP_TIME_US;
	}
#endif /* CONFIG_SOC_SERIES_NRF54H && CONFIG_MPSL_USE_ZEPHYR_PM */

	m_nrf_hfclk_ctrl_data.startup_time_us = startup_time_us;
#else
#error "Unsupported HFCLK statup time get operation"
#endif /* DT_NODE_HAS_STATUS(HFCLK_LABEL, okay) && \
	* DT_NODE_HAS_COMPAT(HFCLK_LABEL, nordic_nrf54h_hfxo) \
	*/
#else
#error "Unsupported HFCLK statup time get operation"
#endif /* CONFIG_CLOCK_CONTROL_NRF */

#if defined(CONFIG_CLOCK_CONTROL_NRF2)
	m_nrf_lfclk_ctrl_data.accuracy_ppm = m_lfclk_accuracy_get();
#endif /* CONFIG_CLOCK_CONTROL_NRF2 */

	return mpsl_clock_ctrl_source_register(&m_nrf_lfclk_ctrl_data, &m_nrf_hfclk_ctrl_data);
}

int32_t mpsl_clock_ctrl_uninit(void)
{
#if defined(CONFIG_MPSL_EXT_CLK_CTRL_NVM_CLOCK_REQUEST)
	int err;

	err = m_nvm_clock_release();
	if (err) {
		return err;
	}
#endif /* CONFIG_MPSL_EXT_CLK_CTRL_NVM_CLOCK_REQUEST */

#if defined(CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW)
	/* Reset the LFCLK timeout to allow for regular re-initialization of the MPSL.*/
	atomic_set(&m_lfclk_req_timeout, (atomic_val_t) false);
#endif /* CONFIG_MPSL_EXT_CLK_CTRL_LFCLK_REQ_TIMEOUT_ALLOW */

	return mpsl_clock_ctrl_source_unregister();
}
