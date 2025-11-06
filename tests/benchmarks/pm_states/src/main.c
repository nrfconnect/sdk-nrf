/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/pm.h>

#define PS_NODE DT_PATH(cpus, power_states)

#if DT_NODE_EXISTS(PS_NODE)
#define PS_VAL(node) DT_PROP_OR(node, min_residency_us, 0),

static const uint32_t min_residencies[] = {DT_FOREACH_CHILD(PS_NODE, PS_VAL)};

#define PS_COUNT ARRAY_SIZE(min_residencies)
#else
static const uint32_t min_residencies[] = {};
#define PS_COUNT 0
#endif

static const struct gpio_dt_spec leds[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),
};
#define LED_COUNT ARRAY_SIZE(leds)

static void notify_pm_state_entry(enum pm_state state)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		gpio_pin_set_dt(&leds[0], 1);
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		gpio_pin_set_dt(&leds[2], 1);
		break;
	default:
		__ASSERT(true, "Unexpected PM state: %d", state);
		break;
	}
}

static void notify_pm_state_exit(enum pm_state state)
{
	for (size_t i = 0; i < LED_COUNT; i++) {
		gpio_pin_set_dt(&leds[i], 0);
	}
}

static struct pm_notifier notifier = {
	.state_entry = notify_pm_state_entry,
	.state_exit = notify_pm_state_exit,
};

int main(void)
{
	int ret;

	for (size_t i = 0; i < LED_COUNT; i++) {
		ret = gpio_is_ready_dt(&leds[i]);
		__ASSERT(ret, "Error: GPIO Device not ready");
		ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
		__ASSERT(ret == 0, "Could not configure led GPIO");
	}

	pm_notifier_register(&notifier);

	k_msleep(1500);

	if (PS_COUNT != 0) {
		while (1) {
			k_usleep(min_residencies[0] - 100);
			for (size_t i = 0; i < PS_COUNT; i++) {
				k_usleep(min_residencies[i] + 100);
			}
			k_busy_wait(400);
		}
	}

	return 0;
}
