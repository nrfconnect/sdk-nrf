/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @addtogroup nrf700x_fmac_api FMAC API
 * @{
 *
 * @brief Header containing declarations for utility functions for
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_STRUCTS_H__
#define __FMAC_STRUCTS_H__

#include <stdbool.h>

#include "osal_api.h"
#include "host_rpu_umac_if.h"
#include "fmac_structs_common.h"

#define MAX_PEERS 5
#define MAX_SW_PEERS (MAX_PEERS + 1)


/**
 * @brief WLAN access categories.
 *
 */
enum wifi_nrf_fmac_ac {
	/** Background access category. */
	WIFI_NRF_FMAC_AC_BK,
	/** Best-effort access category. */
	WIFI_NRF_FMAC_AC_BE,
	/** Video access category. */
	WIFI_NRF_FMAC_AC_VI,
	/** Voice access category. */
	WIFI_NRF_FMAC_AC_VO,
	/** Multicast access category. */
	WIFI_NRF_FMAC_AC_MC,
	/** Maximum number of WLAN access categories. */
	WIFI_NRF_FMAC_AC_MAX
};


/**
 * @brief The operational state of an interface.
 *
 */
enum wifi_nrf_fmac_if_op_state {
	/** Interface is non-operational. */
	WIFI_NRF_FMAC_IF_OP_STATE_DOWN,
	/** Interface is operational. */
	WIFI_NRF_FMAC_IF_OP_STATE_UP,
	/** Invalid value. Used for error checks. */
	WIFI_NRF_FMAC_IF_OP_STATE_INVALID
};


/**
 * @brief The carrier state of an interface.
 *
 */
enum wifi_nrf_fmac_if_carr_state {
	/** Interface is not ready. */
	WIFI_NRF_FMAC_IF_CARR_STATE_OFF,
	/** Interface is ready. */
	WIFI_NRF_FMAC_IF_CARR_STATE_ON,
	/** Invalid value. Used for error checks. */
	WIFI_NRF_FMAC_IF_CARR_STATE_INVALID
};


/**
 * @brief Callback functions to be invoked by UMAC IF layer when a particular event occurs.
 *
 * This structure contains function pointers to all the callback functions that
 * the UMAC IF layer needs to invoke for various events.
 */
struct wifi_nrf_fmac_callbk_fns {
	/** Callback function to be called when an interface association state changes. */
	enum wifi_nrf_status (*if_carr_state_chg_callbk_fn)(void *os_vif_ctx,
							    enum wifi_nrf_fmac_if_carr_state cs);

	/** Callback function to be called when a frame is received. */
	void (*rx_frm_callbk_fn)(void *os_vif_ctx,
				 void *frm);

	/** Callback function to be called when a scan is started. */
	void (*scan_start_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_umac_event_trigger_scan *scan_start_event,
				     unsigned int event_len);

	/** Callback function to be called when a scan is done. */
	void (*scan_done_callbk_fn)(void *os_vif_ctx,
				    struct nrf_wifi_umac_event_trigger_scan *scan_done_event,
				    unsigned int event_len);

	/** Callback function to be called when a scan is aborted. */
	void (*scan_abort_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_umac_event_trigger_scan *scan_done_event,
				     unsigned int event_len);

	/** Callback function to be called when a scan result is received. */
	void (*scan_res_callbk_fn)(void *os_vif_ctx,
				   struct nrf_wifi_umac_event_new_scan_results *scan_res,
				   unsigned int event_len,
				   bool more_res);

	/** Callback function to be called when a display scan result is received. */
	void (*disp_scan_res_callbk_fn)(void *os_vif_ctx,
				  struct nrf_wifi_umac_event_new_scan_display_results *scan_res,
				  unsigned int event_len,
				  bool more_res);

	/** Callback function to be called when an authentication response is received. */
	void (*auth_resp_callbk_fn)(void *os_vif_ctx,
				    struct nrf_wifi_umac_event_mlme *auth_resp_event,
				    unsigned int event_len);

	/** Callback function to be called when an association response is received. */
	void (*assoc_resp_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_umac_event_mlme *assoc_resp_event,
				     unsigned int event_len);

	/** Callback function to be called when a deauthentication frame is received. */
	void (*deauth_callbk_fn)(void *os_vif_ctx,
				 struct nrf_wifi_umac_event_mlme *deauth_event,
				 unsigned int event_len);

	/** Callback function to be called when a disassociation frame is received. */
	void (*disassoc_callbk_fn)(void *os_vif_ctx,
				   struct nrf_wifi_umac_event_mlme *disassoc_event,
				   unsigned int event_len);

	/** Callback function to be called when a management frame is received. */
	void (*mgmt_rx_callbk_fn)(void *os_vif_ctx,
				  struct nrf_wifi_umac_event_mlme *mgmt_rx_event,
				  unsigned int event_len);

	/** Callback function to be called when an unprotected management frame is received. */
	void (*unprot_mlme_mgmt_rx_callbk_fn)(void *os_vif_ctx,
					      struct nrf_wifi_umac_event_mlme *unprot_mlme_event,
					      unsigned int event_len);

	/** Callback function to be called when a get TX power response is received. */
	void (*tx_pwr_get_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_umac_event_get_tx_power *info,
				     unsigned int event_len);

	/** Callback function to be called when a get channel response is received. */
	void (*chnl_get_callbk_fn)(void *os_vif_ctx,
				   struct nrf_wifi_umac_event_get_channel *info,
				   unsigned int event_len);

	/** Callback function to be called when a cookie response is received. */
	void (*cookie_rsp_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_umac_event_cookie_rsp *cookie_rsp,
				     unsigned int event_len);

	/** Callback function to be called when a TX status is received. */
	void (*tx_status_callbk_fn)(void *os_vif_ctx,
				    struct nrf_wifi_umac_event_mlme *tx_status_event,
				    unsigned int event_len);

	/** Callback function to be called when a set interface response is received. */
	void (*set_if_callbk_fn)(void *os_vif_ctx,
				 struct nrf_wifi_umac_event_set_interface *set_if_event,
				 unsigned int event_len);

	/** Callback function to be called when a remain on channel response is received. */
	void (*roc_callbk_fn)(void *os_vif_ctx,
			      struct nrf_wifi_event_remain_on_channel *roc_event,
			      unsigned int event_len);

	/** Callback function to be called when a remain on channel cancel response is received. */
	void (*roc_cancel_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_event_remain_on_channel *roc_cancel_event,
				     unsigned int event_len);

	/** Callback function to be called when a get station response is received. */
	void (*get_station_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_umac_event_new_station *info,
				     unsigned int event_len);

	/** Callback function to be called when a get interface response is received. */
	void (*get_interface_callbk_fn)(void *os_vif_ctx,
				     struct nrf_wifi_interface_info *info,
				     unsigned int event_len);

	/** Callback function to be called when a management TX status is received. */
	void (*mgmt_tx_status)(void *if_priv,
					struct nrf_wifi_umac_event_mlme *mlme_event,
					unsigned int event_len);

	/** Callback function to be called when a TWT configuration response is received. */
	void (*twt_config_callbk_fn)(void *if_priv,
		struct nrf_wifi_umac_cmd_config_twt *twt_config_event_info,
		unsigned int event_len);

	/** Callback function to be called when a TWT teardown response is received. */
	void (*twt_teardown_callbk_fn)(void *if_priv,
		struct nrf_wifi_umac_cmd_teardown_twt *twt_teardown_event_info,
		unsigned int event_len);

	/** Callback function to be called when a get wiphy response is received. */
	void (*event_get_wiphy)(void *if_priv,
		struct nrf_wifi_event_get_wiphy *get_wiphy,
		unsigned int event_len);

	/** Callback function to be called when a TWT sleep response is received. */
	void (*twt_sleep_callbk_fn)(void *if_priv,
		struct nrf_wifi_umac_event_twt_sleep *twt_sleep_event_info,
		unsigned int event_len);

	/** Callback function to be called when a get regulatory response is received. */
	void (*event_get_reg)(void *if_priv,
		struct nrf_wifi_reg *get_reg,
		unsigned int event_len);

	/** Callback function to be called when a get power save information
	 * response is received.
	 */
	void (*event_get_ps_info)(void *if_priv,
		struct nrf_wifi_umac_event_power_save_info *get_ps_config,
		unsigned int event_len);

	/** Callback function to be called when a get connection info response is received. */
	void (*get_conn_info_callbk_fn)(void *os_vif_ctx,
					struct nrf_wifi_umac_event_conn_info *info,
					unsigned int event_len);

#if defined(CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS) || defined(__DOXYGEN__)
	/** Callback function to be called when a beacon/probe response is received. */
	void (*rx_bcn_prb_resp_callbk_fn)(void *os_vif_ctx,
					  void *frm,
					  unsigned short frequency,
					  signed short signal);
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */

	/** Callback function to be called when rssi is to be processed from the received frame. */
	void (*process_rssi_from_rx)(void *os_vif_ctx,
				     signed short signal);
};

/**
 * @brief Structure to hold TX/RX buffer pool configuration data.
 *
 */
struct wifi_nrf_fmac_buf_map_info {
	/** Flag indicating whether the buffer is mapped or not. */
	bool mapped;
	/** The number of words in the buffer. */
	unsigned long nwb;
};

/**
 * @brief Structure to hold peer context information.
 *
 * This structure holds context information for a peer that the RPU is
 * connected with.
 */
struct peers_info {
	/** Peer ID. */
	int peer_id;
	/** VIF index. */
	unsigned char if_idx;
	/** Power save state. */
	unsigned char ps_state;
	/** Legacy or HT/VHT/HE. */
	unsigned char is_legacy;
	/** QoS supported. */
	unsigned char qos_supported;
	/** Pending queue bitmap. */
	unsigned char pend_q_bmp;
	/** Receiver address. */
	unsigned char ra_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** Pairwise cipher. */
	unsigned int pairwise_cipher;
	/** 802.11 power save token count. */
	int ps_token_count;
};


/**
 * @brief Structure to hold transmit path context information.
 *
 */
struct tx_config {
	/** Lock used to make code portions in the TX path atomic. */
	void *tx_lock;
	/** Context information about peers that the RPU firmware is connected to. */
	struct peers_info peers[MAX_SW_PEERS];
	/** Coalesce count of TX frames. */
	unsigned int *send_pkt_coalesce_count_p;
	/** per-peer/per-AC Queue for frames waiting to be passed to the RPU firmware for TX. */
	void *data_pending_txq[MAX_SW_PEERS][WIFI_NRF_FMAC_AC_MAX];
	/** Queue for peers which have woken up from 802.11 power save. */
	void *wakeup_client_q;
	/** Used to store tx descs(buff pool ids). */
	unsigned long *buf_pool_bmp_p;
	/** TX descriptors which have been queued to the RPU firmware. */
	unsigned int outstanding_descs[WIFI_NRF_FMAC_AC_MAX];
	/** Peer who will be get the next opportunity for TX. */
	unsigned int curr_peer_opp[WIFI_NRF_FMAC_AC_MAX];
	/** Access category which will get the next spare descriptor. */
	unsigned int next_spare_desc_ac;
	/** Frame context information. */
	struct tx_pkt_info *pkt_info_p;
	/** Map for the spare descriptor queues
	 *  - First four bits : Spare desc1 queue number,
	 *  - Second four bits: Spare desc2 queue number.
	 */
	unsigned int spare_desc_queue_map;
#if defined(CONFIG_NRF700X_TX_DONE_WQ_ENABLED) || defined(__DOXYGEN__)
	/** Queue for TX done tasklet. */
	void *tx_done_tasklet_event_q;
#endif /* CONFIG_NRF700X_TX_DONE_WQ_ENABLED */
};


/**
 * @brief Structure to hold context information for the UMAC IF layer.
 *
 * This structure maintains the context information necessary for the
 * operation of the UMAC IF layer.
 */
struct wifi_nrf_fmac_priv {
	/** Handle to the OS abstraction layer. */
	struct wifi_nrf_osal_priv *opriv;
	/** Handle to the HAL layer. */
	struct wifi_nrf_hal_priv *hpriv;
	/** Data path configuration parameters. */
	struct nrf_wifi_data_config_params data_config;
	/** Maximum number of tokens available for TX. */
	unsigned char num_tx_tokens;
	/** Maximum number of TX tokens available reserved per AC. */
	unsigned char num_tx_tokens_per_ac;
	/** Number of spare tokens (common to all ACs) available for TX. */
	unsigned char num_tx_tokens_spare;
	/** RX buffer pool configuration data. */
	struct rx_buf_pool_params rx_buf_pools[MAX_NUM_OF_RX_QUEUES];
	/** Starting RX descriptor number for a RX buffer pool. */
	unsigned int rx_desc[MAX_NUM_OF_RX_QUEUES];
	/** Maximum number of host buffers needed for RX frames. */
	unsigned int num_rx_bufs;
	/** Maximum supported AMPDU length per token. */
	unsigned int max_ampdu_len_per_token;
	/** Available (remaining) AMPDU length per token. */
	unsigned int avail_ampdu_len_per_token;
	/** Callback functions to be called on various events. */
	struct wifi_nrf_fmac_callbk_fns callbk_fns;
};

/**
 * @brief The TWT sleep state of device.
 *
 */
enum wifi_nrf_fmac_twt_state {
	/** RPU in TWT sleep state. */
	WIFI_NRF_FMAC_TWT_STATE_SLEEP,
	/** RPU in TWT awake state. */
	WIFI_NRF_FMAC_TWT_STATE_AWAKE
};


/**
 * @brief Structure to hold per device context information for the UMAC IF layer.
 *
 * This structure maintains the context information necessary for the
 * a single instance of an FullMAC based RPU.
 */
struct wifi_nrf_fmac_dev_ctx {
	/** Handle to the FMAC IF abstraction layer. */
	struct wifi_nrf_fmac_priv *fpriv;
	/** Handle to the OS abstraction layer. */
	void *os_dev_ctx;
	/** Handle to the HAL layer. */
	void *hal_dev_ctx;
	/** Array of pointers to virtual interfaces created on this device. */
	struct wifi_nrf_fmac_vif_ctx *vif_ctx[MAX_NUM_VIFS];
	/** Queue for storing mapping info of TX buffers. */
	struct wifi_nrf_fmac_buf_map_info *tx_buf_info;
	/** Queue for storing mapping info of RX buffers. */
	struct wifi_nrf_fmac_buf_map_info *rx_buf_info;
	/** Context information related to TX path. */
	struct tx_config tx_config;
	/** Host statistics. */
	struct rpu_host_stats host_stats;
	/** Number of interfaces in STA mode. */
	unsigned char num_sta;
	/** Number of interfaces in AP mode. */
	unsigned char num_ap;
	/** Firmware statistics. */
	struct rpu_fw_stats *fw_stats;
	/** Firmware statistics requested. */
	bool stats_req;
	/** Firmware boot done. */
	bool fw_boot_done;
	/** Firmware init done. */
	bool fw_init_done;
	/** Firmware deinit done. */
	bool fw_deinit_done;
	/** Alpha2 valid. */
	bool alpha2_valid;
	/** Alpha2 country code, last byte is reserved for null character. */
	unsigned char alpha2[3];
	/** TWT state of the RPU. */
	enum wifi_nrf_fmac_twt_state twt_sleep_status;
#if defined(CONFIG_NRF700X_TX_DONE_WQ_ENABLED) || defined(__DOXYGEN__)
	/** Tasklet for TX done. */
	void *tx_done_tasklet;
#endif /* CONFIG_NRF700X_TX_DONE_WQ_ENABLED */
#if defined(CONFIG_NRF700X_RX_WQ_ENABLED) || defined(__DOXYGEN__)
	/** Tasklet for RX. */
	void *rx_tasklet;
	/** Queue for RX tasklet. */
	void *rx_tasklet_event_q;
#endif /* CONFIG_NRF700X_RX_WQ_ENABLED */
};


/**
 * @brief Structure to hold per VIF context information for the UMAC IF layer.
 *
 * This structure maintains the context information necessary for the
 * a single instance of an VIF.
 */
struct wifi_nrf_fmac_vif_ctx {
	/** Handle to the FMAC IF abstraction layer. */
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx;
	/** Handle to the OS abstraction layer. */
	void *os_vif_ctx;
	/** MAC address of the VIF. */
	char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** Groupwise cipher being used on this VIF. */
	int groupwise_cipher;
	/** Interface flags related to this VIF. */
	bool ifflags;
	/** Interface type of this VIF. */
	int if_type;
	/** BSSID of the AP to which this VIF is connected (applicable only in STA mode). */
	unsigned char bssid[NRF_WIFI_ETH_ADDR_LEN];
};

/**
 * @}
 */
#endif /* __FMAC_STRUCTS_H__ */
