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

/** @brief Length of EFSMSP field's static part. */
#define CRSM_EFSMSP_STATIC_LEN 58
/** @brief Index to Parameter Indicators field in EFSMSP from the end of the field data. */
#define CRSM_EFSMSP_PARAMETER_INDICATORS_FROM_END_INDEX (CRSM_EFSMSP_STATIC_LEN - 2)

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
 * @retval -EINVAL Invalid parameter, that is, number is empty or too long (over 20 characters).
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
		LOG_ERR("SMS number zero length");
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

	if (*number_size > SMS_MAX_ADDRESS_LEN_CHARS) {
		LOG_ERR("Number length exceeds limit (%d): %s",
			SMS_MAX_ADDRESS_LEN_CHARS,
			number);
		return -EINVAL;
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
 * @brief Encode SMS Service Center Address (SCA) field for type approval SIMs.
 *
 * @details This function reads information from SIM card as follows:
 *   - Read EFAD field and check if it is a type approval SIM.
 *     EFAD is specified in 3GPP TS 51.011 chapter 10.3.18.
 *   - Read EFSMSP field and check if it has TS-Service Centre Address.
 *     EFSMSP is specified in 3GPP TS 51.011 chapter 10.5.6.
 *
 * Based on this information, the SCA field is set as follows:
 *   - If not type approval SIM, set empty SCA ("00").
 *   - If type approval SIM and SCA exists, set empty SCA ("00").
 *   - If type approval SIM and SCA does not exist, set SCA to '7' ("0291F7").
 *
 * '7' has been used as SCA for type approval SIMs without SCA in some other systems,
 * such as Android, for more than a decade.
 *
 * SIM fields are read with AT+CRSM command, which is specified in 3GPP TS 27.007 chapter 8.18
 * and some parameter encodings in ETSI TS 102 221 chapter 10.2.1.
 *
 * SCA field is used in SMS-SUBMIT message. This is specified piece by piece
 * in the following specifications:
 *   - 3GPP TS 23.040 chapter 9.1.2.5
 *   - 3GPP TS 27.005 chapter 3.1 (see <pdu>, <tosca> and <toda>)
 *   - 3GPP TS 27.005 chapter 3.3.1
 *   - 3GPP TS 27.005 chapter 4.3
 *   - 3GPP TS 24.011 chapter 8.2.5.2
 *
 * @param[in,out] sca_field Buffer for SCA field. Must be at least 7 bytes long.
 */
static void sms_submit_encode_sca(char *sca_field)
{
	int ret;
	char *efsmsp_parameter_indicators_field;
	int crsm_sw1 = 0;
	int crsm_sw2 = 0;
	int crsm_resp_len = 0;
	uint8_t parameter_indicators = 0;
	int bin_buf_len = 0;

	/* sms_buf_tmp is used for CRSM <response> field which could theoretically be
	 * 512 characters as hexadecimal string (256 bytes). And technically the response
	 * could be split into multiple parts. So we will fail to recognize type approval SIM
	 * and SCA if the <response> is longer than 255 bytes (510 characters).
	 * This limitation should not be a problem in practice.
	 */

	/* Check if this is a type approval SIM */

	/* Read EFAD (0x6FAD = 28589): 3GPP TS 51.011 chapter 10.3.18 */
	ret = nrf_modem_at_scanf(
		"AT+CRSM=176,28589,0,0,0",
		"+CRSM: %d, %d, \"%511[^\"]\"",
		&crsm_sw1,
		&crsm_sw2,
		sms_buf_tmp);

	LOG_DBG("EFAD: (ret=%d, crsm_sw1=%d, crsm_sw2=%d): %s",
		ret, crsm_sw1, crsm_sw2, sms_buf_tmp);

	if (ret != 3 || crsm_sw1 != 0x90 || crsm_sw2 != 0x00 || strlen(sms_buf_tmp) == 0) {
		LOG_WRN("Failed to read UICC EFSMSP (ret=%d, crsm_sw1=%d, crsm_sw2=%d): %s",
			ret, crsm_sw1, crsm_sw2, sms_buf_tmp);
		goto empty_sca_set;
	}
	if (sms_buf_tmp[0] != '8') {
		LOG_DBG("Not a type approval SIM");
		goto empty_sca_set;
	}

	LOG_DBG("Type approval SIM detected");

	/* Check if SIM has SMS Service Center Address */
	sms_buf_tmp[0] = '\0';

	/* Read EFSMSP (0x6F42 = 28482): 3GPP TS 51.011 chapter 10.5.6 */
	ret = nrf_modem_at_scanf(
		"AT+CRSM=178,28482,1,4,0",
		"+CRSM: %d, %d, \"%511[^\"]\"",
		&crsm_sw1,
		&crsm_sw2,
		sms_buf_tmp);

	LOG_DBG("EFSMSP: (ret=%d, crsm_sw1=%d, crsm_sw2=%d): %s",
		ret, crsm_sw1, crsm_sw2, sms_buf_tmp);

	if (ret != 3 || crsm_sw1 != 0x90 || crsm_sw2 != 0x00) {
		LOG_WRN("Failed to read UICC EFSMSP (ret=%d, crsm_sw1=%d, crsm_sw2=%d): %s",
			ret, crsm_sw1, crsm_sw2, sms_buf_tmp);
		goto ta_sim_no_sca;
	}

	crsm_resp_len = strlen(sms_buf_tmp);
	if (crsm_resp_len < CRSM_EFSMSP_STATIC_LEN) {
		LOG_WRN("CRSM response too short (less than %d): %d",
			CRSM_EFSMSP_STATIC_LEN, crsm_resp_len);
		goto ta_sim_no_sca;
	}

	efsmsp_parameter_indicators_field =
		sms_buf_tmp + crsm_resp_len - CRSM_EFSMSP_PARAMETER_INDICATORS_FROM_END_INDEX;

	/* Decode Parameter Indicators */
	bin_buf_len = hex2bin(efsmsp_parameter_indicators_field, 2, &parameter_indicators, 1);

	/* If parameter indicator is not a number, decoding it failed */
	if (bin_buf_len != 1) {
		LOG_WRN("Invalid Parameter Indicators field");
		goto ta_sim_no_sca;
	}

	/* If bit #2 is set, TS-Service Centre Address does not exist */
	if ((parameter_indicators & 0x02) != 0) {
		LOG_DBG("No TS-Service Centre Address in SIM");
		goto ta_sim_no_sca;
	}

	LOG_DBG("TS-Service Centre Address exists in SIM. Using empty address.");

empty_sca_set:
	/* Empty SCA field */
	strcpy(sca_field, "00");
	return;

ta_sim_no_sca:
	/* Add '7' as service center address for type approval without SCA.
	 * We use default value of 145 (0x91), which is for international number format.
	 */
	LOG_DBG("Setting service center address to '7' for type approval SIM");
	sprintf(sca_field, "0291F7");
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
 * @brief Create SMS-SUBMIT message as specified in 3GPP TS 23.040 chapter 9.2.2.2.
 *
 * @details Optionally allows adding User-Data-Header.
 *
 * For the encoding of the SMS PDU, see <pdu> in 3GPP TS 27.005 chapter 3.1.
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
 * @param[in] sca_field SMS Service Center address field of the SMS pdu within CMGS AT command.
 *                      Buffer must be at least 7 bytes long.
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
	char *udh_str,
	char *sca_field)
{
	int err = 0;
	int msg_size;
	uint8_t sms_submit_header_byte = 0x01; /* Indicates SMS-SUBMIT message type */
	uint8_t ud_start_index;
	uint8_t udh_size = (udh_str == NULL) ? 0 : strlen(udh_str) / 2;

	/* Create and send CMGS AT command */
	msg_size =
		/* SMSC address octets are excluded based on 3GPP TS 27.005 chapter 4.3 */
		2 + /* First header byte and TP-MR fields */
		1 + /* Length of phone number */
		1 + /* Phone number Type-of-Address byte */
		encoded_number_size_octets +
		2 + /* TP-PID and TP-DCS fields */
		1 + /* TP-UDL field */
		udh_size +
		encoded_data_size_octets;

#if defined(CONFIG_SMS_STATUS_REPORT)
	/* Request SMS-STATUS-REPORT message with TP-SRR field (bit #5) */
	sms_submit_header_byte |= 0x20;
#endif
	/* If User-Data-Header is added to SMS-SUBMIT, UDHI bit #6 must be set */
	if (udh_str != NULL) {
		sms_submit_header_byte |= 0x40;
	}

	/* First, compose SMS header without User-Data so that we get an index for
	 * User-Data-Header to be added later
	 */
	sprintf(send_buf,
		"AT+CMGS=%d\r%s%02X%02X%02X91%s0000%02X",
		msg_size,
		sca_field,
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

	err = nrf_modem_at_printf("%s", send_buf);
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
 * @param[in] sca_field SMS Service Center address/number field of the CMGS AT command.
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
	char *sca_field,
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

			if (text_encoded_size < text_size) {
				/* Inform that there are more messages to send */
				(void)nrf_modem_at_printf("AT+CMMS=1");
			} else {
				/* Inform that the next message is the last one */
				(void)nrf_modem_at_printf("AT+CMMS=0");
			}

			err = sms_submit_encode(
				sms_buf_tmp,
				encoded_number,
				encoded_number_size,
				encoded_number_size_octets,
				sms_payload_tmp + SMS_UDH_CONCAT_SIZE_OCTETS,
				encoded_data_size_octets - SMS_UDH_CONCAT_SIZE_OCTETS,
				encoded_data_size_septets,
				message_ref,
				udh_str,
				sca_field);

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
	char sca_field[7] = { 0 };

	if (number == NULL) {
		LOG_ERR("SMS number NULL");
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

	/* Encode SCA field */
	sms_submit_encode_sca(sca_field);

	/* Check if this should be sent as concatenated SMS */
	if (size < data_len) {
		LOG_DBG("Entire message doesn't fit into one SMS message. Using concatenated SMS.");

		/* Encode messages to get number of message parts required to send given text */
		err = sms_submit_concat(data,
			encoded_number, encoded_number_size, encoded_number_size_octets, sca_field,
			false, &concat_msg_count);

		/* Then, encode and send message parts */
		err = sms_submit_concat(data,
			encoded_number, encoded_number_size, encoded_number_size_octets, sca_field,
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
			NULL,
			sca_field);
	}

	return err;
}
