/*
 *
 *Copyright (c) 2026 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @addtogroup nrf71_wifi_fw_if Wi-Fi driver and firmware interface
 * @{
 * @brief System interface between host and RPU
 */

#ifndef __NRF71_WIFI_COMMON_H__
#define __NRF71_WIFI_COMMON_H__

#include "nrf71_wifi_rf.h"
#include "common/pack_def.h"


#define NRF_WIFI_RF_PARAMS_CONF_SIZE 42

#define MAX_NUM_OF_RX_QUEUES 3
#define NRF_WIFI_MAX_SAP_CLIENTS 8
#define RPU_DATA_CMD_SIZE_MAX_RX 8

#define NRF_WIFI_RPU_PWR_DATA_TYPE_LFC_ERR 0
#define NRF_WIFI_RPU_PWR_DATA_TYPE_VBAT_MON 1
#define NRF_WIFI_RPU_PWR_DATA_TYPE_TEMP 2
#define NRF_WIFI_RPU_PWR_DATA_TYPE_ALL 3
#define NRF_WIFI_RPU_PWR_DATA_TYPE_MAX 4

#define NRF_WIFI_ETH_ADDR_LEN 6
#define NRF_WIFI_COUNTRY_CODE_LEN 2
#define MAX_TX_AGG_SIZE 16
#define MAX_RX_BUFS_PER_EVNT 64

#define AGGR_DISABLE 0
#define AGGR_ENABLE 1

#define NRF_WIFI_UMAC_VER(version) (((version)&0xFF000000) >> 24)
#define NRF_WIFI_UMAC_VER_MAJ(version) (((version)&0x00FF0000) >> 16)
#define NRF_WIFI_UMAC_VER_MIN(version) (((version)&0x0000FF00) >> 8)
#define NRF_WIFI_UMAC_VER_EXTRA(version) (((version)&0x000000FF) >> 0)

/**
 * @brief Common header included in each command/event.
 *
 * This structure encapsulates the common information included at the start of
 * each command/event exchanged with the RPU.
 */
struct host_rpu_msg_hdr {
	/** Length of the message. */
	unsigned int len;
	/** Flag to indicate whether the recipient is expected to resubmit
	 *  the cmd/event address back to the trasmitting entity.
	 */
	unsigned int resubmit;
} __NRF_WIFI_PKD;

/**
 * @brief This enum defines the different categories of messages that can be exchanged between
 *  the Host and the RPU.
 *
 */
enum nrf_wifi_host_rpu_msg_type {
	/** System interface messages */
	NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
	/** Unused */
	NRF_WIFI_HOST_RPU_MSG_TYPE_SUPPLICANT,
	/** Data path messages */
	NRF_WIFI_HOST_RPU_MSG_TYPE_DATA,
	/** Control path messages */
	NRF_WIFI_HOST_RPU_MSG_TYPE_UMAC
};
/**
 * @brief This structure defines the common message header used to encapsulate each message
 *  exchanged between the Host and UMAC.
 *
 */

struct host_rpu_msg {
	/** Header */
	struct host_rpu_msg_hdr hdr;
	/** Type of the RPU message see &enum nrf_wifi_host_rpu_msg_type */
	signed int type;
	/** Actual message */
	signed char msg[0];
} __NRF_WIFI_PKD;

#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_VO (1 << 0)
#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_VI (1 << 1)
#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_BE (1 << 2)
#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_BK (1 << 3)

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

#define MAX_MGMT_BUFS 16

#define MAX_RF_CALIB_DATA 900

#define PHY_THRESHOLD_NORMAL (-65)
#define PHY_THRESHOLD_PROD_MODE (-93)

#define MAX_TX_STREAMS 1
#define MAX_RX_STREAMS 1

#define MAX_NUM_VIFS 2
#define MAX_NUM_STAS 2
#define MAX_NUM_APS 1

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
	/** Offloaded Raw TX statistics */
	RPU_STATS_TYPE_OFFLOADED_RAW_TX,
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
	/** Command for WIFI & SR coexistence */
	NRF_WIFI_CMD_SRCOEX,
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
	/** Command to set channel in promiscuous, monitor  & packet injector mode */
	NRF_WIFI_CMD_CHANNEL,
	/** Command to configure promiscuous mode, Monitor mode & packet injector mode */
	NRF_WIFI_CMD_RAW_CONFIG_MODE,
	/** Command to configure promiscuous mode, Monitor mode filter*/
	NRF_WIFI_CMD_RAW_CONFIG_FILTER,
	/** Command to configure packet injector mode or Raw Tx mode */
	NRF_WIFI_CMD_RAW_TX_PKT,
	/** Command to reset interface statistics */
	NRF_WIFI_CMD_RESET_STATISTICS,
	/** Command to configure raw tx offloading parameters */
	NRF_WIFI_CMD_OFFLOAD_RAW_TX_PARAMS,
	/** Command to enable/disable raw tx offloading */
	NRF_WIFI_CMD_OFFLOAD_RAW_TX_CTRL,
	/** Configure SGI/LGI */
	NRF_WIFI_CMD_GI_CONFIG,
	/** Enable BCC OR LDPC */
	NRF_WIFI_CMD_CODING_TYPE_CONFIG,
	/** Command to program sqi */
	NRF_WIFI_UMAC_CMD_SQI_CONFIG,
	/** Update XO value */
	NRF_WIFI_CMD_UPDATE_XO,
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
	/** Response to NRF_WIFI_CMD_SRCOEX. */
	NRF_WIFI_EVENT_COEX_CONFIG,
	/** Response to NRF_WIFI_CMD_UMAC_INT_STATS */
	NRF_WIFI_EVENT_INT_UMAC_STATS,
	/** Command status events for radio test commands*/
	NRF_WIFI_EVENT_RADIOCMD_STATUS,
	/** Response to NRF_WIFI_CMD_CHANNEL */
	NRF_WIFI_EVENT_CHANNEL_SET_DONE,
	/** Response to NRF_WIFI_CMD_RAW_CONFIG_MODE */
	NRF_WIFI_EVENT_MODE_SET_DONE,
	/** Response to NRF_WIFI_CMD_RAW_CONFIG_FILTER */
	NRF_WIFI_EVENT_FILTER_SET_DONE,
	/** Tx done event for the Raw Tx */
	NRF_WIFI_EVENT_RAW_TX_DONE,
	/** Command status events for offloaded raw tx commands */
	NRF_WIFI_EVENT_OFFLOADED_RAWTX_STATUS,
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
	/** Operating band see enum op_band */
	unsigned int op_band;
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
	/** Number of packets flushed from reorder buffer before going to sleep */
	unsigned int reorder_flush_pkt_count;
	/** Unprotected error data frames received in security mode */
	unsigned int unsecured_data_error;
	/** Packets received with null skb pointer from LMAC in coalesce event */
	unsigned int pkts_in_null_skb_pointer_event;
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
	/** RPU hardware lockup event detection count */
	unsigned int rpu_hw_lockup_count;
	/** RPU hardware lockup recovery completed count */
	unsigned int rpu_hw_lockup_recovery_done;
	/** Number of Signal quality threshold set commands from host  */
	unsigned int  SQIThresholdCmdsCnt;
	/** Number of Signal quality configuration commands from host  */
	unsigned int  SQIConfigCmdsCnt;
	/** Signal Quality Indication event from lmac   */
	unsigned int  SQIEventsCnt;
	/** Configured sleep type. */
	unsigned int SleepType;
	/** Number of warm boot /sleep cycles in RPU  */
	unsigned int warmBootCnt;
} __NRF_WIFI_PKD;


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
 * NRF_WIFI_CMD_INIT for all modes. The NRF_WIFI_CMD_INIT command is typically used to
 * initialize the Wi-Fi module and prepare it for further communication.
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
	/** MAC address of the interface. Not applicable to Radio Test mode */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** An array containing RF & baseband control params */
	unsigned char rf_params[NRF_WIFI_RF_PARAMS_SIZE];
	/** Indicates whether the rf_params has a valid value */
	unsigned char rf_params_valid;
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
} __NRF_WIFI_PKD;

