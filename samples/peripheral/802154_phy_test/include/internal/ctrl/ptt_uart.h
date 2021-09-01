/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: uart transport interface intended for internal usage on library
 *          level
 */

#ifndef PTT_UART_H__
#define PTT_UART_H__

#include "ptt_errors.h"
#include "ptt_uart_api.h"

#define PTT_VALUE_ON (1u)
#define PTT_VALUE_OFF (0u)

/* commands text */
#define UART_CMD_R_PING_TEXT "custom rping"
#define UART_CMD_L_PING_TIMEOUT_TEXT "custom lpingtimeout"
#define UART_CMD_SETCHANNEL_TEXT "custom setchannel"
#define UART_CMD_L_SETCHANNEL_TEXT "custom lsetchannel"
#define UART_CMD_R_SETCHANNEL_TEXT "custom rsetchannel"
#define UART_CMD_L_GET_CHANNEL_TEXT "custom lgetchannel"
#define UART_CMD_L_SET_POWER_TEXT "custom lsetpower"
#define UART_CMD_R_SET_POWER_TEXT "custom rsetpower"
#define UART_CMD_L_GET_POWER_TEXT "custom lgetpower"
#define UART_CMD_R_GET_POWER_TEXT "custom rgetpower"
#define UART_CMD_R_STREAM_TEXT "custom rstream"
#define UART_CMD_R_START_TEXT "custom rstart"
#define UART_CMD_R_END_TEXT "custom rend"
#define UART_CMD_L_REBOOT_TEXT "custom lreboot"
#define UART_CMD_FIND_TEXT "custom find"
#define UART_CMD_R_HW_VERSION_TEXT "custom rhardwareversion"
#define UART_CMD_R_SW_VERSION_TEXT "custom rsoftwareversion"
#define UART_CMD_CHANGE_MODE_TEXT "custom changemode"
#define UART_CMD_L_GET_CCA_TEXT "custom lgetcca"
#define UART_CMD_L_SET_CCA_TEXT "custom lsetcca"
#define UART_CMD_L_GET_ED_TEXT "custom lgeted"
#define UART_CMD_L_GET_LQI_TEXT "custom lgetlqi"
#define UART_CMD_L_GET_RSSI_TEXT "custom lgetrssi"
#define UART_CMD_L_SET_SHORT_TEXT "custom lsetshort"
#define UART_CMD_L_SET_EXTENDED_TEXT "custom lsetextended"
#define UART_CMD_L_SET_PAN_ID_TEXT "custom lsetpanid"
#define UART_CMD_L_SET_PAYLOAD_TEXT "custom lsetpayload"
#define UART_CMD_L_START_TEXT "custom lstart"
#define UART_CMD_L_END_TEXT "custom lend"
#define UART_CMD_L_SET_ANTENNA_TEXT "custom lsetantenna"
#define UART_CMD_L_SET_TX_ANTENNA_TEXT "custom lsettxantenna"
#define UART_CMD_L_SET_RX_ANTENNA_TEXT "custom lsetrxantenna"
#define UART_CMD_L_GET_RX_ANTENNA_TEXT "custom lgetrxantenna"
#define UART_CMD_L_GET_TX_ANTENNA_TEXT "custom lgettxantenna"
#define UART_CMD_L_GET_LAST_BEST_RX_ANTENNA_TEXT "custom lgetbestrxantenna"
#define UART_CMD_R_SET_ANTENNA_TEXT "custom rsetantenna"
#define UART_CMD_R_SET_TX_ANTENNA_TEXT "custom rsettxantenna"
#define UART_CMD_R_SET_RX_ANTENNA_TEXT "custom rsetrxantenna"
#define UART_CMD_R_GET_RX_ANTENNA_TEXT "custom rgetrxantenna"
#define UART_CMD_R_GET_TX_ANTENNA_TEXT "custom rgettxantenna"
#define UART_CMD_R_GET_LAST_BEST_RX_ANTENNA_TEXT "custom rgetbestrxantenna"
#define UART_CMD_L_TX_TEXT "custom ltx"
#define UART_CMD_L_CLK_TEXT "custom lclk"
#define UART_CMD_L_SET_GPIO_TEXT "custom lsetgpio"
#define UART_CMD_L_GET_GPIO_TEXT "custom lgetgpio"
#define UART_CMD_L_TX_END_TEXT "custom ltxend"
#define UART_CMD_L_CARRIER_TEXT "custom lcarrier"
#define UART_CMD_L_STREAM_TEXT "custom lstream"
#define UART_CMD_L_SET_DCDC_TEXT "custom lsetdcdc"
#define UART_CMD_L_GET_DCDC_TEXT "custom lgetdcdc"
#define UART_CMD_L_SET_ICACHE_TEXT "custom lseticache"
#define UART_CMD_L_GET_ICACHE_TEXT "custom lgeticache"
#define UART_CMD_L_GET_TEMP_TEXT "custom lgettemp"
#define UART_CMD_L_INDICATION_TEXT "custom lindication"
#define UART_CMD_L_SLEEP_TEXT "custom lsleep"
#define UART_CMD_L_RECEIVE_TEXT "custom lreceive"

