/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_PLATFORM_INTERNAL_H_
#define NFC_PLATFORM_INTERNAL_H_

/**@file
 * @defgroup nfc_platform_internal Platform-specific module for NFC
 * @{
 * @brief Internal part of the NFC platform.
 */

#include <nfc_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Function for initializing internal nfc_platform layer.
 *
 *  This internal function initializes callback scheduling mechanism
 *  used by selected NFC configuration.
 *
 *  This function should not be used directly.
 *
 *  @param[in] cb_rslv Pointer to the callback resolution function.
 *
 *  @retval 0 If the operation was successful.
 *  @retval -ENVAL Invalid callback resolution function pointer.
 */
int nfc_platform_internal_init(nfc_lib_cb_resolve_t cb_rslv);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_PLATFORM_INTERNAL_H_ */
