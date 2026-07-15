/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <cmsis_core.h>
#include <zephyr/irq.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#include <hal/nrf_grtc.h>

#include "hid_internal.h"
#include "hid_platform.h"

#define GPIOTE_CHANNEL 0U
#define GPIOTE_MASK    (1UL << GPIOTE_CHANNEL)

static bool input_enabled;

static void gpiote_interrupt_enable(void)
{
#if NRF_GPIOTE_HAS_INT_GROUPS
	nrf_gpiote_int_group_enable(RTFW_GPIOTE, NRF_GPIOTE_IRQ_GROUP,
				    GPIOTE_MASK);
#else
	nrf_gpiote_int_enable(RTFW_GPIOTE, GPIOTE_MASK);
#endif
}

static void gpiote_interrupt_disable(void)
{
#if NRF_GPIOTE_HAS_INT_GROUPS
	nrf_gpiote_int_group_disable(RTFW_GPIOTE, NRF_GPIOTE_IRQ_GROUP,
				     GPIOTE_MASK);
#else
	nrf_gpiote_int_disable(RTFW_GPIOTE, GPIOTE_MASK);
#endif
}

int rtfw_hid_command_handler(const struct rtfw_command *command,
			     void *user_data)
{
	struct rtfw_hid_config config;
	nrf_gpiote_event_t event = nrf_gpiote_in_event_get(GPIOTE_CHANNEL);

	ARG_UNUSED(user_data);
	if (command->id != RTFW_HID_COMMAND_CONFIGURE ||
	    command->data_len != sizeof(struct rtfw_hid_config)) {
		return -EINVAL;
	}

	memcpy(&config, command->data, sizeof(config));
	bool enable = config.enabled != 0U;

	if (enable == input_enabled) {
		return 0;
	}

	if (enable) {
		nrf_gpiote_event_clear(RTFW_GPIOTE, event);
		nrf_gpiote_event_enable(RTFW_GPIOTE, GPIOTE_CHANNEL);
		gpiote_interrupt_enable();
	} else {
		gpiote_interrupt_disable();
		nrf_gpiote_event_disable(RTFW_GPIOTE, GPIOTE_CHANNEL);
		nrf_gpiote_event_clear(RTFW_GPIOTE, event);
	}
	input_enabled = enable;

	return 0;
}

void rtfw_hid_pend_source_irq(void *user_data)
{
	ARG_UNUSED(user_data);
	NVIC_SetPendingIRQ(RTFW_GPIOTE_IRQN);
}

void rtfw_hid_fastpath_handler(void *user_data)
{
	nrf_gpiote_event_t source = nrf_gpiote_in_event_get(GPIOTE_CHANNEL);
	bool hardware_event = nrf_gpiote_event_check(RTFW_GPIOTE, source);

	ARG_UNUSED(user_data);
	if (hardware_event) {
		nrf_gpiote_event_clear(RTFW_GPIOTE, source);
	}

	if (hardware_event && input_enabled) {
		struct rtfw_event event = {
			.type = RTFW_HID_EVENT_EDGE,
			.value = ((nrf_gpio_pin_read(RTFW_INPUT_ABS) != 0U) !=
				  RTFW_INPUT_ACTIVE_LOW),
			.timestamp = nrf_grtc_sys_counter_low_get(NRF_GRTC),
		};

		(void)rtfw_event_push(&event);
	}
}

ISR_DIRECT_DECLARE(rtfw_hid_isr)
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

void rtfw_hid_fastpath_init(void)
{
	input_enabled = false;

	nrf_gpio_cfg_input(RTFW_INPUT_ABS, RTFW_INPUT_PULL);
	nrf_gpiote_event_configure(RTFW_GPIOTE, GPIOTE_CHANNEL, RTFW_INPUT_ABS,
				   NRF_GPIOTE_POLARITY_TOGGLE);
	nrf_gpiote_event_disable(RTFW_GPIOTE, GPIOTE_CHANNEL);
	gpiote_interrupt_disable();
	nrf_gpiote_event_clear(RTFW_GPIOTE,
			       nrf_gpiote_in_event_get(GPIOTE_CHANNEL));

#if defined(CONFIG_SAMPLE_RTFW_DEBUG_PIN)
	nrf_gpio_cfg_output(RTFW_DEBUG_GPIO_ABS);
	nrf_gpio_pin_clear(RTFW_DEBUG_GPIO_ABS);
#endif

	IRQ_DIRECT_CONNECT(RTFW_GPIOTE_IRQN, CONFIG_SAMPLE_RTFW_IRQ_PRIORITY,
			   rtfw_hid_isr, IRQ_ZERO_LATENCY);
	irq_enable(RTFW_GPIOTE_IRQN);
}
