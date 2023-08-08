/*
 *
 *Copyright (c) 2022 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Control interface between host and RPU
 */

#ifndef __HOST_RPU_UMAC_IF_H
#define __HOST_RPU_UMAC_IF_H

#include "host_rpu_data_if.h"
#include "host_rpu_sys_if.h"

#include "pack_def.h"

#define MAX_NRF_WIFI_UMAC_CMD_SIZE 400

/**
 * @brief The host can send the following commands to the RPU.
 *
 */
enum nrf_wifi_umac_commands {
	/** Trigger a new scan @ref nrf_wifi_umac_cmd_scan */
	NRF_WIFI_UMAC_CMD_TRIGGER_SCAN,
	/** Request for scan results @ref nrf_wifi_umac_cmd_get_scan_results */
	NRF_WIFI_UMAC_CMD_GET_SCAN_RESULTS,
	/** Send authentication request to AP @ref nrf_wifi_umac_cmd_auth */
	NRF_WIFI_UMAC_CMD_AUTHENTICATE,
	/** Send associate request to AP @ref nrf_wifi_umac_cmd_assoc */
	NRF_WIFI_UMAC_CMD_ASSOCIATE,
	/** Send deauthentication request to AP @ref nrf_wifi_umac_cmd_disconn */
	NRF_WIFI_UMAC_CMD_DEAUTHENTICATE,
	/** Set wiphy parameters @ref nrf_wifi_umac_cmd_set_wiphy */
	NRF_WIFI_UMAC_CMD_SET_WIPHY,
	/** Add new key @ref nrf_wifi_umac_cmd_key */
	NRF_WIFI_UMAC_CMD_NEW_KEY,
	/** Delete key @ref nrf_wifi_umac_cmd_key */
	NRF_WIFI_UMAC_CMD_DEL_KEY,
	/** Set default key to use @ref nrf_wifi_umac_cmd_set_key */
	NRF_WIFI_UMAC_CMD_SET_KEY,
	/** Unused */
	NRF_WIFI_UMAC_CMD_GET_KEY,
	/** Unused */
	NRF_WIFI_UMAC_CMD_NEW_BEACON,
	/** Change the beacon on an AP interface @ref nrf_wifi_umac_cmd_set_beacon */
	NRF_WIFI_UMAC_CMD_SET_BEACON,
	/** Set the BSS @ref nrf_wifi_umac_cmd_set_bss */
	NRF_WIFI_UMAC_CMD_SET_BSS,
	/** Start soft AP operation on an AP interface @ref nrf_wifi_umac_cmd_start_ap */
	NRF_WIFI_UMAC_CMD_START_AP,
	/** Stop soft AP operation @ref nrf_wifi_umac_cmd_stop_ap */
	NRF_WIFI_UMAC_CMD_STOP_AP,
	/** Create new interface @ref nrf_wifi_umac_cmd_add_vif */
	NRF_WIFI_UMAC_CMD_NEW_INTERFACE,
	/** Change interface configuration @ref nrf_wifi_umac_cmd_chg_vif_attr*/
	NRF_WIFI_UMAC_CMD_SET_INTERFACE,
	/** Delete interface @ref nrf_wifi_umac_cmd_del_vif */
	NRF_WIFI_UMAC_CMD_DEL_INTERFACE,
	/** Change interface flags @ref nrf_wifi_umac_cmd_chg_vif_state */
	NRF_WIFI_UMAC_CMD_SET_IFFLAGS,
	/** Add a new station @ref nrf_wifi_umac_cmd_add_sta */
	NRF_WIFI_UMAC_CMD_NEW_STATION,
	/** Delete station @ref nrf_wifi_umac_cmd_del_sta */
	NRF_WIFI_UMAC_CMD_DEL_STATION,
	/** Change station info @ref nrf_wifi_umac_cmd_chg_sta */
	NRF_WIFI_UMAC_CMD_SET_STATION,
	/** Get station info @ref nrf_wifi_umac_cmd_get_sta */
	NRF_WIFI_UMAC_CMD_GET_STATION,
	/** Start the P2P device @ref nrf_wifi_cmd_start_p2p */
	NRF_WIFI_UMAC_CMD_START_P2P_DEVICE,
	/** Stop the P2P device @ref nrf_wifi_umac_cmd_stop_p2p_dev */
	NRF_WIFI_UMAC_CMD_STOP_P2P_DEVICE,
	/** Remain awake on the specified channel @ref nrf_wifi_umac_cmd_remain_on_channel */
	NRF_WIFI_UMAC_CMD_REMAIN_ON_CHANNEL,
	/** Cancel a pending ROC duration @ref nrf_wifi_umac_cmd_cancel_remain_on_channel */
	NRF_WIFI_UMAC_CMD_CANCEL_REMAIN_ON_CHANNEL,
	/** Unused */
	NRF_WIFI_UMAC_CMD_SET_CHANNEL,
	/** Unused */
	NRF_WIFI_UMAC_CMD_RADAR_DETECT,
	/** Whitelist filter based on frame types @ref nrf_wifi_umac_cmd_mgmt_frame_reg */
	NRF_WIFI_UMAC_CMD_REGISTER_FRAME,
	/** Send a management frame @ref nrf_wifi_umac_cmd_mgmt_tx */
	NRF_WIFI_UMAC_CMD_FRAME,
	/** Unused */
	NRF_WIFI_UMAC_CMD_JOIN_IBSS,
	/** Unused */
	NRF_WIFI_UMAC_CMD_WIN_STA_CONNECT,
	/** Power save Enable/Disable @ref nrf_wifi_umac_cmd_set_power_save */
	NRF_WIFI_UMAC_CMD_SET_POWER_SAVE,
	/** Unused */
	NRF_WIFI_UMAC_CMD_SET_WOWLAN,
	/** Unused */
	NRF_WIFI_UMAC_CMD_SUSPEND,
	/** Unused */
	NRF_WIFI_UMAC_CMD_RESUME,
	/** QOS map @ref nrf_wifi_umac_cmd_set_qos_map */
	NRF_WIFI_UMAC_CMD_SET_QOS_MAP,
	/** Get Channel info @ref nrf_wifi_umac_cmd_get_channel */
	NRF_WIFI_UMAC_CMD_GET_CHANNEL,
	/** Get Tx power level @ref nrf_wifi_umac_cmd_get_tx_power */
	NRF_WIFI_UMAC_CMD_GET_TX_POWER,
	/** Get interface @ref nrf_wifi_cmd_get_interface */
	NRF_WIFI_UMAC_CMD_GET_INTERFACE,
	/** Get Wiphy info @ref nrf_wifi_cmd_get_wiphy */
	NRF_WIFI_UMAC_CMD_GET_WIPHY,
	/** Get hardware address @ref nrf_wifi_cmd_get_ifhwaddr */
	NRF_WIFI_UMAC_CMD_GET_IFHWADDR,
	/** Set hardware address @ref nrf_wifi_cmd_set_ifhwaddr */
	NRF_WIFI_UMAC_CMD_SET_IFHWADDR,
	/** Get regulatory domain @ref nrf_wifi_umac_cmd_get_reg */
	NRF_WIFI_UMAC_CMD_GET_REG,
	/** Unused */
	NRF_WIFI_UMAC_CMD_SET_REG,
	/** Set regulatory domain @ref  nrf_wifi_cmd_req_set_reg */
	NRF_WIFI_UMAC_CMD_REQ_SET_REG,
	/** Config UAPSD @ref nrf_wifi_umac_cmd_config_uapsd */
	NRF_WIFI_UMAC_CMD_CONFIG_UAPSD,
	/** Config TWT @ref nrf_wifi_umac_cmd_config_twt */
	NRF_WIFI_UMAC_CMD_CONFIG_TWT,
	/** Teardown TWT @ref nrf_wifi_umac_cmd_teardown_twt */
	NRF_WIFI_UMAC_CMD_TEARDOWN_TWT,
	/** Abort scan @ref nrf_wifi_umac_cmd_abort_scan */
	NRF_WIFI_UMAC_CMD_ABORT_SCAN,
	/** Multicast filter @ref nrf_wifi_umac_cmd_mcast_filter */
	NRF_WIFI_UMAC_CMD_MCAST_FILTER,
	/** Change macaddress @ref nrf_wifi_umac_cmd_change_macaddr */
	NRF_WIFI_UMAC_CMD_CHANGE_MACADDR,
	/** Set powersave timeout @ref nrf_wifi_umac_cmd_set_power_save_timeout */
	NRF_WIFI_UMAC_CMD_SET_POWER_SAVE_TIMEOUT,
	/** Get connection information @ref nrf_wifi_umac_cmd_conn_info */
	NRF_WIFI_UMAC_CMD_GET_CONNECTION_INFO,
	/** Get power save information @ref nrf_wifi_umac_cmd_get_power_save_info */
	NRF_WIFI_UMAC_CMD_GET_POWER_SAVE_INFO,
	/** Set listen interval @ref nrf_wifi_umac_cmd_set_listen_interval */
	NRF_WIFI_UMAC_CMD_SET_LISTEN_INTERVAL,
	/** Configure extended power save @ref nrf_wifi_umac_cmd_config_extended_ps */
	NRF_WIFI_UMAC_CMD_CONFIG_EXTENDED_PS
};

 /**
  * @brief The host can receive the following events from the RPU.
  *
  */

enum nrf_wifi_umac_events {
	NRF_WIFI_UMAC_EVENT_UNSPECIFIED = 256,
	/** Indicate scan started @ref nrf_wifi_umac_event_trigger_scan */
	NRF_WIFI_UMAC_EVENT_TRIGGER_SCAN_START,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_SCAN_ABORTED,
	/** Indicate scan done @ref nrf_wifi_umac_event_scan_done */
	NRF_WIFI_UMAC_EVENT_SCAN_DONE,
	/** Scan result event @ref nrf_wifi_umac_event_new_scan_results */
	NRF_WIFI_UMAC_EVENT_SCAN_RESULT,
	/** Authentication status @ref nrf_wifi_umac_event_mlme */
	NRF_WIFI_UMAC_EVENT_AUTHENTICATE,
	/** Association status @ref nrf_wifi_umac_event_mlme*/
	NRF_WIFI_UMAC_EVENT_ASSOCIATE,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_CONNECT,
	/** Station deauth event @ref nrf_wifi_umac_event_mlme */
	NRF_WIFI_UMAC_EVENT_DEAUTHENTICATE,
	/** Station disassoc event @ref nrf_wifi_umac_event_mlme */
	NRF_WIFI_UMAC_EVENT_DISASSOCIATE,
	/** Station added indication @ref nrf_wifi_umac_event_new_station */
	NRF_WIFI_UMAC_EVENT_NEW_STATION,
	/** Station added indication @ref nrf_wifi_umac_event_new_station */
	NRF_WIFI_UMAC_EVENT_DEL_STATION,
	/** Station info indication @ref nrf_wifi_umac_event_new_station */
	NRF_WIFI_UMAC_EVENT_GET_STATION,
	/** remain on channel event @ref nrf_wifi_event_remain_on_channel */
	NRF_WIFI_UMAC_EVENT_REMAIN_ON_CHANNEL,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_CANCEL_REMAIN_ON_CHANNEL,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_DISCONNECT,
	/** RX management frame @ref nrf_wifi_umac_event_mlme */
	NRF_WIFI_UMAC_EVENT_FRAME,
	/** Cookie mapping for NRF_WIFI_UMAC_CMD_FRAME @ref nrf_wifi_umac_event_cookie_rsp */
	NRF_WIFI_UMAC_EVENT_COOKIE_RESP,
	/** TX management frame transmitted @ref nrf_wifi_umac_event_mlme */
	NRF_WIFI_UMAC_EVENT_FRAME_TX_STATUS,
	/** @ref nrf_wifi_umac_event_vif_state */
	NRF_WIFI_UMAC_EVENT_IFFLAGS_STATUS,
	/** Send Tx power @ref nrf_wifi_umac_event_get_tx_power */
	NRF_WIFI_UMAC_EVENT_GET_TX_POWER,
	/** Send Channel info @ref nrf_wifi_umac_event_get_channel */
	NRF_WIFI_UMAC_EVENT_GET_CHANNEL,
	/** @ref nrf_wifi_umac_event_set_interface */
	NRF_WIFI_UMAC_EVENT_SET_INTERFACE,
	/** @ref nrf_wifi_umac_event_mlme */
	NRF_WIFI_UMAC_EVENT_UNPROT_DEAUTHENTICATE,
	/** @ref nrf_wifi_umac_event_mlme */
	NRF_WIFI_UMAC_EVENT_UNPROT_DISASSOCIATE,
	/** @ref nrf_wifi_interface_info */
	NRF_WIFI_UMAC_EVENT_NEW_INTERFACE,
	/** @ref nrf_wifi_event_get_wiphy */
	NRF_WIFI_UMAC_EVENT_NEW_WIPHY,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_GET_IFHWADDR,
	/** Get regulatory @ref nrf_wifi_reg */
	NRF_WIFI_UMAC_EVENT_GET_REG,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_SET_REG,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_REQ_SET_REG,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_GET_KEY,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_BEACON_HINT,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_REG_CHANGE,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_WIPHY_REG_CHANGE,
	/** Display scan result @ref nrf_wifi_umac_event_new_scan_display_results */
	NRF_WIFI_UMAC_EVENT_SCAN_DISPLAY_RESULT,
	/** @ref nrf_wifi_umac_event_cmd_status */
	NRF_WIFI_UMAC_EVENT_CMD_STATUS,
	/** @ref nrf_wifi_umac_event_new_scan_results */
	NRF_WIFI_UMAC_EVENT_BSS_INFO,
	/** Send TWT response information @ref nrf_wifi_umac_cmd_config_twt */
	NRF_WIFI_UMAC_EVENT_CONFIG_TWT,
	/** Send TWT teardown information @ref nrf_wifi_umac_cmd_teardown_twt */
	NRF_WIFI_UMAC_EVENT_TEARDOWN_TWT,
	/** Send block or unblock state @ref nrf_wifi_umac_event_twt_sleep */
	NRF_WIFI_UMAC_EVENT_TWT_SLEEP,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_COALESCING,
	/** Unused */
	NRF_WIFI_UMAC_EVENT_MCAST_FILTER,
	/** send connection information @ref nrf_wifi_umac_event_conn_info. */
	NRF_WIFI_UMAC_EVENT_GET_CONNECTION_INFO,
	/** @ref nrf_wifi_umac_event_power_save_info */
	NRF_WIFI_UMAC_EVENT_GET_POWER_SAVE_INFO
};

/**
 * @brief Represents the values that can be used to specify the frequency band.
 *
 */

enum nrf_wifi_band {
	/** 2.4 GHz ISM band */
	NRF_WIFI_BAND_2GHZ,
	/** Around 5 GHz band (4.9 - 5.7 GHz) */
	NRF_WIFI_BAND_5GHZ,
	/** Unused */
	NRF_WIFI_BAND_60GHZ,
};

/**
 * @brief Enable or Disable Management Frame Protection.
 *
 */
enum nrf_wifi_mfp {
	/** Management frame protection not used */
	NRF_WIFI_MFP_NO,
	/** Management frame protection required */
	NRF_WIFI_MFP_REQUIRED,
};

/**
 * @brief Enumerates the various categories of security keys.
 *
 */
enum nrf_wifi_key_type {
	/** Group (broadcast/multicast) key */
	NRF_WIFI_KEYTYPE_GROUP,
	/** Pairwise (unicast/individual) key */
	NRF_WIFI_KEYTYPE_PAIRWISE,
	/** Peer key (DLS) */
	NRF_WIFI_KEYTYPE_PEERKEY,
	/** Number of defined key types */
	NUM_NRF_WIFI_KEYTYPES
};

/**
 * @brief Enumerates the various types of authentication mechanisms.
 *
 */
enum nrf_wifi_auth_type {
	/** Open System authentication */
	NRF_WIFI_AUTHTYPE_OPEN_SYSTEM,
	/** Shared Key authentication (WEP only) */
	NRF_WIFI_AUTHTYPE_SHARED_KEY,
	/** Fast BSS Transition (IEEE 802.11r) */
	NRF_WIFI_AUTHTYPE_FT,
	/** Network EAP (some Cisco APs and mainly LEAP) */
	NRF_WIFI_AUTHTYPE_NETWORK_EAP,
	/** Simultaneous authentication of equals */
	NRF_WIFI_AUTHTYPE_SAE,
	/** Internal */
	__NRF_WIFI_AUTHTYPE_NUM,
	/** Maximum valid auth algorithm */
	NRF_WIFI_AUTHTYPE_MAX = __NRF_WIFI_AUTHTYPE_NUM,
	/** Determine automatically (if necessary by trying multiple times) */
	NRF_WIFI_AUTHTYPE_AUTOMATIC
};

/**
 * @brief Represents the interface's status concerning this BSS (Basic Service Set).
 *
 */
