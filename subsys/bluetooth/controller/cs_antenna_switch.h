/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>

/** @brief Antenna switching Callback for use in Channel Sounding.
 *  @note  This function is called from interrupt context and must not
 *         call Zephyr APIs or any blocking APIs.
 *
 *  See also @ref sdc_support_channel_sounding
 *
 *  @param[in] antenna_index the index of the antenna being switched to.
 *                           Valid range [0, @ref sdc_cfg_cs_cfg_t::num_antennas_supported - 1]
 */
void cs_antenna_switch_func(uint8_t antenna_number);

/** @brief Function to initialize the pins used by antenna switching in Channel Sounding. */
void cs_antenna_switch_init(void);

/** @brief Function to clear the pins used by antenna switching in Channel Sounding. */
void cs_antenna_switch_clear(void);
