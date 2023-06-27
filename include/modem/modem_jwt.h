/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef MODEM_JWT_H__
#define MODEM_JWT_H__

#include <zephyr/types.h>
#include <modem/modem_attest_token.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file modem_jwt.h
 *
 * @brief Request a JWT from the modem.
 * @defgroup modem_jwt JWT generation
 * @{
 *
 */

/**@brief The type of key to be used for signing the JWT. */
enum jwt_key_type {
	JWT_KEY_TYPE_CLIENT_PRIV = 2,
	JWT_KEY_TYPE_ENDORSEMENT = 8,
};

/**@brief JWT signing algorithm */
enum jwt_alg_type {
	JWT_ALG_TYPE_ES256 = 0,
};

/** @brief JWT parameters required for JWT generation and pointer to generated JWT */
struct jwt_data {
	/** Modem sec tag to use for JWT signing */
	unsigned int sec_tag;
	/** Key type in the specified sec tag */
	enum jwt_key_type key;
	/** JWT signing algorithm */
	enum jwt_alg_type alg;

	/** Defines how long the JWT will be valid; in seconds (from generation).
	 * The 'iat' and 'exp' claims will be populated only if the modem has a
	 * valid date and time.
	 */
	uint32_t exp_delta_s;

	/**  NULL terminated 'sub' claim; the principal that is the subject of the JWT */
	const char *subject;
	/**  NULL terminated 'aud' claim; intended recipient of the JWT */
	const char *audience;

	/** Buffer to which the NULL terminated JWT will be copied.
	 * If a buffer is provided by the user, the size must also be set.
	 * If buffer is NULL, memory will be allocated and user must free memory
	 * when finished by calling @ref modem_jwt_free.
	 */
	char *jwt_buf;
	/** Size of the user provided buffer or size of the allocated buffer */
	size_t jwt_sz;
};

/**
 * @brief Generates a JWT using the supplied parameters. If successful,
 * the JWT string will be stored in the supplied struct.
 * This function will allocate memory for the JWT if the user does not
 * provide a buffer.  In that case, the user is responsible for freeing
 * the memory by calling @ref modem_jwt_free.
 *
 * Subject and audience fields may be NULL in which case those fields are left out
 * from generated JWT token.
 *
 * If sec_tag value is given as zero, JWT is signed with Nordic's own keys that
 * already exist in the modem.
 *
 * @param[in,out] jwt Pointer to struct containing JWT parameters and result.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int modem_jwt_generate(struct jwt_data *const jwt);

/**
 * @brief Gets the device and/or modem firmware UUID from the modem
 * and returns it as a NULL terminated string in the supplied struct(s).
 *
 * Uses internally @ref modem_jwt_generate and parses JWT token for "iss"
 * "jti" fields which contains given UUID values.
 *
 * @param[out] dev Pointer to struct containing device UUID string.
 *                 Can be NULL if UUID is not wanted.
 * @param[out] mfw Pointer to struct containing modem fw UUID string.
 *                 Can be NULL if UUID is not wanted.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int modem_jwt_get_uuids(struct nrf_device_uuid *dev,
			struct nrf_modem_fw_uuid *mfw);

/**
 * @brief Frees the JWT buffer allocated by @ref modem_jwt_generate.
 *
 * @param[in] jwt_buf Pointer to JWT buffer; see #jwt_data.
 */
void modem_jwt_free(char *const jwt_buf);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* MODEM_JWT_H__ */