enum nrf_wifi_bss_status {
	/**
	 * Authenticated with this BSS
	 * Note that this is no longer used since cfg80211 no longer
	 * keeps track of whether or not authentication was done with
	 * a given BSS.
	 */
	NRF_WIFI_BSS_STATUS_AUTHENTICATED,
	/** Associated with this BSS */
	NRF_WIFI_BSS_STATUS_ASSOCIATED,
	/** Joined to this IBSS */
	NRF_WIFI_BSS_STATUS_IBSS_JOINED,
};

/**
 * @brief Enumerates the various categories of channels.
 *
 */
enum nrf_wifi_channel_type {
	/** 20 MHz, non-HT channel */
	NRF_WIFI_CHAN_NO_HT,
	/** 20 MHz HT channel */
	NRF_WIFI_CHAN_HT20,
	/** HT40 channel, secondary channel below the control channel */
	NRF_WIFI_CHAN_HT40MINUS,
	/** HT40 channel, secondary channel above the control channel */
	NRF_WIFI_CHAN_HT40PLUS
};

/**
 * @brief Enumerates the various channel widths available.
 *
 */
enum nrf_wifi_chan_width {
	/** 20 MHz, non-HT channel */
	NRF_WIFI_CHAN_WIDTH_20_NOHT,
	/** 20 MHz HT channel */
	NRF_WIFI_CHAN_WIDTH_20,
	/** 40 MHz channel, the @ref %NRF_WIFI_ATTR_CENTER_FREQ1 must be provided as well */
	NRF_WIFI_CHAN_WIDTH_40,
	/** 80 MHz channel, the @ref %NRF_WIFI_ATTR_CENTER_FREQ1 must be provided as well */
	NRF_WIFI_CHAN_WIDTH_80,
	/** 80+80 MHz channel, the @ref %NRF_WIFI_ATTR_CENTER_FREQ1 and
	 *  @ref %NRF_WIFI_ATTR_CENTER_FREQ2 must be provided as well
	 */
	NRF_WIFI_CHAN_WIDTH_80P80,
	/** 160 MHz channel, the %NRF_WIFI_ATTR_CENTER_FREQ1 must be provided as well */
	NRF_WIFI_CHAN_WIDTH_160,
	/**  5 MHz OFDM channel */
	NRF_WIFI_CHAN_WIDTH_5,
	/** 10 MHz OFDM channel */
	NRF_WIFI_CHAN_WIDTH_10,
};

/**
 * @brief Interface types based on functionality.
 *
 */
enum nrf_wifi_iftype {
	/** Unspecified type, driver decides */
	NRF_WIFI_IFTYPE_UNSPECIFIED,
	/** Not Supported */
	NRF_WIFI_IFTYPE_ADHOC,
	/** Managed BSS member */
	NRF_WIFI_IFTYPE_STATION,
	/** Access point */
	NRF_WIFI_IFTYPE_AP,
	/** Not Supported */
	NRF_WIFI_IFTYPE_AP_VLAN,
	/** Not Supported */
	NRF_WIFI_IFTYPE_WDS,
	/** Not Supported */
	NRF_WIFI_IFTYPE_MONITOR,
	/** Not Supported */
	NRF_WIFI_IFTYPE_MESH_POINT,
	/** P2P client */
	NRF_WIFI_IFTYPE_P2P_CLIENT,
	/** P2P group owner */
	NRF_WIFI_IFTYPE_P2P_GO,
	/** P2P device use the @ref %NRF_WIFI_UMAC_CMD_START_P2P_DEVICE &
	 *  @ref %NRF_WIFI_UMAC_CMD_STOP_P2P_DEVICE commands to create and destroy one
	 */
	NRF_WIFI_IFTYPE_P2P_DEVICE,
	/** Not Supported */
	NRF_WIFI_IFTYPE_OCB,
	/** Highest interface type number currently defined */
	NUM_NRF_WIFI_IFTYPES,
	/** Number of defined interface types */
	NRF_WIFI_IFTYPE_MAX = NUM_NRF_WIFI_IFTYPES - 1
};

/**
 * @brief Powersave state.
 *
 */
enum nrf_wifi_ps_state {
	/** powersave is disabled */
	NRF_WIFI_PS_DISABLED,
	/** powersave is enabled */
	NRF_WIFI_PS_ENABLED,
};

/**
 * @brief WLAN security types.
 *
 */
enum nrf_wifi_security_type {
	/** OPEN */
	NRF_WIFI_OPEN,
	/** WEP */
	NRF_WIFI_WEP,
	/** WPA */
	NRF_WIFI_WPA,
	/** WPA2 */
	NRF_WIFI_WPA2,
	/** WPA3 */
	NRF_WIFI_WPA3,
	/** WAPI */
	NRF_WIFI_WAPI,
	/** Enterprise mode */
	NRF_WIFI_EAP,
	/** WPA2 sha 256 */
	NRF_WIFI_WPA2_256
};

/**
 * @brief Denotes the originator of a regulatory domain request.
 *
 */
enum nrf_wifi_reg_initiator {
	/** Core queried CRDA for a dynamic world regulatory domain */
	NRF_WIFI_REGDOM_SET_BY_CORE,
	/** User asked the wireless core to set the regulatory domain */
	NRF_WIFI_REGDOM_SET_BY_USER,
	/**
	 * A wireless drivers has hinted to the wireless core it thinks
	 * its knows the regulatory domain we should be in
	 */
	NRF_WIFI_REGDOM_SET_BY_DRIVER,
	/**
	 * the wireless core has received an
	 * 802.11 country information element with regulatory information it
	 * thinks we should consider. cfg80211 only processes the country
	 * code from the IE, and relies on the regulatory domain information
	 * structure passed by userspace (CRDA) from our wireless-regdb
	 * If a channel is enabled but the country code indicates it should
	 * be disabled we disable the channel and re-enable it upon disassociation
	 */
	NRF_WIFI_REGDOM_SET_BY_COUNTRY_IE,
};

/**
 * @brief Specifies the type of regulatory domain.
 *
 */
enum nrf_wifi_reg_type {
	/**
	 * the regulatory domain set is one that pertains
	 * to a specific country. When this is set you can count on the
	 * ISO / IEC 3166 alpha2 country code being valid.
	 *
	 */
	NRF_WIFI_REGDOM_TYPE_COUNTRY,
	/** the regulatory set domain is the world regulatory domain */
	NRF_WIFI_REGDOM_TYPE_WORLD,
	/**
	 * the regulatory domain set is a custom
	 * driver specific world regulatory domain. These do not apply system-wide
	 * and are only applicable to the individual devices which have requested
	 * them to be applied.
	 */
	NRF_WIFI_REGDOM_TYPE_CUSTOM_WORLD,
	/**
	 * the regulatory domain set is the product
	 * of an intersection between two regulatory domains -- the previously
	 * set regulatory domain on the system and the last accepted regulatory
	 * domain request to be processed.
	 */
	NRF_WIFI_REGDOM_TYPE_INTERSECTION,
};

#define NRF_WIFI_MAX_SSID_LEN 32

/**
 * @brief This structure provides details about the SSID.
 *
 */

struct nrf_wifi_ssid {
	/** length of SSID */
	unsigned char nrf_wifi_ssid_len;
	/** SSID string */
	unsigned char nrf_wifi_ssid[NRF_WIFI_MAX_SSID_LEN];
} __NRF_WIFI_PKD;

#define NRF_WIFI_MAX_IE_LEN 400

/**
 * @brief This structure contains data related to the Information Elements (IEs).
 *
 */

struct nrf_wifi_ie {
	/** length of IE */
	unsigned short ie_len;
	/** Information element data */
	signed char ie[NRF_WIFI_MAX_IE_LEN];
} __NRF_WIFI_PKD;

#define NRF_WIFI_MAX_SEQ_LENGTH 256

/**
 * @brief Transmit key sequence number.
 *
 */

struct nrf_wifi_seq {
	/** Length of the seq parameter */
	signed int nrf_wifi_seq_len;
	/** Key sequence number data */
	unsigned char nrf_wifi_seq[NRF_WIFI_MAX_SEQ_LENGTH];
} __NRF_WIFI_PKD;

#define NRF_WIFI_MAX_KEY_LENGTH 256

/**
 * @brief This structure holds information related to a security key.
 *
 */

struct nrf_wifi_key {
	/** Length of the key data */
	unsigned int nrf_wifi_key_len;
	/** Key data */
	unsigned char nrf_wifi_key[NRF_WIFI_MAX_KEY_LENGTH];
} __NRF_WIFI_PKD;

#define NRF_WIFI_MAX_SAE_DATA_LENGTH 256

/**
 * @brief This structure represents SAE elements in Authentication frame.
 *
 */

struct nrf_wifi_sae {
	/** Length of SAE element data */
	signed int sae_data_len;
	/** SAE element data */
	unsigned char sae_data[NRF_WIFI_MAX_SAE_DATA_LENGTH];
} __NRF_WIFI_PKD;

#define NRF_WIFI_MAX_FRAME_LEN 400

/**
 * @brief This structure defines the frame that is intended for transmission.
 *
 */

struct nrf_wifi_frame {
	/** Length of the frame  */
	signed int frame_len;
	/** frame data */
	signed char frame[NRF_WIFI_MAX_FRAME_LEN];
} __NRF_WIFI_PKD;

#define NRF_WIFI_INDEX_IDS_WDEV_ID_VALID (1 << 0)
#define NRF_WIFI_INDEX_IDS_IFINDEX_VALID (1 << 1)
#define NRF_WIFI_INDEX_IDS_WIPHY_IDX_VALID (1 << 2)

/**
 * @brief This structure contains details about the interface information.
 *
 */

struct nrf_wifi_index_ids {
	/** Indicate which properties below are set */
	unsigned int valid_fields;
	/** wdev id */
	signed int ifaceindex;
	/** Unused */
	signed int nrf_wifi_wiphy_idx;
	/** Unused */
	unsigned long long wdev_id;
} __NRF_WIFI_PKD;

#define NRF_WIFI_SUPP_RATES_BAND_VALID (1 << 0)
#define NRF_WIFI_MAX_SUPP_RATES 60

/**
 * @brief This structure provides information about the rate parameters.
 *
 */

struct nrf_wifi_supp_rates {
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Frequency band, see &enum nrf_wifi_band */
	signed int band;
	/** Number of values in rates parameter */
	signed int nrf_wifi_num_rates;
	/** List of supported rates as defined by IEEE 802.11 7.3.2.2 */
	unsigned char rates[NRF_WIFI_MAX_SUPP_RATES];
} __NRF_WIFI_PKD;

/**
 * @brief This structure contains details about a channel's information.
 *
 */
struct nrf_wifi_channel {
	/** band this channel belongs to */
	signed int band;
	/** center frequency in MHz */
	unsigned int center_frequency;
	/** channel flags from see &enum nrf_wifi_channel_flags */
	unsigned int nrf_wifi_flags;
	/** maximum antenna gain in dBi */
	signed int nrf_wifi_max_antenna_gain;
	/** maximum transmission power (in dBm) */
	signed int nrf_wifi_max_power;
	/** maximum regulatory transmission power (in dBm) */
	signed int nrf_wifi_max_reg_power;
	/** channel flags at registration time, used by regulatory
	 *  code to support devices with additional restrictions
	 */
	unsigned int nrf_wifi_orig_flags;
	/** internal use */
	signed int nrf_wifi_orig_mag;
	/** internal use */
	signed int nrf_wifi_orig_mpwr;
	/** hardware-specific value for the channel */
	unsigned short hw_value;
	/** helper to regulatory code to indicate when a beacon
	 *  has been found on this channel. Use regulatory_hint_found_beacon()
	 *  to enable this, this is useful only on 5 GHz band.
	 */
	signed char nrf_wifi_beacon_found;
} __NRF_WIFI_PKD;

#define NRF_WIFI_SCAN_PARAMS_2GHZ_BAND_VALID (1 << 0)
#define NRF_WIFI_SCAN_PARAMS_5GHZ_BAND_VALID (1 << 1)
#define NRF_WIFI_SCAN_PARAMS_60GHZ_BAND_VALID (1 << 2)
#define NRF_WIFI_SCAN_PARAMS_MAC_ADDR_VALID (1 << 3)
#define NRF_WIFI_SCAN_PARAMS_MAC_ADDR_MASK_VALID (1 << 4)
#define NRF_WIFI_SCAN_PARAMS_SCAN_FLAGS_VALID (1 << 5)
#define NRF_WIFI_SCAN_PARAMS_SUPPORTED_RATES_VALID (1 << 6)

#define NRF_WIFI_SCAN_MAX_NUM_SSIDS 2
#define NRF_WIFI_SCAN_MAX_NUM_FREQUENCIES 64
#define MAX_NUM_CHANNELS 42

#define NRF_WIFI_SCAN_FLAG_LOW_PRIORITY (1 << 0)
#define NRF_WIFI_SCAN_FLAG_FLUSH (1 << 1)
#define NRF_WIFI_SCAN_FLAG_AP (1 << 2)
#define NRF_WIFI_SCAN_FLAG_RANDOM_ADDR (1 << 3)
#define NRF_WIFI_SCAN_FLAG_FILS_MAX_CHANNEL_TIME (1 << 4)
#define NRF_WIFI_SCAN_FLAG_ACCEPT_BCAST_PROBE_RESP (1 << 5)
#define NRF_WIFI_SCAN_FLAG_OCE_PROBE_REQ_HIGH_TX_RATE (1 << 6)
#define NRF_WIFI_SCAN_FLAG_OCE_PROBE_REQ_DEFERRAL_SUPPRESSION (1 << 7)
#define NRF_WIFI_SCAN_FLAG_LOW_SPAN (1 << 8)
#define NRF_WIFI_SCAN_FLAG_LOW_POWER (1 << 9)
#define NRF_WIFI_SCAN_FLAG_HIGH_ACCURACY (1 << 10)
#define NRF_WIFI_SCAN_FLAG_RANDOM_SN (1 << 11)
#define NRF_WIFI_SCAN_FLAG_MIN_PREQ_CONTENT (1 << 12)

/**
 * @brief This structure provides details about the parameters required for a scan request.
 *
 */

struct nrf_wifi_scan_params {
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Number of ssids valid in scan_ssids parameter */
	unsigned char num_scan_ssids;
	/** Number of channels to be scanned */
	unsigned char num_scan_channels;
	/** Scan request control flags (u32). Bit values
	 *  (NRF_WIFI_SCAN_FLAG_LOW_PRIORITY/NRF_WIFI_SCAN_FLAG_RANDOM_ADDR...)
	 */
	unsigned int scan_flags;
	/** ssid info @ref nrf_wifi_ssid */
	struct nrf_wifi_ssid scan_ssids[NRF_WIFI_SCAN_MAX_NUM_SSIDS];
	/** Information element(s) data @ref nrf_wifi_ie*/
	struct nrf_wifi_ie ie;
	/** Supported rates @ref nrf_wifi_supp_rates */
	struct nrf_wifi_supp_rates supp_rates;
	/** MAC address */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** MAC address mask */
	unsigned char mac_addr_mask[NRF_WIFI_ETH_ADDR_LEN];
	/** used to send probe requests at non CCK rate in 2GHz band */
	unsigned char no_cck;
	/** Operating channel duration when STA is connected to AP */
	unsigned short oper_ch_duration;
	/** Max scan duration in TU */
	unsigned short scan_duration[MAX_NUM_CHANNELS];
	/** Max probe count in channels */
	unsigned char probe_cnt[MAX_NUM_CHANNELS];
	/** channels to be scanned @ref nrf_wifi_channel */
	struct nrf_wifi_channel channels[0];
} __NRF_WIFI_PKD;

#define NRF_WIFI_HT_CAPABILITY_VALID (1 << 0)
#define NRF_WIFI_HT_CAPABILITY_MASK_VALID (1 << 1)
#define NRF_WIFI_VHT_CAPABILITY_VALID (1 << 2)
#define NRF_WIFI_VHT_CAPABILITY_MASK_VALID (1 << 3)

#define NRF_WIFI_CMD_HT_VHT_CAPABILITY_DISABLE_HT (1 << 0)
#define NRF_WIFI_HT_VHT_CAPABILITY_MAX_SIZE 256

/**
 * @brief This structure contains specific information about the VHT (Very High Throughput)
 *  and HT ((High Throughput)) capabilities.
 *
 */

struct nrf_wifi_ht_vht_capabilities {
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Indicate which capabilities have been specified */
	unsigned short nrf_wifi_flags;
	/** HT Capability information element (from association request when
	 *  used with NRF_WIFI_UMAC_CMD_NEW_STATION).
	 */
	unsigned char ht_capability[NRF_WIFI_HT_VHT_CAPABILITY_MAX_SIZE];
	/** Specify which bits of the ht_capability are masked */
	unsigned char ht_capability_mask[NRF_WIFI_HT_VHT_CAPABILITY_MAX_SIZE];
	/** VHT Capability information element */
	unsigned char vht_capability[NRF_WIFI_HT_VHT_CAPABILITY_MAX_SIZE];
	/** Specify which bits in vht_capability to which attention should be paid */
	unsigned char vht_capability_mask[NRF_WIFI_HT_VHT_CAPABILITY_MAX_SIZE];
} __NRF_WIFI_PKD;

