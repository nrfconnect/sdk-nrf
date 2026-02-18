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
#if defined(CONFIG_CLOCK_CONTROL_NRF2)
#include <hal/nrf_lrcconf.h>
#endif
#include <nrfx.h>
/* TODO: DRGN-27733 Remove the alternative condition for nRF54LS05B
 * when the errata has been applied to this chip and the errata
 * check can be used instead.
 */
#if NRF54L_ERRATA_20_PRESENT || defined(NRF54LS05B_ENGA_XXAA)
#include <hal/nrf_power.h>
#endif /* NRF54L_ERRATA_20_PRESENT */
#if defined(NRF54LM20A_XXAA)
#include <hal/nrf_clock.h>
#endif /* defined(NRF54LM20A_XXAA) */

/* Empty trim value */
#define TRIM_VALUE_EMPTY 0xFFFFFFFF

#if defined(CONFIG_CLOCK_CONTROL_NRF)
static void clock_init(void)
{
	int err;
	int res;
	struct onoff_manager *clk_mgr;
	struct onoff_client clk_cli;

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!clk_mgr) {
		printk("Unable to get the Clock manager\n");
		return;
	}

	sys_notify_init_spinwait(&clk_cli.notify);

	err = onoff_request(clk_mgr, &clk_cli);
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

#if NRF54L_ERRATA_20_PRESENT
	if (nrf54l_errata_20()) {
		nrf_power_task_trigger(NRF_POWER, NRF_POWER_TASK_CONSTLAT);
	}
	/* TODO: DRGN-27733 Remove the elif block for nRF54LS05B when the errata has
	 * been applied to this chip and the errata check can be used instead.
	 */
#elif defined(NRF54LS05B_ENGA_XXAA)
	nrf_power_task_trigger(NRF_POWER, NRF_POWER_TASK_CONSTLAT);
#endif /* NRF54L_ERRATA_20_PRESENT */

#if defined(NRF54LM20A_XXAA)
	/* MLTPAN-39 */
	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_PLLSTART);
#endif /* defined(NRF54LM20A_XXAA) */

	printk("Clock has started\n");
}

#elif defined(CONFIG_CLOCK_CONTROL_NRF2)

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

#if defined(CONFIG_SOC_SERIES_NRF54H)
static void nrf54hx_radio_trim(void)
{
	/* Apply HMPAN-102 workaround for nRF54H series */
	*(volatile uint32_t *)0x5302C7E4 =
				(((*((volatile uint32_t *)0x5302C7E4)) & 0xFF000FFF) | 0x0012C000);

	/* Apply HMPAN-18 workaround for nRF54H series - load trim values*/
	if (*(volatile uint32_t *) 0x0FFFE458 != TRIM_VALUE_EMPTY) {
		*(volatile uint32_t *) 0x5302C734 = *(volatile uint32_t *) 0x0FFFE458;
	}

	if (*(volatile uint32_t *) 0x0FFFE45C != TRIM_VALUE_EMPTY) {
		*(volatile uint32_t *) 0x5302C738 = *(volatile uint32_t *) 0x0FFFE45C;
	}

	if (*(volatile uint32_t *) 0x0FFFE460 != TRIM_VALUE_EMPTY) {
		*(volatile uint32_t *) 0x5302C73C = *(volatile uint32_t *) 0x0FFFE460;
	}

	if (*(volatile uint32_t *) 0x0FFFE464 != TRIM_VALUE_EMPTY) {
		*(volatile uint32_t *) 0x5302C740 = *(volatile uint32_t *) 0x0FFFE464;
	}

	if (*(volatile uint32_t *) 0x0FFFE468 != TRIM_VALUE_EMPTY) {
		*(volatile uint32_t *) 0x5302C74C = *(volatile uint32_t *) 0x0FFFE468;
	}

	if (*(volatile uint32_t *) 0x0FFFE46C != TRIM_VALUE_EMPTY) {
		*(volatile uint32_t *) 0x5302C7D8 = *(volatile uint32_t *) 0x0FFFE46C;
	}

	if (*(volatile uint32_t *) 0x0FFFE470 != TRIM_VALUE_EMPTY) {
		*(volatile uint32_t *) 0x5302C840 = *(volatile uint32_t *) 0x0FFFE470;
	}

	if (*(volatile uint32_t *) 0x0FFFE474 != TRIM_VALUE_EMPTY) {
		*(volatile uint32_t *) 0x5302C844 = *(volatile uint32_t *) 0x0FFFE474;
	}

	if (*(volatile uint32_t *) 0x0FFFE478 != TRIM_VALUE_EMPTY) {
		*(volatile uint32_t *) 0x5302C848 = *(volatile uint32_t *) 0x0FFFE478;
	}

	if (*(volatile uint32_t *) 0x0FFFE47C != TRIM_VALUE_EMPTY) {
		*(volatile uint32_t *) 0x5302C84C = *(volatile uint32_t *) 0x0FFFE47C;
	}

	/* Apply HMPAN-103 workaround for nRF54H series*/
	if ((*(volatile uint32_t *) 0x5302C8A0 == 0x80000000) ||
		(*(volatile uint32_t *) 0x5302C8A0 == 0x0058120E)) {
		*(volatile uint32_t *) 0x5302C8A0 = 0x0058090E;
	}

	*(volatile uint32_t *) 0x5302C8A4 = 0x00F8AA5F;
	*(volatile uint32_t *) 0x5302C7AC = 0x8672827A;
	*(volatile uint32_t *) 0x5302C7B0 = 0x7E768672;
	*(volatile uint32_t *) 0x5302C7B4 = 0x0406007E;
}
#endif /* defined(CONFIG_SOC_SERIES_NRF54H) */

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

#if defined(CONFIG_SOC_SERIES_NRF54H)
	nrf54hx_radio_trim();
#endif

	return 0;
}
