/*
 *
 *Copyright (c) 2022 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 *
 *@brief <Control interface between host and RPU>
 */
#ifndef __HOST_RPU_UMAC_IF_H
#define __HOST_RPU_UMAC_IF_H

#include "host_rpu_data_if.h"
#include "host_rpu_sys_if.h"

#include "pack_def.h"

#define MAX_IMG_UMAC_CMD_SIZE 400

/**
 * enum img_umac_commands - Commands that the host can send to the RPU.
 *
 * @IMG_UMAC_CMD_TRIGGER_SCAN: Trigger a new scan with the given parameters.
 *	See &struct img_umac_cmd_scan
 * @IMG_UMAC_CMD_GET_SCAN_RESULTS: Request for scan results.
 *	See &struct img_umac_cmd_get_scan_results
 * @IMG_UMAC_CMD_AUTHENTICATE: Send authentication request to AP.
 *	See &struct img_umac_cmd_auth
 * @IMG_UMAC_CMD_ASSOCIATE: Send associate request to AP.
 *	See &struct img_umac_cmd_assoc
 * @IMG_UMAC_CMD_DEAUTHENTICATE: Send deauthentication request to AP.
 *	See &struct img_umac_cmd_disconn
 * @IMG_UMAC_CMD_SET_WIPHY: Set wiphy parameters.
 *	See &struct img_umac_cmd_set_wiphy
 * @IMG_UMAC_CMD_NEW_KEY: Add new key.
 *	See &struct img_umac_cmd_key
 * @IMG_UMAC_CMD_DEL_KEY: Delete crypto key.
 *	See &struct img_umac_cmd_key
 * @IMG_UMAC_CMD_SET_KEY: Set default key to use.
 *	See &struct img_umac_cmd_set_key
 * @IMG_UMAC_CMD_GET_KEY: Unused.
 * @IMG_UMAC_CMD_NEW_BEACON: Set the beacon fields in AP mode.
 *	See &struct img_umac_cmd_start_ap
 * @IMG_UMAC_CMD_SET_BEACON: Set the beacon fields in AP mode.
 *	See &struct img_umac_cmd_set_beacon
 * @IMG_UMAC_CMD_SET_BSS: Set the BSS.
 *	See &struct img_umac_cmd_set_bss
 * @IMG_UMAC_CMD_START_AP: Start the device as Soft AP.
 *	See &struct img_umac_cmd_start_ap
 * @IMG_UMAC_CMD_STOP_AP: Stop the AP mode.
 *	See &struct img_umac_cmd_stop_ap
 * @IMG_UMAC_CMD_NEW_INTERFACE: Adding interface.
 *	See &struct img_umac_cmd_add_vif
 * @IMG_UMAC_CMD_SET_INTERFACE: Change interface configuration.
 *	See &struct img_umac_cmd_chg_vif_attr
 * @IMG_UMAC_CMD_DEL_INTERFACE: Delete interface.
 *	See &struct img_umac_cmd_del_vif
 * @IMG_UMAC_CMD_SET_IFFLAGS: Change interface flags.
 *	See &struct img_umac_cmd_chg_vif_state
 * @IMG_UMAC_CMD_NEW_STATION: Add a new station.
 *	See &struct img_umac_cmd_add_sta
 * @IMG_UMAC_CMD_DEL_STATION: Delete station.
 *	See &struct img_umac_cmd_del_sta
 * @IMG_UMAC_CMD_SET_STATION: Change station info.
 *	See &struct img_umac_cmd_chg_sta
 * @IMG_UMAC_CMD_GET_STATION: Get station info.
 *	See &struct img_umac_cmd_get_sta
 * @IMG_UMAC_CMD_START_P2P_DEVICE: Start the P2P device.
 *	See &struct img_umac_cmd_start_p2p_dev
 * @IMG_UMAC_CMD_STOP_P2P_DEVICE: Stop the P2P device.
 *	See &struct img_umac_cmd_stop_p2p_dev
 * @IMG_UMAC_CMD_REMAIN_ON_CHANNEL: Unused.
 * @IMG_UMAC_CMD_CANCEL_REMAIN_ON_CHANNEL: Unused.
 * @IMG_UMAC_CMD_SET_CHANNEL: Unused.
 * @IMG_UMAC_CMD_RADAR_DETECT: Unused.
 * @IMG_UMAC_CMD_REGISTER_FRAME: Whitelist filter based on frame types.
 *	See &struct img_umac_cmd_mgmt_frame_reg
 * @IMG_UMAC_CMD_FRAME: Send a management frame.
 *	See &struct img_umac_cmd_mgmt_tx
 * @IMG_UMAC_CMD_JOIN_IBSS: Unused.
 * @IMG_UMAC_CMD_WIN_STA_CONNECT: Connect to AP.
 *	See &struct img_umac_cmd_win_sta_connect
 * @IMG_UMAC_CMD_SET_POWER_SAVE: Power save Enable/Disable
 *	See &struct img_umac_cmd_set_power_save
 * @IMG_UMAC_CMD_SET_WOWLAN: Set the WoWLAN trigger configs
 *	See &struct img_umac_cmd_set_wowlan
 * @IMG_UMAC_CMD_SUSPEND: Suspend the bus after WoWLAN configurations
 *	See &struct img_umac_cmd_suspend
 * @IMG_UMAC_CMD_RESUME: Resume the bus activity before wakeup
 *	See &struct img_umac_cmd_resume
 * @IMG_UMAC_CMD_GET_CHANNEL: Get Channel info
 *		See &struct img_umac_cmd_get_channel
 * @IMG_UMAC_CMD_GET_TX_POWER: Get Tx power level
 *		See &struct img_umac_cmd_get_tx_power
 * @IMG_UMAC_CMD_GET_REG : Get Regulatory info
 *		See &struct img_reg_t
 * @IMG_UMAC_CMD_SET_REG : Set Regulatory info
 *		See &struct img_reg_t
 *
 * Lists the different ID's to be used to when sending a command to the RPU.
 * All the commands are to be encapsulated using struct host_rpu_msg.
 */
#define IMG_UMAC_CMD_TRIGGER_SCAN 0
#define IMG_UMAC_CMD_GET_SCAN_RESULTS 1
#define IMG_UMAC_CMD_AUTHENTICATE 2
#define IMG_UMAC_CMD_ASSOCIATE 3
#define IMG_UMAC_CMD_DEAUTHENTICATE 4
#define IMG_UMAC_CMD_SET_WIPHY 5
#define IMG_UMAC_CMD_NEW_KEY 6
#define IMG_UMAC_CMD_DEL_KEY 7
#define IMG_UMAC_CMD_SET_KEY 8
#define IMG_UMAC_CMD_GET_KEY 9
#define IMG_UMAC_CMD_NEW_BEACON 10
#define IMG_UMAC_CMD_SET_BEACON 11
#define IMG_UMAC_CMD_SET_BSS 12
#define IMG_UMAC_CMD_START_AP 13
#define IMG_UMAC_CMD_STOP_AP 14
#define IMG_UMAC_CMD_NEW_INTERFACE 15
#define IMG_UMAC_CMD_SET_INTERFACE 16
#define IMG_UMAC_CMD_DEL_INTERFACE 17
#define IMG_UMAC_CMD_SET_IFFLAGS 18
#define IMG_UMAC_CMD_NEW_STATION 19 /*Add a STA in SoftAP mode*/
#define IMG_UMAC_CMD_DEL_STATION 20
#define IMG_UMAC_CMD_SET_STATION 21
#define IMG_UMAC_CMD_GET_STATION 22
#define IMG_UMAC_CMD_START_P2P_DEVICE 23
#define IMG_UMAC_CMD_STOP_P2P_DEVICE 24
#define IMG_UMAC_CMD_REMAIN_ON_CHANNEL 25
#define IMG_UMAC_CMD_CANCEL_REMAIN_ON_CHANNEL 26
#define IMG_UMAC_CMD_SET_CHANNEL 27
#define IMG_UMAC_CMD_RADAR_DETECT 28
#define IMG_UMAC_CMD_REGISTER_FRAME 29
#define IMG_UMAC_CMD_FRAME 30
#define IMG_UMAC_CMD_JOIN_IBSS 31
#define IMG_UMAC_CMD_WIN_STA_CONNECT 32
#define IMG_UMAC_CMD_SET_POWER_SAVE 33
#define IMG_UMAC_CMD_SET_WOWLAN 34
#define IMG_UMAC_CMD_SUSPEND 35
#define IMG_UMAC_CMD_RESUME 36
#define IMG_UMAC_CMD_SET_QOS_MAP 37
#define IMG_UMAC_CMD_GET_CHANNEL 38
#define IMG_UMAC_CMD_GET_TX_POWER 39
#define IMG_UMAC_CMD_GET_INTERFACE 40
#define IMG_UMAC_CMD_GET_WIPHY 41
#define IMG_UMAC_CMD_GET_IFHWADDR 42
#define IMG_UMAC_CMD_SET_IFHWADDR 43
#define IMG_UMAC_CMD_GET_REG 44
#define IMG_UMAC_CMD_SET_REG 45
#define IMG_UMAC_CMD_REQ_SET_REG 46
#ifdef TWT_SUPPORT
#define IMG_UMAC_CMD_CONFIG_TWT 47
#define IMG_UMAC_CMD_TEARDOWN_TWT 48
#endif
#define IMG_UMAC_CMD_CONFIG_UAPSD 49
#define IMG_UMAC_CMD_ABORT_SCAN 50

/**
 * enum img_umac_events - Events that the RPU can send to the host.
 *
 * @IMG_UMAC_EVENT_TRIGGER_SCAN_START: Unused.
 * @IMG_UMAC_EVENT_SCAN_ABORTED: Indicate scan has been cancelled.
 *	See &struct img_umac_event_trigger_scan
 * @IMG_UMAC_EVENT_SCAN_DONE: Indicate scan results are available.
 *	See &struct img_umac_event_trigger_scan
 * @IMG_UMAC_EVENT_SCAN_RESULT: Scan result. We will receive one event for all
 *	the scan results until umac_hdr->seq == 0.
 *	See &struct img_umac_event_new_scan_results
 * @IMG_UMAC_EVENT_AUTHENTICATE: Authentication status.
 *	See &struct img_umac_event_mlme
 * @IMG_UMAC_EVENT_ASSOCIATE: Association status.
 *	See &struct img_umac_event_mlme
 * @IMG_UMAC_EVENT_CONNECT: Connection complete event.
 *	See &struct img_umac_event_connect
 * @IMG_UMAC_EVENT_DEAUTHENTICATE: Station deauth event.
 *	See &struct img_umac_event_mlme
 * @IMG_UMAC_EVENT_NEW_STATION: Station added indication.
 *	See &struct img_umac_event_new_station
 * @IMG_UMAC_EVENT_DEL_STATION: Station deleted indication.
 *	See &struct img_umac_event_new_station
 * @IMG_UMAC_EVENT_GET_STATION: Station info indication.
 *	See &img_umac_event_station_t
 * @IMG_UMAC_EVENT_REMAIN_ON_CHANNEL: Unused.
 * @IMG_UMAC_EVENT_CANCEL_REMAIN_ON_CHANNEL: Unused.
 * @IMG_UMAC_EVENT_DISCONNECT: Unused.
 * @IMG_UMAC_EVENT_FRAME: RX management frame.
 *	See &struct img_umac_event_mlme
 * @IMG_UMAC_EVENT_COOKIE_RESP: Cookie mapping for IMG_UMAC_CMD_FRAME.
 *	See &struct img_umac_event_cookie_rsp
 * @IMG_UMAC_EVENT_FRAME_TX_STATUS: TX management frame transmitted.
 *	See &struct img_umac_event_mlme
 * @IMG_UMAC_EVENT_GET_CHANNEL: Send Channel info.
 *	See &struct img_umac_event_get_channel
 * @IMG_UMAC_EVENT_GET_TX_POWER: Send Tx power.
 *	See &struct img_umac_event_get_tx_power
 * @IMG_UMAC_EVENT_SET_INTERFACE: IMG_UMAC_CMD_SET_INTERFACE status.
 *	See &struct img_umac_event_set_interface
 * @IMG_UMAC_EVENT_GET_REG: IMG_UMAC_CMD_GET_REG status
 *	See &struct img_reg_t
 * @IMG_UMAC_EVENT_SET_REG: IMG_UMAC_CMD_SET_REG status
 *	See &struct img_reg_t
 * @IMG_UMAC_EVENT_REQ_SET_REG: IMG_UMAC_CMD_REQ_SET_REG status
 *	See &struct img_reg_t
 * @IMG_UMAC_EVENT_SCAN_DISPLAY_RESULT: Scan display result. We will receive one event for all
 *	the scan results until umac_hdr->seq == 0
 * Lists the ID's to used by the RPU when sending a Event to the Host. All the
 * events are encapsulated using struct host_rpu_msg.
 */
#define IMG_UMAC_EVENT_UNSPECIFIED 256
#define IMG_UMAC_EVENT_TRIGGER_SCAN_START 257
#define IMG_UMAC_EVENT_SCAN_ABORTED 258
#define IMG_UMAC_EVENT_SCAN_DONE 259
#define IMG_UMAC_EVENT_SCAN_RESULT 260
#define IMG_UMAC_EVENT_AUTHENTICATE 261
#define IMG_UMAC_EVENT_ASSOCIATE 262
#define IMG_UMAC_EVENT_CONNECT 263
#define IMG_UMAC_EVENT_DEAUTHENTICATE 264
#define IMG_UMAC_EVENT_DISASSOCIATE 265
#define IMG_UMAC_EVENT_NEW_STATION 266
#define IMG_UMAC_EVENT_DEL_STATION 267
#define IMG_UMAC_EVENT_GET_STATION 268
#define IMG_UMAC_EVENT_REMAIN_ON_CHANNEL 269
#define IMG_UMAC_EVENT_CANCEL_REMAIN_ON_CHANNEL 270
#define IMG_UMAC_EVENT_DISCONNECT 271
#define IMG_UMAC_EVENT_FRAME 272
#define IMG_UMAC_EVENT_COOKIE_RESP 273
#define IMG_UMAC_EVENT_FRAME_TX_STATUS 274
#define IMG_UMAC_EVENT_IFFLAGS_STATUS 275
#define IMG_UMAC_EVENT_GET_TX_POWER 276
#define IMG_UMAC_EVENT_GET_CHANNEL 277
#define IMG_UMAC_EVENT_SET_INTERFACE 278
#define IMG_UMAC_EVENT_UNPROT_DEAUTHENTICATE 279
#define IMG_UMAC_EVENT_UNPROT_DISASSOCIATE 280
#define IMG_UMAC_EVENT_NEW_INTERFACE 281
#define IMG_UMAC_EVENT_NEW_WIPHY 282
#define IMG_UMAC_EVENT_GET_IFHWADDR 283
#define IMG_UMAC_EVENT_GET_REG 284
#define IMG_UMAC_EVENT_SET_REG 285
#define IMG_UMAC_EVENT_REQ_SET_REG 286
#define IMG_UMAC_EVENT_GET_KEY 287
#define IMG_UMAC_EVENT_BEACON_HINT 288
#define IMG_UMAC_EVENT_REG_CHANGE 289
#define IMG_UMAC_EVENT_WIPHY_REG_CHANGE 290
#define IMG_UMAC_EVENT_SCAN_DISPLAY_RESULT 291
#define IMG_UMAC_EVENT_CMD_STATUS 292
#ifdef TWT_SUPPORT
#define IMG_UMAC_EVENT_CONFIG_TWT 293
#define IMG_UMAC_EVENT_TEARDOWN_TWT 294
#endif

/**
 * enum img_band - Frequency band.
 *
 * @IMG_BAND_2GHZ: 2.4 GHz ISM band.
 * @IMG_BAND_5GHZ: Around 5 GHz band (4.9 - 5.7 GHz).
 * @IMG_BAND_60GHZ: Unused.
 *
 * This enum represents the values that can be used to specify which frequency
 * band is used.
 */
#define IMG_BAND_2GHZ 0
#define IMG_BAND_5GHZ 1
#define IMG_BAND_60GHZ 2

/**
 * enum img_mfp - Management frame protection state.
 *
 * @IMG_MFP_NO: Management frame protection not used.
 * @IMG_MFP_REQUIRED: Management frame protection required.
 *
 * Enabling/Disabling of Management Frame Protection.
 */
#define IMG_MFP_NO 0
#define IMG_MFP_REQUIRED 1

/**
 * enum img_key_type - Key Type
 *
 * @IMG_KEYTYPE_GROUP: Group (broadcast/multicast) key
 * @IMG_KEYTYPE_PAIRWISE: Pairwise (unicast/individual) key
 * @IMG_KEYTYPE_PEERKEY: Peer key (DLS)
 * @NUM_IMG_KEYTYPES: Number of defined key types
 *
 * Lists the different categories of security keys.
 */
#define IMG_KEYTYPE_GROUP 0
#define IMG_KEYTYPE_PAIRWISE 1
#define IMG_KEYTYPE_PEERKEY 2

#define NUM_IMG_KEYTYPES 3

