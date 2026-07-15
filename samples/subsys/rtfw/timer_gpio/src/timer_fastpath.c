/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <cmsis_core.h>
#include <zephyr/irq.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_timer.h>

#include "timer_internal.h"
#include "timer_platform.h"

#define TIMER_TARGET_HZ 1000000U

static struct rtfw_timer_config applied_config;
static uint8_t timer_prescaler;
static uint32_t timer_hz;
static uint32_t tick_count;

static void timer_clock_resolve(void)
{
	uint32_t base_hz;
	uint8_t prescaler = 0U;

	if (timer_hz != 0U) {
		return;
	}

	base_hz = NRF_TIMER_BASE_FREQUENCY_GET(RTFW_TIMER);
	while (((base_hz >> prescaler) > TIMER_TARGET_HZ) && (prescaler < 9U)) {
		prescaler++;
	}

	timer_prescaler = prescaler;
	timer_hz = base_hz >> prescaler;
}

int rtfw_timer_command_handler(const struct rtfw_command *command,
			       void *user_data)
{
	struct rtfw_timer_config config;
	uint32_t ticks;

	ARG_UNUSED(user_data);
	if (command->id != RTFW_TIMER_COMMAND_CONFIGURE ||
	    command->data_len != sizeof(struct rtfw_timer_config)) {
		return -EINVAL;
	}

	memcpy(&config, command->data, sizeof(config));
	if (config.period_ticks < RTFW_TIMER_MIN_PERIOD_TICKS) {
		return -EINVAL;
	}
	ticks = config.period_ticks;

	/*
	 * A reconfiguration intentionally starts a new phase. STOP/CLEAR before
	 * changing CC0 prevents a smaller compare value from being stranded below
	 * the current counter until the 32-bit counter wraps.
	 */
	nrf_timer_task_trigger(RTFW_TIMER, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(RTFW_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_event_clear(RTFW_TIMER, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_cc_set(RTFW_TIMER, NRF_TIMER_CC_CHANNEL0, ticks);

	applied_config.enabled = config.enabled != 0U;
	applied_config.period_ticks = ticks;
	if (applied_config.enabled != 0U) {
		nrf_timer_task_trigger(RTFW_TIMER, NRF_TIMER_TASK_START);
	} else {
		nrf_gpio_pin_clear(RTFW_GPIO_ABS);
	}

	return 0;
}

void rtfw_timer_pend_source_irq(void *user_data)
{
	ARG_UNUSED(user_data);
	NVIC_SetPendingIRQ(RTFW_TIMER_IRQN);
}

void rtfw_timer_fastpath_handler(void *user_data)
{
	bool timer_event =
		nrf_timer_event_check(RTFW_TIMER, NRF_TIMER_EVENT_COMPARE0);

	ARG_UNUSED(user_data);
	if (timer_event) {
		nrf_timer_event_clear(RTFW_TIMER, NRF_TIMER_EVENT_COMPARE0);
	}

	if (timer_event && applied_config.enabled != 0U) {
		struct rtfw_event event;
		uint32_t current_tick;

		nrf_gpio_pin_toggle(RTFW_GPIO_ABS);
		current_tick = __atomic_add_fetch(&tick_count, 1U, __ATOMIC_RELAXED);

#if CONFIG_SAMPLE_RTFW_TIMER_TELEMETRY_DECIMATION > 0
		if ((current_tick %
		     CONFIG_SAMPLE_RTFW_TIMER_TELEMETRY_DECIMATION) == 0U) {
			event.type = RTFW_TIMER_EVENT_TICK;
			event.value = current_tick;
			event.timestamp = current_tick;
			(void)rtfw_event_push(&event);
		}
#else
		(void)event;
#endif
	}
}

ISR_DIRECT_DECLARE(rtfw_timer_isr)
{
#if defined(CONFIG_SAMPLE_RTFW_DEBUG_PIN)
	nrf_gpio_pin_set(RTFW_DEBUG_GPIO_ABS);
#endif

	rtfw_fastpath_run();

#if defined(CONFIG_SAMPLE_RTFW_ISR_TEST_LOAD)
	for (uint32_t i = 0U;
	     i < CONFIG_SAMPLE_RTFW_ISR_TEST_LOAD_CYCLES; i++) {
		__NOP();
	}
#endif

#if defined(CONFIG_SAMPLE_RTFW_DEBUG_PIN)
	nrf_gpio_pin_clear(RTFW_DEBUG_GPIO_ABS);
#endif

	return 0;
}

uint32_t rtfw_timer_us_to_ticks(uint32_t microseconds)
{
	uint64_t ticks;

	timer_clock_resolve();
	ticks = ((uint64_t)microseconds * timer_hz) / 1000000ULL;
	return (uint32_t)MIN(ticks, UINT32_MAX);
}

uint32_t rtfw_timer_ticks_to_us(uint32_t ticks)
{
	uint64_t microseconds;

	timer_clock_resolve();
	microseconds = ((uint64_t)ticks * 1000000ULL) / timer_hz;
	return (uint32_t)MIN(microseconds, UINT32_MAX);
}

uint32_t rtfw_timer_tick_count_get(void)
{
	return __atomic_load_n(&tick_count, __ATOMIC_RELAXED);
}

void rtfw_timer_fastpath_init(uint32_t initial_period_ticks)
{
	timer_clock_resolve();
	applied_config.enabled = 0U;
	applied_config.period_ticks = initial_period_ticks;
	__atomic_store_n(&tick_count, 0U, __ATOMIC_RELAXED);

	nrf_gpio_cfg_output(RTFW_GPIO_ABS);
	nrf_gpio_pin_clear(RTFW_GPIO_ABS);
#if defined(CONFIG_SAMPLE_RTFW_DEBUG_PIN)
	nrf_gpio_cfg_output(RTFW_DEBUG_GPIO_ABS);
	nrf_gpio_pin_clear(RTFW_DEBUG_GPIO_ABS);
#endif

	nrf_timer_task_trigger(RTFW_TIMER, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(RTFW_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_mode_set(RTFW_TIMER, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(RTFW_TIMER, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_prescaler_set(RTFW_TIMER, timer_prescaler);
	nrf_timer_cc_set(RTFW_TIMER, NRF_TIMER_CC_CHANNEL0, initial_period_ticks);
	nrf_timer_shorts_enable(RTFW_TIMER, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);
	nrf_timer_event_clear(RTFW_TIMER, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_int_enable(RTFW_TIMER, NRF_TIMER_INT_COMPARE0_MASK);

	IRQ_DIRECT_CONNECT(RTFW_TIMER_IRQN, CONFIG_SAMPLE_RTFW_IRQ_PRIORITY,
			   rtfw_timer_isr, IRQ_ZERO_LATENCY);
	irq_enable(RTFW_TIMER_IRQN);
}
