/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: implementation of Zigbee RF Performance Test Plan CMD mode */

#ifndef PTT_MODE_ZB_PERF_CMD_H__
#define PTT_MODE_ZB_PERF_CMD_H__

#include <assert.h>

#include "ctrl/ptt_ctrl.h"
#include "ctrl/ptt_events.h"
#include "ctrl/ptt_timers.h"
#include "ctrl/ptt_uart.h"

#include "ptt_errors.h"
#include "ptt_config.h"
#include "ptt_utils.h"

#define PTT_PING_ERROR_MES "NO ACK"
#define PTT_PING_SUCCESS_MES "ACK"
#define PTT_GET_POWER_ERROR_MES "NO POWER RESPONSE"
#define PTT_END_RX_TEST_ERROR_MES "NO REPORT"
#define PTT_GET_HARDWARE_ERROR_MES "NO HW VERSION RESPONSE"
#define PTT_GET_SOFTWARE_ERROR_MES "NO SW VERSION RESPONSE"
#define PTT_FIND_ERROR_MES "DUT NOT FOUND"
#define PTT_GET_CCA_SUCCESS_MES "CCA:" /* note: value will be added in response function */
#define PTT_GET_CCA_ERROR_MES "CCA FAILED"
#define PTT_GET_ED_SUCCESS_MES "ED:" /* note: value will be added in response function */
#define PTT_GET_ED_ERROR_MES "ED FAILED"
#define PTT_GET_RSSI_SUCCESS_MES "RSSI:" /* note: value will be added in response function */
#define PTT_GET_RSSI_ERROR_MES "RSSI FAILED"
#define PTT_GET_LQI_SUCCESS_MES "LQI:" /* note: value will be added in response function */
#define PTT_GET_LQI_ERROR_MES "LQI:ERROR"
#define PTT_GET_ANTENNA_MES "ANTENNA:" /* note: value will be added in response function */
#define PTT_GET_ANTENNA_ERROR_MES "NO ANTENNA RESPONSE"
#define PTT_L_TX_NO_ACK_MES "NO ACK"
#define PTT_L_TX_CRC_ERROR_MES "CRC ERROR"
#define PTT_L_TX_ERROR_MES "TX ERROR"
#define PTT_L_START_CRC_ERROR_MES "CRC ERROR"
#define PTT_L_START_ERROR_MES "RX ERROR"
#define PTT_GET_DCDC_MES "DCDC:" /* note: value will be added in response function */
#define PTT_GET_ICACHE_MES "ICACHE:" /* note: value will be added in response function */

/** states of OTA commands processing in CMD mode */
enum cmd_ota_state_t {
	CMD_OTA_STATE_IDLE = 0, /**< waiting for next command */
	CMD_OTA_STATE_PING_SENDING, /**< rf module send ping command */
	CMD_OTA_STATE_PING_WAITING_FOR_ACK, /**< ping sent, waiting for ack */
	CMD_OTA_STATE_SET_CHANNEL_SENDING, /**< rf module send set_channel command */
	CMD_OTA_STATE_SET_POWER_SENDING, /**< rf module send set_power command */
	CMD_OTA_STATE_STREAM_SENDING, /**< rf module send stream command */
	CMD_OTA_STATE_START_RX_TEST_SENDING, /**< cmd send start rx test command to dut */
	CMD_OTA_STATE_GET_POWER_SENDING, /**< rf module send get_power command */
	CMD_OTA_STATE_GET_POWER_WAITING_FOR_RSP, /**< get_power sent, waiting for response */
	CMD_OTA_STATE_END_RX_TEST_SENDING, /**< rf module send end_rx_test command */
	CMD_OTA_STATE_END_RX_TEST_WAITING_FOR_REPORT, /**< end_rx_test sent, waiting for report */
	CMD_OTA_STATE_GET_HARDWARE_SENDING, /**< rf module send get hardware command */
	CMD_OTA_STATE_GET_HARDWARE_WAITING_FOR_RSP, /**< get hardware sent, waiting for response*/
	CMD_OTA_STATE_GET_SOFTWARE_SENDING, /**< rf module send get software command */
	CMD_OTA_STATE_GET_SOFTWARE_WAITING_FOR_RSP, /**< get software sent, waiting for response*/
	CMD_OTA_STATE_SET_ANTENNA_SENDING, /**< rf module send set_antenna command */
	CMD_OTA_STATE_GET_ANTENNA_SENDING, /**< rf module send get_antenna command */
	CMD_OTA_STATE_GET_ANTENNA_WAITING_FOR_RSP, /**< get_antenna sent, waiting for response */
	CMD_OTA_STATE_N
	/**< Total number of OTA command processing states */
};

