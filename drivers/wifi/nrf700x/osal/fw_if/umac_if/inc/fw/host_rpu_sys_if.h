/*
 *
 *Copyright (c) 2022 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 *
 *@brief <System interface between host and RPU>
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

/*#define NRF_WIFI_PHY_CALIB_FLAG_RXDC 1*/
/*#define NRF_WIFI_PHY_CALIB_FLAG_TXDC 2*/
/*#define NRF_WIFI_PHY_CALIB_FLAG_TXPOW 4*/
/*#define NRF_WIFI_PHY_CALIB_FLAG_TXIQ 8*/
/*#define NRF_WIFI_PHY_CALIB_FLAG_RXIQ 16*/
/*#define NRF_WIFI_PHY_CALIB_FLAG_DPD 32*/

/**
 * enum nrf_wifi_sys_iftype - Interface types based on functionality.
 *
 * @NRF_WIFI_UMAC_IFTYPE_UNSPECIFIED: Unspecified type, driver decides.
 * @NRF_WIFI_UMAC_IFTYPE_ADHOC: Independent BSS member.
 * @NRF_WIFI_UMAC_IFTYPE_STATION: Managed BSS member.
 * @NRF_WIFI_UMAC_IFTYPE_AP: Access point.
 * @NRF_WIFI_UMAC_IFTYPE_AP_VLAN: VLAN interface for access points; VLAN interfaces
 *	are a bit special in that they must always be tied to a pre-existing
 *	AP type interface.
 * @NRF_WIFI_UMAC_IFTYPE_WDS: Wireless Distribution System.
 * @NRF_WIFI_UMAC_IFTYPE_MONITOR: Monitor interface receiving all frames.
 * @NRF_WIFI_UMAC_IFTYPE_MESH_POINT: Mesh point.
 * @NRF_WIFI_UMAC_IFTYPE_P2P_CLIENT: P2P client.
 * @NRF_WIFI_UMAC_IFTYPE_P2P_GO: P2P group owner.
 * @NRF_WIFI_UMAC_IFTYPE_P2P_DEVICE: P2P device interface type, this is not a netdev
 *	and therefore can't be created in the normal ways, use the
 *	%NRF_WIFI_UMAC_CMD_START_P2P_DEVICE and %NRF_WIFI_UMAC_CMD_STOP_P2P_DEVICE
 *	commands (Refer &enum nrf_wifi_umac_commands) to create and destroy one.
 * @NRF_WIFI_UMAC_IFTYPE_OCB: Outside Context of a BSS.
 *	This mode corresponds to the MIB variable dot11OCBActivated=true.
 * @NRF_WIFI_UMAC_IFTYPE_MAX: Highest interface type number currently defined.
 * @NRF_WIFI_UMAC_IFTYPES: Number of defined interface types.
 *
 * Lists the different interface types based on how they are configured
 * functionally.
 */
enum nrf_wifi_sys_iftype {
	NRF_WIFI_UMAC_IFTYPE_UNSPECIFIED,
	NRF_WIFI_UMAC_IFTYPE_ADHOC,
	NRF_WIFI_UMAC_IFTYPE_STATION,
	NRF_WIFI_UMAC_IFTYPE_AP,
	NRF_WIFI_UMAC_IFTYPE_AP_VLAN,
	NRF_WIFI_UMAC_IFTYPE_WDS,
	NRF_WIFI_UMAC_IFTYPE_MONITOR,
	NRF_WIFI_UMAC_IFTYPE_MESH_POINT,
	NRF_WIFI_UMAC_IFTYPE_P2P_CLIENT,
	NRF_WIFI_UMAC_IFTYPE_P2P_GO,
	NRF_WIFI_UMAC_IFTYPE_P2P_DEVICE,
	NRF_WIFI_UMAC_IFTYPE_OCB,

	/* keep last */
	NRF_WIFI_UMAC_IFTYPES,
	NRF_WIFI_UMAC_IFTYPE_MAX = NRF_WIFI_UMAC_IFTYPES - 1
};

/**
 * enum rpu_op_mode - operating modes.
 *
 * @RPU_OP_MODE_NORMAL: Normal mode is the regular mode of operation
 * @RPU_OP_MODE_DBG: Debug mode can be used to control certain parameters
 *	like TX rate etc in order to debug functional issues
 * @RPU_OP_MODE_PROD: Production mode is used for performing production
 *	tests using continuous Tx/Rx on a configured channel at a particular
 *	rate, power etc
 * @RPU_OP_MODE_FCM: In the FCM mode different type of calibration like RF
 *	calibration can be performed
 *
 * Lists the different types of operating modes.
 */
enum rpu_op_mode {
	RPU_OP_MODE_RADIO_TEST,
	RPU_OP_MODE_FCM,
	RPU_OP_MODE_REG,
	RPU_OP_MODE_DBG,
	RPU_OP_MODE_MAX
};

