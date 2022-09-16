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
 * @brief Initialize mocking variables for nrf_modem_at_scanf() calls.
 *
 * This is intended to be used in setUp() function.
 */
void mock_nrf_modem_at_Init(void);
/**
 * @brief Verify that nrf_modem_at_scanf() calls where executed as expected.
 *
 * This is intended to be used in tearDown() function.
 */
void mock_nrf_modem_at_Verify(void);

/**
 * @brief Mock for nrf_modem_at_scanf().
 */
int __cmock_nrf_modem_at_scanf(const char *cmd, const char *fmt, ...);

/**
 * @brief Set expected parameter and return values for non-variable arguments.
 *
 * Output values of variable arguments for the associated nrf_modem_at_scanf() call
 * must be set after calling this function by using
 * __mock_nrf_modem_at_scanf_ReturnVarg_*() functions before calling this
 * function again.
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

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* MOCK_NRF_MODEM_AT_H_ */
