/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <nrfx_gpiote.h>
#include <sdfw/sdfw_services/suit_service.h>

#define RECOVERY_BUTTON_NODE DT_CHOSEN(ncs_recovery_button)
#define RECOVERY_BUTTON_PIN DT_GPIO_PIN(RECOVERY_BUTTON_NODE, gpios)
#define RECOVERY_BUTTON_PORT_NUM DT_PROP(DT_GPIO_CTLR(RECOVERY_BUTTON_NODE, gpios), port)
#define RECOVERY_BUTTON_FLAGS DT_GPIO_FLAGS(RECOVERY_BUTTON_NODE, gpios)

#define RECOVERY_BUTTON_ABS_PIN NRF_GPIO_PIN_MAP(RECOVERY_BUTTON_PORT_NUM, RECOVERY_BUTTON_PIN)
#define RECOVERY_BUTTON_PULL (RECOVERY_BUTTON_FLAGS & GPIO_PULL_UP ? NRF_GPIO_PIN_PULLUP : \
			      RECOVERY_BUTTON_FLAGS & GPIO_PULL_DOWN ? NRF_GPIO_PIN_PULLDOWN : \
			      NRF_GPIO_PIN_NOPULL)

#define RECOVERY_BUTTON_PRESSED(pin_value) (RECOVERY_BUTTON_FLAGS & GPIO_ACTIVE_LOW ? (!pin_value) \
					    : pin_value)

BUILD_ASSERT(DT_NODE_EXISTS(DT_CHOSEN(ncs_recovery_button)), "No recovery button chosen in dts");

static int recovery_button_check(void)
{
	suit_boot_mode_t mode = SUIT_BOOT_MODE_INVOKE_RECOVERY;
	suit_ssf_err_t err = SUIT_PLAT_SUCCESS;
	int ret = 0;

	err = suit_boot_mode_read(&mode);

	if (err != SUIT_PLAT_SUCCESS) {
		suit_invoke_confirm(-EPIPE);
		return -EPIPE;
	}

	/** Using the recovery button makes sense in two cases:
	 *  1. From a companion application during SUIT manifest processing while the device
	 *     is booting(mode == SUIT_BOOT_MODE_INVOKE).
	 *  2. From the main application, when the device is already booted
	 *     (mode == SUIT_BOOT_MODE_POST_INVOKE).
	 */
	if (mode == SUIT_BOOT_MODE_INVOKE
		|| mode == SUIT_BOOT_MODE_POST_INVOKE) {
		nrf_gpio_pin_dir_t dir = NRF_GPIO_PIN_DIR_INPUT;
		nrf_gpio_pin_input_t input = NRF_GPIO_PIN_INPUT_CONNECT;
		nrf_gpio_pin_pull_t pull = RECOVERY_BUTTON_PULL;

		nrf_gpio_reconfigure(RECOVERY_BUTTON_ABS_PIN, &dir, &input, &pull, NULL, NULL);

		uint32_t pin_value = nrf_gpio_pin_read(RECOVERY_BUTTON_ABS_PIN);

		if (RECOVERY_BUTTON_PRESSED(pin_value)) {
			err = suit_foreground_dfu_required();
		}
	}

	if (err != SUIT_PLAT_SUCCESS) {
		ret = -EPIPE;
	}

	if (mode == SUIT_BOOT_MODE_INVOKE
		|| mode == SUIT_BOOT_MODE_INVOKE_FOREGROUND_DFU
		|| mode == SUIT_BOOT_MODE_INVOKE_RECOVERY) {
		(void)suit_invoke_confirm(ret);
	}

	return ret;
}

SYS_INIT(recovery_button_check, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
