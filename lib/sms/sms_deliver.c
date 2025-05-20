/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <modem/sms.h>
#include <zephyr/logging/log.h>

#include "sms_deliver.h"
#include "sms_internal.h"
#include "parser.h"
#include "string_conversion.h"

LOG_MODULE_DECLARE(sms, CONFIG_SMS_LOG_LEVEL);

/** @brief Maximum length of SMS address, i.e., phone number, in octets. */
#define SMS_MAX_ADDRESS_LEN_OCTETS 10

/** @brief Length of TP-Service-Centre-Time-Stamp field. */
#define SCTS_FIELD_SIZE 7

/**
 * @brief User Data Header Information Element:
 *        Concatenated short messages, 8-bit reference number.
 *
 * @details This is specified in 3GPP TS 23.040 Section 9.2.3.24.1.
 */
#define SMS_UDH_IE_CONCATENATED_SHORT_MESSAGES_8BIT 0x00

/**
 * @brief User Data Header Information Element:
 *        Application port addressing scheme, 8 bit address.
 *
 * @details This is specified in 3GPP TS 23.040 Section 9.2.3.24.3.
 */
#define SMS_UDH_IE_APPLICATION_PORT_ADDRESSING_SCHEME_8BIT 0x04

/**
 * @brief User Data Header Information Element:
 *        Application port addressing scheme, 16 bit address.
 *
 * @details This is specified in 3GPP TS 23.040 Section 9.2.3.24.4.
 */
#define SMS_UDH_IE_APPLICATION_PORT_ADDRESSING_SCHEME_16BIT 0x05

/**
 * @brief User Data Header Information Element:
 *        Concatenated short messages, 16-bit reference number
 *
 * @details This is specified in 3GPP TS 23.040 Section 9.2.3.24.8.
 */
#define SMS_UDH_IE_CONCATENATED_SHORT_MESSAGES_16BIT 0x08

/**
 * @brief First byte of SMS-DELIVER PDU in 3GPP TS 23.040 chapter 9.2.2.1.
 */
struct pdu_deliver_header {
	uint8_t mti : 2;  /** TP-Message-Type-Indicator */
	uint8_t mms : 1;  /** TP-More-Messages-to-Send */
	uint8_t lp : 1;   /** TP-Loop-Prevention */
	uint8_t:1;        /** Empty bit */
	uint8_t sri : 1;  /** TP-Status-Report-Indication */
	uint8_t udhi : 1; /** TP-User-Data-Header-Indicator */
	uint8_t rp : 1;   /** TP-Reply-Path */
};

/**
 * @brief Data Coding Scheme (DCS) in 3GPP TS 23.038 chapter 4.
 *
 * @details This encoding applies if bits 7..6 are 00.
 */
struct pdu_dcs_field {
	uint8_t class:2;      /** Message Class */
	uint8_t alphabet:2;   /** Character set */
	/** If set to 1, indicates that bits 1 to 0 have a message class
	 *  meaning. Otherwise bits 1 to 0 are reserved.
	 */
	uint8_t presence_of_class:1;
	/** If set to 0, indicates the text is uncompressed.
	 *  Otherwise it's compressed.
	 */
	uint8_t compressed:1;
};

/**
 * @brief SMS-DELIVER PDU fields specified in 3GPP TS 23.040 chapter 9.2.2.1
 */
struct pdu_deliver_data {
	struct pdu_deliver_header header; /** First byte of header */
	struct sms_address        oa;  /** TP-Originating-Address */
	uint8_t                   pid; /** TP-Protocol-Identifier */
	struct pdu_dcs_field      dcs; /** TP-Data-Coding-Scheme */
	struct sms_time           timestamp; /** TP-Service-Centre-Time-Stamp */
	uint8_t                   udl; /** TP-User-Data-Length */
	uint8_t                   udhl; /** User Data Header Length */
	struct sms_udh_app_port   udh_app_port; /** Port addressing */
	struct sms_udh_concat     udh_concat; /** Concatenation */
};