/**
 * @brief This enum defines different types of operating bands in 6G
 * Inband scanning.
 */
enum scan_type_6g {
	/** Active scan : PSC channels scanning */
	ACTIVE_PSC,
	/** Passive scan : Unsolicited probe responses*/
	UNSOLIC_PROBE_RESPONSE,
	/** Passive scan : Fast Initial Link Setup Frames */
	FILS
};
/**
 * @brief This enum defines different types of operating bands.
 *
 */
enum op_band {
	/** All bands */
	BAND_ALL,
	/** 2.4Ghz band */
	BAND_24G,
	/** 5 GHz band */
	BAND_5G,
	/** 6 GHz band */
	BAND_6G,
	/** 2.4Ghz & 5GHz band */
	BAND_DUAL
};

/**
 * @brief This enum defines keep alive state
 *
 */
enum nrf_wifi_keep_alive_status {
	/** Keep alive feature disabled */
	KEEP_ALIVE_DISABLED = 0,
	/** Keep alive feature enabled */
	KEEP_ALIVE_ENABLED = 1
};

/**
 * @brief This enum specifies the type of frames used to retrieve buffered data
 *  from the AP in power save mode.
 */
enum ps_exit_strategy {
	/** Uses an intelligent algo and decide whether to
	 * stay or exit power save mode to receive buffered frames.
	 */
	INT_PS = 0,
	/** Exits power save mode for every TIM */
	EVERY_TIM
};

struct rx_buf_pool_params {
	unsigned short buf_sz;
	unsigned short num_bufs;
} __NRF_WIFI_PKD;

struct temp_vbat_config {
	unsigned int temp_based_calib_en;
	unsigned int temp_calib_bitmap;
	unsigned int vbat_calibp_bitmap;
	unsigned int temp_vbat_mon_period;
	int vth_very_low;
	int vth_low;
	int vth_hi;
	int temp_threshold;
	int vbat_threshold;
} __NRF_WIFI_PKD;

#define TWT_EXTEND_SP_EDCA  0x1
#define DISABLE_DFS_CHANNELS 0x2

#define NRF_WIFI_FTM_INITIATOR_ENABLE (1 << 0)
#define NRF_WIFI_FTM_RESPONDER_ENABLE (1 << 1)
#define NRF_WIFI_LCI_ENABLE (1 << 2)
#define NRF_WIFI_CIVILOC_ENABLE (1 << 3)
#define NRF_WIFI_GAS_ENABLE (1 << 4)

struct nrf_wifi_ftm_loc_params {
	/** Enable = 1, Disable=0 */
	unsigned char ftm_enable;
	/** Capabilities NRF_WIFI_FTM_INITIATOR_ENABLE & etc */
	unsigned char capabilities;
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
	/** Country code to set */
	unsigned char country_code[NRF_WIFI_COUNTRY_CODE_LEN];
	/** Operating band see enum op_band */
	unsigned int op_band;
	/** Offload mgmt buffer refill to UMAC when enabled */
	unsigned char mgmt_buff_offload;
	/** Enable features from driver config */
	unsigned int feature_flags;
	/** To deactivate beamforming, By default the RPU enables the beamforming feature.
	 *  If a user wishes to turn it off, they should set this parameter to 1.
	 */
	unsigned int disable_beamforming;
	/** The RPU uses this value (in seconds) to decide how long to wait
	 *  without receiving beacons before disconnection.
	 */
	unsigned int discon_timeout;
	/** RPU uses QoS null frame or PS-Poll frame to retrieve buffered frames
	 * from the AP in power save @ref ps_exit_strategy.
	 */
	unsigned char ps_exit_strategy;
	/** The RPU uses this value to configure watchdog timer */
	unsigned int watchdog_timer_val;
	/** The RPU uses this value to decide whether keep alive
	 *  feature is enabled or not see enum keep_alive_status
	 */
	unsigned char keep_alive_enable;
	/** The RPU uses this value(in seconds) for periodicity of the keep
	 *  alive frame.
	 */
	unsigned int keep_alive_period;
	/** The RPU uses this value to define the limit on display scan BSS entries.
	 *  By default, the limit is set to 250 in scan-only mode and 150 in regular mode.
	 *  If this value is greater than 0, it overrides the default limits.
	 */
	unsigned int display_scan_bss_limit;
	/** The RPU uses this value to enable/disable priority window for Wi-Fi scan
	 *  in the case of coexistence with Short Range radio.
	 */
	unsigned int coex_disable_ptiwin_for_wifi_scan;
	/** The RPU uses this value to enable : 1 or disable : 0 the transmission of
	 *  beacon and probe responses to the host when mgmt buffer offloading is enabled.
	 */
	unsigned char raw_scan_enable;
	/** The RPU uses this value for the number of PS-POLL failures
	 *  to switch from ps-poll power save mode to QoS null-based
	 *  power save mode.
	 *  MIN: 10 (default), MAX: 0xfffffffe.
	 *  Set to 0xffffffff to disable this feature.
	 */
	unsigned int max_ps_poll_fail_cnt;
	/** Enables or disables RX STBC in HT mode.
	 *  By default, RX STBC is enabled.
	 */
	unsigned int stbc_enable_in_ht;
	/* Enables(1) or Disables(0) Dynamic bandwidth signalling control */
	unsigned int dbs_war_ctrl;
	/* Enables(1) or Disables(0) Dynamic ED*/
	unsigned int dynamic_ed;
	/** The RPU uses this value to enable/disable priority window for Wi-Fi scan
	 *  in the case of coexistence with Short Range radio.
	 */
	unsigned int inband_scan_type;
	/** @ref nrf_wifi_ftm_loc_params */
	struct nrf_wifi_ftm_loc_params ftm_loc_params;
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

struct nrf_wifi_cmd_mode {
	struct nrf_wifi_sys_head sys_head;
	signed int mode;

} __NRF_WIFI_PKD;
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
	/** Switch to control the SR antenna or shared WiFi antenna */
	unsigned char sr_ant_switch_ctrl;
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
	/** Capture timeout in seconds */
	unsigned short int capture_timeout;
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
 * @brief This structure represents the channel parameters to configure specific channel.
 *
 */
struct nrf_wifi_cmd_set_channel {
	/** UMAC header, @ref nrf_wifi_sys_head. */
	struct nrf_wifi_sys_head sys_head;
	/** Interface index. */
	unsigned char if_index;
	/** channel parameters, @ref chan_params. */
	struct chan_params chan;
} __NRF_WIFI_PKD;

/**
 * @brief This enum represents different types of filters used.
 */

enum wifi_packet_filter {
	/** Support management, data and control packet sniffing. */
	NRF_WIFI_PACKET_FILTER_ALL = 0x1,
	/** Support only sniffing of management packets. */
	NRF_WIFI_PACKET_FILTER_MGMT = 0x2,
	/** Support only sniffing of data packets. */
	NRF_WIFI_PACKET_FILTER_DATA = 0x4,
	/** Support only sniffing of control packets. */
	NRF_WIFI_PACKET_FILTER_CTRL = 0x8,
};

/**
 * @brief This structure defines the command used to configure
 *  promiscuous mode/Monitor mode/Packet injector mode.
 */
struct nrf_wifi_cmd_raw_config_mode {
	/** UMAC header, @ref nrf_wifi_sys_head. */
	struct nrf_wifi_sys_head sys_head;
	/** Interface index. */
	unsigned char if_index;
	/** Wireless device operating mode. */
	unsigned char op_mode;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to configure
 *  filters and capture length in promiscuous and monitor modes.
 */
struct nrf_wifi_cmd_raw_config_filter {
	/** UMAC header, @ref nrf_wifi_sys_head. */
	struct nrf_wifi_sys_head sys_head;
	/** Interface index. */
	unsigned char if_index;
	/** Wireless device operating mode filters for Promiscuous/Monitor modes. */
	unsigned char filter;
	/** capture length. */
	unsigned short capture_len;
} __NRF_WIFI_PKD;

/**
 * @brief This enum represents the queues used to segregate the TX frames depending on
 * their QoS categories. A separate queue is used for Beacon frames / frames
 * transmitted during DTIM intervals.
 */

enum UMAC_QUEUE_NUM {
	/** Queue for frames belonging to the "Background" Access Category. */
	UMAC_AC_BK = 0,
	/** Queue for frames belonging to the "Best-Effort" Access Category. */
	UMAC_AC_BE,
	/** Queue for frames belonging to the "Video" Access Category. */
	UMAC_AC_VI,
	/** Queue for frames belonging to the "Voice" Access Category. */
	UMAC_AC_VO,
	/** Queue for frames belonging to the "Beacon" Access Category. */
	UMAC_AC_BCN,
	/** Maximum number of transmit queues supported. */
	UMAC_AC_MAX_CNT
};

/**
 * @brief This structure defines the raw tx parameters used in packet injector mode.
 *
 */
struct nrf_wifi_raw_tx_pkt {
	/** Queue number will be BK, BE, VI, VO and BCN refer UMAC_QUEUE_NUM. */
	unsigned char queue_num;
	/** Descriptor identifier or token identifier. */
	unsigned char desc_num;
	/** Number of times a packet should be transmitted at each possible rate. */
	unsigned char rate_retries;
	/** refer see &enum rpu_tput_mode. */
	unsigned char rate_flags;
	/** rate: legacy rates: 1,2,55,11,6,9,12,18,24,36,48,54
	 *		  11N VHT HE  : MCS index 0 to 7.
	 **/
	unsigned char rate;