/**
 * enum img_auth_type - Authentication Type.
 *
 * @IMG_AUTHTYPE_OPEN_SYSTEM: Open System authentication.
 * @IMG_AUTHTYPE_SHARED_KEY: Shared Key authentication (WEP only).
 * @IMG_AUTHTYPE_FT: Fast BSS Transition (IEEE 802.11r).
 * @IMG_AUTHTYPE_NETWORK_EAP: Network EAP (some Cisco APs and mainly LEAP).
 * @IMG_AUTHTYPE_SAE: Simultaneous authentication of equals.
 * @__IMG_AUTHTYPE_NUM: Internal.
 * @IMG_AUTHTYPE_MAX: Maximum valid auth algorithm.
 * @IMG_AUTHTYPE_AUTOMATIC: Determine automatically (if necessary by
 *	trying multiple times); this is invalid in netlink -- leave out
 *	the attribute for this on CONNECT commands.
 *
 * Lists the different types of authentication mechanisms.
 */
#define IMG_AUTHTYPE_OPEN_SYSTEM 0
#define IMG_AUTHTYPE_SHARED_KEY 1
#define IMG_AUTHTYPE_FT 2
#define IMG_AUTHTYPE_NETWORK_EAP 3
#define IMG_AUTHTYPE_SAE 4

	/* keep last */
#define __IMG_AUTHTYPE_NUM 5
#define IMG_AUTHTYPE_MAX (__IMG_AUTHTYPE_NUM)
#define IMG_AUTHTYPE_AUTOMATIC 6

/**
 * enum img_hidden_ssid - Hidden SSID usage.
 * @IMG_HIDDEN_SSID_NOT_IN_USE: Do not hide SSID (i.e., broadcast it in
 *	Beacon frames).
 * @IMG_HIDDEN_SSID_ZERO_LEN: Hide SSID by using zero-length SSID element
 *	in Beacon frames.
 * @IMG_HIDDEN_SSID_ZERO_CONTENTS: Hide SSID by using correct length of SSID
 *	element in Beacon frames but zero out each byte in the SSID.
 *
 * Enable/Disable Hidden SSID feature and also lists the different mechanisms of
 * hiding the SSIDs.
 */
#define IMG_HIDDEN_SSID_NOT_IN_USE 0
#define IMG_HIDDEN_SSID_ZERO_LEN 1
#define IMG_HIDDEN_SSID_ZERO_CONTENTS 2

/**
 * enum img_smps_mode - SMPS mode.
 * @IMG_SMPS_OFF: SMPS off (use all antennas).
 * @IMG_SMPS_STATIC: Static SMPS (use a single antenna).
 * @IMG_SMPS_DYNAMIC: Dynamic smps (start with a single antenna and
 *	turn on other antennas after CTS/RTS).
 *
 * Requested SMPS mode (for AP mode).
 */
#define IMG_SMPS_OFF 0
#define IMG_SMPS_STATIC 1
#define IMG_SMPS_DYNAMIC 2

#define __IMG_SMPS_AFTER_LAST 3
#define IMG_SMPS_MAX (__IMG_SMPS_AFTER_LAST - 1)

/**
 * enum img_bss_status - BSS status.
 * @IMG_BSS_STATUS_AUTHENTICATED: Authenticated with this BSS.
 *	Note that this is no longer used since cfg80211 no longer
 *	keeps track of whether or not authentication was done with
 *	a given BSS.
 * @IMG_BSS_STATUS_ASSOCIATED: Associated with this BSS.
 * @IMG_BSS_STATUS_IBSS_JOINED: Joined to this IBSS.
 *
 * The BSS status is a BSS attribute in scan dumps, which
 * indicates the status the interface has with respect to this BSS.
 */
#define IMG_BSS_STATUS_AUTHENTICATED 0
#define IMG_BSS_STATUS_ASSOCIATED 1
#define IMG_BSS_STATUS_IBSS_JOINED 2

/**
 * enum img_channel_type - Channel type.
 * @IMG_CHAN_NO_HT: 20 MHz, non-HT channel.
 * @IMG_CHAN_HT20: 20 MHz HT channel.
 * @IMG_CHAN_HT40MINUS: HT40 channel, secondary channel
 *      below the control channel.
 * @IMG_CHAN_HT40PLUS: HT40 channel, secondary channel
 *      above the control channel.
 *
 * Lists the different categories of channels.
 */
#define IMG_CHAN_NO_HT 0
#define IMG_CHAN_HT20 1
#define IMG_CHAN_HT40MINUS 2
#define IMG_CHAN_HT40PLUS 3

/**
 * enum img_chan_width - Channel width definitions.
 *
 *
 * @IMG_CHAN_WIDTH_20_NOHT: 20 MHz, non-HT channel.
 * @IMG_CHAN_WIDTH_20: 20 MHz HT channel.
 * @IMG_CHAN_WIDTH_40: 40 MHz channel, the %IMG_ATTR_CENTER_FREQ1
 *	attribute must be provided as well.
 * @IMG_CHAN_WIDTH_80: 80 MHz channel, the %IMG_ATTR_CENTER_FREQ1
 *	attribute must be provided as well.
 * @IMG_CHAN_WIDTH_80P80: 80+80 MHz channel, the %IMG_ATTR_CENTER_FREQ1
 *	and %IMG_ATTR_CENTER_FREQ2 attributes must be provided as well.
 * @IMG_CHAN_WIDTH_160: 160 MHz channel, the %IMG_ATTR_CENTER_FREQ1
 *	attribute must be provided as well.
 * @IMG_CHAN_WIDTH_5: 5 MHz OFDM channel.
 * @IMG_CHAN_WIDTH_10: 10 MHz OFDM channel.
 *
 * Lists the different channel widths.
 */
#define IMG_CHAN_WIDTH_20_NOHT 0
#define IMG_CHAN_WIDTH_20 1
#define IMG_CHAN_WIDTH_40 2
#define IMG_CHAN_WIDTH_80 3
#define IMG_CHAN_WIDTH_80P80 4
#define IMG_CHAN_WIDTH_160 5
#define IMG_CHAN_WIDTH_5 6
#define IMG_CHAN_WIDTH_10 7

/**
 * enum img_iftype - Interface types based on functionality.
 *
 * @IMG_IFTYPE_UNSPECIFIED: Unspecified type, driver decides.
 * @IMG_IFTYPE_ADHOC: Independent BSS member.
 * @IMG_IFTYPE_STATION: Managed BSS member.
 * @IMG_IFTYPE_AP: Access point.
 * @IMG_IFTYPE_AP_VLAN: VLAN interface for access points; VLAN interfaces
 *      are a bit special in that they must always be tied to a pre-existing
 *      AP type interface.
 * @IMG_IFTYPE_WDS: Wireless Distribution System.
 * @IMG_IFTYPE_MONITOR: Monitor interface receiving all frames.
 * @IMG_IFTYPE_MESH_POINT: Mesh point.
 * @IMG_IFTYPE_P2P_CLIENT: P2P client.
 * @IMG_IFTYPE_P2P_GO: P2P group owner.
 * @IMG_IFTYPE_P2P_DEVICE: P2P device interface type, this is not a netdev
 *	and therefore can't be created in the normal ways, use the
 *	%IMG_UMAC_CMD_START_P2P_DEVICE and %IMG_UMAC_CMD_STOP_P2P_DEVICE
 *	commands (Refer &enum img_umac_commands) to create and destroy one.
 * @IMG_IFTYPE_OCB: Outside Context of a BSS.
 *	This mode corresponds to the MIB variable dot11OCBActivated=true.
 * @IMG_IFTYPE_MAX: Highest interface type number currently defined.
 * @NUM_IMG_IFTYPES: Number of defined interface types.
 *
 * Lists the different interface types based on how they are configured
 * functionally.
 */
#define IMG_IFTYPE_UNSPECIFIED 0
#define IMG_IFTYPE_ADHOC 1
#define IMG_IFTYPE_STATION 2
#define IMG_IFTYPE_AP 3
#define IMG_IFTYPE_AP_VLAN 4
#define IMG_IFTYPE_WDS 5
#define IMG_IFTYPE_MONITOR 6
#define IMG_IFTYPE_MESH_POINT 7
#define IMG_IFTYPE_P2P_CLIENT 8
#define IMG_IFTYPE_P2P_GO 9
#define IMG_IFTYPE_P2P_DEVICE 10
#define IMG_IFTYPE_OCB 11

/* keep last */
#define NUM_IMG_IFTYPES 12
#define IMG_IFTYPE_MAX (NUM_IMG_IFTYPES - 1)

/**
 * enum img_ps_state - powersave state
 * @IMG_PS_DISABLED: powersave is disabled
 * @IMG_PS_ENABLED: powersave is enabled
 */
#define IMG_PS_DISABLED 0
#define IMG_PS_ENABLED 1

/*Will add additional info if required*/
/**
 * enum img_security_type - WLAN security type
 * @IMG_WEP: WEP
 * @IMG_WPA: WPA
 * @IMG_WPA2: WPA2
 * @IMG_WPA3: WPA3
 * @IMG_WAPI: WAPI
 */
#define IMG_OPEN 0
#define IMG_WEP 1
#define IMG_WPA 2
#define IMG_WPA2 3
#define IMG_WPA3 4
#define IMG_WAPI 5
#define IMG_EAP 6

/**
 * enum img_reg_initiator - Indicates the initiator of a reg domain request
 * @NL80211_REGDOM_SET_BY_CORE: Core queried CRDA for a dynamic world
 * regulatory domain.
 * @NL80211_REGDOM_SET_BY_USER: User asked the wireless core to set the
 * regulatory domain.
 * @NL80211_REGDOM_SET_BY_DRIVER: a wireless drivers has hinted to the
 * wireless core it thinks its knows the regulatory domain we should be in.
 * @NL80211_REGDOM_SET_BY_COUNTRY_IE: the wireless core has received an
 * 802.11 country information element with regulatory information it
 * thinks we should consider. cfg80211 only processes the country
 *	code from the IE, and relies on the regulatory domain information
 *	structure passed by userspace (CRDA) from our wireless-regdb.
 *	If a channel is enabled but the country code indicates it should
 *	be disabled we disable the channel and re-enable it upon disassociation.
 */
#define IMG_REGDOM_SET_BY_CORE 0
#define IMG_REGDOM_SET_BY_USER 1
#define IMG_REGDOM_SET_BY_DRIVER 2
#define IMG_REGDOM_SET_BY_COUNTRY_IE 3

/**
 * enum img_reg_type - specifies the type of regulatory domain
 * @NL80211_REGDOM_TYPE_COUNTRY: the regulatory domain set is one that pertains
 *	to a specific country. When this is set you can count on the
 *	ISO / IEC 3166 alpha2 country code being valid.
 * @NL80211_REGDOM_TYPE_WORLD: the regulatory set domain is the world regulatory
 * domain.
 * @NL80211_REGDOM_TYPE_CUSTOM_WORLD: the regulatory domain set is a custom
 * driver specific world regulatory domain. These do not apply system-wide
 * and are only applicable to the individual devices which have requested
 * them to be applied.
 * @NL80211_REGDOM_TYPE_INTERSECTION: the regulatory domain set is the product
 *	of an intersection between two regulatory domains -- the previously
 *	set regulatory domain on the system and the last accepted regulatory
 *	domain request to be processed.
 */
#define IMG_REGDOM_TYPE_COUNTRY 0
#define IMG_REGDOM_TYPE_WORLD 1
#define IMG_REGDOM_TYPE_CUSTOM_WORLD 2
#define IMG_REGDOM_TYPE_INTERSECTION 3

#define IMG_MAX_SSID_LEN 32

/**
 * struct img_ssid - SSID list.
 * @img_ssid_len: Indicates the number of values in ssid parameter.
 * @img_ssid: SSID (binary attribute, 0..32 octets).
 *
 * This structure describes the parameters to describe list of SSIDs
 * used by the @scan_common parameter in &struct img_umac_cmd_scan.
 */

struct img_ssid {
	unsigned char img_ssid_len;
	unsigned char img_ssid[IMG_MAX_SSID_LEN];
} __IMG_PKD;

#define IMG_MAX_IE_LEN 400

/**
 * struct img_ie - Information element(s) data.
 * @ie_len: Indicates the number of values in ie parameter.
 * @ie: Information element data.
 *
 * This structure describes the Information element(s) data being passed.
 */

struct img_ie {
	unsigned short ie_len;
	char ie[IMG_MAX_IE_LEN];
} __IMG_PKD;

#define IMG_MAX_SEQ_LENGTH 256

/**
 * struct img_seq - Transmit key sequence number.
 * @img_seq_len: Length of the seq parameter.
 * @img_seq: Key sequence number data.
 *
 * Transmit key sequence number (IV/PN) for TKIP and CCMP keys, each six bytes
 * in little endian.
 */

struct img_seq {
	int img_seq_len;
	unsigned char img_seq[IMG_MAX_SEQ_LENGTH];
} __IMG_PKD;

#define IMG_MAX_KEY_LENGTH 256

/**
 * struct img_key - Key data.
 *
 * @img_key_len: Length of the key data.
 * @img_key: Key data.
 *
 * This structure represents a security key data.
 */

struct img_key {
	unsigned int img_key_len;
	unsigned char img_key[IMG_MAX_KEY_LENGTH];
} __IMG_PKD;

#define IMG_MAX_SAE_DATA_LENGTH 256

/**
 * struct img_umac_sae - SAE element in auth frame.
 *
 * @sae_data_len: Length of SAE element data.
 * @sae_data: SAE element data.
 *
 * This structure represents SAE elements in Authentication frames.
 *
 */

struct img_sae {
	int sae_data_len;
	unsigned char sae_data[IMG_MAX_SAE_DATA_LENGTH];
} __IMG_PKD;

#define IMG_MAX_FRAME_LEN 400

/**
 * struct img_umac_frame - Frame data.
 *
 * @frame_len: Length of the frame.
 * @frame: Frame data.
 *
 * This structure describes a frame being passed.
 */

struct img_frame {
	int frame_len;
	char frame[IMG_MAX_FRAME_LEN];
} __IMG_PKD;

#define IMG_INDEX_IDS_WDEV_ID_VALID (1 << 0)
#define IMG_INDEX_IDS_IFINDEX_VALID (1 << 1)
#define IMG_INDEX_IDS_WIPHY_IDX_VALID (1 << 2)

/**
 * struct img_index_ids - UMAC interface index.
 *
 * @valid_fields: Indicate which properties below are set.
 * @wdev_id: wdev id.
 * @ifindex: Unused.
 * @wiphy_idx: Unused.
 *
 * Command header expected by UMAC. Legacy header in place to handle requests
 * from supplicant in RPU.
 */

struct img_index_ids {
	unsigned int valid_fields;
	int ifaceindex;
	int img_wiphy_idx;
	unsigned long long wdev_id;
} __IMG_PKD;

#define IMG_SUPP_RATES_BAND_VALID (1 << 0)
#define IMG_MAX_SUPP_RATES 60

/**
 * struct img_supp_rates - Scan request parameters.
 *
 * @valid_fields: Indicate which of the following parameters are valid.
 * @band: Frequency band, see enum img_umac_band.
 * @img_num_rates: Number of values in rates parameter.
 * @rates: Rates per to be advertised as supported in scan,
 *	 nested array attribute containing an entry for each band, with the
 *	 entry being a list of supported rates as defined by IEEE 802.11
 *	 7.3.2.2 but without the length restriction (at most
 *	 %IMG_MAX_SUPP_RATES).
 *
 * This structure specifies the parameters to be used when sending
 * %IMG_UMAC_CMD_TRIGGER_SCAN command (Refer &enum img_umac_commands).
 *
 */

struct img_supp_rates {
	unsigned int valid_fields;
	int band;
	int img_num_rates;
	unsigned char rates[IMG_MAX_SUPP_RATES];
} __IMG_PKD;

/**
 * struct img_channel - channel definition
 *
 * This structure describes a single channel for use.
 *
 * @center_frequency: center frequency in MHz
 * @hw_value: hardware-specific value for the channel
 * @img_flags: channel flags from &enum img_channel_flags.
 * @img_orig_flags: channel flags at registration time, used by regulatory
 * code to support devices with additional restrictions
 * @band: band this channel belongs to.
 * @img_max_antenna_gain: maximum antenna gain in dBi
 * @img_max_power: maximum transmission power (in dBm)
 * @img_max_reg_power: maximum regulatory transmission power (in dBm)
 * @img_beacon_found: helper to regulatory code to indicate when a beacon
 * has been found on this channel. Use regulatory_hint_found_beacon()
 * to enable this, this is useful only on 5 GHz band.
 * @img_orig_mag: internal use
 * @img_orig_mpwr: internal use
 */

struct img_channel {
	int band;
	unsigned short center_frequency;
	unsigned short hw_value;
	unsigned short img_flags;
	int img_max_antenna_gain;
	int img_max_power;
	int img_max_reg_power;
	char img_beacon_found;
	unsigned int img_orig_flags;
	int img_orig_mag;
	int img_orig_mpwr;
} __IMG_PKD;

#define IMG_SCAN_PARAMS_2GHZ_BAND_VALID (1 << 0)
#define IMG_SCAN_PARAMS_5GHZ_BAND_VALID (1 << 1)
#define IMG_SCAN_PARAMS_60GHZ_BAND_VALID (1 << 2)
#define IMG_SCAN_PARAMS_MAC_ADDR_VALID (1 << 3)
#define IMG_SCAN_PARAMS_MAC_ADDR_MASK_VALID (1 << 4)
#define IMG_SCAN_PARAMS_SCAN_FLAGS_VALID (1 << 5)
#define IMG_SCAN_PARAMS_SUPPORTED_RATES_VALID (1 << 6)

#define IMG_SCAN_MAX_NUM_SSIDS 2
#define IMG_SCAN_MAX_NUM_FREQUENCIES 64
#define MAX_NUM_CHANNELS 39

