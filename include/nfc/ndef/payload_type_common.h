/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_PAYLOAD_TYPE_COMMON_H_
#define NFC_NDEF_PAYLOAD_TYPE_COMMON_H_

/**@file
 *
 * @defgroup nfc_ndef_payload_type_common Standard NDEF Record Type definitions
 * @{
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
extern const uint8_t nfc_ndef_le_oob_rec_type_field[32];

/**
 * @brief An external reference to the type field of the Handover Select
 * record, defined in the file @c ch_rec.c. It is used in the
 * @em NFC_NDEF_HS_RECORD_DESC_DEF macro.
 */
extern const uint8_t nfc_ndef_ch_hs_rec_type_field[2];

/**
 * @brief An external reference to the type field of the Handover Request
 * record, defined in the file @c ch_rec.c. It is used in the
 * @em NFC_NDEF_HR_RECORD_DESC_DEF macro.
 */
extern const uint8_t nfc_ndef_ch_hr_rec_type_field[2];

/**
 * @brief An external reference to the type field of the Handover Mediation
 * Record, defined in the file @c ch_rec.c. It is used in the
 * @em NFC_NDEF_HM_RECORD_DESC_DEF macro.
 */
extern const uint8_t nfc_ndef_ch_hm_rec_type_field[2];

/**
 * @brief An external reference to the type field of the Handover Initiate
 * Record, defined in the file @c ch_rec.c. It is used in the
 * @em NFC_NDEF_HI_RECORD_DESC_DEF macro.
 */
extern const uint8_t nfc_ndef_ch_hi_rec_type_field[2];

/**
 * @brief An external reference to the type field of the Handover Carrier
 * Record, defined in the file @c ch_rec.c. It is used in the
 * @em NFC_NDEF_HC_RECORD_DESC_DEF macro.
 */
extern const uint8_t nfc_ndef_ch_hc_rec_type_field[2];

/**
 * @brief External reference to the type field of the Alternative Carrier
 * Record, defined in the file @c ch_rec.c. It is used in the
 * @em NFC_NDEF_AC_RECORD_DESC_DEF macro.
 */
extern const uint8_t nfc_ndef_ch_ac_rec_type_field[2];

/**
 * @brief External reference to the type field of the Collision Resolution
 * Record, defined in the file @c ch_rec.c. It is used in the
 * @em NFC_NDEF_CR_RECORD_DESC_DEF macro.
 */
extern const uint8_t nfc_ndef_ch_cr_rec_type_field[2];

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NFC_NDEF_PAYLOAD_TYPE_COMMON_H_ */
