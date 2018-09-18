/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <string.h>
#include <nfc/ndef/nfc_ndef_record.h>
#include <misc/byteorder.h>

/* Sum of sizes of fields: TNF-flags, Type Length, Payload Length in long
 * NDEF record.
 */
#define NDEF_RECORD_BASE_LONG_SIZE (2 + NDEF_RECORD_PAYLOAD_LEN_LONG_SIZE)

static u32_t record_header_size_calc(
			struct nfc_ndef_record_desc const *ndef_record_desc)
{
	u32_t len;

	len = NDEF_RECORD_BASE_LONG_SIZE + ndef_record_desc->id_length +
			ndef_record_desc->type_length;

	if (ndef_record_desc->id_length > 0) {
		len++;
	}

	return len;
}

int nfc_ndef_record_encode(struct nfc_ndef_record_desc const *ndef_record_desc,
			   enum nfc_ndef_record_location const record_location,
			   u8_t *record_buffer,
			   u32_t *record_len)
{
	u8_t *payload_len = NULL; /* use as pointer to payload length field */
	u32_t record_payload_len;

	if (!ndef_record_desc) {
		return -EINVAL;
	}
	/* count record length without payload */
	u32_t record_header_len = record_header_size_calc(ndef_record_desc);

	if (record_buffer) {
		u8_t *flags; /* use as pointer to TNF + flags field */

		/* verify location range */
		if (record_location & (~NDEF_RECORD_LOCATION_MASK)) {
			return -EINVAL;
		}
		/* verify if there is enough available memory */
		if (record_header_len > *record_len) {
			return -ENOSR;
		}

		flags = record_buffer;
		record_buffer++;

		/* set location bits and clear other bits in 1st byte. */
		*flags = record_location;
		*flags |= ndef_record_desc->tnf;

		/* TYPE LENGTH */
		*record_buffer = ndef_record_desc->type_length;
		record_buffer++;
		/* use always long record and remember payload len field memory
		 * offset.
		 */
		payload_len = record_buffer;
		record_buffer += NDEF_RECORD_PAYLOAD_LEN_LONG_SIZE;
		/* ID LENGTH - option */
		if (ndef_record_desc->id_length > 0) {
			*record_buffer = ndef_record_desc->id_length;
			record_buffer++;
			/* IL flag */
			*flags |= NDEF_RECORD_IL_MASK;
		}
		/* TYPE */
		memcpy(record_buffer,
		       ndef_record_desc->type,
		       ndef_record_desc->type_length);
		record_buffer += ndef_record_desc->type_length;
		/* ID */
		if (ndef_record_desc->id_length > 0) {
			memcpy(record_buffer,
			       ndef_record_desc->id,
			       ndef_record_desc->id_length);
			record_buffer += ndef_record_desc->id_length;
		}
		/* count how much memory is left in record buffer for payload
		 * field.
		 */
		record_payload_len = (*record_len - record_header_len);
	}
	/* PAYLOAD */
	if (ndef_record_desc->payload_constructor) {
		int err;

		err = ndef_record_desc->payload_constructor(
					ndef_record_desc->payload_descriptor,
					record_buffer,
					&record_payload_len);

		if (err) {
			return err;
		}
	} else {
		return -EINVAL;
	}

	if (record_buffer) {
		/* PAYLOAD LENGTH */
		*(u32_t *)payload_len = sys_cpu_to_be32(record_payload_len);
	}

	*record_len = record_header_len + record_payload_len;

	return 0;
}

int nfc_ndef_bin_payload_memcopy(
			struct nfc_ndef_bin_payload_desc *payload_descriptor,
			u8_t *buffer,
			u32_t *len)
{
	if (buffer) {
		if (*len < payload_descriptor->payload_length) {
			return -ENOSR;
	}

	memcpy(buffer,
	       payload_descriptor->payload,
	       payload_descriptor->payload_length);
	}

	*len = payload_descriptor->payload_length;

	return 0;
}