	/** AGGR_DISABLE(0)/AGGR_ENABLE(1) */
	unsigned char aggregation;
	/** Number of frames in this command */
	unsigned char num_frames;
	/** Packet lengths of frames */
	unsigned short pkt_length[MAX_TX_AGG_SIZE];
	/** Starting Physical address of each frame in Ext-RAM after dma_mapping */
	unsigned int frame_ddr_pointer[MAX_TX_AGG_SIZE];
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to configure packet injector mode.
 *
 */
struct nrf_wifi_cmd_raw_tx {
	/** UMAC header, @ref nrf_wifi_sys_head. */
	struct nrf_wifi_sys_head sys_head;
	/** Interface index. */
	unsigned char if_index;
	/** Raw tx packet information. */
	struct nrf_wifi_raw_tx_pkt  raw_tx_info;
} __NRF_WIFI_PKD;

/**
 * @brief This enum provides a list of different raw tx offloading types.
 */
enum nrf_wifi_offload_rawtx_ctrl_type {
	NRF_WIFI_OFFLOAD_TX_STOP,
	NRF_WIFI_OFFLOAD_TX_START,
	NRF_WIFI_OFFLOAD_TX_CONFIG,
};

/**
 * @brief This structure defines the offloaded raw tx control information.
 *
 */
struct nrf_wifi_offload_ctrl_params {
	/** Time interval in micro seconds */
	unsigned int period_in_us;
	/** Transmit power in dBm ( 0 to 20) */
	int tx_pwr;
	/** Channel number */
	struct chan_params chan;
} __NRF_WIFI_PKD;

#define NRF_WIFI_ENABLE_HE_SU 0x40
#define NRF_WIFI_ENABLE_HE_ER_SU 0x80

/**
 * @brief This structure defines the offloading raw tx parameters
 *
 */
struct nrf_wifi_offload_tx_ctrl {
	/** Packet lengths of frames, min 26 bytes and max 600 bytes */
	unsigned int pkt_length;
	/** Rate preamble type (USE_SHORT_PREAMBLE/DONT_USE_SHORT_PREAMBLE) */
	unsigned int rate_preamble_type;
	/** Number of times a packet should be transmitted at each possible rate */
	unsigned int rate_retries;
	/** Rate: legacy rates: 1,2,55,11,6,9,12,18,24,36,48,54
	 * 11N VHT HE: MCS index 0 to 7.
	 */
	unsigned int rate;
	/** Refer see &enum rpu_tput_mode */
	unsigned int rate_flags;
	/** HE GI type (NRF_WIFI_HE_GI_800NS/NRF_WIFI_HE_GI_1600NS/NRF_WIFI_HE_GI_3200NS) */
	unsigned char he_gi_type;
	/** HE LTF (NRF_WIFI_HE_LTF_3200NS/NRF_WIFI_HE_LTF_6400NS/NRF_WIFI_HE_LTF_12800NS) */
	unsigned char he_ltf;
	/** Payload pointer */
	unsigned int  pkt_ram_ptr;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used for  offloading Raw tx
 *
 */
struct nrf_wifi_cmd_offload_raw_tx_params {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** Id of the interface */
	unsigned int wdev_id;
	/** Offloaded raw tx control information, @ref nrf_wifi_offload_ctrl_params */
	struct nrf_wifi_offload_ctrl_params ctrl_info;
	/** Offloaded raw tx params, @ref nrf_wifi_offload_tx_ctrl */
	struct nrf_wifi_offload_tx_ctrl tx_params;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used for  offloading Raw tx
 *
 */
struct nrf_wifi_cmd_offload_raw_tx_ctrl {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** Id of the interface */
	unsigned int wdev_id;
	/** Offloading type @ref nrf_wifi_offload_rawtx_ctrl_type */
	unsigned char ctrl_type;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines an event that indicates set channel command done.
 *
 */
struct nrf_wifi_event_set_channel {
	/** UMAC header, @ref nrf_wifi_sys_head. */
	struct nrf_wifi_sys_head sys_head;
	/** Interface index. */
	unsigned char if_index;
	/** channel number. */
	struct chan_params chan;
	/** status of the set channel command, success(0)/Fail(-1). */
	int status;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines an event that indicates set raw config
 * mode command done.
 *
 */
struct nrf_wifi_event_raw_config_mode {
	/** UMAC header, @ref nrf_wifi_sys_head. */
	struct nrf_wifi_sys_head sys_head;
	/** Interface index. */
	unsigned char if_index;
	/** Operating mode. */
	unsigned char op_mode;
	/** status of the set raw config mode command, success(0)/Fail(-1). */
	int status;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines an event that indicates set raw config
 * filter command done.
 *
 */
struct nrf_wifi_event_raw_config_filter {
	/** UMAC header, @ref nrf_wifi_sys_head. */
	struct nrf_wifi_sys_head sys_head;
	/** Interface index. */
	unsigned char if_index;
	/** mode filter configured. */
	unsigned char filter;
	/** capture len configured. */
	unsigned short capture_len;
	/** status of the set raw filter command, success(0)/Fail(-1). */
	int status;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines an event that indicates the Raw tx done.
 *
 */
struct nrf_wifi_event_raw_tx_done {
	/** UMAC header, @ref nrf_wifi_sys_head. */
	struct nrf_wifi_sys_head sys_head;
	/** descriptor number. */
	unsigned char desc_num;
	/** status of the raw tx packet command, success(0)/Fail(-1). */
	int status;
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
 *  for SR coexistence.
 *
 */
struct nrf_wifi_cmd_srcoex {
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
 * @brief This structure is a comprehensive combination of all the firmware statistics
 *  that the RPU (Radio Processing Unit) can provide in System mode.
 *
 */
struct rpu_sys_fw_stats {
	/** PHY statistics  @ref rpu_phy_stats */
	struct rpu_phy_stats phy;
	/** LMAC statistics @ref rpu_lmac_stats */
	struct rpu_lmac_stats lmac;
	/** UMAC statistics @ref rpu_umac_stats */
	struct rpu_umac_stats umac;
} __NRF_WIFI_PKD;

/**
 * @brief This structure is a comprehensive combination of all the firmware statistics
 *  that the RPU (Radio Processing Unit) can provide in Radio test mode.
 *
 */
struct rpu_rt_fw_stats {
	/** PHY statistics  @ref rpu_phy_stats */
	struct rpu_phy_stats phy;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the Offloaded raw tx debug statistics.
 *
 */
struct rpu_off_raw_tx_fw_stats {
	unsigned int offload_raw_tx_state;
	unsigned int offload_raw_tx_cnt;
	unsigned int offload_raw_tx_complete_cnt;
	unsigned int warm_boot_cnt;
} __NRF_WIFI_PKD;


/**
 * @brief This structure represents the event that provides RPU statistics in response
 * to the command NRF_WIFI_CMD_GET_STATS in a wireless communication system in System
 * mode.
 *
 *  The NRF_WIFI_CMD_GET_STATS command is used to request various statistics from the RPU.
 *
 */

struct nrf_wifi_sys_umac_event_stats {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** All the statistics that the firmware can provide @ref rpu_sys_fw_stats*/
	struct rpu_sys_fw_stats fw;
} __NRF_WIFI_PKD;


/**
 * @brief This structure represents the event that provides RPU statistics in response
 * to the command NRF_WIFI_CMD_GET_STATS in a wireless communication system in Radio
 * test mode.
 *
 *  The NRF_WIFI_CMD_GET_STATS command is used to request various statistics from the RPU.
 *
 */

struct nrf_wifi_rt_umac_event_stats {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** All the statistics that the firmware can provide @ref rpu_rt_fw_stats*/
	struct rpu_rt_fw_stats fw;
} __NRF_WIFI_PKD;


/**
 * @brief This structure represents the event that provides RPU statistics in response
 * to the command NRF_WIFI_CMD_GET_STATS in a wireless communication system in Offloaded
 * raw TX mode.
 *
 *  The NRF_WIFI_CMD_GET_STATS command is used to request various statistics from the RPU.
 *
 */

struct nrf_wifi_off_raw_tx_umac_event_stats {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** All the statistics that the firmware can provide @ref rpu_off_raw_tx_fw_stats*/
	struct rpu_off_raw_tx_fw_stats fw;
} __NRF_WIFI_PKD;


/**
 * @brief This enum defines various command status values that can occur
 * during radio tests and offloaded raw transmissions.
 */
enum nrf_wifi_cmd_status {
	/** Command success  */
	NRF_WIFI_UMAC_CMD_SUCCESS = 1,
	/** Invalid channel error */
	NRF_WIFI_UMAC_INVALID_CHNL,
	/** Invalid power error wrt configured regulatory domain */
	NRF_WIFI_UMAC_INVALID_TXPWR
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

/**
 * @brief This structure describes the command for reset of interface statistics.
 *
 */
struct nrf_wifi_cmd_reset_stats {
	/** UMAC header, @ref nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
} __NRF_WIFI_PKD;

struct nrf_wifi_cmd_gi_config {
	/** umac header, see &nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** Fixed Guard Interval Setting: 1 = Enable fixed guard interval, 0 = Disable (adaptive) */
	unsigned char is_guard_interval_fixed;
	/* Guard Interval Type: 1 = Long Guard Interval (LGI), 0 = Short Guard Interval (SGI) */
	unsigned char guard_interval_type;
} __NRF_WIFI_PKD;

#define NRF_WIFI_LMAC_MAX_RX_BUFS 256


#define HW_SLEEP_ENABLE 2
#define SW_SLEEP_ENABLE 1
#define SLEEP_DISABLE 0
#define HW_DELAY 7200
#define SW_DELAY 5000
#define BCN_TIMEOUT 20000
#define CALIB_SLEEP_CLOCK_ENABLE 1
#define ACTIVE_SCAN_DURATION 50
#define PASSIVE_SCAN_DURATION 130
#define WORKING_CH_SCAN_DURATION 50
#define CHNL_PROBE_CNT 2

#define		PKT_TYPE_MPDU				0
#define		PKT_TYPE_MSDU_WITH_MAC		1
#define		PKT_TYPE_MSDU				2

#define		NRF_WIFI_RPU_PWR_STATUS_SUCCESS	0
#define		NRF_WIFI_RPU_PWR_STATUS_FAIL	-1

/**
 * struct lmac_prod_stats - used to get the production mode stats
 **/

/* Events */
#define MAX_RSSI_SAMPLES 10
struct lmac_prod_stats {
	/*Structure that holds all the debug information in LMAC*/
	unsigned int  resetCmdCnt;
	unsigned int  resetCompleteEventCnt;
	unsigned int  unableGenEvent;
	unsigned int  chProgCmdCnt;
	unsigned int  channelProgDone;
	unsigned int  txPktCnt;
	unsigned int  txPktDoneCnt;
	unsigned int  scanPktCnt;
	unsigned int  internalPktCnt;
	unsigned int  internalPktDoneCnt;
	unsigned int  ackRespCnt;
	unsigned int  txTimeout;
	unsigned int  deaggIsr;
	unsigned int  deaggInptrDescEmpty;
	unsigned int  deaggCircularBufferFull;
	unsigned int  lmacRxisrCnt;
	unsigned int  rxDecryptcnt;
	unsigned int  processDecryptFail;
	unsigned int  prepaRxEventFail;
	unsigned int  rxCorePoolFullCnt;
	unsigned int  rxMpduCrcSuccessCnt;
	unsigned int  rxMpduCrcFailCnt;
	unsigned int  rxOfdmCrcSuccessCnt;
	unsigned int  rxOfdmCrcFailCnt;
	unsigned int  rxDSSSCrcSuccessCnt;
	unsigned int  rxDSSSCrcFailCnt;
	unsigned int  rxCryptoStartCnt;
	unsigned int  rxCryptoDoneCnt;
	unsigned int  rxEventBufFull;
	unsigned int  rxExtramBufFull;
	unsigned int  scanReq;
	unsigned int  scanComplete;
	unsigned int  scanAbortReq;
	unsigned int  scanAbortComplete;
	unsigned int  internalBufPoolNull;
	unsigned int  rpuLockupCnt;
	unsigned int  rpuLockupRecoveryDone;
	unsigned int  SQIThresholdCmdsCnt;
	unsigned int  SQIConfigCmdsCnt;
	unsigned int  SQIEventsCnt;
	unsigned int  SleepType;
	unsigned int  warmBootCnt;
};

/**
 * struct phy_prod_stats - used to get the production mode stats
 **/
struct phy_prod_stats {
	unsigned int ofdm_crc32_pass_cnt;
	unsigned int ofdm_crc32_fail_cnt;
	unsigned int dsss_crc32_pass_cnt;
	unsigned int dsss_crc32_fail_cnt;
	char averageRSSI;
};

union rpu_stats {
	struct lmac_prod_stats lmac_stats;
	struct phy_prod_stats  phy_stats;
};


struct hpqmQueue {
	unsigned int  pop_addr;
	unsigned int  push_addr;
	unsigned int  idNum;
	unsigned int  status_addr;
	unsigned int  status_mask;
} __NRF_WIFI_PKD;


struct INT_HPQ {
	unsigned int  id;
	/* The head and tail values are relative
	 * to the start of the
	 * HWQM register block.
	 */
	unsigned int head;
	unsigned int tail;
} __NRF_WIFI_PKD;



/**
 * @brief lmac firmware config params
 */
struct lmac_fw_config_params {
	/** lmac firmware boot status. LMAC will set to 0x5a5a5a5a after completing boot process. */
	unsigned int boot_status;
	/** lmac firmware version */
	unsigned int version;
	unsigned int lmacRxBufferAddr;
	unsigned int lmacRxMaxDescCnt;
	unsigned int lmacRxDescSize;
	/** rpu config name. this is a string and expected sting is explorer or whisper */
	unsigned char rpu_config_name[16];
	/** rpu config number */
	unsigned char rpu_config_number[8];
	unsigned int numRX;
	unsigned int numTX;
	#define FREQ_2_4_GHZ 1
	#define FREQ_5_GHZ  2
	unsigned int bands;
	unsigned int sysFrequencyInMhz;
	/** queue which contains Free GRAM pointers for commands. */
	struct hpqmQueue FreeCmdPtrQ;
	/** Command pointer queue. Host picks gram pointer from FreeCmdPtrQ,
	 *  populates command in GRAM and submits back to this queue for RPU.
	 */
	struct hpqmQueue cmdPtrQ;
	/** Event pointer queue. Host should pick gram event pointer in isr */
	struct hpqmQueue eventPtrQ;
	struct hpqmQueue freeEventPtrQ;
	struct hpqmQueue SKBGramPtrQ_1;
	struct hpqmQueue SKBGramPtrQ_2;
	struct hpqmQueue SKBGramPtrQ_3;
	/** lmac register address to enable ISR to Host */
	unsigned int HP_lmac_to_host_isr_en;
	/** Address to Clear host ISR */
	unsigned int HP_lmac_to_host_isr_clear;
	/** Address to set ISR to lmac Clear host ISR */
	unsigned int HP_set_lmac_isr;