/**
 * enum rpu_stats_type - statistics type.
 *
 * To obtain statistics relevant to the operation mode set via op_mode
 * parameter.
 */

enum rpu_stats_type {
	RPU_STATS_TYPE_ALL,
	RPU_STATS_TYPE_HOST,
	RPU_STATS_TYPE_UMAC,
	RPU_STATS_TYPE_LMAC,
	RPU_STATS_TYPE_PHY,
	RPU_STATS_TYPE_MAX
};

/**
 * enum rpu_tput_mode - Throughput mode
 *
 * Throughput mode to be used for transmitting the packet.
 */
enum rpu_tput_mode {
	RPU_TPUT_MODE_LEGACY,
	RPU_TPUT_MODE_HT,
	RPU_TPUT_MODE_VHT,
	RPU_TPUT_MODE_HE_SU,
	RPU_TPUT_MODE_HE_ER_SU,
	RPU_TPUT_MODE_HE_TB,
	RPU_TPUT_MODE_MAX
};

/**
 * enum nrf_wifi_sys_commands - system commands
 * @NRF_WIFI_CMD_INIT: After host driver bringup host sends the NRF_WIFI_CMD_INIT
 *	to the RPU. then RPU initializes and responds with
 *	NRF_WIFI_EVENT_BUFF_CONFIG.
 * @NRF_WIFI_CMD_BUFF_CONFIG_COMPLETE: Host sends this command to RPU after
 *	completion of all buffers configuration
 * @NRF_WIFI_CMD_TX: command to send a Tx packet
 * @NRF_WIFI_CMD_MODE: command to specify mode of operation
 * @NRF_WIFI_CMD_GET_STATS: command to get statistics
 * @NRF_WIFI_CMD_CLEAR_STATS: command to clear statistics
 * @NRF_WIFI_CMD_RX: command to ENABLE/DISABLE receiving packets in radio test mode
 * @NRF_WIFI_CMD_DEINIT: RPU De-initialization
 * @NRF_WIFI_CMD_HE_GI_LTF_CONFIG: Configure HE_GI & HE_LTF.
 *
 */
enum nrf_wifi_sys_commands {
	NRF_WIFI_CMD_INIT,
	NRF_WIFI_CMD_TX,
	NRF_WIFI_CMD_IF_TYPE,
	NRF_WIFI_CMD_MODE,
	NRF_WIFI_CMD_GET_STATS,
	NRF_WIFI_CMD_CLEAR_STATS,
	NRF_WIFI_CMD_RX,
	NRF_WIFI_CMD_PWR,
	NRF_WIFI_CMD_DEINIT,
	NRF_WIFI_CMD_BTCOEX,
	NRF_WIFI_CMD_RF_TEST,
	NRF_WIFI_CMD_HE_GI_LTF_CONFIG,
	NRF_WIFI_CMD_UMAC_INT_STATS,
	NRF_WIFI_CMD_RADIO_TEST_INIT,
	NRF_WIFI_CMD_RT_REQ_SET_REG,
	NRF_WIFI_CMD_TX_FIX_DATA_RATE,
};

/**
 * enum nrf_wifi_sys_events -
 * @NRF_WIFI_EVENT_BUFF_CONFIG: Response to NRF_WIFI_CMD_INIT
 *	see &struct nrf_wifi_event_buffs_config
 * @NRF_WIFI_EVENT_BUFF_CONFIG_DONE: Response to NRF_WIFI_CMD_BUFF_CONFIG_COMPLETE
 * @NRF_WIFI_EVENT_STATS: Response to NRF_WIFI_CMD_GET_STATS
 * @NRF_WIFI_EVENT_DEINIT_DONE: Response to NRF_WIFI_CMD_DEINIT
 *
 * Events from the RPU for different commands.
 */
enum nrf_wifi_sys_events {
	NRF_WIFI_EVENT_PWR_DATA,

	NRF_WIFI_EVENT_INIT_DONE,
	NRF_WIFI_EVENT_STATS,
	NRF_WIFI_EVENT_DEINIT_DONE,
	NRF_WIFI_EVENT_RF_TEST,
	NRF_WIFI_EVENT_COEX_CONFIG,
	NRF_WIFI_EVENT_INT_UMAC_STATS,
	NRF_WIFI_EVENT_RADIOCMD_STATUS
};

enum rpu_ch_bw {
	RPU_CH_BW_20,
	RPU_CH_BW_40,
	RPU_CH_BW_MAX
};

struct chan_params {
	unsigned int primary_num;
	unsigned char bw;
	signed int sec_20_offset;
	signed int sec_40_offset;
} __NRF_WIFI_PKD;

struct rpu_conf_rx_radio_test_params {
	unsigned char nss;
	/* Input to the RF for operation */
	unsigned char rf_params[NRF_WIFI_RF_PARAMS_SIZE];
	struct chan_params chan;
	signed char phy_threshold;
	unsigned int phy_calib;
	unsigned char rx;
} __NRF_WIFI_PKD;



