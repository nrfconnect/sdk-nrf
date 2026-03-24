/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DTM_TWOWIRE_TYPES_H_
#define DTM_TWOWIRE_TYPES_H_

/**
 * @file
 * @defgroup dtm_tw_types Bluetooth DTM 2-wire UART types
 * @{
 * @brief Macros and enums for different values and positions of DTM 2-wire UART packet fields.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Mask of the CTE type in the CTEInfo. */
#define DTM_TW_CTE_TYPE_MASK 0x03

/** @brief Position of the CTE type in the CTEInfo. */
#define DTM_TW_CTE_TYPE_POS 0x06

/** @brief Mask of the CTE Time in the CTEInfo. */
#define DTM_TW_CTE_CTETIME_MASK 0x1F

/** @brief Mask of the Antenna Number. */
#define DTM_TW_ANTENNA_NUMBER_MASK 0x7F

/** @brief Position of the Antenna switch pattern. */
#define DTM_TW_ANTENNA_SWITCH_PATTERN_POS 0x07

/** @brief Mask of the Antenna switch pattern. */
#define DTM_TW_ANTENNA_SWITCH_PATTERN_MASK 0x80

/** @brief Position of power level in the DTM power level set response. */
#define DTM_TW_TRANSMIT_POWER_RESPONSE_LVL_POS 0x01

/** @brief Mask of the power level in the DTM power level set response. */
#define DTM_TW_TRANSMIT_POWER_RESPONSE_LVL_MASK 0x1FE

/** @brief Maximum power level bit in the power level set response. */
#define DTM_TW_TRANSMIT_POWER_MAX_LVL_BIT BIT(0x0A)

/** @brief Minimum power level bit in the power level set response. */
#define DTM_TW_TRANSMIT_POWER_MIN_LVL_BIT BIT(0x09)

/** @brief Event response data shift. */
#define DTM_TW_EVENT_RESPONSE_POS 0x01

/** @brief Event response data mask. */
#define DTM_TW_EVENT_RESPONSE_MASK 0x7FFE

/** @brief DTM command parameter: Upper bits mask. */
#define DTM_TW_UPPER_BITS_MASK 0xC0

/** @brief DTM command parameter: Upper bits position. */
#define DTM_TW_UPPER_BITS_POS 0x04

/** @brief DTM reporting event output: Packet count mask. */
#define DTM_TW_PACKET_COUNT_MASK 0x7FFF

/* Event status response bits for Read Supported variant of LE Test Setup
 * command.
 */
#define DTM_TW_TEST_SETUP_DLE_SUPPORTED                 BIT(1)
#define DTM_TW_TEST_SETUP_2M_PHY_SUPPORTED              BIT(2)
#define DTM_TW_TEST_SETUP_STABLE_MODULATION_SUPPORTED   BIT(3)
#define DTM_TW_TEST_SETUP_CODED_PHY_SUPPORTED           BIT(4)
#define DTM_TW_TEST_SETUP_CTE_SUPPORTED                 BIT(5)
#define DTM_TW_TEST_SETUP_ANTENNA_SWITCH                BIT(6)
#define DTM_TW_TEST_SETUP_AOD_1US_TX                    BIT(7)
#define DTM_TW_TEST_SETUP_AOD_1US_RX                    BIT(8)
#define DTM_TW_TEST_SETUP_AOA_1US_RX                    BIT(9)

/**
 * @enum dtm_tw_cmd_code
 * @brief DTM command codes
 */
enum dtm_tw_cmd_code {
	/* Test Setup Command: Set PHY or modulation, configure upper two bits
	 * of length, request matrix of supported features or request max
	 * values of parameters.
	 */
	DTM_TW_CMD_TEST_SETUP = 0x0,

	/* Receive Command: Start receive test. */
	DTM_TW_CMD_RECEIVER_TEST = 0x1,

