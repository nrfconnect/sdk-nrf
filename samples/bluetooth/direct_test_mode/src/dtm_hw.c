/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "nrf.h"

#include "dtm_hw.h"

bool dtm_hw_radio_validate(nrf_radio_txpower_t tx_power,
			   nrf_radio_mode_t radio_mode)
{
	/* Initializing code below is quite generic - for BLE, the values are
	 * fixed, and expressions are constant. Non-constant values are
	 * essentially set in radio_prepare().
	 */

	if (!(
#if defined(RADIO_TXPOWER_TXPOWER_Pos8dBm)
		tx_power == NRF_RADIO_TXPOWER_POS8DBM     ||
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos7dBm)
		tx_power == NRF_RADIO_TXPOWER_POS7DBM     ||
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos6dBm)
		tx_power == NRF_RADIO_TXPOWER_POS6DBM     ||
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos5dBm)
		tx_power == NRF_RADIO_TXPOWER_POS5DBM     ||
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos4dBm)
		tx_power == NRF_RADIO_TXPOWER_POS4DBM     ||
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos3dBm)
		tx_power == NRF_RADIO_TXPOWER_POS3DBM     ||
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos2dBm)
		tx_power == NRF_RADIO_TXPOWER_POS2DBM     ||
#endif
		tx_power == NRF_RADIO_TXPOWER_0DBM        ||
		tx_power == NRF_RADIO_TXPOWER_NEG4DBM     ||
		tx_power == NRF_RADIO_TXPOWER_NEG8DBM     ||
		tx_power == NRF_RADIO_TXPOWER_NEG12DBM    ||
		tx_power == NRF_RADIO_TXPOWER_NEG16DBM    ||
		tx_power == NRF_RADIO_TXPOWER_NEG20DBM    ||
		tx_power == NRF_RADIO_TXPOWER_NEG30DBM    ||
#if defined(RADIO_TXPOWER_TXPOWER_Neg40dBm)
		tx_power == NRF_RADIO_TXPOWER_NEG40DBM
#endif
		) ||

		!(
#if defined(RADIO_MODE_MODE_Ble_LR125Kbit)
		radio_mode == NRF_RADIO_MODE_BLE_LR125KBIT  ||
#endif
#if defined(RADIO_MODE_MODE_Ble_LR500Kbit)
		radio_mode == NRF_RADIO_MODE_BLE_LR500KBIT  ||
#endif
		radio_mode == NRF_RADIO_MODE_BLE_1MBIT      ||
		radio_mode == NRF_RADIO_MODE_BLE_2MBIT
		)
	) {
		return false;
	}

	return true;
}

bool dtm_hw_radio_lr_check(nrf_radio_mode_t radio_mode)
{
#if defined(RADIO_MODE_MODE_Ble_LR125Kbit)
	if (radio_mode == NRF_RADIO_MODE_BLE_LR125KBIT) {
		return true;
	}
#endif
#if defined(RADIO_MODE_MODE_Ble_LR500Kbit)
	if (radio_mode == NRF_RADIO_MODE_BLE_LR500KBIT) {
		return true;
	}
#endif

	return false;
}
