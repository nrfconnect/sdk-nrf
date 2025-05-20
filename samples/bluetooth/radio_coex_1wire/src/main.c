/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/console/console.h>
#include <zephyr/drivers/gpio.h>

#include <hal/nrf_radio.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_egu.h>

#if defined(PPI_PRESENT)
#include <helpers/nrfx_gppi.h>
#endif

/* Predefined channels for radio events. */
#include <protocol/mpsl_dppi_protocol_api.h>

#define EGU_NODE DT_ALIAS(egu)
#define NRF_EGU ((NRF_EGU_Type *) DT_REG_ADDR(EGU_NODE))
#define EGU_EVENT_ID 4
#define EGU_TASK NRFX_CONCAT(NRF_EGU_, TASK_TRIGGER, EGU_EVENT_ID)
#define EGU_INT NRFX_CONCAT(NRF_EGU_, INT_TRIGGERED, EGU_EVENT_ID)
#define EGU_EVENT NRFX_CONCAT(NRF_EGU_, EVENT_TRIGGERED, EGU_EVENT_ID)

#if DT_NODE_HAS_STATUS(DT_PHANDLE(DT_NODELABEL(radio), coex), okay)
#define COEX_NODE DT_PHANDLE(DT_NODELABEL(radio), coex)
#else
#define COEX_NODE DT_INVALID_NODE
#error No enabled coex nodes registered in DTS.
#endif

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
#if DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, coex_pta_grant_gpios)
#define APP_GRANT_GPIO_PIN NRF_DT_GPIOS_TO_PSEL(ZEPHYR_USER_NODE, coex_pta_grant_gpios)
#else
#error "Unsupported board: see README and check /zephyr,user node"
#define APP_GRANT_GPIO_PIN 0
#endif

#define APP_GRANT_ACTIVE_LOW                                                                       \
	(GPIO_ACTIVE_LOW & DT_GPIO_FLAGS(COEX_NODE, grant_gpios) ? true : false)

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static atomic_t counter;

static void print_welcome_message(void)
{
	if (console_init()) {
		__ASSERT(false, "Failed to initialise console");
	}

	printk("-----------------------------------------------------\n");
	printk("This sample illustrates the 1Wire coex interface\n");
	printk("The number of radio events is printed every second.\n");
	printk("\n");
	printk("Press a key to change the state of the grant line:\n");
	if (APP_GRANT_ACTIVE_LOW) {
		printk("'g' to set the grant line to low and allow radio activity\n");
		printk("'d' to set the grant line to high and deny radio activity\n");
	} else {
		printk("'g' to set the grant line to high and allow radio activity\n");
		printk("'d' to set the grant line to low and deny radio activity\n");
	}
	printk("-----------------------------------------------------\n");
}

static void check_input(void)
{
	char input_char;

	input_char = console_getchar();

	if ((input_char == 'g') ^ APP_GRANT_ACTIVE_LOW) {
		nrf_gpio_pin_set(APP_GRANT_GPIO_PIN);
		printk("Current state is: Grant every request\n");
	} else if ((input_char == 'd') ^ APP_GRANT_ACTIVE_LOW) {
		nrf_gpio_pin_clear(APP_GRANT_GPIO_PIN);
		printk("Current state is: Deny every request\n");
	} else {
		return;
	}
}

static void egu_handler(const void *context)
{
	atomic_inc(&counter);

	nrf_egu_event_clear(NRF_EGU, EGU_EVENT);
}

static void console_print_thread(void)
{
	while (1) {
		atomic_val_t val;

		val = atomic_set(&counter, 0);
		printk("Number of radio events in the last second: %ld\n", val);

		k_sleep(K_MSEC(1000));
	}
}

#if defined(PPI_PRESENT)
static nrf_ppi_channel_t allocate_gppi_channel(void)
{
	nrf_ppi_channel_t channel;

	if (nrfx_ppi_channel_alloc(&channel) != NRFX_SUCCESS) {
		__ASSERT(false, "(D)PPI channel allocation error");
	}
	return channel;
}
#endif

static void setup_radio_event_counter(void)
{
#if defined(PPI_PRESENT)
	nrf_ppi_channel_t channel = allocate_gppi_channel();

	nrfx_gppi_channel_endpoints_setup(
		channel, nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_READY),
		nrf_egu_task_address_get(NRF_EGU, EGU_TASK));
	nrfx_ppi_channel_enable(channel);
#else
	/* Radio events are published on predefined channels. */
	nrf_egu_subscribe_set(NRF_EGU, EGU_TASK, MPSL_DPPI_RADIO_PUBLISH_READY_CHANNEL_IDX);
#endif

	IRQ_DIRECT_CONNECT(DT_IRQN(EGU_NODE), 5, egu_handler, 0);
	nrf_egu_int_enable(NRF_EGU, EGU_INT);
	NVIC_EnableIRQ(DT_IRQN(EGU_NODE));
}

static void setup_grant_pin(void)
{
	nrf_gpio_cfg_output(APP_GRANT_GPIO_PIN);
	if (APP_GRANT_ACTIVE_LOW) {
		nrf_gpio_pin_clear(APP_GRANT_GPIO_PIN);
	} else {
		nrf_gpio_pin_set(APP_GRANT_GPIO_PIN);
	}
}

int main(void)
{
	printk("Starting Radio Coex Demo 1wire on board %s\n", CONFIG_BOARD);

	if (bt_enable(NULL)) {
		printk("Bluetooth init failed");
		return 0;
	}
	printk("Bluetooth initialized\n");

	setup_grant_pin();
	setup_radio_event_counter();

	if (bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0)) {
		printk("Advertising failed to start");
		return 0;
	}
	printk("Advertising started\n");

	print_welcome_message();

	while (1) {
		k_sleep(K_MSEC(100));
		check_input();
	}
}

K_THREAD_DEFINE(console_print_thread_id, CONFIG_MAIN_STACK_SIZE, console_print_thread, NULL, NULL,
		NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
