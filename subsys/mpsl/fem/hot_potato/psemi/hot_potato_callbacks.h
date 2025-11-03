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

/**
 * @file
 * @brief This file implements a weak functions of hot potato callbacks.
 */

#include "mpsl_fem_power_model.h"

#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

/**
 * @brief Starts CPU usage measurement for the vendor.
 */
__WEAK void VendorUsageCpuMeasureBegin(void)
{
	return;
}

/**
 * @brief Ends CPU usage measurement for the vendor.
 */
__WEAK void VendorUsageCpuMeasureEnd(void)
{
	return;
}

/**
 * @brief Fetches the output parameters from the FEM power model for a given requested power.
 *
 * @param[in]  requested_power    The desired transmit power in dBm.
 * @param[in]  phy                The PHY type (see mpsl_phy_t).
 * @param[in]  freq_mhz           The frequency in MHz.
 * @param[out] p_output           Pointer to the structure to receive the power model output.
 * @param[in]  tx_power_ceiling   If true, the requested power is treated as a ceiling.
 */
__WEAK void power_model_output_fetch(int8_t requested_power, mpsl_phy_t phy, uint16_t freq_mhz,
				     mpsl_fem_power_model_output_t *p_output, bool tx_power_ceiling)
{
	(void)phy;
	(void)freq_mhz;

	p_output->soc_pwr =
		mpsl_tx_power_radio_supported_power_adjust(requested_power, tx_power_ceiling);
	p_output->fem_pa_power_control = FEM_GAIN_BYPASS;
	p_output->achieved_pwr = p_output->soc_pwr;
}