/** states of uart command processing in CMD mode */
enum cmd_uart_state_t {
	CMD_UART_STATE_IDLE = 0,
	/**< waiting for next command */
	CMD_UART_STATE_SET_CHANNEL,
	/**< CMD device set channel on DUT */
	CMD_UART_STATE_STREAM,
	/**< CMD device processes enable stream on DUT */
	CMD_UART_STATE_WAIT_DELAY,
	/**< CMD is waiting for delay */
	CMD_UART_STATE_PING,
	/**< CMD device processes ping command */
	CMD_UART_STATE_WAITING_FOR_LEND,
	/**< CMD device in continuous receive mode and will accepts only "custom lend" command */
	CMD_UART_STATE_WAITING_FOR_RF_PACKET,
	/**< CMD device in continuous receive mode waiting for reception of any rf packet */
	CMD_UART_STATE_LTX_WAITING_FOR_ACK,
	/**< CMD device in continuous receive mode waiting for reception of ACK rf packet */
	CMD_UART_STATE_LTX,
	/**< CMD device processes ltx command */
	CMD_UART_STATE_L_CLK_OUT,
	/**< CMD device is waiting for disabling CLK output mode */
	CMD_UART_STATE_L_CARRIER_INTERVAL,
	/**< CMD device processing lcarrier command and radio is in receiving mode */
	CMD_UART_STATE_L_CARRIER_PULSE,
	/**< CMD device processing lcarrier command and sending continuous carrier */
	CMD_UART_STATE_L_STREAM_INTERVAL,
	/**< CMD device processing lstream command and radio is in receiving mode */
	CMD_UART_STATE_L_STREAM_PULSE,
	/**< CMD device processing lstream command and sending modulated carrier */
	CMD_UART_STATE_N
	/**< Total number of UART command processing states */
};

/* information for "custom ltx" command */
struct cmd_uart_ltx_info_s {
	bool is_infinite;
	bool is_stop_requested;
	uint8_t repeats_cnt;
	uint8_t max_repeats_cnt;
	uint8_t ack_cnt;
	uint16_t timeout;
};

PTT_COMPILE_TIME_ASSERT(sizeof(struct cmd_uart_ltx_info_s) <= PTT_EVENT_CTX_SIZE);

/* information for "custom lcarrier" command */
struct cmd_uart_waveform_timings_s {
	int32_t pulse_duration;
	int32_t interval;
};

PTT_COMPILE_TIME_ASSERT(sizeof(struct cmd_uart_waveform_timings_s) <= PTT_EVENT_CTX_SIZE);

/** @brief Initialize OTA commands processing module in CMD mode
 *
 *  @param none
 *
 *  @return none
 */
void cmd_ota_cmd_init(void);

/** @brief Uninitialize OTA commands processing module in CMD mode
 *
 *  @param none
 *
 *  @return none
 */
void cmd_ota_cmd_uninit(void);

/** @brief Process new OTA command
 *
 *  @param none
 *
 *  @return none
 */
enum ptt_ret cmd_ota_cmd_process(ptt_evt_id_t new_ota_cmd);

/** @brief Notify OTA modyle about "RX done" event
 *
 *  @param new_rf_pkt_evt - event containing the received packet.
 *
 *  @return none
 */
void cmd_ota_rf_rx_done(ptt_evt_id_t new_rf_pkt_evt);

/** @brief Notify OTA mode about "TX finished" event.
 *
 *  @param evt_id - expects to be the same as the event passed to ptt_rf_send_packet()
 *
 *  @return none
 */
void cmd_ota_rf_tx_finished(ptt_evt_id_t evt_id);

