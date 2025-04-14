/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <hal/nrf_grtc.h>

#define TIMER_COUNT_TIME_MS		 995
#define SLEEP_TIME_MS			 1500
/*
 * The timer value readout is not time synchronised,
 * this tolerance accounts for the readout delay
 */
#define READOUT_TIMER_TOLERANCE_IN_TICKS 1500
#define EXPECTED_READOUT_VALUE                                                                     \
	(sys_clock_hw_cycles_per_sec() / 1000) * (SLEEP_TIME_MS - TIMER_COUNT_TIME_MS)

static volatile uint64_t isr_count_value;
static volatile bool count_isr_triggered;

static void timer_compare_interrupt_handler(int32_t id, uint64_t expire_time, void *user_data)
{
	isr_count_value = z_nrf_grtc_timer_read();
	count_isr_triggered = true;
}

int main(void)
{
	int err;
	uint64_t test_ticks = 0;
	uint64_t compare_value = 0;
	uint64_t count_difference = 0;
	char user_data[] = "test_timer_count_in_compare_mode\n";
	int32_t channel = z_nrf_grtc_timer_chan_alloc();

	__ASSERT(channel > 0, "GRTC channel not allocated\n");
	while (1) {
		count_isr_triggered = false;
		test_ticks = z_nrf_grtc_timer_get_ticks(K_MSEC(TIMER_COUNT_TIME_MS));
		err = z_nrf_grtc_timer_set(channel, test_ticks, timer_compare_interrupt_handler,
					   (void *)user_data);

		__ASSERT(err == 0, "z_nrf_grtc_timer_set raised an error: %d", err);

		z_nrf_grtc_timer_compare_read(channel, &compare_value);
		__ASSERT(K_TIMEOUT_EQ(K_TICKS(compare_value), K_TICKS(test_ticks)) == true,
			 "Compare register set failed");
		__ASSERT(err == 0, "Unexpected error raised when setting timer, err: %d", err);

		/* Got to sleep */
		k_sleep(K_MSEC(SLEEP_TIME_MS));

		/*
		 * Read counter value after exiting the sleep
		 * and compare it to the value captured in the ISR
		 */
		count_difference = z_nrf_grtc_timer_read() - isr_count_value;
		__ASSERT((EXPECTED_READOUT_VALUE - READOUT_TIMER_TOLERANCE_IN_TICKS <=
			  count_difference) &&
				 (count_difference <=
				  EXPECTED_READOUT_VALUE + READOUT_TIMER_TOLERANCE_IN_TICKS),
			 "GRTC timer readout after sleep returned incorrect value. Returned, "
			 "expected, abs tolerance (%lld/%d/%d)",
			 count_difference, EXPECTED_READOUT_VALUE,
			 READOUT_TIMER_TOLERANCE_IN_TICKS);
	}

	z_nrf_grtc_timer_chan_free(channel);
	return 0;
}