/**
 * scan_flags -	scan request control flags
 *
 * Scan request control flags are used to control the handling
 * of CMD_TRIGGER_SCAN request.
 *
 * @#define IMG_SCAN_FLAG_LOW_PRIORITY: scan request has low priority
 * @#define IMG_SCAN_FLAG_FLUSH: flush cache before scanning
 * @#define IMG_SCAN_FLAG_AP: force a scan even if the interface is configured
 *	as AP and the beaconing has already been configured. This attribute is
 *	dangerous because will destroy stations performance as a lot of frames
 *	will be lost while scanning off-channel, therefore it must be used only
 *	when really needed
 * @#define IMG_SCAN_FLAG_RANDOM_ADDR: use a random MAC address for this scan (or
 *	for scheduled scan: a different one for every scan iteration). When the
 *	flag is set, depending on device capabilities the @MAC and
 *	@MAC_MASK attributes may also be given in which case only
 *	the masked bits will be preserved from the MAC address and the remainder
 *	randomised. If the attributes are not given full randomisation (46 bits,
 *	locally administered 1, multicast 0) is assumed.
 *	This flag must not be requested when the feature isn't supported, check
 *	the nl80211 feature flags for the device.
 * @#define IMG_SCAN_FLAG_FILS_MAX_CHANNEL_TIME: fill the dwell time in the FILS
 *	request parameters IE in the probe request
 * @#define IMG_SCAN_FLAG_ACCEPT_BCAST_PROBE_RESP: accept broadcast probe responses
 * @#define IMG_SCAN_FLAG_OCE_PROBE_REQ_HIGH_TX_RATE: send probe request frames at
 *	rate of at least 5.5M. In case non OCE AP is dicovered in the channel,
 *	only the first probe req in the channel will be sent in high rate.
 * @#define IMG_SCAN_FLAG_OCE_PROBE_REQ_DEFERRAL_SUPPRESSION: allow probe request
 *	tx deferral (dot11FILSProbeDelay shall be set to 15ms)
 *	and suppression (if it has received a broadcast Probe Response frame,
 *	Beacon frame or FILS Discovery frame from an AP that the STA considers
 *	a suitable candidate for (re-)association - suitable in terms of
 *	SSID and/or RSSI
 */
#define IMG_SCAN_FLAG_LOW_PRIORITY (1 << 0)
#define IMG_SCAN_FLAG_FLUSH (1 << 1)
#define IMG_SCAN_FLAG_AP (1 << 2)
#define IMG_SCAN_FLAG_RANDOM_ADDR (1 << 3)
#define IMG_SCAN_FLAG_FILS_MAX_CHANNEL_TIME (1 << 4)
#define IMG_SCAN_FLAG_ACCEPT_BCAST_PROBE_RESP (1 << 5)
#define IMG_SCAN_FLAG_OCE_PROBE_REQ_HIGH_TX_RATE (1 << 6)
#define IMG_SCAN_FLAG_OCE_PROBE_REQ_DEFERRAL_SUPPRESSION (1 << 7)
#define IMG_SCAN_FLAG_LOW_SPAN (1 << 8)
#define IMG_SCAN_FLAG_LOW_POWER (1 << 9)
#define IMG_SCAN_FLAG_HIGH_ACCURACY (1 << 10)
#define IMG_SCAN_FLAG_RANDOM_SN (1 << 11)
#define IMG_SCAN_FLAG_MIN_PREQ_CONTENT (1 << 12)

/**
 * struct img_scan_params - Scan request parameters.
 *
 * @valid_fields: Indicate which of the following parameters are valid.
 * @num_scan_ssids: Number of elements in scan_ssids parameter.
 * @scan_ssids: Nested attribute with SSIDs, leave out for passive
 *	 scanning and include a zero-length SSID (wildcard) for wildcard scan.
 * @ie: Information element(s) data.
 * @num_scan_channels: Num of scan channels.
 * @scan_frequencies: Channel information.
 * @mac_addr: MAC address (various uses).
 * @mac_addr_mask: MAC address mask.
 * @scan_flags: Scan request control flags (u32). Bit values
 *	(IMG_SCAN_FLAG_LOW_PRIORITY/IMG_SCAN_FLAG_RANDOM_ADDR...)
 * @supp_rates: Supported rates.
 * @no_cck: used to send probe requests at non CCK rate in 2GHz band
 * @oper_ch__duration: Operating channel duration when STA is connected to AP
 * @scan_duration: Max scan duration in TU
 * @channels: See struct img_channel
 *
 * This structure specifies the parameters to be used when sending
 * %IMG_UMAC_CMD_TRIGGER_SCAN command (Refer &enum img_umac_commands).
 */

struct img_scan_params {
	unsigned int valid_fields;
	unsigned char num_scan_ssids;
	unsigned char num_scan_channels;
	unsigned int scan_flags;
	struct img_ssid scan_ssids[IMG_SCAN_MAX_NUM_SSIDS];
	struct img_ie ie;
	struct img_supp_rates supp_rates;
	unsigned char mac_addr[IMG_ETH_ALEN];
	unsigned char mac_addr_mask[IMG_ETH_ALEN];
	unsigned char no_cck;
	unsigned short oper_ch_duration;
	unsigned short scan_duration[MAX_NUM_CHANNELS];
	unsigned char probe_cnt[MAX_NUM_CHANNELS];
	struct img_channel channels[0];
} __IMG_PKD;

#define IMG_HT_CAPABILITY_VALID (1 << 0)
#define IMG_HT_CAPABILITY_MASK_VALID (1 << 1)
#define IMG_VHT_CAPABILITY_VALID (1 << 2)
#define IMG_VHT_CAPABILITY_MASK_VALID (1 << 3)
#define IMG_CMD_HT_VHT_CAPABILITY_DISABLE_HT (1 << 0)
#define IMG_HT_VHT_CAPABILITY_MAX_SIZE 256

/**
 * struct img_ht_vht_capabilities - VHT capability information.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @ht_capability: HT Capability information element (from
 *	association request when used with %IMG_UMAC_CMD_NEW_STATION in
 *	&enum img_umac_commands).
 * @ht_capability_mask: Specify which bits of the
 *	ATTR_HT_CAPABILITY to which attention should be paid.
 *	The values that may be configured are:
 *	MCS rates, MAX-AMSDU, HT-20-40 and HT_CAP_SGI_40
 *	AMPDU density and AMPDU factor.
 *	All values are treated as suggestions and may be ignored
 *	by the driver as required.
 * @vht_capability: VHT Capability information element (from
 *	association request when used with %IMG_UMAC_CMD_NEW_STATION in
 *	&enum img_umac_commands).
 * @vht_capability_mask: Specify which bits in vht_capability to which attention
 *	should be paid.
 * @img_flags: Indicate which capabilities have been specified.
 *
 * This structure encapsulates the VHT capability information.
 */

struct img_ht_vht_capabilities {
	unsigned int valid_fields;
	unsigned short img_flags;
	unsigned char ht_capability[IMG_HT_VHT_CAPABILITY_MAX_SIZE];
	unsigned char ht_capability_mask[IMG_HT_VHT_CAPABILITY_MAX_SIZE];
	unsigned char vht_capability[IMG_HT_VHT_CAPABILITY_MAX_SIZE];
	unsigned char vht_capability_mask[IMG_HT_VHT_CAPABILITY_MAX_SIZE];
} __IMG_PKD;

#define IMG_SIGNAL_TYPE_NONE 1
#define IMG_SIGNAL_TYPE_MBM 2
#define IMG_SIGNAL_TYPE_UNSPEC 3

/**
 * struct img_signal - Signal information.
 *
 * @signal_type: MBM or unspecified.
 * @signal: If MBM signal strength of probe response/beacon
 *	in mBm (100 * dBm) (s32)
 *	If unspecified signal strength of the probe response/beacon
 *	in unspecified units, scaled to 0..100 (u8).
 *
 * This structure represents signal information.
 */

struct img_signal {
	unsigned int signal_type;

	union {
		unsigned int mbm_signal;
		unsigned char unspec_signal;
	} __IMG_PKD signal;
} __IMG_PKD;
#define IMG_WPA_VERSION_1 (1 << 0)
#define IMG_WPA_VERSION_2 (1 << 1)

#define IMG_CONNECT_COMMON_INFO_MAC_ADDR_VALID (1 << 0)
#define IMG_CONNECT_COMMON_INFO_MAC_ADDR_HINT_VALID (1 << 1)
#define IMG_CONNECT_COMMON_INFO_FREQ_VALID (1 << 2)
#define IMG_CONNECT_COMMON_INFO_FREQ_HINT_VALID (1 << 3)
#define IMG_CONNECT_COMMON_INFO_BG_SCAN_PERIOD_VALID (1 << 4)
#define IMG_CONNECT_COMMON_INFO_SSID_VALID (1 << 5)
#define IMG_CONNECT_COMMON_INFO_WPA_IE_VALID (1 << 6)
#define IMG_CONNECT_COMMON_INFO_WPA_VERSIONS_VALID (1 << 7)
#define IMG_CONNECT_COMMON_INFO_CIPHER_SUITES_PAIRWISE_VALID (1 << 8)
#define IMG_CONNECT_COMMON_INFO_CIPHER_SUITE_GROUP_VALID (1 << 9)
#define IMG_CONNECT_COMMON_INFO_AKM_SUITES_VALID (1 << 10)
#define IMG_CONNECT_COMMON_INFO_USE_MFP_VALID (1 << 11)
#define IMG_CONNECT_COMMON_INFO_CONTROL_PORT_ETHER_TYPE (1 << 12)
#define IMG_CONNECT_COMMON_INFO_CONTROL_PORT_NO_ENCRYPT (1 << 13)

#define IMG_MAX_NR_AKM_SUITES 2

#define IMG_CMD_CONNECT_COMMON_INFO_USE_RRM (0 << 1)

/**
 * struct img_connect_common_info - Connection properties.
 *
 * @valid_fields: Indicate which of the following parameters are valid.
 * @mac_addr: MAC address (various uses).
 * @mac_addr_hint: MAC address recommendation as initial BSS.
 * @frequency: Frequency of the selected channel in MHz, defines the channel
 *	together with the (deprecated) %IMG_UMAC_ATTR_WIPHY_CHANNEL_TYPE
 *	attribute or the attributes %IMG_UMAC_ATTR_CHANNEL_WIDTH and if needed
 *	%IMG_UMAC_ATTR_CENTER_FREQ1 and %IMG_UMAC_ATTR_CENTER_FREQ2.
 * @freq_hint: Frequency of the recommended initial BS.
 * @bg_scan_period: Background scan period in seconds or 0 to disable
 *	background scan.
 * @ssid: SSID (binary attribute, 0..32 octets).
 * @wpa_ie: Information element(s) data.
 * @wpa_versions: Used with CONNECT, ASSOCIATE, and NEW_BEACON to
 *	indicate which WPA version(s) the AP we want to associate with is using
 * @num_cipher_suites_pairwise: Number of pairwise cipher suites.
 * @cipher_suites_pairwise: For crypto settings for connect or
 *	other commands, indicates which pairwise cipher suites are used.
 * @cipher_suite_group: For crypto settings for connect or
 *	other commands, indicates which group cipher suite is used.
 * @num_akm_suites: Number of groupwise cipher suites.
 * @akm_suites: Used with CONNECT, ASSOCIATE, and NEW_BEACON to
 *	indicate which key management algorithm(s) to use (an array of u32).
 * @use_mfp: Whether management frame protection (IEEE 802.11w) is
 *	used for the association; this attribute can be used
 *	with %IMG_UMAC_CMD_ASSOCIATE and %IMG_UMAC_CMD_CONNECT requests (Refer
 *	&enum img_umac_commands).
 * @ht_vht_capabilitys: VHT Capability information element (from
 *	association request when used with %IMG_UMAC_CMD_NEW_STATION in
 *	&enum img_umac_commands).
 * @img_flags: Flag for indicating whether the current connection
 *	shall support Radio Resource Measurements (11k). This attribute can be
 *	used with %IMG_UMAC_CMD_ASSOCIATE and %IMG_UMAC_CMD_CONNECT requests
 *	(Refer &enum img_umac_commands).
 *	User space applications are expected to use this flag only if the
 *	underlying device supports these minimal RRM features:
 *	%IMG_UMAC_FEATURE_DS_PARAM_SET_IE_IN_PROBES,
 *	%IMG_UMAC_FEATURE_QUIET,
 *	If this flag is used, driver must add the Power Capabilities IE to the
 *	association request. In addition, it must also set the RRM capability
 *	flag in the association request's Capability Info field.
 *	flag indicating whether user space controls
 *	IEEE 802.1X port, i.e., sets/clears %IMG_UMAC_STA_FLAG_AUTHORIZED, in
 *	station mode. If the flag is included in %IMG_UMAC_CMD_ASSOCIATE
 *	request, the driver will assume that the port is unauthorized until
 *	authorized by user space. Otherwise, port is marked authorized by
 *	default in station mode.
 *
 * This structure specifies the parameters to be used when building
 * connect_common_info when sending %IMG_UMAC_CMD_ASSOCIATE command (Refer
 * &enum img_umac_commands).
 */

struct img_connect_common_info {
	unsigned int valid_fields;
	unsigned int frequency;
	unsigned int freq_hint;
	unsigned int wpa_versions;
	int num_cipher_suites_pairwise;
	unsigned int cipher_suites_pairwise[7];
	unsigned int cipher_suite_group;
	unsigned int num_akm_suites;
	unsigned int akm_suites[IMG_MAX_NR_AKM_SUITES];
	int use_mfp;

	unsigned int img_flags;
	unsigned short bg_scan_period;
	unsigned char mac_addr[IMG_ETH_ALEN];
	unsigned char mac_addr_hint[IMG_ETH_ALEN];
	struct img_ssid ssid;
	struct img_ie wpa_ie;
	struct img_ht_vht_capabilities ht_vht_capabilities;
	unsigned short control_port_ether_type;
	unsigned char control_port_no_encrypt;
	char control_port;

} __IMG_PKD;

#define IMG_BEACON_DATA_MAX_HEAD_LEN 256
#define IMG_BEACON_DATA_MAX_TAIL_LEN 512
#define IMG_BEACON_DATA_MAX_PROBE_RESP_LEN 400

/**
 * struct img_beacon_data - beacon & probe data
 * @head_len: length of @head
 * @tail_len: length of @tail
 * @probe_resp_len: length of probe response template (@probe_resp)
 * @head: head portion of beacon (before TIM IE) or %NULL if not changed
 * @tail: tail portion of beacon (after TIM IE) or %NULL if not changed
 * @probe_resp: probe response template
 */

struct img_beacon_data {
	unsigned int head_len;
	unsigned int tail_len;
	unsigned int probe_resp_len;
	unsigned char head[IMG_BEACON_DATA_MAX_HEAD_LEN];
	unsigned char tail[IMG_BEACON_DATA_MAX_TAIL_LEN];
	unsigned char probe_resp[IMG_BEACON_DATA_MAX_PROBE_RESP_LEN];
} __IMG_PKD;

/*
 * struct img_sta_flag_update - Station flags mask/set.
 * @mask: Mask of station flags to set.
 * @set: Values to set them to.
 *
 * Both mask and set contain bits as per &enum img_sta_flags.
 */

struct img_sta_flag_update {
	unsigned int img_mask;
	unsigned int img_set;
} __IMG_PKD;

#define IMG_IMG_RATE_INFO_BITRATE_VALID (1 << 0)
#define IMG_IMG_RATE_INFO_BITRATE_COMPAT_VALID (1 << 1)
#define IMG_IMG_RATE_INFO_BITRATE_MCS_VALID (1 << 2)
#define IMG_IMG_RATE_INFO_BITRATE_VHT_MCS_VALID (1 << 3)
#define IMG_IMG_RATE_INFO_BITRATE_VHT_NSS_VALID (1 << 4)
#define IMG_IMG_RATE_INFO_0_MHZ_WIDTH (1 << 0)
#define IMG_IMG_RATE_INFO_5_MHZ_WIDTH (1 << 1)
#define IMG_IMG_RATE_INFO_10_MHZ_WIDTH (1 << 2)
#define IMG_IMG_RATE_INFO_40_MHZ_WIDTH (1 << 3)
#define IMG_IMG_RATE_INFO_80_MHZ_WIDTH (1 << 4)
#define IMG_IMG_RATE_INFO_160_MHZ_WIDTH (1 << 5)
#define IMG_IMG_RATE_INFO_SHORT_GI (1 << 6)
#define IMG_IMG_RATE_INFO_80P80_MHZ_WIDTH (1 << 7)

/*
 * struct img_rate_info - Rate information.
 * @valid_fields: Valid fields with in this structure.
 * @bitrate: bitrate.
 * @bitrate_compat: Bitrate copmpatible.
 * @img_mcs: Modulation and Coding Scheme(MCS).
 * @vht_mcs: MCS related to VHT.
 * @vht_nss: NSS related to VHT .
 * @img_flags: Rate flags .
 *
 * Both mask and set contain bits as per &enum img_sta_flags.
 */

struct img_rate_info {
	unsigned int valid_fields;
	unsigned int bitrate;
	unsigned short bitrate_compat;
	unsigned char img_mcs;
	unsigned char vht_mcs;
	unsigned char vht_nss;
	unsigned int img_flags;

} __IMG_PKD;

/*
 * struct img_sta_bss_parameters - BSS parameters for the attached station
 * Information about the currently associated BSS
 * @flags: bitflag of flags (CTS_PROT, SHORT_PREAMBLE, SHORT_SLOT_TIME)
 * @dtim_period: DTIM period for the BSS
 * @beacon_interval: beacon interval
 */

