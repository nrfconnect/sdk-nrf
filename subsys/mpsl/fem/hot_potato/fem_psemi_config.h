/* Copyright (c) 2024, Nordic Semiconductor ASA
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

#ifndef FEM_PSEMI_CONFIG_H__
#define FEM_PSEMI_CONFIG_H__

#include <stdint.h>
#include <stdbool.h>
#include <mpsl_fem_config_common.h>
#include <nrf.h>

#if defined(NRF54L_SERIES)
// #include "../../../../../dragoon/mpsl/libs/fem/include_private/mpsl_fem_task.h"
#include "mpsl_fem_task.h"
#endif

/** Number of ATT pins used by FEM. */
#define FEM_ATT_PINS_NUMBER 4

/** Maximal value of FEM gain. */
#define FEM_MAX_GAIN 20

/** Minimal value of FEM gain. */
#define FEM_MIN_GAIN 5

/** @brief Configuration parameters for the Front End Module Simple GPIO variant.
 *
 *  A Simple GPIO Front End Module may be used with all Front End Modules
 *  which use one wire for a Power Amplifier (PA) and one wire for a Linear
 *  Noise Amplifier (LNA).
 */
typedef struct {
	/** Configration structure of the Simple GPIO Front End Module. */
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

#if defined(NRF52_SERIES)
	/** Array of PPI channels which need to be provided to Front End Module to operate. */
	uint8_t ppi_channels[2];
#elif defined(NRF54L_SERIES)
	/** Array of DPPI channels which need to be provided to Front End Module to operate. */
	uint8_t dppi_channels[3];
	/** Number of EGU instance for which @c egu_channels apply. */
	uint8_t egu_instance_no;
	/** Array of EGU channels (belonging to EGU instance number @c egu_instance_no) which
	 *  need to be provided to Front End Module to operate. */
	uint8_t egu_channels[3];
	mpsl_fem_task_t pa_task[2];
	mpsl_fem_task_t lna_task[2];
#else
#error "Unsupported platform"
#endif

} fem_psemi_interface_config_t;

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
int32_t fem_psemi_interface_config_set(fem_psemi_interface_config_t const *const p_config);

#endif // FEM_PSEMI_CONFIG_H__
