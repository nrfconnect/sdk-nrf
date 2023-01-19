/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing declarations for utility functions etc for
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_STRUCTS_H__
#define __FMAC_STRUCTS_H__

#include <stdbool.h>

#include "osal_api.h"
#include "host_rpu_umac_if.h"

#define MAX_PEERS 5
#define MAX_SW_PEERS (MAX_PEERS + 1)

#ifdef CONFIG_NRF700X_RADIO_TEST
/**
 * struct rpu_op_stats - Structure to hold per device host and firmware
 *                       statistics.
 * @host: Host statistics.
 * @fw: Firmware statistics.
 *
 * This structure holds per device host and firmware statistics.
 */
struct rpu_op_stats {
	struct rpu_fw_stats fw;
};


/**
 * struct wifi_nrf_fmac_priv - Structure to hold context information for the
 *                             UMAC IF layer.
 * @opriv: Pointer to the OS abstraction layer.
 * @hpriv: Pointer to the HAL layer.
 *
 * This structure maintains the context information necessary for the
 * operation of the UMAC IF layer.
 */
struct wifi_nrf_fmac_priv {
	struct wifi_nrf_osal_priv *opriv;
	struct wifi_nrf_hal_priv *hpriv;
};


/**
 * struct wifi_nrf_fmac_dev_ctx - Structure to hold per device context information
 *                                for the UMAC IF layer.
 * @fpriv: Pointer to the UMAC IF abstraction layer.
 * @os_fmac_dev_ctx: Pointer to the per device OS context which is using the
 *               UMAC IF layer.
 * @hal_ctx: Pointer to the per device HAL context.
 * @stats_req: Flag indicating whether a request for statistics has been sent
 *             to the RPU.
 * @fw_stats: Firmware statistics.
 *
 * This structure maintains the context information necessary for the
 * a single instance of an FullMAC based RPU.
 */
struct wifi_nrf_fmac_dev_ctx {
	struct wifi_nrf_fmac_priv *fpriv;
	void *os_dev_ctx;
	void *hal_dev_ctx;
	struct rpu_fw_stats *fw_stats;
	bool stats_req;
	bool fw_boot_done;
	bool fw_init_done;
	bool fw_deinit_done;
	enum nrf_wifi_rf_test rf_test_type;
	void *rf_test_cap_data;
	unsigned int rf_test_cap_sz;
};

#else /* CONFIG_NRF700X_RADIO_TEST */

/**
 * enum wifi_nrf_fmac_ac - WLAN access categories.
 * @WIFI_NRF_FMAC_AC_BK: Background access category.
 * @WIFI_NRF_FMAC_AC_BE: Best-effor access category.
 * @WIFI_NRF_FMAC_AC_VI: Video access category.
 * @WIFI_NRF_FMAC_AC_VO: Voice access category.
 * @WIFI_NRF_FMAC_AC_MAX: Maximum number of WLAN access categories.
 *
 * This enum lists the possible WLAN access categories.
 */
enum wifi_nrf_fmac_ac {
	WIFI_NRF_FMAC_AC_BK,
	WIFI_NRF_FMAC_AC_BE,
	WIFI_NRF_FMAC_AC_VI,
	WIFI_NRF_FMAC_AC_VO,
	WIFI_NRF_FMAC_AC_MC,
	WIFI_NRF_FMAC_AC_MAX
};


/**
 * enum wifi_nrf_fmac_if_op_state - The operational state of an interface.
 * @WIFI_NRF_FMAC_IF_OP_STATE_DOWN: The interface is non-operational.
 * @WIFI_NRF_FMAC_IF_OP_STATE_UP: The interface is operational.
 * @WIFI_NRF_FMAC_IF_OP_STATE_INVALID: Invalid value. Used for error checks.
 *
 * This enum lists the possible operational states of an interface.
 */
enum wifi_nrf_fmac_if_op_state {
	WIFI_NRF_FMAC_IF_OP_STATE_DOWN,
	WIFI_NRF_FMAC_IF_OP_STATE_UP,
	WIFI_NRF_FMAC_IF_OP_STATE_INVALID
};