struct img_sta_bss_parameters {
	unsigned char img_flags;
	unsigned char dtim_period;
	unsigned short beacon_interval;
} __IMG_PKD;

#define IMG_STA_INFO_CONNECTED_TIME_VALID (1 << 0)
#define IMG_STA_INFO_INACTIVE_TIME_VALID (1 << 1)
#define IMG_STA_INFO_RX_BYTES_VALID (1 << 2)
#define IMG_STA_INFO_TX_BYTES_VALID (1 << 3)
#define IMG_STA_INFO_CHAIN_SIGNAL_VALID (1 << 4)
#define IMG_STA_INFO_CHAIN_SIGNAL_AVG_VALID (1 << 5)
#define IMG_STA_INFO_TX_BITRATE_VALID (1 << 6)
#define IMG_STA_INFO_RX_BITRATE_VALID (1 << 7)
#define IMG_STA_INFO_STA_FLAGS_VALID (1 << 8)

#define IMG_STA_INFO_LLID_VALID (1 << 9)
#define IMG_STA_INFO_PLID_VALID (1 << 10)
#define IMG_STA_INFO_PLINK_STATE_VALID (1 << 11)
#define IMG_STA_INFO_SIGNAL_VALID (1 << 12)
#define IMG_STA_INFO_SIGNAL_AVG_VALID (1 << 13)
#define IMG_STA_INFO_RX_PACKETS_VALID (1 << 14)
#define IMG_STA_INFO_TX_PACKETS_VALID (1 << 15)
#define IMG_STA_INFO_TX_RETRIES_VALID (1 << 16)
#define IMG_STA_INFO_TX_FAILED_VALID (1 << 17)
#define IMG_STA_INFO_EXPECTED_THROUGHPUT_VALID (1 << 18)
#define IMG_STA_INFO_BEACON_LOSS_COUNT_VALID (1 << 19)
#define IMG_STA_INFO_LOCAL_PM_VALID (1 << 20)
#define IMG_STA_INFO_PEER_PM_VALID (1 << 21)
#define IMG_STA_INFO_NONPEER_PM_VALID (1 << 22)
#define IMG_STA_INFO_T_OFFSET_VALID (1 << 23)
#define IMG_STA_INFO_RX_DROPPED_MISC_VALID (1 << 24)
#define IMG_STA_INFO_RX_BEACON_VALID (1 << 25)
#define IMG_STA_INFO_RX_BEACON_SIGNAL_AVG_VALID (1 << 26)
#define IMG_STA_INFO_STA_BSS_PARAMS_VALID (1 << 27)
#define IMG_IEEE80211_MAX_CHAINS 4

/*
 * struct img_sta_info - STA information.
 * @valid_fields: Valid fields with in this structure.
 * @connected_time: time since the station is last connected.
 * @inactive_time: time since last activity, in msec.
 * @rx_bytes: total received bytes from this station.
 * @tx_bytes: total transmitted bytes to this station.
 * @chain_signal_mask: per-chain signal mask value.
 * @chain_signal: per-chain signal strength of last PPDU.
 * @chain_signal_avg_mask: per-chain signal strength average mask value.
 * @chain_signal_avg: per-chain signal strength average.
 * @tx_bitrate: see the struct img_rate_info.
 * @tx_bitrate: see the struct img_rate_info.
 * @llid: Not used.
 * @plid: Not used.
 * @plink_state: Not used.
 * @signal: signal strength of last received PPDU, in dbm.
 * @signal_avg: signal strength average, in dbm.
 * @rx_packets: total received packet from this station.
 * @tx_packets: total transmitted packets to this station.
 * @tx_retries: total retries to this station.
 * @tx_failed: total failed packets to this station.
 * @expected_throughput: expected throughput considering also the mac80211
 *	header, in kbps.
 * @beacon_loss_count: count of times beacon loss was detected.
 * @local_pm: Not used.
 * @peer_pm: Not used.
 * @nonpeer_pm: Not used.
 * @sta_flags: see struct img_sta_flag_update.
 * @t_offset: timing offset with respect to this STA.
 * @rx_dropped_misc: count of times other(non beacon) loss was detected.
 * @rx_beacon: count of times beacon.
 * @rx_beacon_signal_avg: average of beacon signal.
 * @bss_param: Station connected BSS params
 */

struct img_sta_info {
	unsigned int valid_fields;
	unsigned int connected_time;
	unsigned int inactive_time;
	unsigned int rx_bytes;
	unsigned int tx_bytes;
	unsigned int chain_signal_mask;
	unsigned char chain_signal[IMG_IEEE80211_MAX_CHAINS];
	unsigned int chain_signal_avg_mask;
	unsigned char chain_signal_avg[IMG_IEEE80211_MAX_CHAINS];
	struct img_rate_info tx_bitrate;
	struct img_rate_info rx_bitrate;
	unsigned short llid;
	unsigned short plid;
	unsigned char plink_state;
	int signal;
	int signal_avg;
	unsigned int rx_packets;
	unsigned int tx_packets;
	unsigned int tx_retries;
	unsigned int tx_failed;
	unsigned int expected_throughput;
	unsigned int beacon_loss_count;
	unsigned int local_pm;
	unsigned int peer_pm;
	unsigned int nonpeer_pm;
	struct img_sta_flag_update sta_flags;
	unsigned long long t_offset;
	unsigned long long rx_dropped_misc;
	unsigned long long rx_beacon;
	long long rx_beacon_signal_avg;
	struct img_sta_bss_parameters bss_param;
} __IMG_PKD;

/**
 * struct img_umac_hdr - Common command/event header.
 *
 * @cmd_evnt: UMAC command/event value. (Refer	&enum img_umac_commands).
 * @ids: Interface properties.
 *
 * Command header expected by UMAC. Legacy header in place to handle requests
 * from supplicant in RPU.
 */

struct img_umac_hdr {
	/* private: presently unused */
	unsigned int portid;
	/* private: presently unused */
	unsigned int seq;
	/* public: */
	unsigned int cmd_evnt;
	/* private: presently unused */
	int rpu_ret_val;
	/* public: */
	struct img_index_ids ids;
} __IMG_PKD;

#define IMG_KEY_VALID (1 << 0)
#define IMG_KEY_TYPE_VALID (1 << 1)
#define IMG_KEY_IDX_VALID (1 << 2)
#define IMG_SEQ_VALID (1 << 3)
#define IMG_CIPHER_SUITE_VALID (1 << 4)
#define IMG_KEY_INFO_VALID (1 << 5)
#define IMG_KEY_DEFAULT (1 << 0)
#define IMG_KEY_DEFAULT_TYPES (1 << 1)
#define IMG_KEY_DEFAULT_MGMT (1 << 2)
#define IMG_KEY_DEFAULT_TYPE_UNICAST (1 << 3)
#define IMG_KEY_DEFAULT_TYPE_MULTICAST (1 << 4)

/**
 * struct img_umac_key_info - Key information.
 *
 * @valid_fields: Indicate which of the following parameters are valid.
 * @key: Key data, see &struct img_key.
 * @key_type: Key Type,	see &enum img_key_type
 * @key_idx: Key ID (0-3).
 * @seq: Transmit key sequence number (IV/PN) for TKIP and
 *	CCMP keys, each six bytes in little endian.
 * @cipher_suite: Key cipher suite (as defined by IEEE 802.11
 *	section 7.3.2.25.1).
 * @img_flags: A nested attribute containing flags
 *	attributes, specifying what a key should be set as default as.
 *
 * This structure represents a security key.
 */

struct img_umac_key_info {
	unsigned int valid_fields;
	unsigned int cipher_suite;
	unsigned short img_flags;
	int key_type;
	struct img_key key;
	struct img_seq seq;
	unsigned char key_idx;
} __IMG_PKD;

#define IMG_CMD_GET_KEY_MAC_ADDR_VALID (1 << 0)
#define IMG_CMD_GET_KEY_KEY_IDX_VALID (1 << 1)

struct img_umac_cmd_get_key {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	unsigned char mac_addr[IMG_ETH_ALEN];
	unsigned char key_idx;
} __IMG_PKD;

#define IMG_EVENT_GET_KEY_MAC_ADDR_VALID (1 << 0)

struct img_umac_event_get_key {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_key_info key_info;
	unsigned char mac_addr[IMG_ETH_ALEN];
} __IMG_PKD;

/**
 * enum scan_mode - scan operation mode
 * @AUTO: auto or legacy scan operation
 * @CHANNEL_MAPPING_SCAN: channel mapping mode. most of parameters will come from host.
 *
 * This enum represents the different types of scanning operations.
 */
#define AUTO_SCAN 0
#define CHANNEL_MAPPING_SCAN 1

/**
 * enum scan_reason - scan reason
 * @SCAN_DISPLAY: scan for display purpose in user space
 * @SCAN_CONNECT: scan for connection purpose.
 *
 * This enum represents the different types of scan reasons.
 */
#define SCAN_DISPLAY 0
#define SCAN_CONNECT 1

/**
 * struct img_umac_scan_info - Scan related information.
 *
 * @scan_mode:see enum scan_mode .
 * @scan_reason:see enum scan_reason .
 * @scan_params: Refer to &struct img_umac_scan_params.
 *
 * Properties to be used when triggering a new scan request
 */

struct img_umac_scan_info {
	int scan_mode;
	int scan_reason;
	struct img_scan_params scan_params;
} __IMG_PKD;

/**
 * struct img_umac_cmd_scan - Scan request properties.
 *
 * @umac_hdr: Refer to &struct img_umac_hdr.
 * @info: Refer to &struct img_umac_scan_info.
 *
 * Properties to be used when triggering a new scan request
 */

struct img_umac_cmd_scan {
	/* public: */
	struct img_umac_hdr umac_hdr;
	struct img_umac_scan_info info;
} __IMG_PKD;
/**
 * struct img_umac_cmd_abort_scan - Abort Scan request.
 *
 * @umac_hdr: Refer to &struct img_umac_hdr.
 *
 */

struct img_umac_cmd_abort_scan {
	struct img_umac_hdr umac_hdr;
} __IMG_PKD;


/**
 * struct img_umac_cmd_get_scan_results - Get scan results.
 *
 * @umac_hdr: Refer to &struct img_umac_hdr.
 * @scan_reason: Refer to &enum scan_reason.
 *
 * Properties to be used when requesting for scan results. This should be
 * allowed only if we received a %IMG_UMAC_EVENT_SCAN_DONE for a
 * %IMG_UMAC_CMD_TRIGGER_SCAN earlier.
 */

struct img_umac_cmd_get_scan_results {
	struct img_umac_hdr umac_hdr;
	int scan_reason;
} __IMG_PKD;

struct img_umac_event_scan_done {
	struct img_umac_hdr umac_hdr;
	int status;
} __IMG_PKD;

#define IMG_CMD_AUTHENTICATE_KEY_INFO_VALID (1 << 0)
#define IMG_CMD_AUTHENTICATE_BSSID_VALID (1 << 1)
#define IMG_CMD_AUTHENTICATE_FREQ_VALID (1 << 2)
#define IMG_CMD_AUTHENTICATE_SSID_VALID (1 << 3)
#define IMG_CMD_AUTHENTICATE_IE_VALID (1 << 4)
#define IMG_CMD_AUTHENTICATE_SAE_VALID (1 << 5)
#define IMG_CMD_AUTHENTICATE_LOCAL_STATE_CHANGE (1 << 0)

/**
 * struct img_umac_auth_info - Authentication command parameters.
 *
 * @frequency: Frequency of the selected channel in MHz,
 *	 defines the channel together with the (deprecated)
 *	 %IMG_UMAC_ATTR_WIPHY_CHANNEL_TYPE attribute or the attributes
 *	 %IMG_UMAC_ATTR_CHANNEL_WIDTH and if needed %IMG_UMAC_ATTR_CENTER_FREQ1
 *	 and %IMG_UMAC_ATTR_CENTER_FREQ2.
 * @img_flags: Flag attribute to indicate that a command
 *	 is requesting a local authentication/association state change without
 *	 invoking actual management frame exchange. This can be used with
 *	 %IMG_UMAC_CMD_AUTHENTICATE, %IMG_UMAC_CMD_DEAUTHENTICATE
 *	 (Refer &enum img_umac_commands).
 * @auth_type: Authentication type.
 * @key_info: Key information in a nested attribute with
 *	 %IMG_UMAC_KEY_* sub-attributes.
 * @ssid: SSID (binary attribute, 0..32 octets).
 * @ie: Information element(s) data.
 * @sae: SAE elements in Authentication frames. This starts
 *	 with the Authentication transaction sequence number field.
 * @img_bssid: MAC address (various uses).
 *
 * This structure specifies the parameters to be used when sending auth request.
 */

struct img_umac_auth_info {
	unsigned int frequency;
	unsigned short img_flags;
	int auth_type;
	struct img_umac_key_info key_info;
	struct img_ssid ssid;
	struct img_ie ie;
	struct img_sae sae;
	unsigned char img_bssid[IMG_ETH_ALEN];
	int scan_width;
	int img_signal;
	int from_beacon;
	struct img_ie bss_ie;
	unsigned short capability;
	unsigned short beacon_interval;
	unsigned long long tsf;
} __IMG_PKD;

/**
 * struct img_umac_cmd_auth - Authentication command structure.
 *
 * @umac_hdr: UMAC command header. See &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @info: Information to be passed along with the authentication command.
 *	See &struct img_umac_auth_info.
 *
 * This structure specifies the format to be used when sending an auth request.
 */

struct img_umac_cmd_auth {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_auth_info info;
} __IMG_PKD;

#define IMG_CMD_ASSOCIATE_MAC_ADDR_VALID (1 << 0)

/**
 * struct img_umac_assoc_info - Association command parameters.
 *
 * @center_frequency: Frequency of the selected channel in MHz, defines the channel
 *	together with the (deprecated)
 *	%IMG_UMAC_ATTR_WIPHY_CHANNEL_TYPE attribute or the attributes
 *	%IMG_UMAC_ATTR_CHANNEL_WIDTH and if needed
 *	%IMG_UMAC_ATTR_CENTER_FREQ1 and %IMG_UMAC_ATTR_CENTER_FREQ2.
 * @ssid: SSID (binary attribute, 0..32 octets).
 * @wpa_ie: WPA information element data.
 * @img_bssid: MAC address (various uses).
 *
 * This structure specifies the parameters to be used when sending an assoc
 * request.
 */

struct img_umac_assoc_info {
	unsigned int center_frequency;
	struct img_ssid ssid;
	unsigned char img_bssid[IMG_ETH_ALEN];
	struct img_ie wpa_ie;
	unsigned char use_mfp;
	char control_port;
} __IMG_PKD;

/**
 * struct img_umac_cmd_assoc - Association command parameters
 *
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @connect_common_info: Connection attributes.
 * @mac_addr: Previous BSSID, to be used by in ASSOCIATE commands to specify
 *	using a reassociate frame.
 *
 * This structure specifies the parameters to be used when sending
 * %IMG_UMAC_CMD_ASSOCIATE command.
 */

struct img_umac_cmd_assoc {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_connect_common_info connect_common_info;
	unsigned char mac_addr[IMG_ETH_ALEN];
} __IMG_PKD;

#define IMG_CMD_MLME_MAC_ADDR_VALID (1 << 0)
#define IMG_CMD_MLME_LOCAL_STATE_CHANGE (1 << 0)

/**
 * struct img_umac_disconn_info - Parameters to be used along with any of the
 *	disconnection commands.
 *
 * @img_flags: Flag attribute to indicate that a command is requesting a local
 *	deauthentication/disassociation state change without invoking
 *	actual management frame exchange. This can be used with
 *	%IMG_UMAC_CMD_DISASSOCIATE, %IMG_UMAC_CMD_DEAUTHENTICATE
 *	(Refer &enum img_umac_commands).
 * @reason_code: ReasonCode for disassociation or deauthentication.
 * @mac_addr: MAC address (various uses).
 *
 * This structure specifies the parameters to be used when sending any of the
 * disconnection commands i.e. %IMG_UMAC_CMD_DISCONNECT (or)
 * %IMG_UMAC_CMD_DISASSOCIATE (or) %IMG_UMAC_CMD_DEAUTHENTICATE.
 */

struct img_umac_disconn_info {
	unsigned short img_flags;
	unsigned short reason_code;
	unsigned char mac_addr[IMG_ETH_ALEN];
} __IMG_PKD;

/**
 * struct img_umac_cmd_disconn - Disconnection command parameters
 *
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 *
 * This structure specifies the parameters to be used when sending
 * %IMG_UMAC_CMD_DISCONNECT (or) %IMG_UMAC_CMD_DISASSOCIATE (or)
 * %IMG_UMAC_CMD_DEAUTHENTICATE.
 */

struct img_umac_cmd_disconn {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_disconn_info info;
} __IMG_PKD;

#define IMG_CMD_NEW_INTERFACE_USE_4ADDR_VALID (1 << 0)
#define IMG_CMD_NEW_INTERFACE_MAC_ADDR_VALID (1 << 1)
#define IMG_CMD_NEW_INTERFACE_IFTYPE_VALID (1 << 2)
#define IMG_CMD_NEW_INTERFACE_IFNAME_VALID (1 << 3)