/* commands payload in space separated words */
#define UART_CMD_R_PING_PAYLOAD_L 0
#define UART_CMD_L_PING_TIMEOUT_PAYLOAD_L 2
#define UART_CMD_SETCHANNEL_PAYLOAD_L 4
#define UART_CMD_L_SETCHANNEL_PAYLOAD_L 4
#define UART_CMD_R_SETCHANNEL_PAYLOAD_L 1
#define UART_CMD_L_GET_CHANNEL_PAYLOAD_L 0
#define UART_CMD_L_SET_POWER_PAYLOAD_L 3
#define UART_CMD_R_SET_POWER_PAYLOAD_L 3
#define UART_CMD_L_GET_POWER_PAYLOAD_L 0
#define UART_CMD_R_GET_POWER_PAYLOAD_L 0
#define UART_CMD_R_STREAM_PAYLOAD_L 2
#define UART_CMD_R_START_PAYLOAD_L 0
#define UART_CMD_R_END_PAYLOAD_L 0
#define UART_CMD_L_REBOOT_PAYLOAD_L 0
#define UART_CMD_FIND_PAYLOAD_L 0
#define UART_CMD_R_HW_VERSION_PAYLOAD_L 0
#define UART_CMD_R_SW_VERSION_PAYLOAD_L 0
#define UART_CMD_CHANGE_MODE_PAYLOAD_L 1
#define UART_CMD_L_GET_CCA_PAYLOAD_L 1
#define UART_CMD_L_SET_CCA_PAYLOAD_L 1
#define UART_CMD_L_GET_ED_PAYLOAD_L 0
#define UART_CMD_L_GET_LQI_PAYLOAD_L 0
#define UART_CMD_L_GET_RSSI_PAYLOAD_L 0
#define UART_CMD_L_SET_SHORT_PAYLOAD_L 1
#define UART_CMD_L_SET_EXTENDED_PAYLOAD_L 1
#define UART_CMD_L_SET_PAN_ID_PAYLOAD_L 1
#define UART_CMD_L_SET_PAYLOAD_PAYLOAD_L 2
#define UART_CMD_L_START_PAYLOAD_L 0
#define UART_CMD_L_END_PAYLOAD_L 0
#define UART_CMD_L_SET_ANTENNA_PAYLOAD_L 1
#define UART_CMD_L_SET_TX_ANTENNA_PAYLOAD_L 1
#define UART_CMD_L_SET_RX_ANTENNA_PAYLOAD_L 1
#define UART_CMD_L_GET_RX_ANTENNA_PAYLOAD_L 0
#define UART_CMD_L_GET_TX_ANTENNA_PAYLOAD_L 0
#define UART_CMD_L_GET_LAST_BEST_RX_ANTENNA_PAYLOAD_L 0
#define UART_CMD_R_SET_ANTENNA_PAYLOAD_L 1
#define UART_CMD_R_SET_TX_ANTENNA_PAYLOAD_L 1
#define UART_CMD_R_SET_RX_ANTENNA_PAYLOAD_L 1
#define UART_CMD_R_GET_RX_ANTENNA_PAYLOAD_L 0
#define UART_CMD_R_GET_TX_ANTENNA_PAYLOAD_L 0
#define UART_CMD_R_GET_LAST_BEST_RX_ANTENNA_PAYLOAD_L 0
#define UART_CMD_L_TX_PAYLOAD_L 2
#define UART_CMD_L_CLK_PAYLOAD_L 2
#define UART_CMD_L_SET_GPIO_PAYLOAD_L 2
#define UART_CMD_L_GET_GPIO_PAYLOAD_L 1
#define UART_CMD_L_TX_END_PAYLOAD_L 0
#define UART_CMD_L_CARRIER_PAYLOAD_L 3
#define UART_CMD_L_STREAM_PAYLOAD_L 3
#define UART_CMD_L_SET_DCDC_PAYLOAD_L 1
#define UART_CMD_L_GET_DCDC_PAYLOAD_L 0
#define UART_CMD_L_SET_ICACHE_PAYLOAD_L 1
#define UART_CMD_L_GET_ICACHE_PAYLOAD_L 0
#define UART_CMD_L_GET_TEMP_PAYLOAD_L 0
#define UART_CMD_L_INDICATION_PAYLOAD_L 1
#define UART_CMD_L_SLEEP_PAYLOAD_L 0
#define UART_CMD_L_RECEIVE_PAYLOAD_L 0