#define NRF_WIFI_SIGNAL_TYPE_NONE 1
#define NRF_WIFI_SIGNAL_TYPE_MBM 2
#define NRF_WIFI_SIGNAL_TYPE_UNSPEC 3

/**
 * @brief This structure represents information related to the signal strength.
 *
 */

struct nrf_wifi_signal {
	/** MBM or unspecified */
	unsigned int signal_type;

	/** signal */
	union {
		/** If MBM signal strength of probe response/beacon
		 *  in mBm (100 * dBm) (s32)
		 */
		unsigned int mbm_signal;
		/** If unspecified signal strength of the probe response/beacon
		 *  in unspecified units, scaled to 0..100 (u8).
		 */
		unsigned char unspec_signal;
	} __NRF_WIFI_PKD signal;
} __NRF_WIFI_PKD;

#define NRF_WIFI_WPA_VERSION_1 (1 << 0)
#define NRF_WIFI_WPA_VERSION_2 (1 << 1)

#define NRF_WIFI_CONNECT_COMMON_INFO_MAC_ADDR_VALID (1 << 0)
#define NRF_WIFI_CONNECT_COMMON_INFO_MAC_ADDR_HINT_VALID (1 << 1)
#define NRF_WIFI_CONNECT_COMMON_INFO_FREQ_VALID (1 << 2)
#define NRF_WIFI_CONNECT_COMMON_INFO_FREQ_HINT_VALID (1 << 3)
#define NRF_WIFI_CONNECT_COMMON_INFO_BG_SCAN_PERIOD_VALID (1 << 4)
#define NRF_WIFI_CONNECT_COMMON_INFO_SSID_VALID (1 << 5)
#define NRF_WIFI_CONNECT_COMMON_INFO_WPA_IE_VALID (1 << 6)
#define NRF_WIFI_CONNECT_COMMON_INFO_WPA_VERSIONS_VALID (1 << 7)
#define NRF_WIFI_CONNECT_COMMON_INFO_CIPHER_SUITES_PAIRWISE_VALID (1 << 8)
#define NRF_WIFI_CONNECT_COMMON_INFO_CIPHER_SUITE_GROUP_VALID (1 << 9)
#define NRF_WIFI_CONNECT_COMMON_INFO_AKM_SUITES_VALID (1 << 10)
#define NRF_WIFI_CONNECT_COMMON_INFO_USE_MFP_VALID (1 << 11)
#define NRF_WIFI_CONNECT_COMMON_INFO_CONTROL_PORT_ETHER_TYPE (1 << 12)
#define NRF_WIFI_CONNECT_COMMON_INFO_CONTROL_PORT_NO_ENCRYPT (1 << 13)

#define NRF_WIFI_MAX_NR_AKM_SUITES 2

#define NRF_WIFI_CMD_CONNECT_COMMON_INFO_USE_RRM (1 << 14)
#define NRF_WIFI_CONNECT_COMMON_INFO_PREV_BSSID (1 << 15)

/**
 * @brief This structure contains parameters related to the connection.
 *
 */

struct nrf_wifi_connect_common_info {
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Frequency of the selected channel in MHz */
	unsigned int frequency;
	/** Frequency of the recommended initial BSS */
	unsigned int freq_hint;
	/** Indicates which WPA version(s) */
	unsigned int wpa_versions;
	/** Number of pairwise cipher suites */
	signed int num_cipher_suites_pairwise;
	/** For crypto settings, indicates which pairwise cipher suites are used */
	unsigned int cipher_suites_pairwise[7];
	/** For crypto settings, indicates which group cipher suite is used */
	unsigned int cipher_suite_group;
	/** Number of groupwise cipher suites */
	unsigned int num_akm_suites;
	/** Indicate which key management algorithm(s) to use */
	unsigned int akm_suites[NRF_WIFI_MAX_NR_AKM_SUITES];
	/** Whether management frame protection (IEEE 802.11w) is used for the association */
	signed int use_mfp;
	/** Flag for indicating whether the current connection
	 *  shall support Radio Resource Measurements (11k)
	 */
	unsigned int nrf_wifi_flags;
	/** Background scan period in seconds or 0 to disable background scan */
	unsigned short bg_scan_period;
	/** MAC address */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** MAC address recommendation as initial BSS */
	unsigned char mac_addr_hint[NRF_WIFI_ETH_ADDR_LEN];
	/** SSID (binary attribute, 0..32 octets) */
	struct nrf_wifi_ssid ssid;
	/** IE's @ref nrf_wifi_ie */
	struct nrf_wifi_ie wpa_ie;
	/** VHT Capability information element @ref nrf_wifi_ht_vht_capabilities */
	struct nrf_wifi_ht_vht_capabilities ht_vht_capabilities;
	/** A 16-bit value indicating the ethertype that will be used for key negotiation.
	 *  If it is not specified, the value defaults to 0x888E.
	 */
	unsigned short control_port_ether_type;
	/** When included along with control_port_ether_type, indicates that the custom
	 *  ethertype frames used for key negotiation must not be encrypted.
	 */
	unsigned char control_port_no_encrypt;
	/** Indicating whether user space controls IEEE 802.1X port, If set, the RPU will
	 *  assume that the port is unauthorized until authorized by user space.
	 *  Otherwise, port is marked authorized by default in station mode.
	 */
	signed char control_port;
	/** previous BSSID, used to specify a request to reassociate
	 *  within an ESS that is, to use Reassociate Request frame (with the value of
	 *  this attribute in the Current AP address field) instead of Association
	 *  Request frame which is used for the initial association to an ESS.
	 */
	unsigned char prev_bssid[NRF_WIFI_ETH_ADDR_LEN];
	/** Bss max idle timeout value in sec which will be encapsulated into
	 *  BSS MAX IDLE IE in assoc request frame.
	 */
	unsigned short maxidle_insec;

} __NRF_WIFI_PKD;

#define NRF_WIFI_BEACON_DATA_MAX_HEAD_LEN 256
#define NRF_WIFI_BEACON_DATA_MAX_TAIL_LEN 512
#define NRF_WIFI_BEACON_DATA_MAX_PROBE_RESP_LEN 400

/**
 * @brief This structure provides information about beacon and probe data.
 *
 */

struct nrf_wifi_beacon_data {
	/** length of head */
	unsigned int head_len;
	/** length of tail */
	unsigned int tail_len;
	/** length of probe response template (probe_resp) */
	unsigned int probe_resp_len;
	/**  head portion of beacon (before TIM IE) or %NULL if not changed  */
	unsigned char head[NRF_WIFI_BEACON_DATA_MAX_HEAD_LEN];
	/** tail portion of beacon (after TIM IE) or %NULL if not changed */
	unsigned char tail[NRF_WIFI_BEACON_DATA_MAX_TAIL_LEN];
	/** probe response template */
	unsigned char probe_resp[NRF_WIFI_BEACON_DATA_MAX_PROBE_RESP_LEN];
} __NRF_WIFI_PKD;

#define NRF_WIFI_STA_FLAG_INVALID (1 << 0)
#define NRF_WIFI_STA_FLAG_AUTHORIZED (1 << 1)
#define NRF_WIFI_STA_FLAG_SHORT_PREAMBLE (1 << 2)
#define NRF_WIFI_STA_FLAG_WME (1 << 3)
#define NRF_WIFI_STA_FLAG_MFP (1 << 4)
#define NRF_WIFI_STA_FLAG_AUTHENTICATED (1 << 5)
#define NRF_WIFI_STA_FLAG_TDLS_PEER (1 << 6)
#define NRF_WIFI_STA_FLAG_ASSOCIATED (1 << 7)

/**
 * @brief This structure provides information regarding station flags.
 *
 */

struct nrf_wifi_sta_flag_update {
	/** Mask of station flags to set */
	unsigned int nrf_wifi_mask;
	/** Values to set them to. NRF_WIFI_STA_FLAG_AUTHORIZED */
	unsigned int nrf_wifi_set;
} __NRF_WIFI_PKD;

#define NRF_WIFI_RATE_INFO_BITRATE_VALID (1 << 0)
#define NRF_WIFI_RATE_INFO_BITRATE_COMPAT_VALID (1 << 1)
#define NRF_WIFI_RATE_INFO_BITRATE_MCS_VALID (1 << 2)
#define NRF_WIFI_RATE_INFO_BITRATE_VHT_MCS_VALID (1 << 3)
#define NRF_WIFI_RATE_INFO_BITRATE_VHT_NSS_VALID (1 << 4)

#define NRF_WIFI_RATE_INFO_0_MHZ_WIDTH (1 << 0)
#define NRF_WIFI_RATE_INFO_5_MHZ_WIDTH (1 << 1)
#define NRF_WIFI_RATE_INFO_10_MHZ_WIDTH (1 << 2)
#define NRF_WIFI_RATE_INFO_40_MHZ_WIDTH (1 << 3)
#define NRF_WIFI_RATE_INFO_80_MHZ_WIDTH (1 << 4)
#define NRF_WIFI_RATE_INFO_160_MHZ_WIDTH (1 << 5)
#define NRF_WIFI_RATE_INFO_SHORT_GI (1 << 6)
#define NRF_WIFI_RATE_INFO_80P80_MHZ_WIDTH (1 << 7)

/**
 * @brief This structure contains information about rate parameters.
 *
 */

struct nrf_wifi_rate_info {
	/** Valid fields with in this structure */
	unsigned int valid_fields;
	/** bitrate */
	unsigned int bitrate;
	/** Bitrate compatible */
	unsigned short bitrate_compat;
	/** Modulation and Coding Scheme(MCS) */
	unsigned char nrf_wifi_mcs;
	/** MCS related to VHT */
	unsigned char vht_mcs;
	/** NSS related to VHT */
	unsigned char vht_nss;
	/** Rate flags NRF_WIFI_RATE_INFO_0_MHZ_WIDTH */
	unsigned int nrf_wifi_flags;

} __NRF_WIFI_PKD;

#define NRF_WIFI_BSS_PARAM_FLAGS_CTS_PROT (1<<0)
#define NRF_WIFI_BSS_PARAM_FLAGS_SHORT_PREAMBLE (1<<1)
#define NRF_WIFI_BSS_PARAM_FLAGS_SHORT_SLOT_TIME (1<<2)

/**
 * @brief This structure provides information about the Basic Service Set (BSS)
 *  parameters for the attached station.
 *
 */
struct nrf_wifi_sta_bss_parameters {
	/** bitfields of flags NRF_WIFI_BSS_PARAM_FLAGS_CTS_PROT */
	unsigned char nrf_wifi_flags;
	/** DTIM period for the BSS */
	unsigned char dtim_period;
	/** beacon interval */
	unsigned short beacon_interval;
} __NRF_WIFI_PKD;

#define NRF_WIFI_STA_INFO_CONNECTED_TIME_VALID (1 << 0)
#define NRF_WIFI_STA_INFO_INACTIVE_TIME_VALID (1 << 1)
#define NRF_WIFI_STA_INFO_RX_BYTES_VALID (1 << 2)
#define NRF_WIFI_STA_INFO_TX_BYTES_VALID (1 << 3)
#define NRF_WIFI_STA_INFO_CHAIN_SIGNAL_VALID (1 << 4)
#define NRF_WIFI_STA_INFO_CHAIN_SIGNAL_AVG_VALID (1 << 5)
#define NRF_WIFI_STA_INFO_TX_BITRATE_VALID (1 << 6)
#define NRF_WIFI_STA_INFO_RX_BITRATE_VALID (1 << 7)
#define NRF_WIFI_STA_INFO_STA_FLAGS_VALID (1 << 8)

#define NRF_WIFI_STA_INFO_LLID_VALID (1 << 9)
#define NRF_WIFI_STA_INFO_PLID_VALID (1 << 10)
#define NRF_WIFI_STA_INFO_PLINK_STATE_VALID (1 << 11)
#define NRF_WIFI_STA_INFO_SIGNAL_VALID (1 << 12)
#define NRF_WIFI_STA_INFO_SIGNAL_AVG_VALID (1 << 13)
#define NRF_WIFI_STA_INFO_RX_PACKETS_VALID (1 << 14)
#define NRF_WIFI_STA_INFO_TX_PACKETS_VALID (1 << 15)
#define NRF_WIFI_STA_INFO_TX_RETRIES_VALID (1 << 16)
#define NRF_WIFI_STA_INFO_TX_FAILED_VALID (1 << 17)
#define NRF_WIFI_STA_INFO_EXPECTED_THROUGHPUT_VALID (1 << 18)
#define NRF_WIFI_STA_INFO_BEACON_LOSS_COUNT_VALID (1 << 19)
#define NRF_WIFI_STA_INFO_LOCAL_PM_VALID (1 << 20)
#define NRF_WIFI_STA_INFO_PEER_PM_VALID (1 << 21)
#define NRF_WIFI_STA_INFO_NONPEER_PM_VALID (1 << 22)
#define NRF_WIFI_STA_INFO_T_OFFSET_VALID (1 << 23)
#define NRF_WIFI_STA_INFO_RX_DROPPED_MISC_VALID (1 << 24)
#define NRF_WIFI_STA_INFO_RX_BEACON_VALID (1 << 25)
#define NRF_WIFI_STA_INFO_RX_BEACON_SIGNAL_AVG_VALID (1 << 26)
#define NRF_WIFI_STA_INFO_STA_BSS_PARAMS_VALID (1 << 27)
#define NRF_WIFI_IEEE80211_MAX_CHAINS 4

/**
 * @brief This structure contains information about a Station (STA).
 *
 */

struct nrf_wifi_sta_info {
	/** Valid fields with in this structure */
	unsigned int valid_fields;
	/** time since the station is last connected */
	unsigned int connected_time;
	/** time since last activity, in msec */
	unsigned int inactive_time;
	/** total received bytes from this station */
	unsigned int rx_bytes;
	/** total transmitted bytes to this station */
	unsigned int tx_bytes;
	/** per-chain signal mask value */
	unsigned int chain_signal_mask;
	/** per-chain signal strength of last PPDU */
	unsigned char chain_signal[NRF_WIFI_IEEE80211_MAX_CHAINS];
	/** per-chain signal strength average mask value */
	unsigned int chain_signal_avg_mask;
	/** per-chain signal strength average */
	unsigned char chain_signal_avg[NRF_WIFI_IEEE80211_MAX_CHAINS];
	/**@ref nrf_wifi_rate_info */
	struct nrf_wifi_rate_info tx_bitrate;
	/**@ref nrf_wifi_rate_info */
	struct nrf_wifi_rate_info rx_bitrate;
	/** Not used */
	unsigned short llid;
	/** Not used */
	unsigned short plid;
	/** Not used */
	unsigned char plink_state;
	/** signal strength of last received PPDU, in dbm */
	signed int signal;
	/** signal strength average, in dbm */
	signed int signal_avg;
	/** total received packet from this station */
	unsigned int rx_packets;
	/** total transmitted packets to this station */
	unsigned int tx_packets;
	/** total retries to this station */
	unsigned int tx_retries;
	/** total failed packets to this station */
	unsigned int tx_failed;
	/** expected throughput in kbps */
	unsigned int expected_throughput;
	/** count of times beacon loss was detected */
	unsigned int beacon_loss_count;
	/** Not used */
	unsigned int local_pm;
	/** Not used */
	unsigned int peer_pm;
	/** Not used */
	unsigned int nonpeer_pm;
	/** station flags @ref nrf_wifi_sta_flag_update */
	struct nrf_wifi_sta_flag_update sta_flags;
	/** timing offset with respect to this STA */
	unsigned long long t_offset;
	/** count of times other(non beacon) loss was detected */
	unsigned long long rx_dropped_misc;
	/** count of times beacon */
	unsigned long long rx_beacon;
	/** average of beacon signal */
	long long rx_beacon_signal_avg;
	/** Station connected BSS params. @ref nrf_wifi_sta_bss_parameters */
	struct nrf_wifi_sta_bss_parameters bss_param;
} __NRF_WIFI_PKD;

/**
 * @brief The command header expected by UMAC to handle requests from the control interface.
 *
 */

struct nrf_wifi_umac_hdr {
	/** unused */
	unsigned int portid;
	/** unused */
	unsigned int seq;
	/** UMAC command/event value see &enum nrf_wifi_umac_commands
	 *  see &enum nrf_wifi_umac_events
	 */
	unsigned int cmd_evnt;
	/** unused */
	signed int rpu_ret_val;
	/** Interface information @ref nrf_wifi_index_ids */
	struct nrf_wifi_index_ids ids;
} __NRF_WIFI_PKD;

