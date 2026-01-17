/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/pm.h>

LOG_MODULE_REGISTER(idle_gpio);

#if IS_ENABLED(CONFIG_SOC_NRF54H20_CPUAPP)
static const struct gpio_dt_spec sw = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
#elif IS_ENABLED(CONFIG_SOC_NRF54H20_CPURAD)
static const struct gpio_dt_spec sw = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
#elif IS_ENABLED(CONFIG_SOC_NRF54H20_CPUPPR)
static const struct gpio_dt_spec sw = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);

#include <uicr/uicr.h>
#define SPU131_ADDR 0x5f920000UL

/* The input pin must be configured as NS to be allowed to trigger PPR's GPIOTE IRQ. */
UICR_SPU_FEATURE_GPIO_PIN_SET(SPU131_ADDR, DT_PROP(DT_GPIO_CTLR(DT_ALIAS(sw1), gpios), port),
			      DT_GPIO_PIN(DT_ALIAS(sw1), gpios), false, NRF_OWNER_APPLICATION);
#else
#error "Invalid core selected. "
#endif

#if DT_NODE_EXISTS(DT_PATH(zephyr_user))
static const struct gpio_dt_spec fake_rts = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), test_gpios);
#endif

static K_SEM_DEFINE(my_gpio_sem, 0, 1);

void my_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	gpio_pin_set_dt(&led, 1);
	LOG_INF("User callback for %s", CONFIG_BOARD_TARGET);
	k_sem_give(&my_gpio_sem);
}

static struct gpio_callback gpio_cb;

int main(void)
{
	unsigned int cnt = 0;
	int rc;

	rc = gpio_is_ready_dt(&led);
	if (rc < 0) {
		LOG_ERR("GPIO Device not ready (%d)", rc);
		return 0;
	}

	rc = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (rc < 0) {
		LOG_ERR("Could not configure led GPIO (%d)", rc);
		return 0;
	}

#if DT_NODE_EXISTS(DT_PATH(zephyr_user))
	rc = gpio_is_ready_dt(&fake_rts);
	if (rc < 0) {
		LOG_ERR("GPIO Device not ready (%d)", rc);
		return 0;
	}

	rc = gpio_pin_configure_dt(&fake_rts, GPIO_OUTPUT_ACTIVE);
	if (rc < 0) {
		LOG_ERR("Could not configure fake_rst GPIO (%d)", rc);
		return 0;
	}
#endif

	rc = gpio_is_ready_dt(&sw);
	if (rc < 0) {
		LOG_ERR("GPIO Device not ready (%d)", rc);
		return 0;
	}

	rc = gpio_pin_configure_dt(&sw, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Could not configure sw GPIO (%d)", rc);
		return 0;
	}

	rc = gpio_pin_interrupt_configure(sw.port, sw.pin, GPIO_INT_LEVEL_ACTIVE);
	if (rc < 0) {
		LOG_ERR("Could not configure sw GPIO interrupt (%d)", rc);
		return 0;
	}
	gpio_init_callback(&gpio_cb, my_gpio_callback, 0xFFFF);
	gpio_add_callback(sw.port, &gpio_cb);
	LOG_INF("Multicore idle_gpio test on %s", CONFIG_BOARD_TARGET);
	while (1) {
		LOG_INF("Multicore idle_gpio test iteration %u", cnt++);
		gpio_pin_set_dt(&led, 0);
		if (k_sem_take(&my_gpio_sem, K_FOREVER) != 0) {
			LOG_ERR("Failed to take a semaphore");
			return 0;
		}
		k_busy_wait(1000000);
		/* De-bounce when button is used. Reset a semaphores count to 0. */
		k_sem_reset(&my_gpio_sem);
	}

	return 0;
}