/**
 * struct img_umac_add_vif_info - Information for creating a new virtual
 *	interface.
 *
 * @iftype: Interface type, see enum img_sys_iftype.
 * @img_use_4addr: Use 4-address frames on a virtual interface.
 * @mon_flags: Monitor configuration flags.
 * @mac_addr: MAC Address.
 * @ifacename: Interface name.
 *
 * This structure represents the information to be passed to the RPU to
 * create a new virtual interface using the %IMG_UMAC_CMD_NEW_INTERFACE
 * command.
 */

struct img_umac_add_vif_info {
	int iftype;
	int img_use_4addr;
	unsigned int mon_flags; /*	|| (1 << enum img_mntr_flags) */
	unsigned char mac_addr[IMG_ETH_ALEN];
	char ifacename[16];
} __IMG_PKD;

/**
 * struct img_umac_cmd_add_vif - Create a new virtual interface
 *
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @info: VIF specific information to be passed to the RPU.
 *
 * This structure represents a command to be passed to inform the RPU to
 * create a new virtual interface.
 */

struct img_umac_cmd_add_vif {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_add_vif_info info;
} __IMG_PKD;

/**
 * struct img_umac_cmd_del_vif - Delete a virtual interface
 *
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 *
 * This structure represents a command to be passed to inform the RPU to
 * delete a virtual interface. This cmd is not allowed on default interface.
 */

struct img_umac_cmd_del_vif {
	struct img_umac_hdr umac_hdr;
} __IMG_PKD;

#define IMG_FRAME_MATCH_MAX_LEN 8

/**
 * struct img_umac_frame_match - Frame data to match for RX filter.
 * @frame_match_len: Length of data.
 * @frame_match: Data to match.
 *
 * This structure represents the frame data to match so that the RPU RX filter
 * can pass up the matching frames.
 */

struct img_umac_frame_match {
	unsigned int frame_match_len;
	unsigned char frame_match[IMG_FRAME_MATCH_MAX_LEN];
} __IMG_PKD;

/**
 * struct img_umac_mgmt_frame_info - Information regarding management frame to
 *	be registered to be received.
 * @frame_type: Frame type/subtype.
 * @frame_match: A binary attribute which typically must contain at least one
 *	byte.
 *
 * This structure represents information regarding a management frame which
 * should not be filtered by the RPU and passed up.
 */

struct img_umac_mgmt_frame_info {
	unsigned short frame_type;
	struct img_umac_frame_match frame_match;
} __IMG_PKD;

/**
 * struct img_umac_cmd_mgmt_frame_reg - Register management frame type to be
 *	received.
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr
 * @info: Management frame specific information to be passed to the RPU.
 *
 * This structure represents a command to be passed to inform the RPU to
 * register a management frame which should not be filtered by the RPU and
 * passed up.
 */

struct img_umac_cmd_mgmt_frame_reg {
	struct img_umac_hdr umac_hdr;
	struct img_umac_mgmt_frame_info info;
} __IMG_PKD;

#define IMG_CMD_KEY_MAC_ADDR_VALID (1 << 0)

/**
 * struct img_umac_cmd_key - Parameters when adding new key
 *
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @key_info: Key information in a nested attribute with
 *	%IMG_UMAC_KEY_* sub-attributes.
 * @mac_addr: MAC address associated with the key.
 *
 * This structure represents a command to add a new key.
 */

struct img_umac_cmd_key {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_key_info key_info;
	unsigned char mac_addr[IMG_ETH_ALEN];
} __IMG_PKD;

/**
 * struct img_umac_set_key - Parameters when setting default key.
 *
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @key_info: Key information in a nested attribute with
 *	%IMG_UMAC_KEY_* sub-attributes.
 *
 * This structure represents a command to set a default key.
 */

struct img_umac_cmd_set_key {
	struct img_umac_hdr umac_hdr;
	struct img_umac_key_info key_info;
} __IMG_PKD;

#define IMG_CMD_SET_BSS_CTS_VALID (1 << 0)
#define IMG_CMD_SET_BSS_PREAMBLE_VALID (1 << 1)
#define IMG_CMD_SET_BSS_SLOT_VALID (1 << 2)
#define IMG_CMD_SET_BSS_HT_OPMODE_VALID (1 << 3)
#define IMG_CMD_SET_BSS_AP_ISOLATE_VALID (1 << 4)
#define IMG_CMD_SET_BSS_P2P_CTWINDOW_VALID (1 << 5)
#define IMG_CMD_SET_BSS_P2P_OPPPS_VALID (1 << 6)

#define IMG_BASIC_MAX_SUPP_RATES 32

/**
 * struct img_umac_bss_info - BSS attributes.
 * @p2p_go_ctwindow: P2P GO Client Traffic Window, used with
 *	the START_AP and SET_BSS commands.
 * @p2p_opp_ps: P2P GO opportunistic PS, used with the
 *	START_AP and SET_BSS commands. This can have the values 0 or 1;
 *	if not given in START_AP 0 is assumed, if not given in SET_BSS
 *	no change is made.
 * @num_basic_rates: Number of basic rate elements.
 * @ht_opmode: HT operation mode.
 * @cts: Whether CTS protection is enabled (0 or 1).
 * @preamble: Whether short preamble is enabled (0 or 1).
 * @slot: Whether short slot time enabled (0 or 1).
 * @ap_isolate: (AP mode) Do not forward traffic between stations connected to
 *	this BSS.
 * @basic_rates: Basic rates, array of basic rates in format defined by
 *	IEEE 802.11 7.3.2.2 but without the length restriction
 *	(at most %IMG_MAX_SUPP_RATES).
 *
 * This structure represents the BSS attributes.
 */

struct img_umac_bss_info {
	unsigned int p2p_go_ctwindow;
	unsigned int p2p_opp_ps;
	unsigned int num_basic_rates;
	unsigned short ht_opmode;
	unsigned char img_cts;
	unsigned char preamble;
	unsigned char img_slot;
	unsigned char ap_isolate;
	unsigned char basic_rates[IMG_BASIC_MAX_SUPP_RATES];
} __IMG_PKD;

/**
 * struct img_umac_cmd_set_bss - Set BSS attributes.
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid
 * @bss_info: BSS specific information to be passed to the RPU.
 *
 * This structure represents a command to set BSS attributes.
 */

struct img_umac_cmd_set_bss {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_bss_info bss_info;
} __IMG_PKD;

#define IMG_SET_FREQ_PARAMS_FREQ_VALID (1 << 0)
#define IMG_SET_FREQ_PARAMS_CHANNEL_WIDTH_VALID (1 << 1)
#define IMG_SET_FREQ_PARAMS_CENTER_FREQ1_VALID (1 << 2)
#define IMG_SET_FREQ_PARAMS_CENTER_FREQ2_VALID (1 << 3)
#define IMG_SET_FREQ_PARAMS_CHANNEL_TYPE_VALID (1 << 4)

/**
 * struct freq_params - Frequency configuration.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @frequency: Value in MHz.
 * @channel_width: Width of the channel (refer &enum img_chan_width).
 * @channel_type: Type of channel (refer &enum img_channel_type).
 *
 * This structure represents a frequency parameters to be set.
 */

struct freq_params {
	unsigned int valid_fields;
	int frequency;
	int channel_width;
	/* private : presently unused */
	int center_frequency1;
	/* private : presently unused */
	int center_frequency2;
	/* public: */
	int channel_type;

} __IMG_PKD;

/**
 * struct img_umac_cmd_set_channel - Set channel configuration.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following structure parameters are valid.
 * @freq_params: Information related to channel parameters
 *
 * This structure represents the command to set the wireless channel configuration.
 */

struct img_umac_cmd_set_channel {
	struct img_umac_hdr umac_hdr;
#define IMG_CMD_SET_CHANNEL_FREQ_PARAMS_VALID (1 << 0)
	unsigned int valid_fields;
	struct freq_params freq_params;
} __IMG_PKD;
/**
 * struct img_txq_params - TX queue parameter attributes.
 * @txop: Transmit oppurtunity.
 * @cwmin: Minimum contention window.
 * @cwmax: Maximum contention window.
 * @aifs: Arbitration interframe spacing.
 * @ac: Access category.
 *
 * This structure represents transmit queue parameters.
 */

struct img_txq_params {
	unsigned short txop;
	unsigned short cwmin;
	unsigned short cwmax;
	unsigned char aifs;
	unsigned char ac;

} __IMG_PKD;

/**
 * enum img_tx_power_setting - TX power adjustment.
 * @IMG_TX_POWER_AUTOMATIC: Automatically determine transmit power.
 * @IMG_TX_POWER_LIMITED: Limit TX power by the mBm parameter.
 * @IMG_TX_POWER_FIXED: Fix TX power to the mBm parameter.
 *
 * Types of transmit power settings.
 */
#define IMG_TX_POWER_AUTOMATIC 0
#define IMG_TX_POWER_LIMITED 1
#define IMG_TX_POWER_FIXED 2

#define IMG_TX_POWER_SETTING_TYPE_VALID (1 << 0)
#define IMG_TX_POWER_SETTING_TX_POWER_LEVEL_VALID (1 << 1)

/**
 * struct img_tx_power_setting - TX power configuration.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @type: Power value type, see img_tx_power_type.
 * @tx_power_level: Transmit power level in signed mBm units.
 *
 * This structure represents the transmit power setting parameters.
 */

struct img_tx_power_setting {
	unsigned int valid_fields;
	int type;
	unsigned int tx_power_level;

} __IMG_PKD;

#define IMG_CMD_SET_WIPHY_FREQ_PARAMS_VALID (1 << 0)
#define IMG_CMD_SET_WIPHY_TXQ_PARAMS_VALID (1 << 1)
#define IMG_CMD_SET_WIPHY_RTS_THRESHOLD_VALID (1 << 2)
#define IMG_CMD_SET_WIPHY_FRAG_THRESHOLD_VALID (1 << 3)
#define IMG_CMD_SET_WIPHY_TX_POWER_SETTING_VALID (1 << 4)
#define IMG_CMD_SET_WIPHY_ANTENNA_TX_VALID (1 << 5)
#define IMG_CMD_SET_WIPHY_ANTENNA_RX_VALID (1 << 6)
#define IMG_CMD_SET_WIPHY_RETRY_SHORT_VALID (1 << 7)
#define IMG_CMD_SET_WIPHY_RETRY_LONG_VALID (1 << 8)
#define IMG_CMD_SET_WIPHY_COVERAGE_CLASS_VALID (1 << 9)
#define IMG_CMD_SET_WIPHY_WIPHY_NAME_VALID (1 << 10)

/**
 * struct img_umac_set_wiphy_info - wiphy configuration.
 * @rts_threshold: RTS threshold (TX frames with length
 *	larger than or equal to this use RTS/CTS handshake); allowed range:
 *	0..65536, disable with (u32)-1.
 * @frag_threshold: Fragmentation threshold, i.e., maximum
 *	length in octets for frames; allowed range: 256..8000, disable
 *	fragmentation with (u32)-1.
 * @antenna_tx: Bitmap of allowed antennas for transmitting.
 *	This can be used to mask out antennas which are not attached or should
 *	not be used for transmitting. If an antenna is not selected in this
 *	bitmap the hardware is not allowed to transmit on this antenna.
 * @antenna_rx: Bitmap of allowed antennas for receiving.
 *	This can be used to mask out antennas which are not attached or should
 *	not be used for receiving. If an antenna is not selected in this bitmap
 *	the hardware should not be configured to receive on this antenna.
 * @freq_params: Frequency of the selected channel in MHz,
 *	defines the channel together with the (deprecated)
 *	%IMG_ATTR_WIPHY_CHANNEL_TYPE attribute or the attributes
 *	%IMG_ATTR_CHANNEL_WIDTH and if needed %IMG_ATTR_CENTER_FREQ1
 *	and %IMG_ATTR_CENTER_FREQ2.
 * @txq_params: A nested array of TX queue parameters.
 * @tx_power_setting: Transmit power setting type. See
 *	&enum img_tx_power_setting for possible values.
 * @retry_short: TX retry limit for frames whose length is
 *	less than or equal to the RTS threshold; allowed range: 1..255.
 * @retry_long: TX retry limit for frames whose length is
 *	greater than the RTS threshold; allowed range: 1..255.
 * @coverage_class:Coverage Class as defined by IEEE 802.11
 *	section 7.3.2.9.
 * @wiphy_name: WIPHY name (used for renaming).
 *
 * This structure represents the wireless PHY configuration.
 */

struct img_umac_set_wiphy_info {
	unsigned int rts_threshold;
	unsigned int frag_threshold;
	unsigned int antenna_tx;
	unsigned int antenna_rx;
	struct freq_params freq_params;
	struct img_txq_params txq_params;
	struct img_tx_power_setting tx_power_setting;
	unsigned char retry_short;
	unsigned char retry_long;
	unsigned char coverage_class;
	char wiphy_name[32];
} __IMG_PKD;

/**
 * struct img_umac_cmd_set_wiphy - Set wiphy configuration.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following structure parameters are valid.
 * @info: Information related to wiphy parameters
 *
 * This structure represents the command to set the wireless PHY configuration.
 */

struct img_umac_cmd_set_wiphy {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_set_wiphy_info info;
} __IMG_PKD;

#define IMG_CMD_DEL_STATION_MAC_ADDR_VALID (1 << 0)
#define IMG_CMD_DEL_STATION_MGMT_SUBTYPE_VALID (1 << 1)
#define IMG_CMD_DEL_STATION_REASON_CODE_VALID (1 << 2)

/**
 * struct img_umac_del_sta_info - Information regarding a station to be
 *	deleted.
 * @mac_addr: MAC address of the station.
 * @mgmt_subtype: Management frame subtype.
 * @reason_code: Reason code for DEAUTHENTICATION and DISASSOCIATION.
 *
 * This structure represents the information regarding a station to be deleted
 * from the RPU.
 */

struct img_umac_del_sta_info {
	unsigned char mac_addr[IMG_ETH_ALEN];
	unsigned char mgmt_subtype;
	unsigned short reason_code;
} __IMG_PKD;

/**
 * struct img_umac_cmd_del_sta - Delete a station entry.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @info: Information regarding the station to be deleted.
 *
 * This structure represents the command to delete a station.
 */

struct img_umac_cmd_del_sta {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_del_sta_info info;
} __IMG_PKD;

/**
 * struct img_umac_get_sta_info - Station information get.
 * @mac_addr: MAC address of the station.
 * This structure represents the information regarding a station info to be get.
 */

struct img_umac_get_sta_info {
	unsigned char mac_addr[IMG_ETH_ALEN];
} __IMG_PKD;

/**
 * struct img_umac_cmd_get_sta - Get a station info.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @info: Information regarding the station to get.
 *
 * This structure represents the command to get station info.
 */

struct img_umac_cmd_get_sta {
	struct img_umac_hdr umac_hdr;
	struct img_umac_get_sta_info info;
} __IMG_PKD;

#define IMG_EXT_CAPABILITY_MAX_LEN 32

struct img_ext_capability {
	unsigned int ext_capability_len;
	unsigned char ext_capability[IMG_EXT_CAPABILITY_MAX_LEN];

} __IMG_PKD;

#define IMG_SUPPORTED_CHANNELS_MAX_LEN 64

struct img_supported_channels {
	unsigned int supported_channels_len;
	unsigned char supported_channels[IMG_SUPPORTED_CHANNELS_MAX_LEN];

} __IMG_PKD;

#define IMG_SUPPORTED_OPER_CLASSES_MAX_LEN 64

struct img_supported_oper_classes {
	unsigned int supported_oper_classes_len;
	unsigned char supported_oper_classes[IMG_SUPPORTED_OPER_CLASSES_MAX_LEN];

} __IMG_PKD;

#define IMG_STA_FLAGS2_MAX_LEN 64

struct img_sta_flags2 {
	unsigned int sta_flags2_len;
	unsigned char sta_flags2[IMG_STA_FLAGS2_MAX_LEN];

} __IMG_PKD;

#define IMG_CMD_SET_STATION_SUPP_RATES_VALID (1 << 0)
#define IMG_CMD_SET_STATION_AID_VALID (1 << 1)
#define IMG_CMD_SET_STATION_PEER_AID_VALID (1 << 2)
#define IMG_CMD_SET_STATION_STA_CAPABILITY_VALID (1 << 3)
#define IMG_CMD_SET_STATION_EXT_CAPABILITY_VALID (1 << 4)
#define IMG_CMD_SET_STATION_STA_VLAN_VALID (1 << 5)
#define IMG_CMD_SET_STATION_HT_CAPABILITY_VALID (1 << 6)
#define IMG_CMD_SET_STATION_VHT_CAPABILITY_VALID (1 << 7)
#define IMG_CMD_SET_STATION_OPMODE_NOTIF_VALID (1 << 9)
#define IMG_CMD_SET_STATION_SUPPORTED_CHANNELS_VALID (1 << 10)
#define IMG_CMD_SET_STATION_SUPPORTED_OPER_CLASSES_VALID (1 << 11)
#define IMG_CMD_SET_STATION_STA_FLAGS2_VALID (1 << 12)
#define IMG_CMD_SET_STATION_STA_WME_UAPSD_QUEUES_VALID (1 << 13)
#define IMG_CMD_SET_STATION_STA_WME_MAX_SP_VALID (1 << 14)
#define IMG_CMD_SET_STATION_LISTEN_INTERVAL_VALID (1 << 15)

