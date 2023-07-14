/*
 *
 *Copyright (c) 2022 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief System interface between host and RPU
 */

#ifndef __HOST_RPU_SYS_IF_H__
#define __HOST_RPU_SYS_IF_H__

#include "host_rpu_common_if.h"
#include "lmac_if_common.h"

#include "pack_def.h"

#define USE_PROTECTION_NONE 0
#define USE_PROTECTION_RTS 1
#define USE_PROTECTION_CTS2SELF 2

#define USE_SHORT_PREAMBLE 0
#define DONT_USE_SHORT_PREAMBLE 1

#define MARK_RATE_AS_MCS_INDEX 0x80
#define MARK_RATE_AS_RATE 0x00

#define ENABLE_GREEN_FIELD 0x01
#define ENABLE_CHNL_WIDTH_40MHZ 0x02
#define ENABLE_SGI 0x04
#define ENABLE_11N_FORMAT 0x08
#define ENABLE_VHT_FORMAT 0x10
#define ENABLE_CHNL_WIDTH_80MHZ 0x20

#define MAX_TX_AGG_SIZE 16
#define MAX_RX_BUFS_PER_EVNT 64
#define MAX_MGMT_BUFS 16

/*#define ETH_ADDR_LEN		6*/
#define MAX_RF_CALIB_DATA 900

#define NRF_WIFI_ETH_ADDR_LEN 6

#define PHY_THRESHOLD_NORMAL (-65)
#define PHY_THRESHOLD_PROD_MODE (-93)

#define MAX_TX_STREAMS 1
#define MAX_RX_STREAMS 1

#define MAX_NUM_VIFS 2
#define MAX_NUM_STAS 2
#define MAX_NUM_APS 1

#define NRF_WIFI_COUNTRY_CODE_LEN 2

/**
 * @brief This enum provides a list of different operating modes.
 *
 */
enum rpu_op_mode {
	/** Radio test mode is used for performing radio tests using
	 *  continuous Tx/Rx on a configured channel at a particular rate or power.
	 */
	RPU_OP_MODE_RADIO_TEST,
	/** In this mode different types of calibration like RF calibration can be performed */
	RPU_OP_MODE_FCM,
	/** Regular mode of operation */
	RPU_OP_MODE_REG,
	/** Debug mode can be used to control certain parameters like TX rate
	 *  in order to debug functional issues.
	 */
	RPU_OP_MODE_DBG,
	/** Highest mode number currently defined */
	RPU_OP_MODE_MAX
};

/**
 *  @brief This enum defines various types of statistics.
 */
enum rpu_stats_type {
	/** All statistics includes PHY, LMAC & UMAC */
	RPU_STATS_TYPE_ALL,
	/** Host statistics */
	RPU_STATS_TYPE_HOST,
	/** UMAC statistics */
	RPU_STATS_TYPE_UMAC,
	/** LMAC statistics*/
	RPU_STATS_TYPE_LMAC,
	/** PHY statistics */
	RPU_STATS_TYPE_PHY,
	/** Highest statistics type number currently defined */
	RPU_STATS_TYPE_MAX
};

/**
 * @brief- Throughput mode
 * Throughput mode to be used for transmitting the packet.
 */
enum rpu_tput_mode {
	/** Legacy mode */
	RPU_TPUT_MODE_LEGACY,
	/** High Throuput mode(11n) */
	RPU_TPUT_MODE_HT,
	/** Very hight throughput(11ac) */
	RPU_TPUT_MODE_VHT,
	/** HE SU mode */
	RPU_TPUT_MODE_HE_SU,
	/** HE ER SU mode */
	RPU_TPUT_MODE_HE_ER_SU,
	/** HE TB mode */
	RPU_TPUT_MODE_HE_TB,
	/** Highest throughput mode currently defined */
	RPU_TPUT_MODE_MAX
};

/**
 * @brief - System commands.
 *
 */
enum nrf_wifi_sys_commands {
	/** Command to initialize RPU and RPU responds with NRF_WIFI_EVENT_INIT_DONE */
	NRF_WIFI_CMD_INIT,
	/** command to send a Tx packet in radiotest mode */
	NRF_WIFI_CMD_TX,
	/** Unused */
	NRF_WIFI_CMD_IF_TYPE,
	/** command to specify mode of operation */
	NRF_WIFI_CMD_MODE,
	/** command to get statistics */
	NRF_WIFI_CMD_GET_STATS,
	/** command to clear statistics */
	NRF_WIFI_CMD_CLEAR_STATS,
	/** command to ENABLE/DISABLE receiving packets in radiotest mode */
	NRF_WIFI_CMD_RX,
	/** Command to measure battery voltage and RPU responds	with NRF_WIFI_EVENT_PWR_DATA */
	NRF_WIFI_CMD_PWR,
	/** RPU De-initialization */
	NRF_WIFI_CMD_DEINIT,
	/** Command for WIFI & Bluetooth coexistence */
	NRF_WIFI_CMD_BTCOEX,
	/** Command to start RF test */
	NRF_WIFI_CMD_RF_TEST,
	/** Configure HE_GI & HE_LTF */
	NRF_WIFI_CMD_HE_GI_LTF_CONFIG,
	/** Command for getting UMAC memory statistics */
	NRF_WIFI_CMD_UMAC_INT_STATS,
	/** Command for Setting the channel & Rf params in radiotest mode */
	NRF_WIFI_CMD_RADIO_TEST_INIT,
	/** Command for setting country in radiotest mode */
	NRF_WIFI_CMD_RT_REQ_SET_REG,
	/** Command to enable/disable fixed data rate in regular mode */
	NRF_WIFI_CMD_TX_FIX_DATA_RATE,
};

/**
 * @brief - Events from the RPU.
 *
 */
