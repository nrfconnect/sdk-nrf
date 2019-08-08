/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <unity.h>
#include <nfc/tnep/tag.h>
#include <zephyr.h>
#include <string.h>
#include <stdio.h>

void test_tnep_version_is_8_bit(void)
{
	bool is_8_bit = (NFC_TNEP_VRESION == (NFC_TNEP_VRESION & (0xff)));

	TEST_ASSERT_EQUAL(true, is_8_bit);
}

void test_tnep_set_message_buffer(void)
{
	u8_t test_buffer[1];

	int err = nfc_tnep_tx_msg_buffer_register(test_buffer, 1);

	nfc_tnep_uninit();

	TEST_ASSERT_EQUAL(0, err);
}

void test_tnep_set_message_buffer_null(void)
{
	int err = nfc_tnep_tx_msg_buffer_register(NULL, 1);

	nfc_tnep_uninit();

	TEST_ASSERT_EQUAL(-EINVAL, err);
}

void test_tnep_set_message_buffer_zero_legnt(void)
{
	u8_t test_buffer[1];

	int err = nfc_tnep_tx_msg_buffer_register(test_buffer, 0);

	nfc_tnep_uninit();

	TEST_ASSERT_EQUAL(-EINVAL, err);
}

void test_tnep_init_one_service(void)
{
	u8_t test_buffer[1];

	u8_t uri[] = "test";

	NFC_TNEP_SERVICE_DEF(one, uri, ARRAY_SIZE(uri), 0, 1, 1, NULL, NULL,
			     NULL, NULL, NULL);

	struct nfc_tnep_service main_services[] = {
					NFC_TNEP_SERVICE(one),
			};

	nfc_tnep_tx_msg_buffer_register(test_buffer, 1);

	int err = nfc_tnep_init(main_services, 1);

	nfc_tnep_uninit();

	TEST_ASSERT_EQUAL(0, err);
}

void test_tnep_uninit(void)
{
	u8_t test_buffer[1];

	u8_t uri[] = "test";

	NFC_TNEP_SERVICE_DEF(one, uri, ARRAY_SIZE(uri), 0, 1, 1, NULL, NULL,
			     NULL, NULL, NULL);

	struct nfc_tnep_service test_service[] = {
					NFC_TNEP_SERVICE(one),
			};

	nfc_tnep_init(test_service, 1);

	nfc_tnep_uninit();

	nfc_tnep_tx_msg_buffer_register(test_buffer, 1);

	int err = nfc_tnep_init(test_service, 1);

	nfc_tnep_uninit();

	TEST_ASSERT_EQUAL(0, err);
}

void test_tnep_init_many_seices(void)
{
	u8_t test_buffer[1];

	u8_t uri[] = "test";

	NFC_TNEP_SERVICE_DEF(one, uri, ARRAY_SIZE(uri), 0, 1, 1, NULL, NULL,
			     NULL, NULL, NULL);

	NFC_TNEP_SERVICE_DEF(two, uri, ARRAY_SIZE(uri), 0, 1, 1, NULL, NULL,
			     NULL, NULL, NULL);

	NFC_TNEP_SERVICE_DEF(three, uri, ARRAY_SIZE(uri), 0, 1, 1, NULL, NULL,
			     NULL, NULL, NULL);

	struct nfc_tnep_service main_services[] = {
					NFC_TNEP_SERVICE(one),
					NFC_TNEP_SERVICE(two),
					NFC_TNEP_SERVICE(three),
			};

	nfc_tnep_uninit();

	nfc_tnep_tx_msg_buffer_register(test_buffer, 1);

	int err = nfc_tnep_init(main_services, 3);

	nfc_tnep_uninit();

	TEST_ASSERT_EQUAL(0, err);
}

void test_tnep_init_null_service_argument(void)
{
	u8_t test_buffer[1];

	nfc_tnep_tx_msg_buffer_register(test_buffer, 1);

	int err = nfc_tnep_init(NULL, 1);

	nfc_tnep_uninit();

	TEST_ASSERT_EQUAL(-EINVAL, err);
}

void test_tnep_init_zero_services_amount_argument(void)
{
	u8_t test_buffer[1];

	struct nfc_tnep_service test_service[1];

	nfc_tnep_tx_msg_buffer_register(test_buffer, 1);

	int err = nfc_tnep_init(test_service, 0);

	nfc_tnep_uninit();

	TEST_ASSERT_EQUAL(-EINVAL, err);
}

