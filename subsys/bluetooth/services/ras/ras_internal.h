/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_RAS_INTERNAL_H_
#define BT_RAS_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/services/ras.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RASCP_CMD_OPCODE_LEN     1
#define RASCP_CMD_OPCODE_OFFSET  0
#define RASCP_CMD_PARAMS_OFFSET  RASCP_CMD_OPCODE_LEN
#define RASCP_CMD_PARAMS_MAX_LEN 4
#define RASCP_WRITE_MAX_LEN      (RASCP_CMD_OPCODE_LEN + RASCP_CMD_PARAMS_MAX_LEN)
#define RASCP_ACK_DATA_TIMEOUT   K_SECONDS(5)

/** @brief RAS Control Point opcodes as defined in RAS Specification, Table 3.10. */
enum rascp_opcode {
	RASCP_OPCODE_GET_RD                    = 0x00,
	RASCP_OPCODE_ACK_RD                    = 0x01,
	RASCP_OPCODE_RETRIEVE_LOST_RD_SEGMENTS = 0x02,
	RASCP_OPCODE_ABORT_OP                  = 0x03,
	RASCP_OPCODE_SET_FILTER                = 0x04,
};

/** @brief RAS Control Point Response opcodes as defined in RAS Specification, Table 3.11. */
enum rascp_rsp_opcode {
	RASCP_RSP_OPCODE_COMPLETE_RD_RSP          = 0x00,
	RASCP_RSP_OPCODE_COMPLETE_LOST_RD_SEG_RSP = 0x01,
	RASCP_RSP_OPCODE_RSP_CODE                 = 0x02,
};

#define RASCP_RSP_OPCODE_COMPLETE_RD_RSP_LEN          2
#define RASCP_RSP_OPCODE_COMPLETE_LOST_RD_SEG_RSP_LEN 4
#define RASCP_RSP_OPCODE_RSP_CODE_LEN                 1

/** @brief RAS Control Point Response Code Values as defined in RAS Specification, Table 3.12. */
enum rascp_rsp_code {
	RASCP_RESPONSE_RESERVED                = 0x00,
	RASCP_RESPONSE_SUCCESS                 = 0x01,
	RASCP_RESPONSE_OPCODE_NOT_SUPPORTED    = 0x02,
	RASCP_RESPONSE_INVALID_PARAMETER       = 0x03,
	RASCP_RESPONSE_SUCCESS_PERSISTED       = 0x04,
	RASCP_RESPONSE_ABORT_UNSUCCESSFUL      = 0x05,
	RASCP_RESPONSE_PROCEDURE_NOT_COMPLETED = 0x06,
	RASCP_RESPONSE_SERVER_BUSY             = 0x07,
	RASCP_RESPONSE_NO_RECORDS_FOUND        = 0x08,
};

/** @brief RAS ATT Application error codes as defined in RAS Specification, Table 2.1. */
enum ras_att_error {
	RAS_ATT_ERROR_CCC_CONFIG         = 0xFD,
	RAS_ATT_ERROR_WRITE_REQ_REJECTED = 0xFC,
};

/** @brief RAS Segmentation Header as defined in RAS Specification, Table 3.5. */
struct ras_seg_header {
	bool    first_seg   : 1;
	bool    last_seg    : 1;
	uint8_t seg_counter : 6;
} __packed;
BUILD_ASSERT(sizeof(struct ras_seg_header) == 1);

/** @brief RAS Ranging Data segment format as defined in RAS Specification, Table 3.4. */
struct ras_segment {
	struct ras_seg_header header;
	uint8_t               data[];
} __packed;

#ifdef __cplusplus
}
#endif

#endif /* BT_RAS_INTERNAL_H_ */