	#define NUM_32_QUEUES 4
	struct INT_HPQ hpq32[NUM_32_QUEUES];

} __NRF_WIFI_PKD;

/**
 * @brief This enum represents different types of operation modes.
 */
enum wifi_operation_modes {
	/** STA mode setting enable. */
	NRF_WIFI_STA_MODE = 0x1,
	/** Monitor mode setting enable. */
	NRF_WIFI_MONITOR_MODE = 0x2,
	/** TX injection mode setting enable. */
	NRF_WIFI_TX_INJECTION_MODE = 0x4,
	/** Promiscuous mode setting enable. */
	NRF_WIFI_PROMISCUOUS_MODE = 0x8,
	/** AP mode setting enable. */
	NRF_WIFI_AP_MODE = 0x10,
	/** STA-AP mode setting enable. */
	NRF_WIFI_STA_AP_MODE = 0x20,
	/** Max limit check based on current modes supported. */
	WIFI_MODE_LIMIT_CHK = 0x2f,
};


/**
 * @enum nrf_wifi_rf_test
 * @brief Enumerates the available RF test commands for the Wi-Fi RPU.
 *
 * This enumeration defines the various RF test commands that can be issued to the
 * Radio Processing Unit (RPU) for testing, calibration, and debugging purposes.
 * The commands cover a range of RF test operations such as ADC capture, packet capture,
 * tone generation, DPD, RSSI measurement, sleep, temperature reading, XO calibration,
 * register/memory access, and compensation/calibration routines.
 */
enum nrf_wifi_rf_test {
	NRF_WIFI_RF_TEST_RX_ADC_CAP,
	NRF_WIFI_RF_TEST_RX_STAT_PKT_CAP,
	NRF_WIFI_RF_TEST_RX_DYN_PKT_CAP,
	NRF_WIFI_RF_TEST_TX_TONE,
	NRF_WIFI_RF_TEST_DPD,
	NRF_WIFI_RF_TEST_RF_RSSI,
	NRF_WIFI_RF_TEST_SLEEP,
	NRF_WIFI_RF_TEST_GET_TEMPERATURE,
	NRF_WIFI_RF_TEST_XO_CALIB,
	NRF_WIFI_RF_TEST_XO_TUNE,
	NRF_WIFI_RF_TEST_GET_BAT_VOLT,
	NRF_WIFI_SET_TEMP_VOLT_RECAL_PARAMS,
	NRF_WIFI_SET_CALIB_CTRL_PARAMS,
	NRF_WIFI_SET_TX_POWER_CEILINGS,
	NRF_WIFI_SET_TX_POWER_OFFSETS,
	NRF_WIFI_SET_PHY_PARAMS,
	NRF_WIFI_SET_TEMP_VOLT_BUF_ADDR,
	NRF_WIFI_SET_RX_GAINS_OFFSETS,
	NRF_WIFI_SET_EDGE_CHANNEL_PARAMS,
	NRF_WIFI_SET_EVM_AFFECTED_CHANNELS,
	NRF_WIFI_SET_ANTENNA_GAIN_PARAMS,
	NRF_WIFI_RF_TEST_GET_STATS = 64,
	NRF_WIFI_RF_TEST_SET_REGS,
	NRF_WIFI_RF_TEST_READ_REGS,
	NRF_WIFI_RF_TEST_SET_MEM,
	NRF_WIFI_RF_TEST_READ_MEM,

