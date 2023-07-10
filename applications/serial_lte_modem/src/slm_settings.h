/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef SLM_SETTINGS_
#define SLM_SETTINGS_

/** @file slm_settings.h
 *
 * @brief Utility functions for serial LTE modem settings.
 * @{
 */

/**
 * @brief Loads the SLM settings from NVM.
 *
 * @retval 0 on success, nonzero otherwise.
 */
int slm_settings_init(void);

/** @brief Sets the FOTA settings to their default values. */
void slm_settings_fota_init(void);

/**
 * @brief Saves the FOTA settings to NVM.
 *
 * @retval 0 on success, nonzero otherwise.
 */
int slm_settings_fota_save(void);

/**
 * @brief Saves the UART settings to NVM.
 *
 * @retval 0 on success, nonzero otherwise.
 */
int slm_settings_uart_save(void);

/** @} */
#endif
