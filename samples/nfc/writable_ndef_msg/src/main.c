/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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
#include <nfc_t4t_lib.h>

#include "ndef_file_m.h"
#include <nfc/ndef/nfc_ndef_msg.h>

#include <soc.h>
#include <device.h>
#include <gpio.h>

#define LED_ON  (u32_t)(0)
#define LED_OFF (u32_t)(1)

static struct device *LED0_dev;
static struct device *LED1_dev;
static struct device *LED3_dev;
static struct device *button;

#define BUTTON_PORT	SW0_GPIO_CONTROLLER
/**< Button's GPIO port used to set default NDEF message. */
#define BUTTON_PIN	SW0_GPIO_PIN
/**< Button's GPIO pin used to set default NDEF message. */

static u8_t ndef_msg_buf[CONFIG_NDEF_FILE_SIZE]; /**< Buffer for NDEF file. */

enum {
	FLASH_WRITE_FINISHED,
	FLASH_BUF_PREP_STARTED,
	FLASH_BUF_PREP_FINISHED,
	FLASH_WRITE_STARTED,
};
static atomic_t op_flags;
static u8_t flash_buf[CONFIG_NDEF_FILE_SIZE]; /**< Buffer for flash update. */
static u8_t flash_buf_len; /**< Length of the flash buffer. */

static void flash_buffer_prepare(size_t data_length)
{
	if (atomic_cas(&op_flags, FLASH_WRITE_FINISHED,
			FLASH_BUF_PREP_STARTED)) {
		flash_buf_len = data_length + NLEN_FIELD_SIZE;
		memcpy(flash_buf, ndef_msg_buf, sizeof(flash_buf));

		atomic_set(&op_flags, FLASH_BUF_PREP_FINISHED);
	} else {
		printk("Flash update pending. Discarding new data...\n");
	}

}

/**
 * @brief Callback function for handling NFC events.
 */
static void nfc_callback(void *context,
			 enum nfc_t4t_event event,
			 const u8_t *data,
			 size_t data_length,
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
		if (data_length > 0) {
			gpio_pin_write(LED1_dev, LED1_GPIO_PIN, LED_ON);
			flash_buffer_prepare(data_length);
		}
		break;

	default:
		break;
	}
}

static int board_init(void)
{
	LED0_dev = device_get_binding(LED0_GPIO_CONTROLLER);
	if (!LED0_dev) {
		printk("LED0 device not found!\n");
		return -ENXIO;
	}

	LED1_dev = device_get_binding(LED1_GPIO_CONTROLLER);
	if (!LED1_dev) {
		printk("LED1 device not found!\n");
		return -ENXIO;
	}

	LED3_dev = device_get_binding(LED3_GPIO_CONTROLLER);
	if (!LED3_dev) {
		printk("LED3 device not found!\n");
		return -ENXIO;
	}

	/* Set LED pin as output */
	int err;

	err = gpio_pin_configure(LED0_dev, LED0_GPIO_PIN, GPIO_DIR_OUT);
	if (err) {
		printk("Cannot configure led 0 port!\n");
		return err;
	}

	err = gpio_pin_configure(LED1_dev, LED1_GPIO_PIN, GPIO_DIR_OUT);
	if (err) {
		printk("Cannot configure led 1 port!\n");
		return err;
	}

	err = gpio_pin_configure(LED3_dev, LED3_GPIO_PIN, GPIO_DIR_OUT);
	if (err) {
		printk("Cannot configure led 3 port!\n");
		return err;
	}
	/* Turn LEDs off */
	gpio_pin_write(LED0_dev, LED0_GPIO_PIN, LED_OFF);
	gpio_pin_write(LED1_dev, LED1_GPIO_PIN, LED_OFF);
	gpio_pin_write(LED3_dev, LED3_GPIO_PIN, LED_OFF);

	button = device_get_binding(BUTTON_PORT);
	if (!button) {
		printk("Button device not found!\n");
		return -ENXIO;
	}
	err = gpio_pin_configure(button, BUTTON_PIN,
				 GPIO_DIR_IN | GPIO_PUD_PULL_UP);
	if (err) {
		printk("Cannot configure button port!\n");
		return err;
	}
	return 0;
}

/**
 * @brief   Function for application main entry.
 */
int main(void)
{
	printk("NFC configuration start.\n");

	/* Configure LED-pins as outputs. */
	if (board_init() < 0) {
		printk("Cannot initialize board!\n");
		goto fail;
	}
	/* Initialize NVS. */
	if (ndef_file_setup() < 0) {
		printk("Cannot setup NDEF file!\n");
		goto fail;
	}
	/* Load NDEF message from the flash file. */
	if (ndef_file_load(ndef_msg_buf, sizeof(ndef_msg_buf)) < 0) {
		printk("Cannot load NDEF file!\n");
		goto fail;
	}
	/* Restore default NDEF message if button is pressed. */
	u32_t val = 0;

	gpio_pin_read(button, BUTTON_PIN, &val);
	if (!val) {
		if (ndef_restore_default(ndef_msg_buf,
					 sizeof(ndef_msg_buf)) < 0) {
			printk("Cannot flash NDEF message!\n");
			goto fail;
		}
		printk("Default NDEF message restored!\n");
	}
	/* Set up NFC */
	int err = nfc_t4t_setup(nfc_callback, NULL);

	if (err < 0) {
		printk("Cannot setup t4t library!\n");
		goto fail;
	}
	/* Run Read-Write mode for Type 4 Tag platform */
	if (nfc_t4t_ndef_rwpayload_set(ndef_msg_buf,
				       sizeof(ndef_msg_buf)) < 0) {
		printk("Cannot set payload!\n");
		goto fail;
	}
	/* Start sensing NFC field */
	if (nfc_t4t_emulation_start() < 0) {
		printk("Cannot start emulation!\n");
		goto fail;
	}
	printk("Writable NDEF message example started.\n");

	while (true) {
		if (atomic_cas(&op_flags, FLASH_BUF_PREP_FINISHED,
				FLASH_WRITE_STARTED)) {
			if (ndef_file_update(flash_buf, flash_buf_len) < 0) {
				printk("Cannot flash NDEF message!\n");
			} else {
				printk("NDEF message successfully flashed.\n");
			}

			atomic_set(&op_flags, FLASH_WRITE_FINISHED);
		}

		__WFE();
	}

fail:
	#if CONFIG_REBOOT
		sys_reboot(SYS_REBOOT_COLD);
	#endif /* CONFIG_REBOOT */
		return -EIO;
}
/** @} */
