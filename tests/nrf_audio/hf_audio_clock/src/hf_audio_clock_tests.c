/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include "nrfx_clock_hfclkaudio.h"
#include "hf_audio_clock.h"
#include "audio_rate_control.h"

DECLARE_FAKE_VALUE_FUNC(int, audio_rate_control_register, uint8_t,
			struct audio_rate_control_ops const *const);
DECLARE_FAKE_VALUE_FUNC(int, nrfx_clock_divider_set, nrf_clock_domain_t, nrf_clock_hfclk_div_t);
DECLARE_FAKE_VALUE_FUNC(int, nrfx_clock_hfclkaudio_init, nrfx_clock_hfclkaudio_event_handler_t);
DECLARE_FAKE_VOID_FUNC(nrfx_clock_hfclkaudio_config_set, uint16_t);
DECLARE_FAKE_VOID_FUNC(nrfx_clock_hfclkaudio_start);

DEFINE_FAKE_VALUE_FUNC(int, audio_rate_control_register, uint8_t,
		       struct audio_rate_control_ops const *const);
DEFINE_FAKE_VALUE_FUNC(int, nrfx_clock_divider_set, nrf_clock_domain_t, nrf_clock_hfclk_div_t);
DEFINE_FAKE_VALUE_FUNC(int, nrfx_clock_hfclkaudio_init, nrfx_clock_hfclkaudio_event_handler_t);
DEFINE_FAKE_VOID_FUNC(nrfx_clock_hfclkaudio_config_set, uint16_t);
DEFINE_FAKE_VOID_FUNC(nrfx_clock_hfclkaudio_start);

extern struct audio_rate_control_ops hf_audio_clock_rate_control_ops;

void hf_audio_clock_test_prepare(void *fixture)
{
	ARG_UNUSED(fixture);
	RESET_FAKE(nrfx_clock_divider_set);
	RESET_FAKE(nrfx_clock_hfclkaudio_init);
	RESET_FAKE(nrfx_clock_hfclkaudio_config_set);
	RESET_FAKE(nrfx_clock_hfclkaudio_start);
	nrfx_clock_divider_set_fake.return_val = 0;
	nrfx_clock_hfclkaudio_init_fake.return_val = 0;
	zassert_equal(hf_audio_clock_init(), 0, "Clock setup failed");
	RESET_FAKE(nrfx_clock_divider_set);
	RESET_FAKE(nrfx_clock_hfclkaudio_init);
	RESET_FAKE(nrfx_clock_hfclkaudio_config_set);
	RESET_FAKE(nrfx_clock_hfclkaudio_start);
}

/* Verify calibration applies the timing correction to the center frequency. */
ZTEST(suite_hf_audio_clock, test_update_calibrated)
{
	int32_t error_us = 331;

	zassert_equal(hf_audio_clock_rate_control_ops.cfg_set(
			      (struct audio_rate_control_cfg const *)&error_us),
		      0, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_config_set_fake.arg0_val, APLL_FREQ_CENTER - 1000,
		      NULL);
}

/* Verify non-calibrated updates apply corrections relative to the current frequency. */
ZTEST(suite_hf_audio_clock, test_update_uses_current_frequency_without_calibration)
{
	int32_t error_us = -331, calibration_error_us = 662;

	zassert_equal(hf_audio_clock_rate_control_ops.cfg_set(
			      (struct audio_rate_control_cfg const *)&calibration_error_us),
		      0, NULL);

	zassert_equal(hf_audio_clock_update(&error_us), 0, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_config_set_fake.arg0_val, APLL_FREQ_CENTER - 1000,
		      NULL);
}

/* Verify out-of-range calibrated updates fail without reconfiguring the clock. */
ZTEST(suite_hf_audio_clock, test_update_rejects_out_of_range_calibration)
{
	int32_t error_us = 1000000;
	unsigned int config_call_count;

	config_call_count = nrfx_clock_hfclkaudio_config_set_fake.call_count;
	zassert_equal(hf_audio_clock_rate_control_ops.cfg_set(
			      (struct audio_rate_control_cfg const *)&error_us),
		      -EINVAL, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_config_set_fake.call_count, config_call_count, NULL);
}

/* Verify non-calibrated updates clamp values below the supported frequency range. */
ZTEST(suite_hf_audio_clock, test_update_clamps_without_calibration)
{
	int32_t error_us = 1000000;

	zassert_equal(hf_audio_clock_update(&error_us), 0, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_config_set_fake.arg0_val, APLL_FREQ_MIN, NULL);
}

/* Verify non-calibrated updates clamp values above the supported frequency range. */
ZTEST(suite_hf_audio_clock, test_update_clamps_without_calibration_at_upper_bound)
{
	int32_t error_us = -1000000;

	zassert_equal(hf_audio_clock_update(&error_us), 0, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_config_set_fake.arg0_val, APLL_FREQ_MAX, NULL);
}

/* Verify initialization sets the divider and center frequency before starting the clock. */
ZTEST(suite_hf_audio_clock, test_init_configures_and_starts_clock)
{
	nrfx_clock_hfclkaudio_init_fake.return_val = 0;

	zassert_equal(hf_audio_clock_init(), 0, NULL);
	zassert_equal(nrfx_clock_divider_set_fake.call_count, 1, NULL);
	zassert_equal(nrfx_clock_divider_set_fake.arg0_val, NRF_CLOCK_DOMAIN_HFCLK, NULL);
	zassert_equal(nrfx_clock_divider_set_fake.arg1_val, NRF_CLOCK_HFCLK_DIV_1, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_config_set_fake.arg0_val, APLL_FREQ_CENTER, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_init_fake.arg0_val, NULL, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_start_fake.call_count, 1, NULL);
}

/* Verify an already initialized nrfx clock is accepted and started normally. */
ZTEST(suite_hf_audio_clock, test_init_accepts_already_initialized_clock)
{
	nrfx_clock_hfclkaudio_init_fake.return_val = -EALREADY;

	zassert_equal(hf_audio_clock_init(), 0, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_start_fake.call_count, 1, NULL);
}

/* Verify divider setup errors stop initialization before later driver calls. */
ZTEST(suite_hf_audio_clock, test_init_returns_divider_error)
{
	nrfx_clock_divider_set_fake.return_val = -EIO;

	zassert_equal(hf_audio_clock_init(), -EIO, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_config_set_fake.call_count, 0, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_init_fake.call_count, 0, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_start_fake.call_count, 0, NULL);
}

/* Verify clock-driver initialization errors are returned without starting the clock. */
ZTEST(suite_hf_audio_clock, test_init_returns_clock_init_error)
{
	nrfx_clock_hfclkaudio_init_fake.return_val = -EIO;

	zassert_equal(hf_audio_clock_init(), -EIO, NULL);
	zassert_equal(nrfx_clock_hfclkaudio_start_fake.call_count, 0, NULL);
}

/* Verify startup registers the audio PLL and exposes the correct control operations. */
ZTEST(suite_hf_audio_clock, test_registers_audio_pll)
{
	zassert_equal(audio_rate_control_register_fake.call_count, 1, NULL);
	zassert_equal(audio_rate_control_register_fake.arg0_val, AUDIO_PLL, NULL);
	zassert_equal(audio_rate_control_register_fake.arg1_val, &hf_audio_clock_rate_control_ops,
		      NULL);
	zassert_equal(hf_audio_clock_rate_control_ops.update, hf_audio_clock_update, NULL);
	zassert_not_null(hf_audio_clock_rate_control_ops.cfg_set, NULL);
	zassert_equal(hf_audio_clock_rate_control_ops.initialize, hf_audio_clock_init, NULL);
}
