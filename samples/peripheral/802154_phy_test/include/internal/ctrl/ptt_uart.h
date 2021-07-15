/* Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Use in source and binary forms, redistribution in binary form only, with
 * or without modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 2. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 3. This software, with or without modification, must only be used with a Nordic
 *    Semiconductor ASA integrated circuit.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Purpose: uart transport interface intended for internal usage on library
 *          level
 */

#ifndef PTT_UART_H__
#define PTT_UART_H__

#include "ptt_errors.h"
#include "ptt_uart_api.h"

#define PTT_VALUE_ON                      (1u)
#define PTT_VALUE_OFF                     (0u)

/* commands text */
#define UART_CMD_R_PING_TEXT              "custom rping"
#define UART_CMD_L_PING_TIMEOUT_TEXT      "custom lpingtimeout"
#define UART_CMD_SETCHANNEL_TEXT          "custom setchannel"
#define UART_CMD_L_SETCHANNEL_TEXT        "custom lsetchannel"
#define UART_CMD_R_SETCHANNEL_TEXT        "custom rsetchannel"
#define UART_CMD_L_GET_CHANNEL_TEXT       "custom lgetchannel"
#define UART_CMD_L_SET_POWER_TEXT         "custom lsetpower"
#define UART_CMD_R_SET_POWER_TEXT         "custom rsetpower"
#define UART_CMD_L_GET_POWER_TEXT         "custom lgetpower"
#define UART_CMD_R_GET_POWER_TEXT         "custom rgetpower"
#define UART_CMD_R_STREAM_TEXT            "custom rstream"
#define UART_CMD_R_START_TEXT             "custom rstart"
#define UART_CMD_R_END_TEXT               "custom rend"
#define UART_CMD_L_REBOOT_TEXT            "custom lreboot"
#define UART_CMD_FIND_TEXT                "custom find"
#define UART_CMD_R_HW_VERSION_TEXT        "custom rhardwareversion"
#define UART_CMD_R_SW_VERSION_TEXT        "custom rsoftwareversion"
#define UART_CMD_CHANGE_MODE_TEXT         "custom changemode"
#define UART_CMD_L_GET_CCA_TEXT           "custom lgetcca"
#define UART_CMD_L_GET_ED_TEXT            "custom lgeted"
#define UART_CMD_L_GET_LQI_TEXT           "custom lgetlqi"
#define UART_CMD_L_GET_RSSI_TEXT          "custom lgetrssi"
#define UART_CMD_L_SET_SHORT_TEXT         "custom lsetshort"
#define UART_CMD_L_SET_EXTENDED_TEXT      "custom lsetextended"
#define UART_CMD_L_SET_PAN_ID_TEXT        "custom lsetpanid"
#define UART_CMD_L_SET_PAYLOAD_TEXT       "custom lsetpayload"
#define UART_CMD_L_START_TEXT             "custom lstart"
#define UART_CMD_L_END_TEXT               "custom lend"
#define UART_CMD_L_SET_ANTENNA_TEXT       "custom lsetantenna"
#define UART_CMD_L_GET_ANTENNA_TEXT       "custom lgetantenna"
#define UART_CMD_R_SET_ANTENNA_TEXT       "custom rsetantenna"
#define UART_CMD_R_GET_ANTENNA_TEXT       "custom rgetantenna"
#define UART_CMD_L_TX_TEXT                "custom ltx"
#define UART_CMD_L_CLK_TEXT               "custom lclk"
#define UART_CMD_L_SET_GPIO_TEXT          "custom lsetgpio"
#define UART_CMD_L_GET_GPIO_TEXT          "custom lgetgpio"
#define UART_CMD_L_TX_END_TEXT            "custom ltxend"
#define UART_CMD_L_CARRIER_TEXT           "custom lcarrier"
#define UART_CMD_L_STREAM_TEXT            "custom lstream"
#define UART_CMD_L_SET_DCDC_TEXT          "custom lsetdcdc"
#define UART_CMD_L_GET_DCDC_TEXT          "custom lgetdcdc"
#define UART_CMD_L_SET_ICACHE_TEXT        "custom lseticache"
#define UART_CMD_L_GET_ICACHE_TEXT        "custom lgeticache"
#define UART_CMD_L_GET_TEMP_TEXT          "custom lgettemp"
#define UART_CMD_L_INDICATION_TEXT        "custom lindication"
#define UART_CMD_L_SLEEP_TEXT             "custom lsleep"
#define UART_CMD_L_RECEIVE_TEXT           "custom lreceive"

