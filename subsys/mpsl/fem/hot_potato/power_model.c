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

/**
 * @file
 * @brief This file implements a glue layer for FEM power model.
 */

#include <assert.h>
#include <mpsl_fem_power_model.h>

#include "models/power_map/vendor_radio_power_map.h"
#include "models/power_map/vendor_radio_power_limit.h"
#include "fem_psemi_common.h"

/** @brief Offset between 802.15.4 and tx_power channel numbering. */
#define CHANNEL_OFFSET (11u)

/** @brief Distance in MHz between consecutive 802.15.4 channels. */
#define CHANNEL_SPACING_MHZ (5u)

/** @brief Base frequency for 802.15.4 channels. */
#define CHANNEL_BASE_MHZ (2405u)

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

__WEAK bool vendor_diag_fem_enabled(void)
{
	return false;
}

__WEAK int8_t vendor_diag_radio_tx_power_get(void)
{
	return 0;
}

__WEAK int8_t vendor_diag_fem_gain_get(void)
{
	return 0;
}

static inline uint8_t freg_mhz_to_channel(uint16_t freq_mhz)
{
	if (freq_mhz < CHANNEL_BASE_MHZ) {
		freq_mhz = CHANNEL_BASE_MHZ;
	}

	return (freq_mhz - CHANNEL_BASE_MHZ) / CHANNEL_SPACING_MHZ + CHANNEL_OFFSET;
}

static int8_t power_limit_apply(uint8_t channel, int8_t unlimited_power)
{
	otError error;
	int8_t output_power_limit;
	int8_t limited_power = unlimited_power;

	error = vendor_radio_power_limit_get(channel, vendor_radio_power_map_tx_path_id_get(),
					     &output_power_limit);
	if (error == OT_ERROR_NONE) {
		limited_power = MIN(output_power_limit, unlimited_power);
	}

	return limited_power;
}

void power_model_output_fetch(int8_t requested_power, uint16_t freq_mhz,
			      mpsl_fem_power_model_output_t *p_output, bool tx_power_ceiling)
{
	(void)tx_power_ceiling;

	if (vendor_diag_fem_enabled()) {
		p_output->soc_pwr = vendor_diag_radio_tx_power_get();
		p_output->fem_pa_power_control = vendor_diag_fem_gain_get();
		p_output->achieved_pwr = p_output->soc_pwr + p_output->fem_pa_power_control;
	} else {
		uint8_t attenuation;
		uint8_t channel;
		int8_t limited_power;

		switch (fem_psemi_state_get()) {
		case FEM_PSEMI_STATE_PA_ACTIVE:
		case FEM_PSEMI_STATE_AUTO:
			channel = freg_mhz_to_channel(freq_mhz);
			limited_power = power_limit_apply(channel, requested_power);

			assert(p_output != NULL);

			if (vendor_radio_power_map_internal_tx_power_get(
				    channel, limited_power, &p_output->soc_pwr, &attenuation,
				    &p_output->achieved_pwr) != OT_ERROR_NONE) {
				assert(false);
			}
			p_output->fem_pa_power_control =
				vendor_radio_power_map_att_to_gain(attenuation);
			break;

		default:
			p_output->soc_pwr = requested_power;
			p_output->fem_pa_power_control = 0;
			p_output->achieved_pwr = requested_power;
		}
	}
}

void power_model_init(void)
{
	vendor_radio_power_map_init();
	vendor_radio_power_limit_init(CONFIG_POWER_LIMIT_TABLE_ID);
}