/**
 * @brief Swap upper and lower 4 bits between each other.
 */
static uint8_t swap_nibbles(uint8_t value)
{
	return ((value & 0x0f) << 4) | ((value & 0xf0) >> 4);
}

/**
 * @brief Converts an octet having two semi-octets into a decimal.
 *
 * @details Semi-octet representation is explained in 3GPP TS 23.040 Section 9.1.2.3.
 * An octet has semi-octets in the following order:
 *   semi-octet-digit2, semi-octet-digit1
 * Octet for decimal number '21' is hence represented as semi-octet bits:
 *   00010010
 * This function is needed in timestamp (TP SCTS) conversion that is specified
 * in 3GPP TS 23.040 Section 9.2.3.11.
 *
 * @param[in] value Octet to be converted.
 *
 * @return Decimal value.
 */
static uint8_t semioctet_to_dec(uint8_t value)
{
	/* 4 LSBs represent decimal that should be multiplied by 10. */
	/* 4 MSBs represent decimal that should be add as is. */
	return ((value & 0xf0) >> 4) + ((value & 0x0f) * 10);
}

/**
 * @brief Convert phone number into string format.
 *
 * @param[in] number Number in semi-octet representation.
 * @param[in] number_length Number length.
 * @param[out] str_number Output buffer where number is stored. Size shall be at minimum twice the
 *                        number length rounded up.
 */
static void convert_number_to_str(uint8_t *number, uint8_t number_length, char *str_number)
{
	uint8_t length = number_length / 2;
	uint8_t number_value;
	uint8_t hex_str_index = 0;
	bool fill_bits = false;

	if (number_length % 2 == 1) {
		/* There is one more number in semi-octet and 4 fill bits*/
		length++;
		fill_bits = true;
	}

	for (int i = 0; i < length; i++) {
		/* Handle most significant 4 bits */
		number_value = (number[i] & 0xF0) >> 4;

		if (number_value >= 10) {
			LOG_WRN("Single number in phone number is higher than 10: "
				"index=%d, number_value=%d, higher semi-octet",
				i, number_value);
		}
		sprintf(str_number + hex_str_index, "%d", number_value);

		if (i < length - 1 || !fill_bits) {
			/* Handle least significant 4 bits */
			number_value = number[i] & 0x0F;

			if (number_value >= 10) {
				LOG_WRN("Single number in phone number is higher than 10: "
					"index=%d, number_value=%d, lower semi-octet",
					i, number_value);
			}
			sprintf(str_number + hex_str_index + 1,	"%d", number_value);
		}
		hex_str_index += 2;
	}
	str_number[hex_str_index] = '\0';
}

/**
 * @brief Decode SMS service center number specified in 3GPP TS 24.011 Section 8.2.5.1.
 *
 * @param[in,out] parser Parser instance.
 * @param[in] buf Buffer containing PDU and pointing to this field.
 *
 * @return Number of parsed bytes.
 */
static int decode_pdu_deliver_smsc(struct parser *parser, uint8_t *buf)
{
	uint8_t smsc_size = *buf;

	buf += smsc_size + 1;

	LOG_DBG("SMSC size: %d", smsc_size);

	return smsc_size + 1;
}

/**
 * @brief Decode first byte of SMS-DELIVER header as specified in 3GPP TS 23.040 Section 9.2.2.1.
 *
 * @param[in,out] parser Parser instance.
 * @param[in] buf Buffer containing PDU and pointing to this field.
 *
 * @return Number of parsed bytes.
 */
static int decode_pdu_deliver_header(struct parser *parser, uint8_t *buf)
{
	struct pdu_deliver_data * const pdata = parser->data;

	pdata->header = *((struct pdu_deliver_header *)buf);

	LOG_DBG("SMS header 1st byte: 0x%02X", *buf);

	LOG_DBG("TP-Message-Type-Indicator: %d", pdata->header.mti);
	LOG_DBG("TP-More-Messages-to-Send: %d", pdata->header.mms);
	LOG_DBG("TP-Status-Report-Indication: %d", pdata->header.sri);
	LOG_DBG("TP-User-Data-Header-Indicator: %d", pdata->header.udhi);
	LOG_DBG("TP-Reply-Path: %d", pdata->header.rp);

	return 1;
}

