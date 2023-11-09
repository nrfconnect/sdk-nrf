/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_NRF_MODEM_AT_H_
#define MOCK_NRF_MODEM_AT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file mock_nrf_modem_at.h
 * @brief Public APIs for the custom nrf_modem_at mock.
 * @defgroup mock_nrf_modem_at Custom mock for nrf_modem_at
 * @{
 */

/**
 * @brief Initialize mock variables.
 *
 * This is intended to be used in setUp() function.
 */
void mock_nrf_modem_at_Init(void);
/**
 * @brief Verify that mock functions where executed as expected.
 *
 * This is intended to be used in tearDown() function.
 */
void mock_nrf_modem_at_Verify(void);

#if defined(CONFIG_MOCK_NRF_MODEM_AT_PRINTF)

/**
 * @brief Mock for nrf_modem_at_printf().
 *
 * This function will verify that the string produced from @ref fmt using vsprintf()
 * with variable arguments matches the expected string.
 *
 * @param[in] fmt Format.
 */
int __cmock_nrf_modem_at_printf(const char *fmt, ...);

/**
 * @brief Sets expected parameter and return value.
 *
 * @param[in] fmt Expected string. This is not the format with specifiers but the
 *                expected string after applying variable arguments with vsprintf().
 * @param[in] return_value Value returned by the mock.
 */
void __mock_nrf_modem_at_printf_ExpectAndReturn(const char *fmt, int return_value);

#endif /* CONFIG_MOCK_NRF_MODEM_AT_PRINTF */

#if defined(CONFIG_MOCK_NRF_MODEM_AT_SCANF)

/**
 * @brief Mock for nrf_modem_at_scanf().
 *
 * @param[in] cmd Command.
 * @param[in] fmt Format.
 */
int __cmock_nrf_modem_at_scanf(const char *cmd, const char *fmt, ...);

/**
 * @brief Set expected parameter and return values for non-variable arguments.
 *
 * Output values of variable arguments for the associated nrf_modem_at_scanf() call
 * must be set after calling this function by using
 * __mock_nrf_modem_at_scanf_ReturnVarg_*() functions before calling this
 * function again.
 *
 * @param[in] cmd Expected command.
 * @param[in] fmt Expected format.
 * @param[in] return_value Value returned by the mock.
 */
void __mock_nrf_modem_at_scanf_ExpectAndReturn(
	const char *cmd, const char *fmt, int return_value);

/**
 * @brief Set next variable argument to be a signed integer output parameter.
 */
void __mock_nrf_modem_at_scanf_ReturnVarg_int(int value);

/**
 * @brief Set next variable argument to be a 32-bit signed integer output parameter.
 */
void __mock_nrf_modem_at_scanf_ReturnVarg_int32(int32_t value);

/**
 * @brief Set next variable argument to be a 16-bit signed integer parameter.
 */
void __mock_nrf_modem_at_scanf_ReturnVarg_int16(int16_t value);

/**
 * @brief Set next variable argument to be a 32-bit unsigned integer output parameter.
 */
void __mock_nrf_modem_at_scanf_ReturnVarg_uint32(uint32_t value);

/**
 * @brief Set next variable argument to be a 16-bit unsigned integer output parameter.
 */
void __mock_nrf_modem_at_scanf_ReturnVarg_uint16(uint16_t value);

/**
 * @brief Set next variable argument to be a string output parameter.
 */
void __mock_nrf_modem_at_scanf_ReturnVarg_string(char *value);

#endif /* CONFIG_MOCK_NRF_MODEM_AT_SCANF */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* MOCK_NRF_MODEM_AT_H_ */
