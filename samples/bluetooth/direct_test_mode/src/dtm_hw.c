/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf.h"

#include "dtm_hw.h"
#include "dtm_hw_config.h"

const uint32_t nrf_power_value[] = {
#if defined(RADIO_TXPOWER_TXPOWER_Neg40dBm)
	RADIO_TXPOWER_TXPOWER_Neg40dBm,
#endif /* RADIO_TXPOWER_TXPOWER_Neg40dBm */
	RADIO_TXPOWER_TXPOWER_Neg30dBm,
	RADIO_TXPOWER_TXPOWER_Neg20dBm,
	RADIO_TXPOWER_TXPOWER_Neg16dBm,
	RADIO_TXPOWER_TXPOWER_Neg12dBm,
	RADIO_TXPOWER_TXPOWER_Neg8dBm,
#if defined(RADIO_TXPOWER_TXPOWER_Neg7dBm)
	RADIO_TXPOWER_TXPOWER_Neg7dBm,
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg7dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Neg6dBm)
	RADIO_TXPOWER_TXPOWER_Neg6dBm,
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg6dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Neg5dBm)
	RADIO_TXPOWER_TXPOWER_Neg5dBm,
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg5dBm) */
	RADIO_TXPOWER_TXPOWER_Neg4dBm,
#if defined(RADIO_TXPOWER_TXPOWER_Neg3dBm)
	RADIO_TXPOWER_TXPOWER_Neg3dBm,
#endif /* defined (RADIO_TXPOWER_TXPOWER_Neg3dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Neg2dBm)
	RADIO_TXPOWER_TXPOWER_Neg2dBm,
#endif /* defined (RADIO_TXPOWER_TXPOWER_Neg2dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Neg1dBm)
	RADIO_TXPOWER_TXPOWER_Neg1dBm,
#endif /* defined (RADIO_TXPOWER_TXPOWER_Neg1dBm) */
	RADIO_TXPOWER_TXPOWER_0dBm,
#if defined(RADIO_TXPOWER_TXPOWER_Pos2dBm)
	RADIO_TXPOWER_TXPOWER_Pos2dBm,
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos2dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos3dBm)
	RADIO_TXPOWER_TXPOWER_Pos3dBm,
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos3dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos4dBm)
	RADIO_TXPOWER_TXPOWER_Pos4dBm,
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos4dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos5dBm)
	RADIO_TXPOWER_TXPOWER_Pos5dBm,
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos5dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos6dBm)
	RADIO_TXPOWER_TXPOWER_Pos6dBm,
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos6dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos7dBm)
	RADIO_TXPOWER_TXPOWER_Pos7dBm,
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos7dBm) */
#if defined(RADIO_TXPOWER_TXPOWER_Pos8dBm)
	RADIO_TXPOWER_TXPOWER_Pos8dBm
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos8dBm) */
};

#if DIRECTION_FINDING_SUPPORTED
/**@brief Antenna pin array.
 */
static const uint32_t antenna_pin[] = {
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), dtm_antenna_1_pin)
	DT_PROP(DT_PATH(zephyr_user), dtm_antenna_1_pin),
#endif
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), dtm_antenna_2_pin)
	DT_PROP(DT_PATH(zephyr_user), dtm_antenna_2_pin),
#endif
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), dtm_antenna_3_pin)
	DT_PROP(DT_PATH(zephyr_user), dtm_antenna_3_pin),
#endif
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), dtm_antenna_4_pin)
	DT_PROP(DT_PATH(zephyr_user), dtm_antenna_4_pin),
#endif
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), dtm_antenna_5_pin)
	DT_PROP(DT_PATH(zephyr_user), dtm_antenna_6_pin),
#endif
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), dtm_antenna_6_pin)
	DT_PROP(DT_PATH(zephyr_user), dtm_antenna_6_pin),
#endif
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), dtm_antenna_7_pin)
	DT_PROP(DT_PATH(zephyr_user), dtm_antenna_7_pin),
#endif
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), dtm_antenna_8_pin)
	DT_PROP(DT_PATH(zephyr_user), dtm_antenna_8_pin),
#endif
};
#endif /* DIRECTION_FINDING_SUPPORTED */

bool dtm_hw_radio_validate(nrf_radio_txpower_t tx_power,
			   nrf_radio_mode_t radio_mode)
{
	/* Initializing code below is quite generic - for BLE, the values are
	 * fixed, and expressions are constant. Non-constant values are
	 * essentially set in radio_prepare().
	 */

	if (!(
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

	for (size_t i = 0; i < ARRAY_SIZE(nrf_power_value); i++) {
		if (tx_power == nrf_power_value[i]) {
			return true;
		}
	}

	return false;
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

uint32_t dtm_hw_radio_min_power_get(void)
{
	return nrf_power_value[0];
}

uint32_t dtm_hw_radio_max_power_get(void)
{
	return nrf_power_value[ARRAY_SIZE(nrf_power_value) - 1];
}

size_t dtm_hw_radio_power_array_size_get(void)
{
	return ARRAY_SIZE(nrf_power_value);
}

const uint32_t *dtm_hw_radio_power_array_get(void)
{
	return nrf_power_value;
}

#if DIRECTION_FINDING_SUPPORTED
size_t dtm_radio_antenna_pin_array_size_get(void)
{
	return ARRAY_SIZE(antenna_pin);
}

const uint32_t *dtm_hw_radion_antenna_pin_array_get(void)
{
	return antenna_pin;
}
#endif /* DIRECTION_FINDING_SUPPORTED */
