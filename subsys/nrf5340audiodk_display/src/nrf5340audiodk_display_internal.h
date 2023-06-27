enum button_pin_names {
	BUTTON_VOLUME_DOWN = DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
	BUTTON_VOLUME_UP = DT_GPIO_PIN(DT_ALIAS(sw1), gpios),
	BUTTON_PLAY_PAUSE = DT_GPIO_PIN(DT_ALIAS(sw2), gpios),
	BUTTON_4 = DT_GPIO_PIN(DT_ALIAS(sw3), gpios),
	BUTTON_5 = DT_GPIO_PIN(DT_ALIAS(sw4), gpios),
};



void create_button_tab(lv_obj_t *screen);