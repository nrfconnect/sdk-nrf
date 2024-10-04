/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nfc_t2t_lib.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>

#define MAX_REC_COUNT	  1
#define NDEF_MSG_BUF_SIZE 128

static K_SEM_DEFINE(my_nfc_sem, 0, 1);

static void nfc_callback(void *context, nfc_t2t_event_t event, const uint8_t *data,
			 size_t data_length)
{
	ARG_UNUSED(context);
	ARG_UNUSED(data);
	ARG_UNUSED(data_length);

	switch (event) {
	case NFC_T2T_EVENT_FIELD_ON:
		k_sem_give(&my_nfc_sem);
		break;
	case NFC_T2T_EVENT_FIELD_OFF:
		break;
	default:
		break;
	}
}

int main(void)
{
	int ret;

	/* Text message in its language code. */
	static const uint8_t en_payload[] = {'H', 'e', 'l', 'l', 'o', ' ',
					     'W', 'o', 'r', 'l', 'd', '!'};
	static const uint8_t en_code[] = {'e', 'n'};

	/* Buffer used to hold an NFC NDEF message. */
	static uint8_t buffer[NDEF_MSG_BUF_SIZE];

	NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_text_rec, UTF_8, en_code, sizeof(en_code), en_payload,
				      sizeof(en_payload));

	NFC_NDEF_MSG_DEF(nfc_text_msg, MAX_REC_COUNT);

	uint32_t len = sizeof(buffer);

	ret = nfc_t2t_setup(nfc_callback, NULL);
	if (ret < 0) {
		printk("Cannot setup NFC T2T library (%d)\n", ret);
		return 0;
	}

	/* Add record */
	ret = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg),
				      &NFC_NDEF_TEXT_RECORD_DESC(nfc_text_rec));
	if (ret < 0) {
		printk("Cannot add record (%d)\n", ret);
		return 0;
	}

	/* Encode welcome message */
	ret = nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_text_msg), buffer, &len);
	if (ret < 0) {
		printk("Cannot encode message (%d)\n", ret);
		return 0;
	}

	/* Set created message as the NFC payload */
	ret = nfc_t2t_payload_set(buffer, len);
	if (ret < 0) {
		printk("Cannot set payload (%d)\n", ret);
		return 0;
	}

	/* Start sensing NFC field */
	ret = nfc_t2t_emulation_start();
	if (ret < 0) {
		printk("Cannot start emulation (%d)\n", ret);
		return 0;
	}

	while (1) {
		if (k_sem_take(&my_nfc_sem, K_FOREVER) != 0) {
			printk("Failed to take a semaphore\n");
			return 0;
		}
		k_busy_wait(1000000);
		k_sem_reset(&my_nfc_sem);
	}

	return 0;
}
