/* Copyright (c) 2023, Nordic Semiconductor ASA
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
 *   This file implements power map table related functions.
 */

#include "vendor_radio_power_map_common.h"
#include "vendor_radio_power_map_psemi.h"

union {
	power_map_psemi_data_t psemi;
} power_map_data;

power_map_psemi_data_t *vendor_radio_power_map_psemi_data_get(void)
{
	return &power_map_data.psemi;
}

const char *vendor_radio_power_map_status_to_cstr(power_map_status_t status)
{
	const char *p_status_str;

	switch (status) {
	case POWER_MAP_STATUS_OK:
		p_status_str = "ok";
		break;

	case POWER_MAP_STATUS_MISSING:
		p_status_str = "not found";
		break;

	case POWER_MAP_STATUS_PARSING_ERR:
		p_status_str = "parsing error";
		break;

	case POWER_MAP_STATUS_INVALID_LEN:
		p_status_str = "invalid length";
		break;

	case POWER_MAP_STATUS_INVALID_CRC:
		p_status_str = "invalid CRC";
		break;

	case POWER_MAP_STATUS_INVALID_VERSION:
		p_status_str = "invalid version";
		break;

	default:
		p_status_str = "unknown status";
		break;
	}

	return p_status_str;
}