/* commands payload in space separated words */
#define UART_CMD_R_PING_PAYLOAD_L         0
#define UART_CMD_L_PING_TIMEOUT_PAYLOAD_L 2
#define UART_CMD_SETCHANNEL_PAYLOAD_L     4
#define UART_CMD_L_SETCHANNEL_PAYLOAD_L   4
#define UART_CMD_R_SETCHANNEL_PAYLOAD_L   1
#define UART_CMD_L_GET_CHANNEL_PAYLOAD_L  0
#define UART_CMD_L_SET_POWER_PAYLOAD_L    3
#define UART_CMD_R_SET_POWER_PAYLOAD_L    3
#define UART_CMD_L_GET_POWER_PAYLOAD_L    0
#define UART_CMD_R_GET_POWER_PAYLOAD_L    0
#define UART_CMD_R_STREAM_PAYLOAD_L       2
#define UART_CMD_R_START_PAYLOAD_L        0
#define UART_CMD_R_END_PAYLOAD_L          0
#define UART_CMD_L_REBOOT_PAYLOAD_L       0
#define UART_CMD_FIND_PAYLOAD_L           0
#define UART_CMD_R_HW_VERSION_PAYLOAD_L   0
#define UART_CMD_R_SW_VERSION_PAYLOAD_L   0
#define UART_CMD_CHANGE_MODE_PAYLOAD_L    1
#define UART_CMD_L_GET_CCA_PAYLOAD_L      1
#define UART_CMD_L_GET_ED_PAYLOAD_L       0
#define UART_CMD_L_GET_LQI_PAYLOAD_L      0
#define UART_CMD_L_GET_RSSI_PAYLOAD_L     0
#define UART_CMD_L_SET_SHORT_PAYLOAD_L    1
#define UART_CMD_L_SET_EXTENDED_PAYLOAD_L 1
#define UART_CMD_L_SET_PAN_ID_PAYLOAD_L   1
#define UART_CMD_L_SET_PAYLOAD_PAYLOAD_L  2
#define UART_CMD_L_START_PAYLOAD_L        0
#define UART_CMD_L_END_PAYLOAD_L          0
#define UART_CMD_L_SET_ANTENNA_PAYLOAD_L  1
#define UART_CMD_L_GET_ANTENNA_PAYLOAD_L  0
#define UART_CMD_R_SET_ANTENNA_PAYLOAD_L  1
#define UART_CMD_R_GET_ANTENNA_PAYLOAD_L  0
#define UART_CMD_L_TX_PAYLOAD_L           2
#define UART_CMD_L_CLK_PAYLOAD_L          2
#define UART_CMD_L_SET_GPIO_PAYLOAD_L     2
#define UART_CMD_L_GET_GPIO_PAYLOAD_L     1
#define UART_CMD_L_TX_END_PAYLOAD_L       0
#define UART_CMD_L_CARRIER_PAYLOAD_L      3
#define UART_CMD_L_STREAM_PAYLOAD_L       3
#define UART_CMD_L_SET_DCDC_PAYLOAD_L     1
#define UART_CMD_L_GET_DCDC_PAYLOAD_L     0
#define UART_CMD_L_SET_ICACHE_PAYLOAD_L   1
#define UART_CMD_L_GET_ICACHE_PAYLOAD_L   0
#define UART_CMD_L_GET_TEMP_PAYLOAD_L     0
#define UART_CMD_L_INDICATION_PAYLOAD_L   1
#define UART_CMD_L_SLEEP_PAYLOAD_L        0
#define UART_CMD_L_RECEIVE_PAYLOAD_L      0

#define UART_TEXT_PAYLOAD_DELIMETERS      " "

#define UART_CMD_SET_POWER_VALUE_PLACE    2

#define UART_SET_PAN_ID_SYM_NUM           6
#define UART_SET_SHORT_SYM_NUM            6
#define UART_SET_EXTENDED_SYM_NUM         18