#define NRF_WIFI_KEY_VALID (1 << 0)
#define NRF_WIFI_KEY_TYPE_VALID (1 << 1)
#define NRF_WIFI_KEY_IDX_VALID (1 << 2)
#define NRF_WIFI_SEQ_VALID (1 << 3)
#define NRF_WIFI_CIPHER_SUITE_VALID (1 << 4)
#define NRF_WIFI_KEY_INFO_VALID (1 << 5)

#define NRF_WIFI_KEY_DEFAULT (1 << 0)
#define NRF_WIFI_KEY_DEFAULT_TYPES (1 << 1)
#define NRF_WIFI_KEY_DEFAULT_MGMT (1 << 2)
#define NRF_WIFI_KEY_DEFAULT_TYPE_UNICAST (1 << 3)
#define NRF_WIFI_KEY_DEFAULT_TYPE_MULTICAST (1 << 4)

/**
 * @brief This structure contains information about a security key.
 *
 */

struct nrf_wifi_umac_key_info {
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Key cipher suite (as defined by IEEE 802.11 section 7.3.2.25.1) */
	unsigned int cipher_suite;
	/** Specify what a key should be set as default as example NRF_WIFI_KEY_DEFAULT_MGMT */
	unsigned short nrf_wifi_flags;
	/** Key Type, see &enum nrf_wifi_key_type */
	signed int key_type;
	/** Key data @ref nrf_wifi_key */
	struct nrf_wifi_key key;
	/** Transmit key sequence number (IV/PN) for TKIP and CCMP keys,
	 *  each six bytes in little endian @ref nrf_wifi_seq
	 */
	struct nrf_wifi_seq seq;
	/** Key ID (0-3) */
	unsigned char key_idx;
} __NRF_WIFI_PKD;

/**
 * @brief This enum represents the various types of scanning operations.
 *
 */
enum scan_mode {
	/** auto or legacy scan operation */
	AUTO_SCAN = 0,
	/** Mapped scan. Host will control channels */
	CHANNEL_MAPPING_SCAN
};

/**
 * @brief This enum describes the different types of scan.
 *
 */
enum scan_reason {
	/** scan for display purpose in user space */
	SCAN_DISPLAY = 0,
	/** scan for connection purpose */
	SCAN_CONNECT
};

/**
 * @brief This structure contains details about scan request information.
 *
 */

