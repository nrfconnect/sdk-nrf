/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MODEM_ATTEST_TOKEN_H__
#define MODEM_ATTEST_TOKEN_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file modem_attest_token.h
 *
 * @brief Modem attestation token and parsing.
 * @defgroup modem_attest_token Modem attestation token
 * @{
 *
 */

/** @brief Base64url attestation and COSE strings */
struct nrf_attestation_token {
	/** NULL terminated base64url attestation string buffer */
	char *attest;
	/** Size of the attestation buffer */
	size_t attest_sz;

	/** NULL terminated base64url COSE string buffer */
	char *cose;
	/** Size of the COSE buffer */
	size_t cose_sz;
};

enum nrf_id_srvc_msg_type {
	NRF_ID_SRVC_MSG_TYPE_INVALID = -1,
	NRF_ID_SRVC_MSG_TYPE_ID_V1 = 1,
	NRF_ID_SRVC_MSG_TYPE_PROV_RESP_V1 = 5,
	NRF_ID_SRVC_MSG_TYPE_PUB_KEY_V2 = 8,
	NRF_ID_SRVC_MSG_TYPE_CSR_V2 = 9,
};

enum nrf_device_type {
	NRF_DEVICE_TYPE_INVALID = -1,
	NRF_DEVICE_TYPE_9160_SIAA = 1,
	NRF_DEVICE_TYPE_9160_SIBA = 2,
	NRF_DEVICE_TYPE_9160_SICA = 3,
};

#define NRF_UUID_BYTE_SZ	16
#define NRF_DEVICE_UUID_SZ	NRF_UUID_BYTE_SZ
#define NRF_MODEM_FW_UUID_SZ	NRF_UUID_BYTE_SZ
#define NRF_ATTEST_NONCE_SZ	16

/* UUID v4 format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx */
#define NRF_UUID_V4_STR_LEN		((NRF_UUID_BYTE_SZ * 2) + 4)
#define NRF_DEVICE_UUID_STR_LEN		NRF_UUID_V4_STR_LEN
#define NRF_MODEM_FW_UUID_STR_LEN	NRF_UUID_V4_STR_LEN

/** @brief Parsed attestation token data */
struct nrf_attestation_data {
	enum nrf_id_srvc_msg_type msg_type;
	enum nrf_device_type dev_type;

	char device_uuid[NRF_DEVICE_UUID_SZ];
	char fw_uuid[NRF_MODEM_FW_UUID_SZ];
	char nonce[NRF_ATTEST_NONCE_SZ];
};

/** @brief Device UUID string (UUID v4 format) */
struct nrf_device_uuid {
	char str[NRF_DEVICE_UUID_STR_LEN + 1];
};

/** @brief Modem firmware UUID string (UUID v4 format) */
struct nrf_modem_fw_uuid {
	char str[NRF_MODEM_FW_UUID_STR_LEN + 1];
};

/**
 * @brief Gets the device attestation token from the modem. If successful,
 * the base64url attestation string and base64url COSE string will be stored
 * in the supplied struct.
 * This function will allocate memory for the strings if buffers are not
 * provided by the user.  In that case, the user is responsible for freeing
 * the memory by calling @ref modem_attest_token_free.
 *
 * @param[in,out] token Pointer to struct containing attestation token strings.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int modem_attest_token_get(struct nrf_attestation_token *const token);

/**
 * @brief Frees the memory allocated by @ref modem_attest_token_get.
 *
 * @param[in] token Pointer to attestation token.
 */
void modem_attest_token_free(struct nrf_attestation_token *const token);

/**
 * @brief Parses attestation token.
 *
 * @param[in] token_in Pointer to struct containing attestation token strings.
 * @param[out] data_out Pointer to struct containing parsed attestation data.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int modem_attest_token_parse(struct nrf_attestation_token const *const token_in,
			     struct nrf_attestation_data *const data_out);

/**
 * @brief Gets the device and/or modem firmware UUID from the modem
 * and returns it as a NULL terminated string in the supplied struct(s).
 *
 * @param[out] dev Pointer to struct containing device UUID string.
 *                 Can be NULL if UUID is not wanted.
 * @param[out] mfw Pointer to struct containing modem fw UUID string.
 *                 Can be NULL if UUID is not wanted.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int modem_attest_token_get_uuids(struct nrf_device_uuid *dev,
				 struct nrf_modem_fw_uuid *mfw);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* MODEM_ATTEST_TOKEN_H__ */
