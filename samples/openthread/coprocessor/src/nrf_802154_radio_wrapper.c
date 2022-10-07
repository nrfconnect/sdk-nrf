/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "nrf_802154_radio_wrapper.h"
#include <nrf_802154.h>
#include <zephyr/device.h>
#include <zephyr/net/ieee802154_radio.h>

static const struct device *const radio_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));
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
	__ASSERT_NO_MSG(device_is_ready(radio_dev));

	radio_api = (struct ieee802154_radio_api *)radio_dev->api;
	__ASSERT_NO_MSG(radio_api != NULL);

	return radio_api->get_capabilities(radio_dev);
}