	/* Transmit Command: Start transmission test. */
	DTM_TW_CMD_TRANSMITTER_TEST = 0x2,

	/* Test End Command: End test and send packet report. */
	DTM_TW_CMD_TEST_END = 0x3,
};

/**
 * @enum dtm_tw_ctrl_code
 * @brief DTM Test Setup Control codes
 */
enum dtm_tw_ctrl_code {
	/* Reset the packet length upper bits and set the PHY to 1Mbit. */
	DTM_TW_TEST_SETUP_RESET = 0x00,

	/* Set the upper two bits of the length field. */
	DTM_TW_TEST_SETUP_SET_UPPER = 0x01,

	/* Select the PHY to be used for packets. */
	DTM_TW_TEST_SETUP_SET_PHY = 0x02,

	/* Select standard or stable modulation index. */
	DTM_TW_TEST_SETUP_SELECT_MODULATION = 0x03,

	/* Read the supported test case features. */
	DTM_TW_TEST_SETUP_READ_SUPPORTED = 0x04,

	/* Read the max supported time and length for packets. */
	DTM_TW_TEST_SETUP_READ_MAX = 0x05,

	/* Set the Constant Tone Extension info. */
	DTM_TW_TEST_SETUP_CONSTANT_TONE_EXTENSION = 0x06,

	/* Set the Constant Tone Extension slot. */
	DTM_TW_TEST_SETUP_CONSTANT_TONE_EXTENSION_SLOT = 0x07,

	/* Set the Antenna number and switch pattern. */
	DTM_TW_TEST_SETUP_ANTENNA_ARRAY = 0x08,

	/* Set the Transmit power. */
	DTM_TW_TEST_SETUP_TRANSMIT_POWER = 0x09
};

/**
 * @enum dtm_tw_phy_code
 * @brief DTM Test Setup PHY codes
 */
enum dtm_tw_phy_code {
	/* Set PHY for future packets to use 1MBit PHY.
	 * Minimum parameter value.
	 */
	DTM_TW_PHY_1M_MIN_RANGE = 0x04,

	/* Set PHY for future packets to use 1MBit PHY.
	 * Maximum parameter value.
	 */
	DTM_TW_PHY_1M_MAX_RANGE = 0x07,

	/* Set PHY for future packets to use 2MBit PHY.
	 * Minimum parameter value.
	 */
	DTM_TW_PHY_2M_MIN_RANGE = 0x08,

	/* Set PHY for future packets to use 2MBit PHY.
	 * Maximum parameter value.
	 */
	DTM_TW_PHY_2M_MAX_RANGE = 0x0B,

	/* Set PHY for future packets to use coded PHY with S=8.
	 * Minimum parameter value.
	 */
	DTM_TW_PHY_LE_CODED_S8_MIN_RANGE = 0x0C,

	/* Set PHY for future packets to use coded PHY with S=8.
	 * Maximum parameter value.
	 */
	DTM_TW_PHY_LE_CODED_S8_MAX_RANGE = 0x0F,

	/* Set PHY for future packets to use coded PHY with S=2.
	 * Minimum parameter value.
	 */
	DTM_TW_PHY_LE_CODED_S2_MIN_RANGE = 0x10,

	/* Set PHY for future packets to use coded PHY with S=2.
	 * Maximum parameter value.
	 */
	DTM_TW_PHY_LE_CODED_S2_MAX_RANGE = 0x13
};

/**
 * @enum dtm_tw_read_supported_code
 * @brief DTM Test Setup Read supported parameters codes.
 */
enum dtm_tw_read_supported_code {
	/* Read maximum supported Tx Octets. Minimum parameter value. */
	DTM_TW_SUPPORTED_TX_OCTETS_MIN_RANGE = 0x00,

	/* Read maximum supported Tx Octets. Maximum parameter value. */
	DTM_TW_SUPPORTED_TX_OCTETS_MAX_RANGE = 0x03,

