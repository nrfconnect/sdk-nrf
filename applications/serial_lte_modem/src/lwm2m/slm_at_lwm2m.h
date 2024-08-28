/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_LWM2M_H_
#define SLM_AT_LWM2M_H_

/**
 * @brief Initialize the LwM2M AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_lwm2m_init(void);

/**
 * @brief Uninitialize the LwM2M AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_lwm2m_uninit(void);

#endif /* SLM_AT_LWM2M_H_ */