	NRF_WIFI_RF_TEST_MAX,
};

enum nrf_wifi_rf_test_event {
	NRF_WIFI_RF_TEST_EVENT_RX_ADC_CAP,
	NRF_WIFI_RF_TEST_EVENT_RX_STAT_PKT_CAP,
	NRF_WIFI_RF_TEST_EVENT_RX_DYN_PKT_CAP,
	NRF_WIFI_RF_TEST_EVENT_TX_TONE_START,
	NRF_WIFI_RF_TEST_EVENT_DPD_ENABLE,
	NRF_WIFI_RF_TEST_EVENT_RF_RSSI,
	NRF_WIFI_RF_TEST_EVENT_SLEEP,
	NRF_WIFI_RF_TEST_EVENT_TEMP_MEAS,
	NRF_WIFI_RF_TEST_EVENT_XO_CALIB,
	NRF_WIFI_RF_TEST_EVENT_XO_TUNE,
	NRF_WIFI_RF_TEST_EVENT_GET_BAT_VOLT,
	NRF_WIFI_EVENT_SET_TEMP_VOLT_RECAL_PARAMS,
	NRF_WIFI_EVENT_SET_CALIB_CTRL_PARAMS,
	NRF_WIFI_EVENT_SET_TX_POWER_CEILINGS,
	NRF_WIFI_EVENT_SET_TX_POWER_OFFSETS,
	NRF_WIFI_EVENT_SET_PHY_PARAMS,
	NRF_WIFI_EVENT_SET_TEMP_VOLT_BUF_ADDR,
	NRF_WIFI_EVENT_SET_RX_GAINS_OFFSETS,
	NRF_WIFI_EVENT_SET_EDGE_CHANNEL_PARAMS,
	NRF_WIFI_EVENT_SET_EVM_AFFECTED_CHANNELS,
	NRF_WIFI_EVENT_SET_ANTENNA_GAIN_PARAMS,
	NRF_WIFI_RF_TEST_EVENT_GET_STATS = 64,
	NRF_WIFI_RF_TEST_EVENT_SET_REGS,
	NRF_WIFI_RF_TEST_EVENT_READ_REGS,
	NRF_WIFI_RF_TEST_EVENT_SET_MEM,
	NRF_WIFI_RF_TEST_EVENT_READ_MEM,
	NRF_WIFI_RF_TEST_EVENT_MAX,
};

#define MAX_REGS_CONF 8
#define MAX_MEM_CONF 8
#define CAL_MEM_SIZE 500

/* Holds the RX capture related info */
struct nrf_wifi_rf_test_capture_params {
	unsigned char test;

	/* Number of samples to be captured. */
	unsigned short int cap_len;

	/* Capture timeout in seconds. */
	unsigned short int cap_time;

	/* Capture status codes:
	 *0: Capture successful after WLAN packet detection
	 *1: Capture failed after WLAN packet detection
	 *2: Capture timedout as no WLAN packets are detected
	 */
	unsigned char capture_status;

	/* LNA Gain to be configured. It is a 3 bit value. The mapping is,
	 * '0' = 24dB
	 * '1' = 18dB
	 * '2' = 12dB
	 * '3' = 0dB
	 * '4' = -12dB
	 */
	unsigned char lna_gain;

	/* Baseband Gain to be configured. It is a 5 bit value.
	 * It supports 64dB range.The increment happens lineraly 2dB/step
	 */
	unsigned char bb_gain;

	/* address of the capture data */
	unsigned int *capture_addr;
} __NRF_WIFI_PKD;

/* Struct to hold the events from RF test SW. */
struct nrf_wifi_rf_test_capture_meas {
	unsigned char test;