	/* Read maximum supported Tx Time. Minimum parameter value. */
	DTM_TW_SUPPORTED_TX_TIME_MIN_RANGE = 0x04,

	/* Read maximum supported Tx Time. Maximum parameter value. */
	DTM_TW_SUPPORTED_TX_TIME_MAX_RANGE = 0x07,

	/* Read maximum supported Rx Octets. Minimum parameter value. */
	DTM_TW_SUPPORTED_RX_OCTETS_MIN_RANGE = 0x08,

	/* Read maximum supported Rx Octets. Maximum parameter value. */
	DTM_TW_SUPPORTED_RX_OCTETS_MAX_RANGE = 0x0B,

	/* Read maximum supported Rx Time. Minimum parameter value. */
	DTM_TW_SUPPORTED_RX_TIME_MIN_RANGE = 0x0C,

	/* Read maximum supported Rx Time. Maximum parameter value. */
	DTM_TW_SUPPORTED_RX_TIME_MAX_RANGE = 0x0F,

	/* Read maximum length of the Constant Tone Extension supported. */
	DTM_TW_SUPPORTED_CTE_LENGTH = 0x10
};

/**
 * @enum dtm_tw_reset_code
 * @brief DTM Test Setup reset code.
 */
enum dtm_tw_reset_code {
	/* Reset. Minimum parameter value. */
	DTM_TW_RESET_MIN_RANGE = 0x00,

	/* Reset. Maximum parameter value. */
	DTM_TW_RESET_MAX_RANGE = 0x03
};

/**
 * @enum dtm_tw_set_upper_bits_code
 * @brief DTM Test Setup upper bits code.
 */
enum dtm_tw_set_upper_bits_code {
	/* Set upper bits. Minimum parameter value. */
	DTM_TW_SET_UPPER_BITS_MIN_RANGE = 0x00,

	/* Set upper bits. Maximum parameter value. */
	DTM_TW_SET_UPPER_BITS_MAX_RANGE = 0x0F
};

/**
 * @enum dtm_tw_modulation_code
 * @brief DTM Test Setup modulation code.
 */
enum dtm_tw_modulation_code {
	/* Set Modulation index to standard. Minimum parameter value. */
	DTM_TW_MODULATION_INDEX_STANDARD_MIN_RANGE = 0x00,

	/* Set Modulation index to standard. Maximum parameter value. */
	DTM_TW_MODULATION_INDEX_STANDARD_MAX_RANGE = 0x03,

	/* Set Modulation index to stable. Minimum parameter value. */
	DTM_TW_MODULATION_INDEX_STABLE_MIN_RANGE = 0x04,

	/* Set Modulation index to stable. Maximum parameter value. */
	DTM_TW_MODULATION_INDEX_STABLE_MAX_RANGE = 0x07
};

/**
 * @enum dtm_tw_feature_read_code
 * @brief DTM Test Setup feature read code.
 */
enum dtm_tw_feature_read_code {
	/* Read test case supported feature. Minimum parameter value. */
	DTM_TW_FEATURE_READ_MIN_RANGE = 0x00,

	/* Read test case supported feature. Maximum parameter value. */
	DTM_TW_FEATURE_READ_MAX_RANGE = 0x03
};

/**
 * @enum dtm_tw_transmit_power_code
 * @brief DTM Test Setup transmit power code.
 */
enum dtm_tw_transmit_power_code {
	/* Minimum supported transmit power level. */
	DTM_TW_TRANSMIT_POWER_LVL_MIN = -127,

	/* Maximum supported transmit power level. */
	DTM_TW_TRANSMIT_POWER_LVL_MAX = 20,

	/* Set minimum transmit power level. */
	DTM_TW_TRANSMIT_POWER_LVL_SET_MIN = 0x7E,

	/* Set maximum transmit power level. */
	DTM_TW_TRANSMIT_POWER_LVL_SET_MAX = 0x7F
};

