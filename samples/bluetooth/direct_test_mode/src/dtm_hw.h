/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DTM_HW_H_
#define DTM_HW_H_

#include <stdbool.h>
#include <zephyr/types.h>

#include <hal/nrf_radio.h>

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

/**@brief Function for getting minimum tx power.
 *
 * @retval Minimum tx power value.
 */
uint32_t dtm_hw_radio_min_power_get(void);

/**@brief Function for getting maximum tx power.
 *
 * @retval Maximum tx power value.
 */
uint32_t dtm_hw_radio_max_power_get(void);

/**@brief Function for getting power array size. This array contains all
 *        possible tx power values for given divice sorted in ascending
 *        order.
 *
 * @reval Size of the tx power array.
 */
size_t dtm_hw_radio_power_array_size_get(void);

/**@brief Function for getting tx power array. This array contains all
 *        possible tx power values for given divice sorted in ascending
 *        order.
 *
 * @retval Size of the tx power array.
 */
const uint32_t *dtm_hw_radio_power_array_get(void);

/**@brief Function for getting antenna pins array. This array contains
 *        all antenna pins data.
 *
 * @retval Size of the antenna pin array.
 */
size_t dtm_radio_antenna_pin_array_size_get(void);

/**@brief Function for getting antenna pins array size.
 *
 * @retval Pointer to the first element in antenna pins array.
 */
const uint32_t *dtm_hw_radion_antenna_pin_array_get(void);

#ifdef __cplusplus
}
#endif

#endif /* DTM_HW_H_ */
