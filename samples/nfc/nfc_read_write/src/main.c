/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdbool.h>
#include <nfc_t4t_lib.h>
#include <string.h>
#include <nfc/tnep/tag.h>
#include <nfc/ndef/nfc_ndef_msg.h>
#include <nfc_t4t_lib.h>
#include <dk_buttons_and_leds.h>
#include <nfc/ndef/msg_parser.h>
#include <nfc/ndef/nfc_text_rec.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <misc/util.h>
#include <device.h>
#include <gpio.h>
#include <dk_buttons_and_leds.h>

#define LED_INIT DK_LED1
#define LED_SVC_ONE DK_LED3
#define LED_SVC_TWO DK_LED4

LOG_MODULE_REGISTER(app);

u8_t msg[] = "Random Data";
static const u8_t en_code[] = {
		'e',
		'n' };

char reqest_msg[] = "Service pi = 3.14159265358979323846";
char svc_two_msg[] = "Service e  = 2.71828182845904523536";

static u8_t training_uri_one[] = "svc:pi";
static u8_t training_uri_two[] = "svc:e";

static u8_t tag_buffer[1024];
static size_t tag_buffer_size = sizeof(tag_buffer);

NFC_TNEP_SERIVCE_SELECT_RECORD_DESC_DEF(deselect_service, 0, NULL);
NFC_NDEF_TEXT_RECORD_DESC_DEF(app_data_rec, UTF_8, en_code, sizeof(en_code),
			      msg, sizeof(msg));
NFC_NDEF_TEXT_RECORD_DESC_DEF(svc_one_rec, UTF_8, en_code, sizeof(en_code),
			      reqest_msg, sizeof(reqest_msg));
NFC_NDEF_TEXT_RECORD_DESC_DEF(svc_two_rec, UTF_8, en_code, sizeof(en_code),
			      svc_two_msg, sizeof(svc_two_msg));

u8_t svc_one_sel(void)
{
	LOG_INF("%s", __func__);

	dk_set_led_on(LED_SVC_ONE);

	return 0;
}
void svc_one_desel(void)
{
	LOG_INF("%s", __func__);

	dk_set_led_off(LED_SVC_ONE);
}
void svc_one_new_msg(void)
{
	LOG_INF("%s", __func__);

	nfc_tnep_tx_msg_app_data(&NFC_NDEF_TEXT_RECORD_DESC(svc_one_rec));
}
void svc_timeout(void)
{
	LOG_INF("%s", __func__);
}
void svc_error(int err_code)
{
	LOG_INF("%s. code %d", __func__, err_code);
}

u8_t svc_two_sel(void)
{
	LOG_INF("%s", __func__);

	dk_set_led_on(LED_SVC_TWO);

	return 0;
}
void svc_two_desel(void)
{
	LOG_INF("%s", __func__);

	dk_set_led_off(LED_SVC_TWO);
}
void svc_two_new_msg(void)
{
	LOG_INF("%s", __func__);

	nfc_tnep_tx_msg_app_data(&NFC_NDEF_TEXT_RECORD_DESC(svc_two_rec));
}

NFC_TNEP_SERVICE_DEF(training_1, training_uri_one, ARRAY_SIZE(training_uri_one),
		     NFC_TNEP_COMM_MODE_SINGLE_RESPONSE, 200, 4, svc_one_sel,
		     svc_one_desel, svc_one_new_msg, svc_timeout, svc_error);

NFC_TNEP_SERVICE_DEF(training_2, training_uri_two, ARRAY_SIZE(training_uri_two),
		     NFC_TNEP_COMM_MODE_SINGLE_RESPONSE, 200, 4, svc_two_sel,
		     svc_two_desel, svc_two_new_msg, svc_timeout, svc_error);

struct nfc_tnep_service training_services[] = {
				NFC_TNEP_SERVICE(training_1),
				NFC_TNEP_SERVICE(training_2),
		};

static void nfc_callback(void *context, enum nfc_t4t_event event,
			 const u8_t *data, size_t data_length, u32_t flags)
{
	switch (event) {
	case NFC_T4T_EVENT_NDEF_READ:
		break;
	case NFC_T4T_EVENT_NDEF_UPDATED: {
		u8_t *ndef_msg = tag_buffer;

		tag_buffer_size = sizeof(tag_buffer);

		if (IS_ENABLED(CONFIG_NFC_NDEF_MSG_WITH_NLEN)) {
			ndef_msg += NLEN_FIELD_SIZE;
			/*ndef_msg_len -= NLEN_FIELD_SIZE;*/
		}

		nfc_tnep_rx_msg_indicate(ndef_msg, tag_buffer_size);
		break;
	}
	case NFC_T4T_EVENT_DATA_TRANSMITTED:
	case NFC_T4T_EVENT_DATA_IND:
	case NFC_T4T_EVENT_NONE:
	case NFC_T4T_EVENT_FIELD_ON:
	case NFC_T4T_EVENT_FIELD_OFF:
	default:
		/* This block intentionally left blank. */
		break;
	}
}