/**
 * @enum dtm_tw_antenna_number
 * @brief DTM Test Setup antenna number max values.
 */
enum dtm_tw_antenna_number {
	/* Minimum antenna number. */
	DTM_TW_ANTENNA_NUMBER_MIN = 0x01,

	/* Maximum antenna number. */
	DTM_TW_ANTENNA_NUMBER_MAX = 0x4B
};

/**
 * @enum dtm_tw_antenna_pattern
 * @brief DTM Test Setup antenna switching pattern.
 */
enum dtm_tw_antenna_pattern {
	/* Constant Tone Extension: Antenna switch pattern 1, 2, 3 ...N. */
	DTM_TW_ANTENNA_PATTERN_123N123N = 0x00,

	/* Constant Tone Extension: Antenna switch pattern
	 * 1, 2, 3 ...N, N - 1, N - 2, ..., 1, ...
	 */
	DTM_TW_ANTENNA_PATTERN_123N2123 = 0x01
};

/**
 * @enum dtm_tw_cte_type_code
 * @brief DTM Test Setup CTE type code
 */
enum dtm_tw_cte_type_code {
	/* CTE Type Angle of Arrival. */
	DTM_TW_CTE_TYPE_AOA = 0x00,

	/* CTE Type Angle of Departure with 1 us slot. */
	DTM_TW_CTE_TYPE_AOD_1US = 0x01,

	/* CTE Type Angle of Departure with 2 us slot.*/
	DTM_TW_CTE_TYPE_AOD_2US = 0x02
};

/**
 * @enum dtm_tw_cte_slot_code
 * @brief DTM Test Setup CTE slot code
 */
enum dtm_tw_cte_slot_code {
	/* CTE 1 us slot duration. */
	DTM_TW_CTE_SLOT_1US = 0x01,

	/* CTE 2 us slot duration. */
	DTM_TW_CTE_SLOT_2US = 0x02
};

/**
 * @enum dtm_tw_pkt_type
 * @brief DTM TX Test Packet Type code
 */
enum dtm_tw_pkt_type {
	/* PRBS9 bit pattern */
	DTM_TW_PKT_PRBS9 = 0x00,

	/* 11110000 bit pattern (LSB is the leftmost bit). */
	DTM_TW_PKT_0X0F = 0x01,

	/* 10101010 bit pattern (LSB is the leftmost bit). */
	DTM_TW_PKT_0X55 = 0x02,

	/* 11111111 bit pattern for Coded PHY.
	 * Vendor specific command for Non-Coded PHY.
	 */
	DTM_TW_PKT_0XFF_OR_VS = 0x03,
};

/**
 * @enum dtm_tw_channel_code
 * @brief DTM TX/RX Test Channel range
 */
enum dtm_tw_channel_code {
	/* Minimum channel number. */
	DTM_TW_CHANNEL_MIN_RANGE = 0x00,

	/* Maximum channel number. */
	DTM_TW_CHANNEL_MAX_RANGE = 0x27
};

/**
 * @enum dtm_tw_test_end_parameter
 * @brief DTM Test End parameter range
 */
enum dtm_tw_test_end_parameter {
	/* Test End. Minimum parameter value. */
	DTM_TW_TEST_END_MIN_RANGE = 0x00,

	/* Test End. Maximum parameter value. */
	DTM_TW_TEST_END_MAX_RANGE = 0x03
};

/**
 * @enum dtm_tw_event
 * @brief DTM events
 */
enum dtm_tw_event {
	/* Status event, indicating success. */
	DTM_TW_EVENT_TEST_STATUS_SUCCESS = 0x0000,

	/* Status event, indicating an error. */
	DTM_TW_EVENT_TEST_STATUS_ERROR = 0x0001,

	/* Packet reporting event, returned by the device to the tester. */
	DTM_TW_EVENT_PACKET_REPORTING = 0x8000,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* DTM_TWOWIRE_TYPES_H_ */
