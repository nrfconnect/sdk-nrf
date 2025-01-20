/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_COMMON_H
#define DECT_PHY_COMMON_H

#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <nrf_modem_dect_phy.h>
#include "dect_common.h"

/******************************************************************************/

#define DECT_PHY_LBT_PERIOD_MAX_SYM (NRF_MODEM_DECT_LBT_PERIOD_MAX / NRF_MODEM_DECT_SYMBOL_DURATION)
#define DECT_PHY_LBT_PERIOD_MIN_SYM (NRF_MODEM_DECT_LBT_PERIOD_MIN / NRF_MODEM_DECT_SYMBOL_DURATION)

/***********************************************************************************************/

struct dect_phy_op_event_msgq_item {
	uint8_t id;
	void *data;
};

/******************************************************************************/

struct dect_phy_common_harq_feedback_data {
	struct nrf_modem_dect_phy_tx_params params;
	struct dect_phy_header_type2_format1_t header;
	uint8_t encoded_data_to_send[DECT_DATA_MAX_LEN]; /* DECT_MAC_MESSAGE_TYPE_HARQ_FEEDBACK */
};

struct dect_phy_common_op_pcc_rcv_params {
	uint64_t time;
	uint64_t stf_start_time;
	struct nrf_modem_dect_phy_pcc_event pcc_status;

	uint8_t phy_len;
	enum dect_phy_packet_length_type phy_len_type;
	union nrf_modem_dect_phy_hdr phy_header;

	uint8_t short_nw_id;
	uint16_t transmitter_short_rd_id;
	uint16_t receiver_short_rd_id;
	uint8_t mcs;
	int16_t pwr_dbm;
};

struct dect_phy_common_op_pcc_crc_fail_params {
	uint64_t time;
	struct nrf_modem_dect_phy_pcc_crc_failure_event crc_failure;
};
struct dect_phy_common_op_pdc_crc_fail_params {
	uint64_t time;
	struct nrf_modem_dect_phy_pdc_crc_failure_event crc_failure;
};

struct dect_phy_commmon_op_pdc_rcv_params {
	struct nrf_modem_dect_phy_pdc_event rx_status;

	uint16_t rx_channel;

	uint8_t last_received_pcc_short_nw_id;
	uint16_t last_received_pcc_transmitter_short_rd_id;
	uint8_t last_received_pcc_phy_len;
	enum dect_phy_packet_length_type last_received_pcc_phy_len_type;

	uint64_t time;
	uint32_t data_length;

	int16_t rx_rssi_level_dbm;
	int16_t rx_pwr_dbm;
	uint8_t rx_mcs;

	uint8_t data[DECT_DATA_MAX_LEN];
};

struct dect_phy_common_op_completed_params {
	uint64_t time;
	uint32_t handle;
	int16_t temperature;
	enum nrf_modem_dect_phy_err status;
};

struct dect_phy_data_rcv_common_params {
	uint64_t time;
	int16_t snr;
	int16_t rssi;
	uint8_t mcs;

	int16_t rx_rssi_dbm;
	int16_t rx_pwr_dbm;
	uint16_t data_length;
	void *data;
};

/******************************************************************************/

#define ANSI_RESET_ALL	  "\x1b[0m"
#define ANSI_COLOR_RED	  "\x1b[31m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_GREEN  "\x1b[32m"
#define ANSI_COLOR_BLUE   "\x1b[34m"

#define DECT_TX_STATUS_LED		 DK_LED1 /* Thingy91x: red */
#define DECT_BEACON_ON_STATUS_LED	 DK_LED2 /* Thingy91x: blue */
#define DECT_CLIENT_CONNECTED_STATUS_LED DK_LED3 /* Thingy91x: green */
#define DECT_CTRL_BATTERY_LOW_STATUS_LED DK_LED1 /* Thingy91x: red */

#define DECT_CTRL_DK_ALL_LEDS_MSK (DK_ALL_LEDS_MSK)

/******************************************************************************/

#endif /* DECT_PHY_COMMON_H */
