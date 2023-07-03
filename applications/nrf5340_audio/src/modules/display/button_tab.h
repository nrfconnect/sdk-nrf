/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

enum button_pin_names {
	BUTTON_VOLUME_DOWN = DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
	BUTTON_VOLUME_UP = DT_GPIO_PIN(DT_ALIAS(sw1), gpios),
	BUTTON_PLAY_PAUSE = DT_GPIO_PIN(DT_ALIAS(sw2), gpios),
	BUTTON_4 = DT_GPIO_PIN(DT_ALIAS(sw3), gpios),
	BUTTON_5 = DT_GPIO_PIN(DT_ALIAS(sw4), gpios),
};

void button_tab_create(lv_obj_t *current_screen);
extern void update_volume_level(void *arg1, void *arg2, void *arg3);