/**
 * @brief Decode TP-Originating-Address as specified in 3GPP TS 23.040 Section 9.2.3.7 and 9.1.2.5.
 *
 * @param[in,out] parser Parser instance.
 * @param[in] buf Buffer containing PDU and pointing to this field.
 *
 * @return Number of parsed bytes.
 */
static int decode_pdu_oa_field(struct parser *parser, uint8_t *buf)
{
	struct pdu_deliver_data * const pdata = parser->data;
	uint8_t address[SMS_MAX_ADDRESS_LEN_OCTETS];
	uint8_t length;

	pdata->oa.length = *buf++;
	pdata->oa.type = (uint8_t)*buf++;

	LOG_DBG("Address-Length: %d", pdata->oa.length);
	LOG_DBG("Type-of-Address: 0x%02X", pdata->oa.type);

	if (pdata->oa.length > SMS_MAX_ADDRESS_LEN_CHARS) {
		LOG_ERR("Maximum address length (%d) exceeded %d. Aborting decoding.",
			SMS_MAX_ADDRESS_LEN_CHARS,
			pdata->oa.length);
		return -EINVAL;
	}

	length = pdata->oa.length / 2;

	if (pdata->oa.length % 2 == 1) {
		/* There is an extra number in semi-octet and fill bits*/
		length++;
	}

	memcpy(address, buf, length);

	for (int i = 0; i < length; i++) {
		address[i] = swap_nibbles(address[i]);
	}

	convert_number_to_str(address, pdata->oa.length, pdata->oa.address_str);

	/* 2 for length and type fields */
	return 2 + length;
}

/**
 * @brief Decode TP-Protocol-Identifier as specified in 3GPP TS 23.040 Section 9.2.3.9.
 *
 * @param[in,out] parser Parser instance.
 * @param[in] buf Buffer containing PDU and pointing to this field.
 *
 * @return Number of parsed bytes.
 */
static int decode_pdu_pid_field(struct parser *parser, uint8_t *buf)
{
	struct pdu_deliver_data * const pdata = parser->data;

	pdata->pid = (uint8_t)*buf;

	LOG_DBG("TP-Protocol-Identifier: %d", pdata->pid);

	return 1;
}

/**
 * @brief Decode TP-Data-Coding-Scheme as specified in 3GPP TS 23.038 Section 4.
 *
 * @param[in,out] parser Parser instance.
 * @param[in] buf Buffer containing PDU and pointing to this field.
 *
 * @return Number of parsed bytes.
 */
static int decode_pdu_dcs_field(struct parser *parser, uint8_t *buf)
{
	struct pdu_deliver_data * const pdata = parser->data;
	uint8_t value = *buf;
	uint8_t temp;

	if ((value & 0b11000000) == 0) {
		/* If bits 7..6 of the Coding Group Bits are 00 */
		pdata->dcs = *((struct pdu_dcs_field *)&value);

	} else if (((value & 0b11110000) >> 4) == 0b1111) {
		/* If bits 7..4 of the Coding Group Bits are 1111,
		 * only first 3 bits are meaningful and they match to
		 * the first 3 bits when Coding Group Bits 7..6 are 00
		 */
		temp = value & 0b00000111;

		pdata->dcs = *(struct pdu_dcs_field *)&temp;
		/* Additionally, to convert Coding Group Bits 7..4=1111 to same
		 * meaning as 7..6=00, message class presence bit should be set.
		 */
		pdata->dcs.presence_of_class = 1;
	} else {
		LOG_ERR("Unsupported data coding scheme (0x%02X)", value);
		return -EINVAL;
	}

	LOG_DBG("TP-Data-Coding-Scheme: 0x%02X", value);

	return 1;
}

