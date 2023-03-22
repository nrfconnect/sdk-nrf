/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FEM_H_
#define FEM_H_

/**
 * @brief Front-end module API.
 *        This file is an abstraction for the MPSL FEM API and provides API for controlling
 *        front-end modules in samples.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <hal/nrf_radio.h>

/**@brief The front-end module (FEM) antennas.
 */
enum fem_antenna {
	/** First antenna. */
	FEM_ANTENNA_1,

	/** Second antenna. */
	FEM_ANTENNA_2
};

/**@brief Initialize the front-end module.
 *
 * @param[in] timer_instance Pointer to a 1-us resolution timer instance.
 * @param[in] compare_channel_mask Mask of the compare channels that can be used by
 *                                 the Front End Module to schedule its own tasks.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fem_init(NRF_TIMER_Type *timer_instance, uint8_t compare_channel_mask);

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

/**@brief Configure Tx gain of the front-end module in arbitrary units.
 *
 * @param[in] gain Tx gain in arbitrary units specific for used front-end module implementation.
 *                 For nRF21540 GPIO/SPI, this is a register value.
 *                 For nRF21540 GPIO, this is MODE pin value.
 *                 Check your front-end module product specification for gain value range.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fem_tx_gain_set(uint32_t gain);

/**@brief Get the default radio ramp-up time for reception or transmission with a given data rate
 *        and modulation.
 *
 * @param[in] rx Get a delay for radio reception. If set to false
 *               get a delay for radio transmission.
 * @param[in] mode Radio data rate and modulation.
 *
 * @return The activation delay value in microseconds.
 */
uint32_t fem_default_ramp_up_time_get(bool rx, nrf_radio_mode_t mode);

/**@brief Configure the front-end module to TX mode (PA).
 *
 * @param[in] ramp_up_time rump_up_time
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fem_tx_configure(uint32_t ramp_up_time);

/**@brief Configure the front-end module to RX mode (LNA).
 *
 * @param[in] ramp_up_time Radio ramp-up time.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fem_rx_configure(uint32_t ramp_up_time);

/**@brief Clear up the configuration provided by the @fem_tx_configure
 *        or @ref fem_rx_configure.
 */
int fem_txrx_configuration_clear(void);

/**@brief Stop the front-end module RX/TX mode immediately.
 */
void fem_txrx_stop(void);

/**@brief Chooses one of two physical antenna outputs.
 *
 * @param[in] ant Selected antenna output
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fem_antenna_select(enum fem_antenna ant);

/**@brief Get the radio ramp-up time in transmit mode.
 *
 * @param[in] fast The radio is in the fast ramp-up mode.
 * @param[in] mode Radio mode.
 *
 * @retval Radio ramp-up time in microseconds.
 */
uint32_t fem_radio_tx_ramp_up_delay_get(bool fast, nrf_radio_mode_t mode);

/**@brief Get the radio ramp-up time in receive mode.
 *
 * @param[in] fast The radio is in the fast ramp-up mode.
 * @param[in] mode Radio mode.
 *
 * @retval Radio ramp-up time in microseconds.
 */
uint32_t fem_radio_rx_ramp_up_delay_get(bool fast, nrf_radio_mode_t mode);

/**@brief Set the front-end module gain and returns output power to be set on the radio peripheral
 *        to get requested output power.
 *
 * This function calculates power value for RADIO peripheral register and
 * sets front-end module gain value.
 *
 * @note If the exact value of @p power cannot be achieved, this function attempts to use less
 *       power to not exceed the limits.
 *
 * @param[in] power TX power requested for transmission on air.
 * @param[out] radio_tx_power Tx power value to be set on the radio peripheral.
 * @param[in] freq_mhz Frequency in MHz. The output power is valid only for this frequency.
 *
 * @return The power in dBm that is achieved as device output power. It can be different from
 *         the value requested by @p power.
 */
int8_t fem_tx_output_power_prepare(int8_t power, int8_t *radio_tx_power, uint16_t freq_mhz);

/**@brief Check a real Tx output power, for the SoC with the front-end module for given parameters.
 *
 * Check if the requested output power can be achieved. This function returns the exact value or
 * if this value is not available for given frequency, the closest greater or lower value to the
 * requested output power.
 *
 * @param[in] power Tx power level to check.
 * @param[in] freq_mhz Frequency in MHz. The output power check is valid only for this frequency.
 * @param[in] tx_power_ceiling Flag to get the ceiling or floor of the requested transmit power
 *                             level.
 *
 * @return The real Tx output power in dBm, which can be achieved for the given frequency. It might
 *         be different than @p power and depending on @p tx_power_ceiling it can be greater or
 *         lower than the requested one.
 */
int8_t fem_tx_output_power_check(int8_t power, uint16_t freq_mhz, bool tx_power_ceiling);

/**@brief Get the minimum Tx output power for the SoC with the front-end module.
 *
 * @param[in] freq_mHz Frequency in MHz. The minimum output power is valid only for this frequency.
 *
 * @return The minimum output power in dBm.
 */
static inline int8_t fem_tx_output_power_min_get(uint16_t freq_mhz)
{
	/* Using INT8_MIN returns the minimum supported Tx output power value. */
	return fem_tx_output_power_check(INT8_MIN, freq_mhz, false);
}

/**@brief Get the maximum Tx output power for the SoC with the front-end module.
 *
 * @param[in] freq_mHz Frequency in MHz. The maximum output power is valid only for this frequency.
 *
 * @return The maximum output power in dBm.
 */
static inline int8_t fem_tx_output_power_max_get(uint16_t freq_mhz)
{
	/* Using INT8_MAX returns the maximum supported Tx output power value. */
	return fem_tx_output_power_check(INT8_MAX, freq_mhz, true);
}

/** @brief Get the front-end module default Tx gain.
 *
 * @return The front-end module default Tx gain value.
 */
uint32_t fem_default_tx_gain_get(void);

/** @brief Apply the workaround for the Errata 254 when appropriate.
 *
 * This workaround is applied only if used with the correct hardware configuration.
 *
 * @param[in] mode Radio mode.
 */
void fem_errata_254(nrf_radio_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* FEM_H_ */
