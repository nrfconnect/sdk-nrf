/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NFC_NDEF_TNEP_REC_H_
#define NFC_NDEF_TNEP_REC_H_

/**@file
 *
 * @defgroup nfc_ndef_tnep_rec TNEP records
 * @{
 * @ingroup  nfc_ndef_messages
 *
 * @brief    Generation of NFC NDEF TNEP specific record descriptions.
 *
 */

#include <zephyr/types.h>
#include <nfc/ndef/nfc_ndef_record.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NFC_NDEF_TNEP_REC_TYPE_LEN 2

/**
 * @brief External reference to the type field of the TNEP records,
 * defined in the file @c nfc_tnep_rec.c.
 */
extern const u8_t nfc_ndef_tnep_rec_type_svc_param[NFC_NDEF_TNEP_REC_TYPE_LEN];
extern const u8_t nfc_ndef_tnep_rec_type_svc_select[NFC_NDEF_TNEP_REC_TYPE_LEN];
extern const u8_t nfc_ndef_tnep_rec_type_status[NFC_NDEF_TNEP_REC_TYPE_LEN];

/** @brief TNEP Status. */
struct nfc_ndef_tnep_status {
	/** Status type for TNEP Status Message */
	u8_t status_type;
};

/** @brief Service structure. */
struct nfc_ndef_tnep_svc_select {
	/** Length of the following Service Name URI. */
	u8_t uri_name_len;

	/** Service Name URI. */
	u8_t *uri_name;
};

/** @brief  Service Parameters in the TNEP's Initial NDEF message. */
struct nfc_ndef_tnep_svc_param {
	/** TNEP Version. */
	u8_t tnep_version;

	/** Length of the Service Name URI. */
	u8_t svc_name_uri_length;

	/** Service Name URI. */
	u8_t *svc_name_uri;

	/** TNEP communication mode */

	u8_t communication_mode;
	/** Minimum waiting time. */
	u8_t min_waiting_time;

	/** Maximum number of waiting time extensions N_wait. */
	u8_t max_waiting_time_ext;

	/** Maximum NDEF message size in bytes. */
	u16_t max_message_size;
};

/**
 * @brief Payload constructor for the NDEF TNEP Status Record.
 *
 * @param nfc_rec_tnep_payload_desc Pointer to the TNEP status record
 * description.
 * @param buffer Pointer to the payload destination. If NULL, function will
 * calculate the expected size of the Text record payload.
 * @param len Size of the available memory to write as input.
 * Size of the generated record payload as output.
 *
 * @retval 0 If the operation was successful.
 * Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_tnep_status_payload(struct nfc_ndef_tnep_status *payload_desc,
				 u8_t *buffer, u32_t *len);

/**
 * @brief Payload constructor for the TNEP Service Select Record.
 *
 * @param nfc_rec_tnep_payload_desc Pointer to the TNEP service select
 * record description.
 * @param buffer Pointer to the payload destination. If NULL, function will
 * calculate the expected size of the Text record payload.
 * @param len Size of the available memory to write as input.
 * Size of the generated record payload as output.
 *
 * @retval 0 If the operation was successful.
 *	   Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_tnep_svc_select_payload(
		struct nfc_ndef_tnep_svc_select *payload_desc, u8_t *buffer,
		u32_t *len);

/**
 * @brief Payload constructor for the Service Parameter Record.
 *
 * @param nfc_rec_tnep_payload_desc Pointer to the TNEP service parameters
 * record description.
 * @param buffer Pointer to the payload destination. If NULL, function will
 * calculate the expected size of the Text record payload.
 * @param len Size of the available memory to write as input.
 * Size of the generated record payload as output.
 *
 * @retval 0 If the operation was successful.
 * Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_tnep_svc_param_payload(
		struct nfc_ndef_tnep_svc_param *payload_desc,
		u8_t *buffer, u32_t *len);

/**
 * @brief Macro for creating and initializing a NFC NDEF record descriptor for
 * a TENP Status Record.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc and an instance of type
 * @ref nfc_tnep_status, which together constitute an instance of
 * a TNEP status record.
 *
 * Use the macro @ref NFC_NDEF_TNEP_RECORD_DESC to access the NDEF TENP record
 * descriptor instance.
 * Use name to access a TNEP Status description.
 *
 * @param name Name of the created record descriptor instance.
 * @param status_type_arg TNEP Message Status.
 */