enum nrf_wifi_sys_events {
	/** Response to NRF_WIFI_CMD_PWR */
	NRF_WIFI_EVENT_PWR_DATA,
	/** Response to NRF_WIFI_CMD_INIT */
	NRF_WIFI_EVENT_INIT_DONE,
	/** Response to NRF_WIFI_CMD_GET_STATS */
	NRF_WIFI_EVENT_STATS,
	/** Response to NRF_WIFI_CMD_DEINIT */
	NRF_WIFI_EVENT_DEINIT_DONE,
	/** Response to NRF_WIFI_CMD_RF_TEST */
	NRF_WIFI_EVENT_RF_TEST,
	/** Response to NRF_WIFI_CMD_BTCOEX. */
	NRF_WIFI_EVENT_COEX_CONFIG,
	/** Response to NRF_WIFI_CMD_UMAC_INT_STATS */
	NRF_WIFI_EVENT_INT_UMAC_STATS,
	/** Command status events for radio test commands*/
	NRF_WIFI_EVENT_RADIOCMD_STATUS
};

/**
 * @brief - Channel Bandwidth types.
 *
 **/
enum rpu_ch_bw {
	/** 20MHz bandwidth */
	RPU_CH_BW_20,
	/** 40MHz bandwidth */
	RPU_CH_BW_40,
	/** 80MHz bandwidth */
	RPU_CH_BW_MAX
};

/**
 * @brief - This structure specifies the parameters required to configure a specific channel.
 *
 */
struct chan_params {
	/** Primary channel number */
	unsigned int primary_num;
	/** Channel bandwidth */
	unsigned char bw;
	/** 20Mhz offset value */
	signed int sec_20_offset;
	/** 40Mhz offset value */
	signed int sec_40_offset;
} __NRF_WIFI_PKD;

/**
 * @brief This structure specifies the parameters required to start or stop the RX (receive)
 *  operation in radiotest mode.
 *
 */
struct rpu_conf_rx_radio_test_params {
	/** Number of spatial streams supported. Currently unused. */
	unsigned char nss;
	/* Input to the RF for operation */
	unsigned char rf_params[NRF_WIFI_RF_PARAMS_SIZE];
	/** An array containing RF and baseband control params */
	struct chan_params chan;
	/** Copy OTP params to this memory */
	signed char phy_threshold;
	/** Calibration bit map value. More information can be found in the phy_rf_params.h file.
	 */
	unsigned int phy_calib;
	/** Start Rx : 1, Stop Rx :0 */
	unsigned char rx;
} __NRF_WIFI_PKD;

/**
 * @brief This structure specifies the UMAC (Upper MAC) RX (receive) debug parameters
 *  specifically designed for debugging purpose.
 *
 */

struct umac_rx_dbg_params {
	/** Total lmac events received to UMAC */
	unsigned int lmac_events;
	/** Total Rx events(LMAC_EVENT_RX) received in ISR */
	unsigned int rx_events;
	/** Received coalised events from LMAC */
	unsigned int rx_coalesce_events;
	/** Total Rx packets received from LMAC */
	unsigned int total_rx_pkts_from_lmac;
	/** Maximum RX packets buffered at any point of time in UMAC.*/
	unsigned int max_refill_gap;
	/** Difference between rx packets received from lmac and packets sent to host */
	unsigned int current_refill_gap;
	/** Number of Packets queued to reorder buffer due to out of order */
	unsigned int out_of_order_mpdus;
	/** Number of packets removed from reorder buffer */
	unsigned int reorder_free_mpdus;
	/** Number of Rx packets resubmitted to LMAC by UMAC */
	unsigned int umac_consumed_pkts;
	/** Number of Rx packets sent to Host for resubmiting */
	unsigned int host_consumed_pkts;
	/** Total events posted to UMAC RX thread from LMAC */
	unsigned int rx_mbox_post;
	/** Total events received to UMAC RX thread from LMAC */
	unsigned int rx_mbox_receive;
	/** Number of packets received in out of order */
	unsigned int reordering_ampdu;
	/** Messages posted  to TX mbox from timer ISR */
	unsigned int timer_mbox_post;
	/** Messages received from timer ISR */
	unsigned int timer_mbox_rcv;
	/** Messages posted to TX mbox from work scheduler */
	unsigned int work_mbox_post;
	/** Messages received from work scheduler */
	unsigned int work_mbox_rcv;
	/** Messages posted to TX mbox from tasklet function */
	unsigned int tasklet_mbox_post;
	/** Messages received from tasklet function */
	unsigned int tasklet_mbox_rcv;
	/** Management frames sent to userspace */
	unsigned int userspace_offload_frames;
	/** Number of times where requested buffer size is not available
	 *  and allocated from next available memory buffer
	 */
	unsigned int alloc_buf_fail;
	/** Total packets count in RX thread */
	unsigned int rx_packet_total_count;
	/** Number of data packets received */
	unsigned int rx_packet_data_count;
	/** Number of Qos data packets received */
	unsigned int rx_packet_qos_data_count;
	/** Number of protected data packets received */
	unsigned int rx_packet_protected_data_count;
	/** Number of management packets received */
	unsigned int rx_packet_mgmt_count;
	/** Number of beacon packets received */
	unsigned int rx_packet_beacon_count;
	/** Number of probe response packets received */
	unsigned int rx_packet_probe_resp_count;
	/** Number of authentication packets received */
	unsigned int rx_packet_auth_count;
	/** Number of deauthentication packets received */
	unsigned int rx_packet_deauth_count;
	/** Number of assoc response packets received */
	unsigned int rx_packet_assoc_resp_count;
	/** Number of disassociation packets received */
	unsigned int rx_packet_disassoc_count;
	/** Number of action frames received */
	unsigned int rx_packet_action_count;
	/** Number of probe request packets received */
	unsigned int rx_packet_probe_req_count;
	/** Other management packets received */
	unsigned int rx_packet_other_mgmt_count;
	/** Maximum coalised packets received from LMAC in any RX event */
	signed char max_coalesce_pkts;
	/** Packets received with null skb pointer from LMAC */
	unsigned int null_skb_pointer_from_lmac;
	/** Number of unexpected management packets received in coalesce event */
	unsigned int unexpected_mgmt_pkt;

} __NRF_WIFI_PKD;

/**
 * @brief This structure specifies the UMAC TX (transmit) debug parameters used for
 *  debugging purposes.
 *
 */
