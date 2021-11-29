/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include "fem.h"
#include "fem_interface.h"
#include "radio_def.h"

static const struct fem_interface_api *fem_api;

int fem_power_up(void)
{
	return fem_api->power_up();
}

int fem_power_down(void)
{
	return fem_api->power_down();
}

int fem_tx_configure(uint32_t activate_event, uint32_t deactivate_event,
		     uint32_t activation_delay)
{
	return fem_api->tx_configure(activate_event, deactivate_event, activation_delay);
}

int fem_rx_configure(uint32_t activate_event, uint32_t deactivate_event,
		     uint32_t activation_delay)
{
	return fem_api->rx_configure(activate_event, deactivate_event, activation_delay);
}

void fem_txrx_configuration_clear(void)
{
	return fem_api->txrx_configuration_clear();
}

int fem_txrx_stop(void)
{
	return fem_api->txrx_stop();
}

int fem_tx_gain_set(uint32_t gain)
{
	return fem_api->tx_gain_set(gain);
}

uint32_t fem_default_active_delay_calculate(bool rx,
					    nrf_radio_mode_t mode)
{
	return fem_api->default_active_delay_calculate(rx, mode);
}

int fem_antenna_select(enum fem_antenna ant)
{
	return fem_api->antenna_select(ant);
}

int fem_interface_api_set(const struct fem_interface_api *api)
{
	if (api == NULL) {
		return -EINVAL;
	}

	fem_api = api;

	return 0;
}

uint32_t fem_radio_tx_ramp_up_delay_get(bool fast, nrf_radio_mode_t mode)
{
	uint32_t ramp_up;

	switch (mode) {
	case NRF_RADIO_MODE_BLE_1MBIT:
		ramp_up = fast ? RADIO_RAMP_UP_TX_1M_FAST_US :
				 RADIO_RAMP_UP_TX_1M_US;
		break;

	case NRF_RADIO_MODE_BLE_2MBIT:
		ramp_up = fast ? RADIO_RAMP_UP_TX_2M_FAST_US :
				 RADIO_RAMP_UP_TX_2M_US;
		break;
#if CONFIG_HAS_HW_NRF_RADIO_BLE_CODED
	case NRF_RADIO_MODE_BLE_LR125KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_TX_S8_FAST_US :
				 RADIO_RAMP_UP_TX_S8_US;
		break;

	case NRF_RADIO_MODE_BLE_LR500KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_TX_S2_FAST_US :
				 RADIO_RAMP_UP_TX_S2_US;
		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */

#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
	case NRF_RADIO_MODE_IEEE802154_250KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_IEEE_FAST_US :
				 RADIO_RAMP_UP_IEEE_US;
		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */

	default:
		ramp_up = fast ? RADIO_RAMP_UP_DEFAULT_FAST_US :
				 RADIO_RAMP_UP_DEFAULT_US;
		break;
	}

	return ramp_up;
}

uint32_t fem_radio_rx_ramp_up_delay_get(bool fast, nrf_radio_mode_t mode)
{
	uint32_t ramp_up;

	switch (mode) {
	case NRF_RADIO_MODE_BLE_1MBIT:
		ramp_up = fast ? RADIO_RAMP_UP_RX_1M_FAST_US :
				 RADIO_RAMP_UP_RX_1M_US;
		break;

	case NRF_RADIO_MODE_BLE_2MBIT:
		ramp_up = fast ? RADIO_RAMP_UP_RX_2M_FAST_US :
				 RADIO_RAMP_UP_RX_2M_US;
		break;

#if CONFIG_HAS_HW_NRF_RADIO_BLE_CODED
	case NRF_RADIO_MODE_BLE_LR125KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_RX_S8_FAST_US :
				 RADIO_RAMP_UP_RX_S8_US;
		break;

	case NRF_RADIO_MODE_BLE_LR500KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_RX_S2_FAST_US :
				 RADIO_RAMP_UP_RX_S2_US;
		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */

#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
	case NRF_RADIO_MODE_IEEE802154_250KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_IEEE_FAST_US :
				 RADIO_RAMP_UP_IEEE_US;
		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */

	default:
		ramp_up = fast ? RADIO_RAMP_UP_DEFAULT_FAST_US :
				 RADIO_RAMP_UP_DEFAULT_US;
		break;
	}

	return ramp_up;
}
