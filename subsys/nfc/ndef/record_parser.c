/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <logging/log.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <nfc/ndef/record_parser.h>

LOG_MODULE_DECLARE(nfc_ndef_parser);

/* Sum of sizes of fields: TNF-flags, Type Length,
 * Payload Length in short NDEF record.
 */
#define NDEF_RECORD_BASE_SHORT_LEN (2 + NDEF_RECORD_PAYLOAD_LEN_SHORT_SIZE)


int nfc_ndef_record_parse(struct nfc_ndef_bin_payload_desc *bin_pay_desc,
			  struct nfc_ndef_record_desc *rec_desc,
			  enum nfc_ndef_record_location *record_location,
			  const u8_t *nfc_data,
			  u32_t *nfc_data_len)
{
	u32_t expected_rec_size = NDEF_RECORD_BASE_SHORT_LEN;

	if (expected_rec_size > *nfc_data_len) {
		return -EINVAL;
	}

	rec_desc->tnf = (enum nfc_ndef_record_tnf) ((*nfc_data) & NDEF_RECORD_TNF_MASK);

	/* An NDEF parser that receives an NDEF record with an unknown
	 * or unsupported TNF field value
	 * SHOULD treat it as Unknown. See NFCForum-TS-NDEF_1.0
	 */
	if (rec_desc->tnf == TNF_RESERVED) {
		rec_desc->tnf = TNF_UNKNOWN_TYPE;
	}

	*record_location = (enum nfc_ndef_record_location) ((*nfc_data) & NDEF_RECORD_LOCATION_MASK);

	u8_t flags = *(nfc_data++);

	rec_desc->type_length = *(nfc_data++);

	u32_t payload_length;

	if (flags & NDEF_RECORD_SR_MASK) {
		payload_length = *(nfc_data++);
	} else {
		expected_rec_size +=
			NDEF_RECORD_PAYLOAD_LEN_LONG_SIZE - NDEF_RECORD_PAYLOAD_LEN_SHORT_SIZE;

		if (expected_rec_size > *nfc_data_len) {
			return -EINVAL;
		}

		payload_length = sys_get_be32(nfc_data);
		nfc_data += NDEF_RECORD_PAYLOAD_LEN_LONG_SIZE;
	}

	if (flags & NDEF_RECORD_IL_MASK) {
		expected_rec_size += NDEF_RECORD_ID_LEN_SIZE;

		if (expected_rec_size > *nfc_data_len) {
			return -EINVAL;
		}

		rec_desc->id_length = *(nfc_data++);
	} else {
		rec_desc->id_length = 0;
		rec_desc->id        = NULL;
	}

	expected_rec_size += rec_desc->type_length + rec_desc->id_length + payload_length;

	if (expected_rec_size > *nfc_data_len) {
		return -EINVAL;
	}

	if (rec_desc->type_length > 0) {
		rec_desc->type = nfc_data;
		nfc_data += rec_desc->type_length;
	} else {
		rec_desc->type = NULL;
	}

	if (rec_desc->id_length > 0) {
		rec_desc->id = nfc_data;
		nfc_data += rec_desc->id_length;
	}

	if (payload_length == 0) {
		bin_pay_desc->payload = NULL;
	} else {
		bin_pay_desc->payload = nfc_data;
	}

	bin_pay_desc->payload_length = payload_length;

	rec_desc->payload_descriptor = bin_pay_desc;
	rec_desc->payload_constructor  = (payload_constructor_t) nfc_ndef_bin_payload_memcopy;

	*nfc_data_len = expected_rec_size;

	return 0;
}

static const char * const tnf_strings[] = {
	"Empty",
	"NFC Forum well-known type",
	"Media-type (RFC 2046)",
	"Absolute URI (RFC 3986)",
	"NFC Forum external type (NFC RTD)",
	"Unknown",
	"Unchanged",
	"Reserved"
};

void nfc_ndef_record_printout(u32_t num,
			      const struct nfc_ndef_record_desc *rec_desc)
{
	LOG_INF("NDEF record %d content:", num);
	LOG_INF("TNF: %s", tnf_strings[rec_desc->tnf]);

	if (rec_desc->id != NULL) {
		LOG_HEXDUMP_INF((u8_t *)rec_desc->id,
				rec_desc->id_length,
				"Record ID: ");
	}

	if (rec_desc->type != NULL) {
		LOG_HEXDUMP_INF((u8_t *)rec_desc->type,
				rec_desc->type_length,
				"Record type: ");
	}

	if (rec_desc->payload_constructor == (payload_constructor_t) nfc_ndef_bin_payload_memcopy) {
		const struct nfc_ndef_bin_payload_desc *bin_pay_desc =
			rec_desc->payload_descriptor;

		if (bin_pay_desc->payload != NULL) {
			LOG_INF("Payload length: %d bytes",
				bin_pay_desc->payload_length);
			LOG_HEXDUMP_DBG((u8_t *)bin_pay_desc->payload,
					bin_pay_desc->payload_length,
					"Payload length: ");
		} else {
			LOG_INF("No payload");
		}
	}
}

