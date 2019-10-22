/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <stdbool.h>
#include <string.h>
#include <zephyr.h>
#include <misc/util.h>

#include <dk_buttons_and_leds.h>

#include <nfc_t4t_lib.h>
#include <nfc/tnep/tag.h>
#include <nfc/ndef/nfc_ndef_msg.h>
#include <nfc/ndef/nfc_text_rec.h>
#include <logging/log.h>


LOG_MODULE_REGISTER(dupa);

#define NDEF_TNEP_MSG_SIZE 1024

#define LED_INIT DK_LED1
#define LED_SVC_ONE DK_LED3
#define LED_SVC_TWO DK_LED4

static const  u8_t msg[] = "Hello World";
static const u8_t en_code[] = {'e', 'n'};

static const char svc_one_msg[] = "Service pi = 3.14159265358979323846";
static const char svc_two_msg[] = "Service e  = 2.71828182845904523536";

static const u8_t svc_one_uri[] = "svc:pi";
static const u8_t svc_two_uri[] = "svc:e";

static u8_t tag_buffer[NDEF_TNEP_MSG_SIZE];
static u8_t tag_buffer2[NDEF_TNEP_MSG_SIZE];

NFC_NDEF_MSG_DEF(ndef_msg, 16);

NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_text, UTF_8, en_code, sizeof(en_code),
			      msg, sizeof(msg));
NFC_NDEF_TEXT_RECORD_DESC_DEF(svc_one_rec, UTF_8, en_code, sizeof(en_code),
			      svc_one_msg, sizeof(svc_one_msg));
NFC_NDEF_TEXT_RECORD_DESC_DEF(svc_two_rec, UTF_8, en_code, sizeof(en_code),
			      svc_two_msg, sizeof(svc_two_msg));


static struct k_poll_event events[NFC_TNEP_EVENTS_NUMBER];

static void tnep_svc_one_selected(void)
{
	int err;

	printk("Service one selected\n");

	err = nfc_tnep_tag_tx_msg_app_data(&NFC_NDEF_TEXT_RECORD_DESC(svc_one_rec),
					   1, NFC_TNEP_STATUS_SUCCESS);
	if (err) {
		printk("Service app data set err: %d\n", err);
	}

	dk_set_led_on(LED_SVC_ONE);
}

static void tnep_svc_one_deselected(void)
{
	printk("Service one deselected\n");

	dk_set_led_off(LED_SVC_ONE);
}
static void tnep_svc_one_msg_received(const u8_t *data, size_t len)
{
	int err;

	printk("New service data received. Finish service data exchange");

	err = nfc_tnep_tag_tx_msg_no_app_data();
	if (err) {
		printk("TNEP Service data finish err: %d\n", err);
	}
}

static void tnep_svc_error(int err_code)
{
	printk("Service data exchange error: %d\n", err_code);
}

static void tnep_svc_two_selected(void)
{
	printk("Service two selected\n");

	dk_set_led_on(LED_SVC_TWO);
}

static void tnep_svc_two_deselected(void)
{
	printk("Service one deselected\n");

	dk_set_led_off(LED_SVC_TWO);
}
static void tnep_svc_two_msg_received(const u8_t *data, size_t len)
{
	int err;

	printk("New service data received. Finish service data exchange");

	err = nfc_tnep_tag_tx_msg_no_app_data();
	if (err) {
		printk("TNEP Service data finish err: %d\n", err);
	}
}

NFC_TNEP_TAG_SERVICE_DEF(svc_one, svc_one_uri, (ARRAY_SIZE(svc_one_uri) - 1),
		     NFC_TNEP_COMM_MODE_SINGLE_RESPONSE, 20, 4, 1024,
		     tnep_svc_one_selected, tnep_svc_one_deselected,
		     tnep_svc_one_msg_received, tnep_svc_error);

NFC_TNEP_TAG_SERVICE_DEF(svc_two, svc_two_uri, (ARRAY_SIZE(svc_two_uri) - 1),
		     NFC_TNEP_COMM_MODE_SINGLE_RESPONSE, 63, 4, 1024,
		     tnep_svc_two_selected, tnep_svc_two_deselected,
		     tnep_svc_two_msg_received, tnep_svc_error);

static struct nfc_tnep_tag_service training_services[] = {
	NFC_TNEP_TAG_SERVICE(svc_one),
	NFC_TNEP_TAG_SERVICE(svc_two),
};

static void nfc_callback(void *context, enum nfc_t4t_event event,
			 const u8_t *data, size_t data_length, u32_t flags)
{
	switch (event) {
	case NFC_T4T_EVENT_NDEF_READ:
		break;
	case NFC_T4T_EVENT_NDEF_UPDATED:
		if (data_length > 0) {
			if (IS_ENABLED(CONFIG_NFC_NDEF_MSG_WITH_NLEN)) {
				data += NLEN_FIELD_SIZE;
			}
			nfc_tnep_tag_rx_msg_indicate(data, data_length);
		}

		break;

	case NFC_T4T_EVENT_DATA_TRANSMITTED:
	case NFC_T4T_EVENT_DATA_IND:
	case NFC_T4T_EVENT_NONE:
	case NFC_T4T_EVENT_FIELD_OFF:
		break;

	case NFC_T4T_EVENT_FIELD_ON:
		nfc_tnep_tag_on_selected();

		break;

	default:
		/* This block intentionally left blank. */
		break;
	}
}

static void button_pressed(u32_t button_state, u32_t has_changed)
{
	int err;
	u32_t button = button_state & has_changed;

	if (button & DK_BTN1_MSK) {
		err = nfc_tnep_tag_tx_msg_app_data(&NFC_NDEF_TEXT_RECORD_DESC(svc_two_rec),
					   1, NFC_TNEP_STATUS_SUCCESS);

		if (err == -EACCES) {
			printk("Service is not in selected state. App data cannot be set\n");
		} else {
			printk("Service app data set err: %d\n", err);
		}
	}
}

void main(void)
{
	int err;

	printk("NFC Tag TNEP application\n");

	err = dk_leds_init();
	if (err) {
		printk("led init error %d", err);
		return;
	}

	err = dk_buttons_init(button_pressed);
	if (err) {
		printk("buttons init error %d", err);
		return;
	}

	/* TNEP init */
	err = nfc_tnep_tag_tx_msg_buffer_register(tag_buffer, tag_buffer2,
						  sizeof(tag_buffer));
	if (err) {
		printk("Cannot register tnep buffer, err: %d\n", err);
		return;
	}

	err = nfc_tnep_tag_init(events, ARRAY_SIZE(events),
				&NFC_NDEF_MSG(ndef_msg),
				nfc_t4t_ndef_rwpayload_set);
	if (err) {
		printk("Cannot initialize TNEP protocol, err: %d\n", err);
		return;
	}

	/* Type X Tag init */
	err = nfc_t4t_setup(nfc_callback, NULL);
	if (err) {
		printk("NFC T4T setup err: %d\n", err);
		return;
	}

	err = nfc_tnep_tag_initial_msg_create(training_services,
					      ARRAY_SIZE(training_services),
					      &NFC_NDEF_TEXT_RECORD_DESC(nfc_text), 1);
	if (err) {
		printk("Cannot create initial TNEP message, err: %d\n", err);
	}

	err = nfc_t4t_emulation_start();
	if (err) {
		printk("NFC T4T emulation start err: %d\n", err);
		return;
	}

	dk_set_led_on(LED_INIT);

	for (;;) {
		k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		nfc_tnep_tag_process();
	}
}
