/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <hal/nrf_grtc.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/poweroff.h>

#include <modem/nrf_modem_lib.h>

#include "retained.h"

/* System OFF time in seconds when GRTC wakeup is enabled */
#define SYSTEM_OFF_TIME_S 2

#define NON_WAKEUP_RESET_REASON (RESET_PIN | RESET_SOFTWARE | RESET_POR | RESET_DEBUG)

#define GRTC_NODE DT_NODELABEL(grtc)
#define GRTC_CC_CHANNEL DT_PROP_BY_IDX(GRTC_NODE, owned_channels, 0)

static const struct device *const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

#if defined(CONFIG_SAMPLE_GPIO_WAKEUP)
static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(DT_NODELABEL(button0), gpios);
#endif

static int print_reset_cause(uint32_t reset_cause)
{
	int ret;
	uint32_t supported;

	ret = hwinfo_get_supported_reset_cause((uint32_t *) &supported);

	if (ret || !(reset_cause & supported)) {
		return -ENOTSUP;
	}

	if (reset_cause & RESET_CLOCK) {
		printf("Wakeup from System OFF by GRTC\n");
	} else if (reset_cause & RESET_LOW_POWER_WAKE) {
		printf("Wakeup from System OFF by GPIO\n");
	} else  {
		printf("Other wakeup cause 0x%08X\n", reset_cause);
	}

	return 0;
}

int main(void)
{
	int ret;
	uint32_t reset_cause;

	if (!device_is_ready(console)) {
		printf("%s: device not ready.\n", console->name);
		return -1;
	}

	printf("\nSystem off sample\n");

	hwinfo_get_reset_cause(&reset_cause);
	ret = print_reset_cause(reset_cause);
	if (ret < 0) {
		printf("Reset cause not supported\n");
		return ret;
	}

	ret = nrf_modem_lib_init();
	if (ret < 0) {
		printf("Failed to initialize modem library: %d\n", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_STATE_RETENTION)) {
		bool retained_ok = retained_validate();

		if (reset_cause & NON_WAKEUP_RESET_REASON) {
			retained.boots = 0;
			retained.off_count = 0;
			retained.uptime_sum = 0;
			retained.uptime_latest = 0;
			retained_ok = true;
		}
		/* Increment for this boot attempt and update */
		retained.boots += 1;
		retained_update();

		printf("Retained data: %s\n", retained_ok ? "valid" : "INVALID");
		printf("Boot count: %u\n", retained.boots);
		printf("Off count: %u\n", retained.off_count);
		printf("Active ticks: %llu\n", retained.uptime_sum);
	} else {
		printf("State retention disabled\n");
	}

	ret = nrf_modem_lib_shutdown();
	if (ret < 0) {
		printf("Failed to shutdown modem library: %d\n", ret);
		return ret;
	}

#if defined(CONFIG_SAMPLE_GRTC_WAKEUP)
	printf("Entering System OFF; wait %u seconds to restart\n", SYSTEM_OFF_TIME_S);
	nrf_grtc_sys_counter_cc_add_set(NRF_GRTC, GRTC_CC_CHANNEL, SYSTEM_OFF_TIME_S * USEC_PER_SEC,
					NRF_GRTC_CC_ADD_REFERENCE_SYSCOUNTER);
#else
	nrf_grtc_sys_counter_compare_event_disable(NRF_GRTC, GRTC_CC_CHANNEL);
#endif /* CONFIG_SAMPLE_GRTC_WAKEUP */

#if defined(CONFIG_SAMPLE_GPIO_WAKEUP)
	/* Configure button 0 as input, interrupt as level active to allow wakeup */
	ret = gpio_pin_configure_dt(&button0, GPIO_INPUT);
	if (ret < 0) {
		printf("Could not configure button 0 GPIO (%d)\n", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&button0, GPIO_INT_LEVEL_ACTIVE);
	if (ret < 0) {
		printf("Could not configure button 0 GPIO interrupt (%d)\n", ret);
		return ret;
	}

	printf("Entering System OFF; press button 0 to restart\n");
#endif /* CONFIG_SAMPLE_GPIO_WAKEUP */

	/* Suspend console */
	do {
		enum pm_device_state state;

		ret = pm_device_state_get(console, &state);
		if (ret) {
			return ret;
		}

		if (state != PM_DEVICE_STATE_ACTIVE) {
			break;
		}

		pm_device_runtime_put(console);
	} while (1);

	if (IS_ENABLED(CONFIG_SAMPLE_STATE_RETENTION)) {
		/* Update the retained state */
		retained.off_count += 1;
		retained_update();
	}

	hwinfo_clear_reset_cause();

	sys_poweroff();

	return 0;
}