struct nrf_wifi_umac_scan_info {
	/** scan mode @ref scan_mode */
	signed int scan_mode;
	/** scan type see &enum scan_reason */
	signed int scan_reason;
	/** scan parameters @ref nrf_wifi_scan_params */
	struct nrf_wifi_scan_params scan_params;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines a command scan request.
 *
 */

struct nrf_wifi_umac_cmd_scan {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** @ref nrf_wifi_umac_scan_info */
	struct nrf_wifi_umac_scan_info info;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines a command to abort a scan request.
 *
 */

struct nrf_wifi_umac_cmd_abort_scan {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
} __NRF_WIFI_PKD;

/**
 * @brief TThis structure defines a command to request scan results.
 * This command should be executed only if we have received a
 * NRF_WIFI_UMAC_EVENT_SCAN_DONE event for a previous scan.
 *
 */

struct nrf_wifi_umac_cmd_get_scan_results {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** scan type see &enum scan_reason */
	signed int scan_reason;
} __NRF_WIFI_PKD;

/**
 * @brief This structure provides details about the "Scan Done" event.
 *
 */
struct nrf_wifi_umac_event_scan_done {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** status, 0=Scan successful & 1=Scan aborted */
	signed int status;
	/** scan type see &enum scan_reason */
	unsigned int scan_type;
} __NRF_WIFI_PKD;


#define MCAST_ADDR_ADD 0
#define MCAST_ADDR_DEL 1

/**
 * @brief This structure represents the parameters used to configure the multicast address filter.
 *
 */
struct nrf_wifi_umac_mcast_cfg {
	/** Add (0) or Delete (1) */
	unsigned int type;
	/** multicast address to be added/deleted */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines a command used to set multicast (mcast) addresses.
 *
 */
struct nrf_wifi_umac_cmd_mcast_filter {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** @ref nrf_wifi_umac_mcast_cfg */
	struct nrf_wifi_umac_mcast_cfg info;
} __NRF_WIFI_PKD;


/**
 * @brief This structure represents the parameters used to change the MAC address.
 *
 */

struct nrf_wifi_umac_change_macaddr_info {
	/** MAC address to be set */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
} __NRF_WIFI_PKD;
/**
 * @brief This structure describes command to change MAC address.
 *  This has to be used only when the interface is down.
 *
 */
struct nrf_wifi_umac_cmd_change_macaddr {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** @ref nrf_wifi_umac_change_macaddr_info */
	struct nrf_wifi_umac_change_macaddr_info macaddr_info;
} __NRF_WIFI_PKD;



#define NRF_WIFI_CMD_AUTHENTICATE_KEY_INFO_VALID (1 << 0)
#define NRF_WIFI_CMD_AUTHENTICATE_BSSID_VALID (1 << 1)
#define NRF_WIFI_CMD_AUTHENTICATE_FREQ_VALID (1 << 2)
#define NRF_WIFI_CMD_AUTHENTICATE_SSID_VALID (1 << 3)
#define NRF_WIFI_CMD_AUTHENTICATE_IE_VALID (1 << 4)
#define NRF_WIFI_CMD_AUTHENTICATE_SAE_VALID (1 << 5)

#define NRF_WIFI_CMD_AUTHENTICATE_LOCAL_STATE_CHANGE (1 << 0)

/**
 * @brief This structure specifies the parameters to be used when sending an authentication request.
 *
 */

struct nrf_wifi_umac_auth_info {
	/** Frequency of the selected channel in MHz */
	unsigned int frequency;
	/** Flag attribute to indicate that a command is requesting a local
	 *  authentication/association state change without invoking actual management
	 *  frame exchange. This can be used with NRF_WIFI_UMAC_CMD_AUTHENTICATE
	 *  NRF_WIFI_UMAC_CMD_DEAUTHENTICATE.
	 */
	unsigned short nrf_wifi_flags;
	/** Authentication type. see &enum nrf_wifi_auth_type */
	signed int auth_type;
	/** Key information */
	struct nrf_wifi_umac_key_info key_info;
	/** SSID (binary attribute, 0..32 octets) */
	struct nrf_wifi_ssid ssid;
	/** Information element(s) data */
	struct nrf_wifi_ie ie;
	/** SAE elements in Authentication frames. This starts
	 *  with the Authentication transaction sequence number field.
	 */
	struct nrf_wifi_sae sae;
	/** MAC address (various uses) */
	unsigned char nrf_wifi_bssid[NRF_WIFI_ETH_ADDR_LEN];
	/** The following parameters will be used to construct bss database in case of
	 * cfg80211 offload to host case.
	 */
	/** scanning width */
	signed int scan_width;
	/** Signal strength */
	signed int nrf_wifi_signal;
	/** Received elements from beacon or probe response */
	signed int from_beacon;
	/** BSS information element data */
	struct nrf_wifi_ie bss_ie;
	/** BSS capability */
	unsigned short capability;
	/** Beacon interval(ms) */
	unsigned short beacon_interval;
	/** Beacon tsf */
	unsigned long long tsf;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines a command used to send an authentication request.
 *
 */

struct nrf_wifi_umac_cmd_auth {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Information to be passed in the authentication command @ref nrf_wifi_umac_auth_info */
	struct nrf_wifi_umac_auth_info info;
} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_ASSOCIATE_MAC_ADDR_VALID (1 << 0)

/**
 * @brief This structure specifies the parameters to be used when sending an association request.
 *
 */

struct nrf_wifi_umac_assoc_info {
	/** Frequency of the selected channel in MHz */
	unsigned int center_frequency;
	/** ssid @ref nrf_wifi_ssid */
	struct nrf_wifi_ssid ssid;
	/** MAC address (various uses) */
	unsigned char nrf_wifi_bssid[NRF_WIFI_ETH_ADDR_LEN];
	/**  WPA information element data. @ref nrf_wifi_ie */
	struct nrf_wifi_ie wpa_ie;
	/** Whether management frame protection (IEEE 802.11w) is used for the association */
	unsigned char use_mfp;
	/** Indicating whether user space controls IEEE 802.1X port. If set, the RPU will
	 *  assume that the port is unauthorized until authorized by user space.
	 *  Otherwise, port is marked authorized by default in station mode.
	 */
	signed char control_port;
	/** Previous BSSID used in flag */
	unsigned int prev_bssid_flag;
	/** Previous BSSID used in Re-assoc. */
	unsigned char prev_bssid[NRF_WIFI_ETH_ADDR_LEN];
	/** Bss max idle timeout value in sec wich will be encapsulated into
	 *  BSS MAX IDLE IE in assoc request frame.
	 */
	unsigned short bss_max_idle_time;
} __NRF_WIFI_PKD;

/**
 * @brief This structure specifies the parameters to be used when sending an association request.
 *
 */

struct nrf_wifi_umac_cmd_assoc {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** @ref nrf_wifi_connect_common_info */
	struct nrf_wifi_connect_common_info connect_common_info;
	/**
	 * Previous BSSID, to be used by in ASSOCIATE commands to specify
	 * using a reassociate frame.
	 */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_MLME_MAC_ADDR_VALID (1 << 0)
#define NRF_WIFI_CMD_MLME_LOCAL_STATE_CHANGE (1 << 0)

/**
 * @brief This structure specifies the parameters to be passed while sending a
 *  deauthentication request (NRF_WIFI_UMAC_CMD_DEAUTHENTICATE).
 *
 */

struct nrf_wifi_umac_disconn_info {
	/** Indicates that a command is requesting a local deauthentication/disassociation
	 *  state change without invoking actual management frame exchange.
	 */
	unsigned short nrf_wifi_flags;
	/** Reason code for disassociation or deauthentication */
	unsigned short reason_code;
	/** MAC address (various uses) */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
} __NRF_WIFI_PKD;

/**
 * @brief This structure specifies the parameters to be used when sending a disconnect request.
 *
 */

struct nrf_wifi_umac_cmd_disconn {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** @ref nrf_wifi_umac_disconn_info */
	struct nrf_wifi_umac_disconn_info info;
} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_NEW_INTERFACE_USE_4ADDR_VALID (1 << 0)
#define NRF_WIFI_CMD_NEW_INTERFACE_MAC_ADDR_VALID (1 << 1)
#define NRF_WIFI_CMD_NEW_INTERFACE_IFTYPE_VALID (1 << 2)
#define NRF_WIFI_CMD_NEW_INTERFACE_IFNAME_VALID (1 << 3)

/**
 * @brief This structure contains the information to be passed to the RPU
 *  to create a new virtual interface using the NRF_WIFI_UMAC_CMD_NEW_INTERFACE command.
 *
 */
struct nrf_wifi_umac_add_vif_info {
	/** Interface type, see enum nrf_wifi_sys_iftype */
	signed int iftype;
	/** Use 4-address frames on a virtual interface */
	signed int nrf_wifi_use_4addr;
	/** Unused */
	unsigned int mon_flags;
	/** MAC Address */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** Interface name */
	signed char ifacename[16];
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines a command used to create a new virtual interface
 *  using the NRF_WIFI_UMAC_CMD_NEW_INTERFACE command.
 *
 */

struct nrf_wifi_umac_cmd_add_vif {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** VIF specific information to be passed to the RPU @ref nrf_wifi_umac_add_vif_info */
	struct nrf_wifi_umac_add_vif_info info;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines a command used to delete a virtual interface.
 *  However, this command is not allowed on the default interface.
 *
 */

struct nrf_wifi_umac_cmd_del_vif {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
} __NRF_WIFI_PKD;

#define NRF_WIFI_FRAME_MATCH_MAX_LEN 8

/**
 * @brief This structure represents the data of management frame that must be matched for
 *  processing in userspace.
 *
 */

struct nrf_wifi_umac_frame_match {
	/** Length of data */
	unsigned int frame_match_len;
	/** Data to match */
	unsigned char frame_match[NRF_WIFI_FRAME_MATCH_MAX_LEN];
} __NRF_WIFI_PKD;

/**
 * @brief This structure contains information about the type of management frame
 *  that should be passed to the driver for processing in userspace.
 *
 */

struct nrf_wifi_umac_mgmt_frame_info {
	/** Frame type/subtype */
	unsigned short frame_type;
	/** Match information Refer &struct nrf_wifi_umac_frame_match */
	struct nrf_wifi_umac_frame_match frame_match;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines a command to inform the RPU to register a management frame,
 *  which must not be filtered by the RPU and should instead be passed to the host for
 *  userspace processing.
 *
 */

struct nrf_wifi_umac_cmd_mgmt_frame_reg {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/**
	 * Management frame specific information to be passed to the RPU.
	 * @ref nrf_wifi_umac_mgmt_frame_info
	 */
	struct nrf_wifi_umac_mgmt_frame_info info;
} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_KEY_MAC_ADDR_VALID (1 << 0)

/**
 * @brief This structure represents command to add a new key.
 *
 */

struct nrf_wifi_umac_cmd_key {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Key information. @ref nrf_wifi_umac_key_info */
	struct nrf_wifi_umac_key_info key_info;
	/** MAC address associated with the key */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines a command that is used to add a new key.
 *
 */

struct nrf_wifi_umac_cmd_set_key {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Key information , @ref nrf_wifi_umac_key_info */
	struct nrf_wifi_umac_key_info key_info;
} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_SET_BSS_CTS_VALID (1 << 0)
#define NRF_WIFI_CMD_SET_BSS_PREAMBLE_VALID (1 << 1)
#define NRF_WIFI_CMD_SET_BSS_SLOT_VALID (1 << 2)
#define NRF_WIFI_CMD_SET_BSS_HT_OPMODE_VALID (1 << 3)
#define NRF_WIFI_CMD_SET_BSS_AP_ISOLATE_VALID (1 << 4)
#define NRF_WIFI_CMD_SET_BSS_P2P_CTWINDOW_VALID (1 << 5)
#define NRF_WIFI_CMD_SET_BSS_P2P_OPPPS_VALID (1 << 6)

#define NRF_WIFI_BASIC_MAX_SUPP_RATES 32

/**
 * @brief This structure contains parameters that describe the BSS (Basic Service Set) information.
 *
 */

struct nrf_wifi_umac_bss_info {
	/** P2P GO Client Traffic Window, used with
	 *  the START_AP and SET_BSS commands.
	 */
	unsigned int p2p_go_ctwindow;
	/** P2P GO opportunistic PS, used with the
	 *  START_AP and SET_BSS commands. This can have the values 0 or 1;
	 *  if not given in START_AP 0 is assumed, if not given in SET_BSS
	 *  no change is made.
	 */
	unsigned int p2p_opp_ps;
	/** Number of basic rate elements */
	unsigned int num_basic_rates;
	/** HT operation mode */
	unsigned short ht_opmode;
	/** Whether CTS protection is enabled (0 or 1) */
	unsigned char nrf_wifi_cts;
	/** Whether short preamble is enabled (0 or 1) */
	unsigned char preamble;
	/** Whether short slot time enabled (0 or 1) */
	unsigned char nrf_wifi_slot;
	/** (AP mode) Do not forward traffic between stations connected to this BSS */
	unsigned char ap_isolate;
	/** Basic rates, array of basic rates in format defined by IEEE 802.11 7.3.2.2 */
	unsigned char basic_rates[NRF_WIFI_BASIC_MAX_SUPP_RATES];
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents a command used to set BSS (Basic Service Set) parameters.
 *
 */

struct nrf_wifi_umac_cmd_set_bss {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** BSS specific information to be passed to the RPU @ref nrf_wifi_umac_bss_info */
	struct nrf_wifi_umac_bss_info bss_info;
} __NRF_WIFI_PKD;

#define NRF_WIFI_SET_FREQ_PARAMS_FREQ_VALID (1 << 0)
#define NRF_WIFI_SET_FREQ_PARAMS_CHANNEL_WIDTH_VALID (1 << 1)
#define NRF_WIFI_SET_FREQ_PARAMS_CENTER_FREQ1_VALID (1 << 2)
#define NRF_WIFI_SET_FREQ_PARAMS_CENTER_FREQ2_VALID (1 << 3)
#define NRF_WIFI_SET_FREQ_PARAMS_CHANNEL_TYPE_VALID (1 << 4)

/**
 * @brief This structure contains information about frequency parameters.
 *
 */

struct freq_params {
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Value in MHz */
	signed int frequency;
	/** Width of the channel @see &enu nrf_wifi_chan_width */
	signed int channel_width;
	/** Unused */
	signed int center_frequency1;
	/** Unused */
	signed int center_frequency2;
	/** Type of channel see &enum nrf_wifi_channel_type */
	signed int channel_type;
} __NRF_WIFI_PKD;

/**
 * @brief This structure contains information about transmit queue parameters.
 *
 */

struct nrf_wifi_txq_params {
	/** Transmit oppurtunity */
	unsigned short txop;
	/** Minimum contention window */
	unsigned short cwmin;
	/** Maximum contention window */
	unsigned short cwmax;
	/** Arbitration interframe spacing */
	unsigned char aifs;
	/** Access category */
	unsigned char ac;

} __NRF_WIFI_PKD;

/**
 * @brief Types of transmit power settings.
 *
 */

enum nrf_wifi_tx_power_type {
	/** Automatically determine transmit power */
	NRF_WIFI_TX_POWER_AUTOMATIC,
	/** Limit TX power by the mBm parameter */
	NRF_WIFI_TX_POWER_LIMITED,
	/** Fix TX power to the mBm parameter */
	NRF_WIFI_TX_POWER_FIXED,
};

#define NRF_WIFI_TX_POWER_SETTING_TYPE_VALID (1 << 0)
#define NRF_WIFI_TX_POWER_SETTING_TX_POWER_LEVEL_VALID (1 << 1)

/**
 * @brief This structure contains the parameters related to the transmit power setting.
 *
 */

struct nrf_wifi_tx_power_setting {
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Power value type, see nrf_wifi_tx_power_type */
	signed int type;
	/** Transmit power level in signed mBm units */
	signed int tx_power_level;

} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_SET_WIPHY_FREQ_PARAMS_VALID (1 << 0)
#define NRF_WIFI_CMD_SET_WIPHY_TXQ_PARAMS_VALID (1 << 1)
#define NRF_WIFI_CMD_SET_WIPHY_RTS_THRESHOLD_VALID (1 << 2)
#define NRF_WIFI_CMD_SET_WIPHY_FRAG_THRESHOLD_VALID (1 << 3)
#define NRF_WIFI_CMD_SET_WIPHY_TX_POWER_SETTING_VALID (1 << 4)
#define NRF_WIFI_CMD_SET_WIPHY_ANTENNA_TX_VALID (1 << 5)
#define NRF_WIFI_CMD_SET_WIPHY_ANTENNA_RX_VALID (1 << 6)
#define NRF_WIFI_CMD_SET_WIPHY_RETRY_SHORT_VALID (1 << 7)
#define NRF_WIFI_CMD_SET_WIPHY_RETRY_LONG_VALID (1 << 8)
#define NRF_WIFI_CMD_SET_WIPHY_COVERAGE_CLASS_VALID (1 << 9)
#define NRF_WIFI_CMD_SET_WIPHY_WIPHY_NAME_VALID (1 << 10)

/**
 * @brief This structure contains information about the configuration parameters
 *  needed to set up and configure the wireless Physical Layer.
 *
 */

struct nrf_wifi_umac_set_wiphy_info {
	/** RTS threshold, TX frames with length larger than or equal to this use RTS/CTS handshake
	 *  allowed range: 0..65536, disable with -1.
	 */
	unsigned int rts_threshold;
	/** Fragmentation threshold, maximum length in octets for frames.
	 *  allowed range: 256..8000, disable fragmentation with (u32)-1.
	 */
	unsigned int frag_threshold;
	/** Bitmap of allowed antennas for transmitting. This can be used to mask out
	 *  antennas which are not attached or should not be used for transmitting.
	 *  If an antenna is not selected in this bitmap the hardware is not allowed
	 *  to transmit on this antenna.
	 */
	unsigned int antenna_tx;
	/** Bitmap of allowed antennas for receiving. This can be used to mask out antennas
	 *  which are not attached or should not be used for receiving. If an antenna is
	 *  not selected in this bitmap the hardware should not be configured to receive
	 *  on this antenna.
	 */
	unsigned int antenna_rx;
	/** Frequency information of the a channel see &struct freq_params */
	struct freq_params freq_params;
	/** TX queue parameters @ref nrf_wifi_txq_params */
	struct nrf_wifi_txq_params txq_params;
	/** Tx power settings @ref nrf_wifi_tx_power_setting @ref nrf_wifi_tx_power_setting */
	struct nrf_wifi_tx_power_setting tx_power_setting;
	/** TX retry limit for frames whose length is less than or equal to the RTS threshold
	 *  allowed range: 1..255.
	 */
	unsigned char retry_short;
	/** TX retry limit for frames whose length is greater than the RTS threshold
	 *  allowed range: 1..255.
	 */
	unsigned char retry_long;
	/** Unused */
	unsigned char coverage_class;
	/** WIPHY name (used for renaming) */
	signed char wiphy_name[32];
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command to set the wireless PHY configuration.
 *
 */

struct nrf_wifi_umac_cmd_set_wiphy {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicates which of the following parameters are valid */
	unsigned int valid_fields;
	/** @ref nrf_wifi_umac_set_wiphy_info */
	struct nrf_wifi_umac_set_wiphy_info info;
} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_DEL_STATION_MAC_ADDR_VALID (1 << 0)
#define NRF_WIFI_CMD_DEL_STATION_MGMT_SUBTYPE_VALID (1 << 1)
#define NRF_WIFI_CMD_DEL_STATION_REASON_CODE_VALID (1 << 2)

/**
 * @brief This structure contains the parameters to delete a station.
 *
 */

struct nrf_wifi_umac_del_sta_info {
	/** MAC address of the station */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** Management frame subtype */
	unsigned char mgmt_subtype;
	/** Reason code for DEAUTHENTICATION and DISASSOCIATION */
	unsigned short reason_code;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command to delete a station.
 *
 */

struct nrf_wifi_umac_cmd_del_sta {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Information regarding the station to be deleted @ref nrf_wifi_umac_del_sta_info */
	struct nrf_wifi_umac_del_sta_info info;
} __NRF_WIFI_PKD;

/**
 * @brief This structure contains the information required for obtaining station details.
 *
 */

struct nrf_wifi_umac_get_sta_info {
	/** MAC address of the station */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command to get station information.
 *
 */

struct nrf_wifi_umac_cmd_get_sta {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Information regarding the station to get @ref nrf_wifi_umac_get_sta_info */
	struct nrf_wifi_umac_get_sta_info info;
} __NRF_WIFI_PKD;

#define NRF_WIFI_EXT_CAPABILITY_MAX_LEN 32

/**
 * @brief Extended capability information.
 */

struct nrf_wifi_ext_capability {
	/** length */
	unsigned int ext_capability_len;
	/** Extended capability info*/
	unsigned char ext_capability[NRF_WIFI_EXT_CAPABILITY_MAX_LEN];

} __NRF_WIFI_PKD;

#define NRF_WIFI_SUPPORTED_CHANNELS_MAX_LEN 64

/**
 * @brief Supported channels.
 */

struct nrf_wifi_supported_channels {
	/** number of channels */
	unsigned int supported_channels_len;
	/** channels info */
	unsigned char supported_channels[NRF_WIFI_SUPPORTED_CHANNELS_MAX_LEN];

} __NRF_WIFI_PKD;

#define NRF_WIFI_OPER_CLASSES_MAX_LEN 64

/**
 * @brief Operating classes information.
 */
struct nrf_wifi_supported_oper_classes {
	/** length */
	unsigned int supported_oper_classes_len;
	/** oper_class info*/
	unsigned char supported_oper_classes[NRF_WIFI_OPER_CLASSES_MAX_LEN];

} __NRF_WIFI_PKD;

#define NRF_WIFI_STA_FLAGS2_MAX_LEN 64

/**
 * @brief Station flags.
 */

struct nrf_wifi_sta_flags2 {
	/** length */
	unsigned int sta_flags2_len;
	/** flags */
	unsigned char sta_flags2[NRF_WIFI_STA_FLAGS2_MAX_LEN];

} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_SET_STATION_SUPP_RATES_VALID (1 << 0)
#define NRF_WIFI_CMD_SET_STATION_AID_VALID (1 << 1)
#define NRF_WIFI_CMD_SET_STATION_PEER_AID_VALID (1 << 2)
#define NRF_WIFI_CMD_SET_STATION_STA_CAPABILITY_VALID (1 << 3)
#define NRF_WIFI_CMD_SET_STATION_EXT_CAPABILITY_VALID (1 << 4)
#define NRF_WIFI_CMD_SET_STATION_STA_VLAN_VALID (1 << 5)
#define NRF_WIFI_CMD_SET_STATION_HT_CAPABILITY_VALID (1 << 6)
#define NRF_WIFI_CMD_SET_STATION_VHT_CAPABILITY_VALID (1 << 7)
#define NRF_WIFI_CMD_SET_STATION_OPMODE_NOTIF_VALID (1 << 9)
#define NRF_WIFI_CMD_SET_STATION_SUPPORTED_CHANNELS_VALID (1 << 10)
#define NRF_WIFI_CMD_SET_STATION_SUPPORTED_OPER_CLASSES_VALID (1 << 11)
#define NRF_WIFI_CMD_SET_STATION_STA_FLAGS2_VALID (1 << 12)
#define NRF_WIFI_CMD_SET_STATION_STA_WME_UAPSD_QUEUES_VALID (1 << 13)
#define NRF_WIFI_CMD_SET_STATION_STA_WME_MAX_SP_VALID (1 << 14)
#define NRF_WIFI_CMD_SET_STATION_LISTEN_INTERVAL_VALID (1 << 15)

/**
 * @brief This structure represents the information needed to update a station entry
 * in the RPU.
 *
 */

struct nrf_wifi_umac_chg_sta_info {
	/** Listen interval as defined by IEEE 802.11 7.3.1.6 */
	signed int nrf_wifi_listen_interval;
	/** Unused */
	unsigned int sta_vlan;
	/** AID or zero for no change */
	unsigned short aid;
	/** Unused */
	unsigned short nrf_wifi_peer_aid;
	/** Station capability */
	unsigned short sta_capability;
	/** Unused */
	unsigned short spare;
	/** Supported rates in IEEE 802.11 format @ref nrf_wifi_supp_rates */
	struct nrf_wifi_supp_rates supp_rates;
	/** Extended capabilities of the station @ref nrf_wifi_ext_capability */
	struct nrf_wifi_ext_capability ext_capability;
	/** Supported channels in IEEE 802.11 format @ref nrf_wifi_supported_channels */
	struct nrf_wifi_supported_channels supported_channels;
	/** Supported oper classes in IEEE 802.11 format @ref nrf_wifi_supported_oper_classes */
	struct nrf_wifi_supported_oper_classes supported_oper_classes;
	/** station flags mask/set @ref nrf_wifi_sta_flag_update @ref nrf_wifi_sta_flag_update */
	struct nrf_wifi_sta_flag_update sta_flags2;
	/** HT capabilities of station */
	unsigned char ht_capability[NRF_WIFI_HT_VHT_CAPABILITY_MAX_SIZE];
	/** VHT capabilities of station */
	unsigned char vht_capability[NRF_WIFI_HT_VHT_CAPABILITY_MAX_SIZE];
	/** Station mac address */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** Information if operating mode field is used */
	unsigned char opmode_notif;
	/** Bitmap of queues configured for uapsd. Same format
	 *  as the AC bitmap in the QoS info field.
	 */
	unsigned char wme_uapsd_queues;
	/** Max Service Period. same format as the MAX_SP in the
	 *  QoS info field (but already shifted down).
	 */
	unsigned char wme_max_sp;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command for updating the parameters of a station entry.
 *
 */

struct nrf_wifi_umac_cmd_chg_sta {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** @ref nrf_wifi_umac_chg_sta_info */
	struct nrf_wifi_umac_chg_sta_info info;
} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_NEW_STATION_SUPP_RATES_VALID (1 << 0)
#define NRF_WIFI_CMD_NEW_STATION_AID_VALID (1 << 1)
#define NRF_WIFI_CMD_NEW_STATION_PEER_AID_VALID (1 << 2)
#define NRF_WIFI_CMD_NEW_STATION_STA_CAPABILITY_VALID (1 << 3)
#define NRF_WIFI_CMD_NEW_STATION_EXT_CAPABILITY_VALID (1 << 4)
#define NRF_WIFI_CMD_NEW_STATION_STA_VLAN_VALID (1 << 5)
#define NRF_WIFI_CMD_NEW_STATION_HT_CAPABILITY_VALID (1 << 6)
#define NRF_WIFI_CMD_NEW_STATION_VHT_CAPABILITY_VALID (1 << 7)
#define NRF_WIFI_CMD_NEW_STATION_OPMODE_NOTIF_VALID (1 << 9)
#define NRF_WIFI_CMD_NEW_STATION_SUPPORTED_CHANNELS_VALID (1 << 10)
#define NRF_WIFI_CMD_NEW_STATION_SUPPORTED_OPER_CLASSES_VALID (1 << 11)
#define NRF_WIFI_CMD_NEW_STATION_STA_FLAGS2_VALID (1 << 12)
#define NRF_WIFI_CMD_NEW_STATION_STA_WME_UAPSD_QUEUES_VALID (1 << 13)
#define NRF_WIFI_CMD_NEW_STATION_STA_WME_MAX_SP_VALID (1 << 14)
#define NRF_WIFI_CMD_NEW_STATION_LISTEN_INTERVAL_VALID (1 << 15)

/**
 * @brief This structure describes the parameters for adding a new station entry to the RPU.
 *
 */

struct nrf_wifi_umac_add_sta_info {
	/** Listen interval as defined by IEEE 802.11 7.3.1.6 */
	signed int nrf_wifi_listen_interval;
	/** Unused */
	unsigned int sta_vlan;
	/** AID or zero for no change */
	unsigned short aid;
	/** Unused */
	unsigned short nrf_wifi_peer_aid;
	/** Station capability */
	unsigned short sta_capability;
	/** Unused */
	unsigned short spare;
	/** Supported rates in IEEE 802.11 format @ref nrf_wifi_supp_rates */
	struct nrf_wifi_supp_rates supp_rates;
	/** Extended capabilities of the station @ref nrf_wifi_ext_capability */
	struct nrf_wifi_ext_capability ext_capability;
	/** Supported channels in IEEE 802.11 format @ref nrf_wifi_supported_channels */
	struct nrf_wifi_supported_channels supported_channels;
	/** Supported oper classes in IEEE 802.11 format @ref nrf_wifi_supported_oper_classes */
	struct nrf_wifi_supported_oper_classes supported_oper_classes;
	/** station flags mask/set @ref nrf_wifi_sta_flag_update */
	struct nrf_wifi_sta_flag_update sta_flags2;
	/** HT capabilities of station */
	unsigned char ht_capability[NRF_WIFI_HT_VHT_CAPABILITY_MAX_SIZE];
	/** VHT capabilities of station */
	unsigned char vht_capability[NRF_WIFI_HT_VHT_CAPABILITY_MAX_SIZE];
	/** Station mac address */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** Information if operating mode field is used */
	unsigned char opmode_notif;
	/** Bitmap of queues configured for uapsd. same format
	 *  as the AC bitmap in the QoS info field.
	 */
	unsigned char wme_uapsd_queues;
	/** Max Service Period. same format as the MAX_SP in the
	 *  QoS info field (but already shifted down).
	 */
	unsigned char wme_max_sp;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command for adding a new station entry.
 *
 */

struct nrf_wifi_umac_cmd_add_sta {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** @ref nrf_wifi_umac_add_sta_info */
	struct nrf_wifi_umac_add_sta_info info;
} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_BEACON_INFO_BEACON_INTERVAL_VALID (1 << 0)
#define NRF_WIFI_CMD_BEACON_INFO_AUTH_TYPE_VALID (1 << 1)
#define NRF_WIFI_CMD_BEACON_INFO_VERSIONS_VALID (1 << 2)
#define NRF_WIFI_CMD_BEACON_INFO_CIPHER_SUITE_GROUP_VALID (1 << 3)
#define NRF_WIFI_CMD_BEACON_INFO_INACTIVITY_TIMEOUT_VALID (1 << 4)
#define NRF_WIFI_CMD_BEACON_INFO_FREQ_PARAMS_VALID (1 << 5)

#define NRF_WIFI_CMD_BEACON_INFO_PRIVACY (1 << 0)
#define NRF_WIFI_CMD_BEACON_INFO_CONTROL_PORT_NO_ENCRYPT (1 << 1)
#define NRF_WIFI_CMD_BEACON_INFO_P2P_CTWINDOW_VALID (1 << 6)
#define NRF_WIFI_CMD_BEACON_INFO_P2P_OPPPS_VALID (1 << 7)

/**
 * @brief This structure describes the parameters required to be passed to the RPU when
 *  initiating a SoftAP (Soft Access Point).
 *
 */

struct nrf_wifi_umac_start_ap_info {
	/** Beacon frame interval */
	unsigned short beacon_interval;
	/** DTIM count */
	unsigned char dtim_period;
	/** Send beacons with wildcard sssid */
	signed int hidden_ssid;
	/** Authentication type, see &enum nrf_wifi_auth_type */
	signed int auth_type;
	/** Unused */
	signed int smps_mode;
	/** Beacon info flags */
	unsigned int nrf_wifi_flags;
	/** Beacon frame, @ref nrf_wifi_beacon_data */
	struct nrf_wifi_beacon_data beacon_data;
	/** SSID string, @ref nrf_wifi_ssid */
	struct nrf_wifi_ssid ssid;
	/** Connect params, @ref nrf_wifi_connect_common_info */
	struct nrf_wifi_connect_common_info connect_common_info;
	/** Channel info, see &struct freq_params */
	struct freq_params freq_params;
	/** Time to stop ap after inactivity period */
	unsigned short inactivity_timeout;
	/** P2P GO Client Traffic Window */
	unsigned char p2p_go_ctwindow;
	/** Opportunistic power save allows P2P Group Owner to save power
	 *  when all its associated clients are sleeping.
	 */
	unsigned char p2p_opp_ps;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command for starting the SoftAP using
 *  NRF_WIFI_UMAC_CMD_NEW_BEACON and NRF_WIFI_UMAC_CMD_START_AP.
 *
 */

struct nrf_wifi_umac_cmd_start_ap {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Parameters that need to be passed to the RPU when starting a SoftAP.
	 *  @ref nrf_wifi_umac_start_ap_info
	 */
	struct nrf_wifi_umac_start_ap_info info;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to stop Soft AP operation.
 *
 */

struct nrf_wifi_umac_cmd_stop_ap {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the parameters that must be passed to the RPU when
 *  configuring Beacon and Probe response data.
 *
 */

struct nrf_wifi_umac_set_beacon_info {
	/** Beacon frame, @ref nrf_wifi_beacon_data */
	struct nrf_wifi_beacon_data beacon_data;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command for setting the beacon data using
 *  NRF_WIFI_UMAC_CMD_SET_BEACON.
 *
 */

struct nrf_wifi_umac_cmd_set_beacon {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** @ref nrf_wifi_umac_set_beacon_info */
	struct nrf_wifi_umac_set_beacon_info info;
} __NRF_WIFI_PKD;

#define NRF_WIFI_SET_INTERFACE_IFTYPE_VALID (1 << 0)
#define NRF_WIFI_SET_INTERFACE_USE_4ADDR_VALID (1 << 1)

/**
 * @brief This structure contains the information that needs to be provided to the RPU
 *  when modifying the attributes of a virtual interface.
 *
 */

struct nrf_wifi_umac_chg_vif_attr_info {
	/** Interface type, see &enum nrf_wifi_iftype */
	signed int iftype;
	/** Unused */
	signed int nrf_wifi_use_4addr;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to change the interface.
 *
 */

struct nrf_wifi_umac_cmd_chg_vif_attr {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Interface attributes to be changed @ref nrf_wifi_umac_chg_vif_attr_info */
	struct nrf_wifi_umac_chg_vif_attr_info info;
} __NRF_WIFI_PKD;

#define IFACENAMSIZ 16

/**
 * @brief This structure contains the information that needs to be passed to the RPU
 *  when changing the interface state, specifically when bringing it up or down
 *
 */

struct nrf_wifi_umac_chg_vif_state_info {
	/** Interface state (1 = UP / 0 = DOWN) */
	signed int state;
	/** Interface name */
	signed char ifacename[IFACENAMSIZ];
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to change the interface state.
 *
 */

struct nrf_wifi_umac_cmd_chg_vif_state {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** @ref nrf_wifi_umac_chg_vif_state_info */
	struct nrf_wifi_umac_chg_vif_state_info info;
} __NRF_WIFI_PKD;
/**
 * @brief This structure defines an event-to-command mapping for NRF_WIFI_UMAC_CMD_SET_IFFLAGS.
 *
 */

struct nrf_wifi_umac_event_vif_state {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Status to command NRF_WIFI_UMAC_CMD_SET_IFFLAGS */
	signed int status;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to start P2P (Peer-to-Peer) mode on an interface.
 */

struct nrf_wifi_cmd_start_p2p {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the command for stopping P2P mode on an interface.
 *
 */

struct nrf_wifi_umac_cmd_stop_p2p_dev {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_FRAME_FREQ_VALID (1 << 0)
#define NRF_WIFI_CMD_FRAME_DURATION_VALID (1 << 1)
#define NRF_WIFI_CMD_SET_FRAME_FREQ_PARAMS_VALID (1 << 2)

#define NRF_WIFI_CMD_FRAME_OFFCHANNEL_TX_OK (1 << 0)
#define NRF_WIFI_CMD_FRAME_TX_NO_CCK_RATE (1 << 1)
#define NRF_WIFI_CMD_FRAME_DONT_WAIT_FOR_ACK (1 << 2)

/**
 * @brief This structure describes the parameters required to transmit a
 *  management frame from the host.
 *
 */

struct nrf_wifi_umac_mgmt_tx_info {
	/** OFFCHANNEL_TX_OK, NO_CCK_RATE, DONT_WAIT_FOR_ACK */
	unsigned int nrf_wifi_flags;
	/** Channel frequency */
	unsigned int frequency;
	/** Duration field value */
	unsigned int dur;
	/** Management frame to transmit, @ref nrf_wifi_frame */
	struct nrf_wifi_frame frame;
	/** Frequency configuration, see &struct freq_params */
	struct freq_params freq_params;
	/** Identifier to be used for processing event,
	 *  NRF_WIFI_UMAC_EVENT_FRAME_TX_STATUS.
	 */
	unsigned long long host_cookie;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to transmit a management frame.
 *
 */

struct nrf_wifi_umac_cmd_mgmt_tx {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Information about the management frame to be transmitted.
	 *  @ref nrf_wifi_umac_mgmt_tx_info
	 */
	struct nrf_wifi_umac_mgmt_tx_info info;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the information regarding the power save state.
 *
 */

struct nrf_wifi_umac_set_power_save_info {
	/** power save is disabled or enabled, see enum nrf_wifi_ps_state */
	signed int ps_state;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the command used to enable or disable the power save
 *  functionality.
 *
 */

struct nrf_wifi_umac_cmd_set_power_save {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Power save setting parameters.
	 * @ref nrf_wifi_umac_set_power_save_info
	 */
	struct nrf_wifi_umac_set_power_save_info info;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the command to configure power save timeout value.
 *
 */

struct nrf_wifi_umac_cmd_set_power_save_timeout {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Timeout value in milli seconds
	 * if timeout < 0 RPU will set timeout to 100ms
	 */
	signed int timeout;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the information of qos_map.
 *
 */

struct nrf_wifi_umac_qos_map_info {
	/** length of qos_map info field */
	unsigned int qos_map_info_len;
	/** contains qos_map info as received from stack */
	unsigned char qos_map_info[256];
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the information related to the Quality of Service (QoS) map.
 *
 */

struct nrf_wifi_umac_cmd_set_qos_map {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** qos map info. @ref nrf_wifi_umac_qos_map_info */
	struct nrf_wifi_umac_qos_map_info info;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to retrieve the transmit power information.
 *
 */

struct nrf_wifi_umac_cmd_get_tx_power {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to obtain the regulatory domain information.
 *
 */

struct nrf_wifi_umac_cmd_get_reg {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to retrieve channel information.
 *
 */

struct nrf_wifi_umac_cmd_get_channel {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
} __NRF_WIFI_PKD;

#define NRF_WIFI_TWT_NEGOTIATION_TYPE_INDIVIDUAL 0
#define NRF_WIFI_TWT_NEGOTIATION_TYPE_BROADCAST 2

/**
 * @brief TWT setup commands and events.
 *
 */

enum nrf_wifi_twt_setup_cmd_type {
	/** STA requests to join a TWT without specifying a target wake time */
	NRF_WIFI_REQUEST_TWT,
	/** STA requests to join a TWT with specifying a target wake time and
	 *  other params, these values can change during negotiation.
	 */
	NRF_WIFI_SUGGEST_TWT,
	/** requests to join a TWT with demanded a target wake time
	 * and other params. STA rejects if AP not scheduling those params.
	 */
	NRF_WIFI_DEMAND_TWT,
	/** Response to the STA request(suggest/demand), these may be different params */
	NRF_WIFI_GROUPING_TWT,
	/** AP accept the STA requested params */
	NRF_WIFI_ACCEPT_TWT,
	/** AP may suggest the params, these may be different from STA requested */
	NRF_WIFI_ALTERNATE_TWT,
	/** AP may suggest the params, these may be different from STA requested */
	NRF_WIFI_DICTATE_TWT,
	/** AP may reject the STA requested params */
	NRF_WIFI_REJECT_TWT,
};

#define NRF_WIFI_TWT_FLOW_TYPE_ANNOUNCED 0
#define NRF_WIFI_TWT_FLOW_TYPE_UNANNOUNCED 1

#define NRF_WIFI_TWT_RESP_RECEIVED 0
#define NRF_WIFI_TWT_RESP_NOT_RECEIVED 1
#define NRF_WIFI_INVALID_TWT_WAKE_INTERVAL 3

/**
 * @brief This structure describes the TWT information.
 *
 */

struct nrf_wifi_umac_config_twt_info {
	/** TWT flow Id */
	unsigned char twt_flow_id;
	/** Negotiation type
	 *  NRF_WIFI_TWT_NEGOTIATION_TYPE_INDIVIDUAL or
	 *  NRF_WIFI_TWT_NEGOTIATION_TYPE_BROADAST
	 */
	unsigned char neg_type;
	/** see &enum nrf_wifi_twt_setup_cmd_type  */
	signed int setup_cmd;
	/** indicating AP to initiate a trigger frame (ps_poll/Null) before data transfer */
	unsigned char ap_trigger_frame;
	/** 1->implicit(same negotiated values to be used),
	 *  0->AP sends new calculated TWT values for every service period.
	 */
	unsigned char is_implicit;
	/** Whether STA has to send the PS-Poll/Null frame
	 *  indicating that it's in wake period(NRF_WIFI_TWT_FLOW_TYPE_ANNOUNCED)
	 */
	unsigned char twt_flow_type;
	/** wake interval exponent value  */
	unsigned char twt_target_wake_interval_exponent;
	/** wake interval mantissa value */
	unsigned short twt_target_wake_interval_mantissa;
	/** start of the waketime value after successful TWT negotiation */
	unsigned long long target_wake_time;
	/** min TWT wake duration */
	unsigned int nominal_min_twt_wake_duration;
	/** dialog_token of twt frame */
	unsigned char dialog_token;
	/** 0->not received 1->received */
	unsigned char twt_resp_status;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the parameters required for setting up TWT session.
 *
 */

struct nrf_wifi_umac_cmd_config_twt {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** TWT configuration info @ref nrf_wifi_umac_config_twt_info */
	struct nrf_wifi_umac_config_twt_info info;
} __NRF_WIFI_PKD;

#define INVALID_TIME 1
#define TRIGGER_NOT_RECEIVED 2

/**
 * @brief This structure represents the TWT delete information.
 *
 */

struct nrf_wifi_umac_teardown_twt_info {
	/** TWT flow Id  */
	unsigned char twt_flow_id;
	/** reason for teardown */
	unsigned char reason_code;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to delete or remove a TWT session
 *
 */

struct nrf_wifi_umac_cmd_teardown_twt {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** @ref nrf_wifi_umac_teardown_twt_info */
	struct nrf_wifi_umac_teardown_twt_info info;
} __NRF_WIFI_PKD;

#define TWT_BLOCK_TX 0
#define TWT_UNBLOCK_TX 1
/**
 * @brief This structure represents the information related to Tx (transmit) block/unblock.
 *
 */
struct twt_sleep_info {
	/** value for blocking/unblocking TX
	 *  (TWT_BLOCK_TX or TWT_UNBLOCK_TX)
	 */
	unsigned int type;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines an event used to indicate to the host whether to block or
 *  unblock Tx (transmit) packets in TWT communication.
 *
 */

struct nrf_wifi_umac_event_twt_sleep {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** @ref twt_sleep_info */
	struct twt_sleep_info info;
} __NRF_WIFI_PKD;

#define UAPSD_Q_MIN 0
#define UAPSD_Q_MAX 15
/**
 * @brief This structure represents the information about UAPSD queues.
 *
 */

struct nrf_wifi_umac_uapsd_info {
	/** UAPSD-Q value */
	unsigned int uapsd_queue;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to configure the UAPSD-Q value.
 *
 */

struct nrf_wifi_umac_cmd_config_uapsd {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** @ref nrf_wifi_umac_uapsd_info */
	struct nrf_wifi_umac_uapsd_info info;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the event used to indicate that a scan has started.
 *
 */

struct nrf_wifi_umac_event_trigger_scan {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Scan request control flags (u32). Bit values
	 * (NRF_WIFI_SCAN_FLAG_LOW_PRIORITY/NRF_WIFI_SCAN_FLAG_RANDOM_ADDR...)
	 */
	unsigned int nrf_wifi_scan_flags;
	/** No.of ssids in scan request */
	unsigned char num_scan_ssid;
	/** No.of frequencies in scan request */
	unsigned char num_scan_frequencies;
	/** center frequencies */
	unsigned short scan_frequencies[NRF_WIFI_SCAN_MAX_NUM_FREQUENCIES];
	/** @ref nrf_wifi_ssid */
	struct nrf_wifi_ssid scan_ssid[NRF_WIFI_SCAN_MAX_NUM_SSIDS];
	/** @ref nrf_wifi_ie */
	struct nrf_wifi_ie ie;
} __NRF_WIFI_PKD;

#define NRF_WIFI_EVENT_NEW_SCAN_RESULTS_MAC_ADDR_VALID (1 << 0)
#define NRF_WIFI_EVENT_NEW_SCAN_RESULTS_IES_TSF_VALID (1 << 1)
#define NRF_WIFI_EVENT_NEW_SCAN_RESULTS_IES_VALID (1 << 2)
#define NRF_WIFI_EVENT_NEW_SCAN_RESULTS_BEACON_IES_TSF_VALID (1 << 3)
#define NRF_WIFI_EVENT_NEW_SCAN_RESULTS_BEACON_IES_VALID (1 << 4)
#define NRF_WIFI_EVENT_NEW_SCAN_RESULTS_BEACON_INTERVAL_VALID (1 << 5)
#define NRF_WIFI_EVENT_NEW_SCAN_RESULTS_SIGNAL_VALID (1 << 6)
#define NRF_WIFI_EVENT_NEW_SCAN_RESULTS_STATUS_VALID (1 << 7)
#define NRF_WIFI_EVENT_NEW_SCAN_RESULTS_BSS_PRESP_DATA (1 << 8)

#define NRF_WIFI_NEW_SCAN_RESULTS_BSS_PRESP_DATA (1 << 0)

/**
 * @brief This structure serves as a response to the command NRF_WIFI_UMAC_CMD_GET_SCAN_RESULTS.
 *  It contains scan results for each entry. RPU sends multiple events of this type for every
 *  scan entry, and when umac_hdr->seq == 0, it indicates the last scan entry.
 *
 */

struct nrf_wifi_umac_event_new_scan_results {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Unused */
	unsigned int generation;
	/** Frequency in MHz */
	unsigned int frequency;
	/** Channel width of the control channel */
	unsigned int chan_width;
	/** Age of this BSS entry in ms */
	unsigned int seen_ms_ago;
	/** Unused */
	unsigned int nrf_wifi_flags;
	/** Status, if this BSS is "used" */
	signed int status;
	/** TSF of the received probe response/beacon (u64) */
	unsigned long long ies_tsf;
	/** TSF of the last received beacon
	 *  (not present if no beacon frame has been received yet).
	 */
	unsigned long long beacon_ies_tsf;
	/** Beacon interval of BSS */
	unsigned short beacon_interval;
	/** Capability field */
	unsigned short capability;
	/** Signal strength, @ref nrf_wifi_signal */
	struct nrf_wifi_signal signal;
	/** BSSID of the BSS (6 octets) */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** Indicates length of IE's present at the starting of ies[0] */
	unsigned int ies_len;
	/** Indicates length of beacon_ies present after ies+ies_len */
	unsigned int beacon_ies_len;
	/** contains raw information elements from the probe response/beacon.
	 * If beacon_ies are not present then the IEs here are from a Probe Response
	 * frame; otherwise they are from a Beacon frame.
	 */
	unsigned char ies[0];
} __NRF_WIFI_PKD;

#define NRF_WIFI_802_11A (1 << 0)
#define NRF_WIFI_802_11B (1 << 1)
#define NRF_WIFI_802_11G (1 << 2)
#define NRF_WIFI_802_11N (1 << 3)
#define NRF_WIFI_802_11AC (1 << 4)
#define NRF_WIFI_802_11AX (1 << 5)

#define NRF_WIFI_MFP_REQUIRED (1 << 0)
#define NRF_WIFI_MFP_CAPABLE  (1 << 1)
/**
 * @brief This structure represents the response for NRF_WIFI_UMAC_CMD_GET_SCAN_RESULTS.
 *  It contains the displayed scan result.
 *
 */

struct umac_display_results {
	/** Network SSID @ref nrf_wifi_ssid */
	struct nrf_wifi_ssid ssid;
	/** BSSID of the BSS (6 octets) */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** Network band of operation, refer &enum nrf_wifi_band */
	signed int nwk_band;
	/** Network channel number */
	unsigned int nwk_channel;
	/**  Protocol type (NRF_WIFI_802_11A) */
	unsigned char protocol_flags;
	/** Network security mode, refer &enum nrf_wifi_security_type */
	signed int security_type;
	/** Beacon interval of the BSS */
	unsigned short beacon_interval;
	/** Capability field */
	unsigned short capability;
	/** Signal strength. Refer &struct nrf_wifi_signal */
	struct nrf_wifi_signal signal;
	/** TWT support */
	unsigned char twt_support;
	/** management frame protection NRF_WIFI_MFP_REQUIRED/NRF_WIFI_MFP_CAPABLE */
	unsigned char mfp_flag;
	/** reserved */
	unsigned char reserved3;
	/** reserved */
	unsigned char reserved4;
} __NRF_WIFI_PKD;

#define DISPLAY_BSS_TOHOST_PEREVNT 8

/**
 * @brief This structure serves as a response to the command NRF_WIFI_UMAC_CMD_GET_SCAN_RESULTS
 *  of display scan type. It contains a maximum of DISPLAY_BSS_TOHOST_PEREVENT scan results
 *  in each event. When umac_hdr->seq == 0, it indicates the last scan event.
 *
 */

struct nrf_wifi_umac_event_new_scan_display_results {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Number of scan results in the current event */
	unsigned char event_bss_count;
	/** Display scan results info @ref umac_display_results */
	struct umac_display_results display_results[DISPLAY_BSS_TOHOST_PEREVNT];
} __NRF_WIFI_PKD;

#define NRF_WIFI_EVENT_MLME_FRAME_VALID (1 << 0)
#define NRF_WIFI_EVENT_MLME_MAC_ADDR_VALID (1 << 1)
#define NRF_WIFI_EVENT_MLME_FREQ_VALID (1 << 2)
#define NRF_WIFI_EVENT_MLME_COOKIE_VALID (1 << 3)
#define NRF_WIFI_EVENT_MLME_RX_SIGNAL_DBM_VALID (1 << 4)
#define NRF_WIFI_EVENT_MLME_WME_UAPSD_QUEUES_VALID (1 << 5)
#define NRF_WIFI_EVENT_MLME_RXMGMT_FLAGS_VALID (1 << 6)
#define NRF_WIFI_EVENT_MLME_IE_VALID   (1 << 7)

#define NRF_WIFI_EVENT_MLME_TIMED_OUT (1 << 0)
#define NRF_WIFI_EVENT_MLME_ACK (1 << 1)

/**
 * @brief This structure represent different responses received from the access point during
 *  various stages of the connection process like Authentication Response and Association Response.
 *
 */

struct nrf_wifi_umac_event_mlme {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Frequency of the channel in MHz */
	unsigned int frequency;
	/** Signal strength in dBm */
	unsigned int rx_signal_dbm;
	/** Indicate whether the frame was acked or timed out */
	unsigned int nrf_wifi_flags;
	/** cookie identifier */
	unsigned long long cookie;
	/** Frame data, including frame header and body @ref nrf_wifi_frame */
	struct nrf_wifi_frame frame;
	/** BSSID of the BSS */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** Bitmap of uapsd queues */
	unsigned char wme_uapsd_queues;
	/** Request(AUTH/ASSOC) ie length */
	unsigned int req_ie_len;
	/** ie's */
	unsigned char req_ie[0];
} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_SEND_STATION_ASSOC_REQ_IES_VALID (1 << 0)

/**
 * @brief This structure represents an event that is generated when a station is added or removed.
 *
 */

struct nrf_wifi_umac_event_new_station {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate if assoc_req ies is valid */
	unsigned int valid_fields;
	/** set to 1: STA supports QoS/WME */
	unsigned char wme;
	/** Set to 1 if STA is Legacy(a/b/g) */
	unsigned char is_sta_legacy;
	/** Station mac address */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** generation number */
	unsigned int generation;
	/** Station information @ref nrf_wifi_sta_info */
	struct nrf_wifi_sta_info sta_info;
	/** @ref nrf_wifi_ie */
	struct nrf_wifi_ie assoc_req_ies;

} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_COOKIE_RSP_COOKIE_VALID (1 << 0)
#define NRF_WIFI_CMD_COOKIE_RSP_MAC_ADDR_VALID (1 << 1)

/**
 * @brief This structure specifies the cookie response event, which is used to receive an
 *  RPU cookie associated with the host cookie passed during NRF_WIFI_UMAC_CMD_FRAME.
 *
 */

struct nrf_wifi_umac_event_cookie_rsp {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate if assoc_req ies is valid */
	unsigned int valid_fields;
	/** Identifier passed during NRF_WIFI_UMAC_CMD_FRAME */
	unsigned long long host_cookie;
	/** Cookie used to indicate TX done in NRF_WIFI_UMAC_EVENT_FRAME_TX_STATUS */
	unsigned long long cookie;
	/** Mac address */
	unsigned char mac_addr[NRF_WIFI_ETH_ADDR_LEN];

} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the event that corresponds to the command
 *  NRF_WIFI_UMAC_CMD_GET_TX_POWER. It is used to retrieve the transmit power
 *  information from the device
 *
 */

struct nrf_wifi_umac_event_get_tx_power {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Tx power in dbm */
	signed int txpwr_level;

} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the response to the command NRF_WIFI_UMAC_CMD_SET_INTERFACE.
 *  It contains the necessary information indicating the result or status of the interface
 *  configuration operation after the command has been executed.
 *
 */

struct nrf_wifi_umac_event_set_interface {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** return value */
	signed int return_value;
} __NRF_WIFI_PKD;

/**
 * @brief channel flags.
 *
 * Channel flags set by the regulatory control code.
 *
 */

enum nrf_wifi_channel_flags {
	/** This channel is disabled */
	CHAN_DISABLED     = 1<<0,
	/** do not initiate radiation, this includes sending probe requests or beaconing */
	CHAN_NO_IR        = 1<<1,
	/** Radar detection is required on this channel hole at 1<<2 */
	CHAN_RADAR        = 1<<3,
	/** extension channel above this channel is not permitted */
	CHAN_NO_HT40PLUS  = 1<<4,
	/** extension channel below this channel is not permitted */
	CHAN_NO_HT40MINUS = 1<<5,
	/** OFDM is not allowed on this channel */
	CHAN_NO_OFDM      = 1<<6,
	/** If the driver supports 80 MHz on the band,
	 * this flag indicates that an 80 MHz channel cannot use this
	 * channel as the control or any of the secondary channels.
	 * This may be due to the driver or due to regulatory bandwidth
	 * restrictions.
	 */
	CHAN_NO_80MHZ     = 1<<7,
	/** If the driver supports 160 MHz on the band,
	 * this flag indicates that an 160 MHz channel cannot use this
	 * channel as the control or any of the secondary channels.
	 * This may be due to the driver or due to regulatory bandwidth
	 * restrictions.
	 */
	CHAN_NO_160MHZ    = 1<<8,
	/** @ref NL80211_FREQUENCY_ATTR_INDOOR_ONLY */
	CHAN_INDOOR_ONLY  = 1<<9,
	/** @ref NL80211_FREQUENCY_ATTR_GO_CONCURRENT */
	CHAN_GO_CONCURRENT    = 1<<10,
	/** 20 MHz bandwidth is not permitted on this channel */
	CHAN_NO_20MHZ     = 1<<11,
	/** 10 MHz bandwidth is not permitted on this channel */
	CHAN_NO_10MHZ     = 1<<12,
};

/**
 * @brief channel definition.
 *
 */

struct nrf_wifi_chan_definition {
	/** Frequency of the selected channel in MHz */
	struct nrf_wifi_channel chan;
	/** channel width */
	signed int width;
	/** center frequency of first segment */
	unsigned int center_frequency1;
	/** center frequency of second segment (only with 80+80 MHz) */
	unsigned int center_frequency2;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents channel information and serves as the event for the
 *  command NRF_WIFI_UMAC_CMD_GET_CHANNEL.
 *
 */
struct nrf_wifi_umac_event_get_channel {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Channel information.@ref nrf_wifi_chan_definition */
	struct nrf_wifi_chan_definition chan_def;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the command used to retrieve connection information.
 *
 */
struct nrf_wifi_umac_cmd_conn_info {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
} __NRF_WIFI_PKD;

enum link_mode {
	NRF_WIFI_MODE_11B = 1,
	NRF_WIFI_MODE_11A,
	NRF_WIFI_MODE_11G,
	NRF_WIFI_MODE_11N,
	NRF_WIFI_MODE_11AC,
	NRF_WIFI_MODE_11AX
};

/**
 * @brief This structure represents the information related to the connection of a station.
 *
 */

struct nrf_wifi_umac_event_conn_info {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Beacon interval */
	unsigned short beacon_interval;
	/** DTIM interval */
	unsigned char dtim_interval;
	/** Station association state */
	unsigned char associated;
	/** TWT supported or not */
	unsigned char twt_capable;
	/** Refer &enum link_mode */
	unsigned char linkmode;
} __NRF_WIFI_PKD;


/**
 * @brief This structure defines the command used to retrieve power save information.
 *
 */
struct nrf_wifi_umac_cmd_get_power_save_info {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to set the listen interval period.
 *  It determines how frequently a device wakes up to check for any pending data or traffic
 *  from the access point. By setting the listen interval, devices can adjust their power-saving
 *  behavior to balance power efficiency and responsiveness to incoming data.
 *
 */
struct nrf_wifi_umac_cmd_set_listen_interval {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** listen interval */
	unsigned short listen_interval;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the command used to enable or disable extended power save mode.
 *  When enabled, the RPU wakes up based on the listen interval, allowing the device to spend more
 *  time in a lower power state. When disabled, the RPU wakes up based on the DTIM period, which
 *  may require more frequent wake-ups but can provide better responsiveness for receiving
 *  multicast/broadcast traffic.
 *
 */
struct nrf_wifi_umac_cmd_config_extended_ps {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** 1=enable 0=disable */
	unsigned char enable_extended_ps;
} __NRF_WIFI_PKD;

#define NRF_WIFI_MAX_TWT_FLOWS 8
#define NRF_WIFI_PS_MODE_LEGACY 0
#define NRF_WIFI_PS_MODE_WMM 1

/**
 * @brief Given that most APs typically use a DTIM value of 3,
 *  we anticipate a minimum listen interval of 3 beacon intervals.
 *
 */
#define NRF_WIFI_LISTEN_INTERVAL_MIN 3

/**
 * @brief This structure represents an event that provides information about the RPU power save
 *  mode. It contains details regarding the current power save mode and its settings.
 *
 */
struct nrf_wifi_umac_event_power_save_info {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Power save mode. NRF_WIFI_PS_MODE_LEGACY/NRF_WIFI_PS_MODE_WMM */
	unsigned char ps_mode;
	/** Power save enable flag */
	unsigned char enabled;
	/** Extended power save ON(1)/OFF(0) */
	unsigned char extended_ps;
	/** Is TWT responder */
	unsigned char twt_responder;
	/** Power save timed out value */
	unsigned int ps_timeout;
	/** Listen interval value */
	unsigned short listen_interval;
	/** Number TWT flows */
	unsigned char num_twt_flows;
	/** TWT info of each flow @ref nrf_wifi_umac_config_twt_info */
	struct nrf_wifi_umac_config_twt_info twt_flow_info[0];
} __NRF_WIFI_PKD;

#define NRF_WIFI_EVENT_TRIGGER_SCAN_IE_VALID (1 << 0)
#define NRF_WIFI_EVENT_TRIGGER_SCAN_SCAN_FLAGS_VALID (1 << 1)
/**
 * @brief This structure contains information relevant to the "Remain on Channel" operation.
 *  It is used to specify the details related to the duration and channel on which a device
 *  needs to stay without regular data transmission or reception.
 *
 */

struct remain_on_channel_info {
	/** Amount of time to remain on specified channel */
	unsigned int dur;
	/** Frequency configuration, see &struct freq_params */
	struct freq_params nrf_wifi_freq_params;
	/** Identifier to be used for processing NRF_WIFI_UMAC_EVENT_COOKIE_RESP event */
	unsigned long long host_cookie;
	/** Unused */
	unsigned long long cookie;

} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_ROC_FREQ_PARAMS_VALID (1 << 0)
#define NRF_WIFI_CMD_ROC_DURATION_VALID (1 << 1)
/**
 * @brief This structure represents the command used to keep the device awake on the specified
 *  channel for a designated period. The command initiates the "Remain on Channel" operation.
 *
 */

struct nrf_wifi_umac_cmd_remain_on_channel {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Information about channel parameters.@ref remain_on_channel_info */
	struct remain_on_channel_info info;

} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_CANCEL_ROC_COOKIE_VALID (1 << 0)
/**
 * @brief This structure represents the command to cancel "Remain on Channel" operation.
 *
 */
struct nrf_wifi_umac_cmd_cancel_remain_on_channel {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** cookie to identify remain on channel */
	unsigned long long cookie;
} __NRF_WIFI_PKD;

#define NRF_WIFI_EVENT_ROC_FREQ_VALID (1 << 0)
#define NRF_WIFI_EVENT_ROC_COOKIE_VALID (1 << 1)
#define NRF_WIFI_EVENT_ROC_DURATION_VALID (1 << 2)
#define NRF_WIFI_EVENT_ROC_CH_TYPE_VALID (1 << 3)
/**
 * @brief This structure represents the response to command "Remain on Channel".
 *
 */

struct nrf_wifi_event_remain_on_channel {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Frequency of the channel */
	unsigned int frequency;
	/** duration that can be requested with the remain-on-channel operation(ms) */
	unsigned int dur;
	/** see &enum nrf_wifi_channel_type */
	unsigned int ch_type;
	/** cookie to identify remain on channel */
	unsigned long long cookie;
} __NRF_WIFI_PKD;

/**
 * @brief This structure defines the command used to retrieve interface information.
 *
 */
struct nrf_wifi_cmd_get_interface {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
} __NRF_WIFI_PKD;

#define NRF_WIFI_INTERFACE_INFO_CHAN_DEF_VALID (1 << 0)
#define NRF_WIFI_INTERFACE_INFO_SSID_VALID (1 << 1)
#define NRF_WIFI_INTERFACE_INFO_IFNAME_VALID (1 << 2)

/**
 * @brief This structure represents an event that contains information about a network interface.
 *
 */

struct nrf_wifi_interface_info {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Interface type, see &enum nrf_wifi_iftype */
	signed int nrf_wifi_iftype;
	/** Interface name */
	signed char ifacename[IFACENAMSIZ];
	/** Mac address */
	unsigned char nrf_wifi_eth_addr[NRF_WIFI_ETH_ADDR_LEN];
	/** @ref nrf_wifi_chan_definition */
	struct nrf_wifi_chan_definition chan_def;
	/** @ref nrf_wifi_ssid */
	struct nrf_wifi_ssid ssid;
} __NRF_WIFI_PKD;

#define NRF_WIFI_HT_MCS_MASK_LEN 10
#define NRF_WIFI_HT_MCS_RES_LEN 3

/**
 * @brief MCS information.
 *
 */
struct nrf_wifi_event_mcs_info {
	/** Highest supported RX rate */
	unsigned short nrf_wifi_rx_highest;
	/** RX mask */
	unsigned char nrf_wifi_rx_mask[NRF_WIFI_HT_MCS_MASK_LEN];
	/** TX parameters */
	unsigned char nrf_wifi_tx_params;
	/** reserved */
	unsigned char nrf_wifi_reserved[NRF_WIFI_HT_MCS_RES_LEN];
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents HT capability parameters.
 *
 */
struct nrf_wifi_event_sta_ht_cap {
	/** 1 indicates HT Supported */
	signed int nrf_wifi_ht_supported;
	/** HT capabilities, as in the HT information IE */
	unsigned short nrf_wifi_cap;
	/** MCS information. @ref nrf_wifi_event_mcs_info */
	struct nrf_wifi_event_mcs_info mcs;
	/** A-MPDU factor, as in 11n */
	unsigned char nrf_wifi_ampdu_factor;
	/** A-MPDU density, as in 11n */
	unsigned char nrf_wifi_ampdu_density;
} __NRF_WIFI_PKD;

#define NRF_WIFI_CHAN_FLAG_FREQUENCY_ATTR_NO_IR (1 << 0)
#define NRF_WIFI_CHAN_FLAG_FREQUENCY_ATTR_NO_IBSS (1 << 1)
#define NRF_WIFI_CHAN_FLAG_FREQUENCY_ATTR_RADAR (1 << 2)
#define NRF_WIFI_CHAN_FLAG_FREQUENCY_ATTR_NO_HT40_MINUS (1 << 3)
#define NRF_WIFI_CHAN_FLAG_FREQUENCY_ATTR_NO_HT40_PLUS (1 << 4)
#define NRF_WIFI_CHAN_FLAG_FREQUENCY_ATTR_NO_80MHZ (1 << 5)
#define NRF_WIFI_CHAN_FLAG_FREQUENCY_ATTR_NO_160MHZ (1 << 6)
#define NRF_WIFI_CHAN_FLAG_FREQUENCY_ATTR_INDOOR_ONLY (1 << 7)
#define NRF_WIFI_CHAN_FLAG_FREQUENCY_ATTR_GO_CONCURRENT (1 << 8)
#define NRF_WIFI_CHAN_FLAG_FREQUENCY_ATTR_NO_20MHZ (1 << 9)
#define NRF_WIFI_CHAN_FLAG_FREQUENCY_ATTR_NO_10MHZ (1 << 10)
#define NRF_WIFI_CHAN_FLAG_FREQUENCY_DISABLED (1 << 11)

#define NRF_WIFI_CHAN_DFS_VALID (1 << 12)
#define NRF_WIFI_CHAN_DFS_CAC_TIME_VALID (1 << 13)

/**
 * @brief This structure represents channel parameters.
 */
struct nrf_wifi_event_channel {
	/** channel flags NRF_WIFI_CHAN_FLAG_FREQUENCY_ATTR_NO_IBSS */
	unsigned short nrf_wifi_flags;
	/** maximum transmission power (in dBm) */
	signed int nrf_wifi_max_power;
	/** DFS state time */
	unsigned int nrf_wifi_time;
	/** DFS CAC time in ms */
	unsigned int dfs_cac_msec;
	/** Channel parameters are valid or not 1=valid */
	signed char ch_valid;
	/** Channel center frequency */
	unsigned short center_frequency;
	/** Current dfs state */
	signed char dfs_state;
} __NRF_WIFI_PKD;

#define NRF_WIFI_EVENT_GET_WIPHY_FLAG_RATE_SHORT_PREAMBLE (1 << 0)
/**
 * @brief This structure represents rate information.
 */
struct nrf_wifi_event_rate {
	/** NRF_WIFI_EVENT_GET_WIPHY_FLAG_RATE_SHORT_PREAMBLE */
	unsigned short nrf_wifi_flags;
	/** Bitrate in units of 100 kbps */
	unsigned short nrf_wifi_bitrate;
} __NRF_WIFI_PKD;
/**
 * @brief VHT MCS information.
 *
 */

struct nrf_wifi_event_vht_mcs_info {
	/** RX MCS map 2 bits for each stream, total 8 streams */
	unsigned short rx_mcs_map;
	/** Indicates highest long GI VHT PPDU data rate
	 *  STA can receive. Rate expressed in units of 1 Mbps.
	 *  If this field is 0 this value should not be used to
	 *  consider the highest RX data rate supported.
	 */
	unsigned short rx_highest;
	/** TX MCS map 2 bits for each stream, total 8 streams */
	unsigned short tx_mcs_map;
	/** Indicates highest long GI VHT PPDU data rate
	 *  STA can transmit. Rate expressed in units of 1 Mbps.
	 *  If this field is 0 this value should not be used to
	 *  consider the highest TX data rate supported.
	 */
	unsigned short tx_highest;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents VHT capability parameters.
 *
 */
struct nrf_wifi_event_sta_vht_cap {
	/** 1 indicates VHT Supported */
	signed char nrf_wifi_vht_supported;
	/** VHT capability info */
	unsigned int nrf_wifi_cap;
	/** Refer @ref nrf_wifi_event_vht_mcs_info */
	struct nrf_wifi_event_vht_mcs_info vht_mcs;
} __NRF_WIFI_PKD;

/**
 * @brief Frequency band information.
 *
 */
struct nrf_wifi_event_supported_band {
	/** No.of channels */
	unsigned short nrf_wifi_n_channels;
	/** No.of bitrates */
	unsigned short nrf_wifi_n_bitrates;
	/** Array of channels the hardware can operate in this band */
	struct nrf_wifi_event_channel channels[29];
	/** Array of bitrates the hardware can operate with in this band */
	struct nrf_wifi_event_rate bitrates[13];
	/** HT capabilities in this band */
	struct nrf_wifi_event_sta_ht_cap ht_cap;
	/** VHT capabilities in this band */
	struct nrf_wifi_event_sta_vht_cap vht_cap;
	/** the band this structure represents */
	signed char band;
} __NRF_WIFI_PKD;

/**
 * @brief Interface limits.
 *
 */
struct nrf_wifi_event_iface_limit {
	/** max interface limits */
	unsigned short nrf_wifi_max;
	/** types */
	unsigned short nrf_wifi_types;
} __NRF_WIFI_PKD;


#define NRF_WIFI_EVENT_GET_WIPHY_VALID_RADAR_DETECT_WIDTHS (1 << 0)
#define NRF_WIFI_EVENT_GET_WIPHY_VALID_RADAR_DETECT_REGIONS (1 << 1)
#define NRF_WIFI_EVENT_GET_WIPHY_VALID_ (1 << 2)
/**
 * @brief This structure defines an event that represents interface combinations.
 *
 */
struct nrf_wifi_event_iface_combination {
	/** channels count */
	unsigned int nrf_wifi_num_different_channels;
	/** Unused */
	signed int beacon_int_infra_match;
	/** @ref nrf_wifi_event_iface_limit */
	struct nrf_wifi_event_iface_limit limits[2];
	/** Max interfaces */
	unsigned short nrf_wifi_max_interfaces;
	/** Not used */
	unsigned char nrf_wifi_radar_detect_widths;
	/** Not used */
	unsigned char nrf_wifi_n_limits;
	/** Not used */
	unsigned char nrf_wifi_radar_detect_regions;
	/** Not used */
	unsigned char comb_valid;
} __NRF_WIFI_PKD;

#define NRF_WIFI_EVENT_GET_WIPHY_IBSS_RSN (1 << 0)
#define NRF_WIFI_EVENT_GET_WIPHY_MESH_AUTH (1 << 1)
#define NRF_WIFI_EVENT_GET_WIPHY_AP_UAPSD (1 << 2)
#define NRF_WIFI_EVENT_GET_WIPHY_SUPPORTS_FW_ROAM (1 << 3)
#define NRF_WIFI_EVENT_GET_WIPHY_SUPPORTS_TDLS (1 << 4)
#define NRF_WIFI_EVENT_GET_WIPHY_TDLS_EXTERNAL_SETUP (1 << 5)
#define NRF_WIFI_EVENT_GET_WIPHY_CONTROL_PORT_ETHERTYPE (1 << 6)
#define NRF_WIFI_EVENT_GET_WIPHY_OFFCHANNEL_TX_OK (1 << 7)

#define NRF_WIFI_GET_WIPHY_VALID_PROBE_RESP_OFFLOAD (1 << 0)
#define NRF_WIFI_GET_WIPHY_VALID_TX_ANT (1 << 1)
#define NRF_WIFI_GET_WIPHY_VALID_RX_ANT (1 << 2)
#define NRF_WIFI_GET_WIPHY_VALID_MAX_NUM_SCAN_SSIDS (1 << 3)
#define NRF_WIFI_GET_WIPHY_VALID_NUM_SCHED_SCAN_SSIDS (1 << 4)
#define NRF_WIFI_GET_WIPHY_VALID_MAX_MATCH_SETS (1 << 5)
#define NRF_WIFI_GET_WIPHY_VALID_MAC_ACL_MAX (1 << 6)
#define NRF_WIFI_GET_WIPHY_VALID_HAVE_AP_SME (1 << 7)
#define NRF_WIFI_GET_WIPHY_VALID_EXTENDED_CAPABILITIES (1 << 8)
#define NRF_WIFI_GET_WIPHY_VALID_MAX_AP_ASSOC_STA (1 << 9)
#define NRF_WIFI_GET_WIPHY_VALID_WIPHY_NAME (1 << 10)
#define NRF_WIFI_GET_WIPHY_VALID_EXTENDED_FEATURES (1 << 11)

#define NRF_WIFI_EVENT_GET_WIPHY_MAX_CIPHER_COUNT 30

#define NRF_WIFI_INDEX_IDS_WIPHY_NAME 32
#define NRF_WIFI_EVENT_GET_WIPHY_NUM_BANDS 2

#define EXTENDED_FEATURE_LEN 60
#define DIV_ROUND_UP_NL(n, d) (((n) + (d)-1) / (d))

/**
 * @brief This structure represents wiphy parameters.
 *
 */
struct nrf_wifi_event_get_wiphy {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Unused */
	unsigned int nrf_wifi_frag_threshold;
	/** RTS threshold value */
	unsigned int nrf_wifi_rts_threshold;
	/** Unused */
	unsigned int nrf_wifi_available_antennas_tx;
	/** Unused */
	unsigned int nrf_wifi_available_antennas_rx;
	/** Unused */
	unsigned int nrf_wifi_probe_resp_offload;
	/** Unused */
	unsigned int tx_ant;
	/** Unused */
	unsigned int rx_ant;
	/** Unused */
	unsigned int split_start2_flags;
	/** Maximum ROC duration */
	unsigned int max_remain_on_channel_duration;
	/** Unused */
	unsigned int ap_sme_capa;
	/** Unused */
	unsigned int features;
	/** Unused */
	unsigned int max_acl_mac_addresses;
	/** maximum number of associated stations supported in AP mode */
	unsigned int max_ap_assoc_sta;
	/** supported cipher suites */
	unsigned int cipher_suites[NRF_WIFI_EVENT_GET_WIPHY_MAX_CIPHER_COUNT];
	/** wiphy flags NRF_WIFI_EVENT_GET_WIPHY_AP_UAPSD */
	unsigned int get_wiphy_flags;
	/** valid parameters NRF_WIFI_GET_WIPHY_VALID_WIPHY_NAME */
	unsigned int params_valid;
	/** Maximum scan IE length */
	unsigned short int max_scan_ie_len;
	/** Unused */
	unsigned short int max_sched_scan_ie_len;
	/** bit mask of interface value of see &enum nrf_wifi_iftype */
	unsigned short int interface_modes;
	/** Unused */
	struct nrf_wifi_event_iface_combination iface_com[6];
	/** Unused */
	signed char supp_commands[40];
	/** Retry limit for short frames */
	unsigned char retry_short;
	/** Retry limit for long frames */
	unsigned char retry_long;
	/** Unused */
	unsigned char coverage_class;
	/** Maximum ssids supported in scan */
	unsigned char max_scan_ssids;
	/** Unused */
	unsigned char max_sched_scan_ssids;
	/** Unused */
	unsigned char max_match_sets;
	/** Unused */
	unsigned char n_cipher_suites;
	/** Unused */
	unsigned char max_num_pmkids;
	/** length of the extended capabilities */
	unsigned char extended_capabilities_len;
	/** Extended capabilities */
	unsigned char extended_capabilities[10];
	/** Extended capabilities mask */
	unsigned char extended_capabilities_mask[10];
	/** Unused */
	unsigned char ext_features[DIV_ROUND_UP_NL(EXTENDED_FEATURE_LEN, 8)];
	/** Unused */
	unsigned char ext_features_len;
	/** Unused */
	signed char num_iface_com;
	/** Wiphy name */
	signed char wiphy_name[NRF_WIFI_INDEX_IDS_WIPHY_NAME];
	/** Supported bands info. @ref nrf_wifi_event_supported_band */
	struct nrf_wifi_event_supported_band sband[NRF_WIFI_EVENT_GET_WIPHY_NUM_BANDS];
} __NRF_WIFI_PKD;
/**
 * @brief This structure represents the command used to retrieve Wireless PHY (wiphy) information.
 *
 */
struct nrf_wifi_cmd_get_wiphy {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the command to get hardware address.
 *
 */
struct nrf_wifi_cmd_get_ifhwaddr {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Interface name */
	signed char ifacename[IFACENAMSIZ];
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the command used to retrieve the hardware address or
 *  MAC address of the device.
 *
 */
struct nrf_wifi_cmd_set_ifhwaddr {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Interface name */
	signed char ifacename[IFACENAMSIZ];
	/** Hardware address to be set */
	unsigned char nrf_wifi_hwaddr[NRF_WIFI_ETH_ADDR_LEN];
} __NRF_WIFI_PKD;

#define REG_RULE_FLAGS_VALID (1 << 0)
#define FREQ_RANGE_START_VALID (1 << 1)
#define FREQ_RANGE_END_VALID (1 << 2)
#define FREQ_RANGE_MAX_BW_VALID (1 << 3)
#define POWER_RULE_MAX_EIRP_VALID (1 << 4)

#define NRF_WIFI_RULE_FLAGS_NO_OFDM (1<<0)
#define NRF_WIFI_RULE_FLAGS_NO_CCK (1<<1)
#define NRF_WIFI_RULE_FLAGS_NO_INDOOR (1<<2)
#define NRF_WIFI_RULE_FLAGS_NO_OUTDOOR (1<<3)
#define NRF_WIFI_RULE_FLAGS_DFS (1<<4)
#define NRF_WIFI_RULE_FLAGS_PTP_ONLY  (1<<5)
#define NRF_WIFI_RULE_FLAGS_PTMP_ONLY (1<<6)
#define NRF_WIFI_RULE_FLAGS_NO_IR (1<<7)
#define NRF_WIFI_RULE_FLAGS_IBSS (1<<8)
#define NRF_WIFI_RULE_FLAGS_AUTO_BW (1<<11)
#define NRF_WIFI_RULE_FLAGS_IR_CONCURRENT (1<<12)
#define NRF_WIFI_RULE_FLAGS_NO_HT40MINUS (1<<13)
#define NRF_WIFI_RULE_FLAGS_NO_HT40PLUS (1<<14)
#define NRF_WIFI_RULE_FLAGS_NO_80MHZ (1<<15)
#define NRF_WIFI_RULE_FLAGS_NO_160MHZ (1<<16)

/**
 * @brief This structure represents the information related to the regulatory domain
 *  of a wireless device. The regulatory domain defines the specific rules and regulations
 *  that govern the usage of radio frequencies in a particular geographical region.
 *
 */

struct nrf_wifi_reg_rules {
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** NRF_WIFI_RULE_FLAGS_NO_CCK and NRF_WIFI_RULE_FLAGS_NO_INDOOR */
	unsigned int rule_flags;
	/** starting frequencry for the regulatory rule in KHz */
	unsigned int freq_range_start;
	/** ending frequency for the regulatory rule in KHz */
	unsigned int freq_range_end;
	/** maximum allowed bandwidth for this frequency range */
	unsigned int freq_range_max_bw;
	/** maximum allowed EIRP mBm (100 * dBm) */
	unsigned int pwr_max_eirp;

} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_SET_REG_ALPHA2_VALID (1 << 0)
#define NRF_WIFI_CMD_SET_REG_RULES_VALID (1 << 1)
#define NRF_WIFI_CMD_SET_REG_DFS_REGION_VALID (1 << 2)

#define MAX_NUM_REG_RULES 32

/**
 * @brief This structure represents an event that contains regulatory domain information.
 *
 */

struct nrf_wifi_reg {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** region for regulatory */
	unsigned int dfs_region;
	/** No.of regulatory rules */
	unsigned int num_reg_rules;
	/** rules info. @ref nrf_wifi_reg_rules */
	struct nrf_wifi_reg_rules nrf_wifi_reg_rules[MAX_NUM_REG_RULES];
	/** Country code */
	unsigned char nrf_wifi_alpha2[NRF_WIFI_COUNTRY_CODE_LEN];
} __NRF_WIFI_PKD;

#define NRF_WIFI_CMD_REQ_SET_REG_ALPHA2_VALID (1 << 0)
#define NRF_WIFI_CMD_REQ_SET_REG_USER_REG_HINT_TYPE_VALID (1 << 1)
#define NRF_WIFI_CMD_REQ_SET_REG_USER_REG_FORCE (1 << 2)
/**
 * @brief This structure represents the command used to set the regulatory domain
 *  for a wireless device. It allows configuring the device to adhere to the rules
 *  and regulations specific to the geographical region in which it is operating.
 *
 */
struct nrf_wifi_cmd_req_set_reg {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Indicate which of the following parameters are valid */
	unsigned int valid_fields;
	/** Type of regulatory hint passed from userspace */
	unsigned int nrf_wifi_user_reg_hint_type;
	/** Country code */
	unsigned char nrf_wifi_alpha2[NRF_WIFI_COUNTRY_CODE_LEN];
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the status code for a command. It is used to indicate
 *  the outcome or result of executing a specific command. The status code provides valuable
 *  information about the success, failure, or any errors encountered during the execution
 *  of the command, helping to understand the current state of the device.
 *
 */
struct nrf_wifi_umac_event_cmd_status {
	/** Header @ref nrf_wifi_umac_hdr */
	struct nrf_wifi_umac_hdr umac_hdr;
	/** Command id. see &enum nrf_wifi_umac_commands */
	unsigned int cmd_id;
	/** Status codes */
	unsigned int cmd_status;
} __NRF_WIFI_PKD;

#endif /* __HOST_RPU_UMAC_IF_H */
