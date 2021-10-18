/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <sys/printk.h>
#include <console/console.h>
#include <sys/__assert.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>

#include <hal/nrf_radio.h>
#include <hal/nrf_egu.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>
#include <helpers/nrfx_gppi.h>

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
#if (DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, coex_pta_req_gpios) && \
	DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, coex_pta_grant_gpios) && \
	DT_NODE_HAS_STATUS(DT_PHANDLE(DT_NODELABEL(radio), coex), okay))
/* PTA input */
#define APP_REQ_GPIO_PIN NRF_DT_GPIOS_TO_PSEL(ZEPHYR_USER_NODE, coex_pta_req_gpios)
/* PTA output */
#define APP_GRANT_GPIO_PIN NRF_DT_GPIOS_TO_PSEL(ZEPHYR_USER_NODE, coex_pta_grant_gpios)
#define COEX_NODE DT_PHANDLE(DT_NODELABEL(radio), coex)
#else
#error "Unsupported board: see README and check /zephyr,user node"
#define APP_REQ_GPIO_PIN 0
#define APP_GRANT_GPIO_PIN 0
#define CX_NODE DT_INVALID_NODE
#endif

#define gppi_channel_t nrf_ppi_channel_t
#define gppi_channel_alloc nrfx_ppi_channel_alloc

#define EGU0_IRQn SWI0_EGU0_IRQn
#define APP_GRANT_TIMER NRF_TIMER2
#define APP_GRANT_TIMER_ID 2
#define APP_GRANT_TIMER_CC0 0

#define EGU_INT_PRIO 4

#define STACKSIZE CONFIG_MAIN_STACK_SIZE
#define THREAD_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static const nrfx_timer_t grant_timer = NRFX_TIMER_INSTANCE(APP_GRANT_TIMER_ID);
static volatile uint32_t radio_event_count;
static gppi_channel_t app_req_assert_ppi_channel;
static bool state_granted;
static uint8_t app_grant_gpiote_ch;
static uint8_t app_req_assert_gpiote_ch;

static gppi_channel_t allocate_gppi_channel(void)
{
	gppi_channel_t channel;

	if (gppi_channel_alloc(&channel) != NRFX_SUCCESS) {
		__ASSERT(false, "(D)PPI channel allocation error");
	}
	return channel;
}

static void config_gpiote_output(uint32_t pin,
				nrf_gpiote_polarity_t polarity,
				nrf_gpiote_outinit_t init_val,
				uint8_t *channel_gpiote)
{
	if (nrfx_gpiote_channel_alloc(channel_gpiote) != NRFX_SUCCESS) {
		__ASSERT(false, "GPIOTE channel output allocation error");
	}
	nrf_gpiote_task_configure(NRF_GPIOTE, *channel_gpiote, pin,
					polarity, init_val);
	nrf_gpiote_task_enable(NRF_GPIOTE, *channel_gpiote);
}

static void config_gpiote_input(uint32_t pin,
				nrf_gpiote_polarity_t polarity,
				uint8_t *channel_gpiote)
{
	if (nrfx_gpiote_channel_alloc(channel_gpiote) != NRFX_SUCCESS) {
		__ASSERT(false, "GPIOTE channel input allocation error");
	}
	nrf_gpiote_event_configure(NRF_GPIOTE, *channel_gpiote, pin, polarity);
	nrf_gpiote_event_enable(NRF_GPIOTE, *channel_gpiote);
}

static void counter_handler(const void *context)
{
	radio_event_count++;
	nrf_egu_event_clear(NRF_EGU0, NRF_EGU_EVENT_TRIGGERED0);
}

static void setup_radio_event_counter(void)
{
	/*
	 * This function configures the EGU to pends an EGU interrupt every time the
	 * RADIO->EVENTS_READY so that we can count the number of events.
	 */
	IRQ_CONNECT(EGU0_IRQn, EGU_INT_PRIO, counter_handler, NULL, 0);
	nrf_egu_int_enable(NRF_EGU0, NRF_EGU_INT_TRIGGERED0);
	NVIC_EnableIRQ(EGU0_IRQn);

	gppi_channel_t channel = allocate_gppi_channel();

	nrfx_gppi_channel_endpoints_setup(channel,
		nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_READY),
		nrf_egu_task_address_get(NRF_EGU0, NRF_EGU_TASK_TRIGGER0));
	nrfx_gppi_channels_enable(BIT(channel));
}

static void dummy_timer_event_handler(nrf_timer_event_t event_type, void *p_context)
{
}

