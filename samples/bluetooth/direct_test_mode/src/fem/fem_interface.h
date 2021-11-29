/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FEM_INTERFACE_H_
#define FEM_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "fem.h"

#include <hal/nrf_radio.h>

/**@brief The API structure of the front-end module interface.
 */
struct fem_interface_api {
	int (*power_up)(void);
	int (*power_down)(void);
	int (*tx_configure)(uint32_t activate_event, uint32_t deactivate_event,
			    uint32_t active_delay);
	int (*rx_configure)(uint32_t activate_event, uint32_t deactivate_event,
			    uint32_t active_delay);
	void (*txrx_configuration_clear)(void);
	int (*txrx_stop)(void);
	int (*tx_gain_set)(uint32_t gain);
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

/**@brief Function for getting a radio ramp-up time in the transmit mode.
 *
 * @param[in] fast The radio is in the fast ramp-up mode.
 * @param[in] mode Radio mode.
 *
 * @retval Radio ramp-up time in microseconds.
 */
uint32_t fem_radio_tx_ramp_up_delay_get(bool fast, nrf_radio_mode_t mode);

/**@brief Function for getting the radio ramp-up time in a receive mode.
 *
 * @param[in] fast The radio is in the fast ramp-up mode.
 * @param[in] mode Radio mode.
 *
 * @retval Radio ramp-up time in microseconds.
 */
uint32_t fem_radio_rx_ramp_up_delay_get(bool fast, nrf_radio_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* FEM_INTERFACE_H_ */