struct umac_rx_dbg_params {
	unsigned int lmac_events;
	unsigned int rx_events;
	unsigned int rx_coalised_events;
	unsigned int total_rx_pkts_from_lmac;

	unsigned int max_refill_gap;
	unsigned int current_refill_gap;

	unsigned int out_oforedr_mpdus;
	unsigned int reoredr_free_mpdus;

	unsigned int umac_consumed_pkts;
	unsigned int host_consumed_pkts;

	unsigned int rx_mbox_post;
	unsigned int rx_mbox_receive;

	unsigned int reordering_ampdu;
	unsigned int timer_mbox_post;
	unsigned int timer_mbox_rcv;
	unsigned int work_mbox_post;
	unsigned int work_mbox_rcv;
	unsigned int tasklet_mbox_post;
	unsigned int tasklet_mbox_rcv;
	unsigned int userspace_offload_frames;
	unsigned int alloc_buf_fail;

	unsigned int rx_packet_total_count;
	unsigned int rx_packet_data_count;
	unsigned int rx_packet_qos_data_count;
	unsigned int rx_packet_protected_data_count;
	unsigned int rx_packet_mgmt_count;
	unsigned int rx_packet_beacon_count;
	unsigned int rx_packet_probe_resp_count;
	unsigned int rx_packet_auth_count;
	unsigned int rx_packet_deauth_count;
	unsigned int rx_packet_assoc_resp_count;
	unsigned int rx_packet_disassoc_count;
	unsigned int rx_packet_action_count;
	unsigned int rx_packet_probe_req_count;
	unsigned int rx_packet_other_mgmt_count;
	signed char max_coalised_pkts;
	unsigned int null_skb_pointer_from_lmac;
	unsigned int unexpected_mgmt_pkt;

} __NRF_WIFI_PKD;

struct umac_tx_dbg_params {
	unsigned int tx_cmd;
	unsigned int tx_non_coalescing_pkts_rcvd_from_host;
	unsigned int tx_coalescing_pkts_rcvd_from_host;
	unsigned int tx_max_coalescing_pkts_rcvd_from_host;
	unsigned int tx_cmds_max_used;
	unsigned int tx_cmds_currently_in_use;
	unsigned int tx_done_events_send_to_host;
	unsigned int tx_done_success_pkts_to_host;
	unsigned int tx_done_failure_pkts_to_host;
	unsigned int tx_cmds_with_crypto_pkts_rcvd_from_host;
	unsigned int tx_cmds_with_non_cryptot_pkts_rcvd_from_host;
	unsigned int tx_cmds_with_broadcast_pkts_rcvd_from_host;
	unsigned int tx_cmds_with_multicast_pkts_rcvd_from_host;
	unsigned int tx_cmds_with_unicast_pkts_rcvd_from_host;

	unsigned int xmit;
	unsigned int send_addba_req;
	unsigned int addba_resp;
	unsigned int softmac_tx;
	unsigned int internal_pkts;
	unsigned int external_pkts;
	unsigned int tx_cmds_to_lmac;
	unsigned int tx_dones_from_lmac;
	unsigned int total_cmds_to_lmac;

	unsigned int tx_packet_data_count;
	unsigned int tx_packet_mgmt_count;
	unsigned int tx_packet_beacon_count;
	unsigned int tx_packet_probe_req_count;
	unsigned int tx_packet_auth_count;
	unsigned int tx_packet_deauth_count;
	unsigned int tx_packet_assoc_req_count;
	unsigned int tx_packet_disassoc_count;
	unsigned int tx_packet_action_count;
	unsigned int tx_packet_other_mgmt_count;
	unsigned int tx_packet_non_mgmt_data_count;

} __NRF_WIFI_PKD;
struct umac_cmd_evnt_dbg_params {
	unsigned char cmd_init;
	unsigned char event_init_done;
	unsigned char cmd_rf_test;
	unsigned char cmd_connect;
	unsigned int cmd_get_stats;
	unsigned int event_ps_state;
	unsigned int cmd_set_reg;
	unsigned int cmd_get_reg;
	unsigned int cmd_req_set_reg;
	unsigned int cmd_trigger_scan;
	unsigned int event_scan_done;
	unsigned int cmd_get_scan;
	unsigned int umac_scan_req;
	unsigned int umac_scan_complete;
	unsigned int umac_scan_busy;
	unsigned int cmd_auth;
	unsigned int cmd_assoc;
	unsigned int cmd_deauth;
	unsigned int cmd_register_frame;
	unsigned int cmd_frame;
	unsigned int cmd_del_key;
	unsigned int cmd_new_key;
	unsigned int cmd_set_key;
	unsigned int cmd_get_key;
	unsigned int event_beacon_hint;
	unsigned int event_reg_change;
	unsigned int event_wiphy_reg_change;
	unsigned int cmd_set_station;
	unsigned int cmd_new_station;
	unsigned int cmd_del_station;
	unsigned int cmd_new_interface;
	unsigned int cmd_set_interface;
	unsigned int cmd_get_interface;
	unsigned int cmd_set_ifflags;
	unsigned int cmd_set_ifflags_done;
	unsigned int cmd_set_bss;
	unsigned int cmd_set_wiphy;
	unsigned int cmd_start_ap;
	unsigned int LMAC_CMD_PS;
	unsigned int CURR_STATE;
} __NRF_WIFI_PKD;

