/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FEM_H_
#define FEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <hal/nrf_radio.h>

/**@brief Value used as event zero-address - for immediate function execution.
 *        This is useful in functions with 'activate_event' input parameter.
 */
#define FEM_EXECUTE_NOW ((uint32_t) 0)

/**@brief Value used as event zero-address - for no-execute the event action.
 *        This is useful in functions with 'deactivate_event' input parameter.
 */
#define FEM_EVENT_UNUSED ((uint32_t) 0)

/**@brief The front-end module (FEM) antennas.
 */
enum fem_antenna {
	/** First antenna. */
	FEM_ANTENNA_1,

	/** Second antenna. */
	FEM_ANTENNA_2
};

/**@brief Power-up the front-end module.
 *
 * The function is synchronous and waits a specific time for the
 * front-end device to be activated.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fem_power_up(void);

/**@brief Power-down the front-end module.
 *
 * This function switches off the front-end module. The gain setting
 * will be reset to the default after calling this function. It can be called
 * after a FEM configuration is cleared by
 * @ref fem_txrx_configuration_clear. The device will turn off after its specific time.
 * This function does not block on that settling time and returns before the actual power-down.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fem_power_down(void);

/**@brief Set the front-end module to TX mode (PA).
 *
 * @note Transition from the Program State to the Transmit State takes
 *       a specific settle time for the device (in microseconds) and is triggered by
 *       the PA pin. Check your FEM specification to obtain that value.
 *       Suitable timing can be achieved by the @p active_delay parameters.
 *       This function triggers the @ref NRF_RADIO_TASK_TXEN task when activate_event
 *       occurs or triggers it immediately when @ref FEM_EXECUTE_NOW is used.
 *
 * @param[in] activate_event An event that triggers start of procedure - this
 *                           event will be connected to appropriate PPI channel.
 *                           @ref FEM_EXECUTE_NOW value starts the
 *                           procedure immediately.
 * @param[in] deactivate_event An event that triggers deactivation of the Tx -
 *                             this event will be connected to appropriate PPI
 *                             channel.
 * @param[in] activation_delay Tx PA activation delay between
 *                             @ref NRF_RADIO_TASK_TXEN and Tx pin setting.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fem_tx_configure(uint32_t activate_event, uint32_t deactivate_event,
		     uint32_t activation_delay);

/**@brief Set the front-end module to RX mode (LNA).
 *
 * @note Transition from the Program State to the Receive State takes
 *       specific settle time for device in microseconds and is triggered by
 *       the LNA pin. Check your FEM specification to obtain that value.
 *       Suitable timing can be achieved by the @p active_delay parameters.
 *       This function triggers the NRF_RADIO_TASK_RXEN task when activate_event
 *       occurs or triggers it immediately when @ref FEM_EXECUTE_NOW is used.
 *
 * @param[in] activate_event An event that triggers start of procedure - this
 *                           event will be connected to appropriate PPI channel.
 *                           @ref FEM_EXECUTE_NOW value causes start
 *                           procedure immediately.
 * @param[in] deactivate_event An event that triggers deactivation of the Rx -
 *                             this event will be connected to appropriate PPI
 *                             channel.
 * @param[in] activation_delay Rx LNA activation delay between
 *                             @ref NRF_RADIO_TASK_RXEN and Rx pin setting.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fem_rx_configure(uint32_t activate_event, uint32_t deactivate_event,
		     uint32_t activation_delay);

/**@brief Clears up the configuration provided by the @fem_tx_configure
 *        or @ref fem_rx_configure.
 */
void fem_txrx_configuration_clear(void);

/**@brief Stop the front-end module RX/TX mode immediately.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fem_txrx_stop(void);

/**@brief Configures Tx gain of the front-end module in arbitrary units.
 *
 * Tx gain value might be set by a synchronous interface like for example
 * the SPI interface in blocking mode.
 *
 * @param[in] gain Tx gain in arbitrary units.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fem_tx_gain_set(uint32_t gain);

/**@brief Calculate the default front-end module delay between PA/LNA GPIO pin activation and
 *        radio readiness for reception or transmission.
 *
 * @param[in] rx Calculate a delay for radio reception. If set to false
 *               calculate a delay for radio transmission.
 * @param[in] mode Radio data rate and modulation.
 *
 * @return The activation delay value in microseconds.
 */
uint32_t fem_default_active_delay_calculate(bool rx,
					    nrf_radio_mode_t mode);

/**@brief Chooses one of two physical antenna outputs.
 *
 * @param[in] ant Selected antenna output
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fem_antenna_select(enum fem_antenna ant);

#ifdef __cplusplus
}
#endif

#endif /* FEM_H_ */
