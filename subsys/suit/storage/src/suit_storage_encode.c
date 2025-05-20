/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_internal.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <zephyr/logging/log.h>
#include <suit_plat_decode_util.h>

#define SUIT_STORAGE_ENVELOPE_VERSION		  1
#define SUIT_STORAGE_ENVELOPE_TAG_VERSION	  0
#define SUIT_STORAGE_ENVELOPE_TAG_CLASS_ID_OFFSET 1
#define SUIT_STORAGE_ENVELOPE_TAG_ENVELOPE_BSTR	  2

LOG_MODULE_REGISTER(suit_storage_env, CONFIG_SUIT_LOG_LEVEL);

static suit_plat_err_t encode_bstr_header(size_t bstr_len, uint8_t *buf, size_t *len)
{
	size_t bytes_used = 0;

	if ((buf == NULL) || (len == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if ((bstr_len > 0x0000FFFF) || (*len < 3)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	/* Encode bstr header */
	if (bstr_len < 24) {
		buf[bytes_used++] = 0x40 + bstr_len;
	} else if (bstr_len < 0x100) {
		buf[bytes_used++] = 0x58;
		buf[bytes_used++] = bstr_len;
	} else {
		buf[bytes_used++] = 0x59;
		buf[bytes_used++] = bstr_len / 0x100;
		buf[bytes_used++] = bstr_len % 0x100;
	}

	*len = bytes_used;

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_bstr_kv_header_len(uint32_t key, size_t bstr_len, size_t *p_len)
{
	if ((p_len == NULL) || (key > 0x0000FFFF) || (bstr_len > 0x0000FFFF)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	*p_len = 0;

	/* Find key length */
	if (key < 24) {
		*p_len += 1;
	} else if (key < 0x100) {
		*p_len += 2;
	} else {
		*p_len += 3;
	}

	/* Find bstr header length */
	if (bstr_len < 24) {
		*p_len += 1;
	} else if (bstr_len < 0x100) {
		*p_len += 2;
	} else {
		*p_len += 3;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_encode_bstr_kv_header(uint32_t key, size_t bstr_len, uint8_t *buf,
						   size_t *len)
{
	size_t bytes_used = 0;

	if ((buf == NULL) || (len == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if ((key > 0x0000FFFF) || (bstr_len > 0x0000FFFF) || (*len < 6)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	/* Encode key */
	if (key < 24) {
		buf[bytes_used++] = key;
	} else if (key < 0x100) {
		buf[bytes_used++] = 0x18;
		buf[bytes_used++] = key;
	} else {
		buf[bytes_used++] = 0x19;
		buf[bytes_used++] = key / 0x100;
		buf[bytes_used++] = key % 0x100;
	}

	/* Encode bstr header */
	size_t bstr_hdr_len = *len - bytes_used;
	suit_plat_err_t err = encode_bstr_header(bstr_len, &buf[bytes_used], &bstr_hdr_len);

	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	bytes_used += bstr_hdr_len;
	*len = bytes_used;

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_decode_envelope_header(const uint8_t *buf, size_t len,
						    suit_envelope_hdr_t *envelope)
{
	bool ok = false;
	zcbor_state_t states[3];
	struct zcbor_string envelope_bstr;
	uint32_t class_id_offset;

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), buf, len, 1, NULL, 0);

	ok = (zcbor_map_start_decode(states) &&
	      zcbor_uint32_expect(states, SUIT_STORAGE_ENVELOPE_TAG_VERSION) &&
	      zcbor_uint32_expect(states, SUIT_STORAGE_ENVELOPE_VERSION) &&
	      zcbor_uint32_expect(states, SUIT_STORAGE_ENVELOPE_TAG_CLASS_ID_OFFSET) &&
	      zcbor_uint32_decode(states, &class_id_offset) &&
	      zcbor_uint32_expect(states, SUIT_STORAGE_ENVELOPE_TAG_ENVELOPE_BSTR) &&
	      zcbor_bstr_decode(states, &envelope_bstr) && zcbor_list_map_end_force_decode(states));

	if (!ok) {
		int err = zcbor_pop_error(states);

		/* Use this error code if additional logging is needed:
		 * LOG_INF("Decoding of envelope header failed:, ZCBOR error %d", err);
		 * This isn't done normally as it would print lots of logs as this
		 * function is called in a loop and most envelope slots are invalid.
		 */
		(void)err;

		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	/* Validate the class ID offset value */
	if ((class_id_offset < 1) || (envelope_bstr.len < class_id_offset)) {
		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	/* Extract pointers to the SUIT envelope */
	envelope->envelope.mem = envelope_bstr.value;
	envelope->envelope.size = envelope_bstr.len;
	envelope->class_id_offset = class_id_offset;

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_encode_envelope_header(suit_envelope_hdr_t *envelope, uint8_t *buf,
						    size_t *len)
{
	const uint8_t envelope_tag[SUIT_STORAGE_ENCODED_ENVELOPE_TAG_LEN] = {
		0xD8, 0x6B, /* SUIT envelope tag (107) */
		0xA2,	    /* map with 2 elements */
	};
	bool ok = false;
	zcbor_state_t states[3];

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), buf, *len, 1, NULL, 0);

	ok = (zcbor_map_start_encode(states, 3) &&
	      zcbor_uint32_put(states, SUIT_STORAGE_ENVELOPE_TAG_VERSION) &&
	      zcbor_uint32_put(states, SUIT_STORAGE_ENVELOPE_VERSION) &&
	      zcbor_uint32_put(states, SUIT_STORAGE_ENVELOPE_TAG_CLASS_ID_OFFSET) &&
	      zcbor_uint32_put(states, envelope->class_id_offset) &&
	      zcbor_uint32_put(states, SUIT_STORAGE_ENVELOPE_TAG_ENVELOPE_BSTR));

	if (ok) {
		/* Get the number of bytes filled inside the output buffer. */
		size_t mem_used = (size_t)states[0].payload - (size_t)buf;
		size_t bstr_hdr_len = (*len - (mem_used + sizeof(envelope_tag)));

		if ((*len < (mem_used + 3 + sizeof(envelope_tag))) ||
		    (envelope->envelope.size > 0x0000FFFF)) {
			LOG_ERR("Unable to fit SUIT envelope header using %d bytes. Needs %d bytes",
				*len, mem_used + 3 + sizeof(envelope_tag));
			return SUIT_PLAT_ERR_NOMEM;
		}

		/* Encode bstr header and length */
		suit_plat_err_t err =
			encode_bstr_header(envelope->envelope.size, &buf[mem_used], &bstr_hdr_len);

		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Encoding of envelope bstr header failed: %d", err);
			return err;
		}
		mem_used += bstr_hdr_len;

		/* Encode SUIT envelope tag and map header. */
		memcpy(&buf[mem_used], envelope_tag, sizeof(envelope_tag));
		mem_used += sizeof(envelope_tag);

		*len = mem_used;
	}

	if (!ok) {
		int err = zcbor_pop_error(states);

		LOG_ERR("Encoding of envelope header failed, ZCBOR error %d", err);

		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_decode_suit_envelope_severed(const uint8_t *buf, size_t len,
							  struct SUIT_Envelope_severed *envelope,
							  size_t *envelope_len)
{
	bool ok = false;
	zcbor_state_t states[3];

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), buf, len, 1, NULL, 0);

	if (zcbor_tag_expect(states, 107) && zcbor_map_start_decode(states) == true) {
		/* Parse authentication wrapper and manifest. */
		ok = (zcbor_uint32_expect(states, SUIT_AUTHENTICATION_WRAPPER_TAG) &&
		      zcbor_bstr_decode(states, &(envelope->suit_authentication_wrapper)) &&
		      zcbor_uint32_expect(states, SUIT_MANIFEST_TAG) &&
		      zcbor_bstr_decode(states, &(envelope->suit_manifest)));

		/* Ignore remaining items, consume backups and finish decoding.*/
		if (zcbor_list_map_end_force_decode(states) == false) {
			ok = false;
		};
	}

	if (ok && (envelope_len != NULL)) {
		*envelope_len = MIN(len, (size_t)states[0].payload - (size_t)buf);
	}

	/* Decode the manifest class id value */
	envelope->suit_manifest_component_id.value = NULL;
	envelope->suit_manifest_component_id.len = 0;

	if (ok) {
		zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t),
				envelope->suit_manifest.value, envelope->suit_manifest.len, 1, NULL,
				0);
		ok = zcbor_map_start_decode(states);

		while (ok) {
			uint32_t manifest_map_tag;

			ok = zcbor_uint32_decode(states, &manifest_map_tag);
			if (!ok) {
				break;
			}

			if (manifest_map_tag == SUIT_MANIFEST_COMPONENT_ID_TAG) {
				const uint8_t *component_id_start = states[0].payload;

				ok = zcbor_any_skip(states, NULL);

				const uint8_t *component_id_end = states[0].payload;

				if (ok) {
					envelope->suit_manifest_component_id.value =
						component_id_start;
					envelope->suit_manifest_component_id.len =
						component_id_end - component_id_start;
					break;
				}
			} else {
				ok = zcbor_any_skip(states, NULL);
			}
		}

		if (zcbor_list_map_end_force_decode(states) == false) {
			ok = false;
		};
	}

	if (!ok) {
		int err = zcbor_pop_error(states);

		LOG_ERR("Decoding of envelope severed elements failed:, ZCBOR error %d", err);

		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	return SUIT_PLAT_SUCCESS;
}
