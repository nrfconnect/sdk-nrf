/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_T4T_APDU_H_
#define NFC_T4T_APDU_H_

/**@file apdu.h
 *
 * @defgroup nfc_t4t_apdu APDU reader and writer
 * @{
 *
 * @brief APDU reader and writer for Type 4 Tag communication.
 *
 */

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Class byte indicating no secure messaging, used in C-APDU. */
#define NFC_T4T_APDU_CLASS_BYTE_NO_SECURE_MSG 0x00

/**
 * @defgroup nfc_t4t_apdu_select Parameters for selecting
 *           instruction code in C-APDU
 * @{
 */

 /** Select by file identifier, first or only occurrence. */
#define NFC_T4T_APDU_SELECT_BY_FILE_ID 0x000C

/** Select by name, first or only occurrence. */
#define NFC_T4T_APDU_SELECT_BY_NAME 0x0400
/**
 * @}
 */

/**
 * @defgroup nfc_t4t_apdu_status Status codes contained in R-APDU
 * @{
 */

/** Command completed successfully. */
#define NFC_T4T_APDU_RAPDU_STATUS_CMD_COMPLETED 0x9000

/** Selected item has not been found. */
#define NFC_T4T_APDU_RAPDU_STATUS_SEL_ITEM_NOT_FOUND 0x6A82

/**
 * @}
 */

/** @brief Possible instruction codes in C-APDU.
 */
enum nfc_t4t_apdu_comm_ins {
	/** Code used for selecting EF or NDEF application. */
	NFC_T4T_APDU_COMM_INS_SELECT = 0xA4,

	/** Code used for selecting EF or NDEF application. */
	NFC_T4T_APDU_COMM_INS_READ = 0xB0,

	/** Code used for selecting EF or NDEF application. */
	NFC_T4T_APDU_COMM_INS_UPDATE = 0xD6
};

/** @brief APDU data field descriptor.
 */
struct nfc_t4t_apdu_data {
	/** Data field length. */
	uint16_t len;

	/** Pointer to data field. */
	uint8_t *buff;
};

/** @brief Command Application Protocol Data Unit (C-APDU) descriptor.
 */
struct nfc_t4t_apdu_comm {
	/* Class byte. */
	uint8_t class_byte;

	/** The chosen code of instruction. */
	enum nfc_t4t_apdu_comm_ins instruction;

	/** Parameters associated with the instruction code. */
	uint16_t parameter;

	/** Optional data fields (Lc + data bytes). */
	struct nfc_t4t_apdu_data data;

	/** Optional response length field (Le). */
	uint16_t resp_len;
};

/** @brief Response Application Protocol Data Unit (R-APDU) descriptor.
 */
struct nfc_t4t_apdu_resp {
	/** Mandatory status field. */
	uint16_t status;

	/** Optional data field. */
	struct nfc_t4t_apdu_data data;
};

/** @brief Clear a C-APDU descriptor and restore its default values.
 *
 *  @param[in] cmd_apdu Pointer to C-APDU descriptor.
 */
static inline void nfc_t4t_apdu_comm_clear(struct nfc_t4t_apdu_comm *cmd_apdu);

/** @brief Clearing an R-APDU descriptor and restore its default values.
 *
 * @param[in] resp_apdu Pointer to R-APDU descriptor.
 */
static inline void nfc_t4t_apdu_resp_clear(struct nfc_t4t_apdu_resp *resp_apdu);

/** @brief Encode C-APDU.
 *
 *  This function encodes C-APDU according to the provided descriptor.
 *
 *  @param[in] cmd_apdu  Pointer to the C-APDU descriptor.
 *  @param[out] raw_data Pointer to the buffer with encoded C-APDU.
 *  @param[in,out] len   Size of the available memory for the C-APDU as input.
 *                       Size of the generated C-APDU as output.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_apdu_comm_encode(const struct nfc_t4t_apdu_comm *cmd_apdu,
			     uint8_t *raw_data, uint16_t *len);

/** @brief Decode R-APDU.
 *
 *  This function decodes buffer with encoded R-APDU and stores results
 *  in the R-APDU descriptor.
 *
 *  @param[out] resp_apdu Pointer to the R-APDU descriptor.
 *  @param[in] raw_data Pointer to the buffer with encoded R-APDU.
 *  @param[in] len Size of the buffer with encoded R-APDU.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_apdu_resp_decode(struct nfc_t4t_apdu_resp *resp_apdu,
			     const uint8_t *raw_data, uint16_t len);

/** @brief Print an R-APDU descriptor.
 *
 *  This function prints an R-APDU descriptor.
 *
 *  @param[in] resp_apdu Pointer to the R-APDU descriptor.
 */
void nfc_t4t_apdu_resp_printout(const struct nfc_t4t_apdu_resp *resp_apdu);

static inline void nfc_t4t_apdu_comm_clear(struct nfc_t4t_apdu_comm *cmd_apdu)
{
	memset(cmd_apdu, 0, sizeof(struct nfc_t4t_apdu_comm));
}

static inline void nfc_t4t_apdu_resp_clear(struct nfc_t4t_apdu_resp *resp_apdu)
{
	memset(resp_apdu, 0, sizeof(struct nfc_t4t_apdu_resp));
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_T4T_APDU_H_ */
