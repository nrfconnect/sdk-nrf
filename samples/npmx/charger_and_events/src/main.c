/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <sys/printk.h>
#include <zephyr.h>
#include <npmx_driver.h>
#include <npmx_gpio.h>

/**
 * @brief Temporary tests: Function callback for vbusin events
 * 
 * @param[in] pm   The instance of nPM device
 * @param[in] type The type of callback, should be always NPMX_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE
 * @param[in] arg  Received event mask @ref npmx_event_group_vbusin_mask_t 
 */
static void vbusin_callback(npmx_instance_t *pm, npmx_callback_type_t type, uint32_t arg)
{
	printk("vbusin %d\n", arg);
}

void main(void)
{
	const struct device *npmx_dev = DEVICE_DT_GET(DT_NODELABEL(npmx));

	if (!device_is_ready(npmx_dev)) {
		printk("NPMX device is not ready.\n");
		return;
	} else {
		printk("NPMX device ok.\n");
	}

	/* Temporary tests: get pointer to npmx device */
	npmx_instance_t *npmx_instance = &((struct npmx_data *)npmx_dev->data)->npmx_instance;

	/* Temporary tests: register callback for vbus events */
	npmx_driver_register_cb(vbusin_callback, NPMX_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE);

	/* Temporary tests: use GPIO 0 as interrupt output */
	npmx_gpio_mode_set(npmx_instance, NPMX_GPIO_INSTANCE_0, NPMX_GPIO_MODE_OUTPUT_IRQ);

	/* Temporary tests: enable interrupt from vbusin events */
	npmx_driver_event_interrupt_enable(NPMX_EVENT_GROUP_VBUSIN_VOLTAGE,
					   NPMX_EVENT_GROUP_VBUSIN_DETECTED_MASK |
						   NPMX_EVENT_GROUP_VBUSIN_REMOVED_MASK);

	while (1) {
		k_sleep(K_FOREVER);
	}
}
