/*
 *
 *Copyright (c) 2022 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 *
 *@brief <Data interface between host and RPU>
 */
#ifndef __HOST_RPU_DATA_IF_H__
#define __HOST_RPU_DATA_IF_H__

#include "host_rpu_common_if.h"
#include "host_rpu_sys_if.h"

#include "pack_def.h"

#define IMG_ETH_ALEN 6
#define TX_BUF_HEADROOM 52

/*
 * -------------------------------------------------------------
 * | host_rpu_msg | cmd1 | cmd2 | cmd2 |...................... |
 * -------------------------------------------------------------
 * host_rpu_msg.msg_len should contains total length of all commands
 */

/**
 * enum img_umac_data_commands - UMAC init and data buffer command/events
 *
 * @IMG_CMD_MGMT_BUFF_CONFIG: Configure MGMT frame buffer.
 *	See &struct img_rx_buff_config
 * @IMG_CMD_TX_BUFF: Transmit data packet.
 *	See &struct img_tx_buff
 * @IMG_CMD_TX_BUFF_DONE: TX done event.
 *	See &struct img_tx_buff_done
 * @IMG_CMD_RX_BUFF: RX data event.
 *	See &struct img_rx_buff
 * @IMG_CMD_CARRIER_ON: STA connection complete event.
 *	See &struct img_data_carrier_state
 * @IMG_CMD_CARRIER_OFF: STA disconnected event.
 *	See &struct img_data_carrier_state
 * @IMG_CMD_PM_MODE: SoftAP client power save event.
 *	See &struct img_sap_client_pwrsave
 * @IMG_CMD_PS_GET_FRAMES: SoftAP client PS get frames event.
 *	See &struct img_sap_ps_get_frames
 *
 */
#define IMG_CMD_MGMT_BUFF_CONFIG 0
/* host uses IMG_CMD_TX_BUFF to give tx packet to RPU */
#define IMG_CMD_TX_BUFF 1
/* RPU sends tx status to the host using IMG_CMD_TX_BUFF_DONE */
#define IMG_CMD_TX_BUFF_DONE 2
/* RPU send received buffer using IMG_CMD_RX_BUFF to host */
#define IMG_CMD_RX_BUFF 3
#define IMG_CMD_CARRIER_ON 4
#define IMG_CMD_CARRIER_OFF 5
#define IMG_CMD_PM_MODE 6
#define IMG_CMD_PS_GET_FRAMES 7

/**
 * struct img_umac_head - Command/Event header.
 * @cmd: Command/Event id.
 * @len: Payload length.
 *
 * This header needs to be initialized in every command and has the event
 * id info in case of events.
 */

struct img_umac_head {
	unsigned int cmd;
	unsigned int len;

} __IMG_PKD;

/**
 * struct img_packet_info - Data packet frame pointers.
 * @head: Pointer to the start of headroom.
 * @data: Potiner to the start of data/actual frame data.
 * @tail: End of frame data.
 * @end: End of tailroom.
 *
 */

struct img_packet_info {
	unsigned int head;
	unsigned int data;
	unsigned int tail;
	unsigned int end;
} __IMG_PKD;

/**
 * struct img_mgmt_buff_config - Configure management buffers.
 * @umac_head: UMAC cmd header, See &struct img_umac_hdr.
 * @num_mgmt_bufs: Number of Mgmt buffers to be configured.
 * @ddr_ptrs: Management DDR buffer pointers.
 *
 * Management buffers once programmed will be used internally by UMAC.
 */

struct img_mgmt_buff_config {
	struct img_umac_head umac_head;
	unsigned int num_mgmt_bufs;
	unsigned int ddr_ptrs[MAX_MGMT_BUFS];
} __IMG_PKD;

