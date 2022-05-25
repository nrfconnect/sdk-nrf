/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dfu_req, CONFIG_SLM_LOG_LEVEL);

#include "dfu_host.h"
#include "slip.h"

#define REQ_DATA_SIZE_MAX		SLIP_SIZE_MAX
#define RSP_DATA_SIZE_MAX		SLIP_SIZE_MAX

/**
 * @brief DFU protocol operation.
 */
enum nrf_dfu_op {
	NRF_DFU_OP_PROTOCOL_VERSION  = 0x00,     /* Retrieve protocol version */
	NRF_DFU_OP_OBJECT_CREATE     = 0x01,     /* Create selected object */
	NRF_DFU_OP_RECEIPT_NOTIF_SET = 0x02,     /* Set receipt notification */
	NRF_DFU_OP_CRC_GET           = 0x03,     /* Request CRC of selected object */
	NRF_DFU_OP_OBJECT_EXECUTE    = 0x04,     /* Execute selected object */
	NRF_DFU_OP_OBJECT_SELECT     = 0x06,     /* Select object */
	NRF_DFU_OP_MTU_GET           = 0x07,     /* Retrieve MTU size */
	NRF_DFU_OP_OBJECT_WRITE      = 0x08,     /* Write selected object */
	NRF_DFU_OP_PING              = 0x09,     /* Ping */
	NRF_DFU_OP_HARDWARE_VERSION  = 0x0A,     /* Retrieve hardware version */
	NRF_DFU_OP_FIRMWARE_VERSION  = 0x0B,     /* Retrieve firmware version */
	NRF_DFU_OP_ABORT             = 0x0C,     /* Abort the DFU procedure */
	NRF_DFU_OP_RESPONSE          = 0x60,     /* Response */
	NRF_DFU_OP_INVALID           = 0xFF
};

/**
 * @brief DFU operation result code.
 */
enum nrf_dfu_result {
	/* Invalid opcode */
	NRF_DFU_RES_CODE_INVALID                 = 0x00,
	/* Operation successful */
	NRF_DFU_RES_CODE_SUCCESS                 = 0x01,
	/* Opcode not supported */
	NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED   = 0x02,
	/* Missing/invalid parameter value */
	NRF_DFU_RES_CODE_INVALID_PARAMETER       = 0x03,
	/* Not enough memory for the data object */
	NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES  = 0x04,
	/* Data object does not match the firmware and hardware requirements,
	 * the signature is wrong, or parsing the command failed
	 */
	NRF_DFU_RES_CODE_INVALID_OBJECT          = 0x05,
	/* Not a valid object type for a Create request */
	NRF_DFU_RES_CODE_UNSUPPORTED_TYPE        = 0x07,
	/* The state of the DFU process does not allow this operation */
	NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED = 0x08,
	/* Hash type specified by the init packet not supported by bootloader */
	NRF_DFU_EXT_ERROR_WRONG_HASH_TYPE        = 0x09,
	/* Operation failed */
	NRF_DFU_RES_CODE_OPERATION_FAILED        = 0x0A,
	/* Extended error. The next byte of the response contains the error
	 * code of the extended error (see @ref nrf_dfu_ext_error_code_t
	 */
	NRF_DFU_RES_CODE_EXT_ERROR               = 0x0B,
	/* Hash of received firmware image not match the hash in the init packet */
	NRF_DFU_EXT_ERROR_VERIFICATION_FAILED    = 0x0C,
	/* Available space on the device is insufficient to hold the firmware */
	NRF_DFU_EXT_ERROR_INSUFFICIENT_SPACE     = 0x0D
};

/**
 * @brief @ref NRF_DFU_OP_OBJECT_SELECT response details.
 */
struct nrf_dfu_response_select {
	uint32_t offset;                    /* Current offset */
	uint32_t crc;                       /* Current CRC */
	uint32_t max_size;                  /* Maximum size of selected object */
};

/**
 * @brief @ref NRF_DFU_OP_CRC_GET response details.
 */
struct nrf_dfu_response_crc {
	uint32_t offset;                    /* Current offset */
	uint32_t crc;                       /* Current CRC */
};

static uint8_t  ping_id;
static uint16_t prn;
static uint16_t mtu;

static uint8_t send_data[REQ_DATA_SIZE_MAX];
static uint8_t receive_data[RSP_DATA_SIZE_MAX];

/* global functions defined in different resources */
int dfu_drv_tx(const uint8_t *data, uint16_t length);
int dfu_drv_rx(uint8_t *data, uint32_t max_len, uint32_t *real_len);

