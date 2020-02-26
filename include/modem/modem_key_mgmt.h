/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file modem_key_mgmt.h
 *
 * @defgroup modem_key_mgmt nRF91 Modem Key Management
 * @{
 */
#ifndef MODEM_KEY_MGMT_H__
#define MODEM_KEY_MGMT_H__

#include <stdbool.h>
#include <stdint.h>

#include <nrf_socket.h>

/**@brief Credential types. */
enum modem_key_mgnt_cred_type {
	MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
	MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
	MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
	MODEM_KEY_MGMT_CRED_TYPE_PSK,
	MODEM_KEY_MGMT_CRED_TYPE_IDENTITY
};

/**@brief Write or update a credential in persistent storage.
 *
 * This function will store the credential and associate it with the given
 * security tag, which can later be used to access it.
 *
 * @note If used when the LTE link is active, the function will return
 *	 an error and the key will not be written.
 *
 * @param[in] sec_tag	Security tag to associate with this credential.
 * @param[in] buf	Buffer containing the credential data.
 * @param[in] len	Length of the buffer.
 *
 * @retval 0		On success.
 * @retval -EINVAL	Invalid parameters.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -ENOMEM	Not enough memory to store the credential.
 * @retval -EACCES	Th operation failed because the LTE link is active.
 * @retval -ENOENT	The security tag could not be written.
 * @retval -EPERM	Insufficient permissions.
 * @retval -EIO		Internal error.
 */
int modem_key_mgmt_write(nrf_sec_tag_t sec_tag,
			 enum modem_key_mgnt_cred_type cred_type,
			 const void *buf, u16_t len);

/**@brief Read a credential from persistent storage.
 *
 * @param[in]		sec_tag		The crediantial security tag.
 * @param[in]		cred_type	Type of credential being read.
 * @param[out]		buf		Buffer to read the credential into.
 * @param[in,out]	len		Length of the buffer.
 *
 * @retval 0		On success.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -ENOMEM	If provided buffer is too small for result data.
 *			The required buffer size is written to @param len.
 * @retval -ENOENT	No credential associated with the given
 *			@p sec_tag and @p cred_type.
 * @retval -EPERM	Insufficient permissions.
 * @retval -EIO		Internal error.
 */
int modem_key_mgmt_read(nrf_sec_tag_t sec_tag,
			enum modem_key_mgnt_cred_type cred_type, void *buf,
			u16_t *len);

/**@brief Delete a credential from persistent storage.
 *
 * @note If used when the LTE link is active, the function will return
 *	 an error and the key will not be written.
 *
 * @param[in] sec_tag	The credential security tag.
 * @param[in] cred_type	Type of credential being deleted.
 *
 * @retval 0		On success.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -EACCES	The operation failed because the LTE link is active.
 * @retval -ENOENT	No credential associated with the given
 *			@p sec_tag and @p cred_type.
 * @retval -EPERM	Insufficient permissions.
 * @retval -EIO		Internal error.
 */
int modem_key_mgmt_delete(nrf_sec_tag_t sec_tag,
			  enum modem_key_mgnt_cred_type cred_type);

/**@brief Set permissions for a credential in persistent storage.
 *
 * @param[in] sec_tag		The credential security tag.
 * @param[in] cred_type		The credential type.
 * @param[in] perm_flags	Permission flags. Not yet implemented.
 *
 * @retval 0		On success.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -ENOENT	No credential associated with the given
 *			@p sec_tag and @p cred_type.
 * @retval -EPERM	Insufficient permissions.
 * @retval -EIO		Internal error.
 */
int modem_key_mgmt_permission_set(nrf_sec_tag_t sec_tag,
				  enum modem_key_mgnt_cred_type cred_type,
				  u8_t perm_flags);

/**@brief Check if a credential exists in persistent storage.
 *
 * @param[in]  sec_tag		The security tag to search for.
 * @param[out] exists		Whether the credential exists.
 *				Only valid if the operation is successful.
 * @param[out] perm_flags	The permission flags of the credential.
 *				Only valid if the operation is successful
 *				and @param exists is true.
 *				Not yet implemented.
 *
 * @retval 0		On success.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -EPERM	Insufficient permissions.
 * @retval -EIO		Internal error.
 */
int modem_key_mgmt_exists(nrf_sec_tag_t sec_tag,
			  enum modem_key_mgnt_cred_type cred_type,
			  bool *exists, u8_t *perm_flags);

#endif /* MODEM_KEY_MGMT_H__ */
/**@} */