static void grant_init(void)
{
	/* Setup request GPIOTE (assert) events for application */
	config_gpiote_input(APP_REQ_GPIO_PIN, NRF_GPIOTE_POLARITY_LOTOHI,
				&app_req_assert_gpiote_ch);

	/* Allocate PPI channel for request assert */
	app_req_assert_ppi_channel = allocate_gppi_channel();

	/* Setup request GPIOTE (de-assert) events for application */
	uint8_t app_req_deassert_gpiote_ch;

	config_gpiote_input(APP_REQ_GPIO_PIN, NRF_GPIOTE_POLARITY_HITOLO,
				&app_req_deassert_gpiote_ch);

	/* Allocate PPI channel for request deassert */
	gppi_channel_t app_req_deassert_ppi_channel = allocate_gppi_channel();

	/* Setup grant GPIOTE task for application */
	config_gpiote_output(APP_GRANT_GPIO_PIN,
				NRF_GPIOTE_POLARITY_LOTOHI,
				NRF_GPIOTE_INITIAL_VALUE_HIGH,
				&app_grant_gpiote_ch);

	/* Setup grant timer for application */
	nrfx_timer_config_t grant_timer_cfg = {
		.frequency = NRF_TIMER_FREQ_1MHz,
		.mode      = NRF_TIMER_MODE_TIMER,
		.p_context = NULL,
	};

	if (nrfx_timer_init(&grant_timer, &grant_timer_cfg, dummy_timer_event_handler)
		!= NRFX_SUCCESS) {
		__ASSERT(false, "Failed to initialise grant timer");
	}

	gppi_channel_t app_grant_timer_ppi_channel = allocate_gppi_channel();

	/* Configure the grant timer to start when the request pin is raised. */
	nrf_timer_cc_set(APP_GRANT_TIMER, APP_GRANT_TIMER_CC0, DT_PROP(COEX_NODE, type_delay_us));
	nrf_timer_shorts_enable(APP_GRANT_TIMER,
		NRF_TIMER_SHORT_COMPARE0_STOP_MASK | NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);
	nrfx_gppi_channel_endpoints_setup(app_grant_timer_ppi_channel,
		nrf_gpiote_event_address_get(NRF_GPIOTE,
			nrf_gpiote_in_event_get(app_req_assert_gpiote_ch)),
		nrf_timer_task_address_get(APP_GRANT_TIMER, NRF_TIMER_TASK_START));
	nrfx_gppi_channels_enable(BIT(app_grant_timer_ppi_channel));

	/* Start with request granted */
	nrfx_gppi_channel_endpoints_setup(app_req_assert_ppi_channel,
		nrf_timer_event_address_get(APP_GRANT_TIMER, NRF_TIMER_EVENT_COMPARE0),
		nrf_gpiote_task_address_get(NRF_GPIOTE,
			nrf_gpiote_set_task_get(app_grant_gpiote_ch)));
	nrfx_gppi_channels_enable(BIT(app_req_assert_ppi_channel));
	radio_event_count = 0;
	state_granted = true;

	/* Make grant return-to-one */
	nrfx_gppi_channel_endpoints_setup(app_req_deassert_ppi_channel,
		nrf_gpiote_event_address_get(NRF_GPIOTE,
			nrf_gpiote_in_event_get(app_req_deassert_gpiote_ch)),
		nrf_gpiote_task_address_get(NRF_GPIOTE,
			nrf_gpiote_out_task_get(app_grant_gpiote_ch)));
	nrfx_gppi_channels_enable(BIT(app_req_deassert_ppi_channel));
}

static void config_pta_to_always_grant_request(void)
{
	nrfx_gppi_channels_disable(BIT(app_req_assert_ppi_channel));
	nrfx_gppi_task_endpoint_setup(app_req_assert_ppi_channel,
		nrf_gpiote_task_address_get(NRF_GPIOTE,
			nrf_gpiote_set_task_get(app_grant_gpiote_ch)));
	nrfx_gppi_channels_enable(BIT(app_req_assert_ppi_channel));
}

static void config_pta_to_always_deny_request(void)
{
	nrfx_gppi_channels_disable(BIT(app_req_assert_ppi_channel));
	nrfx_gppi_task_endpoint_setup(app_req_assert_ppi_channel,
		nrf_gpiote_task_address_get(NRF_GPIOTE,
			nrf_gpiote_clr_task_get(app_grant_gpiote_ch)));
	nrfx_gppi_channels_enable(BIT(app_req_assert_ppi_channel));
}

static void print_state(void)
{
	if (state_granted)
		printk("Current state is: Grant every request\n");
	else
		printk("Current state is: Deny every request\n");
}

static void console_setup(void)
{
	if (console_init()) {
		__ASSERT(false, "Failed to initialise console");
	}

	printk("-----------------------------------------------------\n");
	printk("Press a key to change state:\n");
	printk("'g' to grant every request\n");
	printk("'d' to deny every request\n");
	printk("-----------------------------------------------------\n");
	print_state();
}

static void check_input(void)
{
	char input_char;
	bool new_state_granted = state_granted;

	input_char = console_getchar();
	if (input_char == 'g') {
		new_state_granted = true;
	} else if (input_char == 'd') {
		new_state_granted = false;
	} else {
		return;
	}

	if (state_granted != new_state_granted) {
		if (new_state_granted) {
			config_pta_to_always_grant_request();
		} else {
			config_pta_to_always_deny_request();
		}

		state_granted = new_state_granted;
		print_state();
		radio_event_count = 0;
	}
}

static void console_print_thread(void)
{
	while (1) {
		printk("Number of radio events last second: %d\n", radio_event_count);
		radio_event_count = 0;
		k_sleep(K_MSEC(1000));
	}
}

void main(void)
{
	printk("Starting Radio Coex Demo 3wire on board %s\n", CONFIG_BOARD);

	if (bt_enable(NULL)) {
		printk("Bluetooth init failed");
		return;
	}
	printk("Bluetooth initialized\n");

	setup_radio_event_counter();

	grant_init();

	if (bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0)) {
		printk("Advertising failed to start");
		return;
	}
	printk("Advertising started\n");

	console_setup();

	while (1) {
		check_input();
		k_sleep(K_MSEC(100));
	}
}

K_THREAD_DEFINE(console_print_thread_id, STACKSIZE, console_print_thread,
		NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
