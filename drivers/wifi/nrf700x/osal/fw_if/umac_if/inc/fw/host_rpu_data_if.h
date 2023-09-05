/*
 *
 *Copyright (c) 2022 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Data interface between host and RPU
 */

#ifndef __HOST_RPU_DATA_IF_H__
#define __HOST_RPU_DATA_IF_H__

#include "host_rpu_common_if.h"
#include "host_rpu_sys_if.h"

#include "pack_def.h"

#define NRF_WIFI_ETH_ADDR_LEN 6
#define TX_BUF_HEADROOM 52

/**
 * @brief UMAC data interface commands and events.
 *
 */
enum nrf_wifi_umac_data_commands {
	/** Unused. */
	NRF_WIFI_CMD_MGMT_BUFF_CONFIG,
	/** Transmit data packet @ref nrf_wifi_tx_buff */
	NRF_WIFI_CMD_TX_BUFF,
	/** TX done event @ref nrf_wifi_tx_buff_done */
	NRF_WIFI_CMD_TX_BUFF_DONE,
	/** RX packet event @ref nrf_wifi_rx_buff*/
	NRF_WIFI_CMD_RX_BUFF,
	/** Event to indicate interface is operational
	 *  @ref nrf_wifi_data_carrier_state
	 */
	NRF_WIFI_CMD_CARRIER_ON,
	/** Event to indicate interface is non-operational
	 *  @ref nrf_wifi_data_carrier_state
	 */
	NRF_WIFI_CMD_CARRIER_OFF,
	/** Event to indicate softap client's power save mode
	 *  If client is in power save mode, host should start buffering
	 *  packets until it receives NRF_WIFI_CMD_PS_GET_FRAMES event.
	 */
	NRF_WIFI_CMD_PM_MODE,
	/** Event to indicate to start sending buffered packets for
	 *  softap client @ref nrf_wifi_sap_ps_get_frames.
	 */
	NRF_WIFI_CMD_PS_GET_FRAMES,
};

/**
 * @brief Data interface Command and Event header.
 *
 */
struct nrf_wifi_umac_head {
	/** Command or Event id see &enum nrf_wifi_umac_data_commands */
	unsigned int cmd;
	/** length */
	unsigned int len;

} __NRF_WIFI_PKD;

#define DSCP_TOS_MASK  0xFFFF
#define DSCP_OR_TOS_TWT_EMERGENCY_TX  (1 << 31)

/**
 * @brief Tx mac80211 header information.
 *
 */
struct tx_mac_hdr_info {
	/** Unused */
	signed int umac_fill_flags;
	/** frame control */
	unsigned short fc;
	/** source Mac header */
	unsigned char dest[6];
	/** destination Mac address */
	unsigned char src[6];
	/** Ethernet type */
	unsigned short etype;
	/** Type of Service */
	unsigned int dscp_or_tos;
	/** more frames queued */
	unsigned char more_data;
	/** End Of Service Period flag(applicable in U-APSD) */
	unsigned char eosp;
} __NRF_WIFI_PKD;

/**
 * @brief This structure provides the information of each packet in the tx command.
 *
 */

struct nrf_wifi_tx_buff_info {
	/** Tx packet length */
	unsigned short pkt_length;
	/** Tx packet address */
	unsigned int ddr_ptr;
} __NRF_WIFI_PKD;

/**
 * @brief This structure provides the parameters for the tx command.
 *
 */
struct nrf_wifi_tx_buff {
	/** Command header @ref nrf_wifi_umac_head */
	struct nrf_wifi_umac_head umac_head;
	/** Interface id */
	unsigned char wdev_id;
	/** Descriptor id */
	unsigned char tx_desc_num;
	/** Common mac header for all packets in this command
	 *  @ref tx_mac_hdr_info
	 */
	struct tx_mac_hdr_info mac_hdr_info;
	/** Pending buffer size at host to encode queue size
	 *  in qos control field of mac header in TWT enable case
	 */
	unsigned int pending_buf_size;
	/** Number of packets sending in this command */
	unsigned char num_tx_pkts;
	/** Each packets information @ref nrf_wifi_tx_buff_info */
	struct nrf_wifi_tx_buff_info tx_buff_info[0];
} __NRF_WIFI_PKD;

#define NRF_WIFI_TX_STATUS_SUCCESS 0
#define NRF_WIFI_TX_STATUS_FAILED 1

/**
 * @brief This structure represents the Tx done event(NRF_WIFI_CMD_TX_BUFF_DONE).
 *
 */
