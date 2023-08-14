/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <nrf_modem_at.h>
#include <modem/sms.h>

#include "sms_internal.h"
#include "string_conversion.h"


LOG_MODULE_DECLARE(sms, CONFIG_SMS_LOG_LEVEL);

/** @brief User Data Header size in octets/bytes. */
#define SMS_UDH_CONCAT_SIZE_OCTETS 6

/** @brief User Data Header size in septets. */
#define SMS_UDH_CONCAT_SIZE_SEPTETS 7

/** @brief Maximum length of SMS paylod for message part of concatenated SMS. */
#define SMS_MAX_CONCAT_PAYLOAD_LEN_CHARS \
	(SMS_MAX_PAYLOAD_LEN_CHARS - SMS_UDH_CONCAT_SIZE_SEPTETS)

/**
 * @brief Encode phone number into format specified within SMS header.
 *
 * @details Phone number means address specified in 3GPP TS 23.040 chapter 9.1.2.5.
 *
 * @param[in] number Number as a string.
 * @param[in,out] number_size In: Length of the number string.
 *                    Out: Amount of characters in number. Special characters
 *                         ignored from original number_size. This is also
 *                         number of semi-octets in encoded_number.
 * @param[in] encoded_number Number encoded into 3GPP format.
 * @param[out] encoded_number_size_octets Number of octets/bytes in encoded_number.
 *
 * @retval -EINVAL Invalid parameter.
 * @return Zero on success, otherwise error code.
 */
static int sms_submit_encode_number(
	const char *number,
	uint8_t *number_size,
	char *encoded_number,
	uint8_t *encoded_number_size_octets)
{
	*encoded_number_size_octets = 0;

	__ASSERT_NO_MSG(number != NULL);

	if (*number_size == 0) {
		LOG_ERR("SMS number not given but zero length");
		return -EINVAL;
	}

	if (number[0] == '+') {
		/* If first character of the number is plus, just ignore it.
		 * We are using international number format always anyway.
		 */
		number += 1;
		*number_size = strlen(number);
		LOG_DBG("Ignoring leading '+' in the number");
	}

	memset(encoded_number, 0, SMS_MAX_ADDRESS_LEN_CHARS + 1);
	memcpy(encoded_number, number, *number_size);

	for (int i = 0; i < *number_size; i++) {
		if (!(i % 2)) {
			if (i + 1 < *number_size) {
				char first = encoded_number[i];
				char second = encoded_number[i+1];

				encoded_number[i] = second;
				encoded_number[i+1] = first;
			} else {
				encoded_number[i+1] = encoded_number[i];
				encoded_number[i] = 'F';
			}
			(*encoded_number_size_octets)++;
		}
	}
	return 0;
}

/**
 * @brief Prints error information for positive error codes of @c nrf_modem_at_printf.
 *
 * @param[in] err Error code.
 */
static void sms_submit_print_error(int err)
{
#if (CONFIG_SMS_LOG_LEVEL >= LOG_LEVEL_ERR)
	if (err <= 0) {
		return;
	}

	switch (nrf_modem_at_err_type(err)) {
	case NRF_MODEM_AT_ERROR:
		LOG_ERR("ERROR");
		break;
	case NRF_MODEM_AT_CME_ERROR:
		LOG_ERR("+CME ERROR: %d", nrf_modem_at_err(err));
		break;
	case NRF_MODEM_AT_CMS_ERROR:
		LOG_ERR("+CMS ERROR: %d", nrf_modem_at_err(err));
		break;
	}
#endif
}

/**
 * @brief Create SMS-SUBMIT message as specified in specified in 3GPP TS 23.040 chapter 9.2.2.2.
 *
 * @details Optionally allows adding User-Data-Header.
 *
 * @param[out] send_buf Output buffer where SMS-SUBMIT message is created. Caller should allocate
 *             enough space. Minimum of 400 bytes is required with maximum message size and if
 *             encoded User-Data is smaller, buffer can also be smaller.
 * @param[in] encoded_number Number in semi-octet representation for SMS-SUBMIT message.
 * @param[in] encoded_number_size Number of characters in number (encoded_number).
 * @param[in] encoded_number_size_octets Number of octets in number (encoded_number).
 * @param[in] encoded_data Encoded User-Data in bytes.
 * @param[in] encoded_data_size_octets Encoded User-Data size in octets.
 * @param[in] encoded_data_size_septets Encoded User-Data size in septets.
 * @param[in] message_ref TP-Message-Reference field in SMS-SUBMIT message.
 * @param[in] udh_str User Data Header. Can be set to NULL if not desired.
 */