void test_tnep_init_without_buffer_registered(void)
{
	u8_t uri[] = "test";

	NFC_TNEP_SERVICE_DEF(one, uri, ARRAY_SIZE(uri), 0, 1, 1, NULL, NULL,
			     NULL, NULL, NULL);

	struct nfc_tnep_service test_service[] = {
					NFC_TNEP_SERVICE(one),
			};

	nfc_tnep_uninit();

	int err = nfc_tnep_init(test_service, 1);

	nfc_tnep_uninit();

	TEST_ASSERT_EQUAL(-EIO, err);
}

void test_tnep_init_while_already_inited(void)
{
	u8_t test_buffer[1];

	struct nfc_tnep_service test_service[1];

	nfc_tnep_tx_msg_buffer_register(test_buffer, 1);

	nfc_tnep_init(test_service, 1);

	int err = nfc_tnep_init(test_service, 1);

	nfc_tnep_uninit();

	TEST_ASSERT_EQUAL(-ENOTSUP, err);
}

void test_tnep_process(void)
{
	u8_t test_buffer[1024];
	u8_t uri[] = "test";

	NFC_TNEP_SERVICE_DEF(one, uri, ARRAY_SIZE(uri), 0, 1, 1, NULL, NULL,
			     NULL, NULL, NULL);

	struct nfc_tnep_service main_services[1] = {
					NFC_TNEP_SERVICE(one),
			};

	nfc_tnep_tx_msg_buffer_register(test_buffer, 1024);

	nfc_tnep_init(main_services, 1);

	int err = nfc_tnep_process();

	nfc_tnep_uninit();

	TEST_ASSERT_EQUAL(0, err);
}

void test_tnep_app_record_add(void)
{
	struct nfc_ndef_record_desc record;

	int err = nfc_tnep_tx_msg_app_data(&record);

	TEST_ASSERT_EQUAL(0, err);
}

void test_tnep_app_record_add_many(void)
{
	struct nfc_ndef_record_desc record_1;
	struct nfc_ndef_record_desc record_2;
	struct nfc_ndef_record_desc record_3;

	int err_1 = nfc_tnep_tx_msg_app_data(&record_1);
	int err_2 = nfc_tnep_tx_msg_app_data(&record_2);
	int err_3 = nfc_tnep_tx_msg_app_data(&record_3);

	TEST_ASSERT_EQUAL_MESSAGE(0, err_1, "Add the first app data record");
	TEST_ASSERT_EQUAL_MESSAGE(0, err_2, "Add the second app data record");
	TEST_ASSERT_EQUAL_MESSAGE(0, err_3, "Add the third app data record");
}

void test_tnep_app_record_null_argument(void)
{
	int err = nfc_tnep_tx_msg_app_data(NULL);

	TEST_ASSERT_EQUAL(-EINVAL, err);
}

/* static data used in more complex tests */
static volatile int counter_service_selected;
static volatile int counter_service_deselected;
static volatile int counter_service_message;
static volatile int counter_service_timeout;
static volatile int counter_service_error;

static u8_t training_service_selected(void)
{
	counter_service_selected++;

	return 0;
}

static void training_service_deselected(void)
{
	counter_service_deselected++;
}

static void training_service_new_message(void)
{
	counter_service_message++;
}

static void training_service_timeout(void)
{
	counter_service_timeout++;
}

static void training_service_error(int err_code)
{
	counter_service_error = err_code;
}

u8_t training_uri_one[] = "uri:nfc:one";
const size_t wait_time = 200;
const u8_t wait_time_periods = 4;

NFC_TNEP_SERVICE_DEF(tnep_message, training_uri_one,
		     ARRAY_SIZE(training_uri_one),
		     NFC_TNEP_COMM_MODE_SINGLE_RESPONSE, (u8_t)wait_time,
		     (u8_t)wait_time_periods, training_service_selected,
		     training_service_deselected, training_service_new_message,
		     training_service_timeout, training_service_error);

struct nfc_tnep_service training_services[1] = {
				NFC_TNEP_SERVICE(tnep_message),
		};

/**
 * Find the first occurrence of find in s.
 */
static int strstr_with_len(const char *s, size_t s_len, const char *find,
			   size_t find_len)
{
	for (int i = 0; i <= (s_len - find_len); i++) {
		if (!memcmp(&s[i], find, find_len)) {
			return i;
		}
	}

	return -1;
}

/**
 * Initialized tnep service, declared earlier as @ref tnep_message.
 * @param s Requested string,
 * @param s_len Requested string length.
 *
 * @retval true if buffer message contain given string, false if not.
 */
