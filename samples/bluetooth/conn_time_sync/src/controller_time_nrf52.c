/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** This file implements controller time management for 52 Series devices
 *
 * As the controller clock is not directly accessible, the controller
 * time is obtained using a mirrored RTC peripheral.
 * To achieve microsecond accurate toggling, a timer peripheral is also used.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <mpsl_clock.h>
#include <nrfx_rtc.h>
#include <nrfx_timer.h>
#include <helpers/nrfx_gppi.h>
#include <hal/nrf_egu.h>
#include <soc.h>
#include "conn_time_sync.h"

static nrfx_rtc_t app_rtc_instance = NRFX_RTC_INSTANCE(NRF_RTC2);
static nrfx_timer_t app_timer_instance = NRFX_TIMER_INSTANCE(NRF_TIMER1);

static nrfx_gppi_handle_t ppi_on_rtc_match;
static volatile uint32_t num_rtc_overflows;

static uint32_t offset_ticks_and_controller_to_app_rtc;

static void rtc_isr_handler(nrf_rtc_event_t event_type, void *ctx)
{
	ARG_UNUSED(ctx);

	if (event_type == NRF_RTC_EVENT_OVERFLOW) {
		num_rtc_overflows++;
	}
}

static void unused_timer_isr_handler(nrf_timer_event_t event_type, void *ctx)
{
	ARG_UNUSED(event_type);
	ARG_UNUSED(ctx);
}

static int32_t rtc_diff_get(void)
{
	uint32_t controller_ticks = nrf_rtc_counter_get(NRF_RTC0);
	uint32_t app_ticks = nrf_rtc_counter_get(app_rtc_instance.p_reg);

	return controller_ticks - app_ticks;
}

static int rtc_config(void)
{
	int ret;

	const nrfx_rtc_config_t rtc_cfg = NRFX_RTC_DEFAULT_CONFIG;

	ret = nrfx_rtc_init(&app_rtc_instance, &rtc_cfg, rtc_isr_handler);
	if (ret != 0) {
		printk("Failed initializing RTC (ret: %d)\n", ret);
		return -ENODEV;
	}

#ifndef CONFIG_SOC_SERIES_BSIM_NRFXX
	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_RTC_INST_GET(2)), IRQ_PRIO_LOWEST,
		    nrfx_rtc_irq_handler, &app_rtc_instance, 0);
#else
	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_RTC_INST_GET(2)), 0,
		    (void *)nrfx_rtc_irq_handler, &app_rtc_instance, 0);
#endif

	nrfx_rtc_overflow_enable(&app_rtc_instance, true);
	nrfx_rtc_tick_enable(&app_rtc_instance, false);
	nrfx_rtc_enable(&app_rtc_instance);

	/* To obtain the controller timestamp without modifying the state of RTC0,
	 * we find the offset between RTC0 and our mirrored RTC.
	 * We achieve this by reading out the counter value of both of them.
	 * When the diff between those two have been equal twice, we know the
	 * time difference in ticks.
	 */

	uint32_t sync_attempts = 10;

	while (sync_attempts > 0) {
		sync_attempts--;

		int32_t diff_measurement_1 = rtc_diff_get();

		/* We need to wait half an RTC tick to ensure we are not measuring
		 * the diff between the two RTCs at the point in time where their
		 * values are transitioning.
		 */
		k_busy_wait(15);

		int32_t diff_measurement_2 = rtc_diff_get();

		if (diff_measurement_1 == diff_measurement_2) {
			offset_ticks_and_controller_to_app_rtc = diff_measurement_1;
			return 0;
		}
	}

	printk("Controller time sync failure\n");
	offset_ticks_and_controller_to_app_rtc = 0;
	return -EINVAL;
}

static int timer_config(void)
{
	int ret;
	nrfx_gppi_handle_t ppi_handle_timer_clear_on_rtc_tick;
	const nrfx_timer_config_t timer_cfg = {
		.frequency = NRFX_MHZ_TO_HZ(1UL),
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_8,
		.interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context = NULL};
	uint32_t eep;
	uint32_t tep;

	ret = nrfx_timer_init(&app_timer_instance, &timer_cfg, unused_timer_isr_handler);
	if (ret != 0) {
		printk("Failed initializing timer (ret: %d)\n", ret);
		return -ENODEV;
	}

	/* Clear the TIMER every RTC tick. */
	eep = nrfx_rtc_event_address_get(&app_rtc_instance, NRF_RTC_EVENT_TICK);
	tep = nrfx_timer_task_address_get(&app_timer_instance, NRF_TIMER_TASK_CLEAR);
	ret = nrfx_gppi_conn_alloc(eep, tep, &ppi_handle_timer_clear_on_rtc_tick);
	if (ret < 0) {
		printk("Failed allocating for clearing TIMER on RTC TICK\n");
		return ret;
	}

	nrfx_gppi_conn_enable(ppi_handle_timer_clear_on_rtc_tick);
	nrfx_timer_enable(&app_timer_instance);

	return 0;
}

/** Configure the TIMER and RTC in such a way that microsecond accurate timing can be achieved.
 *
 * To get microsecond accurate toggling, we need to combine both the RTC and TIMER peripheral.
 * That is, both a RTC CC value and TIMER CC value needs to be set.
 * We should only trigger an EGU task when the TIMER CC value matches after the
 * RTC CC value matched.
 * This is achieved using a PPI group. The group is enabled when the RTC CC matches and disabled
 * again then the TIMER CC matches.
 */
