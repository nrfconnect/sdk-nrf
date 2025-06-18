/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_CTRL_PIN_
#define SLM_CTRL_PIN_

/** @file slm_ctrl_pin.h
 *
 * @brief Control pin handling functions for serial LTE modem
 * @{
 */

/**
 * @brief Enter idle.
 */
void slm_ctrl_pin_enter_idle(void);

/**
 * @brief Enter sleep.
 */
void slm_ctrl_pin_enter_sleep(void);

/**
 * @brief Enter sleep without uninitializing AT host.
 */
void slm_ctrl_pin_enter_sleep_no_uninit(void);

/**
 * @brief nRF91 Series SiP enters System OFF mode.
 */
void slm_ctrl_pin_enter_shutdown(void);

/**
 * @brief Temporarily sets the indicate pin high.
 *
 * @retval 0 on success, nonzero otherwise.
 */
int slm_ctrl_pin_indicate(void);

/**
 * @brief Initialize SLM control pin module.
 *
 * @retval 0 on success, nonzero otherwise.
 */
int slm_ctrl_pin_init(void);

/**
 * @brief Initialize SLM control pins, that is, power and indicate pins.
 */
void slm_ctrl_pin_init_gpios(void);

/** @} */

#endif /* SLM_CTRL_PIN_ */