static bool is_tnep_message_buffer_contatins_string(const u8_t *s, size_t s_len)
{
	int err = 0;

	u8_t test_buffer[1024];

	struct nfc_tnep_service training_services[1] = {
					NFC_TNEP_SERVICE(tnep_message),
			};

	nfc_tnep_uninit();

	err = nfc_tnep_tx_msg_buffer_register(test_buffer,
					   ARRAY_SIZE(test_buffer));

	TEST_ASSERT_EQUAL_MESSAGE(0, err, "buffer set error");

	err = nfc_tnep_init(training_services, ARRAY_SIZE(training_services));

	TEST_ASSERT_EQUAL_MESSAGE(0, err, "tnep init error");

	err = nfc_tnep_process();

	TEST_ASSERT_EQUAL_MESSAGE(0, err, "tnep process error");

	nfc_tnep_uninit();

	int strig_position = strstr_with_len(test_buffer,
					     ARRAY_SIZE(test_buffer), s, s_len);

	return (strig_position >= 0);

}

void test_tnep_message_buffer_contatins_uri(void)
{
	bool result = is_tnep_message_buffer_contatins_string(
			training_uri_one, ARRAY_SIZE(training_uri_one) - 1);

	TEST_ASSERT_TRUE_MESSAGE(result, "No uri in tnep message buffer");
}

void test_tnep_message_buffer_contatins_communication_mode(void)
{
	u8_t com_mode_u8 = NFC_TNEP_COMM_MODE_SINGLE_RESPONSE;

	bool result = is_tnep_message_buffer_contatins_string(&com_mode_u8, 2);

	TEST_ASSERT_TRUE_MESSAGE(result,
				 "No communication mode in tnep message buffer");
}

void test_tnep_message_buffer_contatins_wait_time(void)
{
	u8_t wait_time_u8 = (u8_t) wait_time;

	bool result = is_tnep_message_buffer_contatins_string(&wait_time_u8, 1);

	TEST_ASSERT_TRUE_MESSAGE(result, "No wait time in tnep message buffer");
}

void test_tnep_message_buffer_contatins_wait_time_periods(void)
{
	u8_t wait_periods = wait_time_periods;

	bool result = is_tnep_message_buffer_contatins_string(&wait_periods, 1);

	TEST_ASSERT_TRUE_MESSAGE(
			result,
			"No wait time periods number in tnep message buffer");
}

static void tnep_init_and_select_service(u8_t *buffer, size_t buffer_length)
{
	/* Arrange - TNEP init with service details message*/

	int err = 0;

	nfc_tnep_uninit();

	err = nfc_tnep_tx_msg_buffer_register(buffer, buffer_length);

	TEST_ASSERT_EQUAL_MESSAGE(0, err, "Buffer set error.");

	err = nfc_tnep_init(training_services, ARRAY_SIZE(training_services));

	TEST_ASSERT_EQUAL_MESSAGE(0, err, "TNEP Init error.");

	err = nfc_tnep_process();

	TEST_ASSERT_EQUAL_MESSAGE(0, err, "TNEP Process init error.");

	/* Arrange - Prepare mock service select message */

	NFC_TNEP_SERIVCE_SELECT_RECORD_DESC_DEF(
			select_service,
			training_services[0].parameters->svc_name_uri_length,
			training_services[0].parameters->svc_name_uri);

	NFC_NDEF_MSG_DEF(mock_msg, 1);

	err = nfc_ndef_msg_record_add(
			&NFC_NDEF_MSG(mock_msg),
			&NFC_NDEF_TNEP_RECORD_DESC(select_service));

	TEST_ASSERT_EQUAL_MESSAGE(0, err,
				  "Prepare service select message error.");

	u32_t len = buffer_length;
	err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(mock_msg), buffer, &len);

	TEST_ASSERT_EQUAL_MESSAGE(0, err,
				  "Encode mock message to buffer error.");

	if (IS_ENABLED(CONFIG_NFC_NDEF_MSG_WITH_NLEN)) {
		u32_t buffer_start = 0;
		buffer_start += NLEN_FIELD_SIZE;

		memcpy(buffer, &buffer[buffer_start], len);
	}

	nfc_tnep_rx_msg_indicate(buffer, len);
}