#ifndef CONFIG_NRF700X_RADIO_TEST

struct nrf_wifi_interface_stats {
	unsigned int tx_unicast_pkt_count;
	unsigned int tx_multicast_pkt_count;
	unsigned int tx_broadcast_pkt_count;
	unsigned int tx_bytes;
	unsigned int rx_unicast_pkt_count;
	unsigned int rx_multicast_pkt_count;
	unsigned int rx_broadcast_pkt_count;
	unsigned int rx_beacon_success_count;
	unsigned int rx_beacon_miss_count;
	unsigned int rx_bytes;
	unsigned int rx_checksum_error_count;
} __NRF_WIFI_PKD;

struct rpu_umac_stats {
	struct umac_tx_dbg_params tx_dbg_params;
	struct umac_rx_dbg_params rx_dbg_params;
	struct umac_cmd_evnt_dbg_params cmd_evnt_dbg_params;
	struct nrf_wifi_interface_stats interface_data_stats;
} __NRF_WIFI_PKD;

struct rpu_lmac_stats {
	/*LMAC DEBUG Stats*/
	unsigned int reset_cmd_cnt;
	unsigned int reset_complete_event_cnt;
	unsigned int unable_gen_event;
	unsigned int ch_prog_cmd_cnt;
	unsigned int channel_prog_done;
	unsigned int tx_pkt_cnt;
	unsigned int tx_pkt_done_cnt;
	unsigned int scan_pkt_cnt;
	unsigned int internal_pkt_cnt;
	unsigned int internal_pkt_done_cnt;
	unsigned int ack_resp_cnt;
	unsigned int tx_timeout;
	unsigned int deagg_isr;
	unsigned int deagg_inptr_desc_empty;
	unsigned int deagg_circular_buffer_full;
	unsigned int lmac_rxisr_cnt;
	unsigned int rx_decryptcnt;
	unsigned int process_decrypt_fail;
	unsigned int prepa_rx_event_fail;
	unsigned int rx_core_pool_full_cnt;
	unsigned int rx_mpdu_crc_success_cnt;
	unsigned int rx_mpdu_crc_fail_cnt;
	unsigned int rx_ofdm_crc_success_cnt;
	unsigned int rx_ofdm_crc_fail_cnt;
	unsigned int rxDSSSCrcSuccessCnt;
	unsigned int rxDSSSCrcFailCnt;
	unsigned int rx_crypto_start_cnt;
	unsigned int rx_crypto_done_cnt;
	unsigned int rx_event_buf_full;
	unsigned int rx_extram_buf_full;
	unsigned int scan_req;
	unsigned int scan_complete;
	unsigned int scan_abort_req;
	unsigned int scan_abort_complete;
	unsigned int internal_buf_pool_null;
	/*END:LMAC DEBUG Stats*/
} __NRF_WIFI_PKD;

#endif /* !CONFIG_NRF700X_RADIO_TEST */

