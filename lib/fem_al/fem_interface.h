/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FEM_INTERFACE_H_
#define FEM_INTERFACE_H_

/**
 * @brief Front-end module interface definition.
 *        This file defines interface for Front-end module specific behaviors which are not covered
 *        by the MPSL FEM API. This API is intended to be used to implement additional features
 *        specific for given Front-end module.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "fem_al/fem_al.h"

#include <hal/nrf_radio.h>

/**@brief The API structure of the front-end module interface.
 */
struct fem_interface_api {
	int (*power_up)(void);
	int (*power_down)(void);
	int (*tx_gain_validate)(uint32_t gain);
	uint32_t (*tx_default_gain_get)(void);
	uint32_t (*default_active_delay_calculate)(bool rx, nrf_radio_mode_t mode);
	int (*antenna_select)(enum fem_antenna ant);
};

/**@brief Set an API for one of the FEM implementation.
 *
 * @param[in] api The structure containing the front-end module API implementation.
 *
 * @retval 0       An API set successfully
 * @retval -EINVAL Invalid parameter provided.
 */
int fem_interface_api_set(const struct fem_interface_api *api);

#ifdef __cplusplus
}
#endif

#endif /* FEM_INTERFACE_H_ */