int config_egu_trigger_on_rtc_and_timer_match(void)
{
	nrfx_gppi_handle_t ppi_on_timer_match;
	nrfx_gppi_group_handle_t group;
	uint32_t eep0 = nrfx_rtc_event_address_get(&app_rtc_instance, NRF_RTC_EVENT_COMPARE_0);
	uint32_t eep1 = nrfx_timer_event_address_get(&app_timer_instance, NRF_TIMER_EVENT_COMPARE0);
	uint32_t tep1 = nrf_egu_task_address_get(NRF_EGU0, NRF_EGU_TASK_TRIGGER0);
	int ret;

	ret = nrfx_gppi_group_alloc(nrfx_gppi_domain_id_get(eep0), &group);
	if (ret < 0) {
		printk("Failed allocating group\n");
		return ret;
	}

	ret = nrfx_gppi_group_ep_add(group, eep0);
	if (ret < 0) {
		printk("Failed attaching an event to the group\n");
		return ret;
	}

	ret = nrfx_gppi_conn_alloc(eep0, nrfx_gppi_group_task_en_addr(group), &ppi_on_rtc_match);
	if (ret < 0) {
		printk("Failed allocating for RTC match\n");
		return ret;
	}

	ret = nrfx_gppi_conn_alloc(eep1, tep1, &ppi_on_timer_match);
	if (ret < 0) {
		printk("Failed allocating for RTC match\n");
		return ret;
	}
	(void)nrfx_gppi_ep_attach(nrfx_gppi_group_task_dis_addr(group), ppi_on_timer_match);

	return 0;
}

int controller_time_init(void)
{
	int ret;

	ret = rtc_config();
	if (ret) {
		return ret;
	}

	ret = timer_config();
	if (ret) {
		return ret;
	}

	return config_egu_trigger_on_rtc_and_timer_match();
}

static uint64_t rtc_ticks_to_us(uint32_t rtc_ticks)
{
	const uint64_t rtc_ticks_in_femto_units = 30517578125UL;

	return (rtc_ticks * rtc_ticks_in_femto_units) / 1000000000UL;
}

static uint32_t us_to_rtc_ticks(uint32_t timestamp_us)
{
	const uint64_t rtc_ticks_in_femto_units = 30517578125UL;

	return ((uint64_t)(timestamp_us) * 1000000000UL) / rtc_ticks_in_femto_units;
}

uint64_t controller_time_us_get(void)
{
	const uint64_t rtc_overflow_time_us = 512000000UL;

	/* On the 52 series the RTC has to task to capture the current RTC value.
	 * Therefore we cannot capture the TIMER and RTC value simultaneously.
	 * Therefore we only use the RTC to read out the current time here.
	 * This will result in an error of maximum one RTC tick.
	 */

	uint32_t captured_rtc_overflows;
	uint32_t captured_rtc_ticks;

	while (true) {
		captured_rtc_overflows = num_rtc_overflows;

		/* Read out RTC ticks after reading number of overflows. */
		barrier_isync_fence_full();

		captured_rtc_ticks = nrf_rtc_counter_get(app_rtc_instance.p_reg);

		/* Read out number of overflows after reading number of RTC ticks  */
		barrier_isync_fence_full();

		if (captured_rtc_overflows == num_rtc_overflows) {
			/* There were no new overflows after reading ticks.
			 * That is, we can use the captured value.
			 */
			break;
		}
	}

	return rtc_ticks_to_us(captured_rtc_ticks) +
	       (captured_rtc_overflows * rtc_overflow_time_us) +
	       rtc_ticks_to_us(offset_ticks_and_controller_to_app_rtc);
}

void controller_time_trigger_set(uint64_t timestamp_us)
{
	uint64_t timestamp_without_rtc_offset =
		timestamp_us - rtc_ticks_to_us(offset_ticks_and_controller_to_app_rtc);

	uint32_t num_overflows = timestamp_without_rtc_offset / 512000000UL;
	uint64_t overflow_time_us = num_overflows * 512000000UL;

	uint32_t rtc_remainder_time_us = timestamp_without_rtc_offset - overflow_time_us;
	uint32_t rtc_val = us_to_rtc_ticks(rtc_remainder_time_us);
	uint8_t timer_val =	timestamp_without_rtc_offset - rtc_ticks_to_us(rtc_val);

	/* Ensure the timer value lies between 1 and 30 so that it will
	 * always be between two RTC ticks.
	 */
	timer_val = MAX(timer_val, 1);
	timer_val = MIN(timer_val, 30);

	if (nrfx_rtc_cc_set(&app_rtc_instance, 0, rtc_val, false) != 0) {
		printk("Failed setting trigger\n");
	}

	nrfx_timer_compare(&app_timer_instance, 0, timer_val, false);
	nrfx_gppi_conn_enable(ppi_on_rtc_match);
}

uint32_t controller_time_trigger_event_addr_get(void)
{
	return nrf_egu_event_address_get(NRF_EGU0, NRF_EGU_EVENT_TRIGGERED0);
}

SYS_INIT(controller_time_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