/**
 * enum wifi_nrf_fmac_if_carr_state - The carrier state of an interface.
 * @WIFI_NRF_FMAC_IF_CARR_STATE_OFF: The interface carrier is off.
 * @WIFI_NRF_FMAC_IF_CARR_STATE_ON: The interface carrier is on.
 * @WIFI_NRF_FMAC_IF_CARR_STATE_INVALID: Invalid value. Used for error checks.
 *
 * This enum lists the possible operational states of an interface.
 */
enum wifi_nrf_fmac_if_carr_state {
	WIFI_NRF_FMAC_IF_CARR_STATE_OFF,
	WIFI_NRF_FMAC_IF_CARR_STATE_ON,
	WIFI_NRF_FMAC_IF_CARR_STATE_INVALID
};


/**
 * struct wifi_nrf_fmac_callbk_fns - Callback functions to be invoked by UMAC
 *				     IF layer when a paticular event occurs.
 * @if_assoc_callbk_fn: Callback function to be called when an interface association
 *                      state changes.
 * @rx_frm_callbk_fn: Callback function to be called when a frame is received.
 *
 * This structure contains function pointers to all the callback functions that
 * the UMAC IF layer needs to invoked for various events.
 */
struct wifi_nrf_fmac_callbk_fns {
	enum wifi_nrf_status (*if_carr_state_chg_callbk_fn)(void *os_vif_ctx,
							    enum wifi_nrf_fmac_if_carr_state cs);

	void (*rx_frm_callbk_fn)(void *os_vif_ctx,
				 void *frm);

	void (*scan_start_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_umac_event_trigger_scan *scan_start_event,
				     unsigned int event_len);

	void (*scan_done_callbk_fn)(void *os_vif_ctx,
				    struct nrf_wifi_umac_event_trigger_scan *scan_done_event,
				    unsigned int event_len);

	void (*scan_abort_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_umac_event_trigger_scan *scan_done_event,
				     unsigned int event_len);

	void (*scan_res_callbk_fn)(void *os_vif_ctx,
				   struct nrf_wifi_umac_event_new_scan_results *scan_res,
				   unsigned int event_len,
				   bool more_res);

	void (*disp_scan_res_callbk_fn)(void *os_vif_ctx,
				  struct nrf_wifi_umac_event_new_scan_display_results *scan_res,
				  unsigned int event_len,
				  bool more_res);

	void (*auth_resp_callbk_fn)(void *os_vif_ctx,
				    struct nrf_wifi_umac_event_mlme *auth_resp_event,
				    unsigned int event_len);

	void (*assoc_resp_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_umac_event_mlme *assoc_resp_event,
				     unsigned int event_len);

	void (*deauth_callbk_fn)(void *os_vif_ctx,
				 struct nrf_wifi_umac_event_mlme *deauth_event,
				 unsigned int event_len);

	void (*disassoc_callbk_fn)(void *os_vif_ctx,
				   struct nrf_wifi_umac_event_mlme *disassoc_event,
				   unsigned int event_len);

	void (*mgmt_rx_callbk_fn)(void *os_vif_ctx,
				  struct nrf_wifi_umac_event_mlme *mgmt_rx_event,
				  unsigned int event_len);

	void (*unprot_mlme_mgmt_rx_callbk_fn)(void *os_vif_ctx,
					      struct nrf_wifi_umac_event_mlme *unprot_mlme_event,
					      unsigned int event_len);

	void (*tx_pwr_get_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_umac_event_get_tx_power *info,
				     unsigned int event_len);

	void (*chnl_get_callbk_fn)(void *os_vif_ctx,
				   struct nrf_wifi_umac_event_get_channel *info,
				   unsigned int event_len);

	void (*sta_get_callbk_fn)(void *os_vif_ctx,
				  struct nrf_wifi_umac_event_new_station *info,
				  unsigned int event_len);

	void (*cookie_rsp_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_umac_event_cookie_rsp *cookie_rsp,
				     unsigned int event_len);

	void (*tx_status_callbk_fn)(void *os_vif_ctx,
				    struct nrf_wifi_umac_event_mlme *tx_status_event,
				    unsigned int event_len);

