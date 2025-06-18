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

#ifndef POWER_MODEL_H_
#define POWER_MODEL_H_

#include <stdint.h>
#include "mpsl_fem_power_model.h"

/** @brief Initializes the power model.
 *
 *  This function initializes the power model which is based on the power map table
 *  and the power limit table.
 */
void power_model_init(void);

/** @brief Fetches the power model output.
 *
 *  This function fetches the power model output for the given requested power and frequency.
 *
 *  @param[in]  requested_power   Requested power in dBm.
 *  @param[in]  freq_mhz          Frequency in MHz.
 *  @param[out] p_output          Pointer to the output structure.
 *  @param[in]  tx_power_ceiling  Flag indicating if the TX power ceiling should be applied.
 */
void power_model_output_fetch(int8_t requested_power, uint16_t freq_mhz,
			      mpsl_fem_power_model_output_t *p_output, bool tx_power_ceiling);

#endif /* POWER_MODEL_H_ */