/**
 * @brief Decode TP-Service-Centre-Time-Stamp as specified in 3GPP TS 23.040 Section 9.2.3.11.
 *
 * @param[in,out] parser Parser instance.
 * @param[in] buf Buffer containing PDU and pointing to this field.
 *
 * @return Number of parsed bytes.
 */
static int decode_pdu_scts_field(struct parser *parser, uint8_t *buf)
{
	struct pdu_deliver_data * const pdata = parser->data;
	int tmp_tz;

	pdata->timestamp.year   = semioctet_to_dec(*(buf++));
	pdata->timestamp.month  = semioctet_to_dec(*(buf++));
	pdata->timestamp.day    = semioctet_to_dec(*(buf++));
	pdata->timestamp.hour   = semioctet_to_dec(*(buf++));
	pdata->timestamp.minute = semioctet_to_dec(*(buf++));
	pdata->timestamp.second = semioctet_to_dec(*(buf++));

	/* 3GPP TS 23.040 Section 9.2.3.11:
	 *  "The Time Zone indicates the difference, expressed in quarters of an hour,
	 *   between the local time and GMT. In the first of the two semi octets, the first bit
	 *   (bit 3 of the seventh octet of the TP Service Centre Time Stamp field) represents
	 *   the algebraic sign of this difference (0: positive, 1: negative)."
	 */
	tmp_tz = semioctet_to_dec(*buf & 0xf7);

	if (*buf & 0x08) {
		tmp_tz = -(tmp_tz);
	}

	pdata->timestamp.timezone = tmp_tz;

	return SCTS_FIELD_SIZE;
}

/**
 * @brief Decode TP-User-Data-Length as specified in 3GPP TS 23.040 Section 9.2.3.16.
 *
 * @param[in,out] parser Parser instance.
 * @param[in] buf Buffer containing PDU and pointing to this field.
 *
 * @return Number of parsed bytes.
 */
static int decode_pdu_udl_field(struct parser *parser, uint8_t *buf)
{
	struct pdu_deliver_data * const pdata = parser->data;

	pdata->udl = (uint8_t)*buf;

	LOG_DBG("TP-User-Data-Length: %d", pdata->udl);

	return 1;
}

/**
 * @brief Check validity of concatenated message information element.
 *
 * @details This is specified in 3GPP TS 23.040 Section 9.2.3.24.1 and 9.2.3.24.8.
 *
 * @param[in,out] parser Parser instance containing the fields to check. If invalid fields are
 *                       detected, all concatenation fields are reset and whole information
 *                       element is ignored.
 */
static void udh_ie_concat_validity_check(struct pdu_deliver_data * const pdata)
{

	LOG_DBG("UDH concatenated, reference number: %d", pdata->udh_concat.ref_number);
	LOG_DBG("UDH concatenated, total number of messages: %d", pdata->udh_concat.total_msgs);
	LOG_DBG("UDH concatenated, sequence number: %d", pdata->udh_concat.seq_number);

	if (pdata->udh_concat.total_msgs == 0) {
		LOG_ERR("Total number of concatenated messages must be higher than 0, "
			"ignoring concatenated info");
		pdata->udh_concat.present = false;
		pdata->udh_concat.ref_number = 0;
		pdata->udh_concat.total_msgs = 0;
		pdata->udh_concat.seq_number = 0;

	} else if ((pdata->udh_concat.seq_number == 0) ||
		   (pdata->udh_concat.seq_number > pdata->udh_concat.total_msgs)) {

		LOG_ERR("Sequence number of current concatenated message (%d) must be "
			"higher than 0 and smaller or equal than total number of messages (%d), "
			"ignoring concatenated info",
			pdata->udh_concat.seq_number,
			pdata->udh_concat.total_msgs);
		pdata->udh_concat.present = false;
		pdata->udh_concat.ref_number = 0;
		pdata->udh_concat.total_msgs = 0;
		pdata->udh_concat.seq_number = 0;
	}
}

