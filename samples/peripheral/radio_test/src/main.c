/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/printk.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/pm/device_runtime.h>
#if defined(NRF54L15_XXAA)
#include <hal/nrf_clock.h>
#endif /* defined(NRF54L15_XXAA) */
#if defined(CONFIG_CLOCK_CONTROL_NRF_COMMON)
#include <hal/nrf_lrcconf.h>
#endif
#include <nrfx.h>
#if NRF_ERRATA_STATIC_CHECK(54L, 20)
#include <hal/nrf_power.h>
#endif /* NRF_ERRATA_STATIC_CHECK(54L, 20) */
#if defined(NRF54LM20A_XXAA)
#include <hal/nrf_clock.h>
#endif /* defined(NRF54LM20A_XXAA) */

/* Empty trim value */
#define TRIM_VALUE_EMPTY 0xFFFFFFFF

#if defined(CONFIG_CLOCK_CONTROL_NRF) || defined(CONFIG_CLOCK_CONTROL_NRFX_COMMON)
static void clock_init(void)
{
	int err;
	int res;
	struct onoff_client clk_cli;
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *clk_mgr;

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!clk_mgr) {
		printk("Unable to get the Clock manager\n");
		return;
	}

	sys_notify_init_spinwait(&clk_cli.notify);

	err = onoff_request(clk_mgr, &clk_cli);
#else
	static const struct device *dev = DEVICE_DT_GET_ONE(COND_CODE_1(NRF_CLOCK_HAS_HFCLK,
								(nordic_nrf_clock_hfclk),
								(nordic_nrf_clock_xo)));

	if (!dev) {
		printk("Unable to get the Clock device\n");
		return;
	}

	sys_notify_init_spinwait(&clk_cli.notify);

	err = nrf_clock_control_request(dev, NULL, &clk_cli);
#endif
	if (err < 0) {
		printk("Clock request failed: %d\n", err);
		return;
	}

	do {
		err = sys_notify_fetch_result(&clk_cli.notify, &res);
		if (!err && res) {
			printk("Clock could not be started: %d\n", res);
			return;
		}
	} while (err);

#if NRF_ERRATA_STATIC_CHECK(54L, 20)
	if (NRF_ERRATA_DYNAMIC_CHECK(54L, 20)) {
		nrf_power_task_trigger(NRF_POWER, NRF_POWER_TASK_CONSTLAT);
	}
#endif /* NRF_ERRATA_STATIC_CHECK(54L, 20) */

#if defined(NRF54LM20A_XXAA)
	/* MLTPAN-39 */
	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_PLLSTART);
#endif /* defined(NRF54LM20A_XXAA) */

	printk("Clock has started\n");
}

#elif defined(CONFIG_CLOCK_CONTROL_NRF_COMMON)

static void clock_init(void)
{
	int err;
	int res;
	const struct device *radio_clk_dev =
		DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(DT_NODELABEL(radio)));
	struct onoff_client radio_cli;

	/** Keep radio domain powered all the time to reduce latency. */
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_1, true);

	sys_notify_init_spinwait(&radio_cli.notify);

	err = nrf_clock_control_request(radio_clk_dev, NULL, &radio_cli);

	do {
		err = sys_notify_fetch_result(&radio_cli.notify, &res);
		if (!err && res) {
			printk("Clock could not be started: %d\n", res);
			return;
		}
	} while (err == -EAGAIN);

	printk("Clock has started\n");
}

#if NRF_ERRATA_STATIC_CHECK(54H, 103)
static void nrf54hx_radio_trim(void)
{
	if (NRF_ERRATA_DYNAMIC_CHECK(54H, 103)) {
		/* Apply HMPAN-103 workaround for nRF54H series*/
		if ((*(volatile uint32_t *) 0x5302C8A0UL == 0x80000000UL) ||
		    (*(volatile uint32_t *) 0x5302C8A0UL == 0x0058120EUL)) {
			*(volatile uint32_t *) 0x5302C8A0UL = 0x0058090EUL;
		}

		*(volatile uint32_t *) 0x5302C8A4UL = 0x00F8AA5FUL;
		*(volatile uint32_t *) 0x5302C8A8UL = 0x00C00030UL;
		*(volatile uint32_t *) 0x5302C8ACUL = 0x00A80030UL;
		*(volatile uint32_t *) 0x5302C7ACUL = 0x8672827AUL;
		*(volatile uint32_t *) 0x5302C7B0UL = 0x7E768672UL;
		*(volatile uint32_t *) 0x5302C7B4UL = 0x0406007EUL;
		*(volatile uint32_t *) 0x5302C7E4UL = 0x0412C384UL;
	}
}
#endif /* NRF_ERRATA_STATIC_CHECK(54H, 103) */

#if NRF_ERRATA_STATIC_CHECK(54H, 229)
static void nrf54hx_rssi_offset_adjust(void)
{
	if (NRF_ERRATA_DYNAMIC_CHECK(54H, 229)) {
		/* Apply HMPAN-229 workaround for nRF54H series */
		if (*(volatile uint32_t *)0x0FFFE46CUL == 0x0UL) {
			*(volatile uint32_t *)0x5302C7D8UL = 0x00000004UL;
		}
	}
}
#endif /* NRF_ERRATA_STATIC_CHECK(54H, 229) */

#else
BUILD_ASSERT(false, "No Clock Control driver");
#endif /* defined(CONFIG_CLOCK_CONTROL_NRF) */

int main(void)
{
	printk("Starting Radio Test sample\n");

#if defined(CONFIG_SOC_SERIES_NRF54H)
	const struct device *console_uart = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_console));
	const struct device *shell_uart = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_shell_uart));

	if (console_uart != NULL) {
		int ret = pm_device_runtime_get(console_uart);

		if (ret < 0) {
			printk("Failed to get console UART runtime PM: %d\n", ret);
		}
	}

	if (shell_uart != NULL && shell_uart != console_uart) {
		int ret = pm_device_runtime_get(shell_uart);

		if (ret < 0) {
			printk("Failed to get shell UART runtime PM: %d\n", ret);
		}
	}
#endif /* defined(CONFIG_SOC_SERIES_NRF54H) */

	clock_init();

#if NRF_ERRATA_STATIC_CHECK(54H, 103)
	nrf54hx_radio_trim();
#endif /* NRF_ERRATA_STATIC_CHECK(54H, 103) */

#if NRF_ERRATA_STATIC_CHECK(54H, 229)
	nrf54hx_rssi_offset_adjust();
#endif /* NRF_ERRATA_STATIC_CHECK(54H, 229) */

	return 0;
}
