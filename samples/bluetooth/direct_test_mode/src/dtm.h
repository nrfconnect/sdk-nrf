/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef DTM_H_
#define DTM_H_

#include <stdbool.h>
#include <zephyr/types.h>
#include <devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DTM command codes */
enum dtm_cmd_code {
	/* Test Setup Command: Set PHY or modulation, configure upper two bits
	 * of length, request matrix of supported features or request max
	 * values of parameters.
	 */
	LE_TEST_SETUP = 0x0,

	/* Receive Command: Start receive test. */
	LE_RECEIVER_TEST = 0x1,

	/* Transmit Command: Start transmission test. */
	LE_TRANSMITTER_TEST = 0x2,

	/* Test End Command: End test and send packet report. */
	LE_TEST_END = 0x3,
};

/* DTM Test Setup Control codes */
enum dtm_ctrl_code {
	/* Reset the packet length upper bits and set the PHY to 1Mbit. */
	LE_TEST_SETUP_RESET = 0x00,

	/* Set the upper two bits of the length field. */
	LE_TEST_SETUP_SET_UPPER = 0x01,

	/* Select the PHY to be used for packets. */
	LE_TEST_SETUP_SET_PHY = 0x02,

	/* Select standard or stable modulation index. Stable modulation index
	 * is not supported.
	 */
	LE_TEST_SETUP_SELECT_MODULATION = 0x03,

	/* Read the supported test case features. */
	LE_TEST_SETUP_READ_SUPPORTED = 0x04,

	/* Read the max supported time and length for packets. */
	LE_TEST_SETUP_READ_MAX = 0x05,
};

/* DTM Test Setup PHY codes */
enum dtm_phy_code {
	/* Set PHY for future packets to use 1MBit PHY. */
	LE_PHY_1M = 0x01,

	/* Set PHY for future packets to use 2MBit PHY. */
	LE_PHY_2M = 0x02,

	/* Set PHY for future packets to use coded PHY with S=8. */
	LE_PHY_LE_CODED_S8 = 0x03,

	/* Set PHY for future packets to use coded PHY with S=2. */
	LE_PHY_LE_CODED_S2 = 0x04,
};


/* Vendor Specific DTM subcommand for Transmitter Test command.
 * It replaces Frequency field and must be combined with DTM_PKT_0XFF_OR_VS
 * packet type.
 */
enum dtm_vs_subcmd {
	/* Length=0 indicates a constant, unmodulated carrier until LE_TEST_END
	 * or LE_RESET
	 */
	CARRIER_TEST = 0,

	/* nRFgo Studio uses value 1 in length field, to indicate a constant,
	 * unmodulated carrier until LE_TEST_END or LE_RESET
	 */
	CARRIER_TEST_STUDIO = 1,

	/* Set transmission power, value -40..+4 dBm in steps of 4 */
	SET_TX_POWER = 2,

};

/* DTM Packet Type field */
enum dtm_pkt_type {
	/* PRBS9 bit pattern */
	DTM_PKT_PRBS9 = 0x00,

	/* 11110000 bit pattern (LSB is the leftmost bit). */
	DTM_PKT_0X0F = 0x01,

	/* 10101010 bit pattern (LSB is the leftmost bit). */
	DTM_PKT_0X55 = 0x02,

	/* 11111111 bit pattern for Coded PHY.
	 * Vendor specific command for Non-Coded PHY.
	 */
	DTM_PKT_0XFF_OR_VS = 0x03,
};

/* DTM events */
enum dtm_evt {
	/* Status event, indicating success. */
	LE_TEST_STATUS_EVENT_SUCCESS = 0x0000,

	/* Status event, indicating an error. */
	LE_TEST_STATUS_EVENT_ERROR = 0x0001,

	/* Packet reporting event, returned by the device to the tester. */
	LE_PACKET_REPORTING_EVENT = 0x8000,
};

/* DTM error codes */
enum dtm_err_code {
	/* Indicate that the DTM function completed with success. */
	DTM_SUCCESS,

	/* Physical channel number must be in the range 0..39. */
	DTM_ERROR_ILLEGAL_CHANNEL,

	/* Sequencing error: Command is not valid now. */
	DTM_ERROR_INVALID_STATE,

	/* Payload size must be in the range 0..37. */
	DTM_ERROR_ILLEGAL_LENGTH,

	/* Parameter out of range (legal range is function dependent). */
	DTM_ERROR_ILLEGAL_CONFIGURATION,

	/* DTM module has not been initialized by the application. */
	DTM_ERROR_UNINITIALIZED,
};

#if DT_NODE_HAS_PROP(DT_NODELABEL(uart0), current_speed)
/* UART Baudrate used to communicate with the DTM library. */
#define DTM_UART_BAUDRATE DT_PROP(DT_NODELABEL(uart0), current_speed)

/* The UART poll cycle in micro seconds.
 * A baud rate of e.g. 19200 bits / second, and 8 data bits, 1 start/stop bit,
 * no flow control, give the time to transmit a byte:
 * 10 bits * 1/19200 = approx: 520 us. To ensure no loss of bytes,
 * the UART should be polled every 260 us.
 */
#define DTM_UART_POLL_CYCLE ((uint32_t) (10*1e6 / DTM_UART_BAUDRATE / 2))
#else
#error "DTM UART node not found"
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(uart0), currrent_speed) */

/**@brief Function for initializing DTM module.
 *
 *  @return 0 in case of success or negative value in case of error
 */
int dtm_init(void);


/**@brief Function for giving control to DTM module for handling timer and
 *        radio events. It will return to caller at 625us intervals.
 *
 * @return Time counter, incremented every 625 us.
 */
uint32_t dtm_wait(void);


/**@brief Function for calling when a complete command has been prepared by the
 *        Tester.
 *
 * @param[in]   cmd  2-byte DTM command
 *
 * @return      DTM_SUCCESS or one of the DTM_ERROR_ values
 */
enum dtm_err_code dtm_cmd_put(uint16_t cmd);


/**@brief Function for reading the result of a DTM command.
 *
 * @param[out]  dtm_event   16 bit event code according to DTM standard.
 *
 * @return      true: new event
 *              false: no event since last call
 */
bool dtm_event_get(uint16_t *dtm_event);


#ifdef __cplusplus
}
#endif

#endif /* DTM_H_ */

/** @} */
