/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrfx_pwm.h>
#include <hal/nrf_pwm.h>
#include <haly/nrfy_gpio.h>
#include <zephyr/irq.h>

#define PWM_OUTPUT_PIN DT_GPIO_PIN(DT_ALIAS(led2), gpios)

#if defined(CONFIG_NRFX_PWM130)
#define PWM_INSTANCE_NUMBER 130
#elif defined(CONFIG_NRFX_PWM20)
#define PWM_INSTANCE_NUMBER 20
#endif

#define SLEEP_TIME_MS 1000

static const nrfx_pwm_t pwm_instance = NRFX_PWM_INSTANCE(PWM_INSTANCE_NUMBER);
static nrf_pwm_values_common_t pwm_duty_cycle_values[] = {0x05FA, 0x04DE};
static nrf_pwm_sequence_t test_sequence = {.values = {pwm_duty_cycle_values},
					   .length = ARRAY_SIZE(pwm_duty_cycle_values),
					   .repeats = 0,
					   .end_delay = 0};

static volatile nrf_pwm_event_t tested_pwm_event;
static volatile uint32_t pwm_event_counter;

static void pwm_interrupt_service_handler(void)
{
	pwm_event_counter++;
	nrfy_pwm_event_clear(pwm_instance.p_reg, tested_pwm_event);
}

static void enable_pwm_event(const nrfx_pwm_t *pwm_inst, nrf_pwm_event_t pwm_event,
			     nrf_pwm_int_mask_t pwm_interrupt_mask)
{
	nrfy_pwm_event_clear(pwm_instance.p_reg, pwm_event);
	nrfy_pwm_int_init(pwm_instance.p_reg, pwm_interrupt_mask, 0, true);
}

static void disable_pwm_event(const nrfx_pwm_t *pwm_inst, nrf_pwm_event_t pwm_event,
			      nrf_pwm_int_mask_t pwm_interrupt_mask)
{
	nrfy_pwm_event_clear(pwm_instance.p_reg, pwm_event);
	nrfy_pwm_int_disable(pwm_instance.p_reg, pwm_interrupt_mask);
}

static void configure_pwm(const nrfx_pwm_t *pwm_inst)
{
	nrfx_err_t err;
	nrfx_pwm_config_t pwm_config =
		NRFX_PWM_DEFAULT_CONFIG(PWM_OUTPUT_PIN, NRF_PWM_PIN_NOT_CONNECTED,
					NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED);

	/* Connect PWM IRQ to nrfx_pwm_irq_handler */
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(dut_pwm)), DT_IRQ(DT_NODELABEL(dut_pwm), priority),
		    nrfx_isr, pwm_interrupt_service_handler, 0);

	err = nrfx_pwm_init(&pwm_instance, &pwm_config, NULL, NULL);
	if (err != NRFX_SUCCESS) {
		printk("nrfx_pwm_init() failed, err: %d\n", err);
	}
}

static void test_pwm_stop_event(const nrfx_pwm_t *pwm_inst, nrf_pwm_sequence_t *sequence)
{
	pwm_event_counter = 0;
	tested_pwm_event = NRF_PWM_EVENT_STOPPED;

	printk("TESTING PWM EVENTS_STOPPED\n");
	enable_pwm_event(pwm_inst, tested_pwm_event, NRF_PWM_INT_STOPPED_MASK);
	nrfx_pwm_simple_playback(pwm_inst, sequence, 1, NRFX_PWM_FLAG_STOP);
	k_msleep(SLEEP_TIME_MS);
	printk("PWM event counter: %d\n", pwm_event_counter);
	if (pwm_event_counter == 1) {
		printk("PWM EVENTS_STOPPED TEST: PASS\n");
	} else {
		printk("PWM EVENTS_STOPPED TEST: FAIL\n");
	}
	disable_pwm_event(pwm_inst, tested_pwm_event, NRF_PWM_INT_STOPPED_MASK);
}

