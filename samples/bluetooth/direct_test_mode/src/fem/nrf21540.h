/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF21540_H_
#define NRF21540_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <hal/nrf_radio.h>

/**@brief nrf21540 Front-End-Module maximum gain register value */
#define NRF21540_TX_GAIN_Max 31

/**@brief Settling time from state PG to RX.
 */
#define NRF21540_PG_RX_TIME_US 13

/**@brief Settling time from state PG to TX.
 */
#define NRF21540_PG_TX_TIME_US 13

/**@brief The time between activating the PDN and asserting the RX_EN/TX_EN.
 */
#define NRF21540_PD_PG_TIME_US 18

/**@brief Value used as event zero-address - for immediate function execution.
 *        This is useful in functions with 'active_event' input parameter.
 */
#define NRF21540_EXECUTE_NOW ((uint32_t) 0)

/**@brief Value used as event zero-address - for no-execute the event action.
 *        This is useful in functions with 'deactive_event' input parameter.
 */
#define NRF21540_EVENT_UNUSED ((uint32_t) 0)

/**@brief nRF21540 antenna outputs.
 *
 * @note Read more in the Product Specification.
 */
enum nrf21540_ant {
	/** Antenna 1 output. */
	NRF21540_ANT1,

	/** Antenna 2 output. */
	NRF21540_ANT2
};

/**@brief Initialize nrf21540
 *
 * This function initializes modules needed by the nRF21540 without
 * powering it up.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf21540_init(void);

/*@brief Power-up nRF21540.
 *
 * The function is synchronous and waits @ref NRF21540_PD_PG_TIME_US for the
 * device to be activated.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf21540_power_up(void);

/**@brief Switch nRF21540 to the Power Down state.
 *
 * This function switches off the Front End Module. The gain setting
 * will be reset to the default after calling this function. It can be called
 * after nrf21540 configuration is cleared by
 * @ref nrf21540_txrx_configuration_clear. The device will turn off after 10 us.
 * This function doesn't wait that settling time.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf21540_power_down(void);

/**@brief Chooses one of two physical antenna outputs.
 *
 * @param[in] ant Selected antenna output
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf21540_antenna_select(enum nrf21540_ant ant);

/**@brief Configures Tx gain of the nRF21540 in arbitrary units.
 *
 * Tx gain value is set by the SPI interface in blocking mode.
 *
 * @param[in] gain Tx gain in arbitrary units. The gain value must be between
 *                 0 - @ref NRF21540_TX_GAIN_Max.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf21540_tx_gain_set(uint8_t gain);

/**@brief Set nRF21540 to TX mode.
 *
 * @note Transition from the Program State to the Transmit State takes
 *       @ref NRF21540_PG_TX_TIME_US microseconds and is triggered by
 *       the TX_EN pin. Suitable timing can be achieved by the @p
 *       active_delay parameters. This function triggers the
 *       @ref NRF_RADIO_TASK_TXEN task when active_event occurs or triggers it
 *       immediately when @ref NRF21540_EXECUTE_NOW is used.
 *
 * @param[in] active_event An event that triggers start of procedure - this
 *                         event will be connected to appropriate PPI channel.
 *                         @ref NRF21540_EXECUTE_NOW value causes start
 *                         procedure immediately.
 * @param[in] deactive_event An event that triggers deactivation of the Tx -
 *                           this event will be connected to appropriate PPI
 *                           channel.
 * @param[in] active_delay Tx PA activation delay between
 *                         @ref NRF_RADIO_TASK_TXEN and Tx pin setting.
 *
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf21540_tx_configure(uint32_t active_event, uint32_t deactive_event,
			  uint32_t active_delay);

/**@brief Set nRF21540 to RX mode.
 *
 * @note Transition from the Program State to the Receive State takes
 *       @ref NRF21540_PG_RX_TIME_US microseconds and is triggered by
 *       the RX_EN pin. Suitable timing can be achieved by the @p
 *       active_delay parameters. This function triggers the
 *       NRF_RADIO_TASK_RXEN task when active_event occurs or triggers it
 *       immediately when @ref NRF21540_EXECUTE_NOW is used.
 *
 * @param[in] active_event An event that triggers start of procedure - this
 *                         event will be connected to appropriate PPI channel.
 *                         @ref NRF21540_EXECUTE_NOW value causes start
 *                         procedure immediately.
 * @param[in] deactive_event An event that triggers deactivation of the Rx -
 *                           this event will be connected to appropriate PPI
 *                           channel.
 * @param[in] active_delay Rx LNA activation delay between
 *                         @ref NRF_RADIO_TASK_RXEN and Rx pin setting.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf21540_rx_configure(uint32_t active_event, uint32_t deactive_event,
			  uint32_t active_delay);

/**@brief Stop nRF21540 RX/TX mode immediately.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf21540_txrx_stop(void);

/**@brief Clears up the configuration provided by the @nrf21540_tx_configure
 *        or @ref nrf21540_rx_configure.
 */
void nrf21540_txrx_configuration_clear(void);

/**@brief Calculate default nRF21540 delay between RX/TX GPIO pin activation and
 *        radio readiness for reception or transmition.
 *
 * @param[in] rx Calculate a delay for radio reception. If set to false
 *               calculate a delay for radio transmition.
 * @param[in] mode Radio data rate and modulation.
 *
 * @return The activation delay value in microseconds.
 */
uint32_t nrf21540_default_active_delay_calculate(bool rx,
						 nrf_radio_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* NRF21540_H_ */