/**
 * struct img_umac_chg_sta_info - Information about station entry to be updated.
 * @img_listen_interval: Listen interval as defined by IEEE 802.11 7.3.1.6.
 * @sta_vlan: Vlan interface station should belong to.
 * @aid: AID or zero for no change.
 * @img_peer_aid: Unused.
 * @sta_capability: Station capability.
 * @spare: Unused.
 * @supp_rates: Supported rates in IEEE 802.11 format.
 * @ext_capability: Extended capabilities of the station.
 * @supported_channels: Supported channels in IEEE 802.11 format.
 * @supported_oper_classes: Supported oper classes in IEEE 802.11 format.
 * @sta_flags2: Unused.
 * @ht_capability: HT capabilities of station.
 * @vht_capability: VHT capabilities of station.
 * @mac_addr: Station mac address.
 * @opmode_notif: Information if operating mode field is used.
 * @wme_uapsd_queues: Bitmap of queues configured for uapsd. Same format
 *	as the AC bitmap in the QoS info field.
 * @wme_max_sp: Max Service Period. same format as the MAX_SP in the
 *	QoS info field (but already shifted down).
 *
 * This structure represents the information needed to update a station entry
 * in the RPU.
 */

struct img_umac_chg_sta_info {
	int img_listen_interval;
	unsigned int sta_vlan;
	unsigned short aid;
	unsigned short img_peer_aid;
	unsigned short sta_capability;
	unsigned short spare;
	struct img_supp_rates supp_rates;
	struct img_ext_capability ext_capability;
	struct img_supported_channels supported_channels;
	struct img_supported_oper_classes supported_oper_classes;
	/*struct img_sta_flags2 sta_flags2;*/
	struct img_sta_flag_update sta_flags2;
	unsigned char ht_capability[IMG_HT_VHT_CAPABILITY_MAX_SIZE];
	unsigned char vht_capability[IMG_HT_VHT_CAPABILITY_MAX_SIZE];
	unsigned char mac_addr[IMG_ETH_ALEN];
	unsigned char opmode_notif;
	unsigned char wme_uapsd_queues;
	unsigned char wme_max_sp;
} __IMG_PKD;

/**
 * struct img_umac_cmd_chg_sta - Update station entry.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @info: Information about station entry to be updated.
 *
 * This structure represents the command to update the parameters of a
 * station entry.
 */

struct img_umac_cmd_chg_sta {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_chg_sta_info info;
} __IMG_PKD;

#define IMG_CMD_NEW_STATION_SUPP_RATES_VALID (1 << 0)
#define IMG_CMD_NEW_STATION_AID_VALID (1 << 1)
#define IMG_CMD_NEW_STATION_PEER_AID_VALID (1 << 2)
#define IMG_CMD_NEW_STATION_STA_CAPABILITY_VALID (1 << 3)
#define IMG_CMD_NEW_STATION_EXT_CAPABILITY_VALID (1 << 4)
#define IMG_CMD_NEW_STATION_STA_VLAN_VALID (1 << 5)
#define IMG_CMD_NEW_STATION_HT_CAPABILITY_VALID (1 << 6)
#define IMG_CMD_NEW_STATION_VHT_CAPABILITY_VALID (1 << 7)
#define IMG_CMD_NEW_STATION_OPMODE_NOTIF_VALID (1 << 9)
#define IMG_CMD_NEW_STATION_SUPPORTED_CHANNELS_VALID (1 << 10)
#define IMG_CMD_NEW_STATION_SUPPORTED_OPER_CLASSES_VALID (1 << 11)
#define IMG_CMD_NEW_STATION_STA_FLAGS2_VALID (1 << 12)
#define IMG_CMD_NEW_STATION_STA_WME_UAPSD_QUEUES_VALID (1 << 13)
#define IMG_CMD_NEW_STATION_STA_WME_MAX_SP_VALID (1 << 14)
#define IMG_CMD_NEW_STATION_LISTEN_INTERVAL_VALID (1 << 15)

/**
 * struct img_umac_add_sta_info - Information about a station entry to be added.
 * @img_listen_interval: Listen interval as defined by IEEE 802.11 7.3.1.6.
 * @sta_vlan: VLAN interface station should belong to.
 * @aid: AID or zero for no change.
 * @img_peer_aid: Unused.
 * @sta_capability: Station capability.
 * @spare: Unused.
 * @supp_rates: Supported rates in IEEE 802.11 format.
 * @ext_capability: Extended capabilities of the station.
 * @supported_channels: Supported channels in IEEE 802.11 format.
 * @supported_oper_classes: Supported oper classes in IEEE 802.11 format.
 * @sta_flags2: Unused.
 * @ht_capability: HT capabilities of station.
 * @vht_capability: VHT capabilities of station.
 * @mac_addr: Station mac address.
 * @opmode_notif: Information if operating mode field is used
 * @wme_uapsd_queues: Bitmap of queues configured for uapsd. same format
 *	as the AC bitmap in the QoS info field.
 * @wme_max_sp: Max Service Period. same format as the MAX_SP in the
 *	QoS info field (but already shifted down).
 *
 * This structure represents the information about a new station entry to be
 * added to the RPU.
 */

struct img_umac_add_sta_info {
	int img_listen_interval;
	unsigned int sta_vlan;
	unsigned short aid;
	unsigned short img_peer_aid;
	unsigned short sta_capability;
	unsigned short spare;
	struct img_supp_rates supp_rates;
	struct img_ext_capability ext_capability;
	struct img_supported_channels supported_channels;
	struct img_supported_oper_classes supported_oper_classes;
	/*struct img_sta_flags2 sta_flags2;*/
	struct img_sta_flag_update sta_flags2;
	unsigned char ht_capability[IMG_HT_VHT_CAPABILITY_MAX_SIZE];
	unsigned char vht_capability[IMG_HT_VHT_CAPABILITY_MAX_SIZE];
	unsigned char mac_addr[IMG_ETH_ALEN];
	unsigned char opmode_notif;
	unsigned char wme_uapsd_queues;
	unsigned char wme_max_sp;
} __IMG_PKD;

/**
 * struct img_umac_cmd_add_sta - Add station entry
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 *
 * This structure represents the commands to add a new station entry.
 */

struct img_umac_cmd_add_sta {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_add_sta_info info;
} __IMG_PKD;

#define IMG_CMD_BEACON_INFO_BEACON_INTERVAL_VALID (1 << 0)
#define IMG_CMD_BEACON_INFO_AUTH_TYPE_VALID (1 << 1)
#define IMG_CMD_BEACON_INFO_VERSIONS_VALID (1 << 2)
#define IMG_CMD_BEACON_INFO_CIPHER_SUITE_GROUP_VALID (1 << 3)
#define IMG_CMD_BEACON_INFO_INACTIVITY_TIMEOUT_VALID (1 << 4)
#define IMG_CMD_BEACON_INFO_FREQ_PARAMS_VALID (1 << 5)
#define IMG_CMD_BEACON_INFO_PRIVACY (1 << 0)
#define IMG_CMD_BEACON_INFO_CONTROL_PORT_NO_ENCRYPT (1 << 1)
#define IMG_CMD_BEACON_INFO_P2P_CTWINDOW_VALID (1 << 6)
#define IMG_CMD_BEACON_INFO_P2P_OPPPS_VALID (1 << 7)

/**
 * struct img_umac_start_ap_info - Attributes needed to start SoftAP operation.
 * @beacon_interval: Beacon frame interval.
 * @dtim_period: DTIM count.
 * @hidden_ssid: Send beacons with wildcard sssid.
 * @auth_type: Authentication type, see img_auth_type.
 * @smps_mode: Unused.
 * @img_flags: Beacon info flags.
 * @beacon_data: Beacon frame, See &struct img_beacon_data.
 * @ssid: SSID string, See &struct img_ssid.
 * @connect_common_info: Connect params, See &struct img_connect_common_info.
 * @freq_params: Channel info, See &struct freq_params.
 * @inactivity_timeout: Time to stop ap after inactivity period.
 * @p2p_go_ctwindow: P2P GO Client Traffic Window.
 * @p2p_opp_ps: Opportunistic power save allows P2P Group Owner to save power
 *	when all its associated clients are sleeping.
 *
 * This structure represents the attributes that need to be passed to the RPU
 * when starting a SoftAP.
 */

struct img_umac_start_ap_info {
	unsigned short beacon_interval;
	unsigned char dtim_period;
	int hidden_ssid;
	int auth_type;
	int smps_mode;
	unsigned int img_flags;
	struct img_beacon_data beacon_data;
	struct img_ssid ssid;
	struct img_connect_common_info connect_common_info;
	struct freq_params freq_params;
	unsigned short inactivity_timeout;
	unsigned char p2p_go_ctwindow;
	unsigned char p2p_opp_ps;
} __IMG_PKD;

/**
 * struct img_umac_cmd_start_ap - Start SoftAP
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @info: Attributes that need to be passed to the RPU when starting a SoftAP.
 *	*See &struct img_umac_start_ap_info)
 *
 * The struct img_umac_cmd_start_ap is same for the following message types
 * %IMG_UMAC_CMD_NEW_BEACON
 * %IMG_UMAC_CMD_START_AP
 * (Refer &enum img_umac_commands).
 */

struct img_umac_cmd_start_ap {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_start_ap_info info;
} __IMG_PKD;

/**
 * struct img_umac_cmd_stop_ap - Stop SoftAP
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 *
 * This structure represents the command to stop the operation of a Soft AP.
 */

struct img_umac_cmd_stop_ap {
	struct img_umac_hdr umac_hdr;
} __IMG_PKD;

/**
 * struct img_umac_set_beacon_info - Attributes needed to set Beacon & Probe Rsp.
 * @beacon_data: Beacon frame, See &struct img_beacon_data.
 *
 * This structure represents the attributes that need to be passed to the RPU
 * when Beacon & Probe Rsp data settings.
 */

struct img_umac_set_beacon_info {
	struct img_beacon_data beacon_data;
} __IMG_PKD;

/**
 * struct img_umac_cmd_set_beacon - Set beacon data
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @info: Attributes that need to be passed to the RPU when Beacon &
 *	Probe response data to set.
 *	See &struct img_umac_set_beacon_info)
 *
 * %IMG_UMAC_CMD_SET_BEACON
 * (Refer &enum img_umac_commands).
 */

struct img_umac_cmd_set_beacon {
	struct img_umac_hdr umac_hdr;
	struct img_umac_set_beacon_info info;
} __IMG_PKD;

#define IMG_SET_INTERFACE_IFTYPE_VALID (1 << 0)
#define IMG_SET_INTERFACE_USE_4ADDR_VALID (1 << 1)

/**
 * struct img_umac_chg_vif_attr_info - Interface attributes to be changed.
 * @iftype: Interface type, see &enum img_iftype.
 * @img_user_4addr: Unused.
 *
 * This structure represents the information to be passed to the RPU when
 * changing the attributes of a virtual interface.
 */

struct img_umac_chg_vif_attr_info {
	int iftype;
	int img_use_4addr;
} __IMG_PKD;

/**
 * struct img_umac_cmd_chg_vif_attr - Change virtual interface attributes.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @info: Interface attributes to be changed.
 *
 * This structure represents the command to change interface attributes.
 */

struct img_umac_cmd_chg_vif_attr {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_chg_vif_attr_info info;
} __IMG_PKD;

#define IFACENAMSIZ 16

/**
 * struct img_umac_chg_vif_state_info- Interface state information.
 * @state: Interface state (1 = UP / 0 = DOWN).
 * @ifacename: Interface name.
 *
 * This structure represents the information to be passed the RPU when changing
 * the state (up/down) of a virtual interface.
 */

struct img_umac_chg_vif_state_info {
	int state;
	char ifacename[IFACENAMSIZ];
} __IMG_PKD;

/**
 * struct img_umac_cmd_chg_vif_state- Change the interface state
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @info: Interface state information.
 *
 * This structure represents the command to change interface state.
 */

struct img_umac_cmd_chg_vif_state {
	struct img_umac_hdr umac_hdr;
	struct img_umac_chg_vif_state_info info;
} __IMG_PKD;

struct img_umac_event_vif_state {
	struct img_umac_hdr umac_hdr;
	int status;
} __IMG_PKD;

/**
 * struct img_umac_cmd_start_p2p_dev - Start P2P mode on an interface
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 *
 * This structure represents the command to start P2P mode on an interface.
 */

struct img_umac_cmd_start_p2p_dev {
	struct img_umac_hdr umac_hdr;
} __IMG_PKD;

/**
 * struct img_umac_cmd_stop_p2p_dev - stop p2p mode
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 *
 * This structure represents the command to stop P2P mode on an interface.
 */

struct img_umac_cmd_stop_p2p_dev {
	struct img_umac_hdr umac_hdr;
} __IMG_PKD;

#define IMG_CMD_FRAME_FREQ_VALID (1 << 0)
#define IMG_CMD_FRAME_DURATION_VALID (1 << 1)
#define IMG_CMD_SET_FRAME_FREQ_PARAMS_VALID (1 << 2)

#define IMG_CMD_FRAME_OFFCHANNEL_TX_OK (1 << 0)
#define IMG_CMD_FRAME_TX_NO_CCK_RATE (1 << 1)
#define IMG_CMD_FRAME_DONT_WAIT_FOR_ACK (1 << 2)

/**
 * struct img_umac_mgmt_tx_info - Information about a management frame to be
 *	transmitted.
 * @flags: OFFCHANNEL_TX_OK, NO_CCK_RATE, DONT_WAIT_FOR_ACK.
 * @dur: Duration field value.
 * @frame: Management frame to transmit.
 * @frequency: Channel.
 * @freq_params: Frequency configuration, See &struct freq_params.
 * @host_cookie: Identifier to be used for processing done event,
 *	see %IMG_UMAC_EVENT_FRAME_TX_STATUS.
 *
 * This structure represents the information about a management frame to be
 * transmitted.
 */

struct img_umac_mgmt_tx_info {
	unsigned int img_flags;
	unsigned int frequency;
	unsigned int dur;
	struct img_frame frame;
	struct freq_params freq_params;
	unsigned long long host_cookie;
} __IMG_PKD;

/**
 * struct img_umac_cmd_mgmt_tx - Tranmit a management frame.
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @info: Information about the management frame to be transmitted.
 *
 * This structure represents the command to transmit a management frame.
 */

struct img_umac_cmd_mgmt_tx {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct img_umac_mgmt_tx_info info;
} __IMG_PKD;

/**
 * struct img_umac_set_power_save_info - Information about power save
 *	settings.
 * @ps_state: power save is disabled or enabled, see enum img_ps_state.
 *
 * This structure represents the information about power save state
 */

struct img_umac_set_power_save_info {
	int ps_state;
} __IMG_PKD;

/**
 * struct img_umac_cmd_set_power_save - Set power save enable or disbale.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @info: Power save parameters settings.
 *
 * This structure represents the command to enable or disable the power
 * save functionality.
 */

struct img_umac_cmd_set_power_save {
	struct img_umac_hdr umac_hdr;
	struct img_umac_set_power_save_info info;
} __IMG_PKD;

/**
 * struct img_umac_qos_map_info - qos map info.
 * @qos_map_info_len: length of qos_map info field.
 * @qos_map_info: contains qos_map info as received from stack.
 *
 * This structure represents the information of qos_map.
 */

struct img_umac_qos_map_info {
	unsigned int qos_map_info_len;
	unsigned char qos_map_info[256];
} __IMG_PKD;

/**
 * struct img_umac_cmd_set_qos_map - Set qos map info.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @info: qos map info.
 *
 * This structure represents the command to pass qos_map info.
 */

struct img_umac_cmd_set_qos_map {
	struct img_umac_hdr umac_hdr;
	struct img_umac_qos_map_info info;
} __IMG_PKD;

#define IMG_SET_WOWLAN_FLAG_TRIG_ANY (1 << 0)
#define IMG_SET_WOWLAN_FLAG_TRIG_DISCONNECT (1 << 1)
#define IMG_SET_WOWLAN_FLAG_TRIG_MAGIC_PKT (1 << 2)
#define IMG_SET_WOWLAN_FLAG_TRIG_GTK_REKEY_FAILURE (1 << 3)
#define IMG_SET_WOWLAN_FLAG_TRIG_EAP_IDENT_REQUEST (1 << 4)
#define IMG_SET_WOWLAN_FLAG_TRIG_4WAY_HANDSHAKE (1 << 5)

/**
 * struct img_umac_set_wowlan_info - Information about wowlan
 *	trigger settings.
 * @img_flags: Wakeup trigger conditions. IMG_SET_WOWLAN_FLAG_TRIG_ANY,
 * IMG_SET_WOWLAN_FLAG_TRIG_DISCONNECT and etc.
 *
 * This structure represents the information about wowlan settings.
 */

struct img_umac_set_wowlan_info {
	unsigned short img_flags;
} __IMG_PKD;

/**
 * struct img_umac_cmd_set_wowlan - Setting Wake on WLAN configurations.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @info: WoWLAN wakeup trigger information.
 *
 * This structure represents the command to set the WoWLAN triger
 * configs before going to sleep(Host).
 */

struct img_umac_cmd_set_wowlan {
	struct img_umac_hdr umac_hdr;
	struct img_umac_set_wowlan_info info;
} __IMG_PKD;

/**
 * struct img_umac_cmd_suspend - suspend the bus transactions.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 *
 * This structure represents the command to suspend the bus transactions.
 */

struct img_umac_cmd_suspend {
	struct img_umac_hdr umac_hdr;
} __IMG_PKD;

/**
 * struct img_umac_cmd_resume - resume the bus transactions.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 *
 * This structure represents the command to resumes the bus transactions.
 */

struct img_umac_cmd_resume {
	struct img_umac_hdr umac_hdr;
} __IMG_PKD;

/**
 * struct img_umac_cmd_get_tx_power - get tx power.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 *
 * This structure represents the command to get tx power.
 */