/** UART commands */
typedef enum
{
    PTT_UART_CMD_R_PING = 0x00,  /**< send ping to DUT */
    PTT_UART_CMD_L_PING_TIMEOUT, /**< set ping timeout to CMD */
    PTT_UART_CMD_SETCHANNEL,     /**< set channel to CMD and DUT */
    PTT_UART_CMD_L_SETCHANNEL,   /**< set channel to CMD */
    PTT_UART_CMD_R_SETCHANNEL,   /**< set channel to DUT */
    PTT_UART_CMD_L_GET_CHANNEL,  /**< get channel from CMD */
    PTT_UART_CMD_L_SET_POWER,    /**< set power to CMD */
    PTT_UART_CMD_R_SET_POWER,    /**< set power to DUT */
    PTT_UART_CMD_L_GET_POWER,    /**< get power from CMD */
    PTT_UART_CMD_R_GET_POWER,    /**< get power from DUT */
    PTT_UART_CMD_R_STREAM,       /**< DUT transmits a stream */
    PTT_UART_CMD_R_START,        /**< DUT starts RX test */
    PTT_UART_CMD_R_END,          /**< DUT ends RX test and sends a report */
    PTT_UART_CMD_L_REBOOT,       /**< CMD reboot local device */
    PTT_UART_CMD_FIND,           /**< CMD starts find procedure */
    PTT_UART_CMD_R_HW_VERSION,   /**< get HW version from DUT */
    PTT_UART_CMD_R_SW_VERSION,   /**< get SW version from DUT */
    PTT_UART_CMD_CHANGE_MODE,    /**< change device mode */
    PTT_UART_CMD_L_GET_CCA,      /**< CMD will perform CCA with selected mode and print out result*/
    PTT_UART_CMD_L_GET_ED,       /**< CMD will perform ED and print out the result */
    PTT_UART_CMD_L_GET_LQI,      /**< CMD will put the device into receive mode and wait for reception of a single packet*/
    PTT_UART_CMD_L_GET_RSSI,     /**< CMD will measure RSSI and print result */
    PTT_UART_CMD_L_SET_SHORT,    /**< CMD will set its short address */
    PTT_UART_CMD_L_SET_EXTENDED, /**< CMD will set its extended address */
    PTT_UART_CMD_L_SET_PAN_ID,   /**< CMD will set its PAN Id */
    PTT_UART_CMD_L_SET_PAYLOAD,  /**< CMD will set payload for next ltx commands */
    PTT_UART_CMD_L_START,        /**< CMD will put the DUT in a continuous receive mode, where each packet received is displayed in turn */
    PTT_UART_CMD_L_END,          /**< CMD will stop the continuous receive mode, if active. If continuous receive mode is not active, then the command is ignored. */
    PTT_UART_CMD_L_SET_ANTENNA,  /**< CMD will set antenna device is sending frames with. */
    PTT_UART_CMD_L_GET_ANTENNA,  /**< CMD will print out the antenna used by the device */
    PTT_UART_CMD_R_SET_ANTENNA,  /**< CMD will set antenna DUT device is sending frames with. */
    PTT_UART_CMD_R_GET_ANTENNA,  /**< CMD will print out the antenna used by the DUT  device */
    PTT_UART_CMD_L_TX,           /**< CMD will transmit a number of raw 802.15.4 frames with payload previously set by “custom lsetpayload” */
    PTT_UART_CMD_L_CLK,          /**< CMD will output HFCLK to GPIO pin */
    PTT_UART_CMD_L_SET_GPIO,     /**< CMD will set value to GPIO pin */
    PTT_UART_CMD_L_GET_GPIO,     /**< CMD will get value from GPIO pin */
    PTT_UART_CMD_L_TX_END,       /**< CMD will stop ltx command processing if ltx started with infinite number of packets */
    PTT_UART_CMD_L_CARRIER,      /**< CMD will emit carrier signal without modulation */
    PTT_UART_CMD_L_STREAM,       /**< CMD will emit modulated signal with payload previously set by “custom lsetpayload” */
    PTT_UART_CMD_L_SET_DCDC,     /**< CMD will disable/enable DC/DC mode */
    PTT_UART_CMD_L_GET_DCDC,     /**< CMD will print current DC/DC mode */
    PTT_UART_CMD_L_SET_ICACHE,   /**< CMD will disable/enable ICACHE */
    PTT_UART_CMD_L_GET_ICACHE,   /**< CMD will print current state of ICACHE */
    PTT_UART_CMD_L_GET_TEMP,     /**< CMD will print out the SoC temperature */
    PTT_UART_CMD_L_INDICATION,   /**< CMD will control LED indicating received packet */
    PTT_UART_CMD_L_SLEEP,        /**< CMD will put radio into sleep mode */
    PTT_UART_CMD_L_RECEIVE,      /**< CMD will put radio into receive mode*/
    PTT_UART_CMD_N,              /**< command count */
} ptt_uart_cmd_t;

/** Actions to perform with payload before sending it to command handler */
typedef enum
{
    PTT_UART_PAYLOAD_PARSED_AS_BYTES, /**< payload will be parsed as bytes separated by @ref UART_TEXT_PAYLOAD_DELIMETERS before it will be send to command handler */
    PTT_UART_PAYLOAD_RAW,             /**< payload will be written as raw ASCII and command handler will be responsible for parsing */
    PTT_UART_PAYLOAD_N,               /**< actions types count */
} ptt_uart_payload_t;

/** command name and additional info for text command parsing */
typedef struct
{
    const char       * name;         /**< string containing command name */
    ptt_uart_cmd_t     code;         /**< command code */
    uint8_t            payload_len;  /**< payload length for this command in space separated words */
    ptt_uart_payload_t payload_type; /**< action to perform with payload before sending it to command handler */
} uart_text_cmd_t;

/** @brief Send a packet over UART
 *
 *  @param p_pkt - data to send
 *  @param len - length of p_pkt
 *
 *  @return PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_uart_send_packet(const uint8_t * p_pkt, ptt_pkt_len_t len);

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

