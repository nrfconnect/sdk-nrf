/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file mpsl_cx_nrf700x.h
 *
 * @defgroup mpsl_cx_nrf700x Multiprotocol Service Layer nRF700x Coexistence
 *
 * @brief MPSL nRF700x Coexistence
 * @{
 */

#ifndef MPSL_CX_NRF700X__
#define MPSL_CX_NRF700X__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Enables or disables nRF700x Coexistence Interface.
 *
 * The nRF700x Coexistence Interface is enabled by default. Depending on the current state of
 * GRANT pin of the nRF700x, clients of MPSL CX APIs will either be granted or denied access
 * to RF when the interface is enabled.
 *
 * Whenever nRF700x enters shutdown mode, the nRF700x Coexistence Interface must be disabled
 * by calling this function with @p enable equal @c false. Otherwise, all clients of MPSL CX APIs
 * will unconditionally be denied access to RF until nRF700x leaves shutdown mode.
 *
 * Once the nRF700x Coexistence Interface is disable, it can only be enabled again by calling
 * this function with @p enable equal @c true. Since the interface is enabled by default, unless
 * the application disabled it there is no need to call this function to have the nRF700x
 * Coexistence Interface enabled.
 *
 * @note This function is blocking. When it returns, the caller can assume the interface is already
 * in requested state.
 *
 * @param[in]  enable  Indicates if the nRF700x Coexistence Interface should be enabled or disabled.
 */
void mpsl_cx_nrf700x_set_enabled(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* MPSL_CX_NRF700X__ */

/**@} */