/**
 * @brief Decode User Data Header Information Elements as specified
 * in 3GPP TS 23.040 Section 9.2.3.24.
 *
 * @param[in,out] parser Parser instance.
 * @param[in] buf Buffer containing PDU and pointing to this field.
 *
 * @return Number of parsed bytes.
 */
static void decode_pdu_udh_ie(struct parser *parser, uint8_t *buf)
{
	struct pdu_deliver_data * const pdata = parser->data;
	int ie_id;
	int ie_length;
	/* Start from 1 as 0 index has UDHL */
	uint8_t ofs = 1;

	while (ofs < pdata->udhl) {
		ie_id     = buf[ofs++];
		ie_length = buf[ofs++];

		LOG_DBG("User Data Header id=0x%02X, length=%d", ie_id, ie_length);

		if (ie_length > pdata->udhl - ofs) {
			/* 3GPP TS 23.040 Section 9.2.3.24:
			 *  "If the length of the User Data Header is such that there are too few
			 *   or too many octets in the final Information Element
			 *   then the whole User Data Header shall be ignored."
			 */
			LOG_DBG("User Data Header Information Element too long (%d) for "
				"remaining length (%d)",
				ie_length, pdata->udhl - ofs);

			/* Clean UDH information */
			pdata->udh_concat.present = false;
			pdata->udh_concat.ref_number = 0;
			pdata->udh_concat.total_msgs = 0;
			pdata->udh_concat.seq_number = 0;
			pdata->udh_app_port.present = false;
			pdata->udh_app_port.dest_port = 0;
			pdata->udh_app_port.src_port = 0;
			break;
		}

		switch (ie_id) {
		case SMS_UDH_IE_CONCATENATED_SHORT_MESSAGES_8BIT:
			if (ie_length != 3) {
				LOG_ERR("Concatenated short messages, 8-bit reference number: "
					"IE length 3 required, %d received",
					ie_length);
				break;
			}
			pdata->udh_concat.ref_number = buf[ofs];
			pdata->udh_concat.total_msgs = buf[ofs+1];
			pdata->udh_concat.seq_number = buf[ofs+2];
			pdata->udh_concat.present = true;

			udh_ie_concat_validity_check(pdata);
			break;

		case SMS_UDH_IE_APPLICATION_PORT_ADDRESSING_SCHEME_8BIT:
			if (ie_length != 2) {
				LOG_ERR("Application port addressing scheme, 8 bit address: "
					"IE length 2 required, %d received",
					ie_length);
				break;
			}
			pdata->udh_app_port.dest_port = buf[ofs];
			pdata->udh_app_port.src_port = buf[ofs+1];
			pdata->udh_app_port.present = true;

			LOG_DBG("UDH port scheme, destination port: %d",
				pdata->udh_app_port.dest_port);
			LOG_DBG("UDH port scheme, source port: %d",
				pdata->udh_app_port.src_port);
			break;

		case SMS_UDH_IE_APPLICATION_PORT_ADDRESSING_SCHEME_16BIT:
			if (ie_length != 4) {
				LOG_ERR("Application port addressing scheme, 16 bit address: "
					"IE length 4 required, %d received",
					ie_length);
				break;
			}
			pdata->udh_app_port.dest_port = buf[ofs]<<8;
			pdata->udh_app_port.dest_port |= buf[ofs+1];

			pdata->udh_app_port.src_port = buf[ofs+2]<<8;
			pdata->udh_app_port.src_port |= buf[ofs+3];
			pdata->udh_app_port.present = true;

			LOG_DBG("UDH port scheme, destination port: %d",
				pdata->udh_app_port.dest_port);
			LOG_DBG("UDH port scheme, source port: %d",
				pdata->udh_app_port.src_port);
			break;

		case SMS_UDH_IE_CONCATENATED_SHORT_MESSAGES_16BIT:
			if (ie_length != 4) {
				LOG_ERR("Concatenated short messages, 16-bit reference number: "
					"IE length 4 required, %d received",
					ie_length);
				break;
			}
			pdata->udh_concat.ref_number = buf[ofs]<<8;
			pdata->udh_concat.ref_number |= buf[ofs+1];
			pdata->udh_concat.total_msgs = buf[ofs+2];
			pdata->udh_concat.seq_number = buf[ofs+3];
			pdata->udh_concat.present = true;

			udh_ie_concat_validity_check(pdata);
			break;

		default:
			LOG_WRN("Ignoring not supported User Data Header information element "
				"id=0x%02X, length=%d",
				ie_id, ie_length);
			break;
		}
		ofs += ie_length;
	}
}

