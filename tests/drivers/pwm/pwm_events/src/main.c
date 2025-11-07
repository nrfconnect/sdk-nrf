/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <nrfx_pwm.h>
#include <nrfx_timer.h>
#include <helpers/nrfx_gppi.h>

#define PWM_OUTPUT_PIN NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(led2), gpios)

#define SLEEP_TIME_MS 500

struct pwm_events_fixture {
	nrfx_pwm_t *pwm;
	nrf_pwm_sequence_t *pwm_sequence;
	nrfx_timer_t *test_timer;
	uint32_t timer_task_address;
	uint32_t domain_id;
	nrfx_gppi_handle_t gppi_handle;
};

static void configure_pwm(nrfx_pwm_t *pwm)
{
	nrfx_pwm_config_t pwm_config =
		NRFX_PWM_DEFAULT_CONFIG(PWM_OUTPUT_PIN, NRF_PWM_PIN_NOT_CONNECTED,
					NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED);

	pwm_config.count_mode = NRF_PWM_MODE_UP_AND_DOWN;

	zassert_ok(nrfx_pwm_init(pwm, &pwm_config, NULL, NULL), "NRFX PWM init failed\n");
}

static uint32_t configure_test_timer(nrfx_timer_t *timer)
{
	uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer->p_reg);
	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);

	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_16;
	timer_config.mode = NRF_TIMER_MODE_COUNTER;

	TC_PRINT("Timer base frequency: %d Hz\n", base_frequency);

	zassert_ok(nrfx_timer_init(timer, &timer_config, NULL), "Timer init failed\n");
	nrfx_timer_enable(timer);

	return nrfx_timer_task_address_get(timer, NRF_TIMER_TASK_COUNT);
}

static void setup_dppi_connection(nrfx_gppi_handle_t *gppi_handle, uint32_t domain_id,
				  uint32_t timer_task_address, uint32_t pwm_event_address)
{
	zassert_ok(nrfx_gppi_conn_alloc(pwm_event_address, timer_task_address, gppi_handle),
		   "Failed to allocate DPPI connection\n");
	nrfx_gppi_conn_enable(*gppi_handle);
}

static void clear_dppi_connection(nrfx_gppi_handle_t *gppi_handle, uint32_t domain_id,
				  uint32_t timer_task_address, uint32_t pwm_event_address)
{
	nrfx_gppi_conn_disable(*gppi_handle);
	nrfx_gppi_conn_free(pwm_event_address, timer_task_address, *gppi_handle);
}

static void run_pwm_event_test_case(struct pwm_events_fixture *fixture,
				    nrf_pwm_event_t tested_pwm_event,
				    uint32_t expected_triggers_count, char *event_name)
{
	uint32_t pwm_event_address;
	uint32_t timer_cc_before, timer_cc_after;

	pwm_event_address = nrf_pwm_event_address_get(fixture->pwm->p_reg, tested_pwm_event);
	setup_dppi_connection(&fixture->gppi_handle, fixture->domain_id,
			      fixture->timer_task_address, pwm_event_address);

	nrf_pwm_event_clear(fixture->pwm->p_reg, tested_pwm_event);
	timer_cc_before = nrfx_timer_capture(fixture->test_timer, NRF_TIMER_CC_CHANNEL0);
	nrfx_pwm_simple_playback(fixture->pwm, fixture->pwm_sequence, 1, NRFX_PWM_FLAG_STOP);
	k_msleep(SLEEP_TIME_MS);
	timer_cc_after = nrfx_timer_capture(fixture->test_timer, NRF_TIMER_CC_CHANNEL0);

	TC_PRINT("PWM %s events count: %d\n", event_name, timer_cc_after - timer_cc_before);
	zassert_equal(timer_cc_after - timer_cc_before, expected_triggers_count,
		      "PWM %s event triggered count != %u\n", event_name, expected_triggers_count);

	nrf_pwm_event_clear(fixture->pwm->p_reg, tested_pwm_event);
	clear_dppi_connection(&fixture->gppi_handle, fixture->domain_id,
			      fixture->timer_task_address, pwm_event_address);
}

ZTEST_F(pwm_events, test_pwm_stop_event)
{
	run_pwm_event_test_case(fixture, NRF_PWM_EVENT_STOPPED, 1, "STOP");
}

ZTEST_F(pwm_events, test_pwm_seqstarted_event)
{
	run_pwm_event_test_case(fixture, NRF_PWM_EVENT_SEQSTARTED1, 1, "SEQSTARTED1");
}

ZTEST_F(pwm_events, test_pwm_seqend_event)
{
	run_pwm_event_test_case(fixture, NRF_PWM_EVENT_SEQEND1, 1, "SEQEND1");
}

ZTEST_F(pwm_events, test_pwm_periodend_event)
{
	run_pwm_event_test_case(fixture, NRF_PWM_EVENT_PWMPERIODEND, fixture->pwm_sequence->length,
				"PWMPERIODEND");
}

ZTEST_F(pwm_events, test_pwm_loopsdone_event)
{
	run_pwm_event_test_case(fixture, NRF_PWM_EVENT_LOOPSDONE, 1, "LOOPSDONE");
}

static void *test_setup(void)
{
	static struct pwm_events_fixture fixture;
	static nrfx_timer_t test_timer = NRFX_TIMER_INSTANCE(DT_REG_ADDR(DT_NODELABEL(tst_timer)));
	static nrfx_pwm_t pwm = NRFX_PWM_INSTANCE(DT_REG_ADDR(DT_NODELABEL(dut_pwm)));
	static nrf_pwm_values_common_t pwm_duty_cycle_values[] = {0x500, 0x600};
	static nrf_pwm_sequence_t pwm_sequence = {.values = {pwm_duty_cycle_values},
						  .length = ARRAY_SIZE(pwm_duty_cycle_values),
						  .repeats = 0,
						  .end_delay = 0};

	fixture.pwm = &pwm;
	fixture.pwm_sequence = &pwm_sequence;
	configure_pwm(&pwm);

	fixture.test_timer = &test_timer;
	fixture.timer_task_address = configure_test_timer(&test_timer);

	fixture.domain_id = nrfx_gppi_domain_id_get((uint32_t)test_timer.p_reg);

	return &fixture;
}

ZTEST_SUITE(pwm_events, NULL, test_setup, NULL, NULL, NULL);
