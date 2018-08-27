/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

/** @file
 *
 * @defgroup nfc_writable_ndef_msg_example_main main.c
 * @{
 * @ingroup nfc_writable_ndef_msg_example
 * @brief The application main file of NFC writable NDEF message example.
 *
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <nfc/nfc_t4t_lib.h>

#include "ndef_file_m.h"
#include <nfc/ndef/nfc_ndef_msg.h>

#include <board.h>
#include <device.h>
#include <gpio.h>

#include <logging/log_ctrl.h>
#define LOG_MODULE_NAME main
#include <logging/log.h>
LOG_MODULE_REGISTER();

#define LED_ON  (u32_t)(0)
#define LED_OFF (u32_t)(1)

static struct device *LED0_dev;
static struct device *LED1_dev;
static struct device *LED3_dev;
static struct device *button;

#define BUTTON_PORT	SW0_GPIO_NAME
/**< Button's GPIO port used to set default NDEF message. */
#define BUTTON_PIN	SW0_GPIO_PIN
/**< Button's GPIO pin used to set default NDEF message. */

static u8_t m_ndef_msg_buf[CONFIG_NDEF_FILE_SIZE]; /**< Buffer for NDEF file. */
static volatile u8_t m_ndef_msg_len; /**< Length of the NDEF message. */
static volatile bool update_mask; /**< Indicate that update event appeared */

/**
 * @brief Callback function for handling NFC events.
 */
static void nfc_callback(void *context,
			 enum nfc_t4t_event event,
			 const u8_t *data,
			 size_t dataLength,
			 u32_t flags)
{
	ARG_UNUSED(context);
	ARG_UNUSED(data);
	ARG_UNUSED(flags);

	switch (event) {
	case NFC_T4T_EVENT_FIELD_ON:
		gpio_pin_write(LED0_dev, LED0_GPIO_PIN, LED_ON);
		break;

	case NFC_T4T_EVENT_FIELD_OFF:
		gpio_pin_write(LED0_dev, LED0_GPIO_PIN, LED_OFF);
		gpio_pin_write(LED1_dev, LED1_GPIO_PIN, LED_OFF);
		gpio_pin_write(LED3_dev, LED3_GPIO_PIN, LED_OFF);
		break;

	case NFC_T4T_EVENT_NDEF_READ:
		gpio_pin_write(LED3_dev, LED3_GPIO_PIN, LED_ON);
		break;

	case NFC_T4T_EVENT_NDEF_UPDATED:
		if (dataLength > 0) {
			gpio_pin_write(LED1_dev, LED1_GPIO_PIN, LED_ON);
			m_ndef_msg_len = dataLength;
			update_mask = true;
		}
		break;

	default:
		break;
	}
}

static int board_init(void)
{
	LED0_dev = device_get_binding(LED0_GPIO_PORT);
	if (!LED0_dev) {
		LOG_ERR("LED0 device not found");
		return -ENXIO;
	}

	LED1_dev = device_get_binding(LED1_GPIO_PORT);
	if (!LED1_dev) {
		LOG_ERR("LED1 device not found");
		return -ENXIO;
	}

	LED3_dev = device_get_binding(LED3_GPIO_PORT);
	if (!LED3_dev) {
		LOG_ERR("LED3 device not found");
		return -ENXIO;
	}

	/* Set LED pin as output */
	int err;

	err = gpio_pin_configure(LED0_dev, LED0_GPIO_PIN, GPIO_DIR_OUT);
	if (err) {
		LOG_ERR("Cannot configure led 0 port!");
		return err;
	}

	err = gpio_pin_configure(LED1_dev, LED1_GPIO_PIN, GPIO_DIR_OUT);
	if (err) {
		LOG_ERR("Cannot configure led 1 port!");
		return err;
	}

	err = gpio_pin_configure(LED3_dev, LED3_GPIO_PIN, GPIO_DIR_OUT);
	if (err) {
		LOG_ERR("Cannot configure led 3 port!");
		return err;
	}
	/* Turn LEDs off */
	gpio_pin_write(LED0_dev, LED0_GPIO_PIN, LED_OFF);
	gpio_pin_write(LED1_dev, LED1_GPIO_PIN, LED_OFF);
	gpio_pin_write(LED3_dev, LED3_GPIO_PIN, LED_OFF);

	button = device_get_binding(BUTTON_PORT);
	if (!button) {
		LOG_ERR("Button device not found");
		return -ENXIO;
	}
	err = gpio_pin_configure(button,
				      BUTTON_PIN,
				      GPIO_DIR_IN | GPIO_PUD_PULL_UP);
	if (err) {
		LOG_ERR("Cannot configure button port!");
		return err;
	}
	return 0;
}
/**
 * @brief   Function for application main entry.
 */
int main(void)
{
	LOG_INIT();
	LOG_INF("NFC configuration start");

	/* Configure LED-pins as outputs. */
	if (board_init() < 0) {
		LOG_ERR("Cannot initialize board!");
		goto fail;
	}
	/* Initialize NVS. */
	if (ndef_file_setup() < 0) {
		LOG_ERR("Cannot setup NDEF file!");
		goto fail;
	}
	/* Load NDEF message from the flash file. */
	if (ndef_file_load(m_ndef_msg_buf, sizeof(m_ndef_msg_buf)) < 0) {
		LOG_ERR("Cannot load NDEF file!");
		goto fail;
	}
	/* Restore default NDEF message if button is pressed. */
	u32_t val = 0;

	gpio_pin_read(button, BUTTON_PIN, &val);
	if (!val) {
		if (ndef_restore_default(m_ndef_msg_buf,
					 sizeof(m_ndef_msg_buf)) < 0) {
			LOG_ERR("Cannot flash NDEF message");
			goto fail;
		}
		LOG_DBG("Default NDEF message restored!");
	}
	/* Set up NFC */
	int err = nfc_t4t_setup(nfc_callback, NULL);

	if (err < 0) {
		LOG_ERR("Cannot setup t4t library!");
		goto fail;
	}
	/* Run Read-Write mode for Type 4 Tag platform */
	if (nfc_t4t_ndef_rwpayload_set(m_ndef_msg_buf,
				       sizeof(m_ndef_msg_buf)) < 0) {
		LOG_ERR("Cannot set payload!");
		goto fail;
	}
	/* Start sensing NFC field */
	if (nfc_t4t_emulation_start() < 0) {
		LOG_ERR("Cannot start emulation!");
		goto fail;
	}
	LOG_INF("Writable NDEF message example started.");

	while (true) {
		if (update_mask) {
			if (ndef_file_update(m_ndef_msg_buf,
					m_ndef_msg_len + NLEN_FIELD_SIZE) < 0) {
				LOG_ERR("Cannot flash NDEF message");
			} else {
				LOG_INF("NDEF message successfully flashed");
			}
			update_mask = false;
		}
		if (!update_mask && !LOG_PROCESS()) {
			__WFE();
		}
	}

fail:
	#if CONFIG_REBOOT
		sys_reboot(SYS_REBOOT_COLD);
	#endif /* CONFIG_REBOOT */
		return -EIO;
}
/** @} */