static int send_raw(const uint8_t *data, uint32_t size)
{
	return dfu_drv_tx(data, size);
}

static int get_rsp(enum nrf_dfu_op oper, uint32_t *data_cnt)
{
	int rc;

	LOG_DBG("%s", __func__);

	rc = dfu_drv_rx(receive_data, sizeof(receive_data), data_cnt);
	if (!rc) {
		if (*data_cnt >= 3 &&
		    receive_data[0] == NRF_DFU_OP_RESPONSE &&
		    receive_data[1] == oper) {
			if (receive_data[2] != NRF_DFU_RES_CODE_SUCCESS) {
				uint16_t rsp_error = receive_data[2];

				/* get 2-byte error code, if applicable */
				if (*data_cnt >= 4) {
					rsp_error = (rsp_error << 8) + receive_data[3];
				}
				LOG_ERR("Bad result code (0x%X)!", rsp_error);
				rc = 1;
			}
		} else {
			LOG_ERR("Invalid response!");
			rc = 1;
		}
	}

	return rc;
}

static int req_ping(uint8_t id)
{
	int rc;
	uint8_t send_data[2] = { NRF_DFU_OP_PING };

	LOG_DBG("%s", __func__);

	send_data[1] = id;
	rc = send_raw(send_data, sizeof(send_data));

	if (!rc) {
		uint32_t data_cnt;

		rc = get_rsp(NRF_DFU_OP_PING, &data_cnt);
		if (!rc) {
			if (data_cnt != 4 || receive_data[3] != id) {
				LOG_ERR("Bad ping id!");
				rc = 1;
			}
		}
	}

	return rc;
}

static int req_set_prn(uint16_t prn)
{
	int rc;
	uint8_t send_data[3] = { NRF_DFU_OP_RECEIPT_NOTIF_SET };

	LOG_DBG("%s", __func__);
	LOG_INF("Set Packet Receipt Notification %u", prn);

	sys_put_le16(prn, send_data + 1);
	rc = send_raw(send_data, sizeof(send_data));

	if (!rc) {
		uint32_t data_cnt;

		rc = get_rsp(NRF_DFU_OP_RECEIPT_NOTIF_SET, &data_cnt);
	}

	return rc;
}

static int req_get_mtu(uint16_t *mtu)
{

	int rc;
	uint8_t send_data[1] = { NRF_DFU_OP_MTU_GET };

	LOG_DBG("%s", __func__);

	rc = send_raw(send_data, sizeof(send_data));
	if (!rc) {
		uint32_t data_cnt;

		rc = get_rsp(NRF_DFU_OP_MTU_GET, &data_cnt);
		if (!rc) {
			if (data_cnt == 5) {
				uint16_t mtu_value = sys_get_le16(receive_data + 3);

				*mtu = mtu_value;
				LOG_INF("MTU: %d", mtu_value);
			} else {
				LOG_ERR("Invalid MTU!");
				rc = 1;
			}
		}
	}

	return rc;
}

static int req_obj_select(uint8_t obj_type, struct nrf_dfu_response_select *select_rsp)
{
	int rc;
	uint8_t send_data[2] = { NRF_DFU_OP_OBJECT_SELECT };

	LOG_DBG("%s", __func__);
	LOG_DBG("Selecting Object: type:%u", obj_type);

	send_data[1] = obj_type;
	rc = send_raw(send_data, sizeof(send_data));

	if (!rc) {
		uint32_t data_cnt;

		rc = get_rsp(NRF_DFU_OP_OBJECT_SELECT, &data_cnt);
		if (!rc) {
			if (data_cnt == 15) {
				select_rsp->max_size = sys_get_le32(receive_data + 3);
				select_rsp->offset   = sys_get_le32(receive_data + 7);
				select_rsp->crc      = sys_get_le32(receive_data + 11);

				LOG_DBG("Object selected:  max_size:%u offset:%u crc:0x%08X",
					select_rsp->max_size,
					select_rsp->offset,
					select_rsp->crc);
			} else {
				LOG_ERR("Invalid object response!");
				rc = 1;
			}
		}
	}

	return rc;
}

static int req_obj_create(uint8_t obj_type, uint32_t obj_size)
{
	int rc;
	uint8_t send_data[6] = { NRF_DFU_OP_OBJECT_CREATE };

	LOG_DBG("%s", __func__);

	send_data[1] = obj_type;
	sys_put_le32(obj_size, send_data + 2);
	rc = send_raw(send_data, sizeof(send_data));

	if (!rc) {
		uint32_t data_cnt;

		rc = get_rsp(NRF_DFU_OP_OBJECT_CREATE, &data_cnt);
	}

	return rc;
}