/**
 * HEADER_FILL_FLAGS - mac80211 header filled information.
 * @FC_POPULATED: Frame Control field is populated by Host Driver.
 * @DUR_POPULATED: Duration field is populated by Host Driver.
 * @ADDR1_POPULATED: Address 1 field is populated by Host Driver.
 * @ADDR2_POPULATED: Address 2 field is populated by Host Driver.
 * @ADDR3_POPULATED: Address 3 field is populated by Host Driver.
 * @SEQ_CTRL_POPULATED: Sequence Control field is populated by Host Driver.
 * @QOS_CTRL_POPULATED: Qos field is populated by Host Driver.
 *
 */
#define FC_POPULATED (0x00000001 << 1)
#define DUR_POPULATED (0x00000001 << 2)
#define ADDR1_POPULATED (0x00000001 << 3)
#define ADDR2_POPULATED (0x00000001 << 4)
#define ADDR3_POPULATED (0x00000001 << 5)
#define SEQ_CTRL_POPULATED (0x00000001 << 6)
#define QOS_CTRL_POPULATED (0x00000001 << 7)

/**
 * struct tx_mac_hdr_info - Tx mac80211 header information.
 * @umac_head: UMAC cmd header, See &struct img_umac_hdr.
 * @umac_fill_flags: Flags indicates which of the following fields present.
 * @fc: Frame Control.
 * @more_data: 0-> No more Data, 1-> More Data
 * @dest: Destination Address.
 * @src: Source Address.
 * @etype: Ethernet type.
 * @dscp_or_tos: Type of Service.
 * @more_data:more frames queued
 * @eosp: End Of Service Period flag(applicable in U-APSD)
 *
 * Host fills the mac80211 header fields and indicates to UMAC.
 */

struct tx_mac_hdr_info {
	int umac_fill_flags;
	unsigned short fc;
	unsigned char dest[6];
	unsigned char src[6];
	/* needed only if UMAC is preparing LLC header */
	unsigned short etype;
	/* needed for TID */
	unsigned int dscp_or_tos;
	unsigned char more_data;
	unsigned char eosp;
} __IMG_PKD;

/**
 * struct img_tx_buff_info - TX data command info.
 * @pkt_length: Tx packet length.
 * @ddr_ptr: Tx packet data pointer.
 */

struct img_tx_buff_info {
	unsigned short pkt_length;
	unsigned int ddr_ptr;
} __IMG_PKD;

/**
 * struct img_tx_buff - Send TX packet.
 * @umac_head: UMAC cmd header. See &struct img_umac_hdr.
 * @wdev_id: wdev interface id.
 * @tx_desc_num: Descriptor id.
 * @mac_hdr_info: Common mac header for all packets to be Txed.
 * @num_tx_pkts: Number of packets.
 * @tx_buff_info: See img_tx_buff_info_t for details, the array size can be maximum
 *	of MAX_TX_AGG_SIZE
 *
 * Host sends the packet information which needs to transmit by
 * using %IMG_CMD_TX_BUFF.
 */

struct img_tx_buff {
	struct img_umac_head umac_head;
	unsigned char wdev_id;
	unsigned char tx_desc_num;
	/* Common Fields */
	/* common for all coalesced packets: Pre-Aggregation */
	struct tx_mac_hdr_info mac_hdr_info;
	unsigned char num_tx_pkts;
	struct img_tx_buff_info tx_buff_info[0];
} __IMG_PKD;

/**
 * @IMG_TX_STATUS_SUCCESS: TX was successful.
 * @IMG_TX_STATUS_FAILED: TX failed.
 *
 */
#define IMG_TX_STATUS_SUCCESS 0
#define IMG_TX_STATUS_FAILED 1

/**
 * struct img_tx_buff_done - TX done event.
 * @umac_head: UMAC event header. See &struct img_umac_hdr.
 * @tx_desc_num: Descriptor id.
 * @num_tx_status_code: Total number of received tx status code, the array size can be maximum
 *	of MAX_TX_AGG_SIZE
 * @timestamp_t1: Frame sent time at Phy
 * @timestamp_t4: Frame ack received time at Phy
 * @tx_status_code: Status of TX packet.
 *
 * RPU acknowledges the packet transmition by using %IMG_CMD_TX_BUFF_DONE.
 */