static void test_pwm_periodend_event(const nrfx_pwm_t *pwm_inst, nrf_pwm_sequence_t *sequence)
{
	pwm_event_counter = 0;
	tested_pwm_event = NRF_PWM_EVENT_PWMPERIODEND;

	printk("TESTING PWM EVENTS_PWMPERIODEND\n");
	enable_pwm_event(pwm_inst, tested_pwm_event, NRF_PWM_INT_PWMPERIODEND_MASK);
	nrfx_pwm_simple_playback(pwm_inst, sequence, 1, NRFX_PWM_FLAG_STOP);
	k_msleep(SLEEP_TIME_MS);
	printk("PWM event counter: %d\n", pwm_event_counter);
	if (pwm_event_counter == sequence->length) {
		printk("PWM EVENTS_PWMPERIODEND TEST: PASS\n");
	} else {
		printk("PWM EVENTS_PWMPERIODEND TEST: FAIL\n");
	}
	disable_pwm_event(pwm_inst, tested_pwm_event, NRF_PWM_INT_PWMPERIODEND_MASK);
}

static void test_pwm_loopsdone_event(const nrfx_pwm_t *pwm_inst, nrf_pwm_sequence_t *sequence)
{
	pwm_event_counter = 0;
	tested_pwm_event = NRF_PWM_EVENT_LOOPSDONE;

	printk("TESTING PWM EVENTS_LOOPSDONE\n");
	enable_pwm_event(pwm_inst, tested_pwm_event, NRF_PWM_INT_LOOPSDONE_MASK);
	nrfx_pwm_simple_playback(pwm_inst, sequence, 1, NRFX_PWM_FLAG_STOP);
	k_msleep(SLEEP_TIME_MS);
	printk("PWM event counter: %d\n", pwm_event_counter);
	if (pwm_event_counter == 1) {
		printk("PWM EVENTS_LOOPSDONE TEST: PASS\n");
	} else {
		printk("PWM EVENTS_LOOPSDONE TEST: FAIL\n");
	}
	disable_pwm_event(pwm_inst, tested_pwm_event, NRF_PWM_INT_LOOPSDONE_MASK);
}

/*
 * EVENTS_COMPAREMATCH[0] is tested
 * This event is not directly supported in the NRFX
 */
static void test_pwm_comparematch_event(const nrfx_pwm_t *pwm_inst, nrf_pwm_sequence_t *sequence)
{
	pwm_event_counter = 0;
	tested_pwm_event = offsetof(NRF_PWM_Type, EVENTS_COMPAREMATCH);

	printk("TESTING PWM EVENTS_COMPAREMATCH\n");
	nrfy_pwm_event_clear(pwm_inst->p_reg, tested_pwm_event);
	nrfy_pwm_int_init(pwm_inst->p_reg, PWM_INTENSET_COMPAREMATCH0_Msk, 0, true);
	nrfx_pwm_simple_playback(pwm_inst, sequence, 1, NRFX_PWM_FLAG_STOP);
	k_msleep(SLEEP_TIME_MS);
	printk("PWM event counter: %d\n", pwm_event_counter);
	if (pwm_event_counter == 1) {
		printk("PWM EVENTS_COMPAREMATCH TEST: PASS\n");
	} else {
		printk("PWM EVENTS_COMPAREMATCH TEST: FAIL\n");
	}
	nrfy_pwm_event_clear(pwm_inst->p_reg, tested_pwm_event);
	nrfy_pwm_int_disable(pwm_inst->p_reg, PWM_INTENSET_COMPAREMATCH0_Msk);
}

int main(void)
{
	configure_pwm(&pwm_instance);
	test_pwm_comparematch_event(&pwm_instance, &test_sequence);
	test_pwm_stop_event(&pwm_instance, &test_sequence);
	test_pwm_periodend_event(&pwm_instance, &test_sequence);
	test_pwm_loopsdone_event(&pwm_instance, &test_sequence);

	return 0;
}
