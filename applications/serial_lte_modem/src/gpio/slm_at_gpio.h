/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_GPIO_
#define SLM_AT_GPIO_

/**@file slm_at_gpio.h
 *
 * @brief Vendor-specific AT command for GPIO service.
 * @{
 */

/**
 * @brief Initialize GPIO AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_gpio_init(void);

/**
 * @brief Uninitialize GPIO AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_gpio_uninit(void);

/** @} */

#endif /* SLM_AT_GPIO_ */