static int sms_submit_encode(
	char *send_buf,
	uint8_t *encoded_number,
	uint8_t encoded_number_size,
	uint8_t encoded_number_size_octets,
	uint8_t *encoded,
	uint8_t encoded_data_size_octets,
	uint8_t encoded_data_size_septets,
	uint8_t message_ref,
	char *udh_str)
{
	int err = 0;
	int msg_size;
	uint8_t sms_submit_header_byte;
	uint8_t ud_start_index;
	uint8_t udh_size = (udh_str == NULL) ? 0 : strlen(udh_str) / 2;

	/* Create and send CMGS AT command */
	msg_size =
		2 + /* First header byte and TP-MR fields */
		1 + /* Length of phone number */
		1 + /* Phone number Type-of-Address byte */
		encoded_number_size_octets +
		2 + /* TP-PID and TP-DCS fields */
		1 + /* TP-UDL field */
		udh_size +
		encoded_data_size_octets;

	/* Set header byte. If User-Data-Header is added to SMS-SUBMIT, UDHI bit must be set */
	sms_submit_header_byte = (udh_str == NULL) ? 0x21 : 0x61;

	/* First, compose SMS header without User-Data so that we get an index for
	 * User-Data-Header to be added later
	 */
	sprintf(send_buf,
		"AT+CMGS=%d\r00%02X%02X%02X91%s0000%02X",
		msg_size,
		sms_submit_header_byte,
		message_ref,
		encoded_number_size,
		encoded_number,
		encoded_data_size_septets);
	/* Store the position for User-Data */
	ud_start_index = strlen(send_buf);

	if (udh_str != NULL) {
		sprintf(send_buf + ud_start_index, "%s", udh_str);
		ud_start_index = strlen(send_buf);
	}

	/* Then, add the actual user data by creating hexadecimal string
	 * representation of GSM 7bit encoded text
	 */
	for (int i = 0; i < encoded_data_size_octets; i++) {
		sprintf(send_buf + ud_start_index + (2 * i), "%02X", encoded[i]);
	}
	send_buf[ud_start_index + encoded_data_size_octets * 2] = '\x1a';
	send_buf[ud_start_index + encoded_data_size_octets * 2 + 1] = '\0';

	LOG_DBG("Sending encoded SMS data (length=%d):", msg_size);
	LOG_DBG("%s", send_buf);

	err = nrf_modem_at_printf(send_buf);
	if (err) {
		LOG_ERR("Sending AT command failed, err=%d", err);
		sms_submit_print_error(err);
	} else {
		LOG_DBG("Sending AT command succeeded");
	}

	return err;
}

/**
 * @brief Encode and, if requested, send concatenated SMS.
 *
 * @details This function can be used to just encode the text and throw away encoded data by setting
 * send_at_cmds=false, in which case this function will calculate number of message parts
 * required to decode given text. This value would be stored in concat_msg_count.
 * Alternatively, this function can be used to encode and send multiple SMS-SUBMIT messages
 * with CMGS AT command by setting send_at_cmds=true, in which case concat_msg_count is used as
 * input for number of message parts.
 *
 * Includes User-Data-Header for concatenated message to indicate information
 * about multiple messages that should be reconstructed in the receiving end.
 * This function should not be used for short texts that don't need concatenation
 * because this will add concatenation User-Data-Header.
 *
 * @param[in] text Text to be sent.
 * @param[in] encoded_number Number in semi-octet representation for SMS-SUBMIT message.
 * @param[in] encoded_number_size Number of characters in number (encoded_number).
 * @param[in] encoded_number_size_octets Number of octets in number (encoded_number).
 * @param[in] send_at_cmds Indicates whether parts of the concatenated SMS message are sent.
 * @param[in,out] concat_msg_count Number of message parts required to decode given text.
 *                     If send_at_cmds is true, this is input.
 *                     If send_at_cmds is false, this is output.
 */