/**
 * @brief Decode User Data Header as specified in 3GPP TS 23.040 Section 9.2.3.24.
 *
 * @param[in,out] parser Parser instance.
 * @param[in] buf Buffer containing PDU and pointing to this field.
 *
 * @return Number of parsed bytes.
 */
static int decode_pdu_udh(struct parser *parser, uint8_t *buf)
{
	struct pdu_deliver_data * const pdata = parser->data;

	/* Check if TP-User-Data-Header-Indicator is not set */
	if (!pdata->header.udhi) {
		return 0;
	}

	pdata->udhl = buf[0];
	LOG_DBG("User Data Header Length: %d", pdata->udhl);
	pdata->udhl += 1;  /* +1 for length field itself */

	if (pdata->udhl > pdata->udl) {
		LOG_ERR("User Data Header Length %d is bigger than User-Data-Length %d",
			pdata->udhl,
			pdata->udl);
		return -EMSGSIZE;
	}
	if (pdata->udhl > parser->buf_size - parser->buf_pos) {
		LOG_ERR("User Data Header Length %d is bigger than remaining input data length %d",
			pdata->udhl,
			parser->buf_size - parser->buf_pos);
		return -EMSGSIZE;
	}

	decode_pdu_udh_ie(parser, buf);

	/* Returning zero for GSM 7bit encoding so that the start of the payload won't move
	 * further as SMS 7bit encoding is done for UDH also to get the fill bits correctly
	 * for the actual user data. For any other encoding, we'll return User Data Header Length.
	 */
	if (pdata->dcs.alphabet != 0) {
		return pdata->udhl;
	} else {
		return 0;
	}
}

/**
 * @brief Decode user data for GSM 7 bit coding scheme.
 *
 * @details This will decode the user data based on GSM 7 bit coding scheme and packing
 * specified in 3GPP TS 23.038 Section 6.1.2.1 and 6.2.1.
 * User Data Header is also taken into account as specified in 3GPP TS 23.040 Section 9.2.3.24.
 *
 * @param[in,out] parser Parser instance.
 * @param[in] buf Buffer containing PDU and pointing to this field.
 *
 * @return Number of parsed bytes.
 */
