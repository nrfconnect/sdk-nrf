/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * The RST file for this library can be found in doc/nrf/libraries/mpsl/mpsl_assert.rst.
 * Rendered documentation is available at
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/mpsl/mpsl_assert.html.
 */

/**
 * @file mpsl_assert.h
 *
 * @defgroup mpsl_assert Multiprotocol Service Layer assert
 *
 * @brief MPSL assert
 * @{
 */

#ifndef MPSL_ASSERT__
#define MPSL_ASSERT__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application-defined sink for the MPSL assertion mechanism.
 *
 * @param file  The file name where the assertion occurred.
 * @param line  The line number where the assertion occurred.
 */
void mpsl_assert_handle(char *file, uint32_t line);

#ifdef __cplusplus
}
#endif

#endif /* MPSL_ASSERT__ */

/**@} */