struct img_tx_buff_done {
	struct img_umac_head umac_head;
	unsigned char tx_desc_num;
	unsigned char num_tx_status_code;
	/*unsigned char timestamp_t1[6];*/
	/*unsigned char timestamp_t4[6];*/
	unsigned char tx_status_code[0];
} __IMG_PKD;

/**
 * enum img_rx_pkt_type: The Received packet type
 * @IMG_RX_PKT_DATA: The Rx packet is of type data.
 * @IMG_RX_PKT_BCN_PRB_RSP: The RX packet is of type beacon or probe response
 *
 */
#define IMG_RX_PKT_DATA 0
#define IMG_RX_PKT_BCN_PRB_RSP 1

/**
 * struct img_rx_buff_info - RX data event info.
 * @descriptor_id: Descriptor id.
 * @rx_pkt_len: Rx packet length.
 * @timestamp_t2: Frame received time at Phy
 * @timestamp_t3: Ack sent time at Phy
 */

struct img_rx_buff_info {
	unsigned short descriptor_id;
	unsigned short rx_pkt_len;
	unsigned char pkt_type;
} __IMG_PKD;

/**
 * struct img_rx_buf - RX data event.
 * @umac_head: UMAC event header. See &struct img_umac_hdr.
 * @wdev_id: wdev interface id.
 * @rx_pkt_cnt: Number of packets received.
 * @rx_pkt_type: Rx packet type.
 * After receiving the RX packet RPU informs the host using IMG_CMD_RX_BUFF.
 * Host refills the buffer using IMG_CMD_2K_RX_BUFF_CONFIG.
 */

struct img_rx_buff {
	struct img_umac_head umac_head;
	int rx_pkt_type;
	unsigned char wdev_id;
	unsigned char rx_pkt_cnt;
	unsigned char rpu_align_offset;
	unsigned char mac_header_len;
	unsigned short frequency;
	short signal;
	struct img_rx_buff_info rx_buff_info[0];
} __IMG_PKD;

/**
 * struct img_data_carrier_state - Carrier state info.
 * @umac_head: UMAC event header. See &struct img_umac_hdr.
 */

struct img_data_carrier_state {
	struct img_umac_head umac_head;
	unsigned int wdev_id;

} __IMG_PKD;

/**
 * @IMG_CLIENT_ACTIVE: SoftAP client is in active mode.
 * @IMG_CLIENT_PS_MODE: SoftAP client is in power save mode.
 */
#define IMG_CLIENT_ACTIVE 0
#define IMG_CLIENT_PS_MODE 1

/**
 * struct img_sap_client_pwrsave - SofAP client power save info.
 * @umac_head: UMAC event header. See &struct img_umac_hdr.
 * @wdev_id: wdev interface id.
 * @sta_ps_state: IMG_CLIENT_ACTIVE or IMG_CLIENT_PS_MODE
 * @mac_addr: STA MAC Address
 */

struct img_sap_client_pwrsave {
	struct img_umac_head umac_head;
	unsigned int wdev_id;
	unsigned char sta_ps_state;
	unsigned char mac_addr[IMG_ETH_ALEN];

} __IMG_PKD;

/**
 * struct img_sap_ps_get_frames - SofAP client PS get frames info.
 * @umac_head: UMAC event header. See &struct img_umac_hdr.
 * @wdev_id: wdev interface id.
 * @mac_addr: STA MAC Address
 * @num_frames: Num frames to transmit in service period
 */

struct img_sap_ps_get_frames {
	struct img_umac_head umac_head;
	unsigned int wdev_id;
	unsigned char mac_addr[IMG_ETH_ALEN];
	char num_frames;

} __IMG_PKD;

#endif /* __HOST_RPU_DATA_IF_H__ */