static int decode_pdu_ud_7bit(struct parser *parser, uint8_t *buf)
{
	struct pdu_deliver_data * const pdata = parser->data;
	uint16_t actual_data_length;
	uint8_t length;
	uint8_t skip_bits;
	uint8_t skip_septets;
	int length_udh_skipped;

	if (pdata->udl > SMS_MAX_PAYLOAD_LEN_CHARS) {
		LOG_ERR("User Data Length exceeds maximum number of characters (%d) in SMS spec",
			SMS_MAX_PAYLOAD_LEN_CHARS);
		return -EMSGSIZE;
	}

	/* Data length to be used is the minimum from the remaining bytes in the input buffer and
	 * length indicated by User-Data-Length. User-Data-Header-Length is taken into account
	 * later because UDH is part of GSM 7bit encoding w.r.t. fill bits for the actual data.
	 */
	actual_data_length = (parser->buf_size - parser->payload_pos) * 8 / 7;

	actual_data_length = MIN(actual_data_length, pdata->udl);

	/* Convert GSM 7bit data to ASCII characters */
	length = string_conversion_gsm7bit_to_ascii(
		buf, sms_payload_tmp, actual_data_length, true);

	/* Check whether User Data Header is present.
	 * If yes, we need to skip those septets in the sms_payload_tmp, which has all of the data
	 * decoded including User Data Header. This is done because the actual data/text is
	 * aligned into septet (7bit) boundary after User Data Header.
	 */
	skip_bits = pdata->udhl * 8;
	skip_septets = skip_bits / 7;

	if (skip_bits % 7 > 0) {
		skip_septets++;
	}

	/* Number of characters/bytes in the actual data which excludes
	 * User Data Header but minimum is 0. In some corner cases this would
	 * result in negative value causing crashes.
	 */
	length_udh_skipped = (length >= skip_septets) ? (int)(length - skip_septets) : 0;

	/* Verify that payload buffer is not too short */
	__ASSERT(length_udh_skipped <= parser->payload_buf_size,
		"GSM 7bit User-Data-Length shorter than output buffer");

	/* Copy decoded data/text into the output buffer */
	memcpy(parser->payload, sms_payload_tmp + skip_septets, length_udh_skipped);

	return length_udh_skipped;
}

/**
 * @brief Decode user data for 8 bit data coding scheme.
 *
 * @details This will essentially just copy the data from the SMS-DELIVER message into the
 * decoded payload as 8bit data means there is really no coding scheme.
 *
 * User Data Header is also taken into account as specified in 3GPP TS 23.040 Section 9.2.3.24.
 *
 * @param[in,out] parser Parser instance.
 * @param[in] buf Buffer containing PDU and pointing to this field.
 *
 * @return Number of parsed bytes.
 */
static int decode_pdu_ud_8bit(struct parser *parser, uint8_t *buf)
{
	struct pdu_deliver_data * const pdata = parser->data;
	/* Data length to be used is the minimum from the remaining bytes in the input buffer and
	 * length indicated by User-Data-Length taking into account User-Data-Header-Length.
	 */
	uint32_t actual_data_length =
		MIN(parser->buf_size - parser->payload_pos, pdata->udl - pdata->udhl);

	__ASSERT(parser->buf_size >= parser->payload_pos,
		"Data length smaller than data iterator");
	__ASSERT(actual_data_length <= parser->payload_buf_size,
		"8bit User-Data-Length shorter than output buffer");

	memcpy(parser->payload, buf, actual_data_length);

	return actual_data_length;
}

/**
 * @brief Decode user data for SMS-DELIVER message based on data coding scheme.
 *
 * @details User Data Header is also taken into account as specified in
 * 3GPP TS 23.040 Section 9.2.3.24.
 *
 * @param[in,out] parser Parser instance.
 * @param[in] buf Buffer containing PDU and pointing to this field.
 *
 * @return Non-negative number indicates number of parsed bytes. Negative value is an error code.
 */
static int decode_pdu_deliver_message(struct parser *parser, uint8_t *buf)
{
	struct pdu_deliver_data * const pdata = parser->data;

	switch (pdata->dcs.alphabet) {
	case 0:
		return decode_pdu_ud_7bit(parser, buf);
	case 1:
		return decode_pdu_ud_8bit(parser, buf);
	case 2:
		LOG_ERR("Unsupported data coding scheme: UCS2");
		return -ENOTSUP;
	default: /* case 3: is a reserved value */
		LOG_ERR("Unsupported data coding scheme: Reserved");
		return -ENOTSUP;
	};

	return 0;
}

/**
 * @brief List of parser required to parse entire SMS-DELIVER PDU.
 */
const static parser_module sms_pdu_deliver_parsers[] = {
	decode_pdu_deliver_smsc,
	decode_pdu_deliver_header,
	decode_pdu_oa_field,
	decode_pdu_pid_field,
	decode_pdu_dcs_field,
	decode_pdu_scts_field,
	decode_pdu_udl_field,
	decode_pdu_udh,
};

