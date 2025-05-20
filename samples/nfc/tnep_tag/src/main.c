/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <dk_buttons_and_leds.h>

#include <nfc_t4t_lib.h>
#include <nfc/tnep/tag.h>
#include <nfc/t4t/ndef_file.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>

#define TNEP_INITIAL_MSG_RECORD_COUNT 1

#define NDEF_TNEP_MSG_SIZE 1024

#define LED_INIT DK_LED1
#define LED_SVC_ONE DK_LED3
#define LED_SVC_TWO DK_LED4

static const uint8_t en_code[] = {'e', 'n'};

static const uint8_t svc_one_uri[] = "svc:pi";
static const uint8_t svc_two_uri[] = "svc:e";

static uint8_t tag_buffer[NDEF_TNEP_MSG_SIZE];
static uint8_t tag_buffer2[NDEF_TNEP_MSG_SIZE];

static struct k_poll_event events[NFC_TNEP_EVENTS_NUMBER];

/** .. include_startingpoint_tnep_service_rst */
static void tnep_svc_one_selected(void)
{
	int err;
	const char svc_one_msg[] = "Service pi = 3.14159265358979323846";

	printk("Service one selected\n");

	NFC_TNEP_TAG_APP_MSG_DEF(app_msg, 1);
	NFC_NDEF_TEXT_RECORD_DESC_DEF(svc_one_rec, UTF_8, en_code,
				      sizeof(en_code), svc_one_msg,
				      strlen(svc_one_msg));

	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(app_msg),
				      &NFC_NDEF_TEXT_RECORD_DESC(svc_one_rec));

	err = nfc_tnep_tag_tx_msg_app_data(&NFC_NDEF_MSG(app_msg),
					   NFC_TNEP_STATUS_SUCCESS);
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
static void tnep_svc_one_msg_received(const uint8_t *data, size_t len)
{
	int err;

	printk("New service data received. Finish service data exchange\n");

	err = nfc_tnep_tag_tx_msg_no_app_data();
	if (err) {
		printk("TNEP Service data finish err: %d\n", err);
	}
}

static void tnep_svc_error(int err_code)
{
	printk("Service data exchange error: %d\n", err_code);
}

/** .. include_endpoint_tnep_service_rst */

static void tnep_svc_two_selected(void)
{
	printk("Service two selected\n");

	dk_set_led_on(LED_SVC_TWO);
}

static void tnep_svc_two_deselected(void)
{
	printk("Service two deselected\n");

	dk_set_led_off(LED_SVC_TWO);
}
static void tnep_svc_two_msg_received(const uint8_t *data, size_t len)
{
	int err;

	printk("New service data received. Finish service data exchange\n");

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

static void nfc_callback(void *context, nfc_t4t_event_t event,
			 const uint8_t *data, size_t data_length, uint32_t flags)
{
	switch (event) {
	case NFC_T4T_EVENT_NDEF_UPDATED:
		if (data_length > 0) {
			nfc_tnep_tag_rx_msg_indicate(nfc_t4t_ndef_file_msg_get(data),
						     data_length);
		}

		break;

	case NFC_T4T_EVENT_FIELD_ON:
	case NFC_T4T_EVENT_FIELD_OFF:
		nfc_tnep_tag_on_selected();

		break;

	default:
		/* This block intentionally left blank. */
		break;
	}
}

/** .. include_startingpoint_initial_msg_cb_rst */
static int tnep_initial_msg_encode(struct nfc_ndef_msg_desc *msg)
{
	const uint8_t text[] = "Hello World";

	NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_text, UTF_8, en_code,
				      sizeof(en_code), text,
				      strlen(text));

	return nfc_tnep_initial_msg_encode(msg,
					   &NFC_NDEF_TEXT_RECORD_DESC(nfc_text),
					   1);
}
/** .. include_endpoint_initial_cb_msg_rst */

static void button_pressed(uint32_t button_state, uint32_t has_changed)
{
	int err;
	uint32_t button = button_state & has_changed;
	const char svc_two_msg[] = "Service e  = 2.71828182845904523536";

	if (button & DK_BTN1_MSK) {
		NFC_TNEP_TAG_APP_MSG_DEF(app_msg, 1);
		NFC_NDEF_TEXT_RECORD_DESC_DEF(svc_two_rec, UTF_8, en_code,
					      sizeof(en_code), svc_two_msg,
					      strlen(svc_two_msg));

		err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(app_msg),
					      &NFC_NDEF_TEXT_RECORD_DESC(svc_two_rec));

		err = nfc_tnep_tag_tx_msg_app_data(&NFC_NDEF_MSG(app_msg),
						   NFC_TNEP_STATUS_SUCCESS);
		if (err == -EACCES) {
			printk("Service is not in selected state. App data cannot be set\n");
		} else {
			printk("Service app data set err: %d\n", err);
		}
	}
}

int main(void)
{
	int err;

	printk("NFC Tag TNEP application\n");

	err = dk_leds_init();
	if (err) {
		printk("led init error %d", err);
		return 0;
	}

	err = dk_buttons_init(button_pressed);
	if (err) {
		printk("buttons init error %d", err);
		return 0;
	}

	/* TNEP init */
	err = nfc_tnep_tag_tx_msg_buffer_register(tag_buffer, tag_buffer2,
						  sizeof(tag_buffer));
	if (err) {
		printk("Cannot register tnep buffer, err: %d\n", err);
		return 0;
	}

	err = nfc_tnep_tag_init(events, ARRAY_SIZE(events),
				nfc_t4t_ndef_rwpayload_set);
	if (err) {
		printk("Cannot initialize TNEP protocol, err: %d\n", err);
		return 0;
	}

	/* Type 4 Tag setup */
	err = nfc_t4t_setup(nfc_callback, NULL);
	if (err) {
		printk("NFC T4T setup err: %d\n", err);
		return 0;
	}

/** .. include_startingpoint_initial_msg_rst */
	err = nfc_tnep_tag_initial_msg_create(TNEP_INITIAL_MSG_RECORD_COUNT,
					      tnep_initial_msg_encode);
	if (err) {
		printk("Cannot create initial TNEP message, err: %d\n", err);
	}
/** .. include_endpoint_initial_msg_rst */

	err = nfc_t4t_emulation_start();
	if (err) {
		printk("NFC T4T emulation start err: %d\n", err);
		return 0;
	}

	dk_set_led_on(LED_INIT);

	for (;;) {
		k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		nfc_tnep_tag_process();
	}
}