struct rpu_phy_stats {
	signed char rssi_avg;
	unsigned char pdout_val;
	unsigned int ofdm_crc32_pass_cnt;
	unsigned int ofdm_crc32_fail_cnt;
	unsigned int dsss_crc32_pass_cnt;
	unsigned int dsss_crc32_fail_cnt;
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_sys_head - Command/Event header.
 * @cmd: Command/Event id.
 * @len: Payload length.
 *
 * This header needs to be initialized in every command and has the event
 * id info in case of events.
 */

struct nrf_wifi_sys_head {
	unsigned int cmd_event;
	unsigned int len;

} __NRF_WIFI_PKD;

/**
 * struct bgscan_params - Background Scan parameters.
 * @enabled: Enable/Disable background scan.
 * @channel_list: List of channels to scan.
 * @channel_flags: Channel flags for each of the channels which are to be
 *	scanned.
 * @scan_intval: Back ground scan is done at regular intervals. This
 *	value is set to the interval value (in ms).
 * @channel_dur: Time to be spent on each channel (in ms).
 * @serv_channel_dur: In "Connected State" scanning, we need to share the time
 *	between operating channel and non-operating channels.
 *	After scanning each channel, the firmware spends
 *	"serv_channel_dur" (in ms) on the operating channel.
 * @num_channels: Number of channels to be scanned.
 *
 * This structure specifies the parameters which will be used during a
 * Background Scan.
 */

struct nrf_wifi_bgscan_params {
	unsigned int enabled;
	unsigned char channel_list[50];
	unsigned char channel_flags[50];
	unsigned int scan_intval;
	unsigned int channel_dur;
	unsigned int serv_channel_dur;
	unsigned int num_channels;
} __NRF_WIFI_PKD;

/**
 * @NRF_WIFI_FEATURE_DISABLE: Feature Disable.
 * @NRF_WIFI_FEATURE_ENABLE: Feature Enable.
 *
 */
#define NRF_WIFI_FEATURE_DISABLE 0
#define NRF_WIFI_FEATURE_ENABLE 1

/**
 * enum max_rx_ampdu_size - Max Rx AMPDU size in KB
 *
 * Max Rx AMPDU Size
 */
enum max_rx_ampdu_size {
	MAX_RX_AMPDU_SIZE_8KB,
	MAX_RX_AMPDU_SIZE_16KB,
	MAX_RX_AMPDU_SIZE_32KB,
	MAX_RX_AMPDU_SIZE_64KB
};

/**
 * struct nrf_wifi_data_config_params - Data config parameters
 * @rate_protection_type:0->NONE, 1->RTS/CTS, 2->CTS2SELF
 * @aggregation: Agreegation is enabled(NRF_WIFI_FEATURE_ENABLE) or disabled
 *		(NRF_WIFI_FEATURE_DISABLE)
 * @wmm: WMM is enabled(NRF_WIFI_FEATURE_ENABLE) or disabled
 *		(NRF_WIFI_FEATURE_DISABLE)
 * @max_num_tx_agg_sessions: Max number of aggregated TX sessions
 * @max_num_rx_agg_sessions: Max number of aggregated RX sessions
 * @reorder_buf_size: Reorder buffer size (1 to 64)
 * @max_rxampdu_size: Max RX AMPDU size (8/16/32/64 KB), see
 *					enum max_rx_ampdu_size
 *
 * Data configuration parameters provided in command NRF_WIFI_CMD_INIT
 */

struct nrf_wifi_data_config_params {
	unsigned char rate_protection_type;
	unsigned char aggregation;
	unsigned char wmm;
	unsigned char max_num_tx_agg_sessions;
	unsigned char max_num_rx_agg_sessions;
	unsigned char max_tx_aggregation;
	unsigned char reorder_buf_size;
	signed int max_rxampdu_size;
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_sys_params - Init parameters during NRF_WIFI_CMD_INIT
 * @mac_addr: MAC address of the interface
 * @sleep_enable: enable rpu sleep
 * @hw_bringup_time:
 * @sw_bringup_time:
 * @bcn_time_out:
 * @calib_sleep_clk:
 * @rf_params: RF parameters
 * @rf_params_valid: Indicates whether the @rf_params has a valid value.
 * @phy_calib: PHY calibration parameters
 *
 * System parameters provided for command NRF_WIFI_CMD_INIT
 */

struct nrf_wifi_sys_params {
	unsigned int sleep_enable;
	unsigned int hw_bringup_time;
	unsigned int sw_bringup_time;
	unsigned int bcn_time_out;
	unsigned int calib_sleep_clk;
	unsigned int phy_calib;
#ifndef CONFIG_NRF700X_RADIO_TEST
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	unsigned char rf_params[NRF_WIFI_RF_PARAMS_SIZE];
	unsigned char rf_params_valid;
#endif /* !CONFIG_NRF700X_RADIO_TEST */
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_cmd_radiotest_req_set_reg - command for setting regulatory
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head
 * @nrf_wifi_alpha: regulatory country code.
 *
 */
#define NRF_WIFI_CMD_RT_REQ_SET_REG_ALPHA_VALID (1 << 0)

struct nrf_wifi_radiotest_req_set_reg {
		unsigned int valid_fields;
		unsigned char nrf_wifi_alpha[3];
} __NRF_WIFI_PKD;

struct nrf_wifi_cmd_radiotest_req_set_reg {
	struct nrf_wifi_sys_head sys_head;
	struct nrf_wifi_radiotest_req_set_reg set_reg_info;
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_cmd_sys_init - Initialize UMAC
 * @sys_head: umac header, see &nrf_wifi_sys_head
 * @wdev_id : id of the interface.
 * @sys_params: iftype, mac address, see nrf_wifi_sys_params
 * @rx_buf_pools: LMAC Rx buffs pool params, see struct rx_buf_pool_params
 * @data_config_params: Data configuration params, see struct nrf_wifi_data_config_params
 * @tcp_ip_checksum_offload: 0: Native checksum, 1: Offload checksum
 * After host driver bringup host sends the NRF_WIFI_CMD_INIT to the RPU.
 * then RPU initializes and responds with NRF_WIFI_EVENT_BUFF_CONFIG.
 */

struct nrf_wifi_cmd_sys_init {
	struct nrf_wifi_sys_head sys_head;
	unsigned int wdev_id;
	struct nrf_wifi_sys_params sys_params;
	struct rx_buf_pool_params rx_buf_pools[MAX_NUM_OF_RX_QUEUES];
	struct nrf_wifi_data_config_params data_config_params;
	struct temp_vbat_config temp_vbat_config_params;
	unsigned char tcp_ip_checksum_offload;
	struct nrf_wifi_radiotest_req_set_reg set_reg_info;
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_cmd_sys_deinit - De-initialize UMAC
 * @sys_head: umac header, see &nrf_wifi_sys_head
 *
 * De-initializes the RPU.
 */

struct nrf_wifi_cmd_sys_deinit {
	struct nrf_wifi_sys_head sys_head;
} __NRF_WIFI_PKD;

#define NRF_WIFI_HE_GI_800NS 0
#define NRF_WIFI_HE_GI_1600NS 1
#define NRF_WIFI_HE_GI_3200NS 2

#define NRF_WIFI_HE_LTF_3200NS 0
#define NRF_WIFI_HE_LTF_6400NS 1
#define NRF_WIFI_HE_LTF_12800NS 2

/**
 * struct nrf_wifi_cmd_he_gi_ltf_config - Confure HE-GI and HE-LTF.
 * @sys_head: umac header, see &nrf_wifi_sys_head
 * @wdev_id: wdev interface id.
 * @he_gi_type: HE GI type(NRF_WIFI_HE_GI_800NS/NRF_WIFI_HE_GI_1600NS/NRF_WIFI_HE_GI_3200NS).
 * @he_ltf: HE LTF(NRF_WIFI_HE_LTF_3200NS/NRF_WIFI_HE_LTF_6400NS/NRF_WIFI_HE_LTF_12800NS).
 * @enable: Fixed HE GI & LTF values can be enabled and disabled
 * Host configures the HE-GI & HE-LTF for testing purpose
 * need to use this values in Tx command sending to LMAC.
 */

struct nrf_wifi_cmd_he_gi_ltf_config {
	struct nrf_wifi_sys_head sys_head;
	unsigned char wdev_id;
#define NRF_WIFI_HE_GI_800NS 0
#define NRF_WIFI_HE_GI_1600NS 1
#define NRF_WIFI_HE_GI_3200NS 2
	unsigned char he_gi_type;
	unsigned char he_ltf;
	unsigned char enable;
} __NRF_WIFI_PKD;

/* host has to use nrf_wifi_data_buff_config_complete structure to inform rpu about
 * completin of buffers configuration. fill NRF_WIFI_CMD_BUFF_CONFIG_COMPLETE in cmd
 * field.
 */

struct nrf_wifi_cmd_buff_config_complete {
	struct nrf_wifi_sys_head sys_head;
} __NRF_WIFI_PKD;


#define		NRF_WIFI_DISABLE		0
#define		NRF_WIFI_ENABLE		1

enum rpu_pkt_preamble {
	RPU_PKT_PREAMBLE_SHORT,
	RPU_PKT_PREAMBLE_LONG,
	RPU_PKT_PREAMBLE_MIXED,
	RPU_PKT_PREAMBLE_MAX,
};

struct nrf_wifi_cmd_mode {
	struct nrf_wifi_sys_head sys_head;
	signed int mode;

} __NRF_WIFI_PKD;


struct rpu_conf_params {
	unsigned char nss;
	unsigned char antenna_sel;
	unsigned char rf_params[NRF_WIFI_RF_PARAMS_SIZE];
	unsigned char tx_pkt_chnl_bw;
	unsigned char tx_pkt_tput_mode;
	unsigned char tx_pkt_sgi;

