/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <nrf9160.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <string.h>
#include <stdlib.h>
//#include <nrfx_pdm_ns.h>
#include <nrfx_timer.h>
#include <nrfx_dppi.h>
#include <nrfx_gpiote.h>
#include <hal/nrf_gpiote.h>
#include <hal/nrf_timer.h>

#include <gpio.h>

const u32_t rising_edge_pin = 6;
const u32_t falling_edge_pin = 7;

static const nrfx_timer_t timer = NRFX_TIMER_INSTANCE(0);

static void gpiote_event_handler(nrfx_gpiote_pin_t pin,
				 nrf_gpiote_polarity_t action)
{
	if (pin == falling_edge_pin) {
		/* Read out timer CC register and clear it */
		u32_t timer_cc_val = nrf_timer_cc_read(timer.p_reg, 0);
		printk("Raw timer val: %x\n", timer_cc_val);
		/* Calculate pulse length, 16M base freq */
		u32_t pulse_len_us = (timer_cc_val >> 4) - 1;
		u32_t pulse_len_ms = pulse_len_us / 1000;
		printk("ms: %d\n", pulse_len_ms);
		printk("us: %d\n", pulse_len_us);
		nrfx_timer_clear(&timer);
	} else {
		printk("Unknown pin. Check your button configuration\n");
	}
}

static void timer_init(void)
{
	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG;
	timer_config.bit_width = 3;
	timer_config.frequency = NRF_TIMER_FREQ_16MHz;

	u32_t err = nrfx_timer_init(&timer, &timer_config, NULL);
	if (err != NRFX_SUCCESS) {
		printk("Err :%x\n", err);
	}
}

static void dppi_init(void)
{
	u8_t dppi_ch_1, dppi_ch_2;
	u32_t err = nrfx_dppi_channel_alloc(&dppi_ch_1);
	if (err != NRFX_SUCCESS) {
		printk("Err %d\n", err);
		return;
	}

	err = nrfx_dppi_channel_alloc(&dppi_ch_2);
	if (err != NRFX_SUCCESS) {
		printk("Err %d\n", err);
		return;
	}

	err = nrfx_dppi_channel_enable(dppi_ch_1);
	if (err != NRFX_SUCCESS) {
		printk("Err %d\n", err);
		return;
	}

	err = nrfx_dppi_channel_enable(dppi_ch_2);
	if (err != NRFX_SUCCESS) {
		printk("Err %d\n", err);
		return;
	}

	/* Tie it all together */
	nrf_gpiote_publish_set(NRF_GPIOTE_EVENTS_IN_0, dppi_ch_1);
	nrf_gpiote_publish_set(NRF_GPIOTE_EVENTS_IN_1, dppi_ch_2);
	nrf_timer_subscribe_set(timer.p_reg, NRF_TIMER_TASK_START, dppi_ch_1);
	nrf_timer_subscribe_set(timer.p_reg, NRF_TIMER_TASK_STOP, dppi_ch_2);
	nrf_timer_subscribe_set(timer.p_reg, NRF_TIMER_TASK_CAPTURE0,
				dppi_ch_2);
}

static void gpiote_init(void)
{
	nrfx_gpiote_in_config_t falling_edge = { 0 };
	nrfx_gpiote_in_config_t rising_edge = { 0 };

	falling_edge.hi_accuracy = 1;
	falling_edge.pull = NRF_GPIO_PIN_PULLUP;
	falling_edge.sense = NRF_GPIOTE_POLARITY_LOTOHI;

	rising_edge.hi_accuracy = 1;
	rising_edge.pull = NRF_GPIO_PIN_PULLUP;
	rising_edge.sense = NRF_GPIOTE_POLARITY_HITOLO;

	u32_t err = nrfx_gpiote_init();
	if (err != NRFX_SUCCESS) {
		printk("gpiote1 err: %x", err);
		return;
	}

	err = nrfx_gpiote_in_init(rising_edge_pin, &rising_edge, NULL);
	if (err != NRFX_SUCCESS) {
		printk("gpiote1 err: %x", err);
		return;
	}

	err = nrfx_gpiote_in_init(falling_edge_pin, &falling_edge,
				  gpiote_event_handler);
	if (err != NRFX_SUCCESS) {
		printk("gpiote2 err: %x", err);
		return;
	}

	nrfx_gpiote_in_event_enable(falling_edge_pin, 1);
	nrfx_gpiote_in_event_enable(rising_edge_pin, 1);
}

static void manual_isr_setup()
{
	IRQ_DIRECT_CONNECT(GPIOTE1_IRQn, 0, nrfx_gpiote_irq_handler, 0);
	irq_enable(GPIOTE1_IRQn);
}

void main(void)
{
	const char *devzone_thread =
		"https://devzone.nordicsemi.com/"
		"f/nordic-q-a/51450/timer-0-interface-program";
	printk("Starting nrfx timed signal program!\n");
	printk("This sample requires DPPIC to be set as nonsecure\n");
	printk("See %s for more details!\n", devzone_thread);
	printk("This sample takes in a signal,"
	       "and uses two GPIOTE IN channels to time it via TIMER0 and DPPIC\n");
	timer_init();
	dppi_init();
	gpiote_init();
	manual_isr_setup();
}