	/* Mean of I samples. Format: Q.11 */
	signed short mean_I;

	/* Mean of Q samples. Format: Q.11 */
	signed short mean_Q;

	/* RMS of I samples */
	unsigned int rms_I;

	/* RMS of Q samples */
	unsigned int rms_Q;
} __NRF_WIFI_PKD;

/* Holds the transmit related info */
struct nrf_wifi_rf_test_tx_params {
	unsigned char test;

	/* Desired tone frequency in MHz in steps of 1 MHz from -10 MHz to +10 MHz. */
	signed char tone_freq;

	/* Desired TX power in the range -16 dBm to +24 dBm.
	 * in steps of 2 dBm
	 */
	signed char tx_pow;

	/* Set 1 for staring tone transmission. */
	unsigned char enabled;
} __NRF_WIFI_PKD;

struct nrf_wifi_rf_test_transmit_samples {
	unsigned char test;
	unsigned char enabled;
} __NRF_WIFI_PKD;


struct nrf_wifi_rf_test_dpd_params {
	unsigned char test;
	unsigned char enabled;

} __NRF_WIFI_PKD;

struct nrf_wifi_temperature_params {
	unsigned char test;

	/** Current measured temperature */
	signed int temperature;

	/** Temperature measurment status.
	 * 0: Reading successful
	 * 1: Reading failed
	 */
	unsigned int readTemperatureStatus;
} __NRF_WIFI_PKD;

struct nrf_wifi_rf_get_rf_rssi {
	unsigned char test;
	unsigned char lna_gain;
	unsigned char bb_gain;
	unsigned char agc_status_val;
} __NRF_WIFI_PKD;

struct nrf_wifi_rf_test_xo_calib {
	unsigned char test;

	/* XO value in the range between 0 to 127 */
	unsigned char xo_val;

} __NRF_WIFI_PKD;

struct nrf_wifi_rf_get_xo_value {
	unsigned char test;

	/* Optimal XO value computed. */
	unsigned char xo_value;
} __NRF_WIFI_PKD;


struct nrf_wifi_rf_test_enter_sleep {
	unsigned char test;
	unsigned char enabled;

} __NRF_WIFI_PKD;

/* Structure to hold battery voltage parameters */
struct nrf_wifi_bat_volt_params {
	unsigned char test;

	/** Measured battery voltage in volts. */
	unsigned char voltage;

	/** Status of the voltage measurement command.
	 * 0: Reading successful
	 * 1: Reading failed
	 */
	unsigned int cmd_status;
} __NRF_WIFI_PKD;

/**
 * @struct nrf_wifi_rf_get_rx_debug_stats
 * @brief Holds debug statistics related to the RX.
 */
struct nrf_wifi_rf_get_rx_debug_stats {
	unsigned char test;

	/**
	 * @todo Refactor the code to use the WLAN_PHY_RX_STATS_T structure.
	 *
	 * The current implementation uses individual structure members that are
	 * already present in WLAN_PHY_RX_STATS_T. To improve maintainability
	 * and reduce redundancy, consider replacing the individual members with
	 * this unified structure.
	 */

	unsigned int edCnt;
	unsigned int ofdmCrc32PassCnt;
	unsigned int ofdmCrc32FailCnt;
	unsigned int dsssCrc32PassCnt;
	unsigned int dsssCrc32FailCnt;
	unsigned int ofdmCorrPassCnt;
	unsigned int dsssCorrPassCnt;
	unsigned int ofdmLtfCorrFailCnt;
	unsigned int ofdmLsigFailCnt;
	unsigned int ofdmHtsigAFailCnt;
	unsigned int ofdmVhtsigAFailCnt;
	unsigned int ofdmVhtsigBFailCnt;
	unsigned int ofdmHesigAFailCnt;
	unsigned int ofdmHesigBFailCnt;
	unsigned int ofdmUsigFailCnt;
	unsigned int ofdmEhtsigFailCnt;
	unsigned int ofdmzeroLenMpduCnt;
	unsigned int ofdminvalidDelimiterCnt;
	unsigned int dsssSyncFailCnt;
	unsigned int dsssFsyncFailCnt;
	unsigned int dsssSfdFailCnt;
	unsigned int dsssHdrFailCnt;
	unsigned int lgPktCnt;
	unsigned int htPktCnt;
	unsigned int vhtPktCnt;
	unsigned int heSuPktCnt;
	unsigned int heMuPktCnt;
	unsigned int heErSuPktCnt;
	unsigned int heTbPktCnt;
	unsigned int ehtMuPktCnt;
	unsigned int popCnt;
	unsigned int midPacketCnt;
	unsigned int lowEnergyEventCnt;
	unsigned int unSupportedCnt;
	unsigned int otherStaPktCnt;
	unsigned int vhtNdpCnt;
	unsigned int hesuNdpCnt;
	unsigned int ehtNdpCnt;
	unsigned int ofdmS2lTimeOutFailCnt;
	unsigned int spatialReuseCnt;
} __NRF_WIFI_PKD;

/**
 * Enum for selecting operating mode of RF
 * rx_only_mode: Only RX is supported in this mode.
 * trx_normal_mode: Both RX and TX are supported by RF.
 */
typedef enum {
	rx_only_mode,
	trx_normal_mode
} sys_oper_mode_e;

/**
 * @struct nrf_wifi_rf_config_regs
 * @brief Holds configuration details for RPU registers.
 */
struct nrf_wifi_rf_config_regs {
	/** Test identifier */
	unsigned char test;

	/** Number of registers to be configured */
	unsigned char num_regs;

	/** Array to store register values */
	unsigned int reg_val[MAX_REGS_CONF];

	/** Array to store register addresses */
	unsigned int reg_addr[MAX_REGS_CONF];

} __NRF_WIFI_PKD;

/**
 * @struct nrf_wifi_rf_config_mem
 * @brief Holds memory configuration details for RPU.
 */
struct nrf_wifi_rf_config_mem {
	/** Test identifier */
	unsigned char test;

	/** Number of memory locations to be configured */
	unsigned char num_memory_loc;

	/** Array to store memory values */
	unsigned int mem_val[MAX_MEM_CONF];

	/** Array to store memory addresses */
	unsigned int mem_addr[MAX_MEM_CONF];

} __NRF_WIFI_PKD;

/**
 * @struct nrf_wifi_rf_calib
 * @brief Holds calibration configuration details for RPU.
 */
struct nrf_wifi_rf_calib {
	/** Test identifier */
	unsigned char test;

	/** RF calibration bit map */
	unsigned char calib_bitmap;

	sys_oper_mode_e sys_operating_mode;

	/** Result structure to store calibration results */
	unsigned char rf_calib_results[CAL_MEM_SIZE];
} __NRF_WIFI_PKD;

/**
 * @struct nrf_wifi_rf_read_calib_results
 * @brief Read calibration results structure for the current channel.
 */
struct nrf_wifi_rf_read_calib_results {
	/** Test identifier */
	unsigned char test;

