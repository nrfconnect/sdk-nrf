/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_COMMON_H
#define DECT_COMMON_H

#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <nrf_modem_dect_phy.h>

#define DECT_MAX_TBS	  5600
#define DECT_DATA_MAX_LEN (DECT_MAX_TBS / 8)

/******************************************************************************/

typedef enum {
	DECT_PHY_HEADER_TYPE1 = 0,
	DECT_PHY_HEADER_TYPE2,
} dect_phy_header_type_t;

typedef enum {
	DECT_PHY_HEADER_FORMAT_000 = 0,
	DECT_PHY_HEADER_FORMAT_001 = 1,
} dect_phy_header_format;

enum dect_phy_packet_length_type {
	DECT_PHY_HEADER_PKT_LENGTH_TYPE_SUBSLOTS = 0,
	DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS = 1,
};

/******************************************************************************/

/**
 * Physical Layer Control Field common part.
 *
 * The part of the Physical Layer Control Field that is common between all its variants.
 * See Section 6.2 of [2].
 */

struct dect_phy_ctrl_field_common {
	uint8_t  packet_length: 4;
	uint8_t  packet_length_type: 1;
	uint8_t  header_format: 3;

	uint8_t  short_network_id: 8;

	uint8_t  transmitter_identity_hi: 8;

	uint8_t  transmitter_identity_lo: 8;

	uint8_t  df_mcs: 3;
	uint8_t  reserved: 1;
	uint8_t  transmit_power: 4;

	uint32_t  pad: 24;
};

/* Table 6.2.1-1: Physical Layer Control Field: Type 1, Header Format: 000 */
struct dect_phy_header_type1_format0_t {
	uint8_t packet_length: 4;
	uint8_t packet_length_type: 1;
	uint8_t format: 3;

	uint8_t short_network_id;

	uint8_t transmitter_identity_hi;
	uint8_t transmitter_identity_lo;

	uint8_t df_mcs: 3;
	uint8_t reserved: 1;
	uint8_t transmit_power: 4;
};
typedef struct {
	uint8_t transmission_feedback0: 1;
	uint8_t harq_process_number0: 3;
	uint8_t format: 4;

	uint8_t CQI: 4;
	uint8_t buffer_status: 4;
} dect_phy_feedback_format_1_t;

typedef struct {
	uint8_t transmission_feedback0: 1;
	uint8_t harq_process_number0: 3;
	uint8_t format: 4;

	uint8_t CQI: 4;
	uint8_t transmission_feedback1: 1;
	uint8_t harq_process_number1: 3;
} dect_phy_feedback_format_3_t;

typedef struct {
	uint8_t harq_feedback_bitmap_proc3: 1;
	uint8_t harq_feedback_bitmap_proc2: 1;
	uint8_t harq_feedback_bitmap_proc1: 1;
	uint8_t harq_feedback_bitmap_proc0: 1;
	uint8_t format: 4;

	uint8_t CQI: 4;
	uint8_t harq_feedback_bitmap_proc7: 1;
	uint8_t harq_feedback_bitmap_proc6: 1;
	uint8_t harq_feedback_bitmap_proc5: 1;
	uint8_t harq_feedback_bitmap_proc4: 1;
} dect_phy_feedback_format_4_t;

typedef struct {
	uint8_t transmission_feedback: 1;
	uint8_t harq_process_number: 3;
	uint8_t format: 4;

	uint8_t codebook_index: 6;
	uint8_t mimo_feedback: 2;
} dect_phy_feedback_format_5_t;

typedef struct {
	uint8_t reserved: 1;
	uint8_t harq_process_number: 3;
	uint8_t format: 4;
	uint8_t CQI: 4;
	uint8_t buffer_status: 4;
} dect_phy_feedback_format_6_t;

typedef union {
	dect_phy_feedback_format_1_t format1;
	dect_phy_feedback_format_3_t format3;
	dect_phy_feedback_format_4_t format4;
	dect_phy_feedback_format_5_t format5;
	dect_phy_feedback_format_6_t format6;
} dect_phy_feedback_t;

/* Table 6.2.1-2: Physical Layer Control Field: Type 2, Header Format: 000 */
struct dect_phy_header_type2_format0_t {
	uint8_t packet_length: 4;
	uint8_t packet_length_type: 1;
	uint8_t format: 3;

	uint8_t short_network_id;

	uint8_t transmitter_identity_hi;
	uint8_t transmitter_identity_lo;

	uint8_t df_mcs: 4;
	uint8_t transmit_power: 4;

	uint8_t receiver_identity_hi;
	uint8_t receiver_identity_lo;

	uint8_t df_harq_process_number: 3;
	uint8_t df_new_data_indication_toggle: 1;
	uint8_t df_redundancy_version: 2;
	uint8_t spatial_streams: 2;

	dect_phy_feedback_t feedback;
};

/* Table 6.2.1-2a: Physical Layer Control Field: Type 2, Header Format: 001 */
struct dect_phy_header_type2_format1_t {
	uint8_t packet_length: 4;
	uint8_t packet_length_type: 1;
	uint8_t format: 3;

	uint8_t short_network_id;

	uint8_t transmitter_identity_hi;
	uint8_t transmitter_identity_lo;

