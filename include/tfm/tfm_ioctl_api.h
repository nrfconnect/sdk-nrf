/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief TFM IOCTL API header.
 */


#ifndef TFM_IOCTL_API_H__
#define TFM_IOCTL_API_H__

/**
 * @defgroup tfm_ioctl_api TFM IOCTL API
 * @{
 *
 */

#include <limits.h>
#include <stdint.h>
#include <tfm_api.h>
#include <tfm_platform_api.h>
#include <hal/nrf_gpio.h>

/* Include core IOCTL services */
#include <tfm_ioctl_core_api.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Board specific IOCTL services can be added here */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* TFM_IOCTL_API_H__ */