	unsigned char rf_calib_results[CAL_MEM_SIZE];
} __NRF_WIFI_PKD;


/* TODO: Below OTP + PCB loss won't work for nRF71, but added
 * here to avoid code churn. Need to revisit this.
 */
#define NUM_PCB_LOSS_OFFSET 4

/** The byte offsets of RF parameters indicate the start offset
 * of PCB loss for 2.4 GHz and 5 GHz bands.
 */
enum PCB_LOSS_BYTE_OFFSETS {
	PCB_LOSS_BYTE_2G_OFST = 185,
	PCB_LOSS_BYTE_5G_BAND1_OFST,
	PCB_LOSS_BYTE_5G_BAND2_OFST,
	PCB_LOSS_BYTE_5G_BAND3_OFST
};

/** The byte offsets of RF parameters indicate the start offset
 * of band edge backoffs for different frame formats and
 * different sub-bands of 2.4 GHz and 5 GHz frequency band.
 */
enum EDGE_BACKOFF_OFFSETS {
	BAND_2G_LW_ED_BKF_DSSS_OFST = 155,
	BAND_2G_LW_ED_BKF_HT_OFST,
	BAND_2G_LW_ED_BKF_HE_OFST,
	BAND_2G_UW_ED_BKF_DSSS_OFST,
	BAND_2G_UW_ED_BKF_HT_OFST,
	BAND_2G_UW_ED_BKF_HE_OFST,
	BAND_UNII_1_LW_ED_BKF_HT_OFST,
	BAND_UNII_1_LW_ED_BKF_HE_OFST,
	BAND_UNII_1_UW_ED_BKF_HT_OFST,
	BAND_UNII_1_UW_ED_BKF_HE_OFST,
	BAND_UNII_2A_LW_ED_BKF_HT_OFST,
	BAND_UNII_2A_LW_ED_BKF_HE_OFST,
	BAND_UNII_2A_UW_ED_BKF_HT_OFST,
	BAND_UNII_2A_UW_ED_BKF_HE_OFST,
	BAND_UNII_2C_LW_ED_BKF_HT_OFST,
	BAND_UNII_2C_LW_ED_BKF_HE_OFST,
	BAND_UNII_2C_UW_ED_BKF_HT_OFST,
	BAND_UNII_2C_UW_ED_BKF_HE_OFST,
	BAND_UNII_3_LW_ED_BKF_HT_OFST,
	BAND_UNII_3_LW_ED_BKF_HE_OFST,
	BAND_UNII_3_UW_ED_BKF_HT_OFST,
	BAND_UNII_3_UW_ED_BKF_HE_OFST,
	BAND_UNII_4_LW_ED_BKF_HT_OFST,
	BAND_UNII_4_LW_ED_BKF_HE_OFST,
	BAND_UNII_4_UW_ED_BKF_HT_OFST,
	BAND_UNII_4_UW_ED_BKF_HE_OFST,
	NUM_EDGE_BACKOFF = 26
};


/* Size of XO calibration value stored in the OTP field CALIB_XO */
#define OTP_SZ_CALIB_XO 1

/* Byte offsets of XO calib value in CALIB_XO field in the OTP */
#define OTP_OFF_CALIB_XO 0

/* Masks to program bit fields in REGION_DEFAULTS field in the OTP */
#define QSPI_KEY_FLAG_MASK ~(1U<<0)
#define MAC0_ADDR_FLAG_MASK ~(1U<<1)
#define MAC1_ADDR_FLAG_MASK ~(1U<<2)
#define CALIB_XO_FLAG_MASK ~(1U<<3)

/* RF register address to facilitate OTP access */
#define OTP_VOLTCTRL_ADDR 0x19004
/* Voltage value to be written into the above RF register for OTP write access */
#define OTP_VOLTCTRL_2V5 0x3b
/* Voltage value to be written into the above RF register for OTP read access */
#define OTP_VOLTCTRL_1V8 0xb

#define OTP_POLL_ADDR 0x01B804
#define OTP_WR_DONE 0x1
#define OTP_READ_VALID 0x2
#define OTP_READY 0x4


#define OTP_RWSBMODE_ADDR 0x01B800
#define OTP_READ_MODE 0x1
#define OTP_BYTE_WRITE_MODE 0x42


#define OTP_RDENABLE_ADDR 0x01B810
#define OTP_READREG_ADDR 0x01B814

#define OTP_WRENABLE_ADDR 0x01B808
#define OTP_WRITEREG_ADDR 0x01B80C

#define OTP_TIMING_REG1_ADDR 0x01B820
#define OTP_TIMING_REG1_VAL 0x0
#define OTP_TIMING_REG2_ADDR 0x01B824
#define OTP_TIMING_REG2_VAL 0x030D8B

#define OTP_FRESH_FROM_FAB 0xFFFFFFFF
#define OTP_PROGRAMMED 0x00000000
#define OTP_ENABLE_PATTERN 0x50FA50FA
#define OTP_INVALID 0xDEADBEEF

#define FT_PROG_VER_MASK 0xF0000

/**
 * @struct nrf_wifi_board_params
 * @brief This structure defines board dependent parameters like PCB loss.
 *
 */
struct nrf_wifi_board_params {
	/** PCB loss for 2.4 GHz band */
	unsigned char pcb_loss_2g;
	/** PCB loss for 5 GHz band (5150 MHz - 5350 MHz) */
	unsigned char pcb_loss_5g_band1;
	/** PCB loss for 5 GHz band (5470 MHz - 5730 MHz) */
	unsigned char pcb_loss_5g_band2;
	/** PCB loss for 5 GHz band (5730 MHz - 5895 MHz) */
	unsigned char pcb_loss_5g_band3;
} __NRF_WIFI_PKD;

/**
 * @struct host_rpu_umac_info
 * @brief This structure represents the information related to UMAC.
 *
 */
struct host_rpu_umac_info {
	/** Boot status signature */
	unsigned int boot_status;
	/** UMAC version */
	unsigned int version;
	/** OTP params */
	unsigned int info_part;
	/** OTP params */
	unsigned int info_variant;
	/** OTP params */
	unsigned int info_lromversion;
	/** OTP params */
	unsigned int info_uromversion;
	/** OTP params */
	unsigned int info_uuid[4];
	/** OTP params */
	unsigned int info_spare0;
	/** OTP params */
	unsigned int info_spare1;
	/** OTP params */
	unsigned int mac_address0[2];
	/** OTP params */
	unsigned int mac_address1[2];
	/** OTP params */
	unsigned int calib[9];
} __NRF_WIFI_PKD;

/**
 * @brief Structure to hold OTP region information.
 *
 */
struct nrf_wifi_fmac_otp_info {
	/** OTP region information. */
	struct host_rpu_umac_info info;
	/** Flags indicating which OTP regions are valid. */
	unsigned int flags;
};


/** XO adjustment value */
struct nrf_wifi_xo_freq_offset {
	unsigned char xo_freq_offset;
} __NRF_WIFI_PKD;

/** Power detector adjustment factor for MCS7 */
struct nrf_wifi_pd_adst_val {
	/** PD adjustment value corresponding to Channel 7 */
	signed char pd_adjt_lb_chan;
	/** PD adjustment value corresponding to Channel 36 */
	signed char pd_adjt_hb_low_chan;
	/** PD adjustment value corresponding to Channel 100 */
	signed char pd_adjt_hb_mid_chan;
	/** PD adjustment value corresponding to Channel 165 */
	signed char pd_adjt_hb_high_chan;
} __NRF_WIFI_PKD;

/** TX power systematic offset is the difference between set power
 *  and the measured power
 */
struct nrf_wifi_tx_pwr_systm_offset {
	/** Systematic adjustment value corresponding to Channel 7 */
	signed char syst_off_lb_chan;
	/** Systematic adjustment value corresponding to Channel 36 */
	signed char syst_off_hb_low_chan;
	/** Systematic adjustment value corresponding to Channel 100 */
	signed char syst_off_hb_mid_chan;
	/** Systematic adjustment value corresponding to Channel 165 */
	signed char syst_off_hb_high_chan;
} __NRF_WIFI_PKD;

/** Max TX power value for which both EVM and SEM pass */
struct nrf_wifi_tx_pwr_ceil {
	/** Max output power for 11b for channel 7 */
	signed char max_dsss_pwr;
	/** Max output power for MCS7 for channel 7 */
	signed char max_lb_mcs7_pwr;
	/** Max output power for MCS0 for channel 7 */
	signed char max_lb_mcs0_pwr;
	/** Max output power for MCS7 for channel 36 */
	signed char max_hb_low_chan_mcs7_pwr;
	/** Max output power for MCS7 for channel 100 */
	signed char max_hb_mid_chan_mcs7_pwr;
	/** Max output power for MCS7 for channel 165 */
	signed char max_hb_high_chan_mcs7_pwr;
	/** Max output power for MCS0 for channel 36 */
	signed char max_hb_low_chan_mcs0_pwr;
	/** Max output power for MCS0 for channel 100 */
	signed char max_hb_mid_chan_mcs0_pwr;
	/** Max output power for MCS0 for channel 165 */
	signed char max_hb_high_chan_mcs0_pwr;
} __NRF_WIFI_PKD;

/** RX gain adjustment offsets */
struct nrf_wifi_rx_gain_offset {
	/** Channel 7 */
	signed char rx_gain_lb_chan;
	/** Channel 36 */
	signed char rx_gain_hb_low_chan;
	/** Channel 100 */
	signed char rx_gain_hb_mid_chan;
	/** Channel 165 */
	signed char rx_gain_hb_high_chan;
} __NRF_WIFI_PKD;

/** Voltage and temperature dependent backoffs */
struct nrf_wifi_temp_volt_depend_params {
	/** Maximum chip temperature in centigrade */
	signed char max_chip_temp;
	/** Minimum chip temperature in centigrade */
	signed char min_chip_temp;
	/** TX power backoff at high temperature in 2.4GHz */
	signed char lb_max_pwr_bkf_hi_temp;
	/** TX power backoff at low temperature in 2.4GHz */
	signed char lb_max_pwr_bkf_low_temp;
	/** TX power backoff at high temperature in 5GHz */
	signed char hb_max_pwr_bkf_hi_temp;
	/** TX power backoff at low temperature in 5GHz */
	signed char hb_max_pwr_bkf_low_temp;
	/** Voltage back off value in LowBand when VBAT< VBAT_VERYLOW */
	signed char lb_vbt_lt_vlow;
	/** Voltage back off value in HighBand when VBAT< VBAT_VERYLOW */
	signed char hb_vbt_lt_vlow;
	/** Voltage back off value in LowBand when VBAT< VBAT_LOW */
	signed char lb_vbt_lt_low;
	/** Voltage back off value in HighBand when VBAT< VBAT_LOW */
	signed char hb_vbt_lt_low;
	/** Reserved bytes */
	signed char reserved[4];
} __NRF_WIFI_PKD;

/** The top-level structure holds substructures,
 * each containing information related to the
 * first 42 bytes of RF parameters.
 */
struct nrf_wifi_phy_rf_params {
	unsigned char reserved[6];
	struct nrf_wifi_xo_freq_offset xo_offset;
	struct nrf_wifi_pd_adst_val pd_adjust_val;
	struct nrf_wifi_tx_pwr_systm_offset syst_tx_pwr_offset;
	struct nrf_wifi_tx_pwr_ceil max_pwr_ceil;
	struct nrf_wifi_rx_gain_offset rx_gain_offset;
	struct nrf_wifi_temp_volt_depend_params temp_volt_backoff;
	unsigned char phy_params[NRF_WIFI_RF_PARAMS_SIZE - NRF_WIFI_RF_PARAMS_CONF_SIZE];
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the parameters used to control the max transmit (TX) power
 * in both frequency bands for different data rates.
 */
struct nrf_wifi_tx_pwr_ceil_params {
	/** Maximum power permitted while transmitting DSSS rates in 2.4G band.
	 *  Resolution is 0.25dBm.
	 */
	unsigned char max_pwr_2g_dsss;
	/** Maximum power permitted while transmitting MCS0 rate in 2.4G band.
	 *  Resolution is 0.25dBm.
	 */
	unsigned char max_pwr_2g_mcs0;
	/** Maximum power permitted while transmitting MCS7 rate in 2.4G band.
	 *  Resolution is 0.25dBm.
	 */
	unsigned char max_pwr_2g_mcs7;