	void (*set_if_callbk_fn)(void *os_vif_ctx,
				 struct nrf_wifi_umac_event_set_interface *set_if_event,
				 unsigned int event_len);

	void (*roc_callbk_fn)(void *os_vif_ctx,
			      struct nrf_wifi_event_remain_on_channel *roc_event,
			      unsigned int event_len);

	void (*roc_cancel_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_event_remain_on_channel *roc_cancel_event,
				     unsigned int event_len);

	void (*get_station_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_umac_event_new_station *info,
				     unsigned int event_len);

	void (*get_interface_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_interface_info *info,
				     unsigned int event_len);

	void (*mgmt_tx_status)(void *if_priv,
					struct nrf_wifi_umac_event_mlme *mlme_event,
					unsigned int event_len);

	void (*twt_config_callbk_fn)(void *if_priv,
		struct nrf_wifi_umac_cmd_config_twt *twt_config_event_info,
		unsigned int event_len);

	void (*twt_teardown_callbk_fn)(void *if_priv,
		struct nrf_wifi_umac_cmd_teardown_twt *twt_teardown_event_info,
		unsigned int event_len);

	void (*event_get_wiphy)(void *if_priv,
		struct nrf_wifi_event_get_wiphy *get_wiphy,
		unsigned int event_len);

	void (*twt_sleep_callbk_fn)(void *if_priv,
		struct nrf_wifi_umac_event_twt_sleep *twt_sleep_event_info,
		unsigned int event_len);

	void (*event_get_reg)(void *if_priv,
		struct nrf_wifi_reg *get_reg,
		unsigned int event_len);

	void (*event_get_ps_info)(void *if_priv,
		struct nrf_wifi_umac_event_power_save_info *get_ps_config,
		unsigned int event_len);
};


struct wifi_nrf_fmac_buf_map_info {
	bool mapped;
	unsigned long nwb;
};


/**
 * struct rpu_host_stats - Structure to hold host specific statistics.
 * @total_tx_pkts: Total number of frames transmitted.
 * @total_tx_done_pkts: Total number of TX dones received.
 * @total_rx_pkts: Total number of frames received.
 * @tx_coalesce_frames: Total number of transmit frames coalesced.
 * @tx_done_coalesce_frames: Total number of TX dones received for coalesced
 *                           frames.
 * @no_isrs: Total number of interrupts received.
 * @no_events: Total number of events received.
 * @no_events_resubmit: Total number of event pointer resubmitted back to
 *                      the RPU.
 *
 * This structure holds the host specific statistics.
 */
struct rpu_host_stats {
	unsigned long long total_tx_pkts;
	unsigned long long total_tx_done_pkts;
	unsigned long long total_rx_pkts;
};


/**
 * struct rpu_op_stats - Structure to hold per device host and firmware
 *                       statistics.
 * @host: Host statistics.
 * @fw: Firmware statistics.
 *
 * This structure holds per device host and firmware statistics.
 */
struct rpu_op_stats {
	struct rpu_host_stats host;
	struct rpu_fw_stats fw;
};


/**
 * struct peer_info - Structure to hold peer context information.
 * @peer_id: Index with which a peer can be identified locally.
 * @if_idx: Index of the VIF on which the peer is connected.
 * @ps_state: Power save state of the peer.
 * @is_legacy: Flag to indicate if the peer is HT/VHT or non-HT/VHT.
 * @qos_supported: Flag to indicate if the peer support WMM-QoS.
 * @pend_q_bmp: Bitmap representing status of the TX pending queue.
 * @ra_addr: The receiver address (RA) for the peer.
 * @pairwise_cipher: The pairwise cipher being used with the peer.
 * @ps_token_count: The 802.11 powersave token count for the peer.
 *
 * This structure holds context information for a peer that the RPU is
 * connected with.
 */
struct peers_info {
	int peer_id;
	unsigned char if_idx;
	unsigned char ps_state;
	unsigned char is_legacy;
	unsigned char qos_supported;
	unsigned char pend_q_bmp;
	unsigned char ra_addr[NRF_WIFI_ETH_ADDR_LEN];
	unsigned int pairwise_cipher;
	int ps_token_count;
};


