/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NFC_NDEF_PAYLOAD_TYPE_COMMON_H_
#define NFC_NDEF_PAYLOAD_TYPE_COMMON_H_

/**@file
 *
 * @defgroup nfc_ndef_payload_type_common Standard NDEF Record Type definitions
 *
 * @brief    Set of standard NDEF Record Types defined in NFC Forum
 *           specifications.
 *
 */
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief External reference to the type field of the Bluetooth LE Carrier
 * Configuration NDEF Record, defined in the file @c payload_type_common.c.
 */
extern const u8_t nfc_ndef_le_oob_rec_type_field[32];

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NFC_NDEF_PAYLOAD_TYPE_COMMON_H_ */
