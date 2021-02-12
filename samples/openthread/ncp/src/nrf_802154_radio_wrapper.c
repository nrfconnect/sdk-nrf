/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "nrf_802154_radio_wrapper.h"
#include <nrf_802154.h>
#include <device.h>
#include <net/ieee802154_radio.h>

static const struct device *radio_dev;
static struct ieee802154_radio_api *radio_api;

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
	radio_dev = device_get_binding(CONFIG_NET_CONFIG_IEEE802154_DEV_NAME);
	__ASSERT_NO_MSG(radio_dev != NULL);

	radio_api = (struct ieee802154_radio_api *)radio_dev->api;
	__ASSERT_NO_MSG(radio_api != NULL);

	return radio_api->get_capabilities(radio_dev);
}
