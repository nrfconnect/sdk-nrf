/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: Adaptation layer for RF configuring and processing */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <kernel.h>

#include "nrf_802154.h"

#include "rf_proc.h"

#include "ptt.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(rf_proc);

/**< Maximum size of RF pool of received packets */
#define RF_RX_POOL_N 16u

static struct rf_rx_pkt_s rf_rx_pool[RF_RX_POOL_N];
static struct rf_rx_pkt_s ack_packet;
static uint8_t temp_tx_pkt[RF_PSDU_MAX_SIZE];

static inline void rf_rx_pool_init(void);
static void rf_rx_pool_clear(void);

static void rf_tx_finished_fn(struct k_work *work)
{
	ptt_rf_tx_finished();
}

K_WORK_DEFINE(rf_tx_finished_wq, rf_tx_finished_fn);

static void rf_tx_ack_received_fn(struct k_work *work)
{
	ptt_rf_push_packet(ack_packet.data, ack_packet.length, ack_packet.rssi, ack_packet.lqi);
	nrf_802154_buffer_free_raw(ack_packet.rf_buf);
}

K_WORK_DEFINE(rf_tx_ack_received_wq, rf_tx_ack_received_fn);

static void rf_cca_failed_fn(struct k_work *work)
{
	ptt_rf_cca_failed();
}

K_WORK_DEFINE(rf_cca_failed_wq, rf_cca_failed_fn);

static void rf_ed_failed_fn(struct k_work *work)
{
	ptt_rf_ed_failed();
}

K_WORK_DEFINE(rf_ed_failed_wq, rf_ed_failed_fn);

struct rf_rx_failed_info {
	struct k_work work;
	ptt_rf_tx_error_t rx_error;
} rf_rx_failed_info;
static void rf_rx_failed_fn(struct k_work *work)
{
	ptt_rf_rx_failed(rf_rx_failed_info.rx_error);
}

struct rf_tx_failed_info {
	struct k_work work;
	ptt_rf_tx_error_t tx_error;
} rf_tx_failed_info;
static void rf_tx_failed_fn(struct k_work *work)
{
	ptt_rf_tx_failed(rf_tx_failed_info.tx_error);
}

struct rf_cca_done_info {
	struct k_work work;
	bool channel_free;
} rf_cca_done_info;
static void rf_cca_done_fn(struct k_work *work)
{
	ptt_rf_cca_done((bool)rf_cca_done_info.channel_free);
}

struct rf_ed_detected_info {
	struct k_work work;
	uint8_t result;
} rf_ed_detected_info;
static void rf_ed_detected_fn(struct k_work *work)
{
	ptt_rf_ed_detected((ptt_ed_t)rf_ed_detected_info.result);
}

#if CONFIG_PTT_ANTENNA_DIVERSITY
#warning "Antenna diversity enabled but not yet supported in NCS"
/* TODO: Implement when antenna diversity is supported in NCS */
static void configure_antenna_diversity(void);

#endif /* CONFIG_PTT_ANTENNA_DIVERSITY */

void rf_init(void)
{
	k_work_init(&rf_rx_failed_info.work, rf_rx_failed_fn);
	k_work_init(&rf_tx_failed_info.work, rf_tx_failed_fn);
	k_work_init(&rf_cca_done_info.work, rf_cca_done_fn);
	k_work_init(&rf_ed_detected_info.work, rf_ed_detected_fn);

	/* clear a pool for received packets */
	rf_rx_pool_init();
	ack_packet = (struct rf_rx_pkt_s){ 0 };

	/* nrf radio driver initialization */
	nrf_802154_init();

#if CONFIG_PTT_ANTENNA_DIVERSITY
	/* TODO: Implement when antenna diversity is supported in NCS */
	configure_antenna_diversity();
#endif
}

void rf_uninit(void)
{
	/* free packets and marks them as free */
	rf_rx_pool_clear();
	ack_packet = (struct rf_rx_pkt_s){ 0 };
	nrf_802154_deinit();
}

#if CONFIG_PTT_ANTENNA_DIVERSITY
static void configure_antenna_diversity(void)
{
	/* TODO: Implement when antenna diversity is supported in NCS */
}

#endif /* CONFIG_PTT_ANTENNA_DIVERSITY */

static void rf_process_rx_packets(void)
{
	struct rf_rx_pkt_s *rx_pkt = NULL;

	for (uint8_t i = 0; i < RF_RX_POOL_N; i++) {
		rx_pkt = &rf_rx_pool[i];
		if (rx_pkt->data != NULL) {
			ptt_rf_push_packet(rx_pkt->data, rx_pkt->length, rx_pkt->rssi, rx_pkt->lqi);
			nrf_802154_buffer_free_raw(rx_pkt->rf_buf);
			rx_pkt->data = NULL;
		}
	}
}

