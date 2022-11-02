/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MODEM_KEY_MGMT_H__
#define MODEM_KEY_MGMT_H__

#include <stdbool.h>
#include <stdint.h>

#include <nrf_socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file modem_key_mgmt.h
 *
 * @defgroup modem_key_mgmt nRF91 Modem Key Management
 * @{
 */

/**@brief Credential types. */
enum modem_key_mgmt_cred_type {
	MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
	MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
	MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
	MODEM_KEY_MGMT_CRED_TYPE_PSK,
	MODEM_KEY_MGMT_CRED_TYPE_IDENTITY
};

/**
 * @brief Write or update a credential in persistent storage.
 *
 * Store the credential and associate it with the given security tag,
 * which can later be used to access it.
 *
 * @note If used when the LTE link is active, the function will return
 *	 an error and the key will not be written.
 *
 * @param[in] sec_tag		Security tag to associate with this credential.
 * @param[in] cred_type		The credential type.
 * @param[in] buf		Buffer containing the credential data.
 * @param[in] len		Length of the buffer.
 *
 * @retval 0		On success.
 * @retval -EINVAL	Invalid parameters.
 * @retval -ENOMEM	Not enough memory to store the credential.
 * @retval -ENOENT	The security tag @p sec_tag is invalid.
 * @retval -EACCES	Access to credential not allowed.
 * @retval -EALREADY	Credential already exists (and can't be overwritten).
 * @retval -EPERM	Not permitted when the LTE link is active.
 * @retval -ECANCELED	Canceled because voltage is low (power off warning).
 */
int modem_key_mgmt_write(nrf_sec_tag_t sec_tag,
			 enum modem_key_mgmt_cred_type cred_type,
			 const void *buf, size_t len);

/**
 * @brief Delete a credential from persistent storage.
 *
 * @note If used when the LTE link is active, the function will return
 *	 an error and the key will not be written.
 *
 * @param[in] sec_tag		The security tag of the credential.
 * @param[in] cred_type		The credential type.
 *
 * @retval 0		On success.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -ENOENT	No credential associated with the given
 *			@p sec_tag and @p cred_type.
 * @retval -EACCES	Access to credential not allowed.
 * @retval -EPERM	Not permitted when the LTE link is active.
 * @retval -ECANCELED	Canceled because voltage is low (power off warning).
 */
int modem_key_mgmt_delete(nrf_sec_tag_t sec_tag,
			  enum modem_key_mgmt_cred_type cred_type);

/**
 * @brief Read a credential from persistent storage.
 *
 * @param[in]		sec_tag		The security tag of the credential.
 * @param[in]		cred_type	The credential type.
 * @param[out]		buf		Buffer to read the credential into.
 * @param[in,out]	len		Length of the buffer,
 *					length of the credential.
 *
 * @retval 0		On success.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -ENOMEM	Credential does not fit in @p buf.
 * @retval -ENOENT	No credential associated with the given
 *			@p sec_tag and @p cred_type.
 * @retval -EACCES	Access to credential not allowed.
 */
int modem_key_mgmt_read(nrf_sec_tag_t sec_tag,
			enum modem_key_mgmt_cred_type cred_type,
			void *buf, size_t *len);

/**
 * @brief Compare a credential with a credential in persistent storage.
 *
 * @param[in] sec_tag		The security tag of the credential.
 * @param[in] cred_type		The credential type.
 * @param[in] buf		Buffer to compare the credential to.
 * @param[in] len		Length of the buffer.

 * @retval 0		If the credentials match.
 * @retval 1		If the credentials do not match.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -ENOENT	No credential associated with the given
 *			@p sec_tag and @p cred_type.
 * @retval -EACCES	Access to credential not allowed.
 */
int modem_key_mgmt_cmp(nrf_sec_tag_t sec_tag,
		       enum modem_key_mgmt_cred_type cred_type,
		       const void *buf, size_t len);

/**
 * @brief Check if a credential exists in persistent storage.
 *
 * @param[in] sec_tag		The security tag to search for.
 * @param[in] cred_type		The credential type.
 * @param[out] exists		Whether the credential exists.
 *				Only valid if the operation is successful.
 *
 * @retval 0		On success.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -EACCES	Access to credential not allowed.
 */
int modem_key_mgmt_exists(nrf_sec_tag_t sec_tag,
			  enum modem_key_mgmt_cred_type cred_type,
			  bool *exists);

/**@} */

#ifdef __cplusplus
}
#endif

#endif /* MODEM_KEY_MGMT_H__ */
