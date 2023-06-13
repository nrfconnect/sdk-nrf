/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_PROVISIONING_AT_H__
#define NRF_PROVISIONING_AT_H__

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum nrf_provisioning_at_cmee_state {
	NRF_PROVISIONING_AT_CMEE_STATE_DISABLE = 0,
	NRF_PROVISIONING_AT_CMEE_STATE_ENABLE = 1,
};

/**
 * @brief Retrieves an attestation token.
 *
 *
 * @param buff Buffer to store the token
 * @param size Size of the buffer
 *
 * @returns Zero on success, negative error code on failure.
 */
int nrf_provisioning_at_attest_token_get(void *const buff, size_t size);

/**
 * @brief Retrieves network time.
 *
 * @param buff Buffer to receive the time into.
 * @param size Buffer size.
 *
 * @returns  Zero on success, negative error code on failure
 * @retval A positive value On "ERROR", "+CME ERROR", and "+CMS ERROR" responses.
 *	    The type of error can be distinguished using @c nrf_modem_at_err_type.
 *	    The error value can be retrieved using @c nrf_modem_at_err.
 */
int nrf_provisioning_at_time_get(void *const time_buff, size_t size);

/**
 * @brief Checks if mobile termination error reports are enabled.
 *
 * @returns True if enabled, false if disabled
 */
bool nrf_provisioning_at_cmee_is_active(void);

/**
 * @brief Sets mobile termination error reporting to a given state.
 *
 * @param state State for error reporting.
 *
 * @returns  Zero on success, negative error code on failure.
 */
int nrf_provisioning_at_cmee_control(enum nrf_provisioning_at_cmee_state state);

/**
 * @brief Enables mobile termination error reporting state.
 *
 * @returns  Previous state
 */
bool nrf_provisioning_at_cmee_enable(void);

/**
 * @brief Send any AT command.
 *
 * @param resp Buffer to receive the response into.
 * @param resp_sz Buffer size.
 * @param cmd AT command to be send.
 *
 * @returns Zero on success, negative error code on failure or positive error code when CMEE code
 * needs to be checked.
 */
int nrf_provisioning_at_cmd(void *resp, size_t resp_sz, const char *cmd);

/**
 * @brief Delete credential.
 *
 * @param tag The security tag of the credential.
 * @param type The credential type.
 *
 * @returns  Zero on success, negative error code on failure.
 */
int nrf_provisioning_at_del_credential(int sec_tag, int type);

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_AT_H__ */