struct img_umac_cmd_get_tx_power {
	struct img_umac_hdr umac_hdr;
} __IMG_PKD;

/**
 * struct img_umac_cmd_get_channel - get channel info.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 *
 * This structure represents the command to get channel information.
 */

struct img_umac_cmd_get_channel {
	struct img_umac_hdr umac_hdr;
} __IMG_PKD;

#ifdef TWT_SUPPORT
#define IMG_TWT_NEGOTIATION_TYPE_INDIVIDUAL 0
#define IMG_TWT_NEGOTIATION_TYPE_BROADCAST 2

/**
 * enum img_twt_setup_cmd_type - TWT Setup command/Response type.
 * @IMG_REQUEST_TWT:STA requests to join a TWT without specifying a target wake time.
 * @IMG_SUGGEST_TWT:STA requests to join a TWT with specifying a target wake time and other params.
 *		se values can change during negotiation.
 * @IMG_DEMAND_TWT:requests to join a TWT with demanded a target wake time and other params.
 *		STA rejects if AP not scheduling those params.
 * @IMG_GROUPING_TWT:Response to the STA request(suggest/demand), these are may be different params.
 * @IMG_ACCEPT_TWT: AP accept the STA requested params.
 * @IMG_ALTERNATE_TWT:AP may suggest the params, these may be different from STA requested.
 * @IMG_DICTATE_TWT:AP may suggest the params, these may be different from STA requested.
 * @IMG_REJECT_TWT: AP may reject the STA requested params.
 *
 * Types of TWT setup command/events.
 */
#define IMG_REQUEST_TWT 0
#define IMG_SUGGEST_TWT 1
#define IMG_DEMAND_TWT 2
#define IMG_GROUPING_TWT 3
#define IMG_ACCEPT_TWT 4
#define IMG_ALTERNATE_TWT 5
#define IMG_DICTATE_TWT 6
#define IMG_REJECT_TWT 7

#define IMG_TWT_FLOW_TYPE_ANNOUNCED 0
#define IMG_TWT_FLOW_TYPE_UNANNOUNCED 1
/**
 * struct img_umac_config_twt_info - TWT params info.
 * @twt_flow_id: TWT flow Id.
 * @neg_type:IMG_TWT_NEGOTIATION_TYPE_INDIVIDUAL/IMG_TWT_NEGOTIATION_TYPE_BROADAST
 * @setup_cmd: see enum img_twt_setup_cmd_type
 * @ap_trigger_frame: indicating AP to initiate a trigger frame(ps_poll/Null) before data transfer
 * @is_implicit:1->implicit(same negotiated values to be used), 0->AP sends new calculated TWT
 *	values for every service period.
 * @twt_flow_type: Whether STA has to send the PS-Poll/Null frame
 *		indicating that it's in wake period(IMG_TWT_FLOW_TYPE_ANNOUNCED)
 * @twt_target_wake_interval_exponent: wake interval exponent value
 * @twt_target_wake_interval_mantissa: wake interval mantissa value
 * @target_wake_time: start of the waketime value after successful TWT negotiation
 * @nominal_min_twt_wake_duration: min TWT wake duration
 * This structure represents the command provides TWT information.
 */

struct img_umac_config_twt_info {
	unsigned char twt_flow_id;
	unsigned char neg_type;
	int setup_cmd;
	unsigned char ap_trigger_frame;
	unsigned char is_implicit;
	unsigned char twt_flow_type;
	unsigned char twt_target_wake_interval_exponent;
	unsigned short twt_target_wake_interval_mantissa;
	/*unsigned char target_wake_time[8];*/
	unsigned long long target_wake_time;
	unsigned short nominal_min_twt_wake_duration;
} __IMG_PKD;

/**
 * struct img_umac_cmd_config_twt - configuring TWT params.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @info: refer to struct img_umac_config_twt_info.
 * This structure represents the command provides TWT information.
 */

struct img_umac_cmd_config_twt {
	struct img_umac_hdr umac_hdr;
	struct img_umac_config_twt_info info;
} __IMG_PKD;

/**
 * struct img_umac_teardown_twt_info - delete TWT params info.
 * @twt_flow_id: TWT flow Id.
 * This structure represents the command provides TWT delete information.
 */

struct img_umac_teardown_twt_info {
	unsigned char twt_flow_id;
} __IMG_PKD;

/**
 * struct img_umac_cmd_del_twt - delete TWT establishment.
 * @umac_hdr: UMAC command header. Refer &struct img_umac_hdr.
 * @info: refer to struct img_umac_teardown_twt_info.
 * This structure represents the command provides TWT delete establishment.
 */

struct img_umac_cmd_teardown_twt {
	struct img_umac_hdr umac_hdr;
	struct img_umac_teardown_twt_info info;
} __IMG_PKD;
#endif
/**
 * struct img_umac_event_trigger_scan - Scan complete event
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 *
 * This structure represents the event to indicate a scan complete and includes
 * the scan complete information.
 */

struct img_umac_event_trigger_scan {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	unsigned int img_scan_flags;
	unsigned char num_scan_ssid;
	unsigned char num_scan_frequencies;
	unsigned short scan_frequencies[IMG_SCAN_MAX_NUM_FREQUENCIES];
	struct img_ssid scan_ssid[IMG_SCAN_MAX_NUM_SSIDS];
	struct img_ie ie;
} __IMG_PKD;

#define IMG_EVENT_NEW_SCAN_RESULTS_MAC_ADDR_VALID (1 << 0)
#define IMG_EVENT_NEW_SCAN_RESULTS_IES_TSF_VALID (1 << 1)
#define IMG_EVENT_NEW_SCAN_RESULTS_IES_VALID (1 << 2)
#define IMG_EVENT_NEW_SCAN_RESULTS_BEACON_IES_TSF_VALID (1 << 3)
#define IMG_EVENT_NEW_SCAN_RESULTS_BEACON_IES_VALID (1 << 4)
#define IMG_EVENT_NEW_SCAN_RESULTS_BEACON_INTERVAL_VALID (1 << 5)
#define IMG_EVENT_NEW_SCAN_RESULTS_SIGNAL_VALID (1 << 6)
#define IMG_EVENT_NEW_SCAN_RESULTS_STATUS_VALID (1 << 7)
#define IMG_EVENT_NEW_SCAN_RESULTS_BSS_PRESP_DATA (1 << 8)

#define IMG_NEW_SCAN_RESULTS_BSS_PRESP_DATA (1 << 0)

/**
 * struct img_umac_event_new_scan_results - Scan result
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @generation: Used to indicate consistent snapshots for
 *	dumps. This number increases whenever the object list being
 *	dumped changes, and as such userspace can verify that it has
 *	obtained a complete and consistent snapshot by verifying that
 *	all dump messages contain the same generation number. If it
 *	changed then the list changed and the dump should be repeated
 *	completely from scratch.
 * @mac_addr: BSSID of the BSS (6 octets).
 * @ies_tsf: TSF of the received probe response/beacon (u64)
 *	(if @IMG_BSS_PRESP_DATA is present then this is known to be
 *	from a probe response, otherwise it may be from the same beacon
 *	that the IMG_BSS_BEACON_TSF will be from).
 * @ies: Binary attribute containing the
 *	raw information elements from the probe response/beacon (bin);
 *	if the %IMG_BSS_BEACON_IES attribute is present and the data is
 *	different then the IEs here are from a Probe Response frame; otherwise
 *	they are from a Beacon frame.
 *	However, if the driver does not indicate the source of the IEs, these
 *	IEs may be from either frame subtype.
 *	If present, the @IMG_BSS_PRESP_DATA attribute indicates that the
 *	data here is known to be from a probe response, without any heuristics.
 * @beacon_ies_tsf: TSF of the last received beacon
 *	(not present if no beacon frame has been received yet).
 * @beacon_ies: Binary attribute containing the raw information
 *	elements from a Beacon frame (bin); not present if no Beacon frame has
 *	yet been received.
 * @beacon_interval: Beacon interval of the (I)BSS.
 * @capability: Capability field (CPU order).
 * @frequency: Frequency in MHz.
 * @chan_width: Channel width of the control channel.
 * @seen_ms_ago: Age of this BSS entry in ms.
 * @signal: If MBMsignal strength of probe response/beacon
 *	in mBm (100 * dBm) (s32) or signal strength of the probe
 *	response/beacon in unspecified units, scaled to 0..100
 * @status: Status, if this BSS is "used".
 *
 * This structure is returned as a response for %IMG_UMAC_CMD_GET_SCAN_RESULTS. It
 * contains a scan result entry.
 */

struct img_umac_event_new_scan_results {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	unsigned int generation;
	unsigned int frequency;
	unsigned int chan_width;
	unsigned int seen_ms_ago;
	unsigned int img_flags;
	int status;
	unsigned long long ies_tsf;
	unsigned long long beacon_ies_tsf;
	unsigned short beacon_interval;
	unsigned short capability;
	struct img_ie ies;
	struct img_ie beacon_ies;
	struct img_signal signal;
	unsigned char mac_addr[IMG_ETH_ALEN];
} __IMG_PKD;

/**
 * struct img_umac_event_new_scan_display_results - Scan result for dispaly purpose
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @ssid: Network SSID.
 * @mac_addr: BSSID of the BSS (6 octets).
 * @nwk_band: Network band of operation, refer &enum img_band.
 * @nwk_channel: Network channel number.
 * @protocol: Network protocol, refer &enum img_wlan_protocol.
 * @security_type: Network security mode, refer &enum img_security_type.
 * @beacon_interval: Beacon interval of the (I)BSS.
 * @capability: Capability field (CPU order).
 * @signal: If MBMsignal strength of probe response/beacon
 *	in mBm (100 * dBm) (s32) or signal strength of the probe
 *	response/beacon in unspecified units, scaled to 0..100
 *
 * This structure is returned as a response for %IMG_UMAC_CMD_GET_SCAN_RESULTS. It
 * contains a scan result entry.
 */

#define IMG_802_11A (1 << 0)
#define IMG_802_11B (1 << 1)
#define IMG_802_11G (1 << 2)
#define IMG_802_11N (1 << 3)
#define IMG_802_11AC (1 << 4)
#define IMG_802_11AX (1 << 5)

struct umac_display_results {
	struct img_ssid ssid;
	unsigned char mac_addr[IMG_ETH_ALEN];
	int nwk_band;
	unsigned int nwk_channel;
	unsigned char protocol_flags;
	int security_type;
	unsigned short beacon_interval;
	unsigned short capability;
	struct img_signal signal;
	unsigned char reserved1;
	unsigned char reserved2;
	unsigned char reserved3;
	unsigned char reserved4;
} __IMG_PKD;

struct img_umac_event_new_scan_display_results {
	struct img_umac_hdr umac_hdr;
	/*Number of scan results in the current event*/
	unsigned char event_bss_count;
	struct umac_display_results display_results[10];
} __IMG_PKD;

#define IMG_EVENT_MLME_FRAME_VALID (1 << 0)
#define IMG_EVENT_MLME_MAC_ADDR_VALID (1 << 1)
#define IMG_EVENT_MLME_FREQ_VALID (1 << 2)
#define IMG_EVENT_MLME_COOKIE_VALID (1 << 3)
#define IMG_EVENT_MLME_RX_SIGNAL_DBM_VALID (1 << 4)
#define IMG_EVENT_MLME_WME_UAPSD_QUEUES_VALID (1 << 5)
#define IMG_EVENT_MLME_RXMGMT_FLAGS_VALID (1 << 6)
#define IMG_EVENT_MLME_TIMED_OUT (1 << 0)
#define IMG_EVENT_MLME_ACK (1 << 1)

/**
 * struct img_umac_event_mlme - MLME event
 *
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid
 * @frame: Frame data (binary attribute), including frame header
 *	and body, but not FCS; used, e.g., with IMG_UMAC_CMD_AUTHENTICATE and
 *	%IMG_UMAC_CMD_ASSOCIATE events.
 * @mac_addr: BSSID of the BSS (6 octets)
 * @frequency: Frequency of the selected channel in MHz
 * @cookie: Generic 64-bit cookie to identify objects.
 * @rx_signal_dbm: Signal strength in dBm (as a 32-bit int);
 *	this attribute is (depending on the driver capabilities) added to
 *	received frames indicated with %IMG_CMD_FRAME.
 * @wme_uapsd_queues: Bitmap of uapsd queues.
 * @img_flags: Indicate whether the frame was acked or timed out.
 *
 * This structure represents different STA MLME events for e.g. Authentication
 * Response received, Association Response received etc.
 *
 */

struct img_umac_event_mlme {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	unsigned int frequency;
	unsigned int rx_signal_dbm;
	unsigned int img_flags;
	unsigned long long cookie;
	struct img_frame frame;
	unsigned char mac_addr[IMG_ETH_ALEN];
	unsigned char wme_uapsd_queues;
} __IMG_PKD;

#define IMG_EVENT_CONNECT_STATUS_CODE_VALID (1 << 0)
#define IMG_EVENT_CONNECT_MAC_ADDR_VALID (1 << 1)
#define IMG_EVENT_CONNECT_REQ_IE_VALID (1 << 2)
#define IMG_EVENT_CONNECT_RESP_IE_VALID (1 << 3)
#define IMG_EVENT_CONNECT_AUTHORIZED_VALID (1 << 4)
#define IMG_EVENT_CONNECT_KEY_REPLAY_CTR_VALID (1 << 5)
#define IMG_EVENT_CONNECT_PTK_KCK_VALID (1 << 6)
#define IMG_EVENT_CONNECT_PTK_KEK_VALID (1 << 7)

/*
 * struct img_umac_event_connect - Connection complete event.
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate which of the following parameters are valid.
 * @status_code: StatusCode for the %IMG_CMD_CONNECT event.
 * @mac_addr: BSSID of the BSS (6 octets).
 * @req_ie: if TRUE connect_ie will be correspnding to req_ie
 * @resp_ie: if TRUE connect_ie will be correspnding to resp_ie
 * @connect_ie: (Re)association request/response information elements as sent by peer,
 *	for ROAM and successful CONNECT events.
 *
 */

struct img_umac_event_connect {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	unsigned short status_code;
	unsigned char mac_addr[IMG_ETH_ALEN];
	unsigned char req_ie;
	unsigned char resp_ie;
	struct img_ie connect_ie;

} __IMG_PKD;

#define IMG_CMD_SEND_STATION_ASSOC_REQ_IES_VALID (1 << 0)

/**
 * struct img_umac_event_new_station - Station add event.
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate if assoc_req ies is valid.
 * @is_sta_legacy: Set to 1 if STA is Legacy(a/b/g)
 * @wme: set to 1: STA supports QoS/WME
 * @mac_addr: Station mac address.
 * @generation: generation number
 * @sta_info: Station information.
 * @assoc_req_ies: Ies passed by station doing assoc request.
 *
 * This structure represents an event which is generated when a station is
 * added or deleted.
 */

struct img_umac_event_new_station {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	unsigned char wme;
	unsigned char is_sta_legacy;
	unsigned char mac_addr[IMG_ETH_ALEN];
	unsigned int generation;
	struct img_sta_info sta_info;
	struct img_ie assoc_req_ies;

} __IMG_PKD;

#define IMG_CMD_COOKIE_RSP_COOKIE_VALID (1 << 0)
#define IMG_CMD_COOKIE_RSP_MAC_ADDR_VALID (1 << 1)

/**
 * struct	img_umac_event_cookie_rsp - Cookie for management frame.
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @valid_fields: Indicate if assoc_req ies is valid.
 * @host_cookie: Identifier passed during %IMG_UMAC_CMD_FRAME.
 * @cookie: Cookie used to indicate TX done in %IMG_UMAC_EVENT_FRAME_TX_STATUS
 *
 * We receive an RPU cookie that is associated with the host cookie
 * passed during IMG_UMAC_CMD_FRAME
 */

struct img_umac_event_cookie_rsp {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	unsigned long long host_cookie;
	unsigned long long cookie;
	unsigned char mac_addr[IMG_ETH_ALEN];

} __IMG_PKD;

/**
 * struct	img_umac_event_get_txpwr - Tx power.
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @txpwr_level: Tx power level
 *
 * Tx power information in dbm
 */

struct img_umac_event_get_tx_power {
	struct img_umac_hdr umac_hdr;
	int txpwr_level;

} __IMG_PKD;

/**
 * struct	img_umac_event_set_interface - set interface status.
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @return_value: return value
 *
 * IMG_UMAC_CMD_SET_INTERFACE status
 */

struct img_umac_event_set_interface {
	struct img_umac_hdr umac_hdr;
	int return_value;
} __IMG_PKD;

/**
 * enum img_channel_flags - channel flags
 *
 * Channel flags set by the regulatory control code.
 *
 * @CHAN_DISABLED: This channel is disabled.
 * @CHAN_NO_IR: do not initiate radiation, this includes
 * sending probe requests or beaconing.
 * @CHAN_RADAR: Radar detection is required on this channel.
 * @CHAN_NO_HT40PLUS: extension channel above this channel
 *	is not permitted.
 * @CHAN_NO_HT40MINUS: extension channel below this channel
 *	is not permitted.
 * @CHAN_NO_OFDM: OFDM is not allowed on this channel.
 * @CHAN_NO_80MHZ: If the driver supports 80 MHz on the band,
 *	this flag indicates that an 80 MHz channel cannot use this
 *	channel as the control or any of the secondary channels.
 *	This may be due to the driver or due to regulatory bandwidth
 *	restrictions.
 * @CHAN_NO_160MHZ: If the driver supports 160 MHz on the band,
 *	this flag indicates that an 160 MHz channel cannot use this
 *	channel as the control or any of the secondary channels.
 *	This may be due to the driver or due to regulatory bandwidth
 *	restrictions.
 * @CHAN_INDOOR_ONLY: see %NL80211_FREQUENCY_ATTR_INDOOR_ONLY
 * @CHAN_GO_CONCURRENT: see %NL80211_FREQUENCY_ATTR_GO_CONCURRENT
 * @CHAN_NO_20MHZ: 20 MHz bandwidth is not permitted
 *	on this channel.
 * @CHAN_NO_10MHZ: 10 MHz bandwidth is not permitted
 *	on this channel.
 *
 */