static int stream_data(const uint8_t *data, uint32_t data_size)
{
	int rc = 0;
	uint32_t pos, stp;
	uint32_t stp_max = 5;

	LOG_DBG("%s", __func__);

	if (data == NULL || !data_size) {
		rc = 1;
	}

	if (!rc) {
		if (mtu >= 5) {
			stp_max = (mtu - 1) / 2 - 1;
		} else {
			LOG_ERR("MTU is too small to send data!");
			rc = 1;
			return 1;
		}
	}

	for (pos = 0; !rc && pos < data_size; pos += stp) {
		send_data[0] = NRF_DFU_OP_OBJECT_WRITE;
		stp = MIN((data_size - pos), stp_max);
		memcpy(send_data + 1, data + pos, stp);
		rc = send_raw(send_data, stp + 1);
	}

	return rc;
}

static int req_get_crc(struct nrf_dfu_response_crc *crc_rsp)
{
	int rc;
	uint8_t send_data[1] = { NRF_DFU_OP_CRC_GET };

	LOG_DBG("%s", __func__);

	rc = send_raw(send_data, sizeof(send_data));
	if (!rc) {
		uint32_t data_cnt;

		rc = get_rsp(NRF_DFU_OP_CRC_GET, &data_cnt);
		if (!rc) {
			if (data_cnt == 11) {
				crc_rsp->offset = sys_get_le32(receive_data + 3);
				crc_rsp->crc    = sys_get_le32(receive_data + 7);
			} else {
				LOG_ERR("Invalid CRC response!");
				rc = 1;
			}
		}
	}

	return rc;
}

static int req_obj_execute(void)
{
	int rc;
	uint8_t send_data[1] = { NRF_DFU_OP_OBJECT_EXECUTE };

	LOG_DBG("%s", __func__);

	rc = send_raw(send_data, sizeof(send_data));
	if (!rc) {
		uint32_t data_cnt;

		rc = get_rsp(NRF_DFU_OP_OBJECT_EXECUTE, &data_cnt);
	}

	return rc;
}

static int stream_data_crc(const uint8_t *data, uint32_t data_size,
			   uint32_t pos, uint32_t *crc)
{
	int rc;
	struct nrf_dfu_response_crc rsp_crc;

	LOG_DBG("%s", __func__);
	LOG_DBG("Streaming Data: len:%u offset:%u crc:0x%08X", data_size, pos, *crc);

	rc = stream_data(data, data_size);

	if (!rc) {
		*crc = crc32_ieee_update(*crc, data, data_size);
		rc = req_get_crc(&rsp_crc);
	}

	if (!rc) {
		if (rsp_crc.offset != pos + data_size) {
			LOG_ERR("Invalid offset (%u -> %u)!", pos + data_size, rsp_crc.offset);
			rc = 2;
		}
		if (rsp_crc.crc != *crc) {
			LOG_ERR("Invalid CRC (0x%08X -> 0x%08X)!", *crc, rsp_crc.crc);
			rc = 2;
		}
	}

	return rc;
}

static int try_recover_ip(const uint8_t *data, uint32_t data_size,
			  struct nrf_dfu_response_select *rsp_recover,
			  const struct nrf_dfu_response_select *rsp_select)
{
	int rc = 0;
	uint32_t pos_start, len_remain;
	uint32_t crc_32;

	LOG_DBG("%s", __func__);

	*rsp_recover = *rsp_select;
	pos_start = rsp_recover->offset;

	if (pos_start > 0 && pos_start <= data_size) {
		crc_32 = crc32_ieee(data, pos_start);
		if (rsp_select->crc != crc_32) {
			pos_start = 0;
		}
	} else {
		pos_start = 0;
	}

	if (pos_start > 0 && pos_start < data_size) {
		len_remain = data_size - pos_start;
		rc = stream_data_crc(data + pos_start, len_remain, pos_start, &crc_32);
		if (!rc) {
			pos_start += len_remain;
		} else if (rc == 2) {
			/* when there is a CRC error, discard previous init packet */
			rc = 0;
			pos_start = 0;
		}
	}

	if (!rc && pos_start == data_size) {
		rc = req_obj_execute();
	}

	rsp_recover->offset = pos_start;

	return rc;
}

