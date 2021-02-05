/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief RAM section power down header.
 */

#ifndef RAM_PWRDN_H__
#define RAM_PWRDN_H__

/**
 * @defgroup ram_pwrdn RAM Power Down
 * @{
 * @brief Module for disabling unused sections of RAM.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Request powering down unused RAM sections.
 *
 * Not all devices support this feature.
 */
void power_down_unused_ram(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* RAM_PWRDN_H__ */
