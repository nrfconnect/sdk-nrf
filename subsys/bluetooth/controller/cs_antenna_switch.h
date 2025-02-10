/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>

/** @brief Antenna switching Callback for use in Channel Sounding.
 *
 *  See also @ref sdc_support_channel_sounding
 *
 *  @param[in] antenna_index the index of the antenna being switched to. Valid range [0, 3]
 */
void cs_antenna_switch_func(uint8_t antenna_number);