static int try_recover_fw(const uint8_t *data, uint32_t data_size,
			  struct nrf_dfu_response_select *rsp_recover,
			  const struct nrf_dfu_response_select *rsp_select)
{
	int rc = 0;
	uint32_t max_size, stp_size;
	uint32_t pos_start, len_remain;
	uint32_t crc_32;
	int obj_exec = 1;

	LOG_DBG("%s", __func__);

	*rsp_recover = *rsp_select;
	pos_start = rsp_recover->offset;

	if (pos_start > data_size) {
		LOG_ERR("Invalid firmware offset reported!");
		rc = 1;
	} else if (pos_start > 0) {
		max_size = rsp_select->max_size;
		crc_32 = crc32_ieee(data, pos_start);
		len_remain = pos_start % max_size;

		if (rsp_select->crc != crc_32) {
			pos_start -= ((len_remain > 0) ? len_remain : max_size);
			rsp_recover->offset = pos_start;

			return rc;
		}

		if (len_remain > 0) {
			stp_size = max_size - len_remain;

			rc = stream_data_crc(data + pos_start, stp_size, pos_start, &crc_32);
			if (!rc) {
				pos_start += stp_size;
			} else if (rc == 2) {
				rc = 0;
				pos_start -= len_remain;
				obj_exec = 0;
			}

			rsp_recover->offset = pos_start;
		}

		if (!rc && obj_exec) {
			rc = req_obj_execute();
		}
	}

	return rc;
}

bool dfu_host_bl_mode_check(void)
{
	int rc = 0;
	int cnt = 3;	/* Try 3 times */

	while (cnt--) {
		rc = req_ping(ping_id++);
		if (!rc) {
			break;
		}
	}

	return rc == 0;
}

int dfu_host_setup(uint16_t mtu_size, uint16_t pause_time)
{
	int rc;

	ARG_UNUSED(mtu_size);
	ARG_UNUSED(pause_time);

	ping_id++;

	rc = req_ping(ping_id);

	if (!rc) {
		rc = req_set_prn(prn);
	}

	if (!rc) {
		rc = req_get_mtu(&mtu);
	}

	return rc;
}

int dfu_host_send_ip(const uint8_t *data, uint32_t data_size)
{
	int rc = 0;
	uint32_t crc_32 = 0;
	struct nrf_dfu_response_select rsp_select;
	struct nrf_dfu_response_select rsp_recover;

	LOG_INF("Sending init packet...");

	if (data == NULL || !data_size) {
		LOG_ERR("Invalid init packet!");
		rc = 1;
	}

	if (!rc) {
		rc = req_obj_select(0x01, &rsp_select);
	}

	if (!rc) {
		rc = try_recover_ip(data, data_size, &rsp_recover, &rsp_select);
		if (!rc && rsp_recover.offset == data_size) {
			return rc;
		}
	}

	if (!rc) {
		if (data_size > rsp_select.max_size) {
			LOG_ERR("Init packet too big!");
			rc = 1;
		}
	}

	if (!rc) {
		rc = req_obj_create(0x01, data_size);
	}

	if (!rc) {
		rc = stream_data_crc(data, data_size, 0, &crc_32);
	}

	if (!rc) {
		rc = req_obj_execute();
	}

	return rc;
}

int dfu_host_send_fw(const uint8_t *data, uint32_t data_size)
{
	int rc = 0;
	uint32_t max_size, stp_size, pos;
	uint32_t crc_32 = 0;
	struct nrf_dfu_response_select rsp_select;
	struct nrf_dfu_response_select rsp_recover;
	uint32_t pos_start;

	LOG_INF("Sending firmware file...");

	if (data == NULL || !data_size) {
		LOG_ERR("Invalid firmware data!");
		rc = 1;
	}

	if (!rc) {
		rc = req_obj_select(0x02, &rsp_select);
	}

	if (!rc) {
		rc = try_recover_fw(data, data_size, &rsp_recover, &rsp_select);
	}

	if (!rc) {
		max_size = rsp_select.max_size;
		pos_start = rsp_recover.offset;
		crc_32 = crc32_ieee_update(crc_32, data, pos_start);

		for (pos = pos_start; pos < data_size; pos += stp_size) {
			stp_size = MIN((data_size - pos), max_size);
			rc = req_obj_create(0x02, stp_size);
			if (!rc) {
				rc = stream_data_crc(data + pos, stp_size, pos, &crc_32);
			}

			if (!rc) {
				rc = req_obj_execute();
			}

			if (rc) {
				break;
			}
		}
	}

	return rc;
}
