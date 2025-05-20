/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_NATIVE_TLS_
#define SLM_NATIVE_TLS_

/**@file slm_native_tls.h
 *
 * @brief Utility functions for serial LTE modem native TLS socket
 * @{
 */

#include <zephyr/net/tls_credentials.h>
#include "slm_trap_macros.h"

/**
 * @brief Store credential to Zephyr settings storage.
 *
 * @param[in] sec_tag sec_tag under which the credential is stored.
 * @param[in] type Type of the credential to be stored.
 * @param[in] data Pointer to the credential data.
 * @param[in] len Length of the credential data.
 *
 * @retval 0 If succeeded.
 * @retval -E2BIG if credentials under sec_tag would exceed SLM_NATIVE_TLS_CREDENTIAL_BUFFER_SIZE.
 * @retval -errno Negative errno for other failures.
 */
int slm_native_tls_store_credential(sec_tag_t sec_tag, uint16_t type, const void *data, size_t len);

/**
 * @brief List credential sec_tags and types from Zephyr settings storage.
 *
 * @retval 0 If succeeded.
 * @retval -errno Negative errno for failures.
 */
int slm_native_tls_list_credentials(void);

/**
 * @brief Load credentials from Zephyr settings storage to TLS credentials storage for Mbed TLS.
 *
 * @param[in] sec_tag sec_tag under which the credentials are loaded.
 *
 * @retval 0 If succeeded.
 * @retval -errno Negative errno for failures.
 */
int slm_native_tls_load_credentials(sec_tag_t sec_tag);

/**
 * @brief Delete credential from Zephyr settings storage.
 *
 * @param[in] sec_tag sec_tag under which the credential is deleted.
 * @param[in] type Type of the credential to be deleted.
 *
 * @retval 0 If succeeded.
 * @retval -errno Negative errno for failures.
 */
int slm_native_tls_delete_credential(sec_tag_t sec_tag, uint16_t type);

/** @} */

#endif /* SLM_NATIVE_TLS_ */