	unsigned char tx_pkt_nss;
	unsigned char tx_pkt_preamble;
	unsigned char tx_pkt_stbc;
	unsigned char tx_pkt_fec_coding;
	signed char tx_pkt_mcs;
	signed char tx_pkt_rate;
	signed char phy_threshold;
	unsigned int phy_calib;
	signed int op_mode;
	struct chan_params chan;
	unsigned char tx_mode;
	signed int tx_pkt_num;
	unsigned short tx_pkt_len;
	unsigned int tx_power;
	unsigned char tx;
	unsigned char rx;
	unsigned char aux_adc_input_chain_id;
	unsigned char agg;
	unsigned char he_ltf;
	unsigned char he_gi;
	unsigned char set_he_ltf_gi;
	unsigned char power_save;
	unsigned int rts_threshold;
	unsigned int uapsd_queue;
	unsigned int tx_pkt_gap_us;
	unsigned char wlan_ant_switch_ctrl;
	unsigned char ble_ant_switch_ctrl;
	unsigned char ru_tone;
	unsigned char ru_index;
	signed char tx_tone_freq;
	unsigned char lna_gain;
	unsigned char bb_gain;
	unsigned short int capture_length;
	unsigned char bypass_regulatory;
	unsigned int tx_pkt_cw;
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_cmd_mode_params
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head
 * @conf: configuration parameters of different modes see &union rpu_conf_params
 *
 * configures the RPU with config parameters provided in this command
 */

struct nrf_wifi_cmd_mode_params {
	struct nrf_wifi_sys_head sys_head;
	struct rpu_conf_params conf;
	unsigned short pkt_length[MAX_TX_AGG_SIZE];
	unsigned int ddr_ptrs[MAX_TX_AGG_SIZE];
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_cmd_radio_test_init - command radio_test_init
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head
 * @conf: radiotest init configuration parameters
 * see &struct nrf_wifi_radio_test_init_info
 *
 */
struct nrf_wifi_radio_test_init_info {
	unsigned char rf_params[NRF_WIFI_RF_PARAMS_SIZE];
	struct chan_params chan;
	signed char phy_threshold;
	unsigned int phy_calib;
} __NRF_WIFI_PKD;

struct nrf_wifi_cmd_radio_test_init {
	struct nrf_wifi_sys_head sys_head;
	struct nrf_wifi_radio_test_init_info conf;
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_cmd_rx - command rx
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head
 * @conf: rx configuration parameters see &struct rpu_conf_rx_radio_test_params
 * @:rx_enable: 1-Enable Rx to receive packets contineously on specified channel
 *	0-Disable Rx stop receiving packets and clear statistics
 *
 * Command RPU to Enable/Disable Rx
 */

struct nrf_wifi_cmd_rx {
	struct nrf_wifi_sys_head sys_head;
	struct rpu_conf_rx_radio_test_params conf;
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_cmd_get_stats - Get statistics
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head
 * @stats_type: Statistics type see &enum rpu_stats_type
 * @op_mode: Production mode or FCM mode
 *
 * This command is to Request the statistics corresponding to stats_type
 * selected
 *
 */

struct nrf_wifi_cmd_get_stats {
	struct nrf_wifi_sys_head sys_head;
	signed int stats_type;
	signed int op_mode;
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_cmd_clear_stats - clear statistics
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head.
 * @stats_type: Type of statistics to clear see &enum rpu_stats_type
 *
 * This command is to clear the statistics corresponding to stats_type selected
 */

struct nrf_wifi_cmd_clear_stats {
	struct nrf_wifi_sys_head sys_head;
	signed int stats_type;
} __NRF_WIFI_PKD;

struct nrf_wifi_cmd_pwr {
	struct nrf_wifi_sys_head sys_head;
	signed int data_type;
} __NRF_WIFI_PKD;

struct coex_wlan_switch_ctrl {
	signed int rpu_msg_id;
	signed int switch_A;
} __NRF_WIFI_PKD;

struct nrf_wifi_cmd_btcoex {
	struct nrf_wifi_sys_head sys_head;
	struct coex_wlan_switch_ctrl conf;
} __NRF_WIFI_PKD;

struct rpu_cmd_coex_config_info {
	unsigned int len;
	unsigned char coex_cmd[0];
} __NRF_WIFI_PKD;

struct nrf_wifi_cmd_coex_config {
	struct nrf_wifi_sys_head sys_head;
	struct rpu_cmd_coex_config_info coex_config_info;
} __NRF_WIFI_PKD;

struct rpu_evnt_coex_config_info {
	unsigned int len;
	unsigned char coex_event[0];
} __NRF_WIFI_PKD;

struct nrf_wifi_event_coex_config {
	struct nrf_wifi_sys_head sys_head;
	struct rpu_evnt_coex_config_info coex_config_info;
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_cmd_fix_tx_rate - UMAC deinitialization done
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head.
 * rate_flags: refer &enum rpu_tput_mode.
 * fixed_rate: -1 Disable fixed rate and use ratecontrol selected rate.
 *             >0 legacy rates: 1,2,55,11,6,9,12,18,24,36,48,54.
 *                11N VHT HE  : MCS index 0 to 7.
 */

struct nrf_wifi_cmd_fix_tx_rate {
	struct nrf_wifi_sys_head sys_head;
	unsigned char rate_flags;
	int fixed_rate;
} __NRF_WIFI_PKD;

struct rpu_cmd_rftest_info {
	unsigned int len;
	unsigned char rfcmd[0];
} __NRF_WIFI_PKD;

struct nrf_wifi_cmd_rftest {
	struct nrf_wifi_sys_head sys_head;
	struct rpu_cmd_rftest_info rf_test_info;
} __NRF_WIFI_PKD;

struct rpu_evnt_rftest_info {
	unsigned int len;
	unsigned char rfevent[0];
} __NRF_WIFI_PKD;

struct nrf_wifi_event_rftest {
	struct nrf_wifi_sys_head sys_head;
	struct rpu_evnt_rftest_info rf_test_info;
} __NRF_WIFI_PKD;

struct nrf_wifi_event_pwr_data {
	struct nrf_wifi_sys_head sys_head;
	signed int mon_status;
	signed int data_type;
	struct nrf_wifi_rpu_pwr_data data;
} __NRF_WIFI_PKD;


/**
 * struct rpu_fw_stats - FW statistics
 * @phy:  PHY statistics  see &struct rpu_phy_stats
 * @lmac: LMAC statistics see &struct rpu_lmac_stats
 * @umac: UMAC statistics see &struct rpu_umac_stats
 *
 * This structure is a combination of all the statistics that the RPU firmware
 * can provide
 *
 */

struct rpu_fw_stats {
	struct rpu_phy_stats phy;
#ifndef CONFIG_NRF700X_RADIO_TEST
	struct rpu_lmac_stats lmac;
	struct rpu_umac_stats umac;
#endif /* !CONFIG_NRF700X_RADIO_TEST */
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_umac_event_stats - statistics event
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head.
 * @fw: All the statistics that the firmware can provide.
 *
 * This event is the response to command NRF_WIFI_CMD_GET_STATS.
 *
 */

struct nrf_wifi_umac_event_stats {
	struct nrf_wifi_sys_head sys_head;
	struct rpu_fw_stats fw;
} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_umac_event_cmd_err_status - cmd error indication
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head.
 * @status: status of the command ie Fail(Type of err) or success.
 *
 * This event is the response to command chanl_prog.
 *
 */
enum nrf_wifi_radio_test_err_status {
		NRF_WIFI_UMAC_CMD_SUCCESS = 1,
		NRF_WIFI_UMAC_INVALID_CHNL
};

struct nrf_wifi_umac_event_err_status {
		struct nrf_wifi_sys_head sys_head;
		unsigned int status;
} __NRF_WIFI_PKD;

#ifndef CONFIG_NRF700X_RADIO_TEST
/**
 * struct nrf_wifi_event_buff_config_done - Buffers configuration done
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head.
 * @mac_addr: Mac address of the RPU
 *
 * RPU sends this event in response to NRF_WIFI_CMD_BUFF_CONFIG_COMPLETE informing
 * RPU is initialized
 */

struct nrf_wifi_event_buff_config_done {
	struct nrf_wifi_sys_head sys_head;
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];

} __NRF_WIFI_PKD;

/**
 * struct nrf_wifi_txrx_buffs_config - TX/RX buffers config event.
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head.
 * @max_tx_descs: Max number of tx descriptors.
 * @max_2k_rx_descs: Max number of 2k rx descriptors.
 * @num_8k_rx_descs: Max number of 2k rx descriptors.
 * @num_mgmt_descs: Max number of mgmt buffers.
 *
 * After initialization RPU sends NRF_WIFI_EVENT_BUFF_CONFIG
 * to inform host regarding descriptors.
 * 8K buffer are for internal purpose. At initialization time host
 * submits the 8K buffer and UMAC uses buffers to configure LMAC
 * for receiving AMSDU packets.
 */

struct nrf_wifi_event_buffs_config {
	struct nrf_wifi_sys_head sys_head;
	unsigned int max_tx_descs;
	unsigned int max_2k_rx_descs;
	unsigned int num_8k_rx_descs;
	unsigned int num_mgmt_descs;

} __NRF_WIFI_PKD;

#endif /* !CONFIG_NRF700X_RADIO_TEST */

/**
 * struct nrf_wifi_event_init_done - UMAC initialization done
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head.
 *
 * RPU sends this event in response to NRF_WIFI_CMD_INIT indicating that the RPU is
 * initialized
 */

struct nrf_wifi_event_init_done {
	struct nrf_wifi_sys_head sys_head;
} __NRF_WIFI_PKD;

struct pool_data_to_host {
	unsigned int buffer_size;
	unsigned char num_pool_items;
	unsigned char items_num_max_allocated;
	unsigned char items_num_cur_allocated;
	unsigned int items_num_total_allocated;
	unsigned int items_num_not_allocated;
} __NRF_WIFI_PKD;

struct umac_int_stats {
	struct nrf_wifi_sys_head sys_head;
	struct pool_data_to_host scratch_dynamic_memory_info[56];
	struct pool_data_to_host retention_dynamic_memory_info[56];
} __NRF_WIFI_PKD;
/**
 * struct nrf_wifi_event_deinit_done - UMAC deinitialization done
 * @sys_head: UMAC header, See &struct nrf_wifi_sys_head.
 *
 * RPU sends this event in response to NRF_WIFI_CMD_DEINIT indicating that the RPU is
 * deinitialized
 */

struct nrf_wifi_event_deinit_done {
	struct nrf_wifi_sys_head sys_head;
} __NRF_WIFI_PKD;

#endif /* __HOST_RPU_SYS_IF_H__ */
