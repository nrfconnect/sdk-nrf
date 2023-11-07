/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_TNEP_REC_H_
#define NFC_NDEF_TNEP_REC_H_

/**@file
 *
 * @defgroup nfc_ndef_tnep_rec TNEP records API
 * @{
 * @brief Generation of NFC NDEF TNEP specific record descriptions.
 *
 */

#include <zephyr/types.h>
#include <nfc/ndef/record.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NFC_NDEF_TNEP_REC_TYPE_LEN 2

/*
 * @brief External reference to the type field of the TNEP records,
 *        defined in the file @c nfc_tnep_rec.c.
 */
extern const uint8_t nfc_ndef_tnep_rec_type_svc_param[];
extern const uint8_t nfc_ndef_tnep_rec_type_svc_select[];
extern const uint8_t nfc_ndef_tnep_rec_type_status[];

/** @brief TNEP Status. */
struct nfc_ndef_tnep_rec_status {
	/** Status type for TNEP Status Message */
	uint8_t status;
};

/** @brief Service structure. */
struct nfc_ndef_tnep_rec_svc_select {
	/** Length of the following Service Name URI. */
	uint8_t uri_len;

	/** Service Name URI. */
	const uint8_t *uri;
};

/** @brief Service Parameters in the TNEP's Initial NDEF message. */
struct nfc_ndef_tnep_rec_svc_param {
	/** TNEP Version. */
	uint8_t version;

	/** Length of the Service Name URI. */
	uint8_t uri_length;

	/** Service Name URI. */
	const uint8_t *uri;

	/** TNEP communication mode */
	uint8_t communication_mode;

	/** Minimum waiting time. */
	uint8_t min_time;

	/** Maximum number of waiting time extensions N_wait. */
	uint8_t max_time_ext;

	/** Maximum NDEF message size in bytes. */
	uint16_t max_size;
};

/**
 * @brief Payload constructor for the NDEF TNEP Status Record.
 *
 * @param[in] payload_desc Pointer to the TNEP status record
 *                         description.
 * @param[out] buffer Pointer to the payload destination. If NULL, function will
 *                    calculate the expected size of the Text record payload.
 * @param[in,out] len Size of the available memory to write as input.
 *                    Size of the generated record payload as output.
 *
 * @retval 0 If the operation was successful.
 * Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_tnep_rec_status_payload(struct nfc_ndef_tnep_rec_status *payload_desc,
				     uint8_t *buffer, uint32_t *len);

/**
 * @brief Payload constructor for the TNEP Service Select Record.
 *
 * @param[in] payload_desc Pointer to the TNEP service select
 *                         record description.
 * @param[out] buffer Pointer to the payload destination. If NULL, function will
 *                    calculate the expected size of the Text record payload.
 * @param[in,out] len Size of the available memory to write as input.
 *                    Size of the generated record payload as output.
 *
 * @retval 0 If the operation was successful.
 *	   Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_tnep_rec_svc_select_payload(struct nfc_ndef_tnep_rec_svc_select *payload_desc,
					 uint8_t *buffer, uint32_t *len);

/**
 * @brief Payload constructor for the Service Parameter Record.
 *
 * @param[in] payload_desc Pointer to the TNEP service parameters
 *                         record description.
 * @param[out] buffer Pointer to the payload destination. If NULL, function will
 *                    calculate the expected size of the Text record payload.
 * @param[in,out] len Size of the available memory to write as input.
 *                    Size of the generated record payload as output.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_tnep_rec_svc_param_payload(struct nfc_ndef_tnep_rec_svc_param *payload_desc,
					uint8_t *buffer, uint32_t *len);

/**
 * @brief Macro for creating and initializing a NFC NDEF record descriptor for
 *        a TENP Status Record.
 *
 * This macro creates and initializes an instance of type
 * nfc_ndef_record_desc and an instance of type nfc_ndef_tnep_rec_status,
 * which together constitute an instance of a TNEP status record.
 *
 * Use the macro @ref NFC_NDEF_TNEP_RECORD_DESC to access the NDEF TENP record
 * descriptor instance. Use name to access a TNEP Status description.
 *
 * @param[in] _name Name of the created record descriptor instance.
 * @param[in] _status TNEP Message Status.
 */
