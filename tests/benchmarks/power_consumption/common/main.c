/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
#if defined(CONFIG_CLOCK_CONTROL)
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#endif
#endif

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

static bool state = true;
extern void thread_definition(void);

/* Some tests require that test thread controls the moment when it is
 * suspended. In that case test implements this function and returns true to
 * indicated that test thread will take case of the suspension and it can
 * be skipped in the common code.
 */
__weak bool self_suspend_req(void)
{
	return false;
}
#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
#if defined(CONFIG_CLOCK_CONTROL)
const struct nrf_clock_spec clk_spec_local_hsfll = {
	.frequency = MHZ(CONFIG_LOCAL_DOMAIN_CLOCK_FREQUENCY_MHZ)
};

/*
 * Set Local Domain frequency (HSFLL120)
 * based on: CONFIG_LOCAL_DOMAIN_CLOCK_FREQUENCY_MHZ
 */
void set_local_domain_frequency(void)
{
	int err;
	int res;
	struct onoff_client cli;
	const struct device *hsfll_dev = DEVICE_DT_GET(DT_NODELABEL(cpuapp_hsfll));

	printk("Requested frequency [Hz]: %d\n", clk_spec_local_hsfll.frequency);
	sys_notify_init_spinwait(&cli.notify);
	err = nrf_clock_control_request(hsfll_dev, &clk_spec_local_hsfll, &cli);
	printk("Return code: %d\n", err);
	__ASSERT_NO_MSG(err < 3);
	__ASSERT_NO_MSG(err >= 0);
	do {
		err = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (err == -EAGAIN);
	printk("Clock control request return value: %d\n", err);
	printk("Clock control request response code: %d\n", res);
	__ASSERT_NO_MSG(err == 0);
	__ASSERT_NO_MSG(res == 0);
}
#endif /* CONFIG_CLOCK_CONTROL */
#endif

K_THREAD_DEFINE(thread_id, 500, thread_definition, NULL, NULL, NULL,
				5, 0, 0);

void timer_handler(struct k_timer *dummy)
{
	if (state == true) {
		state = false;
		gpio_pin_set_dt(&led, 1);
		k_thread_resume(thread_id);
	} else {
		state = true;
		gpio_pin_set_dt(&led, 0);
		if (self_suspend_req() == false) {
			k_thread_suspend(thread_id);
		}
	}
}

K_TIMER_DEFINE(timer, timer_handler, NULL);

int main(void)
{
	int rc;

	rc = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT_NO_MSG(rc == 0);

#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
#if defined(CONFIG_CLOCK_CONTROL)
	/* Wait a bit to solve NRFS request timeout issue. */
	k_msleep(100);
	set_local_domain_frequency();
#endif
#endif

	k_timer_start(&timer, K_SECONDS(1), K_SECONDS(1));

	while (1) {
		k_msleep(1000);
	}
	return 0;
}
