/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <nfc/ndef/tnep_rec.h>

#define NFC_NDEF_TNEP_STATUS_SIZE 1
#define NFC_NDEF_TNEP_SELECT_NO_URI_SIZE 1
#define NFC_NDEF_TNEP_PARAMS_NO_URI_SIZE 7

const uint8_t nfc_ndef_tnep_rec_type_status[] = {'T', 'e' };
const uint8_t nfc_ndef_tnep_rec_type_svc_select[] = {'T', 's'};
const uint8_t nfc_ndef_tnep_rec_type_svc_param[] = {'T', 'p'};

/* Function for calculating TNEP service select record payload size. */
static uint32_t nfc_tnep_calc_size_service_select(struct nfc_ndef_tnep_rec_svc_select *payload_decs)
{
	uint32_t record_length = 0;

	if (payload_decs->uri_len) {
		record_length = payload_decs->uri_len +
			NFC_NDEF_TNEP_SELECT_NO_URI_SIZE;
	}

	return record_length;
}

/* Function for calculating TNEP service parameters record payload size. */
static uint32_t nfc_tnep_calc_size_service_param(struct nfc_ndef_tnep_rec_svc_param *payload_decs)
{
	uint32_t record_length = 0;

	if (payload_decs->uri_length != 0) {
		record_length = payload_decs->uri_length +
			NFC_NDEF_TNEP_PARAMS_NO_URI_SIZE;
	}

	return record_length;
}

int nfc_ndef_tnep_rec_status_payload(struct nfc_ndef_tnep_rec_status *payload_desc,
				     uint8_t *buffer, uint32_t *len)
{
	uint32_t payload_size;

	if (!payload_desc || !len) {
		return -EINVAL;
	}

	payload_size = NFC_NDEF_TNEP_STATUS_SIZE;

	if (buffer) {
		if (payload_size > *len) {
			return -ENOSR;
		}

		*buffer = (payload_desc->status);
	}

	*len = payload_size;

	return 0;
}

int nfc_ndef_tnep_rec_svc_select_payload(struct nfc_ndef_tnep_rec_svc_select *payload_desc,
					 uint8_t *buffer, uint32_t *len)
{
	if (!payload_desc || !len) {

		return -EINVAL;
	}

	uint32_t payload_size;

	payload_size = nfc_tnep_calc_size_service_select(payload_desc);

	if (buffer) {
		if (payload_size > *len) {
			return -ENOSR;
		}

		*buffer = (payload_desc->uri_len);
		buffer++;

		memcpy(buffer, payload_desc->uri,
		       payload_desc->uri_len);
		buffer += payload_desc->uri_len;
	}

	*len = payload_size;

	return 0;
}

int nfc_ndef_tnep_rec_svc_param_payload(struct nfc_ndef_tnep_rec_svc_param *payload_desc,
					uint8_t *buffer, uint32_t *len)
{
	if (!payload_desc->version ||
	    !payload_desc->uri_length ||
	    !payload_desc->uri ||
	    !len) {
		return -EINVAL;
	}

	uint32_t payload_size = nfc_tnep_calc_size_service_param(payload_desc);

	if (buffer) {
		if (payload_size > *len) {
			return -ENOSR;
		}

		*buffer = payload_desc->version;
		buffer++;

		*buffer = payload_desc->uri_length;
		buffer++;

		memcpy(buffer, payload_desc->uri,
		       payload_desc->uri_length);
		buffer += payload_desc->uri_length;

		*buffer = (payload_desc->communication_mode);
		buffer++;

		*buffer = (payload_desc->min_time);
		buffer++;

		*buffer = (payload_desc->max_time_ext);
		buffer++;

		sys_put_be16(payload_desc->max_size, buffer);
	}

	*len = payload_size;

	return 0;
}