#define NFC_TNEP_STATUS_RECORD_DESC_DEF(name,				\
				status_type_arg)			\
	struct nfc_ndef_tnep_status name =				\
	{								\
		.status_type = status_type_arg,				\
	};								\
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(name,				\
				TNF_WELL_KNOWN,				\
				0,					\
				0,					\
				nfc_ndef_tnep_rec_type_status,		\
				NFC_NDEF_TNEP_REC_TYPE_LEN,		\
				nfc_ndef_tnep_status_payload,		\
				&(name))

/**
 * @brief Macro for creating and initializing a NFC NDEF record descriptor for
 *	 a TNEP Service Select Record.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc and an instance of type
 * @ref nfc_tnep_service_select, which together constitute an instance of
 * a TNEP service select record.
 *
 * Use the macro @ref NFC_NDEF_TNEP_RECORD_DESC to access the NDEF TNEP record
 * descriptor instance.
 * Use name to access a TNEP Service Select description.
 *
 * @param name Name of the created record descriptor instance.
 * @param service_name_uri_length_arg  Length of the following Service Name URI.
 * @param service_name_uri_arg Service Name URI.
 */
#define NFC_TNEP_SERIVCE_SELECT_RECORD_DESC_DEF(name,			\
					svc_name_uri_length_arg,	\
					svc_name_uri_arg)		\
	struct nfc_ndef_tnep_svc_select name =				\
	{								\
		.uri_name_len = svc_name_uri_length_arg,		\
		.uri_name = svc_name_uri_arg,				\
	};								\
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(name,				\
			TNF_WELL_KNOWN,					\
			0,						\
			0,						\
			nfc_ndef_tnep_rec_type_svc_select,		\
			NFC_NDEF_TNEP_REC_TYPE_LEN,			\
			nfc_ndef_tnep_svc_select_payload,		\
			&(name))

/**
 * @brief Macro for creating and initializing a NFC NDEF record descriptor for
 *	 a TNEP Service Parameter Record.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc and an instance of type
 * @ref nfc_tnep_service_parameters, which together constitute an instance of
 * a TNEP service parameter record.
 *
 * Use the macro @ref NFC_NDEF_TNEP_RECORD_DESC to access the NDEF TNEP record
 * descriptor instance.
 * Use name to access a TNEP Service Parameter description.
 *
 * @param name Name of the created record descriptor instance.
 * @param tnep_version_arg TNEP Version.
 * @param service_name_length_arg Service name uri length in bytes.
 * @param service_name_arg Service name uri.
 * @param communication_mode_arg TNEP communication mode.
 * @param min_waiting_time_arg Minimum waiting time.
 * @param max_waiting_time_exec_arg Maximum number of waiting time extensions
 * N_wait.
 * @param max_message_size_arg Maximum NDEF message size in bytes.
 */
#define NFC_TNEP_SERIVCE_PARAM_RECORD_DESC_DEF(name,			\
				tnep_version_arg,			\
				svc_name_length_arg,			\
				svc_name_arg,				\
				communication_mode_arg,			\
				min_waiting_time_arg,			\
				max_waiting_time_exec_arg,		\
				max_message_size_arg)			\
	struct nfc_ndef_tnep_svc_param name =				\
	{								\
		.tnep_version = tnep_version_arg,			\
		.svc_name_uri_length = svc_name_length_arg,		\
		.svc_name_uri = svc_name_arg,				\
		.communication_mode = communication_mode_arg,		\
		.min_waiting_time = min_waiting_time_arg,		\
		.max_waiting_time_ext = max_waiting_time_exec_arg,	\
		.max_message_size = max_message_size_arg,		\
	};								\
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(name,				\
			 TNF_WELL_KNOWN,				\
			 0,						\
			 0,						\
			 nfc_ndef_tnep_rec_type_svc_param,		\
			 NFC_NDEF_TNEP_REC_TYPE_LEN,			\
			 nfc_ndef_tnep_svc_param_payload,		\
			 &(name))

/**
 * @brief Macro for accessing the NFC NDEF TNEP record descriptor
 * instance that was created with @ref NFC_NDEF_TNEP_RECORD_DESC.
 */

#define NFC_NDEF_TNEP_RECORD_DESC(name) NFC_NDEF_GENERIC_RECORD_DESC(name)

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NFC_NDEF_TNEP_REC_H_ */