#define CHAN_DISABLED (1 << 0)
#define CHAN_NO_IR (1 << 1)
/* hole at 1<<2 */
#define CHAN_RADAR (1 << 3)
#define CHAN_NO_HT40PLUS (1 << 4)
#define CHAN_NO_HT40MINUS (1 << 5)
#define CHAN_NO_OFDM (1 << 6)
#define CHAN_NO_80MHZ (1 << 7)
#define CHAN_NO_160MHZ (1 << 8)
#define CHAN_INDOOR_ONLY (1 << 9)
#define CHAN_GO_CONCURRENT (1 << 10)
#define CHAN_NO_20MHZ (1 << 11)
#define CHAN_NO_10MHZ (1 << 12)

/**
 * struct img_chan_def - channel definition
 * @chan: the (control) channel
 * @width: channel width
 * @center_frequency1: center frequency of first segment
 * @center_frequency2: center frequency of second segment
 * (only with 80+80 MHz)
 */

struct img_chan_definition {
	struct img_channel chan;
	int width;
	unsigned int center_frequency1;
	unsigned int center_frequency2;
} __IMG_PKD;

/**
 * struct	img_umac_event_get_channel - Get channel info.
 * @umac_hdr: UMAC event header. Refer &struct img_umac_hdr.
 * @chan_def: Channel definition.
 *
 * The structure gives Channel information.
 */

struct img_umac_event_get_channel {
	struct img_umac_hdr umac_hdr;
	struct img_chan_definition chan_def;
} __IMG_PKD;

struct umac_cmd_ibss_join {
	struct img_umac_hdr umac_hdr;
	struct img_ssid ssid;
	struct freq_params freq_params;
	unsigned char img_bssid[IMG_ETH_ALEN];
} __IMG_PKD;

struct img_cmd_leave_ibss {
	struct img_umac_hdr umac_hdr;

} __IMG_PKD;

struct img_umac_cmd_win_sta_connect {
	struct img_umac_hdr umac_hdr;
	unsigned int center_frequency;
	int auth_type;
	struct img_ie wpa_ie;
	struct img_ssid ssid;
	unsigned char img_bssid[IMG_ETH_ALEN];
} __IMG_PKD;

/* CMD_START_P2P_DEVICE */

struct img_cmd_start_p2p {
	struct img_umac_hdr umac_hdr;
} __IMG_PKD;

struct img_cmd_get_ifindex {
	struct img_umac_hdr umac_hdr;
	char ifacename[IFACENAMSIZ];
} __IMG_PKD;

#define IMG_TX_BITRATE_MASK_LEGACY_MAX_LEN 8
#define IMG_IEEE80211_HT_MCS_MASK_LEN 10
#define IMG_VHT_NSS_MAX 8

struct tx_rates {
	unsigned int legacy_len;
	unsigned int ht_mcs_len;
	unsigned int vht_mcs_len;
	unsigned char legacy[IMG_TX_BITRATE_MASK_LEGACY_MAX_LEN];
	unsigned char ht_mcs[IMG_IEEE80211_HT_MCS_MASK_LEN];
	unsigned short vht_mcs[IMG_VHT_NSS_MAX];

} __IMG_PKD;

#define IMG_SET_TX_BITRATE_MASK_BAND_2GHZ_VALID (1 << 0)
#define IMG_SET_TX_BITRATE_MASK_BAND_5GHZ_VALID (1 << 1)
#define IMG_SET_TX_BITRATE_MASK_BAND_60GHZ_VALID (1 << 2)

struct img_set_tx_bitrate_mask {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct tx_rates control[3];

} __IMG_PKD;

#define IMG_EVENT_TRIGGER_SCAN_IE_VALID (1 << 0)
#define IMG_EVENT_TRIGGER_SCAN_SCAN_FLAGS_VALID (1 << 1)

struct remain_on_channel_info {
	unsigned int dur;
	struct freq_params img_freq_params;
	unsigned long long host_cookie;
	unsigned long long cookie;

} __IMG_PKD;

#define IMG_CMD_ROC_FREQ_PARAMS_VALID (1 << 0)
#define IMG_CMD_ROC_DURATION_VALID (1 << 1)

struct img_umac_cmd_remain_on_channel {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	struct remain_on_channel_info info;

} __IMG_PKD;

#define IMG_CMD_CANCEL_ROC_COOKIE_VALID (1 << 0)

struct img_umac_cmd_cancel_remain_on_channel {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	unsigned long long cookie;

} __IMG_PKD;

#define IMG_EVENT_ROC_FREQ_VALID (1 << 0)
#define IMG_EVENT_ROC_COOKIE_VALID (1 << 1)
#define IMG_EVENT_ROC_DURATION_VALID (1 << 2)
#define IMG_EVENT_ROC_CH_TYPE_VALID (1 << 3)

struct img_event_remain_on_channel {
	struct img_umac_hdr umac_hdr;
	unsigned int valid_fields;
	unsigned int frequency;
	unsigned int dur;
	unsigned int ch_type;
	unsigned long long cookie;

} __IMG_PKD;

struct img_cmd_get_interface {
	struct img_umac_hdr umac_hdr;
} __IMG_PKD;

struct img_interface_info {
	struct img_umac_hdr umac_hdr;
#define IMG_INTERFACE_INFO_CHAN_DEF_VALID (1 << 0)
#define IMG_INTERFACE_INFO_SSID_VALID (1 << 1)
#define IMG_INTERFACE_INFO_IFNAME_VALID (1 << 2)
	unsigned int valid_fields;
	int img_iftype;
	char ifacename[IFACENAMSIZ];
	unsigned char img_eth_addr[IMG_ETH_ALEN];
	struct img_chan_definition chan_def;
	struct img_ssid ssid;
} __IMG_PKD;

struct img_event_mcs_info {
#define IMG_HT_MCS_MASK_LEN 10
	unsigned short img_rx_highest;
	unsigned char img_rx_mask[IMG_HT_MCS_MASK_LEN];
	unsigned char img_tx_params;
	unsigned char img_reserved[3];
} __IMG_PKD;

struct img_event_sta_ht_cap {
	int img_ht_supported;
	unsigned short img_cap;
	struct img_event_mcs_info mcs;
	unsigned char img_ampdu_factor;
	unsigned char img_ampdu_density;
} __IMG_PKD;

struct img_event_channel {
#define IMG_CHAN_FLAG_FREQUENCY_ATTR_NO_IR (1 << 0)
#define IMG_CHAN_FLAG_FREQUENCY_ATTR_NO_IBSS (1 << 1)
#define IMG_CHAN_FLAG_FREQUENCY_ATTR_RADAR (1 << 2)
#define IMG_CHAN_FLAG_FREQUENCY_ATTR_NO_HT40_MINUS (1 << 3)
#define IMG_CHAN_FLAG_FREQUENCY_ATTR_NO_HT40_PLUS (1 << 4)
#define IMG_CHAN_FLAG_FREQUENCY_ATTR_NO_80MHZ (1 << 5)
#define IMG_CHAN_FLAG_FREQUENCY_ATTR_NO_160MHZ (1 << 6)
#define IMG_CHAN_FLAG_FREQUENCY_ATTR_INDOOR_ONLY (1 << 7)
#define IMG_CHAN_FLAG_FREQUENCY_ATTR_GO_CONCURRENT (1 << 8)
#define IMG_CHAN_FLAG_FREQUENCY_ATTR_NO_20MHZ (1 << 9)
#define IMG_CHAN_FLAG_FREQUENCY_ATTR_NO_10MHZ (1 << 10)
#define IMG_CHAN_FLAG_FREQUENCY_DISABLED (1 << 11)

#define IMG_CHAN_DFS_VALID (1 << 12)
#define IMG_CHAN_DFS_CAC_TIME_VALID (1 << 13)
	unsigned short img_flags;
	int img_max_power;
	unsigned int img_time;
	unsigned int dfs_cac_msec;
	char ch_valid;
	unsigned short center_frequency;
	char dfs_state;
} __IMG_PKD;

struct img_event_rate {
#define IMG_EVENT_GET_WIPHY_FLAG_RATE_SHORT_PREAMBLE (1 << 0)
	unsigned short img_flags;
	unsigned short img_bitrate;
} __IMG_PKD;

struct img_event_vht_mcs_info {
	unsigned short rx_mcs_map;
	unsigned short rx_highest;
	unsigned short tx_mcs_map;
	unsigned short tx_highest;
} __IMG_PKD;

struct img_event_sta_vht_cap {
	char img_vht_supported;
	unsigned int img_cap;
	struct img_event_vht_mcs_info vht_mcs;
} __IMG_PKD;

struct img_event_supported_band {
	unsigned short img_n_channels;
	unsigned short img_n_bitrates;
	struct img_event_channel channels[26];
	struct img_event_rate bitrates[13];
	struct img_event_sta_ht_cap ht_cap;
	struct img_event_sta_vht_cap vht_cap;
	char band;
} __IMG_PKD;

struct img_event_iface_limit {
	unsigned short img_max;
	unsigned short img_types;
} __IMG_PKD;

struct img_event_iface_combination {
	unsigned int img_num_different_channels;
	int beacon_int_infra_match;
	struct img_event_iface_limit limits[2];
	unsigned short img_max_interfaces;
	unsigned char img_radar_detect_widths;
	unsigned char img_n_limits;
	unsigned char img_radar_detect_regions;

#define IMG_EVENT_GET_WIPHY_VALID_RADAR_DETECT_WIDTHS (1 << 0)
#define IMG_EVENT_GET_WIPHY_VALID_RADAR_DETECT_REGIONS (1 << 1)
#define IMG_EVENT_GET_WIPHY_VALID_ (1 << 2)
	unsigned char comb_valid;
} __IMG_PKD;

struct img_event_get_wiphy {
	struct img_umac_hdr umac_hdr;

	unsigned int img_frag_threshold;
	unsigned int img_rts_threshold;
	unsigned int img_available_antennas_tx;
	unsigned int img_available_antennas_rx;
	unsigned int img_probe_resp_offload;
	unsigned int tx_ant;
	unsigned int rx_ant;
	unsigned int split_start2_flags;
	unsigned int max_remain_on_channel_duration;
	unsigned int ap_sme_capa;
	unsigned int features;
	unsigned int max_acl_mac_addresses;
	unsigned int max_ap_assoc_sta;
#define IMG_EVENT_GET_WIPHY_MAX_CIPHER_COUNT 30
	unsigned int cipher_suites[IMG_EVENT_GET_WIPHY_MAX_CIPHER_COUNT];

#define IMG_EVENT_GET_WIPHY_IBSS_RSN (1 << 0)
#define IMG_EVENT_GET_WIPHY_MESH_AUTH (1 << 1)
#define IMG_EVENT_GET_WIPHY_AP_UAPSD (1 << 2)
#define IMG_EVENT_GET_WIPHY_SUPPORTS_FW_ROAM (1 << 3)
#define IMG_EVENT_GET_WIPHY_SUPPORTS_TDLS (1 << 4)
#define IMG_EVENT_GET_WIPHY_TDLS_EXTERNAL_SETUP (1 << 5)
#define IMG_EVENT_GET_WIPHY_CONTROL_PORT_ETHERTYPE (1 << 6)
#define IMG_EVENT_GET_WIPHY_OFFCHANNEL_TX_OK (1 << 7)
	unsigned int get_wiphy_flags;

#define IMG_GET_WIPHY_VALID_PROBE_RESP_OFFLOAD (1 << 0)
#define IMG_GET_WIPHY_VALID_TX_ANT (1 << 1)
#define IMG_GET_WIPHY_VALID_RX_ANT (1 << 2)
#define IMG_GET_WIPHY_VALID_MAX_NUM_SCAN_SSIDS (1 << 3)
#define IMG_GET_WIPHY_VALID_NUM_SCHED_SCAN_SSIDS (1 << 4)
#define IMG_GET_WIPHY_VALID_MAX_MATCH_SETS (1 << 5)
#define IMG_GET_WIPHY_VALID_MAC_ACL_MAX (1 << 6)
#define IMG_GET_WIPHY_VALID_HAVE_AP_SME (1 << 7)
#define IMG_GET_WIPHY_VALID_EXTENDED_CAPABILITIES (1 << 8)
#define IMG_GET_WIPHY_VALID_MAX_AP_ASSOC_STA (1 << 9)
#define IMG_GET_WIPHY_VALID_WIPHY_NAME (1 << 10)
#define IMG_GET_WIPHY_VALID_EXTENDED_FEATURES (1 << 11)
	unsigned int params_valid;

	unsigned short int max_scan_ie_len;
	unsigned short int max_sched_scan_ie_len;
	unsigned short int interface_modes;
	struct img_event_iface_combination iface_com[6];

	char supp_commands[40];
	unsigned char retry_short;
	unsigned char retry_long;
	unsigned char coverage_class;
	unsigned char max_scan_ssids;
	unsigned char max_sched_scan_ssids;
	unsigned char max_match_sets;
	unsigned char n_cipher_suites;
	unsigned char max_num_pmkids;
	unsigned char extended_capabilities_len;
	unsigned char extended_capabilities[10];
	unsigned char extended_capabilities_mask[10];
#define EXTENDED_FEATURE_LEN 60
#define DIV_ROUND_UP_NL(n, d) (((n) + (d)-1) / (d))

	unsigned char ext_features[DIV_ROUND_UP_NL(EXTENDED_FEATURE_LEN, 8)];
	unsigned char ext_features_len;
	char num_iface_com;
#define IMG_INDEX_IDS_WIPHY_NAME 32
	char wiphy_name[IMG_INDEX_IDS_WIPHY_NAME];

#define IMG_EVENT_GET_WIPHY_NUM_BANDS 3
	struct img_event_supported_band sband[IMG_EVENT_GET_WIPHY_NUM_BANDS];
} __IMG_PKD;

/* NL80211_CMD_GET_WIPHY */

struct img_cmd_get_wiphy {
	struct img_umac_hdr umac_hdr;
} __IMG_PKD;

struct img_cmd_get_ifhwaddr {
	struct img_umac_hdr umac_hdr;
	char ifacename[IFACENAMSIZ];
} __IMG_PKD;

struct img_cmd_set_ifhwaddr {
	struct img_umac_hdr umac_hdr;
	char ifacename[IFACENAMSIZ];
	unsigned char img_hwaddr[IMG_ETH_ALEN];
} __IMG_PKD;

struct img_reg_rules {
#define REG_RULE_FLAGS_VALID (1 << 0)
#define FREQ_RANGE_START_VALID (1 << 1)
#define FREQ_RANGE_END_VALID (1 << 2)
#define FREQ_RANGE_MAX_BW_VALID (1 << 3)
#define POWER_RULE_MAX_EIRP_VALID (1 << 4)
	unsigned int valid_fields;

	unsigned int rule_flags;
	unsigned int freq_range_start;
	unsigned int freq_range_end;
	unsigned int freq_range_max_bw;
	unsigned int pwr_max_eirp;

} __IMG_PKD;

/* NL80211_CMD_SET_REG , NL80211_CMD_GET_REG*/

struct img_reg {
	struct img_umac_hdr umac_hdr;

#define IMG_CMD_SET_REG_ALPHA2_VALID (1 << 0)
#define IMG_CMD_SET_REG_RULES_VALID (1 << 1)
#define IMG_CMD_SET_REG_DFS_REGION_VALID (1 << 2)
	unsigned int valid_fields;
	unsigned int dfs_region;
#define MAX_NUM_REG_RULES 32
	unsigned int num_reg_rules;
	struct img_reg_rules img_reg_rules[MAX_NUM_REG_RULES];
	unsigned char img_alpha2[3];
} __IMG_PKD;

/* NL80211_CMD_REQ_SET_REG */

struct img_cmd_req_set_reg {
	struct img_umac_hdr umac_hdr;

#define IMG_CMD_REQ_SET_REG_ALPHA2_VALID (1 << 0)
#define IMG_CMD_REQ_SET_REG_USER_REG_HINT_TYPE_VALID (1 << 1)
	unsigned int valid_fields;
	unsigned int img_user_reg_hint_type;
	unsigned char img_alpha2[3];
} __IMG_PKD;

struct img_event_send_beacon_hint {
	struct img_umac_hdr umac_hdr;
	struct img_event_channel channel_before;
	struct img_event_channel channel_after;

} __IMG_PKD;

#define IMG_EVNT_WIPHY_SELF_MANAGED (1 << 0)

struct img_event_regulatory_change {
	struct img_umac_hdr umac_hdr;
	unsigned short img_flags;
	int intr;
	char regulatory_type;
	unsigned char img_alpha2[2];

} __IMG_PKD;

struct img_umac_event_cmd_status {
	struct img_umac_hdr umac_hdr;
	unsigned int cmd_id;
	unsigned int cmd_status;
} __IMG_PKD;
#endif /* __HOST_RPU_UMAC_IF_H */
