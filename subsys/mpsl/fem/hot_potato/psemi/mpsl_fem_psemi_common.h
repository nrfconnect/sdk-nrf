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

#ifndef FEM_PSEMI_COMMON_H_
#define FEM_PSEMI_COMMON_H_

#include <stdint.h>

#include "protocol/mpsl_fem_protocol_api.h"
#include "mpsl_fem_psemi.h"

/** @brief Gets the capabilities of the FEM.
 *
 *  @param[out] p_caps  Pointer to the capabilities structure to be filled in.
 */
void fem_psemi_caps_get(mpsl_fem_caps_t *p_caps);

/** @brief Implements @ref mpsl_fem_tx_power_split for fem psemi.
 *
 *  See @ref mpsl_fem_tx_power_split for documentation of parameters and return value.
 */
int8_t fem_psemi_tx_power_split(const mpsl_tx_power_t power,
				mpsl_tx_power_split_t *const p_tx_power_split, mpsl_phy_t phy,
				uint16_t freq_mhz, bool tx_power_ceiling);

/** @brief Sets the PA power control.
 *
 * @note The PA power control set by this function will be applied to radio transmissions
 * following the call. If the function is called during radio transmission
 * or during ramp-up for transmission it is unspecified if the control is applied.
 *
 * @param[in] pa_power_control  PA power control to be applied to the FEM.
 *
 * @retval   0             PA power control has been applied successfully.
 * @retval   -NRF_EINVAL   PA power control could not be applied. Provided @p pa_power_control is
 * invalid.
 */
int32_t fem_psemi_pa_power_control_set(mpsl_fem_pa_power_control_t pa_power_control);

/** @brief Set FEM gain to default state. All ATT pins are set to inactive state.
 *
 * @param[in] p_config  Pointer to the FEM configuration.
 */
void fem_psemi_pa_gain_default(mpsl_fem_psemi_t const *const p_config);

/** @brief Configure GPIO for gain control.
 *
 * @param[in] p_config  Pointer to the FEM configuration.
 */
void fem_psemi_gain_gpio_configure(mpsl_fem_psemi_t const *const p_config);

#endif /* FEM_PSEMI_COMMON_H_ */