static int sms_submit_concat(
	const char *text,
	uint8_t *encoded_number,
	uint8_t encoded_number_size,
	uint8_t encoded_number_size_octets,
	bool send_at_cmds,
	uint8_t *concat_msg_count)
{
	int err = 0;
	char user_data[SMS_MAX_PAYLOAD_LEN_CHARS];
	char udh_str[13] = {0};

	static uint8_t concat_msg_id = 1;
	static uint8_t message_ref = 1;
	uint8_t concat_seq_number = 0;

	uint8_t size;
	uint16_t text_size = strlen(text);
	uint8_t encoded_data_size_octets = 0;
	uint8_t encoded_data_size_septets = 0;

	uint16_t text_encoded_size = 0;
	const char *text_index = text;
	uint16_t text_part_size;

	memset(user_data, 0, SMS_UDH_CONCAT_SIZE_SEPTETS);

	/* More message parts are created until entire input text is encoded into messages */
	while (text_encoded_size < text_size) {
		concat_seq_number++;

		text_part_size = MIN(strlen(text_index), SMS_MAX_CONCAT_PAYLOAD_LEN_CHARS);
		memcpy(user_data + SMS_UDH_CONCAT_SIZE_SEPTETS, text_index, text_part_size);

		/* User Data Header is included into encoded buffer because the actual data/text
		 * must be aligned into septet (7bit) boundary after User Data Header.
		 */
		size = string_conversion_ascii_to_gsm7bit(user_data,
			SMS_UDH_CONCAT_SIZE_SEPTETS + text_part_size,
			sms_payload_tmp, &encoded_data_size_octets, &encoded_data_size_septets,
			true);

		/* User Data Header septets must be ignored from the text size and indexing */
		text_encoded_size += size - SMS_UDH_CONCAT_SIZE_SEPTETS;
		text_index += size - SMS_UDH_CONCAT_SIZE_SEPTETS;

		if (send_at_cmds) {
			sprintf(udh_str, "050003%02X%02X%02X",
				concat_msg_id, *concat_msg_count, concat_seq_number);

			err = sms_submit_encode(
				sms_buf_tmp,
				encoded_number,
				encoded_number_size,
				encoded_number_size_octets,
				sms_payload_tmp + SMS_UDH_CONCAT_SIZE_OCTETS,
				encoded_data_size_octets - SMS_UDH_CONCAT_SIZE_OCTETS,
				encoded_data_size_septets,
				message_ref,
				udh_str);

			if (err) {
				break;
			}

			message_ref++;

			/* Just looping without threading seems to work fine and we don't
			 * need to wait for CDS response. Otherwise we would need to send
			 * 2nd message from work queue and store a lot of state information.
			 */
		}
	}

	if (send_at_cmds) {
		concat_msg_id++;
	} else {
		*concat_msg_count = concat_seq_number;
	}
	return err;
}

int sms_submit_send(
	const char *number,
	const uint8_t *data,
	uint16_t data_len,
	enum sms_data_type type)
{
	int err;
	uint8_t encoded_number[SMS_MAX_ADDRESS_LEN_CHARS + 1];
	uint8_t encoded_number_size;
	uint8_t encoded_number_size_octets = SMS_MAX_ADDRESS_LEN_CHARS + 1;
	uint8_t size = 0;
	uint8_t encoded_data_size_octets = 0;
	uint8_t encoded_data_size_septets = 0;
	uint8_t concat_msg_count = 0;

	if (number == NULL) {
		LOG_ERR("SMS number not given but NULL");
		return -EINVAL;
	}
	__ASSERT_NO_MSG(data != NULL);

	if (type == SMS_DATA_TYPE_ASCII) {
		LOG_DBG("Sending SMS to number=%s, text='%s', data_len=%d", number, data, data_len);
	} else {
		LOG_DBG("Sending SMS to number=%s, data_len=%d", number, data_len);
	}

	/* Encode number into format required in SMS header */
	encoded_number_size = strlen(number);
	err = sms_submit_encode_number(number, &encoded_number_size,
		encoded_number, &encoded_number_size_octets);
	if (err) {
		return err;
	}

	/* Encode data into GSM 7bit encoding */

	if (type == SMS_DATA_TYPE_ASCII) {
		size = string_conversion_ascii_to_gsm7bit(
			data, data_len, sms_payload_tmp,
			&encoded_data_size_octets, &encoded_data_size_septets, true);
	} else {
		__ASSERT_NO_MSG(type == SMS_DATA_TYPE_GSM7BIT);
		if (data_len > SMS_MAX_PAYLOAD_LEN_CHARS) {
			LOG_ERR("Maximum length (%d) for GMS 7bit encoded message exceeded %d",
				SMS_MAX_PAYLOAD_LEN_CHARS, data_len);
			return -E2BIG;
		}

		memcpy(sms_payload_tmp, data, data_len);
		encoded_data_size_octets = string_conversion_7bit_sms_packing(
			sms_payload_tmp, data_len);
		encoded_data_size_septets = data_len;
		size = data_len;
	}

	/* Check if this should be sent as concatenated SMS */
	if (size < data_len) {
		LOG_DBG("Entire message doesn't fit into one SMS message. Using concatenated SMS.");

		/* Encode messages to get number of message parts required to send given text */
		err = sms_submit_concat(data,
			encoded_number, encoded_number_size, encoded_number_size_octets,
			false, &concat_msg_count);

		/* Then, encode and send message parts */
		err = sms_submit_concat(data,
			encoded_number, encoded_number_size, encoded_number_size_octets,
			true, &concat_msg_count);
	} else {
		/* Single message SMS */
		err = sms_submit_encode(
			sms_buf_tmp,
			encoded_number,
			encoded_number_size,
			encoded_number_size_octets,
			sms_payload_tmp,
			encoded_data_size_octets,
			encoded_data_size_septets,
			0,
			NULL);
	}

	return err;
}