/**
 * struct tx_config - Structure to hold transmit path context information.
 * @tx_lock: Lock used to make code portions in the TX path atomic.
 * @peers: Context information about peers that the RPU is connected to.
 * @send_pkt_coalesce_count_p: Colasece count of TX frames.
 * @data_pending_txq: Queue for frames waiting to be passed to the RPU for TX.
 * @wakeup_client_q: Queue for peers which have woken up from 802.11
 *                   power save.
 * @buf_pool_bmp_p: Bitmap of the TX buffer pool.
 * @outstanding_descs: TX description which have been queued to the RPU.
 * @curr_peer_opp: Peer who will be get the next opportunity for TX.
 * @next_spare_desc_ac: Access category which will get the next spare
 *                      descriptor.
 * @pkt_info_p: Frame context information.
 * @spare_desc_queue_map: Map for the spare descriptor queues.
 *
 * This structure holds context information for the transmit path.
 */
struct tx_config {
	void *tx_lock;

	struct peers_info peers[MAX_SW_PEERS];
	unsigned int *send_pkt_coalesce_count_p;
	void *data_pending_txq[MAX_SW_PEERS][WIFI_NRF_FMAC_AC_MAX];
	void *wakeup_client_q;

	/* Used to store tx descs(buff pool ids) */
	unsigned long *buf_pool_bmp_p;

	unsigned int outstanding_descs[WIFI_NRF_FMAC_AC_MAX];
	unsigned int curr_peer_opp[WIFI_NRF_FMAC_AC_MAX];
	unsigned int next_spare_desc_ac;

	struct tx_pkt_info *pkt_info_p;
	/* First fout bits Spare desc1 queue number,
	 * Second four bits Spare desc2 queue number
	 */
	unsigned int spare_desc_queue_map;
};


/**
 * struct wifi_nrf_fmac_priv - Structure to hold context information for the
 *                             UMAC IF layer.
 * @opriv: Pointer to the OS abstraction layer.
 * @hpriv: Pointer to the HAL layer.
 * @num_tx_tokens: Maximum number of tokens available for TX.
 * @num_tx_tokens_per_ac: Maximum number of TX tokens available reserved per
 *                        AC.
 * @num_tx_tokens_spare: Number of spare tokens available for TX.
 * @rx_buf_pools: RX buffer pool configuration data.
 * @rx_desc: Starting RX descriptor number for a RX buffer pool.
 * @num_rx_bufs: Maximum number of host buffers needed for RX frames.
 * @callbk_fns: Callback functions to be called on various events.
 *
 * This structure maintains the context information necessary for the
 * operation of the UMAC IF layer.
 */
struct wifi_nrf_fmac_priv {
	struct wifi_nrf_osal_priv *opriv;
	struct wifi_nrf_hal_priv *hpriv;

	struct nrf_wifi_data_config_params data_config;
	unsigned char num_tx_tokens;
	unsigned char num_tx_tokens_per_ac;
	unsigned char num_tx_tokens_spare;
	struct rx_buf_pool_params rx_buf_pools[MAX_NUM_OF_RX_QUEUES];
	unsigned int rx_desc[MAX_NUM_OF_RX_QUEUES];
	unsigned int num_rx_bufs;

	struct wifi_nrf_fmac_callbk_fns callbk_fns;

	bool twt_sleep_status;
	int last_tx_done_desc;
};