/**
 * @brief Return parsers.
 *
 * @return Parsers.
 */
static void *sms_deliver_get_parsers(void)
{
	return (parser_module *)sms_pdu_deliver_parsers;
}

/**
 * @brief Data decoder for the parser.
 */
static void *sms_deliver_get_decoder(void)
{
	return decode_pdu_deliver_message;
}

/**
 * @brief Return number of parsers.
 *
 * @return Number of parsers.
 */
static int sms_deliver_get_parser_count(void)
{
	return ARRAY_SIZE(sms_pdu_deliver_parsers);
}

/**
 * @brief Return deliver data structure size to store all the information.
 *
 * @return Data structure size.
 */
static uint32_t sms_deliver_get_data_size(void)
{
	return sizeof(struct pdu_deliver_data);
}

/**
 * @brief Get SMS-DELIVER header for given parser.
 *
 * @param[in] parser Parser instance.
 * @param[out] header Output structure of type: struct sms_deliver_header*
 *
 * @return Zero on success, otherwise error code.
 */
static int sms_deliver_get_header(struct parser *parser, void *header)
{
	struct sms_deliver_header *sms_header = header;
	struct pdu_deliver_data * const pdata = parser->data;

	memcpy(&sms_header->time, &pdata->timestamp, sizeof(struct sms_time));

	memcpy(&sms_header->originating_address, &pdata->oa, sizeof(struct sms_address));

	sms_header->app_port = pdata->udh_app_port;
	sms_header->concatenated = pdata->udh_concat;

	return 0;
}

/**
 * @brief Parser API functions for SMS-DELIVER PDU parsing.
 */
const static struct parser_api sms_deliver_api = {
	.data_size        = sms_deliver_get_data_size,
	.get_parsers      = sms_deliver_get_parsers,
	.get_decoder      = sms_deliver_get_decoder,
	.get_parser_count = sms_deliver_get_parser_count,
	.get_header       = sms_deliver_get_header,
};

/**
 * @brief Return SMS-DELIVER parser API.
 *
 * @return SMS-DELIVER API structure of type struct parser_api*.
 */
void *sms_deliver_get_api(void)
{
	return (struct parser_api *)&sms_deliver_api;
}

int sms_deliver_pdu_parse(const char *pdu, struct sms_data *data)
{
	static struct parser sms_deliver;
	struct sms_deliver_header *header;
	int err = 0;

	__ASSERT(pdu != NULL, "Parameter 'pdu' cannot be NULL.");
	__ASSERT(data != NULL, "Parameter 'data' cannot be NULL.");

	header = &data->header.deliver;

	__ASSERT(header != NULL, "Parameter 'header' cannot be NULL.");
	memset(header, 0, sizeof(struct sms_deliver_header));

	err = parser_create(&sms_deliver, sms_deliver_get_api());
	if (err) {
		return err;
	}

	err = parser_process_str(&sms_deliver, pdu);
	if (err) {
		LOG_ERR("Parsing error (%d) in decoding SMS-DELIVER message", err);
		return err;
	}

	parser_get_header(&sms_deliver, header);

	data->payload_len = parser_get_payload(&sms_deliver,
					       data->payload,
					       SMS_MAX_PAYLOAD_LEN_CHARS);

	if (data->payload_len < 0) {
		LOG_ERR("Decoding SMS-DELIVER payload failed: %d", data->payload_len);
		return data->payload_len;
	}

	LOG_DBG("Time:   %02d-%02d-%02d %02d:%02d:%02d",
		header->time.year,
		header->time.month,
		header->time.day,
		header->time.hour,
		header->time.minute,
		header->time.second);
	LOG_DBG("Text:   '%s'", (char *)data->payload);

	LOG_DBG("Length: %d", data->payload_len);

	parser_delete(&sms_deliver);
	return 0;
}
