/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @defgroup nfc_writable_ndef_msg_example_main main.c
 * @{
 * @ingroup nfc_writable_ndef_msg_example
 * @brief The application main file of NFC writable NDEF message example.
 *
 */

#include <zephyr/kernel.h>
#if defined(CONFIG_REBOOT)
#include <zephyr/sys/reboot.h>
#endif /* CONFIG_REBOOT */

#include "ndef_file_m.h"

#include <nfc_t4t_lib.h>
#include <nfc/ndef/msg.h>
#include <nfc/t4t/ndef_file.h>

#if defined(CONFIG_DK_LIBRARY)
#include <dk_buttons_and_leds.h>

#define NFC_FIELD_LED		DK_LED1
#define NFC_WRITE_LED		DK_LED2
#define NFC_READ_LED		DK_LED4

#define NDEF_RESTORE_BTN_MSK	DK_BTN1_MSK
#endif /* CONFIG_DK_LIBRARY */

static uint8_t ndef_msg_buf[CONFIG_NDEF_FILE_SIZE]; /**< Buffer for NDEF file. */

enum {
	FLASH_WRITE_FINISHED,
	FLASH_BUF_PREP_STARTED,
	FLASH_BUF_PREP_FINISHED,
	FLASH_WRITE_STARTED,
};

static atomic_t op_flags;

static uint8_t flash_buf[CONFIG_NDEF_FILE_SIZE]; /**< Buffer for storage update. */
static uint8_t flash_buf_len; /**< Length of the storage buffer. */

static K_SEM_DEFINE(nfc_write_sem, 0, 1);

static void flash_buffer_prepare(size_t data_length)
{
	if (atomic_cas(&op_flags, FLASH_WRITE_FINISHED,
			FLASH_BUF_PREP_STARTED)) {
		flash_buf_len = data_length + NFC_NDEF_FILE_NLEN_FIELD_SIZE;
		memcpy(flash_buf, ndef_msg_buf, sizeof(flash_buf));

		atomic_set(&op_flags, FLASH_BUF_PREP_FINISHED);
		k_sem_give(&nfc_write_sem);
	} else {
		printk("Flash update pending. Discarding new data...\n");
	}

}

/**
 * @brief Callback function for handling NFC events.
 */
static void nfc_callback(void *context,
			 nfc_t4t_event_t event,
			 const uint8_t *data,
			 size_t data_length,
			 uint32_t flags)
{
	ARG_UNUSED(context);
	ARG_UNUSED(data);
	ARG_UNUSED(flags);

	switch (event) {
	case NFC_T4T_EVENT_FIELD_ON:
#if defined(CONFIG_DK_LIBRARY)
		dk_set_led_on(NFC_FIELD_LED);
#endif /* CONFIG_DK_LIBRARY */
		break;

	case NFC_T4T_EVENT_FIELD_OFF:
#if defined(CONFIG_DK_LIBRARY)
		dk_set_leds(DK_NO_LEDS_MSK);
#endif /* CONFIG_DK_LIBRARY */
		break;

	case NFC_T4T_EVENT_NDEF_READ:
#if defined(CONFIG_DK_LIBRARY)
		dk_set_led_on(NFC_READ_LED);
#endif /* CONFIG_DK_LIBRARY */
		break;

	case NFC_T4T_EVENT_NDEF_UPDATED:
		if (data_length > 0) {
#if defined(CONFIG_DK_LIBRARY)
			dk_set_led_on(NFC_WRITE_LED);
#endif /* CONFIG_DK_LIBRARY */
			flash_buffer_prepare(data_length);
		}
		break;

	default:
		break;
	}
}

static int board_init(void)
{
	int err = 0;

#if defined(CONFIG_DK_LIBRARY)
	err = dk_buttons_init(NULL);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
		return err;
	}

	err = dk_leds_init();
	if (err) {
		printk("Cannot init LEDs (err: %d)\n", err);
	}
#endif /* CONFIG_DK_LIBRARY */

	return err;
}

/**
 * @brief   Function for application main entry.
 */
int main(void)
{
	/* Configure LED-pins as outputs. */
	if (board_init() < 0) {
		printk("Cannot initialize board!\n");
		goto fail;
	}

	/* Initialize storage. */
	if (ndef_file_setup() < 0) {
		printk("Cannot setup NDEF file!\n");
		goto fail;
	}

	/* Load NDEF message. */
	if (ndef_file_load(ndef_msg_buf, sizeof(ndef_msg_buf)) < 0) {
		printk("Cannot load NDEF file!\n");
		goto fail;
	}

#if defined(CONFIG_DK_LIBRARY)
	/* Restore default NDEF message if button is pressed. */
	uint32_t button_state;

	dk_read_buttons(&button_state, NULL);
	if (button_state & NDEF_RESTORE_BTN_MSK) {
		if (ndef_restore_default(ndef_msg_buf,
					 sizeof(ndef_msg_buf)) < 0) {
			printk("Cannot flash NDEF message!\n");
			goto fail;
		}
		printk("Default NDEF message restored!\n");
	}
#endif /* CONFIG_DK_LIBRARY */

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
	printk("Starting NFC Writable NDEF Message sample\n");

	while (true) {
		k_sem_take(&nfc_write_sem, K_FOREVER);
		if (atomic_cas(&op_flags, FLASH_BUF_PREP_FINISHED,
				FLASH_WRITE_STARTED)) {
			if (ndef_file_update(flash_buf, flash_buf_len) < 0) {
				printk("Cannot update NDEF message!\n");
			} else {
				printk("NDEF message successfully updated.\n");
			}
			atomic_set(&op_flags, FLASH_WRITE_FINISHED);
		}
	}

fail:
#if defined(CONFIG_REBOOT)
	sys_reboot(SYS_REBOOT_COLD);
#endif /* CONFIG_REBOOT */
	return -EIO;
}
/** @} */
