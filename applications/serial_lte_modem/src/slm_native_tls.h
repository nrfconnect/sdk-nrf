/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
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

#include <zephyr/types.h>
#include <zephyr/net/tls_credentials.h>
#include <modem/modem_key_mgmt.h>
#include "slm_trap_macros.h"

#define MAX_SLM_SEC_TAG (INT_MAX/10)
#define MIN_SLM_SEC_TAG 0

/**
 * @brief Map SLM security tag to nRF security tag
 * When SLM native TLS is enabled, the credentials are mapped to
 * continuous security tags in modem and stored as Root CA certificate
 * so that the credentials can be read from modem.
 *
 * The available sec_tag in modem (0 – 2147483647) are divided by 10
 * to store mapped credentials:
 *   Root CA certificate (ASCII text) at sec_tag*10 + 0
 *   Client/Server certificate (ASCII text) at sec_tag*10 + 1
 *   Client/Server private key (ASCII text) at sec_tag*10 + 2
 *   Pre-shared Key (PSK) at sec_tag*10 + 3
 *   PSK identity (ASCII text) at sec_tag*10 + 4
 *   Public Key (ASCII text) at sec_tag*10 + 5
 *
 *   Currently PSK, PSK identity and Public Key are not supported.
 *
 * @param[in]  sec_tag SLM security tag of the credential
 * @param[in]  type TLS credential types
 *
 * @return Non-negative nRF security tag if successful,
 *         negative error code if failure.
 */
nrf_sec_tag_t slm_tls_map_sectag(sec_tag_t sec_tag, uint16_t type);

/**
 * @brief Load credential
 *
 * @param[in] sec_tag TLS reference
 *
 * @return 0 if successful, negative error code if failure.
 */
int slm_tls_loadcrdl(sec_tag_t sec_tag);

/**
 * @brief Unload credential
 *
 * @param[in] sec_tag TLS reference
 *
 * @return 0 if successful, negative error code if failure.
 */
int slm_tls_unloadcrdl(sec_tag_t sec_tag);

/** @} */

#endif /* SLM_NATIVE_TLS_ */
