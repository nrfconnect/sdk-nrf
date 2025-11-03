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

#ifndef MPSL_FEM_CONFIG_PSEMI_H__
#define MPSL_FEM_CONFIG_PSEMI_H__

#include <stdint.h>
#include <mpsl_fem_config_common.h>
#include "mpsl_fem_pins_common.h"
#include "mpsl_fem_task.h"

#include "include/mpsl_fem_psemi_interface.h"

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
	mpsl_fem_gpiote_pin_t pa_gpiote_pin;
	/** Low Noise Amplifier pin configuration. */
	mpsl_fem_gpiote_pin_t lna_gpiote_pin;

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
	mpsl_fem_task_t pa_task[2];
	mpsl_fem_task_t lna_task[2];
#else
#error "Unsupported platform"
#endif

} mpsl_fem_psemi_t;

#endif /* MPSL_FEM_CONFIG_PSEMI_H__ */
