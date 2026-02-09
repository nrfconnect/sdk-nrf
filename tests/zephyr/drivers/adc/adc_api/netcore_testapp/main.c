/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

const struct gpio_dt_spec gpios[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_PATH(zephyr_user), gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};
const uint8_t number_of_gpios = ARRAY_SIZE(gpios);


int main(void)
{
	for (uint8_t i = 0; i < number_of_gpios; i++) {
		gpio_is_ready_dt(&gpios[i]);
		gpio_pin_configure_dt(&gpios[i], GPIO_INPUT);
	}

	return 0;
}