void test_tnep_service_select(void)
{
	int err;

	/* Arrange */
	u8_t test_buffer[1024];

	tnep_init_and_select_service(test_buffer, ARRAY_SIZE(test_buffer));

	/* Act - indicate and process prepared service select message */

	counter_service_selected = 0;

	err = nfc_tnep_process();

	TEST_ASSERT_EQUAL_MESSAGE(0, err, "TNEP Process service select error.");

	/* Assert - Was the callback function called? */

	TEST_ASSERT_GREATER_THAN_MESSAGE(0, counter_service_selected,
					 "No service selected callback error.");
}

void test_tnep_service_deselect(void)
{
	int err;

	/* Arrange - Service select */
	u8_t test_buffer[1024];

	tnep_init_and_select_service(test_buffer, ARRAY_SIZE(test_buffer));

	err = nfc_tnep_process();

	TEST_ASSERT_EQUAL_MESSAGE(0, err,
				  "Can't select service before deselection.");

	/* Arrange - Service deselect message */

	NFC_TNEP_SERIVCE_SELECT_RECORD_DESC_DEF(deselect_service, 0, NULL);

	NFC_NDEF_MSG_DEF(mock_msg, 1);

	err = nfc_ndef_msg_record_add(
			&NFC_NDEF_MSG(mock_msg),
			&NFC_NDEF_TNEP_RECORD_DESC(deselect_service));

	TEST_ASSERT_EQUAL_MESSAGE(0, err,
				  "Prepare service deselect message error.");

	u32_t len = ARRAY_SIZE(test_buffer);
	err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(mock_msg), test_buffer, &len);

	TEST_ASSERT_EQUAL_MESSAGE(0, err,
				  "Encode mock message to buffer error.");

	if (IS_ENABLED(CONFIG_NFC_NDEF_MSG_WITH_NLEN)) {
		u32_t buffer_start = 0;
		buffer_start += NLEN_FIELD_SIZE;

		memcpy(test_buffer, &test_buffer[buffer_start],
		       ARRAY_SIZE(test_buffer));
	}

	nfc_tnep_rx_msg_indicate(test_buffer, ARRAY_SIZE(test_buffer));

	/* Act */

	counter_service_deselected = 0;

	err = nfc_tnep_process();

	TEST_ASSERT_EQUAL_MESSAGE(0, err,
				  "TNEP Process service deselect error.");

	int strig_position = strstr_with_len(test_buffer,
					     ARRAY_SIZE(test_buffer),
					     training_uri_one,
					     ARRAY_SIZE(training_uri_one));

	/* Assert - Was the callback function called? */

	TEST_ASSERT_GREATER_THAN_MESSAGE(
			0, counter_service_deselected,
			"No service deselected callback error.");

	TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(
			0, strig_position,
			"No service details message after deselection.");

}

void test_tnep_service_new_message(void)
{
	int err;

	/* Arrange - Service select */
	u8_t test_buffer[1024];

	tnep_init_and_select_service(test_buffer, ARRAY_SIZE(test_buffer));

	err = nfc_tnep_process();

	TEST_ASSERT_EQUAL_MESSAGE(
			0, err, "Can't select service before sending message.");

	/* Arrange - Message without service data*/
	u8_t msg[] = "application data";

	NFC_NDEF_RECORD_BIN_DATA_DEF(bin_data_rec, TNF_WELL_KNOWN, NULL, 0,
				     NULL, 0, msg, sizeof(msg));

	NFC_NDEF_MSG_DEF(mock_msg, 1);

	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(mock_msg),
				      &NFC_NDEF_RECORD_BIN_DATA(bin_data_rec));

	TEST_ASSERT_EQUAL_MESSAGE(0, err,
				  "Prepare service deselect message error.");

	u32_t len = ARRAY_SIZE(test_buffer);
	err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(mock_msg), test_buffer, &len);

	TEST_ASSERT_EQUAL_MESSAGE(0, err,
				  "Encode mock message to buffer error.");

	if (IS_ENABLED(CONFIG_NFC_NDEF_MSG_WITH_NLEN)) {
		u32_t buffer_start = 0;
		buffer_start += NLEN_FIELD_SIZE;

		memcpy(test_buffer, &test_buffer[buffer_start],
		       ARRAY_SIZE(test_buffer));
	}

	nfc_tnep_rx_msg_indicate(test_buffer, ARRAY_SIZE(test_buffer));

	/* Act - indicate and process prepared service select message */

	counter_service_message = 0;

	err = nfc_tnep_process();

	TEST_ASSERT_EQUAL_MESSAGE(0, err, "TNEP Process service select error.");

	/* Assert - Was the callback function called? */

	TEST_ASSERT_GREATER_THAN_MESSAGE(0, counter_service_message,
					 "No message callback error.");
}

