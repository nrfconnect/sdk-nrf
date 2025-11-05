/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_RADIO_H_
#define NRF_RPC_RADIO_H_

/**
 * @brief Returns if the radio has PA modulation fix enabled.
 *
 * @retval True   PA modulation fix is enabled.
 * @retval False  PA modulation fix is disabled.
 */
bool nrf_802154_pa_modulation_fix_get(void);

/**
 * @brief Enables or disables the PA modulation fix.
 *
 * @note The PA modulation fix is enabled by default on the chips that require it.
 *
 * @param[in]  enabled  If the PA modulation fix is to be enabled.
 */
void nrf_802154_pa_modulation_fix_set(bool enable);

#endif /* NRF_RPC_RADIO_H_ */
