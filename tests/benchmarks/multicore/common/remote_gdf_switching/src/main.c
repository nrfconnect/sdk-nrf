/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/cache.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/kernel.h>

#define HOST_IS_READY	(1)
#define REMOTE_IS_READY	(2)
#define SHM_START_ADDR		(DT_REG_ADDR(DT_NODELABEL(cpuapp_cpurad_ipc_shm)))
volatile static uint32_t *shared_var = (volatile uint32_t *) SHM_START_ADDR;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

const uint32_t freq[] = {320, 256, 128, 64};

/*
 * Set Global Domain frequency (HSFLL120)
 */
void set_global_domain_frequency(uint32_t freq)
{
	int err;
	int res;
	struct onoff_client cli;
	const struct device *hsfll_dev = DEVICE_DT_GET(DT_NODELABEL(hsfll120));
	const struct nrf_clock_spec clk_spec_global_hsfll = {.frequency = MHZ(freq)};

	printk("Requested frequency [Hz]: %d\n", clk_spec_global_hsfll.frequency);
	sys_notify_init_spinwait(&cli.notify);
	err = nrf_clock_control_request(hsfll_dev, &clk_spec_global_hsfll, &cli);
	__ASSERT((err >= 0 && err < 3), "Wrong nrf_clock_control_request return code");
	do {
		err = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (err == -EAGAIN);
	__ASSERT(err == 0, "Wrong clock control request return code");
	__ASSERT(res == 0, "Wrong clock control request response");
}

int main(void)
{
	uint32_t counter = 0;
	int32_t sync_timeout = 100;

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	printk("REMOTE starts");
	while (*shared_var != HOST_IS_READY && sync_timeout > 0) {
		k_msleep(1);
		sys_cache_data_invd_range((void *) shared_var, sizeof(*shared_var));
		printk("shared_var is: %u", *shared_var);
		sync_timeout--;
	}
	printk("REMOTE found that HOST_IS_READY or sync_timeout: %d too low", sync_timeout);
	*shared_var = REMOTE_IS_READY;
	sys_cache_data_flush_range((void *) shared_var, sizeof(*shared_var));
	printk("REMOTE wrote REMOTE_IS_READY: %u", *shared_var);
	printk("REMOTE continues");
	k_msleep(300);

	while (1) {
		gpio_pin_set_dt(&led, 1);
		set_global_domain_frequency(freq[counter++ % ARRAY_SIZE(freq)]);
		k_busy_wait(1000);
		gpio_pin_set_dt(&led, 0);
		k_msleep(2000);
	}

	return 0;
}