void check_service_message(int value)
{

	int err = -1;

	NFC_TNEP_SERIVCE_SELECT_RECORD_DESC_DEF(
			my_service_1,
			training_services[0].parameters->svc_name_uri_length,
			training_services[0].parameters->svc_name_uri);

	NFC_TNEP_SERIVCE_SELECT_RECORD_DESC_DEF(
			my_service_2,
			training_services[1].parameters->svc_name_uri_length,
			training_services[1].parameters->svc_name_uri);

	NFC_NDEF_MSG_DEF(my_service_msg, 1);

	switch (value) {
	case 1:
		err = nfc_ndef_msg_record_add(
				&NFC_NDEF_MSG(my_service_msg),
				&NFC_NDEF_TNEP_RECORD_DESC(my_service_1));
		break;
	case 2:
		err = nfc_ndef_msg_record_add(
				&NFC_NDEF_MSG(my_service_msg),
				&NFC_NDEF_TNEP_RECORD_DESC(deselect_service));
		break;
	case 3:
		err = nfc_ndef_msg_record_add(
				&NFC_NDEF_MSG(my_service_msg),
				&NFC_NDEF_TEXT_RECORD_DESC(app_data_rec));
		break;
	case 4:
		err = nfc_ndef_msg_record_add(
				&NFC_NDEF_MSG(my_service_msg),
				&NFC_NDEF_TNEP_RECORD_DESC(my_service_2));
		break;
	default:
		printk("check service message argument error\n");
		break;
	}

	if (err < 0) {
		printk("Cannot add record!\n");
	}

	memset(tag_buffer, 0x00, sizeof(tag_buffer));

	tag_buffer_size = sizeof(tag_buffer);

	err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(my_service_msg), tag_buffer,
				  &tag_buffer_size);
	if (err < 0) {
		printk("Cannot encode message!\n");
	}

	/* NLEN field detection */
	if (IS_ENABLED(CONFIG_NFC_NDEF_MSG_WITH_NLEN)) {
		u32_t buffer_start = 0;

		buffer_start += NLEN_FIELD_SIZE;

		memcpy(tag_buffer, &tag_buffer[buffer_start], tag_buffer_size);
	}
}

static void button_pressed(u32_t button_state, u32_t has_changed)
{
	u32_t button = button_state & has_changed;

	if (button & DK_BTN1_MSK) {
		check_service_message(1);

		nfc_tnep_rx_msg_indicate(tag_buffer, tag_buffer_size);
	}

	if (button & DK_BTN2_MSK) {
		check_service_message(2);

		nfc_tnep_rx_msg_indicate(tag_buffer, tag_buffer_size);
	}

	if (button & DK_BTN3_MSK) {
		check_service_message(3);

		nfc_tnep_rx_msg_indicate(tag_buffer, tag_buffer_size);
	}

	if (button & DK_BTN4_MSK) {
		check_service_message(4);

		nfc_tnep_rx_msg_indicate(tag_buffer, tag_buffer_size);
	}
}

/**
 * @brief   Function for application main entry.
 */
int main(void)
{
	printk("nfc read write demo\n");

	int err;

	log_init();

	err = dk_leds_init();

	if (err) {
		printk("led init error %d", err);
	}

	err = dk_buttons_init(button_pressed);

	if (err) {
		printk("buttons init error %d", err);
	}

	/* TNEP init */

	nfc_tnep_tx_msg_buffer_register(tag_buffer, tag_buffer_size);

	nfc_tnep_init(training_services, ARRAY_SIZE(training_services));

	nfc_tnep_tx_msg_app_data(&NFC_NDEF_TEXT_RECORD_DESC(app_data_rec));

	/* Type X Tag init */

	err = nfc_t4t_setup(nfc_callback, NULL);
	if (err < 0) {
		LOG_ERR("nfc_t4t_setup");
		return err;
	}

	err = nfc_t4t_ndef_rwpayload_set(tag_buffer, tag_buffer_size);
	if (err < 0) {
		LOG_ERR("nfc_t4t_ndef_rwpayload_set");
		return err;
	}

	err = nfc_t4t_emulation_start();
	if (err < 0) {
		LOG_ERR("nfc_t4t_emulation_start");
		return err;
	}

	dk_set_led_on(LED_INIT);

	/* loop */
	for (;;) {

		err = nfc_tnep_process();

		if (!log_process(true)) {
			__WFE();
		}
	}

	return 0;
}
/** @} */
