/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <helpers/nrfx_gppi.h>
#include <hal/nrf_ipct.h>
#include <nrfx_timer.h>

#if defined(CONFIG_SOC_NRF54H20_CPUAPP)
LOG_MODULE_REGISTER(App);

#define TIME_TO_WAIT_US (uint32_t)(3000000)

static void t131_handler(nrf_timer_event_t event_type, void * p_context)
{
	LOG_INF("T020 and T130 started");
}

int main(void)
{
	uint8_t t131_to_t130_and_ipct_chan;
	uint8_t ipct_to_t130_chan;
	nrfx_err_t status;
	nrfx_timer_t t131 = NRFX_TIMER_INSTANCE(131);
	nrfx_timer_t t130 = NRFX_TIMER_INSTANCE(130);
	uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(t131.p_reg);
	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);

    	LOG_INF("Application start");
    	/* Timers configuration. */
	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER131), DT_IRQ(DT_NODELABEL(timer131), priority),
		    nrfx_timer_131_irq_handler, 0, 0);
	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
	timer_config.p_context = &t131;
	status = nrfx_timer_init(&t131, &timer_config, t131_handler);
	NRFX_ASSERT(status == NRFX_SUCCESS);
	status = nrfx_timer_init(&t130, &timer_config, NULL);
	NRFX_ASSERT(status == NRFX_SUCCESS);

	nrfx_timer_clear(&t130);
	nrfx_timer_clear(&t131);

	uint32_t desired_ticks = nrfx_timer_us_to_ticks(&t131, TIME_TO_WAIT_US);
	nrfx_timer_compare(&t131, NRF_TIMER_CC_CHANNEL5, desired_ticks, true);
	nrfx_timer_enable(&t131);

    	/* Channel for CC5 event of TIMER131. Will be received by IPCT130 and TIMER130. */
	status = nrfx_gppi_channel_alloc(&t131_to_t130_and_ipct_chan);
	NRFX_ASSERT(status == NRFX_SUCCESS);
    	/* Channel for START task of TIMER131. Will be sent by IPCT130. */
	status = nrfx_gppi_channel_alloc(&ipct_to_t130_chan);
	NRFX_ASSERT(status == NRFX_SUCCESS);

    	/* Connection for CC5 event of TIMER131. Will be received by IPCT130 and TIMER130. */
	nrfx_gppi_channel_endpoints_setup(
		t131_to_t130_and_ipct_chan,
		nrfx_timer_compare_event_address_get(&t131, NRF_TIMER_CC_CHANNEL5),
		nrf_ipct_task_address_get(NRF_IPCT130, NRF_IPCT_TASK_SEND_4));
    	/* Connection for START task of TIMER131. Will be sent by IPCT130. */
	nrfx_gppi_channel_endpoints_setup(
		ipct_to_t130_chan,
		nrf_ipct_event_address_get(NRF_IPCT130, NRF_IPCT_EVENT_RECEIVE_3),
		nrfx_timer_task_address_get(&t130, NRF_TIMER_TASK_STOP));
	/* As the TIMER130 is "on the way" to an existing connection
	 * (DPPIC133->DPPIC130), the event can be passed using fork connection: */
	nrfx_gppi_fork_endpoint_setup(t131_to_t130_and_ipct_chan, nrfx_timer_task_address_get(&t130, NRF_TIMER_TASK_START));

	/* Enable configured connections. */
	nrfx_gppi_channels_enable(NRFX_BIT(t131_to_t130_and_ipct_chan));
	nrfx_gppi_channels_enable(NRFX_BIT(ipct_to_t130_chan));

	while(1)
	{
		uint32_t t131_val = nrfx_timer_capture(&t131, NRF_TIMER_CC_CHANNEL0);
		uint32_t t130_val = nrfx_timer_capture(&t130, NRF_TIMER_CC_CHANNEL0);
		LOG_INF("T131: %u, T130: %u", t131_val, t130_val);
		k_msleep(500);
	}
	return 0;
}

#elif defined(CONFIG_SOC_NRF54H20_CPURAD)
LOG_MODULE_REGISTER(Rad);
#define TIME_TO_WAIT_US (uint32_t)(6000000)
static uint8_t ipct_to_t020_chan;
static uint8_t t020_to_ipct_chan;

static void t020_handler(nrf_timer_event_t event_type, void * p_context)
{
	LOG_INF("T130 stopped");
}

int main(void)
{
	nrfx_err_t status;

	nrfx_timer_t timer020 = NRFX_TIMER_INSTANCE(020);
	uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer020.p_reg);
	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);

	LOG_INF("Application start");
	/* Timers configuration. */
	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
	timer_config.p_context = &timer020;
	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER020), DT_IRQ(DT_NODELABEL(timer020), priority),
		    nrfx_timer_020_irq_handler, 0, 0);

	status = nrfx_timer_init(&timer020, &timer_config, t020_handler);
	NRFX_ASSERT(status == NRFX_SUCCESS);

	nrfx_timer_clear(&timer020);
	uint32_t desired_ticks = nrfx_timer_us_to_ticks(&timer020, TIME_TO_WAIT_US);
	nrfx_timer_compare(&timer020, NRF_TIMER_CC_CHANNEL0, desired_ticks, true);

	/* Channel for START task of TIMER020. Will be sent by local IPCT. */
	status = nrfx_gppi_channel_alloc(&ipct_to_t020_chan);
	NRFX_ASSERT(status == NRFX_SUCCESS);
	/* Channel for CC0 event of TIMER020. Will be received by local IPCT. */
	status = nrfx_gppi_channel_alloc(&t020_to_ipct_chan);
	NRFX_ASSERT(status == NRFX_SUCCESS);

	/* Connection for START task of TIMER020. Will be sent by local IPCT. */
	nrfx_gppi_channel_endpoints_setup(
		ipct_to_t020_chan,
		nrf_ipct_event_address_get(NRF_IPCT, NRF_IPCT_EVENT_RECEIVE_6),
		nrfx_timer_task_address_get(&timer020, NRF_TIMER_TASK_START));

	/* Connection for CC0 event of TIMER020. Will be received by local IPCT. */
	nrfx_gppi_channel_endpoints_setup(
		t020_to_ipct_chan,
		nrfx_timer_compare_event_address_get(&timer020, NRF_TIMER_CC_CHANNEL0),
		nrf_ipct_task_address_get(NRF_IPCT, NRF_IPCT_TASK_SEND_7));

	/* Enable configured connections. */
	nrfx_gppi_channels_enable(NRFX_BIT(ipct_to_t020_chan));
	nrfx_gppi_channels_enable(NRFX_BIT(t020_to_ipct_chan));

	while(1)
	{
		uint32_t timer020_val = nrfx_timer_capture(&timer020, NRF_TIMER_CC_CHANNEL1);
		LOG_INF("T020: %u", timer020_val);
		k_msleep(500);
	}
	return 0;
}
#endif