#define NFC_TNEP_STATUS_RECORD_DESC_DEF(_name, _status)                   \
	struct nfc_ndef_tnep_rec_status _name = {                         \
		.status = _status,                                        \
	};                                                                \
									  \
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(_name, TNF_WELL_KNOWN,           \
					 0, 0,                            \
					 nfc_ndef_tnep_rec_type_status,   \
					 NFC_NDEF_TNEP_REC_TYPE_LEN,      \
					 nfc_ndef_tnep_rec_status_payload,\
					 &(_name))

/**
 * @brief Macro for creating and initializing a NFC NDEF record descriptor for
 *	 a TNEP Service Select Record.
 *
 * This macro creates and initializes an instance of type
 * nfc_ndef_record_desc and an instance of type nfc_ndef_tnep_rec_svc_select,
 * which together constitute an instance of a TNEP service select record.
 *
 * Use the macro @ref NFC_NDEF_TNEP_RECORD_DESC to access the NDEF TNEP record
 * descriptor instance. Use name to access a TNEP Service Select description.
 *
 * @param[in] _name Name of the created record descriptor instance.
 * @param[in] _uri_length Length of the following Service Name URI.
 * @param[in] _uri Service Name URI.
 */
#define NFC_TNEP_SERIVCE_SELECT_RECORD_DESC_DEF(_name, _uri_length, _uri)      \
	struct nfc_ndef_tnep_rec_svc_select _name = {                          \
		.uri_len = _uri_length,                                        \
		.uri = _uri,	                                               \
	};                                                                     \
									       \
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(_name, TNF_WELL_KNOWN,                \
					 0, 0,                                 \
					 nfc_ndef_tnep_rec_type_svc_select,    \
					 NFC_NDEF_TNEP_REC_TYPE_LEN,           \
					 nfc_ndef_tnep_rec_svc_select_payload, \
					 &(_name))

/**
 * @brief Macro for creating and initializing a NFC NDEF record descriptor for
 *	 a TNEP Service Parameter Record.
 *
 * This macro creates and initializes an instance of type
 * nfc_ndef_record_desc and an instance of type nfc_ndef_tnep_rec_svc_param,
 * which together constitute an instance of a TNEP service parameter record.
 *
 * Use the macro @ref NFC_NDEF_TNEP_RECORD_DESC to access the
 * NDEF TNEP record descriptor instance. Use name to access a
 * TNEP Service Parameter description.
 *
 * @param[in] _name Name of the created record descriptor instance.
 * @param[in] _tnep_version TNEP Version.
 * @param[in] _uri_length Service name uri length in bytes.
 * @param[in] _uri Service name uri.
 * @param[in] _mode TNEP communication mode.
 * @param[in] _min_time Minimum waiting time.
 * @param[in] _max_time_ext Maximum number of waiting time extensions.
 * @param[in] _max_message_size Maximum NDEF message size in bytes.
 */
#define NFC_TNEP_SERIVCE_PARAM_RECORD_DESC_DEF(_name, _tnep_version,          \
					       _uri_length, _uri,             \
					       _mode, _min_time,              \
					       _max_time_ext,                 \
					       _max_message_size)             \
	struct nfc_ndef_tnep_rec_svc_param _name = {                          \
		.version = _tnep_version,                                     \
		.uri_length = _uri_length,                                    \
		.uri = _uri,                                                  \
		.communication_mode = _mode,                                  \
		.min_time = _min_time,                                        \
		.max_time_ext = _max_time_ext,                                \
		.max_size = _max_message_size,                                \
	};                                                                    \
									      \
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(_name, TNF_WELL_KNOWN,               \
					 0, 0,                                \
					 nfc_ndef_tnep_rec_type_svc_param,    \
					 NFC_NDEF_TNEP_REC_TYPE_LEN,          \
					 nfc_ndef_tnep_rec_svc_param_payload, \
					 &(_name))

/**
 * @brief Macro for accessing the NFC NDEF TNEP record descriptor
 *        instance that was created with @ref NFC_NDEF_TNEP_RECORD_DESC.
 */

#define NFC_NDEF_TNEP_RECORD_DESC(_name) NFC_NDEF_GENERIC_RECORD_DESC(_name)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_NDEF_TNEP_REC_H_ */
