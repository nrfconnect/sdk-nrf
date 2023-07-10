/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <bluetooth/services/esl.h>
#include "esl_hw_impl.h"

LOG_MODULE_DECLARE(peripheral_esl);
#if DT_HAS_COMPAT_STATUS_OKAY(solomon_ssd16xxfb)
#define DT_DRV_COMPAT solomon_ssd16xxfb
#endif

const struct device *display_dev;
const struct device *spi = DEVICE_DT_GET(DT_NODELABEL(arduino_spi));
const struct gpio_dt_spec reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios);
const struct gpio_dt_spec dc_gpio = GPIO_DT_SPEC_INST_GET(0, dc_gpios);
PINCTRL_DT_DEFINE(DT_NODELABEL(arduino_spi));
const struct pinctrl_dev_config *pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(arduino_spi));
static struct display_capabilities capabilities;
#if defined(CONFIG_ESL_POWER_PROFILE)
/** Turn off SPI and Wavashare gpio to save power */
static int display_gpio_onoff(bool onoff)
{
	int err;

	if (onoff) {
		LOG_DBG("reset_gpio pin %d", reset_gpio.pin);
		err = gpio_pin_configure_dt(&reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (err < 0) {
			LOG_ERR("Failed to configure reset GPIO");
			return err;
		}

		LOG_DBG("dc_gpio pin %d", dc_gpio.pin);
		err = gpio_pin_configure_dt(&dc_gpio, GPIO_OUTPUT_INACTIVE);
		if (err < 0) {
			LOG_ERR("Failed to configure DC GPIO");
			return err;
		}

	} else {
		*(volatile uint32_t *)(DT_REG_ADDR(DT_NODELABEL(arduino_spi)) | 0xFFC) = 0;
		*(volatile uint32_t *)(DT_REG_ADDR(DT_NODELABEL(arduino_spi)) | 0xFFC);
		*(volatile uint32_t *)(DT_REG_ADDR(DT_NODELABEL(arduino_spi)) | 0xFFC) = 1;

		err = gpio_pin_configure_dt(&reset_gpio, GPIO_DISCONNECTED);
		if (err < 0) {
			LOG_ERR("Failed to configure reset GPIO disconneted");
			return err;
		}

		err = gpio_pin_configure_dt(&dc_gpio, GPIO_DISCONNECTED);
		if (err < 0) {
			LOG_ERR("Failed to configure DC GPIO disconneted");
			return err;
		}

		/* SPI related pin need to be disconnect after SPI module powered off */
		err = pinctrl_apply_state(pcfg, PINCTRL_STATE_SLEEP);
		if (err < 0) {
			return err;
		}
	}

	LOG_DBG("display_gpio_onoff leave");

	return 0;
}
#endif /* ESL_POWER_PROFILE */

int display_init(void)
{

#if (CONFIG_DISPLAY)
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display is not ready, aborting test");
		return -ENODEV;
	}

	display_get_capabilities(display_dev, &capabilities);
#endif

#if defined(CONFIG_ESL_POWER_PROFILE)
	(void)pm_device_action_run(spi, PM_DEVICE_ACTION_SUSPEND);
	display_gpio_onoff(false);
#endif
	return 0;
}

int display_control(uint8_t disp_idx, uint8_t img_idx, bool enable)
{
	ARG_UNUSED(disp_idx);
	size_t img_size, cur_pos, chunk_size;
	struct display_buffer_descriptor buf_desc;
	struct waveshare_gray_head *img_head;
	struct bt_esls *esl_obj = esl_get_esl_obj();
	int err, rc;

#if defined(CONFIG_ESL_POWER_PROFILE)
	display_gpio_onoff(true);
	(void)pm_device_action_run(spi, PM_DEVICE_ACTION_RESUME);
	/**
	 * To optimize power remove static declaration of ssd16xx_init in
	 * zephyr/drivers/display/ssd16xx.c
	 **/
	ssd16xx_init(display_dev);
	LOG_DBG("display %d img %d on/off %d", disp_idx, img_idx, enable);
#endif /* ESL_POWER_PROFILE */

	/* Load image to buffer for all kind of E-paper*/
	memset(esl_obj->img_obj_buf, 0, CONFIG_ESL_IMAGE_BUFFER_SIZE);
	img_size = esl_obj->cb.read_img_size_from_storage(img_idx);
	/* Read header first */
	err = esl_obj->cb.read_img_from_storage(img_idx, esl_obj->img_obj_buf,
						sizeof(struct waveshare_gray_head), 0);
	if (err < 0) {
		LOG_ERR("read image idx %d failed (err %d)", img_idx, err);
	}

	img_head = (struct waveshare_gray_head *)esl_obj->img_obj_buf;
	buf_desc.width = img_head->w;
	buf_desc.height = img_head->h;
	buf_desc.pitch = buf_desc.width;
	buf_desc.buf_size = (img_head->w * img_head->h) / 8;
	img_size -= sizeof(struct waveshare_gray_head);

	display_blanking_on(display_dev);
	cur_pos = sizeof(struct waveshare_gray_head);
	chunk_size = img_head->w;

	err = esl_obj->cb.read_img_from_storage(img_idx, esl_obj->img_obj_buf, img_size, 0);
	rc = display_write(display_dev, 0, 0, &buf_desc,
			   (esl_obj->img_obj_buf + sizeof(struct display_buffer_descriptor)));
	if (rc) {
		LOG_ERR("display_write (rc %d)", rc);
	}

	display_blanking_off(display_dev);
	LOG_DBG("Use Raw display interface API");
#if defined(CONFIG_ESL_POWER_PROFILE)
	(void)pm_device_action_run(spi, PM_DEVICE_ACTION_SUSPEND);
	display_gpio_onoff(false);
#endif
	return 0;
}

void display_unassociated(uint8_t disp_idx)
{

#if defined(CONFIG_ESL_POWER_PROFILE)
	display_gpio_onoff(true);
	/**
	 * To optimize power remove static declaration of ssd16xx_init in
	 * zephyr/drivers/display/ssd16xx.c
	 **/
	ssd16xx_init(display_dev);
	(void)pm_device_action_run(spi, PM_DEVICE_ACTION_RESUME);
#endif
	/* Do something about display device when Tag is unassociated */
#if defined(CONFIG_ESL_POWER_PROFILE)
	(void)pm_device_action_run(spi, PM_DEVICE_ACTION_SUSPEND);
#endif
}

void display_associated(uint8_t disp_idx)
{

#if defined(CONFIG_ESL_POWER_PROFILE)
	display_gpio_onoff(true);
	(void)pm_device_action_run(spi, PM_DEVICE_ACTION_RESUME);
	/**
	 * To optimize power remove static declaration of ssd16xx_init in
	 * zephyr/drivers/display/ssd16xx.c
	 **/
	ssd16xx_init(display_dev);
#endif
	/* Do something about display device when Tag is associated but not synchronized */
#if defined(CONFIG_ESL_POWER_PROFILE)
	(void)pm_device_action_run(spi, PM_DEVICE_ACTION_SUSPEND);
#endif
}