#define UART_TEXT_PAYLOAD_DELIMETERS " "

#define UART_CMD_SET_POWER_VALUE_PLACE 2

#define UART_SET_PAN_ID_SYM_NUM 6
#define UART_SET_SHORT_SYM_NUM 6
#define UART_SET_EXTENDED_SYM_NUM 18

/** UART commands */
enum ptt_uart_cmd {
	PTT_UART_CMD_R_PING = 0x00, /**< send ping to DUT */
	PTT_UART_CMD_L_PING_TIMEOUT, /**< set ping timeout to CMD */
	PTT_UART_CMD_SETCHANNEL, /**< set channel to CMD and DUT */
	PTT_UART_CMD_L_SETCHANNEL, /**< set channel to CMD */
	PTT_UART_CMD_R_SETCHANNEL, /**< set channel to DUT */
	PTT_UART_CMD_L_GET_CHANNEL, /**< get channel from CMD */
	PTT_UART_CMD_L_SET_POWER, /**< set power to CMD */
	PTT_UART_CMD_R_SET_POWER, /**< set power to DUT */
	PTT_UART_CMD_L_GET_POWER, /**< get power from CMD */
	PTT_UART_CMD_R_GET_POWER, /**< get power from DUT */
	PTT_UART_CMD_R_STREAM, /**< DUT transmits a stream */
	PTT_UART_CMD_R_START, /**< DUT starts RX test */
	PTT_UART_CMD_R_END, /**< DUT ends RX test and sends a report */
	PTT_UART_CMD_L_REBOOT, /**< CMD reboot local device */
	PTT_UART_CMD_FIND, /**< CMD starts find procedure */
	PTT_UART_CMD_R_HW_VERSION, /**< get HW version from DUT */
	PTT_UART_CMD_R_SW_VERSION, /**< get SW version from DUT */
	PTT_UART_CMD_CHANGE_MODE, /**< change device mode */
	PTT_UART_CMD_L_GET_CCA, /**< CMD will perform CCA with selected mode and print result*/
	PTT_UART_CMD_L_SET_CCA, /**< CMD will toggle performing CCA prior to TX*/
	PTT_UART_CMD_L_GET_ED, /**< CMD will perform ED and print out the result */
	PTT_UART_CMD_L_GET_LQI,
	/**< CMD will put the device into receive mode and wait for reception of a single packet */
	PTT_UART_CMD_L_GET_RSSI, /**< CMD will measure RSSI and print result */
	PTT_UART_CMD_L_SET_SHORT, /**< CMD will set its short address */
	PTT_UART_CMD_L_SET_EXTENDED, /**< CMD will set its extended address */
	PTT_UART_CMD_L_SET_PAN_ID, /**< CMD will set its PAN Id */
	PTT_UART_CMD_L_SET_PAYLOAD, /**< CMD will set payload for next ltx commands */
	PTT_UART_CMD_L_START,
	/**< CMD will put the DUT in a continuous receive mode, where each packet received */
	/** is displayed in turn */
	PTT_UART_CMD_L_END,
	/**< CMD will stop the continuous receive mode, if active. If continuous receive mode */
	/** is not active, the command is ignored. */
	PTT_UART_CMD_L_SET_ANTENNA, /**< CMD will set antenna device is sending frames with. */
	PTT_UART_CMD_L_SET_TX_ANTENNA, /**< CMD will set TX antenna. */
	PTT_UART_CMD_L_SET_RX_ANTENNA, /**< CMD will set RX antenna. */
	PTT_UART_CMD_L_GET_RX_ANTENNA, /**< CMD will print out the rx antenna used by the device */
	PTT_UART_CMD_L_GET_TX_ANTENNA, /**< CMD will print out the tx antenna used by the device */
	PTT_UART_CMD_L_GET_LAST_BEST_RX_ANTENNA,
	PTT_UART_CMD_R_SET_ANTENNA, /**< CMD will set antenna DUT device is sending frames with. */
	PTT_UART_CMD_R_SET_TX_ANTENNA, /**< CMD will set TX antenna for DUT. */
	PTT_UART_CMD_R_SET_RX_ANTENNA, /**< CMD will set RX antenna for DUT. */
	PTT_UART_CMD_R_GET_RX_ANTENNA, /**< CMD will print out the RX antenna used by the DUT. */
	PTT_UART_CMD_R_GET_TX_ANTENNA, /**< CMD will print out the TX antenna used by the DUT. */
	PTT_UART_CMD_R_GET_LAST_BEST_RX_ANTENNA,
	PTT_UART_CMD_L_TX,
	/**< CMD will transmit a number of raw 802.15.4 frames with */
	/** payload previously set by “custom lsetpayload” */
	PTT_UART_CMD_L_CLK, /**< CMD will output HFCLK to GPIO pin */
	PTT_UART_CMD_L_SET_GPIO, /**< CMD will set value to GPIO pin */
	PTT_UART_CMD_L_GET_GPIO, /**< CMD will get value from GPIO pin */
	PTT_UART_CMD_L_TX_END,
	/**< CMD will stop ltx command processing if ltx started with infinite number of packets */
	PTT_UART_CMD_L_CARRIER, /**< CMD will emit carrier signal without modulation */
	PTT_UART_CMD_L_STREAM,
	/**< CMD will emit modulated signal with payload set by “custom lsetpayload”*/
	PTT_UART_CMD_L_SET_DCDC, /**< CMD will disable/enable DC/DC mode */
	PTT_UART_CMD_L_GET_DCDC, /**< CMD will print current DC/DC mode */
	PTT_UART_CMD_L_SET_ICACHE, /**< CMD will disable/enable ICACHE */
	PTT_UART_CMD_L_GET_ICACHE, /**< CMD will print current state of ICACHE */
	PTT_UART_CMD_L_GET_TEMP, /**< CMD will print out the SoC temperature */
	PTT_UART_CMD_L_INDICATION, /**< CMD will control LED indicating received packet */
	PTT_UART_CMD_L_SLEEP, /**< CMD will put radio into sleep mode */
	PTT_UART_CMD_L_RECEIVE, /**< CMD will put radio into receive mode*/
	PTT_UART_CMD_N, /**< command count */
};

