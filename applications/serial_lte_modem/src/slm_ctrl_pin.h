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

void slm_enter_idle(void);
void slm_enter_sleep(void);
void slm_enter_shutdown(void);

/** @brief Temporarily sets the indicate pin high. */
int slm_indicate(void);

int slm_ctrl_pin_init(void);
void slm_ctrl_pin_init_gpios(void);

/** @} */

#endif /* SLM_CTRL_PIN_ */
