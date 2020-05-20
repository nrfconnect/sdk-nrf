/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef DTM_HW_H_
#define DTM_HW_H_

#include <stdbool.h>
#include <zephyr/types.h>

#include <nrfx/hal/nrf_radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Function for validating tx power and radio mode settings.
 * @param[in] tx_power    TX power for transmission test.
 * @param[in] radio_mode  Radio mode value.
 *
 * @retval true  If validation was successful
 * @retval false Otherwise
 */
bool dtm_hw_radio_validate(nrf_radio_txpower_t tx_power,
			   nrf_radio_mode_t radio_mode);

/**@brief Function for checking if Radio operates in Long Range mode.
 * @param[in] radio_mode  Radio mode value.
 *
 * @retval true  If Long Range Radio mode is set
 * @retval false Otherwise
 */
bool dtm_hw_radio_lr_check(nrf_radio_mode_t radio_mode);

#ifdef __cplusplus
}
#endif

#endif /* DTM_HW_H_ */
