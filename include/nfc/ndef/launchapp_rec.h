/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_LAUNCHAPP_REC_H__
#define NFC_LAUNCHAPP_REC_H__

/**@file
 *
 * @defgroup nfc_launchapp_rec Launch App records
 * @{
 * @ingroup nfc_launchapp_msg
 *
 * @brief Generation of NFC NDEF record descriptions that launch apps.
 *
 */

#include <stdint.h>
#include <nfc/ndef/record.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Size of the type field of the Android Application Record, defined in the file
 * @c launchapp_rec.c. It is used in the @ref NFC_NDEF_ANDROID_LAUNCHAPP_RECORD_DESC macro.
 */
#define NFC_ANDROID_REC_TYPE_LENGTH 15

/**
 * @brief External reference to the type field of the NFC NDEF Android Application Record, defined
 * in the file @c launchapp_rec.c. It is used in the
 * @ref NFC_NDEF_ANDROID_LAUNCHAPP_RECORD_DESC_DEF macro.
 */
extern const uint8_t ndef_android_launchapp_rec_type[NFC_ANDROID_REC_TYPE_LENGTH];

/** @brief Macro for generating a description of an NFC NDEF Android Application Record (AAR).
 *
 * This macro declares and initializes an instance of an NFC NDEF record description
 * of an Android Application Record (AAR).
 *
 * @note The record descriptor is declared as an automatic variable, which implies that
 *       the NDEF message encoding (see @ref nfc_launchapp_msg_encode) must be done
 *       in the same variable scope.
 *
 * @param[in] name Name for accessing the record descriptor.
 * @param[in] package_name Pointer to the Android package name string.
 * @param[in] package_name_length Length of the Android package name.
 */
#define NFC_NDEF_ANDROID_LAUNCHAPP_RECORD_DESC_DEF(name,			\
						   package_name,		\
						   package_name_length)		\
	NFC_NDEF_RECORD_BIN_DATA_DEF(name,					\
				     TNF_EXTERNAL_TYPE,				\
				     NULL,					\
				     0,						\
				     ndef_android_launchapp_rec_type,		\
				     sizeof(ndef_android_launchapp_rec_type),	\
				     (package_name),				\
				     (package_name_length))

/**
 * @brief Macro for accessing the NFC NDEF Android Application Record descriptor
 * instance that was created with @ref NFC_NDEF_ANDROID_LAUNCHAPP_RECORD_DESC_DEF.
 */
#define NFC_NDEF_ANDROID_LAUNCHAPP_RECORD_DESC(name) NFC_NDEF_RECORD_BIN_DATA(name)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // NFC_LAUNCHAPP_REC