struct umac_tx_dbg_params {
	/** Total number of tx commands received from host */
	unsigned int tx_cmd;
	/** Non coalesce packets received */
	unsigned int tx_non_coalesce_pkts_rcvd_from_host;
	/** coalesce packets received */
	unsigned int tx_coalesce_pkts_rcvd_from_host;
	/** Maximum number of coalesce packets received in any
	 *  TX command coalesce packets received
	 */
	unsigned int tx_max_coalesce_pkts_rcvd_from_host;
	/** Maximum Tx commands currently in process at any point of time in UMAC */
	unsigned int tx_cmds_max_used;
	/** Number of Tx commands that are currently in process in UMAC */
	unsigned int tx_cmds_currently_in_use;
	/** Number of tx done events sent to host */
	unsigned int tx_done_events_send_to_host;
	/** Number of tx done success packets sent to host */
	unsigned int tx_done_success_pkts_to_host;
	/** Number of tx done failure packets sent to host */
	unsigned int tx_done_failure_pkts_to_host;
	/** Number of packets received from host that needs to be encrypted */
	unsigned int tx_cmds_with_crypto_pkts_rcvd_from_host;
	/** Number of packets received from host that need not to be encrypted */
	unsigned int tx_cmds_with_non_crypto_pkts_rcvd_from_host;
	/** Number of broadcast	packets received from host */
	unsigned int tx_cmds_with_broadcast_pkts_rcvd_from_host;
	/** Number of multicast	packets received from host */
	unsigned int tx_cmds_with_multicast_pkts_rcvd_from_host;
	/** Number of unicast packets received from host */
	unsigned int tx_cmds_with_unicast_pkts_rcvd_from_host;
	/** UMAC internal count */
	unsigned int xmit;
	/** Number of addba requests sent */
	unsigned int send_addba_req;
	/** Total ADD BA responses received from host */
	unsigned int addba_resp;
	/** Total packets received in softmac tx function */
	unsigned int softmac_tx;
	/** Number of packets generated internally in UMAC */
	unsigned int internal_pkts;
	/** Number of packets Received from host */
	unsigned int external_pkts;
	/** Total tx commmands sent to lmac */
	unsigned int tx_cmds_to_lmac;
	/** Tx dones received from LMAC */
	unsigned int tx_dones_from_lmac;
	/** Total commands sent to lmac in UMAC hal */
	unsigned int total_cmds_to_lmac;
	/** Number of data packets sent */
	unsigned int tx_packet_data_count;
	/** Number of management packets sent */
	unsigned int tx_packet_mgmt_count;
	/** Number of beacon packets sent */
	unsigned int tx_packet_beacon_count;
	/** Number of probe request packets sent */
	unsigned int tx_packet_probe_req_count;
	/** Number of authentication packets sent */
	unsigned int tx_packet_auth_count;
	/** Number of deauthentication packets sent */
	unsigned int tx_packet_deauth_count;
	/** Number of association request packets sent */
	unsigned int tx_packet_assoc_req_count;
	/** Number of disassociation packets sent */
	unsigned int tx_packet_disassoc_count;
	/** Number of action packets sent */
	unsigned int tx_packet_action_count;
	/** Other management packets sent */
	unsigned int tx_packet_other_mgmt_count;
	/** Number of Non management packets sent */
	unsigned int tx_packet_non_mgmt_data_count;

} __NRF_WIFI_PKD;

/**
 * @brief This structure specifies the UMAC command and event debug parameters used for
 *  debugging purpose.
 *
 */
struct umac_cmd_evnt_dbg_params {
	/** Number of command init received from host */
	unsigned char cmd_init;
	/** Number of init_done events sent to host */
	unsigned char event_init_done;
	/** Number of rf test command received from host */
	unsigned char cmd_rf_test;
	/** Number of connect command received from host */
	unsigned char cmd_connect;
	/** Number of get_stats command received from host */
	unsigned int cmd_get_stats;
	/** Number of power save state events sent to host */
	unsigned int event_ps_state;
	/** Unused*/
	unsigned int cmd_set_reg;
	/** Number of get regulatory commands received from host */
	unsigned int cmd_get_reg;
	/** Number of request set regulatory commands received from host */
	unsigned int cmd_req_set_reg;
	/** Number of trigger scan commands received from host */
	unsigned int cmd_trigger_scan;
	/** Number of scan done events sent to host */
	unsigned int event_scan_done;
	/** Number of get scan commands received from the host to get scan results */
	unsigned int cmd_get_scan;
	/** Number of scan commands sent to LMAC */
	unsigned int umac_scan_req;
	/** Number of scan complete events received from LMAC */
	unsigned int umac_scan_complete;
	/** Number of scan requests received from host when previous scan is in progress */
	unsigned int umac_scan_busy;
	/** Number of authentication requests received from host */
	unsigned int cmd_auth;
	/** Number of association requests received from host */
	unsigned int cmd_assoc;
	/** Number of deauthentication requests received from host */
	unsigned int cmd_deauth;
	/** Number of register frame commands received from host to register
	 *  a management frame type which should be passed to host
	 */
	unsigned int cmd_register_frame;
	/** Number of command frames from host which will be used for
	 *  transmitting management frames
	 */
	unsigned int cmd_frame;
	/** Number of delete key commands from host */
	unsigned int cmd_del_key;
	/** Number of new key commands received from host */
	unsigned int cmd_new_key;
	/** Number of set key commands received from host */
	unsigned int cmd_set_key;
	/** Number of get key commands received from host */
	unsigned int cmd_get_key;
	/** Number of beacon hint events sent to host */
	unsigned int event_beacon_hint;
	/** Number of regulatory change events sent to host when regulatory change command
	 *  received from host such as in response to command NL80211_CMD_REG_CHANGE
	 */
	unsigned int event_reg_change;
	/** Number of regulatory change events sent to host other than
	 *  host request for regulatory change
	 */
	unsigned int event_wiphy_reg_change;
	/** Number of set station commands received from host */
	unsigned int cmd_set_station;
	/** Number of new station commands received from host */
	unsigned int cmd_new_station;
	/** Number of del station commands received from host */
	unsigned int cmd_del_station;
	/** Number of new interface commands received from host */
	unsigned int cmd_new_interface;
	/** Number of set interface commands received from host */
	unsigned int cmd_set_interface;
	/** Number of get interface commands received from host */
	unsigned int cmd_get_interface;
	/** Number of set_ifflags commands received from host */
	unsigned int cmd_set_ifflags;
	/** Number of set_ifflags events sent to host */
	unsigned int cmd_set_ifflags_done;
	/** Number of set bss command received from host */
	unsigned int cmd_set_bss;
	/** Number of set wiphy command received from host */
	unsigned int cmd_set_wiphy;
	/** Number of start access point command received from host */
	unsigned int cmd_start_ap;
	/** Number of power save configuration commands sent to LMAC */
	unsigned int LMAC_CMD_PS;
	/** Current power save state configured to LMAC through LMAC_CMD_PS command */
	unsigned int CURR_STATE;
} __NRF_WIFI_PKD;