struct nrf_wifi_tx_buff_done {
	/** Header @ref nrf_wifi_umac_head */
	struct nrf_wifi_umac_head umac_head;
	/** Descriptor id */
	unsigned char tx_desc_num;
	/** Number of packets in this Tx done event */
	unsigned char num_tx_status_code;
	/** Frame sent time at Phy */
	unsigned char timestamp_t1[6];
	/** Frame ack received time at Phy */
	unsigned char timestamp_t4[6];
	/** Status of Tx packet. Maximum of MAX_TX_AGG_SIZE */
	unsigned char tx_status_code[0];
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the type of received packet.
 *
 */
enum nrf_wifi_rx_pkt_type {
	/** The Rx packet is of type data */
	NRF_WIFI_RX_PKT_DATA,
	/** RX packet is beacon or probe response */
	NRF_WIFI_RX_PKT_BCN_PRB_RSP
};

/**
 * @brief This structure provides information about the parameters in the RX data event.
 *
 */
struct nrf_wifi_rx_buff_info {
	/** Descriptor id */
	unsigned short descriptor_id;
	/** Rx packet length */
	unsigned short rx_pkt_len;
	/** type PKT_TYPE_MPDU/PKT_TYPE_MSDU_WITH_MAC/PKT_TYPE_MSDU */
	unsigned char pkt_type;
	/** Frame received time at Phy */
	unsigned char timestamp_t2[6];
	/** Ack sent time at Phy */
	unsigned char timestamp_t3[6];
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents RX data event(NRF_WIFI_CMD_RX_BUFF).
 *
 */
struct nrf_wifi_rx_buff {
	/** Header @ref nrf_wifi_umac_head */
	struct nrf_wifi_umac_head umac_head;
	/** Rx packet type. see &enum nrf_wifi_rx_pkt_type */
	signed int rx_pkt_type;
	/** Interface id */
	unsigned char wdev_id;
	/** Number of packets in this event */
	unsigned char rx_pkt_cnt;
	/** Depricated */
	unsigned char reserved;
	/** MAC header length. Same for all packets in this event */
	unsigned char mac_header_len;
	/** Frequency on which this packet received */
	unsigned short frequency;
	/** signal strength */
	signed short signal;
	/** Information of each packet. @ref nrf_wifi_rx_buff_info */
	struct nrf_wifi_rx_buff_info rx_buff_info[0];
} __NRF_WIFI_PKD;

/**
 * @brief This structure provides information about the carrier (interface) state.
 *
 */
struct nrf_wifi_data_carrier_state {
	/** Header @ref nrf_wifi_umac_head */
	struct nrf_wifi_umac_head umac_head;
	/** Interface id */
	unsigned int wdev_id;

} __NRF_WIFI_PKD;

/** SoftAP client is in active mode */
#define NRF_WIFI_CLIENT_ACTIVE 0
/** SoftAP client is in power save mode */
#define NRF_WIFI_CLIENT_PS_MODE 1

/**
 * @brief This structure describes an event related to the power save state of the softap's client.
 *  When the client is in PS mode (NRF_WIFI_CLIENT_PS_MODE), the host should queue Tx packets.
 *  When the client is in wakeup mode (NRF_WIFI_CLIENT_ACTIVE), the host should send all
 *  buffered and upcoming Tx packets.
 *
 */
struct nrf_wifi_sap_client_pwrsave {
	/** Header @ref nrf_wifi_umac_head */
	struct nrf_wifi_umac_head umac_head;
	/** Interface id */
	unsigned int wdev_id;
	/** state NRF_WIFI_CLIENT_ACTIVE or NRF_WIFI_CLIENT_PS_MODE */
	unsigned char sta_ps_state;
	/** STA MAC Address */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];

} __NRF_WIFI_PKD;

/**
 * @brief This structure represents an event that instructs the host to transmit a specific
 *  number of frames that host queued when softap's client is in power save mode.
 *  This event is primarily used when Softap's client operates in legacy power save mode.
 *  In this scenario, the access point (AP) is required to send a single packet for every PS POLL
 *  frame it receives from the client. Additionally, this mechanism will also be utilized in
 *  UAPSD power save.
 *
 */
struct nrf_wifi_sap_ps_get_frames {
	/** Header @ref nrf_wifi_umac_head */
	struct nrf_wifi_umac_head umac_head;
	/** Interface id */
	unsigned int wdev_id;
	/** STA MAC Address */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** Number of frames to be transmitted in this service period */
	signed char num_frames;

} __NRF_WIFI_PKD;

#endif /* __HOST_RPU_DATA_IF_H__ */
