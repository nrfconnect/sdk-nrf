/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrf_modem_dect_phy.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/drivers/hwinfo.h>

LOG_MODULE_REGISTER(app);

BUILD_ASSERT(CONFIG_CARRIER, "Carrier must be configured according to local regulations");

#define DATA_LEN_MAX 32

static bool exit;
static uint16_t device_id;

/* Header type 1, due to endianness the order is different than in the specification. */
struct phy_ctrl_field_common {
	uint32_t packet_length : 4;
	uint32_t packet_length_type : 1;
	uint32_t header_format : 3;
	uint32_t short_network_id : 8;
	uint32_t transmitter_id_hi : 8;
	uint32_t transmitter_id_lo : 8;
	uint32_t df_mcs : 3;
	uint32_t reserved : 1;
	uint32_t transmit_power : 4;
	uint32_t pad : 24;
};

/* Semaphore to synchronize modem calls. */
K_SEM_DEFINE(operation_sem, 0, 1);

/* Callback after init operation. */
static void init(const struct nrf_modem_dect_phy_init_event *evt)
{
	if (evt->err) {
		LOG_ERR("PHY initialization failed, err %d", evt->err);
		exit = true;
		return;
	}

	k_sem_give(&operation_sem);
}

/* Callback after deinit operation. */
static void deinit(const struct nrf_modem_dect_phy_deinit_event *evt)
{
	if (evt->err) {
		LOG_ERR("Deinit failed, err %d", evt->err);
		return;
	}

	k_sem_give(&operation_sem);
}

/* Operation complete notification. */
static void op_complete(const struct nrf_modem_dect_phy_op_complete_event *evt)
{
	LOG_DBG("Operation 0x%x complete, result 0x%x time %"PRIu64,
		evt->handle, evt->err, evt->time);
	k_sem_give(&operation_sem);
}

/* Callback after receive stop operation. */
static void cancel(const struct nrf_modem_dect_phy_cancel_event *evt)
{
	LOG_DBG("Operation 0x%x canceled, result 0x%x time %"PRIu64,
		evt->handle, evt->err, evt->time);
	k_sem_give(&operation_sem);
}

/* Physical Control Channel reception notification. */
static void pcc(const struct nrf_modem_dect_phy_pcc_event *evt)
{
	struct phy_ctrl_field_common *header = (struct phy_ctrl_field_common *)evt->hdr.type_1;

	LOG_INF("Received header from device ID %d",
		header->transmitter_id_hi << 8 |  header->transmitter_id_lo);
}

/* Physical Control Channel CRC error notification. */
static void pcc_crc_err(const struct nrf_modem_dect_phy_pcc_crc_failure_event *evt)
{
	LOG_DBG("CRC error on PCC, time %"PRIu64, evt->time);
}

/* Physical Data Channel reception notification. */
static void pdc(const struct nrf_modem_dect_phy_pdc_event *evt)
{
	/* Received RSSI value is in fixed precision format Q14.1 */
	LOG_INF("Received data (RSSI: %d.%d): %s",
		(evt->rssi_2 / 2), (evt->rssi_2 & 0b1) * 5, (char *)evt->data);
}

/* Physical Data Channel CRC error notification. */
static void pdc_crc_err(const struct nrf_modem_dect_phy_pdc_crc_failure_event *evt)
{
	LOG_DBG("CRC error on PDC, time %"PRIu64, evt->time);
}

/* RSSI measurement result notification. */
static void rssi(const struct nrf_modem_dect_phy_rssi_event *evt)
{
	LOG_DBG("RSSI measurement, handle 0x%x time %"PRIu64" carrier %d",
		evt->handle, evt->time, evt->carrier);
}

/* Callback after link configuration operation. */
static void link_config(const struct nrf_modem_dect_phy_link_config_event *evt)
{
	LOG_DBG("Link configuration, result 0x%x", evt->err);
}

/* Callback after time query operation. */
static void time_get(const struct nrf_modem_dect_phy_time_get_event *evt)
{
	LOG_DBG("Time query configuration, result 0x%x time %"PRIu64, evt->err, evt->time);
}

/* Callback after capability get operation. */
static void capability_get(const struct nrf_modem_dect_phy_capability_get_event *evt)
{
	LOG_DBG("Capability get, result 0x%x time %"PRIu64, evt->err, evt->time);
}

/* Dect PHY callbacks. */
static struct nrf_modem_dect_phy_callbacks dect_phy_callbacks = {
	.init = init,
	.deinit = deinit,
	.op_complete = op_complete,
	.cancel = cancel,
	.pcc = pcc,
	.pcc_crc_err = pcc_crc_err,
	.pdc = pdc,
	.pdc_crc_err = pdc_crc_err,
	.rssi = rssi,
	.link_config = link_config,
	.time_get = time_get,
	.capability_get = capability_get,
};

/* Dect PHY init parameters. */
static struct nrf_modem_dect_phy_init_params dect_phy_init_params = {
	.harq_rx_expiry_time_us = 5000000,
	.harq_rx_process_count = 4,
};

