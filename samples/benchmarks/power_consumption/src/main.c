/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#include <hal/nrf_lfxo.h>

#if defined(CONFIG_RAM_POWER_DOWN_LIBRARY)
#include <ram_pwrdn.h>
#endif

#define IDLE_TIME K_SECONDS(CONFIG_SAMPLE_POWER_CONSUMPTION_IDLE_SECONDS)

/*
 * LFXO.TRIM.PIXO and LFXO.MIRROR are not in the public HAL struct for
 * this SoC -- compute their addresses from the real NRF_LFXO pointer,
 * matching the pattern already proven out in the diagnostic
 * investigation that found this fix.
 */
#define LFXO_TRIM_PIXO_ADDR ((uintptr_t)NRF_LFXO + 0x44CU)
#define LFXO_MIRROR_ADDR    ((uintptr_t)NRF_LFXO + 0x480U)

static void configure_lfxo_trim(void)
{
	/*
	 * TRIM.PIXO is mirrored (reset-locked); unlock, set the PIXO
	 * pulse-injection supply trim to its maximum amplitude (+15, 2's
	 * complement, VAL bits [4:0]), then relock. Boosting this away
	 * from the default (0, 202 mV) makes the PIXO<->Pierce handoff
	 * that CONFIG_PM's idle hook performs every cycle (see
	 * soc/nordic/nrf71/soc_power.c) reliably succeed instead of
	 * intermittently or permanently failing.
	 */
	*(volatile uint32_t *)LFXO_MIRROR_ADDR = 0;
	*(volatile uint32_t *)LFXO_TRIM_PIXO_ADDR = 0x0FU;
	*(volatile uint32_t *)LFXO_MIRROR_ADDR = 1U;
}

static void configure_ram_retention(void)
{
#if defined(CONFIG_SAMPLE_POWER_CONSUMPTION_RAM_RETAIN_UNUSED_ONLY)
	power_down_unused_ram();
#elif defined(CONFIG_SAMPLE_POWER_CONSUMPTION_RAM_RETAIN_64K) || \
	defined(CONFIG_SAMPLE_POWER_CONSUMPTION_RAM_RETAIN_128K) || \
	defined(CONFIG_SAMPLE_POWER_CONSUMPTION_RAM_RETAIN_256K) || \
	defined(CONFIG_SAMPLE_POWER_CONSUMPTION_RAM_RETAIN_512K)
	uintptr_t ram_start = DT_REG_ADDR(DT_CHOSEN(zephyr_sram));
	uintptr_t ram_end = ram_start + DT_REG_SIZE(DT_CHOSEN(zephyr_sram));

#if defined(CONFIG_SAMPLE_POWER_CONSUMPTION_RAM_RETAIN_64K)
	uintptr_t retained_size = 64U * 1024U;
#elif defined(CONFIG_SAMPLE_POWER_CONSUMPTION_RAM_RETAIN_128K)
	uintptr_t retained_size = 128U * 1024U;
#elif defined(CONFIG_SAMPLE_POWER_CONSUMPTION_RAM_RETAIN_256K)
	uintptr_t retained_size = 256U * 1024U;
#elif defined(CONFIG_SAMPLE_POWER_CONSUMPTION_RAM_RETAIN_512K)
	uintptr_t retained_size = 512U * 1024U;
#endif

	power_down_ram(ram_start + retained_size, ram_end);
#endif
	/* CONFIG_SAMPLE_POWER_CONSUMPTION_RAM_RETAIN_FULL (default): no
	 * power-down call at all -- every byte of application RAM stays retained.
	 */
}

int main(void)
{
#if defined(CONFIG_SERIAL)
	const struct device *const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	int err;

	if (!device_is_ready(console)) {
		return 0;
	}

	printk("\nPower consumption demo ready.\n"
	       "Switch the board's power off and back on now to get correct "
	       "power consumption readings.\n\n");
#endif

	configure_ram_retention();
	configure_lfxo_trim();

#if defined(CONFIG_SERIAL)
	uint32_t wakeups = 0;
#endif

	while (true) {
#if defined(CONFIG_SERIAL)
		/* Suspend the console (UART) before sleeping -- otherwise it
		 * stays fully active (and keeps whatever clock it depends on
		 * requested) for the whole "idle" window, which is the
		 * single biggest cause of elevated idle current in a sample
		 * like this.
		 */
		err = pm_device_action_run(console, PM_DEVICE_ACTION_SUSPEND);
		if (err != 0) {
			printk("Failed to suspend console: %d\n", err);
			return 0;
		}
#endif

		k_sleep(IDLE_TIME);

#if defined(CONFIG_SERIAL)
		err = pm_device_action_run(console, PM_DEVICE_ACTION_RESUME);
		if (err != 0) {
			return 0;
		}

		wakeups++;
		printk("Woken up %u time(s)\n", wakeups);
#endif
	}

	return 0;
}
