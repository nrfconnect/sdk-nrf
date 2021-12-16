/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "dm_io.h"
#include <nrfx.h>
#include <nrfx_gpiote.h>
#include <debug/ppi_trace.h>
#include <hal/nrf_radio.h>

struct {
	enum dm_io_output out;
	nrfx_gpiote_pin_t pin;
	nrfx_gpiote_out_config_t conf;
} static const debug_pin_config[] = {
#ifdef CONFIG_DM_GPIO_DEBUG
	{
	.out = DM_IO_RANGING,
	.pin = CONFIG_DM_RANGING_PIN,
	.conf = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(false)
	},
	{
	.out = DM_IO_ADD_REQUEST,
	.pin = CONFIG_DM_ADD_REQUEST_PIN,
	.conf = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(false)
	},
#endif
};


int dm_io_init(void)
{
	if (IS_ENABLED(CONFIG_DM_GPIO_DEBUG)) {
		nrfx_err_t err_code = NRFX_SUCCESS;

		if (nrfx_gpiote_is_init() == false) {
			err_code = nrfx_gpiote_init(0);
		}

		if (err_code != NRFX_SUCCESS) {
			return -ENODEV;
		}

		for (size_t i = 0; i < ARRAY_SIZE(debug_pin_config); i++) {
			if (nrfx_gpiote_out_init(debug_pin_config[i].pin,
						&debug_pin_config[i].conf) != NRFX_SUCCESS) {
				return -ENODEV;
			}
		}
		return err_code == NRFX_SUCCESS ? 0 : -ENODEV;
	}

	return 0;
}

void dm_io_set(enum dm_io_output out)
{
	if (IS_ENABLED(CONFIG_DM_GPIO_DEBUG)) {
		for (size_t i = 0; i < ARRAY_SIZE(debug_pin_config); i++) {
			if (out == debug_pin_config[i].out) {
				nrfx_gpiote_out_set(debug_pin_config[i].pin);
			}
		}
	}
}

void dm_io_clear(enum dm_io_output out)
{
	if (IS_ENABLED(CONFIG_DM_GPIO_DEBUG)) {
		for (size_t i = 0; i < ARRAY_SIZE(debug_pin_config); i++) {
			if (out == debug_pin_config[i].out) {
				nrfx_gpiote_out_clear(debug_pin_config[i].pin);
			}
		}
	}
}
