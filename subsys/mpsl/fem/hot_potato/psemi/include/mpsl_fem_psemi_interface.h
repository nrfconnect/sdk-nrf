/* Copyright (c) 2025, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef MPSL_FEM_PSEMI_INTERFACE_H__
#define MPSL_FEM_PSEMI_INTERFACE_H__

#include <stdint.h>
#include <stdbool.h>
#include "mpsl_fem_config_common.h"

/** Number of ATT pins used by FEM. */
#define FEM_ATT_PINS_NUMBER 4

/** Number of DPPI channels used by FEM. */
#define DPPI_CHANNELS_NUMBER 3

/** Number of EGU channels used by FEM. */
#define EGU_CHANNELS_NUMBER 3

/** Maximal value of FEM gain. */
#define FEM_MAX_GAIN 20

/** Minimal value of FEM gain. */
#define FEM_MIN_GAIN 5

/** FEM gain that indicates FEM bypass. */
#define FEM_GAIN_BYPASS 0

/** FEM state. */
typedef enum {
	FEM_PSEMI_STATE_DISABLED = 0,
	FEM_PSEMI_STATE_LNA_ACTIVE = 1,
	FEM_PSEMI_STATE_PA_ACTIVE = 2,
	FEM_PSEMI_STATE_BYPASS = 3,
	FEM_PSEMI_STATE_AUTO = 4
} mpsl_fem_psemi_state_t;

/** @brief Configuration parameters for the Front End Module Simple GPIO variant.
 *
 *  A Simple GPIO Front End Module may be used with all Front End Modules
 *  which use one wire for a Power Amplifier (PA) and one wire for a Linear
 *  Noise Amplifier (LNA).
 */
typedef struct {
	/** Configuration structure of the Simple GPIO Front End Module. */
	struct {
		/** Time between the activation of the PA pin and the start of the radio
		 * transmission. Should be no bigger than Radio Ramp-Up time. */
		uint32_t pa_time_gap_us;
		/** Time between the activation of the LNA pin and the start of the radio reception.
		 *  Should be no bigger than Radio Ramp-Up time. */
		uint32_t lna_time_gap_us;
		/** Configurable LNA gain. Ignored if the amplifier is not supporting this feature.
		 */
		int8_t lna_gain_db;
	} fem_config;

	/** Power Amplifier pin configuration. */
	mpsl_fem_gpiote_pin_config_t pa_pin_config;
	/** Low Noise Amplifier pin configuration. */
	mpsl_fem_gpiote_pin_config_t lna_pin_config;
	/** Pins for controlling fem gain. Ignored if the amplifier is not supporting this feature.
	 */
	mpsl_fem_gpio_pin_config_t att_pins[FEM_ATT_PINS_NUMBER];

	bool bypass_ble_enabled; /**< Enable BLE bypass mode. */
	bool pa_ldo_switch_enabled; /**< Enable PA LDO power supply switching. */

#if defined(NRF54L_SERIES)
	/** Array of DPPI channels which need to be provided to Front End Module to operate. */
	uint8_t dppi_channels[DPPI_CHANNELS_NUMBER];
	/** Number of EGU instance for which @c egu_channels apply. */
	uint8_t egu_instance_no;
	/** Array of EGU channels (belonging to EGU instance number @c egu_instance_no) which
	 *  need to be provided to Front End Module to operate. */
	uint8_t egu_channels[EGU_CHANNELS_NUMBER];
#else
#error "Unsupported platform"
#endif

} mpsl_fem_psemi_interface_config_t;

/** @brief Configures the PA and LNA device interface.
 *
 * This function sets device interface parameters for the PA/LNA module.
 * The module can then be used to control PA or LNA (or both) through the given interface and
 * resources.
 *
 * The function also sets the PPI and GPIOTE channels to be configured for the PA/LNA interface.
 *
 * @param[in] p_config Pointer to the interface parameters for the PA/LNA device.
 *
 * @retval   0             PA/LNA control successfully configured.
 * @retval   -NRF_EPERM    PA/LNA is not available.
 *
 */
int32_t mpsl_fem_psemi_interface_config_set(mpsl_fem_psemi_interface_config_t const *const p_config);

/** @brief Set the state of the FEM.
 *
 * @param[in] state  State to be set.
 *
 * @retval   0             State has been set successfully.
 * @retval   -NRF_EINVAL   Provided @p state is invalid.
 */
int32_t mpsl_fem_psemi_state_set(mpsl_fem_psemi_state_t state);

/** @brief Get the state of the FEM.
 *
 * @return Current state of the FEM.
 */
mpsl_fem_psemi_state_t mpsl_fem_psemi_state_get(void);

#endif /* MPSL_FEM_PSEMI_INTERFACE_H__ */