/** @brief Notify OTA mode about "TX failed" event.
 *
 *  @param evt_id - expects to be the same as the event passed to ptt_rf_send_packet()
 *
 *  @return none
 */
void cmd_ota_rf_tx_failed(ptt_evt_id_t evt_id);

/** @brief Handler for "TX finished" event.
 *
 *  @param evt_id - expects to be the same as the event passed to ptt_rf_send_packet()
 *
 *  @return none
 */
void cmd_rf_tx_finished(ptt_evt_id_t evt_id);

/** @brief Handler for "TX failed" event.
 *
 *  @param evt_id - expects to be the same as the event passed to ptt_rf_send_packet()
 *
 *  @return none
 */
void cmd_rf_tx_failed(ptt_evt_id_t evt_id);

/** @brief Handler for new received RF packet
 *
 *  @param new_rf_pkt_evt - new event containing the received packet.
 *  Handler is responsible for freeing the event.
 *
 *  @return none
 */
void cmd_rf_rx_done(ptt_evt_id_t new_rf_pkt_evt);

/** @brief Handler for "RX failed" event.
 *
 *  @param evt_id - new event containing the error code of reception fail.
 *  Handler is responsible for freeing the event.
 *
 *  @return none
 */
void cmd_rf_rx_failed(ptt_evt_id_t evt_id);

/** @brief Initialize UART commands processing module in CMD mode
 *
 *  @param none
 *
 *  @return none
 */
void cmd_uart_cmd_init(void);

/** @brief Uninitialize UART commands processing module in CMD mode
 *
 *  @param none
 *
 *  @return none
 */
void cmd_uart_cmd_uninit(void);

/** @brief Handler for new received UART packet
 *
 *  @param new_uart_cmd - new event containing the received UART packet.
 *  Handler is responsible for freeing the event.
 *
 *  @return none
 */
void cmd_uart_pkt_received(ptt_evt_id_t new_uart_cmd);

/** @brief Handler for "CCA done" event
 *
 *  @evt_id - event same as used for uart module locking.
 *  Contains CCA result in context.
 *  Handler is responsible for freeing the event.
 *
 *  @return none
 */
void cmd_rf_cca_done(ptt_evt_id_t evt_id);

/** @brief Handler for "CCA failed" event
 *
 *  @evt_id - event same as used for uart module locking.
 *  Handler is responsible for freeing the event.
 *
 *  @return none
 */
void cmd_rf_cca_failed(ptt_evt_id_t evt_id);

/** @brief Handler for "ED detected" event
 *
 *  @evt_id - event same as used for uart module locking.
 *  Contains ED result in context.
 *  Handler is responsible for freeing the event.
 *
 *  @return none
 */
void cmd_rf_ed_detected(ptt_evt_id_t evt_id);

/** @brief Handler for "ED failed" event
 *
 *  @evt_id - event same as used for uart module locking.
 *  Handler is responsible for freeing the event.
 *
 *  @return none
 */
void cmd_rf_ed_failed(ptt_evt_id_t evt_id);

/** @brief Handler for OTA command failed result such as timeouts and errors
 *
 *  @param evt_id - expects to be the same as the event passed to cmd_ota_cmd_process()
 *
 *  @return none
 */
void cmt_uart_ota_cmd_timeout_notify(ptt_evt_id_t evt_id);

/** @brief Handler for OTA command success result with the response in the context if expected
 *
 *  @param evt_id - expects to be the same as the event passed to cmd_ota_cmd_process()
 *
 *  @return none
 */
void cmt_uart_ota_cmd_finish_notify(ptt_evt_id_t evt_id);

/* Send responses through UART. Responses use passed event as memory
 * for storing prepared UART packet.
 * If payload is expected in context it will be extracted and processed.
 * It depends on UART command.
 */