void rf_thread(void)
{
	while (1) {
		rf_process_rx_packets();
		k_sleep(K_MSEC(1));
	}
}

void nrf_802154_received_raw(uint8_t *data, int8_t power, uint8_t lqi)
{
	struct rf_rx_pkt_s *rx_pkt = NULL;
	bool pkt_placed = false;

	assert(data != NULL);

	for (uint8_t i = 0; i < RF_RX_POOL_N; i++) {
		if (rf_rx_pool[i].data == NULL) {
			rx_pkt = &rf_rx_pool[i];
			rx_pkt->rf_buf = data;
			rx_pkt->data = &data[1];
			rx_pkt->length = data[0] - RF_FCS_SIZE;
			rx_pkt->rssi = power;
			rx_pkt->lqi = lqi;
			pkt_placed = true;
			break;
		}
	}
	if (false == pkt_placed) {
		LOG_ERR("Not enough space to store packet. Will drop it.");
		nrf_802154_buffer_free_raw(data);
	}
}

void nrf_802154_receive_failed(nrf_802154_rx_error_t error, uint32_t id)
{
	LOG_INF("rx failed error %d!", error);

	/* mapping nrf_802154 errors into PTT RF errors */
	/* actually only invalid ACK FCS matters at the moment */
	if (error == NRF_802154_RX_ERROR_INVALID_FCS) {
		rf_rx_failed_info.rx_error = PTT_RF_RX_ERROR_INVALID_FCS;
	} else {
		rf_rx_failed_info.rx_error = PTT_RF_RX_ERROR_OPERATION_FAILED;
	}

	k_work_submit(&rf_rx_failed_info.work);
}

void nrf_802154_transmitted_raw(uint8_t *frame,
	const nrf_802154_transmit_done_metadata_t *metadata)
{
	ARG_UNUSED(frame);

	if (metadata->data.transmitted.p_ack != NULL) {
		ack_packet = (struct rf_rx_pkt_s){ 0 };

		ack_packet.data = metadata->data.transmitted.p_ack;
		ack_packet.length = metadata->data.transmitted.length;
		ack_packet.rssi = metadata->data.transmitted.power;
		ack_packet.lqi = metadata->data.transmitted.lqi;
		ack_packet.rf_buf = metadata->data.transmitted.p_ack;

		k_work_submit(&rf_tx_ack_received_wq);
	}

	k_work_submit(&rf_tx_finished_wq);
}

void nrf_802154_transmit_failed(uint8_t *frame, nrf_802154_tx_error_t error,
	const nrf_802154_transmit_done_metadata_t *p_metadata)
{
	ARG_UNUSED(frame);

	LOG_INF("tx failed error %d!", error);

	/* mapping nrf_802154 errors into PTT RF errors */
	if (error == NRF_802154_TX_ERROR_INVALID_ACK) {
		rf_tx_failed_info.tx_error = PTT_RF_TX_ERROR_INVALID_ACK_FCS;
	} else if (error == NRF_802154_TX_ERROR_NO_ACK) {
		rf_tx_failed_info.tx_error = PTT_RF_TX_ERROR_NO_ACK;
	} else {
		rf_tx_failed_info.tx_error = PTT_RF_TX_ERROR_OPERATION_FAILED;
	}

	k_work_submit(&rf_tx_failed_info.work);
}

void nrf_802154_cca_done(bool channel_free)
{
	LOG_INF("CCA finished with result %d", channel_free);

	rf_cca_done_info.channel_free = channel_free;
	k_work_submit(&rf_cca_done_info.work);
}

void nrf_802154_cca_failed(nrf_802154_cca_error_t error)
{
	ARG_UNUSED(error);

	LOG_INF("CCA failed error %d!", error);

	k_work_submit(&rf_cca_failed_wq);
}

void nrf_802154_energy_detected(uint8_t result)
{
	LOG_INF("ED finished with result %d", result);

	rf_ed_detected_info.result = result;
	k_work_submit(&rf_ed_detected_info.work);
}

void nrf_802154_energy_detection_failed(nrf_802154_ed_error_t error)
{
	ARG_UNUSED(error);

	LOG_INF("ED failed error %d!", error);

	k_work_submit(&rf_ed_failed_wq);
}

void ptt_rf_set_channel_ext(uint8_t channel)
{
	nrf_802154_channel_set(channel);
}