	/** Maximum power permitted while transmitting MCS0 rate in 5G lowband.
	 * Low band corresponds to ch: 36 to 64 Resolution is 0.25dBm.
	 */
	unsigned char max_pwr_5g_low_mcs0;
	/** Maximum power permitted while transmitting MCS7 rate in 5G lowband.
	 * Low band corresponds to ch: 36 to 64, resolution is 0.25dBm.
	 */
	unsigned char max_pwr_5g_low_mcs7;
	/** Maximum power permitted while transmitting MCS0 rate in 5G midband.
	 * Mid band corresponds to ch: 96 to 132, resolution is 0.25dBm.
	 */
	unsigned char max_pwr_5g_mid_mcs0;
	/** Maximum power permitted while transmitting MCS7 rate in 5G midband.
	 * Mid band corresponds to ch: 96 to 132, resolution is 0.25dBm.
	 */
	unsigned char max_pwr_5g_mid_mcs7;
	/** Maximum power permitted while transmitting MCS0 rate in 5G highband.
	 * High band corresponds to ch: 136 to 177, resolution is 0.25dBm.
	 */
	unsigned char max_pwr_5g_high_mcs0;
	/** Maximum power permitted while transmitting MCS7 rate in 5G highband.
	 * High band corresponds to ch: 136 to 177, resolution is 0.25dBm.
	 */
	unsigned char max_pwr_5g_high_mcs7;
} __NRF_WIFI_PKD;

struct nrf_wifi_cmd_coding_type_config {
	/** umac header, see &nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** 0 = BCC 1= LDPC */
	unsigned char coding_type;
} __NRF_WIFI_PKD;

struct nrf_wifi_cmd_xo_tune {
	/** umac header, see &nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** xo value */
	int xo_val;
} __NRF_WIFI_PKD;

/**
 * Below NRF_WIFI_SQI_* Macros are used for SQI feature
 */
#define NRF_WIFI_SQI_RSSI_MIN -90
#define NRF_WIFI_SQI_RSSI_MAX -60
#define NRF_WIFI_SQI_RSSI_HYST_MIN 4
#define NRF_WIFI_SQI_RSSI_HYST_MAX 6
#define NRF_WIFI_SQI_RSSI_HYST_DEF 4

/**
 * @enum nrf_wifi_sqi_signal_source
 * @brief Enumerates the available signal sources for SQI measurement.
 *
 * This enumeration defines the various signal sources that can be used for
 * Signal Quality Indicator (SQI) measurement in the Wi-Fi RPU.
 *
 */
enum nrf_wifi_sqi {
	/** Invalid SQI source */
	NRF_WIFI_SQI_RX_INVALID = 0,
	/** Beacon frames */
	NRF_WIFI_SQI_RX_BCN = (1 << 0),
	/** Unicast and broadcast frames */
	NRF_WIFI_SQI_RX_UCAST_BCAST = (1 << 1),
	/** BSS traffic unicast frames */
	NRF_WIFI_SQI_RX_BSS_TP_UCAST = (1 << 2),
	/* @NRF_WIFI_SQI_RX_ALL sets all the above flags */
	NRF_WIFI_SQI_RX_ALL = (1 << 3) - 1,
} __NRF_WIFI_PKD;

/**
 * @enum nrf_wifi_sqi_sample_count
 * @brief Enumerates the available sample counts for SQI measurement.
 *
 * This enumeration defines the various sample counts that can be used for
 * Signal Quality Indicator (SQI) measurement in the Wi-Fi RPU.
 */
enum nrf_wifi_sqi_sample_count {
	/** 4 samples */
	NRF_WIFI_SQI_SAMPLE_COUNT_4 = 4,
	/** 8 samples */
	NRF_WIFI_SQI_SAMPLE_COUNT_8 = 8,
	/** 16 samples */
	NRF_WIFI_SQI_SAMPLE_COUNT_16 = 16,
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the parameters used to configure
 * Signal Quality Indicator (SQI) measurement.
 */
struct nrf_wifi_cmd_sqi {
	/** umac header, see &nrf_wifi_sys_head */
	struct nrf_wifi_sys_head sys_head;
	/** Signal source for SQI measurement */
	unsigned int nrf_wifi_signal_src;
	/** Number of average samples count */
	unsigned int nrf_wifi_avg_cnt;
} __NRF_WIFI_PKD;

/**
 * @}
 */
#endif /* __NRF71_WIFI_COMMON_H__ */
