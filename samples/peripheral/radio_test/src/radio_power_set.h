/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RADIO_POWER_SET_H_
#define RADIO_POWER_SET_H_

#include <stdint.h>
#include <stdbool.h>
#include <hal/nrf_radio.h>

/**
 * @brief Set the radio power.
 *
 * @param[in] mode  Radio mode.
 * @param[in] channel  Channel.
 * @param[in] power  Power.
 */
void radio_power_set(nrf_radio_mode_t mode, uint8_t channel, int8_t power);

#endif /* RADIO_POWER_SET_H_ */
