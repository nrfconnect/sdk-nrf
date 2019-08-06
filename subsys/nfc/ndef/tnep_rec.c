/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include <misc/byteorder.h>
#include <nfc/ndef/tnep_rec.h>

#define NFC_NDEF_TNEP_SIZE_STATUS 1
#define NFC_NDEF_TNEP_SIZE_SELECT_NO_URI 1
#define NFC_NDEF_TNEP_SIZE_PARMAS_NO_URI 7

const u8_t nfc_ndef_tnep_rec_type_status[NFC_NDEF_TNEP_REC_TYPE_LEN] = {
		'T',
		'e' };
const u8_t nfc_ndef_tnep_rec_type_svc_select[NFC_NDEF_TNEP_REC_TYPE_LEN] = {
		'T',
		's' };
const u8_t nfc_ndef_tnep_rec_type_svc_param[NFC_NDEF_TNEP_REC_TYPE_LEN] = {
		'T',
		'p' };

/* Function for calculating TNEP status record payload size. */
static u32_t nfc_tnep_calc_size_status(
		struct nfc_ndef_tnep_status *payload_decs)
{
	return NFC_NDEF_TNEP_SIZE_STATUS;
}

/* Function for calculating TNEP service select record payload size. */
static u32_t nfc_tnep_calc_size_service_select(
		struct nfc_ndef_tnep_svc_select *payload_decs)
{
	u32_t record_length = 0;

	if (payload_decs->uri_name_len) {
		record_length = payload_decs->uri_name_len
				+ NFC_NDEF_TNEP_SIZE_SELECT_NO_URI;
	}

	return record_length;
}

/* Function for calculating TNEP service parameters record payload size. */
static u32_t nfc_tnep_calc_size_service_param(
		struct nfc_ndef_tnep_svc_param *payload_decs)
{
	u32_t record_length = 0;

	if (payload_decs->svc_name_uri_length != 0) {
		record_length = payload_decs->svc_name_uri_length
				+ NFC_NDEF_TNEP_SIZE_PARMAS_NO_URI;
	}

	return record_length;
}

int nfc_ndef_tnep_status_payload(struct nfc_ndef_tnep_status *payload_desc,
				 u8_t *buffer, u32_t *len)
{
	if ((!payload_desc) || (!len)) {

		return -EINVAL;
	}

	u32_t payload_size;

	payload_size = nfc_tnep_calc_size_status(payload_desc);

	if (buffer) {
		if (payload_size > *len) {
			return -ENOSR;
		}

		*buffer = (payload_desc->status_type);

	}

	*len = payload_size;

	return 0;
}

int nfc_ndef_tnep_svc_select_payload(
		struct nfc_ndef_tnep_svc_select *payload_desc, u8_t *buffer,
		u32_t *len)
{
	if ((!len)) {

		return -EINVAL;
	}

	u32_t payload_size;

	payload_size = nfc_tnep_calc_size_service_select(payload_desc);

	if (buffer) {
		if (payload_size > *len) {
			return -ENOSR;
		}

		*buffer = (payload_desc->uri_name_len);
		buffer++;

		memcpy(buffer, payload_desc->uri_name,
		       payload_desc->uri_name_len);
		buffer += payload_desc->uri_name_len;

	}

	*len = payload_size;

	return 0;
}

int nfc_ndef_tnep_svc_param_payload(
		struct nfc_ndef_tnep_svc_param *payload_desc, u8_t *buffer,
		u32_t *len)
{
	if ((!payload_desc->tnep_version)
			|| (!payload_desc->svc_name_uri_length)
			|| (!payload_desc->svc_name_uri) || (!len)) {

		return -EINVAL;
	}

	u32_t payload_size = nfc_tnep_calc_size_service_param(payload_desc);

	if (buffer) {
		if (payload_size > *len) {
			return -ENOSR;
		}

		*buffer = (payload_desc->tnep_version);
		buffer++;

		*buffer = (payload_desc->svc_name_uri_length);
		buffer++;

		memcpy(buffer, payload_desc->svc_name_uri,
		       payload_desc->svc_name_uri_length);
		buffer += payload_desc->svc_name_uri_length;

		*buffer = (payload_desc->communication_mode);
		buffer++;

		*buffer = (payload_desc->min_waiting_time);
		buffer++;

		*buffer = (payload_desc->max_waiting_time_ext);
		buffer++;

		u8_t max_msg_size_be[2];

		sys_put_be16(payload_desc->max_message_size, max_msg_size_be);

		memcpy(buffer, max_msg_size_be, sizeof(max_msg_size_be));
	}

	*len = payload_size;

	return 0;
}