/** Actions to perform with payload before sending it to command handler */
enum ptt_uart_payload {
	/**< payload will be parsed as bytes separated by @ref UART_TEXT_PAYLOAD_DELIMETERS*/
	/** will be send to command handler */
	PTT_UART_PAYLOAD_PARSED_AS_BYTES,
	/**< payload will be written as raw ASCII and command handlerwill be */
	/** responsible for parsing actions types count */
	PTT_UART_PAYLOAD_RAW,
	PTT_UART_PAYLOAD_N,
};

/** command name and additional info for text command parsing */
struct uart_text_cmd_s {
	const char *name; /**< string containing command name */
	enum ptt_uart_cmd code; /**< command code */
	uint8_t payload_len; /**< payload length in space separated words */
	enum ptt_uart_payload payload_type;
	/**< action to perform with payload before sending it to command handler\ */
};

/** @brief Send a packet over UART
 *
 *  @param pkt - data to send
 *  @param len - length of pkt
 *
 *  @return PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_uart_send_packet(const uint8_t *pkt, ptt_pkt_len_t len);

/** @brief Send @ref PTT_UART_PROMPT_STR over UART
 *
 *  @param - none
 *
 *  @return - none
 */
void ptt_uart_print_prompt(void);

/** @brief Notify UART module a handler is busy
 *
 *  @param - none
 *
 *  @return - none
 */
void ptt_handler_busy(void);

/** @brief Notify UART module a handler is free
 *
 *  @param - none
 *
 *  @return - none
 */
void ptt_handler_free(void);

#endif /* PTT_UART_H__ */