void ptt_rf_set_power_ext(int8_t power)
{
	nrf_802154_tx_power_set(power);
}

int8_t ptt_rf_get_power_ext(void)
{
	return nrf_802154_tx_power_get();
}

void ptt_rf_set_antenna_ext(uint8_t antenna)
{
#if CONFIG_PTT_ANTENNA_DIVERSITY
	/* TODO: Implement when antenna diversity is supported in NCS */
#endif
}

uint8_t ptt_rf_get_antenna_ext(void)
{
#if CONFIG_PTT_ANTENNA_DIVERSITY
	/* TODO: Implement when antenna diversity is supported in NCS */
	return 0;
#else
	return 0;
#endif
}

void ptt_rf_promiscuous_set_ext(bool value)
{
	nrf_802154_promiscuous_set(value);
}

bool ptt_rf_receive_ext(void)
{
	return nrf_802154_receive();
}

void ptt_rf_set_pan_id_ext(const uint8_t *pan_id)
{
	nrf_802154_pan_id_set(pan_id);
}

void ptt_rf_set_extended_address_ext(const uint8_t *extended_address)
{
	nrf_802154_extended_address_set(extended_address);
}

void ptt_rf_set_short_address_ext(const uint8_t *short_address)
{
	nrf_802154_short_address_set(short_address);
}

bool ptt_rf_cca_ext(uint8_t mode)
{
	bool ret = false;

	if ((mode >= NRF_RADIO_CCA_MODE_CARRIER) && (mode <= NRF_RADIO_CCA_MODE_CARRIER_OR_ED)) {
		nrf_802154_cca_cfg_t cca_cfg;

		nrf_802154_cca_cfg_get(&cca_cfg);

		cca_cfg.mode = mode;

		nrf_802154_cca_cfg_set(&cca_cfg);

		ret = nrf_802154_cca();
	} else {
		ret = false;
	}

	return ret;
}

bool ptt_rf_ed_ext(uint32_t time_us)
{
	return nrf_802154_energy_detection(time_us);
}

bool ptt_rf_rssi_measure_begin_ext(void)
{
	return nrf_802154_rssi_measure_begin();
}

int8_t ptt_rf_rssi_last_get_ext(void)
{
	return nrf_802154_rssi_last_get();
}

bool ptt_rf_send_packet_ext(const uint8_t *pkt, ptt_pkt_len_t len, bool cca)
{
	bool ret = false;

	if ((pkt == NULL) || (len > RF_PSDU_MAX_SIZE - RF_FCS_SIZE)) {
		ret = false;
	} else {
		/* temp_tx_pkt is protected inside ptt rf by locking mechanism */
		temp_tx_pkt[0] = len + RF_FCS_SIZE;
		memcpy(&temp_tx_pkt[RF_PSDU_START], pkt, len);
		const nrf_802154_transmit_metadata_t metadata = {
			.cca = cca
		};
		ret = nrf_802154_transmit_raw(temp_tx_pkt, &metadata);
	}

	return ret;
}

bool ptt_rf_modulated_stream_ext(const uint8_t *pkt, ptt_pkt_len_t len)
{
	bool ret = false;

	if ((pkt == NULL) || (len > RF_PSDU_MAX_SIZE - RF_FCS_SIZE)) {
		ret = false;
	} else {
		/* temp_tx_pkt is protected inside ptt rf by locking mechanism */
		temp_tx_pkt[0] = len + RF_FCS_SIZE;
		memcpy(&temp_tx_pkt[RF_PSDU_START], pkt, len);

		ret = nrf_802154_modulated_carrier(temp_tx_pkt);
	}

	return ret;
}

bool ptt_rf_sleep_ext(void)
{
	return nrf_802154_sleep();
}

bool ptt_rf_continuous_carrier_ext(void)
{
	return nrf_802154_continuous_carrier();
}

static inline void rf_rx_pool_init(void)
{
	for (uint8_t i = 0; i < RF_RX_POOL_N; i++) {
		rf_rx_pool[i] = (struct rf_rx_pkt_s){ 0 };
	}
}

static void rf_rx_pool_clear(void)
{
	struct rf_rx_pkt_s *rx_pkt = NULL;

	for (uint8_t i = 0; i < RF_RX_POOL_N; i++) {
		rx_pkt = &rf_rx_pool[i];
		if (rx_pkt->data != NULL) {
			nrf_802154_buffer_free_raw(rx_pkt->rf_buf);
			rx_pkt->data = NULL;
		}
	}
}