/**
 * struct wifi_nrf_fmac_dev_ctx - Structure to hold per device context information
 *                                for the UMAC IF layer.
 * @fpriv: Pointer to the UMAC IF abstraction layer.
 * @os_fmac_dev_ctx: Pointer to the per device OS context which is using the
 *               UMAC IF layer.
 * @hal_ctx: Pointer to the per device HAL context.
 * @vif_ctx: Array of pointers to virtual interfaces created on this device.
 * @tx_buf_info: Context information for a TX buffer.
 * @rx_buf_info: Context information for a RX buffer.
 * @mgmt_buf_info: Context information for a management buffer.
 * @tx_config: Context information related to TX path.
 * @stats_req: Flag indicating whether a request for statistics has been sent
 *             to the RPU.
 * @host_stats: Host statistics.
 * @pwr_req: Flag indicating whether a request for power monitoring data has
 *           been sent to the RPU;
 * @pwr_status: The status of the power monitor data request to the RPU.
 * @pwr_data_type: Power monitor data type requested from the RPU.
 * @pwr_data: Power monitor data received from the RPU.
 * @fw_stats: Firmware statistics.
 * @umac_ver: UMAC version information.
 * @lmac_ver: LMAC version information.
 * @num_sta: Present number of STAs created on the device.
 * @num_ap: Present number of APs created on the device.
 *
 * This structure maintains the context information necessary for the
 * a single instance of an FullMAC based RPU.
 */
struct wifi_nrf_fmac_dev_ctx {
	struct wifi_nrf_fmac_priv *fpriv;
	void *os_dev_ctx;
	void *hal_dev_ctx;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx[MAX_NUM_VIFS];
	struct wifi_nrf_fmac_buf_map_info *tx_buf_info;
	struct wifi_nrf_fmac_buf_map_info *rx_buf_info;
	struct tx_config tx_config;
	struct rpu_host_stats host_stats;
	unsigned char num_sta;
	unsigned char num_ap;
	struct rpu_fw_stats *fw_stats;
	bool stats_req;
	bool fw_boot_done;
	bool fw_init_done;
	bool fw_deinit_done;
	bool alpha2_valid;
	unsigned char alpha2[3];
};


/**
 * struct wifi_nrf_fmac_vif_ctx - Structure to hold per VIF context information
 *                                for the UMAC IF layer.
 * @fmac_dev_ctx: Pointer to the device context at the UMAC IF layer.
 * @os_vif_ctx: Pointer to the per VIF OS context which is using the
 *              UMAC IF layer.
 * @mac_addr: The mac address assigned to the VIF.
 * @groupwise_cipher: Groupwise cipher being used on this VIF.
 * @ifflags: Interface flags related to this VIF.
 * @if_type: Interface type of this VIF.
 * @bssid: BSSID of the AP to which this VIF is connected
 *         (applicable only in STA mode).
 *
 * This structure maintains the context information necessary for the
 * a single instance of an VIF.
 */
struct wifi_nrf_fmac_vif_ctx {
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx;
	void *os_vif_ctx;
	char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	int groupwise_cipher;
	bool ifflags;
	int if_type;
	unsigned char bssid[NRF_WIFI_ETH_ADDR_LEN];
};
#endif /* !CONFIG_NRF700X_RADIO_TEST */


struct wifi_nrf_fw_info {
	const void *data;
	unsigned int size;
};


/**
 * struct wifi_nrf_fmac_fw_info - Structure to hold firmware information
 *                                for the UMAC IF layer.
 * @lmac_ram: Information related to the LMAC RAM binary.
 * @umac_ram: Information related to the UMAC RAM binary.
 * @lmac_rom: Information related to the LMAC ROM binary.
 * @umac_rom: Information related to the UMAC ROM binary.
 * @lmac_patch_bimg: Information related to the LMAC BIMG patch binary.
 * @lmac_patch_bin: Information related to the LMAC BIN patch binary.
 * @umac_patch_bimg: Information related to the UMAC BIMG patch binary.
 * @umac_patch_bin: Information related to the UMAC BIN patch binary.
 *
 * This structure holds the UMAC and LMAC firmware information.
 */
struct wifi_nrf_fmac_fw_info {
	struct wifi_nrf_fw_info lmac_patch_pri;
	struct wifi_nrf_fw_info lmac_patch_sec;
	struct wifi_nrf_fw_info umac_patch_pri;
	struct wifi_nrf_fw_info umac_patch_sec;
};


struct wifi_nrf_fmac_otp_info {
	struct host_rpu_umac_info info;
	unsigned int flags;
};
#endif /* __FMAC_STRUCTS_H__ */
