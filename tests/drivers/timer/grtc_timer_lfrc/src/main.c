/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <hal/nrf_grtc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include<dk_buttons_and_leds.h>

#define REQUEST_SERVING_WAIT_TIME_US 10000
#if defined(CONFIG_COVERAGE)
#define DIFF_TOLERANCE 2
#else
#define DIFF_TOLERANCE 1
#endif
#define NUMBER_OF_REPETITIONS 5

typedef enum {
	NO_CHANGE = 0,
	CHANGE_TO_SYNTH = 1,
	CHANGE_TO_LFRC = 2
} lfclk_source_change_option;

struct accuracy_test_limit {
	uint32_t grtc_timer_count_time_ms;
	uint32_t time_delta_abs_tolerance_us;
};

static volatile uint8_t compare_isr_call_counter;
static volatile uint64_t compare_count_value;
static volatile uint64_t reference_timer_value_us;

static int32_t grtc_channel;

const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_ALIAS(tst_timer));
const struct device *const lfclk_dev = DEVICE_DT_GET(DT_NODELABEL(lfclk));
const struct device *const fll16m_dev = DEVICE_DT_GET(DT_NODELABEL(fll16m));

const struct nrf_clock_spec lfclk_lfrc_mode = {
	.frequency = 32768,
	.accuracy = 0,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

const struct nrf_clock_spec lfclk_synth_mode = {
	.frequency = 32768,
	.accuracy = 30,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

const struct nrf_clock_spec fll16m_bypass_mode = {
	.frequency = MHZ(16),
	.accuracy = 30,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

static int request_clock_spec(const struct device *clk_dev, const struct nrf_clock_spec *clk_spec)
{
	int ret = 0;
	int res = 0;
	struct onoff_client cli;
	uint32_t rate;

	printk("Clock: %s, requesting frequency=%d, accuracy=%d, precision=%d\n", clk_dev->name,
	       clk_spec->frequency, clk_spec->accuracy, clk_spec->precision);
	sys_notify_init_spinwait(&cli.notify);
	ret = nrf_clock_control_request(clk_dev, clk_spec, &cli);
	if (!(ret >= 0 && ret <= 2)) {
		return -1;
	}
	do {
		ret = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (ret == -EAGAIN);
	if (ret != 0) {
		return -2;
	}
	if (res != 0) {
		return -3;
	}
	ret = clock_control_get_rate(clk_dev, NULL, &rate);
	if (ret != -ENOSYS) {
		if (ret != 0) {
			return -4;
		}
		if (rate != clk_spec->frequency) {
			return -5;
		};
		k_busy_wait(REQUEST_SERVING_WAIT_TIME_US);
	}

	return 0;
}

static void timer_compare_interrupt_continous(int32_t id, uint64_t expire_time, void *user_data)
{

	int err;
	int key;
	uint32_t *timer_count_time_ms = (uint32_t *)user_data;
	uint64_t test_ticks = 0;

	if(compare_isr_call_counter % 2)
	{
		dk_set_led_on(DK_LED1);
	}
	else
	{
		dk_set_led_off(DK_LED1);
	}
	compare_isr_call_counter++;

	key = z_nrf_grtc_timer_compare_int_lock(grtc_channel);
	test_ticks = z_nrf_grtc_timer_get_ticks(K_MSEC(*timer_count_time_ms));
	err = z_nrf_grtc_timer_set(grtc_channel, test_ticks, timer_compare_interrupt_continous, (void *)timer_count_time_ms);
	if (err != 0) {
		printk("Unexpected error raised when setting timer, err: %d\n", err);
		return;
	}

	z_nrf_grtc_timer_compare_int_unlock(grtc_channel, key);
}

static void grtc_continous_count(int32_t channel, uint32_t timer_count_time_ms)
{
	int err;
	uint64_t test_ticks = 0;
	uint64_t compare_value = 0;

	test_ticks = z_nrf_grtc_timer_get_ticks(K_MSEC(timer_count_time_ms));
	err = z_nrf_grtc_timer_set(channel, test_ticks, timer_compare_interrupt_continous, (void *)&timer_count_time_ms);
	if (err != 0) {
		printk("Unexpected error raised when setting timer, err: %d\n", err);
		return;
	}
	z_nrf_grtc_timer_compare_read(channel, &compare_value);
	if (K_TIMEOUT_EQ(K_TICKS(compare_value), K_TICKS(test_ticks)) == false) {
		printk("Compare register set failed\n");
		return;
	}

	printk("grtc_continous_count START\n");

	k_sleep(K_FOREVER);

}

int main(void)
{
	int ret = 0;

	ret = request_clock_spec(fll16m_dev, &fll16m_bypass_mode);
	if (ret != 0) {
		printk("Clock request failed: %d\n", ret);
		return -1;
	}

	grtc_channel = z_nrf_grtc_timer_chan_alloc();

	printk("Allocated GRTC channel %d\n", grtc_channel);
	if (grtc_channel < 0) {
		printk("Failed to allocate GRTC channel, chan=%d\n", grtc_channel);
		return -2;
	}

	dk_leds_init();
	dk_set_led_off(DK_LED1);
	dk_set_led_off(DK_LED2);

	grtc_continous_count(grtc_channel, 500);

	z_nrf_grtc_timer_chan_free(grtc_channel);

	return 0;
}
