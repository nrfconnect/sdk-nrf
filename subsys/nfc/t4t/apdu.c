/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <errno.h>
#include <zephyr/sys/byteorder.h>
#include <nfc/t4t/apdu.h>

LOG_MODULE_REGISTER(nfc_t4t_apdu, CONFIG_NFC_T4T_APDU_LOG_LEVEL);

/** @brief Field sizes that can be present in C-APDU.
 */
#define CLASS_TYPE_SIZE 1U
#define INSTRUCTION_TYPE_SIZE 1U
#define PARAMETER_SIZE 2U
#define LC_SHORT_FORMAT_SIZE 1U
#define LC_LONG_FORMAT_SIZE 3U
#define LE_SHORT_FORMAT_SIZE 1U
#define LE_LONG_FORMAT_SIZE 2U

/** @brief Values used to encode Lc field in C-APDU.
 */
#define LC_LONG_FORMAT_TOKEN 0x00
#define LC_LONG_FORMAT_THR 0xFF

/** @brief Values used to encode Le field in C-APDU.
 */
#define LE_FIELD_ABSENT 0U
#define LE_LONG_FORMAT_THR 0x0100
#define LE_ENCODED_VAL_256 0x00

/* Size of Status field contained in R-APDU. */
#define STATUS_SIZE 2U

static uint16_t nfc_t4t_apdu_comm_size_calc(const struct nfc_t4t_apdu_comm *cmd_apdu)
{
	uint16_t res = CLASS_TYPE_SIZE + INSTRUCTION_TYPE_SIZE + PARAMETER_SIZE;

	if (cmd_apdu->data.buff) {
		if (cmd_apdu->data.len > LC_LONG_FORMAT_THR) {
			res += LC_LONG_FORMAT_SIZE;
		} else {
			res += LC_SHORT_FORMAT_SIZE;
		}
	}

	res += cmd_apdu->data.len;

	if (cmd_apdu->resp_len != LE_FIELD_ABSENT) {
		if (cmd_apdu->resp_len > LE_LONG_FORMAT_THR) {
			res += LE_LONG_FORMAT_SIZE;
		} else {
			res += LE_SHORT_FORMAT_SIZE;
		}
	}

	return res;
}

static int nfc_t4t_apdu_comm_args_validate(const struct nfc_t4t_apdu_comm *cmd_apdu,
					   uint8_t *raw_data, uint16_t *len)
{
	if ((!cmd_apdu) || (!raw_data) || (!len)) {
		return -EINVAL;
	}

	if ((cmd_apdu->data.buff) && (cmd_apdu->data.len == 0)) {
		return -EFAULT;
	}

	return 0;
}

int nfc_t4t_apdu_comm_encode(const struct nfc_t4t_apdu_comm *cmd_apdu,
			     uint8_t *raw_data, uint16_t *len)
{
	int err;

	/*  Validate passed arguments. */
	err = nfc_t4t_apdu_comm_args_validate(cmd_apdu, raw_data, len);
	if (err) {
		return err;
	}

	/* Check if there is enough memory in the provided buffer to store
	 * described C-APDU.
	 */
	uint16_t comm_apdu_len = nfc_t4t_apdu_comm_size_calc(cmd_apdu);

	if (comm_apdu_len > *len) {
		return -ENOMEM;
	}

	*len = comm_apdu_len;

	/* Start to encode described C-APDU in the buffer. */
	*raw_data++ = cmd_apdu->class_byte;
	*raw_data++ = cmd_apdu->instruction;

	sys_put_be16(cmd_apdu->parameter, raw_data);
	raw_data += sizeof(uint16_t);

	/* Check if optional data field should be included. */
	if (cmd_apdu->data.buff) {
		/* Use long data length encoding. */
		if (cmd_apdu->data.len > LC_LONG_FORMAT_THR) {
			*raw_data++ = LC_LONG_FORMAT_TOKEN;

			sys_put_be16(cmd_apdu->data.len, raw_data);
			raw_data += sizeof(uint16_t);
		} else {
			/* Use short data length encoding. */
			*raw_data++ = cmd_apdu->data.len;
		}

		memcpy(raw_data, cmd_apdu->data.buff, cmd_apdu->data.len);
		raw_data += cmd_apdu->data.len;
	}

	/* Check if optional response length field present (Le) should be
	 * included.
	 */
	if (cmd_apdu->resp_len != LE_FIELD_ABSENT) {
		/* Use long response length encoding. */
		if (cmd_apdu->resp_len > LE_LONG_FORMAT_THR) {
			sys_put_be16(cmd_apdu->resp_len, raw_data);
			raw_data += sizeof(uint16_t);
		} else {
			/* Use short response length encoding. */
			if (cmd_apdu->resp_len == LE_LONG_FORMAT_THR) {
				*raw_data++ = LE_ENCODED_VAL_256;
			} else {
				*raw_data++ = cmd_apdu->resp_len;
			}
		}
	}

	return 0;
}

static int nfc_t4t_apdu_resp_args_validate(const struct nfc_t4t_apdu_resp *resp_apdu,
					   const uint8_t *raw_data,
					   uint16_t len)
{
	if ((!resp_apdu) || (!raw_data)) {
		return -EINVAL;
	}

	if (len < STATUS_SIZE) {
		return -EPERM;
	}

	return 0;
}

int nfc_t4t_apdu_resp_decode(struct nfc_t4t_apdu_resp *resp_apdu,
			     const uint8_t *raw_data,
			     uint16_t len)
{
	int err;

	/* Validate passed arguments. */
	err = nfc_t4t_apdu_resp_args_validate(resp_apdu, raw_data, len);
	if (err) {
		return err;
	}

	nfc_t4t_apdu_resp_clear(resp_apdu);

	/*  Optional data field is present in R-APDU. */
	if (len > STATUS_SIZE) {
		resp_apdu->data.len = len - STATUS_SIZE;
		resp_apdu->data.buff = (uint8_t *)raw_data;
	}

	resp_apdu->status = sys_get_be16(raw_data + resp_apdu->data.len);

	return 0;
}

void nfc_t4t_apdu_resp_printout(const struct nfc_t4t_apdu_resp *resp_apdu)
{
	LOG_INF("R-APDU status: %4X ", resp_apdu->status);

	if (resp_apdu->data.buff) {
		LOG_INF("R-APDU data: ");
		LOG_HEXDUMP_INF(resp_apdu->data.buff, resp_apdu->data.len,
				"R_APDU data: ");
	} else {
		LOG_INF("R-APDU no data field present.");
	}
}