	uint8_t df_mcs: 4;
	uint8_t transmit_power: 4;

	uint8_t receiver_identity_hi;
	uint8_t receiver_identity_lo;

	uint8_t reserved: 6;
	uint8_t spatial_streams: 2;

	dect_phy_feedback_t feedback;
};

/******************************************************************************/

typedef struct {
	/** @brief Network ID (24 bits), ETSI TS 103 636-4 4.2.3.1 */
	uint32_t network_id;

	/** @brief Transmitter Long Radio ID, ETSI TS 103 636-4 4.2.3.2 */
	uint32_t transmitter_long_rd_id;

	/** @brief Receiver Long Radio ID, ETSI TS 103 636-4 4.2.3.2 */
	uint32_t receiver_long_rd_id;

} dect_mac_address_info_t;

/************************************************************************************************/

/* Supported DECT bands. See ETSI TS 103 636-2 v1.3.1 Table 5.4.2-1. (3rd column) */

#define DECT_PHY_SUPPORTED_CHANNEL_BAND1_MIN 1657
#define DECT_PHY_SUPPORTED_CHANNEL_BAND1_MAX 1677

#define DECT_PHY_SUPPORTED_CHANNEL_BAND2_MIN 1680
#define DECT_PHY_SUPPORTED_CHANNEL_BAND2_MAX 1700

#define DECT_PHY_SUPPORTED_CHANNEL_BAND4_MIN 524
#define DECT_PHY_SUPPORTED_CHANNEL_BAND4_MAX 552

#define DECT_PHY_SUPPORTED_CHANNEL_BAND9_MIN 1703
#define DECT_PHY_SUPPORTED_CHANNEL_BAND9_MAX 1711

#define DECT_PHY_SUPPORTED_CHANNEL_BAND22_MIN 1691
#define DECT_PHY_SUPPORTED_CHANNEL_BAND22_MAX 1711

#define DESH_DECT_PHY_SUPPORTED_BAND_COUNT 5

/************************************************************************************************/

#define DECT_PHY_CLASS_1_MAX_TX_POWER_DBM 23
#define DECT_PHY_CLASS_2_MAX_TX_POWER_DBM 21
#define DECT_PHY_CLASS_3_MAX_TX_POWER_DBM 19
#define DECT_PHY_CLASS_4_MAX_TX_POWER_DBM 10

/************************************************************************************************/

#define US_TO_MODEM_TICKS(x) ((uint64_t)(((x) * NRF_MODEM_DECT_MODEM_TIME_TICK_RATE_KHZ) / 1000))
#define MS_TO_MODEM_TICKS(x) ((uint64_t)((x) * NRF_MODEM_DECT_MODEM_TIME_TICK_RATE_KHZ))
#define SECONDS_TO_MODEM_TICKS(s) (((uint64_t)(s)) * 1000 * NRF_MODEM_DECT_MODEM_TIME_TICK_RATE_KHZ)

#define MODEM_TICKS_TO_MS(x) (((double)(x)) / NRF_MODEM_DECT_MODEM_TIME_TICK_RATE_KHZ)

#define DECT_SHORT_RD_ID_BROADCAST ((uint16_t)(0xFFFF))
#define DECT_LONG_RD_ID_BROADCAST  ((uint32_t)(0xFFFFFFFF))

#define DECT_RADIO_FRAME_SUBSLOT_COUNT_IN_SLOT (2)
#define DECT_RADIO_FRAME_SLOT_COUNT	       (24)

#define DECT_RADIO_FRAME_SUBSLOT_COUNT                                                             \
	(DECT_RADIO_FRAME_SUBSLOT_COUNT_IN_SLOT * DECT_RADIO_FRAME_SLOT_COUNT)
#define DECT_RADIO_FRAME_DURATION_US		(10000)
#define DECT_RADIO_FRAME_DURATION_MS		(10)
#define DECT_RADIO_SLOT_DURATION_US		((double)DECT_RADIO_FRAME_DURATION_US / 24)
#define DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS (US_TO_MODEM_TICKS(DECT_RADIO_SLOT_DURATION_US))
#define DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS                                                 \
	((DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS) / 2) /* Note: assumes that mu = 1 */
#define DECT_RADIO_FRAME_DURATION_IN_MODEM_TICKS (MS_TO_MODEM_TICKS(DECT_RADIO_FRAME_DURATION_MS))

#define DECT_PHY_TX_RX_SCHEDULING_OFFSET_MDM_TICKS (2 * DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS)

#define MS_TO_SUBSLOTS(x) (((x) / 10) * 48)

#define DECT_RADIO_FRAME_SYMBOL_COUNT \
	((NRF_MODEM_DECT_MODEM_TIME_TICK_RATE_KHZ * 10) / NRF_MODEM_DECT_SYMBOL_DURATION)

#define DECT_RADIO_SLOT_SYMBOL_COUNT (DECT_RADIO_FRAME_SYMBOL_COUNT / DECT_RADIO_FRAME_SLOT_COUNT)
#define DECT_RADIO_SUBSLOT_SYMBOL_COUNT (DECT_RADIO_SLOT_SYMBOL_COUNT / 2)

/******************************************************************************/
#endif /* DECT_COMMON_H */