/* Send operation. */
static int transmit(uint32_t handle, void *data, size_t data_len)
{
	int err;

	struct phy_ctrl_field_common header = {
		.header_format = 0x0,
		.packet_length_type = 0x0,
		.packet_length = 0x01,
		.short_network_id = (CONFIG_NETWORK_ID & 0xff),
		.transmitter_id_hi = (device_id >> 8),
		.transmitter_id_lo = (device_id & 0xff),
		.transmit_power = CONFIG_TX_POWER,
		.reserved = 0,
		.df_mcs = CONFIG_MCS,
	};

	struct nrf_modem_dect_phy_tx_params tx_op_params = {
		.start_time = 0,
		.handle = handle,
		.network_id = CONFIG_NETWORK_ID,
		.phy_type = 0,
		.lbt_rssi_threshold_max = 0,
		.carrier = CONFIG_CARRIER,
		.lbt_period = NRF_MODEM_DECT_LBT_PERIOD_MAX,
		.phy_header = (union nrf_modem_dect_phy_hdr *)&header,
		.data = data,
		.data_size = data_len,
	};

	err = nrf_modem_dect_phy_tx(&tx_op_params);
	if (err != 0) {
		return err;
	}

	return 0;
}

/* Receive operation. */
static int receive(uint32_t handle)
{
	int err;

	struct nrf_modem_dect_phy_rx_params rx_op_params = {
		.start_time = 0,
		.handle = handle,
		.network_id = CONFIG_NETWORK_ID,
		.mode = NRF_MODEM_DECT_PHY_RX_MODE_CONTINUOUS,
		.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF,
		.link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED,
		.rssi_level = -60,
		.carrier = CONFIG_CARRIER,
		.duration = CONFIG_RX_PERIOD_S * MSEC_PER_SEC *
			    NRF_MODEM_DECT_MODEM_TIME_TICK_RATE_KHZ,
		.filter.short_network_id = CONFIG_NETWORK_ID & 0xff,
		.filter.is_short_network_id_used = 1,
		/* listen for everything (broadcast mode used) */
		.filter.receiver_identity = 0,
	};

	err = nrf_modem_dect_phy_rx(&rx_op_params);
	if (err != 0) {
		return err;
	}

	return 0;
}

int main(void)
{
	int err;
	uint32_t tx_handle = 0;
	uint32_t rx_handle = 1;
	uint32_t tx_counter_value = 0;
	uint8_t tx_buf[DATA_LEN_MAX];
	size_t tx_len;

	LOG_INF("Dect NR+ PHY Hello sample started");

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("modem init failed, err %d", err);
		return err;
	}

	err = nrf_modem_dect_phy_callback_set(&dect_phy_callbacks);
	if (err) {
		LOG_ERR("nrf_modem_dect_phy_callback_set failed, err %d", err);
		return err;
	}

	err = nrf_modem_dect_phy_init(&dect_phy_init_params);
	if (err) {
		LOG_ERR("nrf_modem_dect_phy_init failed, err %d", err);
		return err;
	}

	k_sem_take(&operation_sem, K_FOREVER);
	if (exit) {
		return -EIO;
	}

	hwinfo_get_device_id((void *)&device_id, sizeof(device_id));

	LOG_INF("Dect NR+ PHY initialized, device ID: %d", device_id);

	err = nrf_modem_dect_phy_capability_get();
	if (err) {
		LOG_ERR("nrf_modem_dect_phy_capability_get failed, err %d", err);
	}

	while (1) {
		/** Transmitting message */
		LOG_INF("Transmitting %d", tx_counter_value);
		tx_len = sprintf(tx_buf, "Hello DECT! %d", tx_counter_value);

		err = transmit(tx_handle, tx_buf, tx_len);
		if (err) {
			LOG_ERR("Transmisstion failed, err %d", err);
			return err;
		}

		tx_counter_value++;

		if ((tx_counter_value >= CONFIG_TX_TRANSMISSIONS) && CONFIG_TX_TRANSMISSIONS) {
			LOG_INF("Reached maximum number of transmissions (%d)",
				CONFIG_TX_TRANSMISSIONS);
			break;
		}

		/* Wait for TX operation to complete. */
		k_sem_take(&operation_sem, K_FOREVER);

		/** Receiving messages for CONFIG_RX_PERIOD_S seconds. */
		err = receive(rx_handle);
		if (err) {
			LOG_ERR("Reception failed, err %d", err);
			return err;
		}

		/* Wait for RX operation to complete. */
		k_sem_take(&operation_sem, K_FOREVER);
	}

	LOG_INF("Shutting down");

	err = nrf_modem_dect_phy_deinit();
	if (err) {
		LOG_ERR("nrf_modem_dect_phy_deinit() failed, err %d", err);
		return err;
	}

	k_sem_take(&operation_sem, K_FOREVER);

	err = nrf_modem_lib_shutdown();
	if (err) {
		LOG_ERR("nrf_modem_lib_shutdown() failed, err %d", err);
		return err;
	}

	LOG_INF("Bye!");

	return 0;
}
