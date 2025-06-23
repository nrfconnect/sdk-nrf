/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "nrf_802154_radio_wrapper.h"
#include <nrf_802154.h>

#ifdef CONFIG_NETWORKING
#include <zephyr/device.h>
#include <zephyr/net/ieee802154_radio.h>

static const struct device *const radio_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));
static struct ieee802154_radio_api *radio_api;

#else
static uint16_t get_capabilities(void)
{
	nrf_802154_capabilities_t caps = nrf_802154_capabilities_get();

	/* Convert nrf_802154_capabilities_t to ieee802154_hw_caps to keep backwards compatibility
	 * See @ref ieee802154_hw_caps in zephyr/net/ieee802154_radio.h for more details.
	 */
	return /* IEEE802154_HW_FCS */ BIT(1) |
	       /* IEEE802154_HW_PROMISC */ BIT(3) |
	       /* IEEE802154_HW_FILTER */ BIT(2) |
	       /* IEEE802154_HW_CSMA */ ((caps & NRF_802154_CAPABILITY_CSMA) ? BIT(4) : 0UL) |
	       /* IEEE802154_HW_TX_RX_ACK */ BIT(5) |
	       /* IEEE802154_HW_RX_TX_ACK */ BIT(7) |
	       /* IEEE802154_HW_ENERGY_SCAN */ BIT(0) |
	       /* IEEE802154_HW_TXTIME */
	       ((caps & NRF_802154_CAPABILITY_DELAYED_TX) ? BIT(8) : 0UL) |
	       /* IEEE802154_HW_RXTIME */
	       ((caps & NRF_802154_CAPABILITY_DELAYED_RX) ? BIT(10) : 0UL) |
	       /* IEEE802154_HW_SLEEP_TO_TX */ BIT(9) |
	       /* IEEE802154_HW_TX_SEC */ BIT(12) |
	       ((caps & NRF_802154_CAPABILITY_SECURITY) ? BIT(11) : 0UL)
#if defined(CONFIG_IEEE802154_NRF5_MULTIPLE_CCA)
	       | /* IEEE802154_OPENTHREAD_HW_CAPS_BITS_START */ BIT(14)
#endif
#if defined(CONFIG_IEEE802154_SELECTIVE_TXCHANNEL)
	       | /* IEEE802154_HW_SELECTIVE_TXCHANNEL */ BIT(13)
#endif
#if defined(CONFIG_IEEE802154_NRF5_CST_ENDPOINT)
	       | /* IEEE802154_OPENTHREAD_HW_CAPS_BITS_START + 1*/ BIT(15)
#endif
		;
}

#endif

bool nrf_802154_radio_wrapper_auto_ack_get(void)
{
	return nrf_802154_auto_ack_get();
}

void nrf_802154_radio_wrapper_auto_ack_set(bool enabled)
{
	nrf_802154_auto_ack_set(enabled);
}

uint16_t nrf_802154_radio_wrapper_hw_capabilities_get(void)
{
#ifdef CONFIG_NETWORKING
	__ASSERT_NO_MSG(device_is_ready(radio_dev));

	radio_api = (struct ieee802154_radio_api *)radio_dev->api;
	__ASSERT_NO_MSG(radio_api != NULL);

	return radio_api->get_capabilities(radio_dev);
#else
	return get_capabilities();
#endif
}
