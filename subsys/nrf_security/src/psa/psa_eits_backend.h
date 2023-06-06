/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef PSA_EITS_BACKEND_H
#define PSA_EITS_BACKEND_H

#include <psa/internal_trusted_storage.h>

/**
 * \brief create a new or modify an existing uid/value pair
 *
 * \param[in] uid           the identifier for the data
 * \param[in] data_length   The size in bytes of the data in `p_data`
 * \param[in] p_data        A buffer containing the data
 * \param[in] create_flags  The flags that the data will be stored with
 *
 * \return      A status indicating the success/failure of the operation
 *
 * \retval      PSA_SUCCESS                      The operation completed successfully
 * \retval      PSA_ERROR_NOT_PERMITTED          The operation failed because the provided `uid` value was already created with PSA_STORAGE_WRITE_ONCE_FLAG
 * \retval      PSA_ERROR_NOT_SUPPORTED          The operation failed because one or more of the flags provided in `create_flags` is not supported or is not valid
 * \retval      PSA_ERROR_INSUFFICIENT_STORAGE   The operation failed because there was insufficient space on the storage medium
 * \retval      PSA_ERROR_STORAGE_FAILURE        The operation failed because the physical storage has failed (Fatal error)
 * \retval      PSA_ERROR_INVALID_ARGUMENT       The operation failed because one of the provided pointers(`p_data`)
 *                                               is invalid, for example is `NULL` or references memory the caller cannot access
 */
psa_status_t psa_its_set_backend( psa_storage_uid_t uid, uint32_t data_length,
                                  const void *p_data,
                                  psa_storage_create_flags_t create_flags );

/**
 * \brief Retrieve the value associated with a provided uid
 *
 * \param[in] uid               The uid value
 * \param[in] data_offset       The starting offset of the data requested
 * \param[in] data_length       the amount of data requested (and the minimum allocated size of the `p_data` buffer)
 * \param[out] p_data           The buffer where the data will be placed upon successful completion
 * \param[out] p_data_length    The amount of data returned in the p_data buffer
 *
 *
 * \return      A status indicating the success/failure of the operation
 *
 * \retval      PSA_SUCCESS                  The operation completed successfully
 * \retval      PSA_ERROR_DOES_NOT_EXIST     The operation failed because the provided `uid` value was not found in the storage
 * \retval      PSA_ERROR_INVALID_SIZE       The operation failed because the data associated with provided uid is larger than `data_size`
 * \retval      PSA_ERROR_STORAGE_FAILURE    The operation failed because the physical storage has failed (Fatal error)
 * \retval      PSA_ERROR_INVALID_ARGUMENT   The operation failed because one of the provided pointers(`p_data`, `p_data_length`)
 *                                           is invalid. For example is `NULL` or references memory the caller cannot access.
 *                                           In addition, this can also happen if an invalid offset was provided.
 */
psa_status_t psa_its_get_backend( psa_storage_uid_t uid, uint32_t data_offset,
                                  uint32_t data_length, void *p_data,
                                  size_t *p_data_length );

/**
 * \brief Retrieve the metadata about the provided uid
 *
 * \param[in] uid           The uid value
 * \param[out] p_info       A pointer to the `psa_storage_info_t` struct that will be populated with the metadata
 *
 * \return      A status indicating the success/failure of the operation
 *
 * \retval      PSA_SUCCESS                  The operation completed successfully
 * \retval      PSA_ERROR_DOES_NOT_EXIST     The operation failed because the provided uid value was not found in the storage
 * \retval      PSA_ERROR_STORAGE_FAILURE    The operation failed because the physical storage has failed (Fatal error)
 * \retval      PSA_ERROR_INVALID_ARGUMENT   The operation failed because one of the provided pointers(`p_info`)
 *                                           is invalid, for example is `NULL` or references memory the caller cannot access
 */
psa_status_t psa_its_get_info_backend( psa_storage_uid_t uid,
                                       struct psa_storage_info_t *p_info );

/**
 * \brief Remove the provided key and its associated data from the storage
 *
 * \param[in] uid   The uid value
 *
 * \return  A status indicating the success/failure of the operation
 *
 * \retval      PSA_SUCCESS                  The operation completed successfully
 * \retval      PSA_ERROR_DOES_NOT_EXIST     The operation failed because the provided key value was not found in the storage
 * \retval      PSA_ERROR_NOT_PERMITTED      The operation failed because the provided key value was created with PSA_STORAGE_WRITE_ONCE_FLAG
 * \retval      PSA_ERROR_STORAGE_FAILURE    The operation failed because the physical storage has failed (Fatal error)
 */
psa_status_t psa_its_remove_backend( psa_storage_uid_t uid );

#endif /* PSA_EITS_BACKEND_H */
