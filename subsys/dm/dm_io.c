/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "dm_io.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#if IS_ENABLED(CONFIG_DM_GPIO_DEBUG)
#define HAS_RANGING_GPIO DT_NODE_HAS_PROP(DT_PATH(dm_gpio, dm_ranging), gpios)
#define HAS_ADD_REQUEST_GPIO DT_NODE_HAS_PROP(DT_PATH(dm_gpio, dm_add_request), gpios)

BUILD_ASSERT(HAS_ADD_REQUEST_GPIO,
	"You must set the gpios property in the /dm_gpio/dm-ranging node to enable GPIO Debug.");
BUILD_ASSERT(HAS_RANGING_GPIO,
	"You must set the gpios property in the /dm_gpio/dm-add-request node to enable GPIO Debug.");

#define DM_IO_RANGING_NODE	DT_NODELABEL(dm_ranging)
#define DM_IO_ADD_REQUEST_NODE	DT_NODELABEL(dm_add_request)

static const struct gpio_dt_spec dm_io_ranging = GPIO_DT_SPEC_GET(DM_IO_RANGING_NODE, gpios);
static const struct gpio_dt_spec dm_io_add_request =
					     GPIO_DT_SPEC_GET(DM_IO_ADD_REQUEST_NODE, gpios);
#endif /* IS_ENABLED(CONFIG_DM_GPIO_DEBUG) */

static void dm_io_set_value(enum dm_io_output out, int value)
{
#if IS_ENABLED(CONFIG_DM_GPIO_DEBUG)
	switch (out) {
	case DM_IO_RANGING:
		gpio_pin_set_dt(&dm_io_ranging, value);
		break;
	case DM_IO_ADD_REQUEST:
		gpio_pin_set_dt(&dm_io_add_request, value);
		break;
	default:
		break;
	}
#endif /* IS_ENABLED(CONFIG_DM_GPIO_DEBUG) */
}

int dm_io_init(void)
{
#if IS_ENABLED(CONFIG_DM_GPIO_DEBUG)
	int ret;

	if (!device_is_ready(dm_io_ranging.port) ||
		!device_is_ready(dm_io_add_request.port)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&dm_io_ranging, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&dm_io_add_request, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return -ENODEV;
	}
#endif /* IS_ENABLED(CONFIG_DM_GPIO_DEBUG) */

	return 0;
}

void dm_io_set(enum dm_io_output out)
{
	dm_io_set_value(out, 1);
}

void dm_io_clear(enum dm_io_output out)
{
	dm_io_set_value(out, 0);
}
