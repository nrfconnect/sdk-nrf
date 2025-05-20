/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <debug/ppi_trace.h>
#include <hal/nrf_radio.h>
#include <hal/nrf_uarte.h>

LOG_MODULE_DECLARE(app);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void radio_ppi_trace_setup(void)
{
	uint32_t start_evt;
	uint32_t stop_evt;
	void *handle;

	start_evt = nrf_radio_event_address_get(NRF_RADIO,
				NRF_RADIO_EVENT_READY);
	stop_evt = nrf_radio_event_address_get(NRF_RADIO,
				NRF_RADIO_EVENT_DISABLED);

	handle = ppi_trace_pair_config(CONFIG_PPI_TRACE_PIN_RADIO_ACTIVE,
				start_evt, stop_evt);
	__ASSERT(handle != NULL, "Failed to configure PPI trace pair.\n");

	ppi_trace_enable(handle);
}

static void bt_ready(int err)
{
	radio_ppi_trace_setup();

	__ASSERT(err == 0, "Bluetooth init failed (err %d)\n", err);

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	__ASSERT(err == 0, "Advertising start failed (err %d)\n", err);

	LOG_INF("Bluetooth advertising started.");
}

void bluetooth_enable(void)
{
	int err;

	LOG_INF("Starting Bluetooth advertising.");


	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	__ASSERT(err == 0, "Bluetooth enabling failed (err %d)\n", err);
}
