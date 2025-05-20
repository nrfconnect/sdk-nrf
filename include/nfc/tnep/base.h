/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_TNEP_BASE_H_
#define NFC_TNEP_BASE_H_

/**
 * @file
 * @defgroup nfc_tnep_base Tag NDEF Exchange Protocol (TNEP) common API.
 * @{
 * @brief Tag NDEF Exchange Protocol common data structure definitions.
 */

#include <stddef.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/tnep_rec.h>

/** TNEP Version.
 *  A major version number in high nibble, a minor version number in low nibble.
 */
#define NFC_TNEP_VERSION 0x10
#define NFC_TNEP_NDEF_NLEN_SIZE 2

/** @brief Service communication modes. */
enum nfc_tnep_comm_mode {
	/** Single response communication mode */
	NFC_TNEP_COMM_MODE_SINGLE_RESPONSE,

	/** Service specific communication mode */
	NFC_TNEP_COMM_MODE_SERVICE_SPECYFIC = 0xFE
};

/** @brief Service status - payload in TNEP Status Record. */
enum nfc_tnep_status_value {
	/** Success */
	NFC_TNEP_STATUS_SUCCESS,

	/** TNEP protocol error */
	NFC_TNEP_STATUS_PROTOCOL_ERROR,

	/** First service error code. */
	NFC_TNEP_STATUS_SERVICE_ERROR_BEGIN = 0x80,

	/** Last service error code. */
	NFC_TNEP_STATUS_SERVICE_ERROR_END = 0xFE,
};

/**
 * @}
 */

#endif /* NFC_TNEP_BASE_H_ */