#ifndef CONFIG_NRF700X_RADIO_TEST

/**
 * @brief This structure specifies the UMAC interface debug parameters used for debugging purpose.
 *
 */
struct nrf_wifi_interface_stats {
	/** Number of unicast packets sent */
	unsigned int tx_unicast_pkt_count;
	/** Number of multicast packets sent */
	unsigned int tx_multicast_pkt_count;
	/** Number of broadcast packets sent */
	unsigned int tx_broadcast_pkt_count;
	/** Number of tx data bytes sent */
	unsigned int tx_bytes;
	/** Number of unicast packets received */
	unsigned int rx_unicast_pkt_count;
	/** Number of multicast packets received */
	unsigned int rx_multicast_pkt_count;
	/** Number of broadcast packets received */
	unsigned int rx_broadcast_pkt_count;
	/** Number of beacon packets received */
	unsigned int rx_beacon_success_count;
	/** Number of beacon packets missed */
	unsigned int rx_beacon_miss_count;
	/** Number of rx data bytes received */
	unsigned int rx_bytes;
	/** Number of packets with checksum mismatch received */
	unsigned int rx_checksum_error_count;
	/** Number of duplicate packets received */
	unsigned int replay_attack_drop_cnt;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the UMAC debug statistics. It contains the necessary parameters
 *  and fields used to gather and present debugging statistics within the UMAC layer.
 *
 */
struct rpu_umac_stats {
	/** Transmit debug statistics @ref umac_tx_dbg_params */
	struct umac_tx_dbg_params tx_dbg_params;
	/** Receive debug statistics @ref umac_rx_dbg_params */
	struct umac_rx_dbg_params rx_dbg_params;
	/** Command Event debug statistics @ref umac_cmd_evnt_dbg_params */
	struct umac_cmd_evnt_dbg_params cmd_evnt_dbg_params;
	/** Interface debug parameters @ref nrf_wifi_interface_stats */
	struct nrf_wifi_interface_stats interface_data_stats;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the LMAC debug parameters.
 *
 */
struct rpu_lmac_stats {
	/** Number of reset command counts from UMAC */
	unsigned int reset_cmd_cnt;
	/** Number of reset complete events sent to UMAC */
	unsigned int reset_complete_event_cnt;
	/** Number of events unable to generate */
	unsigned int unable_gen_event;
	/** Number of channel program commands from UMAC */
	unsigned int ch_prog_cmd_cnt;
	/** Number of channel program done events to UMAC */
	unsigned int channel_prog_done;
	/** Number of Tx commands from UMAC */
	unsigned int tx_pkt_cnt;
	/** Number of Tx done events to UMAC */
	unsigned int tx_pkt_done_cnt;
	/** Unused */
	unsigned int scan_pkt_cnt;
	/** Number of internal Tx packets */
	unsigned int internal_pkt_cnt;
	/** Number of Tx dones for internal packets */
	unsigned int internal_pkt_done_cnt;
	/** Number of acknowledgment responses */
	unsigned int ack_resp_cnt;
	/** Number of transmit timeouts */
	unsigned int tx_timeout;
	/** Number of deaggregation ISRs */
	unsigned int deagg_isr;
	/** Number of deaggregation input descriptor empties */
	unsigned int deagg_inptr_desc_empty;
	/** Number of deaggregation circular buffer full events */
	unsigned int deagg_circular_buffer_full;
	/** Number of LMAC received ISRs */
	unsigned int lmac_rxisr_cnt;
	/** Number of received packets decrypted */
	unsigned int rx_decryptcnt;
	/** Number of packet decryption failures during processing */
	unsigned int process_decrypt_fail;
	/** Number of RX event preparation failures */
	unsigned int prepa_rx_event_fail;
	/** Number of RX core pool full counts */
	unsigned int rx_core_pool_full_cnt;
	/** Number of RX MPDU CRC successes */
	unsigned int rx_mpdu_crc_success_cnt;
	/** Number of RX MPDU CRC failures */
	unsigned int rx_mpdu_crc_fail_cnt;
	/** Number of RX OFDM CRC successes */
	unsigned int rx_ofdm_crc_success_cnt;
	/** Number of RX OFDM CRC failures */
	unsigned int rx_ofdm_crc_fail_cnt;
	/** Number of RX DSSS CRC successes */
	unsigned int rxDSSSCrcSuccessCnt;
	/** Number of RX DSSS CRC failures */
	unsigned int rxDSSSCrcFailCnt;
	/** Number of RX crypto start counts */
	unsigned int rx_crypto_start_cnt;
	/** Number of RX crypto done counts */
	unsigned int rx_crypto_done_cnt;
	/** Number of RX event buffer full counts */
	unsigned int rx_event_buf_full;
	/** Number of RX external RAM buffer full counts */
	unsigned int rx_extram_buf_full;
	/** Number of scan requests receive from UMAC */
	unsigned int scan_req;
	/** Number of scan complete events sent to UMAC */
	unsigned int scan_complete;
	/** Number of scan abort requests */
	unsigned int scan_abort_req;
	/** Number of scan abort complete events */
	unsigned int scan_abort_complete;
	/** Number of internal buffer pool null counts */
	unsigned int internal_buf_pool_null;
} __NRF_WIFI_PKD;

#endif /* !CONFIG_NRF700X_RADIO_TEST */

/**
 * @brief This structure defines the PHY (Physical Layer) debug statistics.
 *
 */
struct rpu_phy_stats {
	/** Rssi average value received from LMAC */
	signed char rssi_avg;
	/** Unused */
	unsigned char pdout_val;
	/** Number of OFDM CRC Pass packets */
	unsigned int ofdm_crc32_pass_cnt;
	/** Number of OFDM CRC Fail packets */
	unsigned int ofdm_crc32_fail_cnt;
	/** Number of DSSS CRC Pass packets */
	unsigned int dsss_crc32_pass_cnt;
	/** Number of DSSS CRC Fail packets */
	unsigned int dsss_crc32_fail_cnt;
} __NRF_WIFI_PKD;

/**
 * @brief The UMAC header structure for system commands and events defines the format
 *  used to transmit and receive system-level commands and events.
 *
 */
struct nrf_wifi_sys_head {
	/** Command/Event id */
	unsigned int cmd_event;
	/** message length */
	unsigned int len;
} __NRF_WIFI_PKD;

/** Feature Disable */
#define NRF_WIFI_FEATURE_DISABLE 0
/** Feature Enable */
#define NRF_WIFI_FEATURE_ENABLE 1

/**
 * @brief The maximum Rx (receive) A-MPDU size in KB.
 *
 */
enum max_rx_ampdu_size {
	/** 8KB AMPDU Size */
	MAX_RX_AMPDU_SIZE_8KB,
	/** 16KB AMPDU Size */
	MAX_RX_AMPDU_SIZE_16KB,
	/** 32KB AMPDU Size */
	MAX_RX_AMPDU_SIZE_32KB,
	/** 64KB AMPDU Size */
	MAX_RX_AMPDU_SIZE_64KB
};

/**
 * @brief This structure specifies the configuration parameters used for configuring
 *  data-related settings.
 *
 */

struct nrf_wifi_data_config_params {
	/** rate_protection_type:0->NONE, 1->RTS/CTS, 2->CTS2SELF */
	unsigned char rate_protection_type;
	/** Aggregation is enabled(NRF_WIFI_FEATURE_ENABLE) or
	 *  disabled(NRF_WIFI_FEATURE_DISABLE)
	 */
	unsigned char aggregation;
	/** WMM is enabled(NRF_WIFI_FEATURE_ENABLE) or
	 *  disabled(NRF_WIFI_FEATURE_DISABLE)
	 */
	unsigned char wmm;
	/** Max number of aggregated TX sessions */
	unsigned char max_num_tx_agg_sessions;
	/** Max number of aggregated RX sessions */
	unsigned char max_num_rx_agg_sessions;
	/** maximum aggregation size */
	unsigned char max_tx_aggregation;
	/** Reorder buffer size (1 to 64) */
	unsigned char reorder_buf_size;
	/** Max RX AMPDU size (8/16/32/64 KB), see &enum max_rx_ampdu_size */
	signed int max_rxampdu_size;
} __NRF_WIFI_PKD;

/**
 * @brief This structure specifies the parameters that need to be provided for the command
 *  NRF_WIFI_CMD_INIT. The NRF_WIFI_CMD_INIT command is typically used to initialize the
 *  Wi-Fi module and prepare it for further communication.
 *
 */

struct nrf_wifi_sys_params {
	/** enable rpu sleep */
	unsigned int sleep_enable;
	/** Normal/FTM mode */
	unsigned int hw_bringup_time;
	/** Antenna Configuration, applicable only for 1x1 */
	unsigned int sw_bringup_time;
	/** Internal tuning parameter */
	unsigned int bcn_time_out;
	/** Set to 1 if rpu is expected to perform sleep clock calibration */
	unsigned int calib_sleep_clk;
	/** calib bit map value. More info can be found in phy_rf_params.h NRF_WIFI_DEF_PHY_CALIB */
	unsigned int phy_calib;
#ifndef CONFIG_NRF700X_RADIO_TEST
	/** MAC address of the interface */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** An array containing RF & baseband control params */
	unsigned char rf_params[NRF_WIFI_RF_PARAMS_SIZE];
	/** Indicates whether the rf_params has a valid value */
	unsigned char rf_params_valid;
#endif /* !CONFIG_NRF700X_RADIO_TEST */
} __NRF_WIFI_PKD;


/**
 * @brief This structure defines the parameters used to control the transmit (TX) power.
 *
 */

struct nrf_wifi_tx_pwr_ctrl_params {
	/** Antenna gain for 2.4 GHz band */
	unsigned char ant_gain_2g;
	/** Antenna gain for 5 GHz band (5150 MHz - 5350 MHz) */
	unsigned char ant_gain_5g_band1;
	/** Antenna gain for 5 GHz band (5470 MHz - 5730 MHz) */
	unsigned char ant_gain_5g_band2;
	/** Antenna gain for 5 GHz band (5730 MHz - 5895 MHz) */
	unsigned char ant_gain_5g_band3;
	/** Transmit power backoff (in dB) for lower edge of 2.4 GHz frequency band */
	unsigned char band_edge_2g_lo;
	/** Transmit power backoff (in dB) for upper edge of 2.4 GHz frequency band */
	unsigned char band_edge_2g_hi;
	/** Transmit power backoff (in dB) for lower edge of UNII-1 frequency band */
	unsigned char band_edge_5g_unii_1_lo;
	/** Transmit power backoff (in dB) for upper edge of UNII-1 frequency band */
	unsigned char band_edge_5g_unii_1_hi;
	/** Transmit power backoff (in dB) for lower edge of UNII-2A frequency band */
	unsigned char band_edge_5g_unii_2a_lo;
	/** Transmit power backoff (in dB) for upper edge of UNII-2A frequency band */
	unsigned char band_edge_5g_unii_2a_hi;
	/** Transmit power backoff (in dB) for lower edge of UNII-2C frequency band */
	unsigned char band_edge_5g_unii_2c_lo;
	/** Transmit power backoff (in dB) for upper edge of UNII-2C frequency band */
	unsigned char band_edge_5g_unii_2c_hi;
	/** Transmit power backoff (in dB) for lower edge of UNII-3 frequency band */
	unsigned char band_edge_5g_unii_3_lo;
	/** Transmit power backoff (in dB) for upper edge of UNII-3 frequency band */
	unsigned char band_edge_5g_unii_3_hi;
	/** Transmit power backoff (in dB) for lower edge of UNII-4 frequency band */
	unsigned char band_edge_5g_unii_4_lo;
	/** Transmit power backoff (in dB) for upper edge of UNII-4 frequency band */
	unsigned char band_edge_5g_unii_4_hi;
} __NRF_WIFI_PKD;

/**
 * @brief This enum defines different types of operating bands.
 *
 */
enum op_band {
	/** All bands */
	BAND_ALL,
	/** 2.4Ghz band */
	BAND_24G
};

/**
 * @brief This structure defines the command responsible for initializing the UMAC.
 *  After the host driver brings up, the host sends NRF_WIFI_CMD_INIT to the RPU.
 *  The RPU then performs the initialization and responds with NRF_WIFI_EVENT_INIT_DONE
 *  once the initialization is completed.
 *
 */

struct nrf_wifi_cmd_sys_init {
	/** umac header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** id of the interface */
	unsigned int wdev_id;
	/** @ref nrf_wifi_sys_params */
	struct nrf_wifi_sys_params sys_params;
	/** LMAC Rx buffs pool params, @ref rx_buf_pool_params */
	struct rx_buf_pool_params rx_buf_pools[MAX_NUM_OF_RX_QUEUES];
	/** Data configuration params, @ref nrf_wifi_data_config_params */
	struct nrf_wifi_data_config_params data_config_params;
	/** Calibration trigger control info based on battery voltage and temperature changes.
	 *  @ref temp_vbat_config from lmac_if_common.h
	 */
	struct temp_vbat_config temp_vbat_config_params;
	/** 0:umac checksum disable 1: umac checksum enable */
	unsigned char tcp_ip_checksum_offload;
	/** Country code to set */
	unsigned char country_code[NRF_WIFI_COUNTRY_CODE_LEN];
	/** Operating band see enum op_band */
	unsigned int op_band;
	/** System parameters provided for controlling the TX power */
	struct nrf_wifi_tx_pwr_ctrl_params tx_pwr_ctrl_params;
	/** Offload mgmt buffer refill to UMAC when enabled */
	unsigned char mgmt_buff_offload;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to de-initialize the RPU.
 *
 */

struct nrf_wifi_cmd_sys_deinit {
	/** umac header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
} __NRF_WIFI_PKD;

#define NRF_WIFI_HE_GI_800NS 0
#define NRF_WIFI_HE_GI_1600NS 1
#define NRF_WIFI_HE_GI_3200NS 2

#define NRF_WIFI_HE_LTF_3200NS 0
#define NRF_WIFI_HE_LTF_6400NS 1
#define NRF_WIFI_HE_LTF_12800NS 2

/**
 * @brief This structure defines the command used to configure
 *  High-Efficiency Guard Interval(HE-GI) and High-Efficiency Long Training Field (HE-LTF).
 *
 *  HE-GI duration determines the guard interval length used in the HE transmission.
 *  HE-LTF is used for channel estimation and signal detection in HE transmissions.
 *
 */

struct nrf_wifi_cmd_he_gi_ltf_config {
	/** umac header, see &nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** wdev interface id */
	unsigned char wdev_id;
	/** HE GI type (NRF_WIFI_HE_GI_800NS/NRF_WIFI_HE_GI_1600NS/NRF_WIFI_HE_GI_3200NS) */
	unsigned char he_gi_type;
	/** HE LTF (NRF_WIFI_HE_LTF_3200NS/NRF_WIFI_HE_LTF_6400NS/NRF_WIFI_HE_LTF_12800NS) */
	unsigned char he_ltf;
	/** Fixed HE GI & LTF values can be enabled and disabled */
	unsigned char enable;
} __NRF_WIFI_PKD;

#define		NRF_WIFI_DISABLE	0
#define		NRF_WIFI_ENABLE		1
/**
 * @brief This enum represents the different types of preambles used.
 *  Preambles are sequences of known symbols transmitted before the actual
 *  data transmission to enable synchronization, channel estimation, and
 *  frame detection at the receiver.
 *
 */
enum rpu_pkt_preamble {
	/** Short preamble packet */
	RPU_PKT_PREAMBLE_SHORT,
	/** Long preamble packet */
	RPU_PKT_PREAMBLE_LONG,
	/** mixed preamble packet */
	RPU_PKT_PREAMBLE_MIXED,
	/** Highest preamble type currently defined */
	RPU_PKT_PREAMBLE_MAX,
};


/**
 * @brief This structure describes different Physical Layer (PHY) configuration parameters used
 *  in RF test and Radio test scenarios. These parameters are specific to testing and evaluating
 *  the performance of the radio hardware.
 *
 */
struct rpu_conf_params {
	/** Unused. Number of spatial streams supported. Support is there for 1x1 only. */
	unsigned char nss;
	/** Unused */
	unsigned char antenna_sel;
	/** An array containing RF & baseband control params */
	unsigned char rf_params[NRF_WIFI_RF_PARAMS_SIZE];
	/** Not required */
	unsigned char tx_pkt_chnl_bw;
	/** WLAN packet formats. 0->Legacy 1->HT 2->VHT 3->HE(SU) 4->HE(ERSU) and 5->HE(TB) */
	unsigned char tx_pkt_tput_mode;
	/** Short Guard enable/disable */
	unsigned char tx_pkt_sgi;
	/** Not required */
	unsigned char tx_pkt_nss;
	/** Preamble type. 0->short, 1->Long and 2->Mixed */
	unsigned char tx_pkt_preamble;
	/** Not used */
	unsigned char tx_pkt_stbc;
	/** 0->BCC 1->LDPC. Supporting only BCC in nRF7002 */
	unsigned char tx_pkt_fec_coding;
	/** Valid MCS number between 0 to 7 */
	signed char tx_pkt_mcs;
	/** Legacy rate to be used in Mbps (1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54) */
	signed char tx_pkt_rate;
	/** Copy OTP params to this memory */
	signed char phy_threshold;
	/** Calibration bit map value. refer NRF_WIFI_DEF_PHY_CALIB */
	unsigned int phy_calib;
	/** Radio test mode or System mode selection */
	signed int op_mode;
	/** Channel related info viz, channel, bandwidth, primary 20 offset */
	struct chan_params chan;
	/** Value of 0 means continuous transmission.Greater than 1 is invalid */
	unsigned char tx_mode;
	/** Number of packets to be transmitted. Any number above 0.
	 *  Set -1 for continuous transmission
	 */
	signed int tx_pkt_num;
	/** Length of the packet (in bytes) to be transmitted */
	unsigned short tx_pkt_len;
	/** Desired TX power in dBm in the range 0 dBm to 21 dBm in steps of 1 dBm */
	unsigned int tx_power;
	/** Transmit WLAN packet */
	unsigned char tx;
	/** Receive WLAN packet */
	unsigned char rx;
	/**  Not required */
	unsigned char aux_adc_input_chain_id;
	/**  Unused */
	unsigned char agg;
	/** Select HE LTF type viz, 0->1x, 1->2x and 2->4x */
	unsigned char he_ltf;
	/** Select HE LTF type viz, 0->0.8us, 1->1.6us and 2->3.2us */
	unsigned char he_gi;
	/** Not required */
	unsigned char set_he_ltf_gi;
	/** Not required */
	unsigned char power_save;
	/** Not required */
	unsigned int rts_threshold;
	/** Not required */
	unsigned int uapsd_queue;
	/** Interval between TX packets in us (Min: 200, Max: 200000, Default: 200) */
	unsigned int tx_pkt_gap_us;
	/** Configure WLAN antenna switch(0-separate/1-shared) */
	unsigned char wlan_ant_switch_ctrl;
	/** Switch to control the BLE antenna or shared WiFi antenna */
	unsigned char ble_ant_switch_ctrl;
	/** Resource unit (RU) size (26,52,106 or 242) */
	unsigned char ru_tone;
	/** Location of resource unit (RU) in 20 MHz spectrum */
	unsigned char ru_index;
	/** Desired tone frequency to be transmitted */
	signed char tx_tone_freq;
	/** RX LNA gain */
	unsigned char lna_gain;
	/** RX BB gain */
	unsigned char bb_gain;
	/** Number of RX samples to be captured */
	unsigned short int capture_length;
	/** Configure WLAN to bypass regulatory */
	unsigned char bypass_regulatory;
	/** Two letter country code (00: Default for WORLD) */
	unsigned char country_code[NRF_WIFI_COUNTRY_CODE_LEN];
	/** Contention window value to be configured */
	unsigned int tx_pkt_cw;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to configure the RPU with different
 *  PHY configuration parameters specifically designed for RF test and Radio test scenarios.
 *  The command is intended to set up the RPU for testing and evaluating the performance
 *  of the radio hardware.
 *
 */

struct nrf_wifi_cmd_mode_params {
	/** UMAC header, See &struct nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** configuration parameters of different modes see &union rpu_conf_params */
	struct rpu_conf_params conf;
	/** Packet length */
	unsigned short pkt_length[MAX_TX_AGG_SIZE];
	/** Packet ddr pointer */
	unsigned int ddr_ptrs[MAX_TX_AGG_SIZE];
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the parameters required to initialize a radio test.
 *
 */
struct nrf_wifi_radio_test_init_info {
	/** An array containing RF & baseband control params */
	unsigned char rf_params[NRF_WIFI_RF_PARAMS_SIZE];
	/** Channel related info viz, channel, bandwidth, primary 20 offset */
	struct chan_params chan;
	/** Phy threshold value to be sent to LMAC in channel programming */
	signed char phy_threshold;
	/** Calibration bit map value. refer phy_rf_params.h NRF_WIFI_DEF_PHY_CALIB */
	unsigned int phy_calib;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to initialize a radio test.
 *
 */
struct nrf_wifi_cmd_radio_test_init {
	/** UMAC header, @ref nrf_wifi_sys_head*/
	struct nrf_wifi_sys_head sys_head;
	/** radiotest init configuration parameters @ref nrf_wifi_radio_test_init_info */
	struct nrf_wifi_radio_test_init_info conf;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to enable or disable the reception (Rx).
 *  It allows controlling the radio hardware's receive functionality to start or stop listening
 *  for incoming data frames.
 */

struct nrf_wifi_cmd_rx {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** rx configuration parameters @ref rpu_conf_rx_radio_test_params */
	struct rpu_conf_rx_radio_test_params conf;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to retrieve statistics from the RPU.
 *
 */
struct nrf_wifi_cmd_get_stats {
	/** UMAC header, @ref nrf_wifi_sys_head*/
	struct nrf_wifi_sys_head sys_head;
	/** Statistics type &enum rpu_stats_type */
	signed int stats_type;
	/** Production mode or FCM mode */
	signed int op_mode;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to clear or reset statistics.
 *
 *
 */
struct nrf_wifi_cmd_clear_stats {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** Type of statistics to clear &enum rpu_stats_type */
	signed int stats_type;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the command used to obtain power monitor information
 *  specific to different data types.
 *
 */
struct nrf_wifi_cmd_pwr {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** Type of Control info that host need */
	signed int data_type;
} __NRF_WIFI_PKD;

/**
 * @brief Structure for coexistence (coex) switch configuration.
 *
 */
struct coex_wlan_switch_ctrl {
	/** Host to coexistence manager message id */
	signed int rpu_msg_id;
	/** Switch configuration value */
	signed int switch_A;
} __NRF_WIFI_PKD;

/**
 * @brief The structure represents the command used to configure the Wi-Fi side shared switch
 *  for Bluetooth coexistence (btcoex).
 *
 */
struct nrf_wifi_cmd_btcoex {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** Switch configuration data */
	struct coex_wlan_switch_ctrl conf;
} __NRF_WIFI_PKD;

/**
 * @brief The structure defines the parameters used to configure the coexistence hardware.
 *
 */

struct rpu_cmd_coex_config_info {
	/** Length of coexistence configuration data */
	unsigned int len;
	/** Coexistence configuration data */
	unsigned char coex_cmd[0];
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to configure the coexistence hardware.
 *
 */
struct nrf_wifi_cmd_coex_config {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** Coexistence configuration data. @ref rpu_cmd_coex_config_info */
	struct rpu_cmd_coex_config_info coex_config_info;
} __NRF_WIFI_PKD;

/**
 * @brief This structure describes the coexistence configuration data received
 *  in the NRF_WIFI_EVENT_COEX_CONFIG event.
 *
 */
struct rpu_evnt_coex_config_info {
	/** Length of coexistence configuration data */
	unsigned int len;
	/** Coexistence configuration data */
	unsigned char coex_event[0];
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the event used to represent coexistence configuration.
 *
 */
struct nrf_wifi_event_coex_config {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** Coexistence configuration data in the event. @ref rpu_evnt_coex_config_info */
	struct rpu_evnt_coex_config_info coex_config_info;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to fix the transmission (Tx) data rate.
 *  The command allows setting a specific data rate for data transmission, ensuring that the
 *  system uses the designated rate instead of dynamically adapting to changing channel conditions.
 *
 */

struct nrf_wifi_cmd_fix_tx_rate {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** refer see &enum rpu_tput_mode */
	unsigned char rate_flags;
	/** fixed_rate: -1 Disable fixed rate and use ratecontrol selected rate
	 *  fixed rate: >0 legacy rates: 1,2,55,11,6,9,12,18,24,36,48,54
	 *		  11N VHT HE  : MCS index 0 to 7.
	 */
	int fixed_rate;
} __NRF_WIFI_PKD;

/**
 * @brief This structure describes rf test command information.
 *
 */
struct rpu_cmd_rftest_info {
	/** length of the rf test command */
	unsigned int len;
	/** Rf test command data */
	unsigned char rfcmd[0];
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used for RF (Radio Frequency) testing.
 *  RF test commands are specifically designed to configure and control the radio hardware
 *  for conducting tests and evaluating its performance in various scenarios.
 *
 */
struct nrf_wifi_cmd_rftest {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** @ref rpu_cmd_rftest_info */
	struct rpu_cmd_rftest_info rf_test_info;
} __NRF_WIFI_PKD;

/**
 * @brief This structure describes rf test event information.
 *
 */
struct rpu_evnt_rftest_info {
	/** length of the rf test event */
	unsigned int len;
	/** Rf test event data */
	unsigned char rfevent[0];
} __NRF_WIFI_PKD;

/**
 * @brief This structure describes the event generated during RF (Radio Frequency) testing.
 *
 */
struct nrf_wifi_event_rftest {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** @ref rpu_evnt_rftest_info */
	struct rpu_evnt_rftest_info rf_test_info;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the power data event generated in response to
 *  the NRF_WIFI_CMD_PWR command.
 *
 *  The NRF_WIFI_CMD_PWR command is used to retrieve power-related data or measurements
 *  from the radio hardware.
 *
 */
struct nrf_wifi_event_pwr_data {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** Power monitor command status info */
	signed int mon_status;
	/** Data */
	signed int data_type;
	/** Data that host may want to read from Power IP */
	struct nrf_wifi_rpu_pwr_data data;
} __NRF_WIFI_PKD;

/**
 * @brief This structure is a comprehensive combination of all the firmware statistics
 *  that the RPU (Radio Processing Unit) can provide.
 *
 */
struct rpu_fw_stats {
	/** PHY statistics  @ref rpu_phy_stats */
	struct rpu_phy_stats phy;
#ifndef CONFIG_NRF700X_RADIO_TEST
	/** LMAC statistics @ref rpu_lmac_stats */
	struct rpu_lmac_stats lmac;
	/** UMAC statistics @ref rpu_umac_stats */
	struct rpu_umac_stats umac;
#endif /* !CONFIG_NRF700X_RADIO_TEST */
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the event that provides RPU statistics in response
 *  to the command NRF_WIFI_CMD_GET_STATS in a wireless communication system.
 *
 *  The NRF_WIFI_CMD_GET_STATS command is used to request various statistics from the RPU.
 *
 */

struct nrf_wifi_umac_event_stats {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** All the statistics that the firmware can provide @ref rpu_fw_stats*/
	struct rpu_fw_stats fw;
} __NRF_WIFI_PKD;

/**
 * @brief This enum defines various error status values that may occur during a radio test.
 *
 */
enum nrf_wifi_radio_test_err_status {
	/** Command success  */
	NRF_WIFI_UMAC_CMD_SUCCESS = 1,
	/** Invalid channel error */
	NRF_WIFI_UMAC_INVALID_CHNL
};

/**
 * @brief This structure defines an event that indicates the error status values that may occur
 *  during a radio test. It serves as a response to the radio test commands.
 *
 */
struct nrf_wifi_umac_event_err_status {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** status of the command, Fail/success &enum nrf_wifi_radio_test_err_status */
	unsigned int status;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the UMAC initialization done event.
 *  The event is sent by the RPU (Radio Processing Unit) in response to
 *  the NRF_WIFI_CMD_INIT command, indicating that the RPU initialization
 *  process has been completed successfully.
 */

struct nrf_wifi_event_init_done {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
} __NRF_WIFI_PKD;

/**
 * @brief structure for UMAC memory pool information.
 *
 */
struct pool_data_to_host {
	/** Size of the memory buffer */
	unsigned int buffer_size;
	/** Number of pool items available for the above memory buffer */
	unsigned char num_pool_items;
	/** Maximum pools allocated at any point of time */
	unsigned char items_num_max_allocated;
	/** Currently allocated pools */
	unsigned char items_num_cur_allocated;
	/** Total number of pool allocated */
	unsigned int items_num_total_allocated;
	/** Number of times this memory pool is full */
	unsigned int items_num_not_allocated;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the event that provides UMAC (Upper MAC) internal
 *  memory statistics in response to the NRF_WIFI_CMD_UMAC_INT_STATS command.
 *
 */
struct umac_int_stats {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** See @ref pool_data_to_host */
	struct pool_data_to_host scratch_dynamic_memory_info[56];
	/** See @ref pool_data_to_host */
	struct pool_data_to_host retention_dynamic_memory_info[56];
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the event that indicates the completion of UMAC
 *  deinitialization. The RPU sends this event as a response to the NRF_WIFI_CMD_DEINIT
 *  command, signaling that the UMAC has been successfully deinitialized.
 */

struct nrf_wifi_event_deinit_done {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
} __NRF_WIFI_PKD;

#endif /* __HOST_RPU_SYS_IF_H__ */