/* Sends ACK response through UART */
void cmd_uart_send_rsack(ptt_evt_id_t evt_id);
/* Sends NO ACK response through UART */
void cmd_uart_send_rsp_no_ack(ptt_evt_id_t evt_id);
/* Sends current CMD channel through UART */
void cmd_uart_send_rsp_l_channel(ptt_evt_id_t evt_id);
/* Sends NO POWER RESPONSE through UART */
void cmd_uart_send_rsp_power_error(ptt_evt_id_t evt_id);
/* Sends DUT power through UART */
void cmd_uart_send_rsp_power(ptt_evt_id_t evt_id);
/* Sends NO REPORT through UART */
void cmd_uart_send_rsp_rx_test_error(ptt_evt_id_t evt_id);
/* Sends RX test report through UART */
void cmd_uart_send_rsp_rx_test(ptt_evt_id_t evt_id);
/* Sends NO HW VERSION through UART */
void cmd_uart_send_rsp_hw_error(ptt_evt_id_t evt_id);
/* Sends HW version through UART */
void cmd_uart_send_rsp_hw(ptt_evt_id_t evt_id);
/* Sends NO SW VERSION through UART */
void cmd_uart_send_rsp_sw_error(ptt_evt_id_t evt_id);
/* Sends SW version through UART */
void cmd_uart_send_rsp_sw(ptt_evt_id_t evt_id);
/* Sends found DUT channel through UART */
void cmd_uart_send_rsp_find(ptt_evt_id_t evt_id);
/* Sends DUT NOT FOUND through UART */
void cmd_uart_send_rsp_find_timeout(ptt_evt_id_t evt_id);
/* Sends CCA FAILED through UART */
void cmd_uart_send_rsp_cca_failed(ptt_evt_id_t evt_id);
/* Sends CCA result through UART */
void cmd_uart_send_rsp_cca_done(ptt_evt_id_t evt_id);
/* Sends ED FAILED through UART */
void cmd_uart_send_rsp_ed_failed(ptt_evt_id_t evt_id);
/* Sends ED result through UART */
void cmd_uart_send_rsp_ed_detected(ptt_evt_id_t evt_id);
/* Sends RSSI FAILED through UART */
void cmd_uart_send_rsp_rssi_failed(ptt_evt_id_t evt_id);
/* Sends RSSI result through UART */
void cmd_uart_send_rsp_rssi_done(ptt_evt_id_t evt_id);
/* Sends new packet report through UART */
void cmd_uart_send_rsp_l_start_new_packet(ptt_evt_id_t new_rf_pkt_evt);
/* Sends report about received packet from previous call to lstart */
void cmd_uart_send_rsp_l_end(ptt_evt_id_t evt_id, uint32_t proto_pkts);
/* Sends LQI FAILED through UART */
void cmd_uart_send_rsp_lqi_failed(ptt_evt_id_t evt_id);
/* Sends LQI result through UART */
void cmd_uart_send_rsp_lqi_done(ptt_evt_id_t evt_id);
/* Sends current antenna through UART */
void cmd_uart_send_rsp_antenna(ptt_evt_id_t evt_id);
/* Sends NO ANTENNA RESPONSE through UART */
void cmd_uart_send_rsp_antenna_error(ptt_evt_id_t evt_id);
/* Sends NO ACK through UART */
void cmd_uart_send_rsp_ltx_failed(ptt_evt_id_t evt_id);
/* Sends ack packet through UART */
void cmd_uart_send_rsp_ltx_ack(ptt_evt_id_t evt_id, uint32_t packet_n);
/* Sends new packet reception fail report through UART */
void cmd_uart_send_rsp_l_start_rx_fail(ptt_evt_id_t new_rf_pkt_evt);
/* Sends GPIO pin value through UART */
void cmd_uart_send_rsp_l_get_gpio(ptt_evt_id_t evt_id);
/* Sends GPIO pin reading error through UART */
void cmd_uart_send_rsp_l_get_gpio_error(ptt_evt_id_t evt_id);
/* Sends SoC temperature through UART */
void cmd_uart_send_rsp_get_temp(ptt_evt_id_t evt_id);
/* Sends current DCDC mode through UART */
void cmd_uart_send_rsp_get_dcdc(ptt_evt_id_t evt_id);
/* Sends current state of ICACHE through UART */
void cmd_uart_send_rsp_get_icache(ptt_evt_id_t evt_id);

#endif /* PTT_MODE_ZB_PERF_CMD_H__ */
