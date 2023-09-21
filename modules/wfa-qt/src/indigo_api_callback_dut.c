/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <zephyr/net/net_ip.h>

#include <common/wpa_ctrl.h>
#include <ctrl_iface_zephyr.h>
#include <supp_main.h>

#include "indigo_api.h"
#include "vendor_specific.h"
#include "utils.h"
#include "wpa_ctrl.h"
#include "indigo_api_callback.h"
#include "hs2_profile.h"

static char pac_file_path[S_BUFFER_LEN] = {0};
struct interface_info *band_transmitter[16];
struct interface_info *band_first_wlan[16];
extern struct sockaddr_in *tool_addr;
int sta_configured;
int sta_started;

void register_apis(void)
{
	/* Basic */
	register_api(API_GET_IP_ADDR, NULL, get_ip_addr_handler);
	register_api(API_GET_MAC_ADDR, NULL, get_mac_addr_handler);
	register_api(API_GET_CONTROL_APP_VERSION, NULL, get_control_app_handler);
	register_api(API_START_LOOP_BACK_SERVER, NULL, start_loopback_server);
	register_api(API_STOP_LOOP_BACK_SERVER, NULL, stop_loop_back_server_handler);
	register_api(API_CREATE_NEW_INTERFACE_BRIDGE_NETWORK, NULL, create_bridge_network_handler);
	register_api(API_ASSIGN_STATIC_IP, NULL, assign_static_ip_handler);
	register_api(API_DEVICE_RESET, NULL, reset_device_handler);
	register_api(API_START_DHCP, NULL, start_dhcp_handler);
	register_api(API_STOP_DHCP, NULL, stop_dhcp_handler);
#ifdef CONFIG_WPS
	register_api(API_GET_WSC_PIN, NULL, get_wsc_pin_handler);
	register_api(API_GET_WSC_CRED, NULL, get_wsc_cred_handler);
	register_api(API_STA_START_WPS, NULL, start_wps_sta_handler);
	register_api(API_STA_ENABLE_WSC, NULL, enable_wsc_sta_handler);
#endif /* End Of CONFIG_WPS */
#ifdef CONFIG_AP
	/* AP */
	register_api(API_AP_START_UP, NULL, start_ap_handler);
	register_api(API_AP_STOP, NULL, stop_ap_handler);
	register_api(API_AP_CONFIGURE, NULL, configure_ap_handler);
	register_api(API_AP_TRIGGER_CHANSWITCH, NULL, trigger_ap_channel_switch);
	register_api(API_AP_SEND_DISCONNECT, NULL, send_ap_disconnect_handler);
	register_api(API_AP_SET_PARAM, NULL, set_ap_parameter_handler);
#ifdef CONFIG_WNM
	register_api(API_AP_SEND_BTM_REQ, NULL, send_ap_btm_handler);
#endif /* End Of CONFIG_WNM */
	register_api(API_AP_START_WPS, NULL, start_wps_ap_handler);
	register_api(API_AP_CONFIGURE_WSC, NULL, configure_ap_wsc_handler);
#endif /* End Of CONFIG_AP */
	/* STA */
	register_api(API_STA_ASSOCIATE, NULL, associate_sta_handler);
	register_api(API_STA_CONFIGURE, NULL, configure_sta_handler);
	register_api(API_STA_DISCONNECT, NULL, stop_sta_handler);
	register_api(API_STA_SEND_DISCONNECT, NULL, send_sta_disconnect_handler);
	register_api(API_STA_REASSOCIATE, NULL, send_sta_reconnect_handler);
	register_api(API_STA_SET_PARAM, NULL, set_sta_parameter_handler);
	register_api(API_STA_SCAN, NULL, sta_scan_handler);
#ifdef CONFIG_WNM
	register_api(API_STA_SEND_BTM_QUERY, NULL, send_sta_btm_query_handler);
#endif /* End Of CONFIG_WNM */
#ifdef CONFIG_HS20
	register_api(API_STA_SEND_ANQP_QUERY, NULL, send_sta_anqp_query_handler);
	register_api(API_STA_HS2_ASSOCIATE, NULL, set_sta_hs2_associate_handler);
	register_api(API_STA_ADD_CREDENTIAL, NULL, sta_add_credential_handler);
	register_api(API_STA_INSTALL_PPSMO, NULL, set_sta_install_ppsmo_handler);
#endif /* End Of CONFIG_HS20 */
	/* TODO: Add the handlers */
	register_api(API_STA_SET_CHANNEL_WIDTH, NULL, NULL);
	register_api(API_STA_POWER_SAVE, NULL, NULL);
#ifdef CONFIG_P2P
	register_api(API_P2P_START_UP, NULL, start_up_p2p_handler);
	register_api(API_P2P_FIND, NULL, p2p_find_handler);
	register_api(API_P2P_LISTEN, NULL, p2p_listen_handler);
	register_api(API_P2P_ADD_GROUP, NULL, add_p2p_group_handler);
	register_api(API_P2P_START_WPS, NULL, p2p_start_wps_handler);
	register_api(API_P2P_CONNECT, NULL, p2p_connect_handler);
	register_api(API_P2P_GET_INTENT_VALUE, NULL, get_p2p_intent_value_handler);
	register_api(API_P2P_INVITE, NULL, p2p_invite_handler);
	register_api(API_P2P_STOP_GROUP, NULL, stop_p2p_group_handler);
	register_api(API_P2P_SET_SERV_DISC, NULL, set_p2p_serv_disc_handler);
	register_api(API_P2P_SET_EXT_LISTEN, NULL, set_p2p_ext_listen_handler);
#endif /*End Of CONFIG_P2P */
}

static int get_control_app_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	char ipAddress[INET_ADDRSTRLEN];
	char buffer[S_BUFFER_LEN];
#ifdef _VERSION_
	snprintf(buffer, sizeof(buffer), "%s", _VERSION_);
#else
	snprintf(buffer, sizeof(buffer), "%s", TLV_VALUE_APP_VERSION);
#endif
	if (tool_addr) {
		inet_ntop(AF_INET, &(tool_addr->sin_addr), ipAddress, INET_ADDRSTRLEN);
		indigo_logger(LOG_LEVEL_DEBUG,
			      "Tool Control IP address on DUT network path: %s", ipAddress);
	}

	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, TLV_VALUE_STATUS_OK);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(TLV_VALUE_OK), TLV_VALUE_OK);
	fill_wrapper_tlv_bytes(resp, TLV_CONTROL_APP_VERSION, strlen(buffer), buffer);
	return 0;
}

static int reset_device_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_RESET_NOT_OK;
	char role[32], log_level[32], band[32];
	struct tlv_hdr *tlv = NULL;

	/* TLV: ROLE */
	tlv = find_wrapper_tlv_by_id(req, TLV_ROLE);
	memset(role, 0, sizeof(role));
	if (tlv) {
		memcpy(role, tlv->value, tlv->len);
	} else {
		goto done;
	}
	/* TLV: DEBUG_LEVEL */
	tlv = find_wrapper_tlv_by_id(req, TLV_DEBUG_LEVEL);
	memset(log_level, 0, sizeof(log_level));
	if (tlv) {
		memcpy(log_level, tlv->value, tlv->len);
	}
	/* TLV: TLV_BAND */
	memset(band, 0, sizeof(band));
	tlv = find_wrapper_tlv_by_id(req, TLV_BAND);
	if (tlv) {
		memcpy(band, tlv->value, tlv->len);
	}

	if (atoi(role) == DUT_TYPE_STAUT) {
		/* TODO: Implement this for zephyr */
	} else if (atoi(role) == DUT_TYPE_APUT) {
#ifdef CONFIG_AP
		/* TODO: Implement this for zephyr */
#endif /* End Of CONFIG_AP */
	} else if (atoi(role) == DUT_TYPE_P2PUT) {
#ifdef CONFIG_P2P
		/* If TP is P2P client, GO can't stop before client removes group monitor if */
		/* sprintf(buffer, "killall %s 1>/dev/null 2>/dev/null", get_wpas_exec_file()); */
		/* reset_interface_ip(get_wireless_interface()); */
		if (strlen(log_level)) {
			set_wpas_debug_level(get_debug_level(atoi(log_level)));
		}
#endif /* End Of CONFIG_P2P */
	}

	if (strcmp(band, TLV_BAND_24GHZ) == 0) {
		set_default_wireless_interface_info(BAND_24GHZ);
	} else if (strcmp(band, TLV_BAND_5GHZ) == 0) {
		set_default_wireless_interface_info(BAND_5GHZ);
	} else if (strcmp(band, TLV_BAND_6GHZ) == 0) {
		set_default_wireless_interface_info(BAND_6GHZ);
	}

	memset(band_transmitter, 0, sizeof(band_transmitter));
	memset(band_first_wlan, 0, sizeof(band_first_wlan));

	vendor_device_reset();

	k_sleep(K_SECONDS(1));

	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_RESET_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}


#ifdef CONFIG_AP
/* RESP: {<ResponseTLV.STATUS: 40961>: '0', */
/* <ResponseTLV.MESSAGE: 40960>: 'AP stop completed : Hostapd service is inactive.'} */
static int stop_ap_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int reset = 0, status = TLV_VALUE_STATUS_NOT_OK;
	char buffer[S_BUFFER_LEN], reset_type[16];
	char *message = TLV_VALUE_HOSTAPD_STOP_NOT_OK;
	struct tlv_hdr *tlv = NULL;

	/* TLV: RESET_TYPE */
	tlv = find_wrapper_tlv_by_id(req, TLV_RESET_TYPE);
	memset(reset_type, 0, sizeof(reset_type));
	if (tlv) {
		memcpy(reset_type, tlv->value, tlv->len);
		reset = atoi(reset_type);
		indigo_logger(LOG_LEVEL_DEBUG, "Reset Type: %d", reset);
	}

	/* TODO: Add functionality to stop Hostapd */

	/* reset interfaces info */
	clear_interfaces_resource();
	memset(band_transmitter, 0, sizeof(band_transmitter));
	memset(band_first_wlan, 0, sizeof(band_first_wlan));

	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}

#ifdef _RESERVED_
/* The function is reserved for the defeault hostapd config */
#define HOSTAPD_DEFAULT_CONFIG_SSID                 "QuickTrack"
#define HOSTAPD_DEFAULT_CONFIG_CHANNEL              "36"
#define HOSTAPD_DEFAULT_CONFIG_HW_MODE              "a"
#define HOSTAPD_DEFAULT_CONFIG_WPA_PASSPHRASE       "12345678"
#define HOSTAPD_DEFAULT_CONFIG_IEEE80211N           "1"
#define HOSTAPD_DEFAULT_CONFIG_WPA                  "2"
#define HOSTAPD_DEFAULT_CONFIG_WPA_KEY_MGMT         "WPA-PSK"
#define HOSTAPD_DEFAULT_CONFIG_RSN_PAIRWISE         "CCMP"

static void append_hostapd_default_config(struct packet_wrapper *wrapper)
{
	if (find_wrapper_tlv_by_id(wrapper, TLV_SSID) == NULL) {
		add_wrapper_tlv(wrapper, TLV_SSID,
				strlen(HOSTAPD_DEFAULT_CONFIG_SSID),
				HOSTAPD_DEFAULT_CONFIG_SSID);
	}
	if (find_wrapper_tlv_by_id(wrapper, TLV_CHANNEL) == NULL) {
		add_wrapper_tlv(wrapper, TLV_CHANNEL,
				strlen(HOSTAPD_DEFAULT_CONFIG_CHANNEL),
				HOSTAPD_DEFAULT_CONFIG_CHANNEL);
	}
	if (find_wrapper_tlv_by_id(wrapper, TLV_HW_MODE) == NULL) {
		add_wrapper_tlv(wrapper, TLV_HW_MODE,
				strlen(HOSTAPD_DEFAULT_CONFIG_HW_MODE),
				HOSTAPD_DEFAULT_CONFIG_HW_MODE);
	}
	if (find_wrapper_tlv_by_id(wrapper, TLV_WPA_PASSPHRASE) == NULL) {
		add_wrapper_tlv(wrapper, TLV_WPA_PASSPHRASE,
				strlen(HOSTAPD_DEFAULT_CONFIG_WPA_PASSPHRASE),
				HOSTAPD_DEFAULT_CONFIG_WPA_PASSPHRASE);
	}
	if (find_wrapper_tlv_by_id(wrapper, TLV_IEEE80211_N) == NULL) {
		add_wrapper_tlv(wrapper, TLV_IEEE80211_N,
				strlen(HOSTAPD_DEFAULT_CONFIG_IEEE80211N),
				HOSTAPD_DEFAULT_CONFIG_IEEE80211N);
	}
	if (find_wrapper_tlv_by_id(wrapper, TLV_WPA) == NULL) {
		add_wrapper_tlv(wrapper, TLV_WPA, strlen(HOSTAPD_DEFAULT_CONFIG_WPA),
				HOSTAPD_DEFAULT_CONFIG_WPA);
	}
	if (find_wrapper_tlv_by_id(wrapper, TLV_WPA_KEY_MGMT) == NULL) {
		add_wrapper_tlv(wrapper, TLV_WPA_KEY_MGMT,
				strlen(HOSTAPD_DEFAULT_CONFIG_WPA_KEY_MGMT),
				HOSTAPD_DEFAULT_CONFIG_WPA_KEY_MGMT);
	}
	if (find_wrapper_tlv_by_id(wrapper, TLV_RSN_PAIRWISE) == NULL) {
		add_wrapper_tlv(wrapper, TLV_RSN_PAIRWISE,
				strlen(HOSTAPD_DEFAULT_CONFIG_RSN_PAIRWISE),
				HOSTAPD_DEFAULT_CONFIG_RSN_PAIRWISE);
	}
}
#endif /* _RESERVED_ */

static int generate_hostapd_config(char *output, int output_size,
				   struct packet_wrapper *wrapper,
				   struct interface_info *wlanp)
{
	int has_sae = 0, has_wpa = 0, has_pmf = 0;
	int has_owe = 0, has_transition = 0, has_sae_groups = 0;
	int channel = 0, chwidth = 1, enable_ax = 0;
	int chwidthset = 0, enable_muedca = 0, vht_chwidthset = 0;
	int enable_ac = 0, enable_hs20 = 0;
	size_t i;
	int enable_wps = 0, use_mbss = 0;
	char buffer[S_BUFFER_LEN], cfg_item[2*BUFFER_LEN];
	char band[64], value[16];
	char country[16];
	struct tlv_to_config_name *cfg = NULL;
	struct tlv_hdr *tlv = NULL;
	int is_6g_only = 0, unsol_pr_resp_interval = 0;
	struct tlv_to_profile *profile = NULL;
	int semicolon_list_size = sizeof(semicolon_list) / sizeof(struct tlv_to_config_name);
	int hs20_icons_attached = 0;
	int is_multiple_bssid = 0;

	(void) output_size;

#if HOSTAPD_SUPPORT_MBSSID
	if ((wlanp->mbssid_enable && !wlanp->transmitter) || (band_first_wlan[wlanp->band])) {
		sprintf(output, "bss=%s\nctrl_interface=%s\n",
			wlanp->ifname, HAPD_CTRL_PATH_DEFAULT);
		is_multiple_bssid = 1;
	} else {
		sprintf(output,
			"ctrl_interface=%s\nctrl_interface_group=0\ninterface=%s\n",
			HAPD_CTRL_PATH_DEFAULT, wlanp->ifname);
	}
#else
	sprintf(output,
		"ctrl_interface=%s\nctrl_interface_group=0\ninterface=%s\n",
		HAPD_CTRL_PATH_DEFAULT, wlanp->ifname);
#endif

#ifdef _RESERVED_
	/* The function is reserved for the defeault hostapd config */
	append_hostapd_default_config(wrapper);
#endif

	memset(country, 0, sizeof(country));

	/* QCA WTS image doesn't apply 11ax, mu_edca, country, 11d, 11h in hostapd */
	for (i = 0; i < wrapper->tlv_num; i++) {
		tlv = wrapper->tlv[i];
		memset(buffer, 0, sizeof(buffer));
		memset(cfg_item, 0, sizeof(cfg_item));

		if (tlv->id == TLV_CHANNEL) {
			memset(value, 0, sizeof(value));
			memcpy(value, tlv->value, tlv->len);
			channel = atoi(value);
			if (is_multiple_bssid) {
				/* channel will be configured on the first wlan */
				continue;
			}
		}

		if (tlv->id == TLV_HE_6G_ONLY) {
			is_6g_only = 1;
			continue;
		}

		if (tlv->id == TLV_BSS_IDENTIFIER) {
			use_mbss = 1;
			if (is_band_enabled(BAND_6GHZ) && !wlanp->mbssid_enable) {
				strcat(output, "rnr=1\n");
			}
			continue;
		}

		/* This is used when hostapd will use multiple lines to
		 * configure multiple items in the same configuration parameter
		 * (use semicolon to separate multiple configurations)
		 */
		cfg = find_generic_tlv_config(tlv->id, semicolon_list, semicolon_list_size);
		if (cfg) {
			char *token = NULL, *delimit = ";";

			memcpy(buffer, tlv->value, tlv->len);
			token = strtok(buffer, delimit);

			while (token != NULL) {
				sprintf(cfg_item, "%s=%s\n", cfg->config_name, token);
				strcat(output, cfg_item);
				token = strtok(NULL, delimit);
			}
			continue;
		}

		if (tlv->id == TLV_HESSID && strstr(tlv->value, "self")) {
			char mac_addr[64];

			memset(mac_addr, 0, sizeof(mac_addr));
			get_mac_address(mac_addr, sizeof(mac_addr), get_wireless_interface());
			sprintf(cfg_item, "hessid=%s\n", mac_addr);
			strcat(output, cfg_item);
			continue;
		}

		/* profile config */
		profile = find_tlv_hs2_profile(tlv->id);
		if (profile) {
			char *hs2_config = 0;

			memcpy(buffer, tlv->value, tlv->len);
			if (((tlv->id == TLV_OSU_PROVIDERS_LIST) ||
			     (tlv->id == TLV_OPERATOR_ICON_METADATA)) &&
			     (!hs20_icons_attached)) {
				attach_hs20_icons(output);
				hs20_icons_attached = 1;
			}

			if (atoi(buffer) > profile->size) {
				indigo_logger(LOG_LEVEL_ERROR,
					      "profile index out of bound!: %d, array_size:%d",
					      atoi(buffer), profile->size);
			} else {
				hs2_config = (char *)profile->profile[atoi(buffer)];
			}

			sprintf(cfg_item, "%s", hs2_config);
			strcat(output, cfg_item);
			continue;
		}
		/* wps settings */
		if (tlv->id == TLV_WPS_ENABLE) {
			int j;
			/* To get AP wps vendor info */
			wps_setting *s = get_vendor_wps_settings(WPS_AP);

			if (!s) {
				indigo_logger(LOG_LEVEL_WARNING, "Failed to get APUT WPS settings");
				continue;
			}
			enable_wps = 1;
			memcpy(buffer, tlv->value, tlv->len);
			if (atoi(buffer) == WPS_ENABLE_OOB) {
				/* WPS OOB: Out-of-Box */
				for (j = 0; j < AP_SETTING_NUM; j++) {
					memset(cfg_item, 0, sizeof(cfg_item));
					sprintf(cfg_item, "%s=%s\n", s[j].wkey, s[j].value);
					strcat(output, cfg_item);
				}
				indigo_logger(LOG_LEVEL_INFO, "APUT Configure WPS: OOB.");
			} else if (atoi(buffer) == WPS_ENABLE_NORMAL) {
				/* WPS Normal: Configure manually. */
				for (j = 0; j < AP_SETTING_NUM; j++) {
					memset(cfg_item, 0, sizeof(cfg_item));
					/* set wps state */
					if ((atoi(s[j].attr) == atoi(WPS_OOB_ONLY)) &&
					    (!(memcmp(s[j].wkey, WPS_OOB_STATE,
						strlen(WPS_OOB_STATE))))) {
						/*set wps state to Configured compulsorily*/
						sprintf(cfg_item, "%s=%s\n",
							s[j].wkey, WPS_OOB_CONFIGURED);
					}
					/* set wps common settings */
					if (atoi(s[j].attr) == atoi(WPS_COMMON)) {
						sprintf(cfg_item, "%s=%s\n", s[j].wkey, s[j].value);
					}
					strcat(output, cfg_item);
				}
				indigo_logger(LOG_LEVEL_INFO,
					      "APUT Configure WPS: Manually Configured.");
			} else {
				indigo_logger(LOG_LEVEL_ERROR,
					      "Unknown WPS TLV value: %d (TLV ID 0x%04x)",
					      atoi(buffer), tlv->id);
			}
			continue;
		}

		/* wps er support. upnp */
		if (tlv->id == TLV_WPS_ER_SUPPORT) {
			memset(cfg_item, 0, sizeof(cfg_item));
			sprintf(cfg_item, "upnp_iface=%s\n", wlanp->ifname);
			strcat(output, cfg_item);
			memset(cfg_item, 0, sizeof(cfg_item));
			sprintf(cfg_item, "friendly_name=WPS Access Point\n");
			strcat(output, cfg_item);
			memset(cfg_item, 0, sizeof(cfg_item));
			sprintf(cfg_item, "model_description=Wireless Access Point\n");
			strcat(output, cfg_item);
			continue;
		}

		cfg = find_tlv_config(tlv->id);
		if (!cfg) {
			indigo_logger(LOG_LEVEL_ERROR,
				      "Unknown AP configuration name: TLV ID 0x%04x", tlv->id);
			continue;
		}
		if (tlv->id == TLV_WPA_KEY_MGMT && strstr(tlv->value, "SAE") &&
		    strstr(tlv->value, "WPA-PSK")) {
			has_transition = 1;
		}

		if (tlv->id == TLV_WPA_KEY_MGMT && strstr(tlv->value, "OWE")) {
			has_owe = 1;
		}

		if (tlv->id == TLV_WPA_KEY_MGMT && strstr(tlv->value, "SAE")) {
			has_sae = 1;
		}

		if (tlv->id == TLV_WPA && strstr(tlv->value, "2")) {
			has_wpa = 1;
		}

		if (tlv->id == TLV_IEEE80211_W) {
			has_pmf = 1;
		}

		if (tlv->id == TLV_HW_MODE) {
			memset(band, 0, sizeof(band));
			memcpy(band, tlv->value, tlv->len);
		}

		if (tlv->id == TLV_HE_OPER_CHWIDTH) {
			memset(value, 0, sizeof(value));
			memcpy(value, tlv->value, tlv->len);
			chwidth = atoi(value);
			chwidthset = 1;
		}

		if (tlv->id == TLV_VHT_OPER_CHWIDTH) {
			memset(value, 0, sizeof(value));
			memcpy(value, tlv->value, tlv->len);
			chwidth = atoi(value);
			vht_chwidthset = 1;
		}

		if (tlv->id == TLV_IEEE80211_AC && strstr(tlv->value, "1")) {
			enable_ac = 1;
		}

		if (tlv->id == TLV_IEEE80211_AX && strstr(tlv->value, "1")) {
			enable_ax = 1;
		}

		if (tlv->id == TLV_HE_MU_EDCA) {
			enable_muedca = 1;
		}

		if (tlv->id == TLV_SAE_GROUPS) {
			has_sae_groups = 1;
		}

		if (tlv->id == TLV_COUNTRY_CODE) {
			memcpy(country, tlv->value, tlv->len);
		}

		if (tlv->id == TLV_IEEE80211_H) {
		}

		if (tlv->id == TLV_HE_UNSOL_PR_RESP_CADENCE) {
			memset(value, 0, sizeof(value));
			memcpy(value, tlv->value, tlv->len);
			unsol_pr_resp_interval = atoi(value);
		}

		if (tlv->id == TLV_HS20 && strstr(tlv->value, "1")) {
			enable_hs20 = 1;
		}

		if (tlv->id == TLV_OWE_TRANSITION_BSS_IDENTIFIER) {
			struct bss_identifier_info bss_info;
			struct interface_info *wlan;
			int bss_identifier;
			char bss_identifier_str[8];

			memset(&bss_info, 0, sizeof(bss_info));
			memset(bss_identifier_str, 0, sizeof(bss_identifier_str));
			memcpy(bss_identifier_str, tlv->value, tlv->len);
			bss_identifier = atoi(bss_identifier_str);
			parse_bss_identifier(bss_identifier, &bss_info);
			wlan = get_wireless_interface_info(bss_info.band, bss_info.identifier);
			if (wlan == NULL) {
				wlan = assign_wireless_interface_info(&bss_info);
			}
			indigo_logger(LOG_LEVEL_DEBUG,
				      "TLV_OWE_TRANSITION_BSS_IDENTIFIER: TLV_BSS_IDENTIFIER 0x%x "
				      "identifier %d mapping ifname %s\n",
				      bss_identifier,
				      bss_info.identifier, wlan ? wlan->ifname : "n/a");
			if (wlan) {
				memcpy(buffer, wlan->ifname, strlen(wlan->ifname));
				sprintf(cfg_item, "%s=%s\n", cfg->config_name, buffer);
				strcat(output, cfg_item);
				if (has_owe) {
					memset(cfg_item, 0, sizeof(cfg_item));
					sprintf(cfg_item, "ignore_broadcast_ssid=1\n");
					strcat(output, cfg_item);
				}
			}
		} else {
			memcpy(buffer, tlv->value, tlv->len);
			sprintf(cfg_item, "%s=%s\n", cfg->config_name, buffer);
			strcat(output, cfg_item);
		}
	}

	/* add rf band according to TLV_BSS_IDENTIFIER/TLV_HW_MODE/TLV_WPS_ENABLE */
	if (enable_wps) {
		if (use_mbss) {
			/* The wps test for mbss should always be dual concurrent. */
			strcat(output, "wps_rf_bands=ag\n");
		} else {
			if (!strncmp(band, "a", 1)) {
				strcat(output, "wps_rf_bands=a\n");
			} else if (!strncmp(band, "g", 1)) {
				strcat(output, "wps_rf_bands=g\n");
			}
		}
	}

	if (has_pmf == 0) {
		if (has_transition) {
			strcat(output, "ieee80211w=1\n");
		} else if (has_sae && has_wpa) {
			strcat(output, "ieee80211w=2\n");
		} else if (has_owe) {
			strcat(output, "ieee80211w=2\n");
		} else if (has_wpa) {
			strcat(output, "ieee80211w=1\n");
		}
	}

	if (has_sae == 1) {
		strcat(output, "sae_require_mfp=1\n");
	}

#if HOSTAPD_SUPPORT_MBSSID
	if (wlanp->mbssid_enable && wlanp->transmitter) {
		strcat(output, "multiple_bssid=1\n");
	}
#endif

	/* Note: if any new DUT configuration is added for sae_groups,
	 * then the following unconditional sae_groups addition should be
	 * changed to become conditional on there being no other sae_groups
	 * configuration
	 * e.g.:
	 * if RequestTLV.SAE_GROUPS not in tlv_values:
	 * field_name = tlv_hostapd_config_mapper.get(RequestTLV.SAE_GROUPS)
	 * hostapd_config += "\n" + field_name + "=15 16 17 18 19 20 21"
	 * Append the default SAE groups for SAE and no SAE groups TLV
	 * if (has_sae && has_sae_groups == 0) {
	 *	strcat(output, "sae_groups=15 16 17 18 19 20 21\n");
	 * }
	 */

	/* Channel width configuration
	 * Default: 20MHz in 2.4G(No configuration required) 80MHz(40MHz for 11N only) in 5G
	 * if (enable_ac == 0 && enable_ax == 0) {
	 *	chwidth = 0;
	 * }
	 */
	/* Add country IE if there is no country config */
	if (strlen(country) == 0) {
		strcat(output, "ieee80211d=1\n");
		strcat(output, "country_code=US\n");
	}

	if (is_6g_only) {
		if (chwidthset == 0) {
			sprintf(buffer, "he_oper_chwidth=%d\n", chwidth);
			strcat(output, buffer);
		}
		if (chwidth == 1) {
			strcat(output, "op_class=133\n");
		} else if (chwidth == 2) {
			strcat(output, "op_class=134\n");
		}
		sprintf(buffer, "he_oper_centr_freq_seg0_idx=%d\n",
			get_6g_center_freq_index(channel, chwidth));
		strcat(output, buffer);
		if (unsol_pr_resp_interval) {
			sprintf(buffer,
				"unsol_bcast_probe_resp_interval=%d\n",
				unsol_pr_resp_interval);
			strcat(output, buffer);
		} else {
			strcat(output, "fils_discovery_max_interval=20\n");
		}
		/* Enable bss_color IE */
		strcat(output, "he_bss_color=19\n");
	} else if (strstr(band, "a")) {
		if (is_ht40plus_chan(channel)) {
			strcat(output, "ht_capab=[HT40+]\n");
		} else if (is_ht40minus_chan(channel)) {
			strcat(output, "ht_capab=[HT40-]\n");
		} else {/* Ch 165 and avoid hostapd configuration error */
			chwidth = 0;
		}
		if (chwidth > 0) {
			int center_freq = get_center_freq_index(channel, chwidth);

			if (enable_ac) {
				if (vht_chwidthset == 0) {
					sprintf(buffer, "vht_oper_chwidth=%d\n", chwidth);
					strcat(output, buffer);
				}
				sprintf(buffer, "vht_oper_centr_freq_seg0_idx=%d\n", center_freq);
				strcat(output, buffer);
			}
			if (enable_ax) {
			}
		}
	}

	if (enable_muedca) {
		strcat(output, "he_mu_edca_qos_info_queue_request=1\n");
		strcat(output, "he_mu_edca_ac_be_aifsn=0\n");
		strcat(output, "he_mu_edca_ac_be_ecwmin=15\n");
		strcat(output, "he_mu_edca_ac_be_ecwmax=15\n");
		strcat(output, "he_mu_edca_ac_be_timer=255\n");
		strcat(output, "he_mu_edca_ac_bk_aifsn=0\n");
		strcat(output, "he_mu_edca_ac_bk_aci=1\n");
		strcat(output, "he_mu_edca_ac_bk_ecwmin=15\n");
		strcat(output, "he_mu_edca_ac_bk_ecwmax=15\n");
		strcat(output, "he_mu_edca_ac_bk_timer=255\n");
		strcat(output, "he_mu_edca_ac_vi_ecwmin=15\n");
		strcat(output, "he_mu_edca_ac_vi_ecwmax=15\n");
		strcat(output, "he_mu_edca_ac_vi_aifsn=0\n");
		strcat(output, "he_mu_edca_ac_vi_aci=2\n");
		strcat(output, "he_mu_edca_ac_vi_timer=255\n");
		strcat(output, "he_mu_edca_ac_vo_aifsn=0\n");
		strcat(output, "he_mu_edca_ac_vo_aci=3\n");
		strcat(output, "he_mu_edca_ac_vo_ecwmin=15\n");
		strcat(output, "he_mu_edca_ac_vo_ecwmax=15\n");
		strcat(output, "he_mu_edca_ac_vo_timer=255\n");
	}

	if (enable_hs20) {
		strcat(output, "hs20_release=3\n");
		strcat(output, "manage_p2p=1\n");
		strcat(output, "allow_cross_connection=0\n");
		strcat(output, "bss_load_update_period=100\n");
		strcat(output, "hs20_deauth_req_timeout=3\n");
	}

	/* vendor specific config, not via hostapd */
	configure_ap_radio_params(band, country, channel, chwidth);

	return strlen(output);
}

/* RESP: {<ResponseTLV.STATUS: 40961>: '0', */
/* <ResponseTLV.MESSAGE: 40960>: 'DUT configured as AP : Configuration file created'} */
static int configure_ap_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int len = 0;
	char buffer[L_BUFFER_LEN], ifname[S_BUFFER_LEN];
	struct tlv_hdr *tlv;
	char *message = "DUT configured as AP : Configuration file created";
	int bss_identifier = 0, band;
	struct interface_info *wlan = NULL;
	char bss_identifier_str[16], hw_mode_str[8];
	struct bss_identifier_info bss_info;

	memset(buffer, 0, sizeof(buffer));
	tlv = find_wrapper_tlv_by_id(req, TLV_BSS_IDENTIFIER);
	memset(ifname, 0, sizeof(ifname));
	memset(&bss_info, 0, sizeof(bss_info));
	if (tlv) {
		/* Multiple wlans configure must carry TLV_BSS_IDENTIFIER */
		memset(bss_identifier_str, 0, sizeof(bss_identifier_str));
		memcpy(bss_identifier_str, tlv->value, tlv->len);
		bss_identifier = atoi(bss_identifier_str);
		parse_bss_identifier(bss_identifier, &bss_info);
		wlan = get_wireless_interface_info(bss_info.band, bss_info.identifier);
		if (wlan == NULL) {
			wlan = assign_wireless_interface_info(&bss_info);
		}
		if (wlan && bss_info.mbssid_enable) {
			configure_ap_enable_mbssid();
			if (bss_info.transmitter) {
				band_transmitter[bss_info.band] = wlan;
			}
		}
		indigo_logger(LOG_LEVEL_DEBUG,
			      "TLV_BSS_IDENTIFIER 0x%x band %d multiple_bssid %d "
			      "transmitter %d identifier %d\n",
			      bss_identifier,
			      bss_info.band,
			      bss_info.mbssid_enable,
			      bss_info.transmitter,
			      bss_info.identifier);
	} else {
		/* Single wlan case */
		tlv = find_wrapper_tlv_by_id(req, TLV_HW_MODE);
		if (tlv) {
			memset(hw_mode_str, 0, sizeof(hw_mode_str));
			memcpy(hw_mode_str, tlv->value, tlv->len);
			if (find_wrapper_tlv_by_id(req, TLV_HE_6G_ONLY)) {
				band = BAND_6GHZ;
			} else if (!strncmp(hw_mode_str, "a", 1)) {
				band = BAND_5GHZ;
			} else {
				band = BAND_24GHZ;
			}
			/* Single wlan use ID 1 */
			bss_info.band = band;
			bss_info.identifier = 1;
			wlan = assign_wireless_interface_info(&bss_info);
		}
	}
	if (wlan) {
		indigo_logger(LOG_LEVEL_DEBUG, "ifname %s hostapd conf file %s\n",
			      wlan ? wlan->ifname : "n/a",
			      wlan ? wlan->hapd_conf_file : "n/a");
		len = generate_hostapd_config(buffer, sizeof(buffer), req, wlan);
		if (len) {
#if HOSTAPD_SUPPORT_MBSSID
			if (bss_info.mbssid_enable && !bss_info.transmitter) {
				if (band_transmitter[bss_info.band]) {
					indigo_logger(LOG_LEVEL_DEBUG, "Append bss conf to %s",
						band_transmitter[bss_info.band]->hapd_conf_file);
					append_file(band_transmitter[bss_info.band]->hapd_conf_file,
						    buffer, len);
				}
				memset(wlan->hapd_conf_file, 0, sizeof(wlan->hapd_conf_file));
			} else if (band_first_wlan[bss_info.band]) {
				indigo_logger(LOG_LEVEL_DEBUG,
					      "Append bss conf to %s",
					      band_first_wlan[bss_info.band]->hapd_conf_file);
				append_file(band_first_wlan[bss_info.band]->hapd_conf_file,
					    buffer, len);
				memset(wlan->hapd_conf_file, 0, sizeof(wlan->hapd_conf_file));
			} else
#endif
				write_file(wlan->hapd_conf_file, buffer, len);
		}

		if (!band_first_wlan[bss_info.band]) {
			/* For the first configured ap */
			band_first_wlan[bss_info.band] = wlan;
		}
	}
	show_wireless_interface_info();

	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS,
			      len > 0 ? TLV_VALUE_STATUS_OK : TLV_VALUE_STATUS_NOT_OK);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}

#ifdef HOSTAPD_SUPPORT_MBSSID_WAR
extern int use_openwrt_wpad;
#endif

/* RESP: {<ResponseTLV.STATUS: 40961>: '0', */
/* <ResponseTLV.MESSAGE: 40960>: 'AP is up : Hostapd service is active'} */
static int start_ap_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	char *message = TLV_VALUE_HOSTAPD_START_OK;
	char buffer[S_BUFFER_LEN];
	int len;
	int swap_hostapd = 0;

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%s -B -t -P /var/run/hostapd.pid -g %s %s -f /var/log/hostapd.log %s",
		get_hapd_full_exec_path(),
		get_hapd_global_ctrl_path(),
		get_hostapd_debug_arguments(),
		get_all_hapd_conf_files(&swap_hostapd));
	len = system(buffer);
	k_sleep(K_SECONDS(1));

	/* Bring up VAPs with MBSSID disable using WFA hostapd */
	if (swap_hostapd) {
#ifdef HOSTAPD_SUPPORT_MBSSID_WAR
		indigo_logger(LOG_LEVEL_INFO, "Use WFA hostapd for MBSSID disable VAPs with RNR");
		system("cp /overlay/hostapd /usr/sbin/hostapd");
		use_openwrt_wpad = 0;
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer,
			"%s -B -t -P /var/run/hostapd_1.pid %s -f /var/log/hostapd_1.log %s",
			get_hapd_full_exec_path(),
			get_hostapd_debug_arguments(),
			get_all_hapd_conf_files(&swap_hostapd));
		len = system(buffer);
		k_sleep(K_SECONDS(1));
#endif
	}

	bridge_init(get_wlans_bridge());

	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS,
			      len == 0 ? TLV_VALUE_STATUS_OK : TLV_VALUE_STATUS_NOT_OK);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}

/* RESP: {<ResponseTLV.STATUS: 40961>: '0', */
/*<ResponseTLV.MESSAGE: 40960>: 'Configure and start wsc ap successfully. (Configure and start)'}*/
static int configure_ap_wsc_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int len_1 = -1, len_2 = 0, len_3 = -1;
	char buffer[L_BUFFER_LEN], ifname[S_BUFFER_LEN];
	struct tlv_hdr *tlv;
	char *message = NULL;
	int bss_identifier = 0, band;
	struct interface_info *wlan = NULL;
	char bss_identifier_str[16], hw_mode_str[8];
	struct bss_identifier_info bss_info;
	int swap_hostapd = 0;

	/* Stop hostapd [Begin] */

	/* TODO: Add functionality to stop Hostapd */

	/* Stop hostapd [End] */

	/* Generate hostapd configuration file [Begin] */
	memset(buffer, 0, sizeof(buffer));
	tlv = find_wrapper_tlv_by_id(req, TLV_BSS_IDENTIFIER);
	memset(ifname, 0, sizeof(ifname));
	memset(&bss_info, 0, sizeof(bss_info));
	if (tlv) {
		/* Multiple wlans configure must carry TLV_BSS_IDENTIFIER */
		memset(bss_identifier_str, 0, sizeof(bss_identifier_str));
		memcpy(bss_identifier_str, tlv->value, tlv->len);
		bss_identifier = atoi(bss_identifier_str);
		parse_bss_identifier(bss_identifier, &bss_info);
		wlan = get_wireless_interface_info(bss_info.band, bss_info.identifier);
		if (wlan == NULL) {
			wlan = assign_wireless_interface_info(&bss_info);
		}
		if (wlan && bss_info.mbssid_enable) {
			configure_ap_enable_mbssid();
			if (bss_info.transmitter) {
				band_transmitter[bss_info.band] = wlan;
			}
		}
		printf("TLV_BSS_IDENTIFIER 0x%x band %d multiple_bssid %d "
			"transmitter %d identifier %d\n",
			bss_identifier,
			bss_info.band,
			bss_info.mbssid_enable,
			bss_info.transmitter,
			bss_info.identifier);
	} else {
		/* Single wlan case */
		/* reset interfaces info */
		clear_interfaces_resource();

		tlv = find_wrapper_tlv_by_id(req, TLV_HW_MODE);
		if (tlv) {
			memset(hw_mode_str, 0, sizeof(hw_mode_str));
			memcpy(hw_mode_str, tlv->value, tlv->len);
			if (find_wrapper_tlv_by_id(req, TLV_HE_6G_ONLY)) {
				band = BAND_6GHZ;
			} else if (!strncmp(hw_mode_str, "a", 1)) {
				band = BAND_5GHZ;
			} else {
				band = BAND_24GHZ;
			}
			/* Single wlan use ID 1 */
			bss_info.band = band;
			bss_info.identifier = 1;
			wlan = assign_wireless_interface_info(&bss_info);
		}
	}
	if (wlan) {
		printf("ifname %s hostapd conf file %s\n",
			wlan ? wlan->ifname : "n/a",
			wlan ? wlan->hapd_conf_file : "n/a");
		len_2 = generate_hostapd_config(buffer, sizeof(buffer), req, wlan);
		if (len_2) {
#if HOSTAPD_SUPPORT_MBSSID
			if (bss_info.mbssid_enable && !bss_info.transmitter) {
				if (band_transmitter[bss_info.band]) {
					append_file(band_transmitter[bss_info.band]->hapd_conf_file,
						    buffer, len_2);
				}
				memset(wlan->hapd_conf_file, 0, sizeof(wlan->hapd_conf_file));
			} else
#endif
				write_file(wlan->hapd_conf_file, buffer, len_2);
		} else {
			message = "Failed to generate hostapd configuration file.";
			goto done;
		}
	}
	show_wireless_interface_info();
	/* Generate hostapd configuration file [End] */

	tlv = find_wrapper_tlv_by_id(req, TLV_WSC_CONFIG_ONLY);
	if (tlv) {
		/* Configure only (without starting hostapd) */
		message = "Configure wsc ap successfully. (Configure only)";
		goto done;
	}

	/* Start hostapd [Begin] */

	/* TODO: Add functionality to start Hostapd */

	/* Bring up VAPs with MBSSID disable using WFA hostapd */
	if (swap_hostapd) {
#ifdef HOSTAPD_SUPPORT_MBSSID_WAR
		indigo_logger(LOG_LEVEL_INFO, "Use WFA hostapd for MBSSID disable VAPs with RNR");
		system("cp /overlay/hostapd /usr/sbin/hostapd");
		use_openwrt_wpad = 0;
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer,
			"%s -B -t -P /var/run/hostapd_1.pid %s -f /var/log/hostapd_1.log %s",
			get_hapd_full_exec_path(),
			get_hostapd_debug_arguments(),
			get_all_hapd_conf_files(&swap_hostapd));
		len_3 = system(buffer);
		k_sleep(K_SECONDS(1));
#endif
	}

	bridge_init(get_wlans_bridge());
	if (len_3 == 0) {
		message = "Confiugre and start wsc ap successfully. (Configure and start)";
	} else {
		message = "Failed to start hostapd.";
	}
	/* Start hostapd [End] */

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp,
			      TLV_STATUS,
			      (len_1 == 0 || len_2 > 0 || len_3 == 0) ?
			      TLV_VALUE_STATUS_OK : TLV_VALUE_STATUS_NOT_OK);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}

#ifdef CONFIG_WNM
static int send_ap_btm_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	size_t resp_len;
	char *message = NULL;
	struct tlv_hdr *tlv = NULL;
	struct wpa_ctrl *w = NULL;
	char request[4096];
	char response[4096];
	char buffer[1024];

	char bssid[256];
	char disassoc_imminent[256];
	char disassoc_timer[256];
	char candidate_list[256];
	char reassoc_retry_delay[256];
	char bss_term_bit[256];
	char bss_term_tsf[256];
	char bss_term_duration[256];

	memset(bssid, 0, sizeof(bssid));
	memset(disassoc_imminent, 0, sizeof(disassoc_imminent));
	memset(disassoc_timer, 0, sizeof(disassoc_timer));
	memset(candidate_list, 0, sizeof(candidate_list));
	memset(reassoc_retry_delay, 0, sizeof(reassoc_retry_delay));
	memset(bss_term_bit, 0, sizeof(bss_term_bit));
	memset(bss_term_tsf, 0, sizeof(bss_term_tsf));
	memset(bss_term_duration, 0, sizeof(bss_term_duration));

	/* ControlApp on DUT */
	/* TLV: BSSID (required) */
	tlv = find_wrapper_tlv_by_id(req, TLV_BSSID);
	if (tlv) {
		memcpy(bssid, tlv->value, tlv->len);
	}
	/* DISASSOC_IMMINENT            disassoc_imminent=%s */
	tlv = find_wrapper_tlv_by_id(req, TLV_DISASSOC_IMMINENT);
	if (tlv) {
		memcpy(disassoc_imminent, tlv->value, tlv->len);
	}
	/* DISASSOC_TIMER               disassoc_timer=%s */
	tlv = find_wrapper_tlv_by_id(req, TLV_DISASSOC_TIMER);
	if (tlv) {
		memcpy(disassoc_timer, tlv->value, tlv->len);
	}
	/* REASSOCIAITION_RETRY_DELAY   mbo=0:{}:0 */
	tlv = find_wrapper_tlv_by_id(req, TLV_REASSOCIAITION_RETRY_DELAY);
	if (tlv) {
		memcpy(reassoc_retry_delay, tlv->value, tlv->len);
	}
	/* CANDIDATE_LIST              pref=1 */
	tlv = find_wrapper_tlv_by_id(req, TLV_CANDIDATE_LIST);
	if (tlv) {
		memcpy(candidate_list, tlv->value, tlv->len);
	}
	/* BSS_TERMINATION              bss_term_bit */
	tlv = find_wrapper_tlv_by_id(req, TLV_BSS_TERMINATION);
	if (tlv) {
		memcpy(bss_term_bit, tlv->value, tlv->len);
	}
	/* BSS_TERMINATION_TSF          bss_term_tsf */
	tlv = find_wrapper_tlv_by_id(req, TLV_BSS_TERMINATION_TSF);
	if (tlv) {
		memcpy(bss_term_tsf, tlv->value, tlv->len);
	}
	/* BSS_TERMINATION_DURATION     bss_term_duration */
	tlv = find_wrapper_tlv_by_id(req, TLV_BSS_TERMINATION_DURATION);
	if (tlv) {
		memcpy(bss_term_duration, tlv->value, tlv->len);
	}

	/* Assemble hostapd command for BSS_TM_REQ */
	memset(request, 0, sizeof(request));
	sprintf(request, "BSS_TM_REQ %s", bssid);
	/*  disassoc_imminent=%s */
	if (strlen(disassoc_imminent)) {
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, " disassoc_imminent=%s", disassoc_imminent);
		strcat(request, buffer);
	}
	/* disassoc_timer=%s */
	if (strlen(disassoc_timer)) {
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, " disassoc_timer=%s", disassoc_timer);
		strcat(request, buffer);
	}
	/* reassoc_retry_delay=%s */
	if (strlen(reassoc_retry_delay)) {
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, " mbo=0:%s:0", reassoc_retry_delay);
		strcat(request, buffer);
	}
	/* if bss_term_bit && bss_term_tsf && bss_term_duration,
	 * then bss_term={bss_term_tsf},{bss_term_duration}
	 */
	if (strlen(bss_term_bit) && strlen(bss_term_tsf) && strlen(bss_term_duration)) {
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, " bss_term=%s,%s", bss_term_tsf, bss_term_duration);
		strcat(request, buffer);
	}
	/* candidate_list */
	if (strlen(candidate_list) && atoi(candidate_list) == 1) {
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, " pref=1");
		strcat(request, buffer);
	}
	indigo_logger(LOG_LEVEL_DEBUG, "cmd:%s", request);

	/* Open hostapd UDS socket */
	w = wpa_ctrl_open(get_hapd_ctrl_path());
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to hostapd");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_HOSTAPD_CTRL_NOT_OK;
		goto done;
	}
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, request, strlen(request), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		message = TLV_VALUE_HOSTAPD_RESP_NOT_OK;
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;
done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	if (w) {
		wpa_ctrl_close(w);
	}
	return 0;
}
#endif /* End Of CONFIG_WNM */
#endif /* End Of CONFIG_AP */

/* deprecated */
static int create_bridge_network_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int err = 0;
	char static_ip[S_BUFFER_LEN];
	struct tlv_hdr *tlv;
	char *message = TLV_VALUE_CREATE_BRIDGE_OK;

	/* TLV: TLV_STATIC_IP */
	memset(static_ip, 0, sizeof(static_ip));
	tlv = find_wrapper_tlv_by_id(req, TLV_STATIC_IP);
	if (tlv) {
		memcpy(static_ip, tlv->value, tlv->len);
	} else {
		message = TLV_VALUE_CREATE_BRIDGE_NOT_OK;
		err = -1;
		goto response;
	}

	/* Create new bridge */
	create_bridge(get_wlans_bridge());

	add_all_wireless_interface_to_bridge(get_wlans_bridge());

	set_interface_ip(get_wlans_bridge(), static_ip);

response:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS,
			      err >= 0 ? TLV_VALUE_STATUS_OK : TLV_VALUE_STATUS_NOT_OK);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}

/* Bytes to DUT : 01 50 06 00 ed ff ff 00 55 0c 31 39 32 2e 31 36 38 2e 31 30 2e 33 */
/* RESP :{<ResponseTLV.STATUS: 40961>: '0', */
/* <ResponseTLV.MESSAGE: 40960>: 'Static Ip successfully assigned to wireless interface'} */
static int assign_static_ip_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int len = 0;
	char buffer[64];
	struct tlv_hdr *tlv = NULL;
	char *ifname = NULL;
	char *message = TLV_VALUE_ASSIGN_STATIC_IP_OK;

	memset(buffer, 0, sizeof(buffer));
	tlv = find_wrapper_tlv_by_id(req, TLV_STATIC_IP);
	if (tlv) {
		memcpy(buffer, tlv->value, tlv->len);
	} else {
		message = "Failed.";
		goto response;
	}

	if (is_bridge_created()) {
		ifname = get_wlans_bridge();
	} else {
		ifname = get_wireless_interface();
	}

	/* Release IP address from interface */
	reset_interface_ip(ifname);
	/* Bring up interface */
	control_interface(ifname, "up");
	/* Set IP address with network mask */
	strcat(buffer, "/24");
	len = set_interface_ip(ifname, buffer);
	if (len) {
		message = TLV_VALUE_ASSIGN_STATIC_IP_NOT_OK;
	}

response:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS,
			      len == 0 ? TLV_VALUE_STATUS_OK : TLV_VALUE_STATUS_NOT_OK);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}

/* Bytes to DUT : 01 50 01 00 ee ff ff
 * ACK:  Bytes from DUT : 01 00 01 00 ee ff ff a0 01 01 30 a0 00 15 41 43 4b 3a
 * 20 43 6f 6d 6d 61 6e 64 20 72 65 63 65 69 76 65 64
 * RESP: {<ResponseTLV.STATUS: 40961>: '0',
 * <ResponseTLV.MESSAGE: 40960>: '9c:b6:d0:19:40:c7',
 * <ResponseTLV.DUT_MAC_ADDR: 40963>: '9c:b6:d0:19:40:c7'}
 */
static int get_mac_addr_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct tlv_hdr *tlv;
	struct wpa_ctrl *w = NULL;

	int status = TLV_VALUE_STATUS_NOT_OK;
	size_t resp_len = 0;
	char *message = TLV_VALUE_NOT_OK;

	char cmd[16];
	char response[L_BUFFER_LEN];

	char band[S_BUFFER_LEN];
	char ssid[S_BUFFER_LEN];
	char role[S_BUFFER_LEN];

	char connected_freq[S_BUFFER_LEN];
	char connected_ssid[S_BUFFER_LEN];
	char mac_addr[S_BUFFER_LEN];
	int bss_identifier = 0;
	struct interface_info *wlan = NULL;
	char bss_identifier_str[16];
	struct bss_identifier_info bss_info;
	char buff[S_BUFFER_LEN];
	struct wpa_supplicant *wpa_s = NULL;

	if (req->tlv_num == 0) {
		get_mac_address(mac_addr, sizeof(mac_addr), get_wireless_interface());
		status = TLV_VALUE_STATUS_OK;
		message = TLV_VALUE_OK;
		goto done;
	} else {
		/* TLV: TLV_ROLE */
		memset(role, 0, sizeof(role));
		tlv = find_wrapper_tlv_by_id(req, TLV_ROLE);
		if (tlv) {
			memcpy(role, tlv->value, tlv->len);
		}

		/* TLV: TLV_BAND */
		memset(band, 0, sizeof(band));
		tlv = find_wrapper_tlv_by_id(req, TLV_BAND);
		if (tlv) {
			memcpy(band, tlv->value, tlv->len);
		}

		/* TLV: TLV_SSID */
		memset(ssid, 0, sizeof(ssid));
		tlv = find_wrapper_tlv_by_id(req, TLV_SSID);
		if (tlv) {
			memcpy(ssid, tlv->value, tlv->len);
		}

		memset(&bss_info, 0, sizeof(bss_info));
		tlv = find_wrapper_tlv_by_id(req, TLV_BSS_IDENTIFIER);
		if (tlv) {
			memset(bss_identifier_str, 0, sizeof(bss_identifier_str));
			memcpy(bss_identifier_str, tlv->value, tlv->len);
			bss_identifier = atoi(bss_identifier_str);
			parse_bss_identifier(bss_identifier, &bss_info);
			indigo_logger(LOG_LEVEL_DEBUG,
				      "TLV_BSS_IDENTIFIER 0x%x identifier %d band %d\n",
				      bss_identifier,
				      bss_info.identifier,
				      bss_info.band);
		} else {
			bss_info.identifier = -1;
		}
	}

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	if (atoi(role) == DUT_TYPE_STAUT) {
		w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	} else if (atoi(role) == DUT_TYPE_P2PUT) {
#ifdef CONFIG_P2P
		/* Get P2P GO/Client or Device MAC */
		if (get_p2p_mac_addr(mac_addr, sizeof(mac_addr))) {
			indigo_logger(LOG_LEVEL_INFO,
				      "Can't find P2P Device MAC. Use wireless IF MAC");
			get_mac_address(mac_addr, sizeof(mac_addr), get_wireless_interface());
		}
		status = TLV_VALUE_STATUS_OK;
		message = TLV_VALUE_OK;
		goto done;
#endif /* End Of CONFIG_P2P */
	} else {
#ifdef CONFIG_AP
		wlan = get_wireless_interface_info(bss_info.band, bss_info.identifier);
		w = wpa_ctrl_open(get_hapd_ctrl_path_by_id(wlan));
#endif /* End Of CONFIG_AP */
	}

	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to %s",
			      atoi(role) == DUT_TYPE_STAUT ? "wpa_supplicant" : "hostapd");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_NOT_OK;
		goto done;
	}

	/* Assemble hostapd command */
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "STATUS");
	/* Send command to hostapd UDS socket */
	resp_len = sizeof(response) - 1;
	memset(response, 0, sizeof(response));
	wpa_ctrl_request(w, cmd, strlen(cmd), response, &resp_len, NULL);

	/* Check response */
	get_key_value(connected_freq, response, "freq");

	memset(mac_addr, 0, sizeof(mac_addr));
	if (atoi(role) == DUT_TYPE_STAUT) {
		get_key_value(connected_ssid, response, "ssid");
		get_key_value(mac_addr, response, "address");
	} else {
#if HOSTAPD_SUPPORT_MBSSID
		if (bss_info.identifier >= 0) {
			sprintf(buff, "ssid[%d]", wlan->hapd_bss_id);
			get_key_value(connected_ssid, response, buff);
			sprintf(buff, "bssid[%d]", wlan->hapd_bss_id);
			get_key_value(mac_addr, response, buff);
		} else {
			get_key_value(connected_ssid, response, "ssid[0]");
			get_key_value(mac_addr, response, "bssid[0]");
		}
#else
		get_key_value(connected_ssid, response, "ssid[0]");
		get_key_value(mac_addr, response, "bssid[0]");
#endif
	}

	if (bss_info.identifier >= 0) {
		indigo_logger(LOG_LEVEL_DEBUG, "Get mac_addr %s\n", mac_addr);
		status = TLV_VALUE_STATUS_OK;
		message = TLV_VALUE_OK;
		goto done;
	}

	/* Check band and connected freq*/
	if (strlen(band)) {
		int band_id = 0;

		if (strcmp(band, "2.4GHz") == 0) {
			band_id = BAND_24GHZ;
		} else if (strcmp(band, "5GHz") == 0) {
			band_id = BAND_5GHZ;
		} else if (strcmp(band, "6GHz") == 0) {
			band_id = BAND_6GHZ;
		}

		if (verify_band_from_freq(atoi(connected_freq), band_id) == 0) {
			status = TLV_VALUE_STATUS_OK;
			message = TLV_VALUE_OK;
		} else {
			status = TLV_VALUE_STATUS_NOT_OK;
			message = "Unable to get mac address associated with the given band";
			goto done;
		}
	}

	/* Check SSID and connected SSID */
	if (strlen(ssid)) {
		if (strcmp(ssid, connected_ssid) == 0) {
			status = TLV_VALUE_STATUS_OK;
			message = TLV_VALUE_OK;
		} else {
			status = TLV_VALUE_STATUS_NOT_OK;
			message = "Unable to get mac address associated with the given ssid";
			goto done;
		}
	}

	/* TODO: BSSID */

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	if (status == TLV_VALUE_STATUS_OK) {
		fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(mac_addr), mac_addr);
		fill_wrapper_tlv_bytes(resp, TLV_DUT_MAC_ADDR, strlen(mac_addr), mac_addr);
	} else {
		fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	}
	return 0;
}

static int start_loopback_server(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	char local_ip[256];
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_LOOPBACK_SVR_START_NOT_OK;
	char tool_udp_port[16];
#ifdef CONFIG_P2P
	char if_name[32];
#endif /* End Of CONFIG_P2P */

	/* Find network interface. If P2P Group or bridge exists,
	 * then use it. Otherwise, it uses the initiation value.
	 */
	memset(local_ip, 0, sizeof(local_ip));
#ifdef CONFIG_P2P
	if (get_p2p_group_if(if_name, sizeof(if_name)) == 0 &&
	    find_interface_ip(local_ip, sizeof(local_ip), if_name)) {
		indigo_logger(LOG_LEVEL_DEBUG, "use %s", if_name);
	} else if (find_interface_ip(local_ip, sizeof(local_ip), get_wlans_bridge())) {
#else
	if (find_interface_ip(local_ip, sizeof(local_ip), get_wlans_bridge())) {
#endif /* End Of CONFIG_P2P */
		indigo_logger(LOG_LEVEL_DEBUG, "use %s", get_wlans_bridge());
	} else if (find_interface_ip(local_ip, sizeof(local_ip), get_wireless_interface())) {
		indigo_logger(LOG_LEVEL_DEBUG, "use %s", get_wireless_interface());
/* #ifdef __TEST__ */
	} else if (find_interface_ip(local_ip, sizeof(local_ip), "eth0")) {
		indigo_logger(LOG_LEVEL_DEBUG, "use %s", "eth0");
/* #endif */
	} else {
		indigo_logger(LOG_LEVEL_ERROR, "No available interface");
		goto done;
	}
	/* Start loopback */
	if (!loopback_server_start(local_ip, tool_udp_port, LOOPBACK_TIMEOUT)) {
		status = TLV_VALUE_STATUS_OK;
		message = TLV_VALUE_LOOPBACK_SVR_START_OK;
	}
done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	fill_wrapper_tlv_bytes(resp, TLV_LOOP_BACK_SERVER_PORT,
			       strlen(tool_udp_port), tool_udp_port);

	return 0;
}

/* RESP: {<ResponseTLV.STATUS: 40961>: '0', */
/* <ResponseTLV.MESSAGE: 40960>: 'Loopback server in idle state'} */
static int stop_loop_back_server_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	/* Stop loopback */
	if (loopback_server_status()) {
		loopback_server_stop();
	}
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, TLV_VALUE_STATUS_OK);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE,
			       strlen(TLV_VALUE_LOOP_BACK_STOP_OK), TLV_VALUE_LOOP_BACK_STOP_OK);

	return 0;
}

#ifdef CONFIG_AP
static int send_ap_disconnect_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int len, status = TLV_VALUE_STATUS_NOT_OK;
	char buffer[S_BUFFER_LEN];
	char response[S_BUFFER_LEN];
	char address[32];
	char *message = NULL;
	struct tlv_hdr *tlv = NULL;
	struct wpa_ctrl *w = NULL;
	size_t resp_len;

	/* Check hostapd status. TODO: it may use UDS directly */

	/* Open hostapd UDS socket */
	w = wpa_ctrl_open(get_hapd_ctrl_path());
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to hostapd");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_HOSTAPD_CTRL_NOT_OK;
		goto done;
	}
	/* ControlApp on DUT */
	/* TLV: TLV_ADDRESS */
	memset(address, 0, sizeof(address));
	tlv = find_wrapper_tlv_by_id(req, TLV_ADDRESS);
	if (tlv) {
		memcpy(address, tlv->value, tlv->len);
	} else {
		indigo_logger(LOG_LEVEL_ERROR, "Missed TLV:Address");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_INSUFFICIENT_TLV;
		goto done;
	}
	/* Assemble hostapd command */
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "DISASSOCIATE %s reason=1", address);
	/* Send command to hostapd UDS socket */
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		message = TLV_VALUE_HOSTAPD_RESP_NOT_OK;
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_HOSTAPD_STOP_OK;
done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	if (w) {
		wpa_ctrl_close(w);
	}
	return 0;
}

static int set_ap_parameter_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	size_t resp_len;
	char *message = NULL;
	char buffer[8192];
	char response[1024];
	char param_name[32];
	char param_value[256];
	struct tlv_hdr *tlv = NULL;
	struct wpa_ctrl *w = NULL;

	/* Open hostapd UDS socket */
	w = wpa_ctrl_open(get_hapd_ctrl_path());
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to hostapd");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_HOSTAPD_CTRL_NOT_OK;
		goto done;
	}

	/* ControlApp on DUT */
	/* TLV: MBO_ASSOC_DISALLOW or GAS_COMEBACK_DELAY */
	memset(param_value, 0, sizeof(param_value));
	tlv = find_wrapper_tlv_by_id(req, TLV_MBO_ASSOC_DISALLOW);
	if (!tlv) {
		tlv = find_wrapper_tlv_by_id(req, TLV_GAS_COMEBACK_DELAY);
	}
	if (tlv && find_tlv_config_name(tlv->id) != NULL) {
		strcpy(param_name, find_tlv_config_name(tlv->id));
		memcpy(param_value, tlv->value, tlv->len);
	} else {
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_INSUFFICIENT_TLV;
		indigo_logger(LOG_LEVEL_ERROR,
			      "Missed TLV: TLV_MBO_ASSOC_DISALLOW or TLV_GAS_COMEBACK_DELAY");
		goto done;
	}
	/* Assemble hostapd command */
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "SET %s %s", param_name, param_value);
	/* Send command to hostapd UDS socket */
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		message = TLV_VALUE_HOSTAPD_RESP_NOT_OK;
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;
done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	if (w) {
		wpa_ctrl_close(w);
	}
	return 0;
}

static int trigger_ap_channel_switch(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	size_t resp_len;
	char *message = NULL;
	struct tlv_hdr *tlv = NULL;
	struct wpa_ctrl *w = NULL;
	char request[S_BUFFER_LEN];
	char response[S_BUFFER_LEN];

	char channel[64];
	char frequency[64];
	int freq, center_freq, offset;

	memset(channel, 0, sizeof(channel));
	memset(frequency, 0, sizeof(frequency));

	/* ControlApp on DUT */
	/* TLV: TLV_CHANNEL (required) */
	tlv = find_wrapper_tlv_by_id(req, TLV_CHANNEL);
	if (tlv) {
		memcpy(channel, tlv->value, tlv->len);
	} else {
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_INSUFFICIENT_TLV;
		indigo_logger(LOG_LEVEL_ERROR, "Missed TLV: TLV_CHANNEL");
		goto done;
	}
	/* TLV_FREQUENCY (required) */
	tlv = find_wrapper_tlv_by_id(req, TLV_FREQUENCY);
	if (tlv) {
		memcpy(frequency, tlv->value, tlv->len);
	} else {
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_INSUFFICIENT_TLV;
		indigo_logger(LOG_LEVEL_ERROR, "Missed TLV: TLV_FREQUENCY");
	}

	center_freq = 5000 + get_center_freq_index(atoi(channel), 1) * 5;
	freq = atoi(frequency);
	if ((center_freq == freq + 30) || (center_freq == freq - 10)) {
		offset = 1;
	} else {
		offset = -1;
	}
	/* Assemble hostapd command for channel switch */
	memset(request, 0, sizeof(request));
	sprintf(request,
		"CHAN_SWITCH 10 %s center_freq1=%d sec_channel_offset=%d bandwidth=80 vht",
		frequency, center_freq, offset);
	indigo_logger(LOG_LEVEL_INFO, "%s", request);

	/* Open hostapd UDS socket */
	w = wpa_ctrl_open(get_hapd_ctrl_path());
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to hostapd");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_HOSTAPD_CTRL_NOT_OK;
		goto done;
	}
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, request, strlen(request), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		message = TLV_VALUE_HOSTAPD_RESP_NOT_OK;
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;
done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	if (w) {
		wpa_ctrl_close(w);
	}
	return 0;
}
#endif /* End Of CONFIG_AP */

static int get_ip_addr_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = NULL;
	char buffer[64];
	struct tlv_hdr *tlv = NULL;
	char value[16];
#ifdef CONFIG_P2P
	char if_name[32];
#endif /* End Of CONFIG_P2P */
	int role = 0;

	memset(value, 0, sizeof(value));
	tlv = find_wrapper_tlv_by_id(req, TLV_ROLE);
	if (tlv) {
		memcpy(value, tlv->value, tlv->len);
		role = atoi(value);
	}

#ifdef CONFIG_P2P
	if (role == DUT_TYPE_P2PUT &&
		get_p2p_group_if(if_name, sizeof(if_name)) == 0 &&
		find_interface_ip(buffer, sizeof(buffer), if_name)) {
		status = TLV_VALUE_STATUS_OK;
		message = TLV_VALUE_OK;
	} else if (find_interface_ip(buffer, sizeof(buffer), get_wlans_bridge())) {
#else
	if (find_interface_ip(buffer, sizeof(buffer), get_wlans_bridge())) {
#endif /* End Of CONFIG_P2P */
		status = TLV_VALUE_STATUS_OK;
		message = TLV_VALUE_OK;
	} else if (find_interface_ip(buffer, sizeof(buffer), get_wireless_interface())) {
		status = TLV_VALUE_STATUS_OK;
		message = TLV_VALUE_OK;
	} else {
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_NOT_OK;
	}

	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	if (status == TLV_VALUE_STATUS_OK) {
		fill_wrapper_tlv_bytes(resp, TLV_DUT_WLAN_IP_ADDR, strlen(buffer), buffer);
	}
	return 0;
}

static int stop_sta_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	/* TODO: Implement this for zephyr */

	return 0;
}

#ifdef _RESERVED_
/* The function is reserved for the defeault wpas config */
#define WPAS_DEFAULT_CONFIG_SSID                    "QuickTrack"
#define WPAS_DEFAULT_CONFIG_WPA_KEY_MGMT            "WPA-PSK"
#define WPAS_DEFAULT_CONFIG_PROTO                   "RSN"
#define HOSTAPD_DEFAULT_CONFIG_RSN_PAIRWISE         "CCMP"
#define WPAS_DEFAULT_CONFIG_WPA_PASSPHRASE          "12345678"

static void append_wpas_network_default_config(struct packet_wrapper *wrapper)
{
	if (find_wrapper_tlv_by_id(wrapper, TLV_SSID) == NULL) {
		add_wrapper_tlv(wrapper, TLV_SSID,
				strlen(WPAS_DEFAULT_CONFIG_SSID),
				WPAS_DEFAULT_CONFIG_SSID);
	}
	if (find_wrapper_tlv_by_id(wrapper, TLV_WPA_KEY_MGMT) == NULL) {
		add_wrapper_tlv(wrapper, TLV_WPA_KEY_MGMT,
				strlen(WPAS_DEFAULT_CONFIG_WPA_KEY_MGMT),
				WPAS_DEFAULT_CONFIG_WPA_KEY_MGMT);
	}
	if (find_wrapper_tlv_by_id(wrapper, TLV_PROTO) == NULL) {
		add_wrapper_tlv(wrapper, TLV_PROTO,
				strlen(WPAS_DEFAULT_CONFIG_PROTO),
				WPAS_DEFAULT_CONFIG_PROTO);
	}
	if (find_wrapper_tlv_by_id(wrapper, TLV_RSN_PAIRWISE) == NULL) {
		add_wrapper_tlv(wrapper, TLV_RSN_PAIRWISE,
				strlen(HOSTAPD_DEFAULT_CONFIG_RSN_PAIRWISE),
				HOSTAPD_DEFAULT_CONFIG_RSN_PAIRWISE);
	}
	if (find_wrapper_tlv_by_id(wrapper, TLV_WPA_PASSPHRASE) == NULL) {
		add_wrapper_tlv(wrapper, TLV_WPA_PASSPHRASE,
				strlen(WPAS_DEFAULT_CONFIG_WPA_PASSPHRASE),
				WPAS_DEFAULT_CONFIG_WPA_PASSPHRASE);
	}
}
#endif /* _RESERVED_ */

static int generate_wpas_config(char *buffer, int buffer_size, struct packet_wrapper *wrapper)
{
	size_t i;
	char value[S_BUFFER_LEN], cfg_item[2*S_BUFFER_LEN], buf[S_BUFFER_LEN];
	int ieee80211w_configured = 0;
	int transition_mode_enabled = 0;
	int owe_configured = 0;
	int sae_only = 0;
	struct tlv_to_config_name* cfg = NULL;

	(void) buffer_size;

	sprintf(buffer, "ctrl_interface=%s\nap_scan=1\npmf=1\n", WPAS_CTRL_PATH_DEFAULT);

	for (i = 0; i < wrapper->tlv_num; i++) {
		cfg = find_wpas_global_config_name(wrapper->tlv[i]->id);
		if (cfg) {
			memset(value, 0, sizeof(value));
			memcpy(value, wrapper->tlv[i]->value, wrapper->tlv[i]->len);
			sprintf(cfg_item, "%s=%s\n", cfg->config_name, value);
			strcat(buffer, cfg_item);
		}
	}

	strcat(buffer, "network={\n");

#ifdef _RESERVED_
	/* The function is reserved for the defeault wpas config */
	append_wpas_network_default_config(wrapper);
#endif /* _RESERVED_ */

	for (i = 0; i < wrapper->tlv_num; i++) {
		cfg = find_tlv_config(wrapper->tlv[i]->id);
		if (cfg && find_wpas_global_config_name(wrapper->tlv[i]->id) == NULL) {
			memset(value, 0, sizeof(value));
			memcpy(value, wrapper->tlv[i]->value, wrapper->tlv[i]->len);

			if ((wrapper->tlv[i]->id == TLV_IEEE80211_W) ||
			    (wrapper->tlv[i]->id == TLV_STA_IEEE80211_W)) {
				ieee80211w_configured = 1;
			} else if (wrapper->tlv[i]->id == TLV_KEY_MGMT) {
				if (strstr(value, "WPA-PSK") && strstr(value, "SAE")) {
					transition_mode_enabled = 1;
				}
				if (!strstr(value, "WPA-PSK") && strstr(value, "SAE")) {
					sae_only = 1;
				}

				if (strstr(value, "OWE")) {
					owe_configured = 1;
				}
			} else if ((wrapper->tlv[i]->id == TLV_CA_CERT) &&
				    strcmp("DEFAULT", value) == 0) {
				sprintf(value, "/etc/ssl/certs/ca-certificates.crt");
			} else if ((wrapper->tlv[i]->id == TLV_PAC_FILE)) {
				memset(pac_file_path, 0, sizeof(pac_file_path));
				snprintf(pac_file_path, sizeof(pac_file_path), "%s", value);
			} else if (wrapper->tlv[i]->id == TLV_SERVER_CERT) {
				memset(buf, 0, sizeof(buf));
				get_server_cert_hash(value, buf);
				memcpy(value, buf, sizeof(buf));
			}

			if (cfg->quoted) {
				sprintf(cfg_item, "%s=\"%s\"\n", cfg->config_name, value);
				strcat(buffer, cfg_item);
			} else {
				sprintf(cfg_item, "%s=%s\n", cfg->config_name, value);
				strcat(buffer, cfg_item);
			}
		}
	}

	if (ieee80211w_configured == 0) {
		if (transition_mode_enabled) {
			strcat(buffer, "ieee80211w=1\n");
		} else if (sae_only) {
			strcat(buffer, "ieee80211w=2\n");
		} else if (owe_configured) {
			strcat(buffer, "ieee80211w=2\n");
		}
	}

	/* TODO: merge another file */
	/* python source code:
	 * if merge_config_file:
	 * appended_supplicant_conf_str = ""
	 * existing_conf = StaCommandHelper.get_existing_supplicant_conf()
	 * wpa_supplicant_dict = StaCommandHelper.__convert_config_str_to_dict(config = wps_config)
	 * for each_key in existing_conf:
	 * 	if each_key not in wpa_supplicant_dict:
	 * 		wpa_supplicant_dict[each_key] = existing_conf[each_key]
	 * for each_supplicant_conf in wpa_supplicant_dict:
	 * 	appended_supplicant_conf_str +=
	 * 	each_supplicant_conf + "=" + wpa_supplicant_dict[each_supplicant_conf] + "\n"
	 * 	wps_config = appended_supplicant_conf_str.rstrip()
	 */

	strcat(buffer, "}\n");

	return strlen(buffer);
}

static int configure_sta_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int len;
	char buffer[L_BUFFER_LEN];
	char *message = "DUT configured as STA : Configuration file created";

	memset(buffer, 0, sizeof(buffer));
	len = generate_wpas_config(buffer, sizeof(buffer), req);
	if (len) {
		sta_configured = 1;
		write_file(get_wpas_conf_file(), buffer, len);
	}

	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS,
			      len > 0 ? TLV_VALUE_STATUS_OK : TLV_VALUE_STATUS_NOT_OK);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}

static int associate_sta_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	/* TODO: Implement this for zephyr */

	return 0;
}

static int send_sta_disconnect_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char *message = TLV_VALUE_WPA_S_DISCONNECT_NOT_OK;
	char buffer[256], response[1024];
	int status = TLV_VALUE_STATUS_NOT_OK;
	size_t resp_len;
	struct wpa_supplicant *wpa_s = NULL;

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	/* Open WPA supplicant UDS socket */
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_DISCONNECT_NOT_OK;
		goto done;
	}
	/* Send command to hostapd UDS socket */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "DISCONNECT");
	memset(response, 0, sizeof(response));
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_WPA_S_DISCONNECT_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}

static int send_sta_reconnect_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char *message = TLV_VALUE_WPA_S_RECONNECT_NOT_OK;
	char buffer[256], response[1024];
	int status = TLV_VALUE_STATUS_NOT_OK;
	size_t resp_len;
	struct wpa_supplicant *wpa_s = NULL;

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	/* Open WPA supplicant UDS socket */
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_RECONNECT_NOT_OK;
		goto done;
	}
	/* Send command to hostapd UDS socket */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "RECONNECT");
	memset(response, 0, sizeof(response));
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_WPA_S_RECONNECT_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}

static int set_sta_parameter_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	size_t resp_len, i;
	char *message = NULL;
	char buffer[BUFFER_LEN];
	char response[BUFFER_LEN];
	char param_name[32];
	char param_value[256];
	struct tlv_hdr *tlv = NULL;
	struct wpa_ctrl *w = NULL;
	struct wpa_supplicant *wpa_s = NULL;

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}

	/* Open wpa_supplicant UDS socket */
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}

	for (i = 0; i < req->tlv_num; i++) {
		memset(param_name, 0, sizeof(param_name));
		memset(param_value, 0, sizeof(param_value));
		tlv = req->tlv[i];
		strcpy(param_name, find_tlv_config_name(tlv->id));
		memcpy(param_value, tlv->value, tlv->len);

		/* Assemble wpa_supplicant command */
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "SET %s %s", param_name, param_value);
		/* Send command to wpa_supplicant UDS socket */
		resp_len = sizeof(response) - 1;
		wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
		/* Check response */
		if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
			indigo_logger(LOG_LEVEL_ERROR,
				      "Failed to execute the command. Response: %s", response);
			message = TLV_VALUE_WPA_SET_PARAMETER_NO_OK;
			goto done;
		}
	}

	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;
done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}

#ifdef CONFIG_WNM
static int send_sta_btm_query_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	size_t resp_len;
	char *message = TLV_VALUE_WPA_S_BTM_QUERY_NOT_OK;
	char buffer[1024];
	char response[1024];
	char reason_code[256];
	char candidate_list[256];
	struct tlv_hdr *tlv = NULL;
	struct wpa_ctrl *w = NULL;
	struct wpa_supplicant *wpa_s = NULL;

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	/* Open wpa_supplicant UDS socket */
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}
	/* TLV: BTMQUERY_REASON_CODE */
	tlv = find_wrapper_tlv_by_id(req, TLV_BTMQUERY_REASON_CODE);
	if (tlv) {
		memcpy(reason_code, tlv->value, tlv->len);
	} else {
		goto done;
	}

	/* TLV: TLV_CANDIDATE_LIST */
	tlv = find_wrapper_tlv_by_id(req, TLV_CANDIDATE_LIST);
	if (tlv) {
		memcpy(candidate_list, tlv->value, tlv->len);
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "WNM_BSS_QUERY %s", reason_code);
	if (strcmp(candidate_list, "1") == 0) {
		strcat(buffer, " list");
	}

	/* Send command to wpa_supplicant UDS socket */
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}
#endif /* End Of CONFIG_WNM */

#ifdef CONFIG_P2P
static int start_up_p2p_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	char *message = TLV_VALUE_WPA_S_START_UP_NOT_OK;
	char buffer[S_BUFFER_LEN];
	int len, status = TLV_VALUE_STATUS_NOT_OK;

	/* TODO: Add functionality to stop Supplicant */

	/* Generate P2P config file */
	sprintf(buffer, "ctrl_interface=%s\n", WPAS_CTRL_PATH_DEFAULT);
	/* Add Device name and Device type */
	strcat(buffer, "device_name=WFA P2P Device\n");
	strcat(buffer, "device_type=1-0050F204-1\n");
	/* Add config methods */
	strcat(buffer, "config_methods=keypad display push_button\n");
	len = strlen(buffer);

	if (len) {
		write_file(get_wpas_conf_file(), buffer, len);
	}

	/* Start WPA supplicant */
	/* TODO: Add functionality to start Supplicant */

	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_WPA_S_START_UP_OK;

	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}

static int p2p_find_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char buffer[S_BUFFER_LEN], response[BUFFER_LEN];
	size_t resp_len;
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_P2P_FIND_NOT_OK;
	struct wpa_supplicant *wpa_s = NULL;

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n", __func__,
			      WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	/* Open wpa_supplicant UDS socket */
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}
	/* P2P_FIND */
	memset(buffer, 0, sizeof(buffer));
	memset(response, 0, sizeof(response));
	sprintf(buffer, "P2P_FIND");
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}

static int p2p_listen_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char buffer[S_BUFFER_LEN], response[BUFFER_LEN];
	size_t resp_len;
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_P2P_LISTEN_NOT_OK;
	struct wpa_supplicant *wpa_s = NULL;

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	/* Open wpa_supplicant UDS socket */
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}
	/* P2P_LISTEN */
	memset(buffer, 0, sizeof(buffer));
	memset(response, 0, sizeof(response));
	sprintf(buffer, "P2P_LISTEN");
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}

static int add_p2p_group_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char buffer[S_BUFFER_LEN], response[BUFFER_LEN];
	char freq[64], he[16];
	size_t resp_len;
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_P2P_ADD_GROUP_NOT_OK;
	struct tlv_hdr *tlv = NULL;
	struct wpa_supplicant *wpa_s = NULL;

	memset(freq, 0, sizeof(freq));
	/* TLV_FREQUENCY (required) */
	tlv = find_wrapper_tlv_by_id(req, TLV_FREQUENCY);
	if (tlv) {
		memcpy(freq, tlv->value, tlv->len);
	} else {
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_INSUFFICIENT_TLV;
		indigo_logger(LOG_LEVEL_ERROR, "Missed TLV: TLV_FREQUENCY");
		goto done;
	}

	memset(he, 0, sizeof(he));
	tlv = find_wrapper_tlv_by_id(req, TLV_IEEE80211_AX);
	if (tlv) {
		snprintf(he, sizeof(he), " he");
	}

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n", __func__,
			      WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	/* Open wpa_supplicant UDS socket */
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}

	memset(buffer, 0, sizeof(buffer));
	memset(response, 0, sizeof(response));
	sprintf(buffer, "P2P_GROUP_ADD freq=%s%s", freq, he);
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}

static int stop_p2p_group_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char buffer[S_BUFFER_LEN], response[BUFFER_LEN];
	int persist = 0;
	size_t resp_len;
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_P2P_ADD_GROUP_NOT_OK;
	struct tlv_hdr *tlv = NULL;
	char if_name[32], p2p_dev_if[32];
	struct wpa_supplicant *wpa_s = NULL;

	tlv = find_wrapper_tlv_by_id(req, TLV_PERSISTENT);
	if (tlv) {
		persist = 1;
	}

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	/* Open wpa_supplicant UDS socket */
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}

	if (get_p2p_group_if(if_name, sizeof(if_name)) != 0) {
		message = "Failed to get P2P Group Interface";
		goto done;
	}
	memset(buffer, 0, sizeof(buffer));
	memset(response, 0, sizeof(response));
	sprintf(buffer, "P2P_GROUP_REMOVE %s", if_name);
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}

	if (persist == 1) {
		/* Can use global ctrl if global ctrl is initialized */
		get_p2p_dev_if(p2p_dev_if, sizeof(p2p_dev_if));
		indigo_logger(LOG_LEVEL_DEBUG, "P2P Dev IF: %s", p2p_dev_if);
		/* Open wpa_supplicant UDS socket */
		w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
		if (!w) {
			indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
			status = TLV_VALUE_STATUS_NOT_OK;
			message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
			goto done;
		}

		/* Clear the persistent group with id 0 */
		memset(buffer, 0, sizeof(buffer));
		memset(response, 0, sizeof(response));
		sprintf(buffer, "REMOVE_NETWORK 0");
		resp_len = sizeof(response) - 1;
		wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
		/* Check response */
		if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
			indigo_logger(LOG_LEVEL_ERROR,
				      "Failed to execute the command. Response: %s", response);
			goto done;
		}
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}

static int p2p_start_wps_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char buffer[S_BUFFER_LEN], response[BUFFER_LEN];
	char pin_code[64], if_name[32];
	size_t resp_len;
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_P2P_START_WPS_NOT_OK;
	struct tlv_hdr *tlv = NULL;
	struct wpa_supplicant *wpa_s = NULL;

	memset(buffer, 0, sizeof(buffer));
	tlv = find_wrapper_tlv_by_id(req, TLV_PIN_CODE);
	if (tlv) {
		memset(pin_code, 0, sizeof(pin_code));
		memcpy(pin_code, tlv->value, tlv->len);
		sprintf(buffer, "WPS_PIN any %s", pin_code);
	} else {
		sprintf(buffer, "WPS_PBC");
	}

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	/* Open wpa_supplicant UDS socket */
	if (get_p2p_group_if(if_name, sizeof(if_name))) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to get P2P group interface");
		goto done;
	}
	indigo_logger(LOG_LEVEL_DEBUG, "P2P group interface: %s", if_name);
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}

	memset(response, 0, sizeof(response));
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	if (strncmp(response, WPA_CTRL_FAIL, strlen(WPA_CTRL_FAIL)) == 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command(%s).", buffer);
		goto done;
	}

	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}

static int get_p2p_intent_value_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_OK;
	char *message = TLV_VALUE_OK;
	char response[S_BUFFER_LEN];

	memset(response, 0, sizeof(response));
	snprintf(response, sizeof(response), "%d", P2P_GO_INTENT);


done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	if (status == TLV_VALUE_STATUS_OK) {
		fill_wrapper_tlv_bytes(resp, TLV_P2P_INTENT_VALUE, strlen(response), response);
	}
	return 0;
}

static int p2p_invite_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char buffer[S_BUFFER_LEN], response[BUFFER_LEN];
	char addr[32], if_name[16], persist[32], p2p_dev_if[32];
	char freq[16], he[16];
	size_t resp_len;
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_P2P_INVITE_NOT_OK;
	struct tlv_hdr *tlv = NULL;
	struct wpa_supplicant *wpa_s = NULL;

	memset(addr, 0, sizeof(addr));
	/* TLV_ADDRESS (required) */
	tlv = find_wrapper_tlv_by_id(req, TLV_ADDRESS);
	if (tlv) {
		memcpy(addr, tlv->value, tlv->len);
	} else {
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_INSUFFICIENT_TLV;
		indigo_logger(LOG_LEVEL_ERROR, "Missed TLV: TLV_ADDRESS");
		goto done;
	}

	memset(persist, 0, sizeof(persist));
	tlv = find_wrapper_tlv_by_id(req, TLV_PERSISTENT);
	if (tlv) {
		/* Assume persistent group id is 0 */
		snprintf(persist, sizeof(persist), "persistent=0");
	} else if (get_p2p_group_if(if_name, sizeof(if_name)) != 0) {
		message = "Failed to get P2P Group Interface";
		goto done;
	}

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	/* Can use global ctrl if global ctrl is initialized */
	get_p2p_dev_if(p2p_dev_if, sizeof(p2p_dev_if));
	indigo_logger(LOG_LEVEL_DEBUG, "P2P Dev IF: %s", p2p_dev_if);
	/* Open wpa_supplicant UDS socket */
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}

	memset(buffer, 0, sizeof(buffer));
	memset(response, 0, sizeof(response));
	if (persist[0] != 0) {
		memset(he, 0, sizeof(he));
		tlv = find_wrapper_tlv_by_id(req, TLV_IEEE80211_AX);
		if (tlv) {
			snprintf(he, sizeof(he), " he");
		}

		tlv = find_wrapper_tlv_by_id(req, TLV_FREQUENCY);
		if (tlv) {
			memset(freq, 0, sizeof(freq));
			memcpy(freq, tlv->value, tlv->len);
			sprintf(buffer, "P2P_INVITE %s peer=%s%s freq=%s", persist, addr, he, freq);
		} else {
			sprintf(buffer, "P2P_INVITE %s peer=%s%s", persist, addr, he);
		}
	} else {
		sprintf(buffer, "P2P_INVITE group=%s peer=%s", if_name, addr);
	}
	indigo_logger(LOG_LEVEL_DEBUG, "Command: %s", buffer);
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}

static int p2p_connect_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char buffer[S_BUFFER_LEN], response[BUFFER_LEN];
	char pin_code[64], if_name[32];
	char method[16], mac[32], type[16];
	size_t resp_len;
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_P2P_CONNECT_NOT_OK;
	struct tlv_hdr *tlv = NULL;
	char go_intent[32], he[16], persist[32];
	int intent_value = P2P_GO_INTENT;
	struct wpa_supplicant *wpa_s = NULL;

	memset(buffer, 0, sizeof(buffer));
	memset(mac, 0, sizeof(mac));
	memset(method, 0, sizeof(method));
	memset(type, 0, sizeof(type));
	memset(he, 0, sizeof(he));
	memset(persist, 0, sizeof(persist));
	tlv = find_wrapper_tlv_by_id(req, TLV_ADDRESS);
	if (tlv) {
		memcpy(mac, tlv->value, tlv->len);
	} else {
		indigo_logger(LOG_LEVEL_ERROR, "Missed TLV: TLV_ADDRESS");
		goto done;
	}
	tlv = find_wrapper_tlv_by_id(req, TLV_GO_INTENT);
	if (tlv) {
		memset(go_intent, 0, sizeof(go_intent));
		memcpy(go_intent, tlv->value, tlv->len);
		intent_value = atoi(go_intent);
	}
	tlv = find_wrapper_tlv_by_id(req, TLV_P2P_CONN_TYPE);
	if (tlv) {
		memcpy(type, tlv->value, tlv->len);
		if (atoi(type) == P2P_CONN_TYPE_JOIN) {
			snprintf(type, sizeof(type), " join");
			memset(go_intent, 0, sizeof(go_intent));
		} else if (atoi(type) == P2P_CONN_TYPE_AUTH) {
			snprintf(type, sizeof(type), " auth");
			snprintf(go_intent, sizeof(go_intent), " go_intent=%d", intent_value);
		}
	} else {
		snprintf(go_intent, sizeof(go_intent), " go_intent=%d", intent_value);
	}
	tlv = find_wrapper_tlv_by_id(req, TLV_IEEE80211_AX);
	if (tlv) {
		snprintf(he, sizeof(he), " he");
	}
	tlv = find_wrapper_tlv_by_id(req, TLV_PERSISTENT);
	if (tlv) {
		snprintf(persist, sizeof(persist), " persistent");
	}
	tlv = find_wrapper_tlv_by_id(req, TLV_PIN_CODE);
	if (tlv) {
		memset(pin_code, 0, sizeof(pin_code));
		memcpy(pin_code, tlv->value, tlv->len);
		tlv = find_wrapper_tlv_by_id(req, TLV_PIN_METHOD);
		if (tlv) {
			memcpy(method, tlv->value, tlv->len);
		} else {
			indigo_logger(LOG_LEVEL_ERROR, "Missed TLV PIN_METHOD???");
		}
		sprintf(buffer, "P2P_CONNECT %s %s %s%s%s%s%s",
			mac, pin_code, method, type, go_intent, he, persist);
	} else {
		tlv = find_wrapper_tlv_by_id(req, TLV_WSC_METHOD);
		if (tlv) {
			memcpy(method, tlv->value, tlv->len);
		} else {
			indigo_logger(LOG_LEVEL_ERROR, "Missed TLV WSC_METHOD");
		}
		sprintf(buffer, "P2P_CONNECT %s %s%s%s%s%s",
			mac, method, type, go_intent, he, persist);
	}
	indigo_logger(LOG_LEVEL_DEBUG, "Command: %s", buffer);

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	/* Open wpa_supplicant UDS socket */
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}

	memset(response, 0, sizeof(response));
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;
done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}
#endif /* End Of CONFIG_P2P */

static int sta_scan_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_WPA_S_SCAN_NOT_OK;
	char buffer[1024];
	char response[1024];
	struct wpa_ctrl *w = NULL;
	size_t resp_len;
	struct wpa_supplicant *wpa_s = NULL;

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	/* Open wpa_supplicant UDS socket */
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}
	/* SCAN */
	memset(buffer, 0, sizeof(buffer));
	memset(response, 0, sizeof(response));
	sprintf(buffer, "SCAN");
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}
	indigo_logger(LOG_LEVEL_DEBUG, "%s -> resp: %s\n", buffer, response);
	k_sleep(K_SECONDS(10));

	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}

#ifdef CONFIG_HS20
static int send_sta_anqp_query_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int len, status = TLV_VALUE_STATUS_NOT_OK, i;
	char *message = TLV_VALUE_WPA_S_BTM_QUERY_NOT_OK;
	char buffer[1024];
	char response[1024];
	char bssid[256];
	char anqp_info_id[256];
	struct tlv_hdr *tlv = NULL;
	struct wpa_ctrl *w = NULL;
	size_t resp_len;
	char *token = NULL;
	char *delimit = ";";
	char realm[S_BUFFER_LEN];
	struct wpa_supplicant *wpa_s = NULL;

	/* It may need to check whether to just scan */
	memset(buffer, 0, sizeof(buffer));
	len = sprintf(buffer, "ctrl_interface=%s\nap_scan=1\n", WPAS_CTRL_PATH_DEFAULT);
	if (len) {
		write_file(get_wpas_conf_file(), buffer, len);
	}

	/* TODO: Add functionality to start supplicant */

	/* Open wpa_supplicant UDS socket */
	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}
	/* SCAN */
	memset(buffer, 0, sizeof(buffer));
	memset(response, 0, sizeof(response));
	sprintf(buffer, "SCAN");
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}
	k_sleep(K_SECONDS(10));

	/* TLV: BSSID */
	tlv = find_wrapper_tlv_by_id(req, TLV_BSSID);
	if (tlv) {
		memset(bssid, 0, sizeof(bssid));
		memcpy(bssid, tlv->value, tlv->len);
	} else {
		goto done;
	}

	/* TLV: ANQP_INFO_ID */
	tlv = find_wrapper_tlv_by_id(req, TLV_ANQP_INFO_ID);
	if (tlv) {
		memset(anqp_info_id, 0, sizeof(anqp_info_id));
		memcpy(anqp_info_id, tlv->value, tlv->len);
	}

	if (strcmp(anqp_info_id, "NAIHomeRealm") == 0) {
		/* TLV: REALM */
		memset(realm, 0, sizeof(realm));
		tlv = find_wrapper_tlv_by_id(req, TLV_REALM);
		if (tlv) {
			memcpy(realm, tlv->value, tlv->len);
			sprintf(buffer, "HS20_GET_NAI_HOME_REALM_LIST %s realm=%s", bssid, realm);
		} else {
			goto done;
		}
	} else {
		token = strtok(anqp_info_id, delimit);
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "ANQP_GET %s ", bssid);
		while (token != NULL) {
			for (i = 0;
			     i < sizeof(anqp_maps)/sizeof(struct anqp_tlv_to_config_name), i++) {
				if (strcmp(token, anqp_maps[i].element) == 0) {
					strcat(buffer, anqp_maps[i].config);
				}
			}

			token = strtok(NULL, delimit);
			if (token != NULL) {
				strcat(buffer, ",");
			}
		}
	}

	/* Send command to wpa_supplicant UDS socket */
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);

	indigo_logger(LOG_LEVEL_DEBUG, "%s -> resp: %s\n", buffer, response);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}

static int set_sta_hs2_associate_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	size_t resp_len;
	char *message = TLV_VALUE_WPA_S_ASSOC_NOT_OK;
	char buffer[BUFFER_LEN];
	char response[BUFFER_LEN];
	char bssid[256];
	struct tlv_hdr *tlv = NULL;
	struct wpa_ctrl *w = NULL;
	struct wpa_supplicant *wpa_s = NULL;

	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	/* Open wpa_supplicant UDS socket */
	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}

	memset(bssid, 0, sizeof(bssid));
	tlv = find_wrapper_tlv_by_id(req, TLV_BSSID);
	if (tlv) {
		memset(bssid, 0, sizeof(bssid));
		memcpy(bssid, tlv->value, tlv->len);
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "INTERWORKING_CONNECT %s", bssid);
	} else {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "INTERWORKING_SELECT auto");
	}

	/* Send command to wpa_supplicant UDS socket */
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command %s.\n Response: %s",
			      buffer, response);
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;
done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}

static int sta_add_credential_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	char *message = TLV_VALUE_WPA_S_ADD_CRED_NOT_OK;
	char buffer[BUFFER_LEN];
	int len, status = TLV_VALUE_STATUS_NOT_OK, cred_id, wpa_ret;
	size_t resp_len, i;
	char response[BUFFER_LEN];
	char param_value[256];
	struct tlv_hdr *tlv = NULL;
	struct wpa_ctrl *w = NULL;
	struct tlv_to_config_name *cfg = NULL;
	struct wpa_supplicant *wpa_s = NULL;

	if (sta_configured == 0) {
		sta_configured = 1;
		/* TODO: Add functionality to stop Supplicant */
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "ctrl_interface=%s\nap_scan=1\n", WPAS_CTRL_PATH_DEFAULT);
		len = strlen(buffer);
		if (len) {
			write_file(get_wpas_conf_file(), buffer, len);
		}
	}
	if (sta_started == 0) {
		sta_started = 1;
		/* Start WPA supplicant */
		/* TODO: Add functionality to start Supplicant */
	}

	/* Open wpa_supplicant UDS socket */
	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}
	/* Assemble wpa_supplicant command */
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "ADD_CRED");
	resp_len = sizeof(response) - 1;
	wpa_ret = wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	if (wpa_ret < 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command ADD_CRED. Response: %s", response);
		goto done;
	}
	cred_id = atoi(response);

	for (i = 0; i < req->tlv_num; i++) {
		memset(param_value, 0, sizeof(param_value));
		tlv = req->tlv[i];
		cfg = find_tlv_config(tlv->id);
		if (!cfg) {
			continue;
		}
		memcpy(param_value, tlv->value, tlv->len);

		/* Assemble wpa_supplicant command */
		memset(buffer, 0, sizeof(buffer));

		if (cfg->quoted) {
			snprintf(buffer, sizeof(buffer),
				 "SET_CRED %d %s \"%s\"", cred_id, cfg->config_name, param_value);
		} else {
			snprintf(buffer, sizeof(buffer),
				 "SET_CRED %d %s %s", cred_id, cfg->config_name, param_value);
		}
		indigo_logger(LOG_LEVEL_DEBUG, "Execute the command: %s", buffer);
		/* Send command to wpa_supplicant UDS socket */
		resp_len = sizeof(response) - 1;
		wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
		/* Check response */
		if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
			indigo_logger(LOG_LEVEL_ERROR,
				      "Failed to execute the command. Response: %s", response);
			message = TLV_VALUE_WPA_SET_PARAMETER_NO_OK;
			goto done;
		}
	}

	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_WPA_S_ADD_CRED_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}

static int set_sta_install_ppsmo_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{

	/* TODO: Implement the functionality for zephyr */

	return 0;
}
#endif /* End Of CONFIG_HS20 */


static int start_dhcp_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_START_DHCP_NOT_OK;
	char buffer[S_BUFFER_LEN];
	char ip_addr[32], role[8];
	struct tlv_hdr *tlv = NULL;
	char if_name[32];

	memset(role, 0, sizeof(role));
	tlv = find_wrapper_tlv_by_id(req, TLV_ROLE);
	if (tlv) {
		memcpy(role, tlv->value, tlv->len);
		if (atoi(role) == DUT_TYPE_P2PUT) {
#ifdef CONFIG_P2P
			get_p2p_group_if(if_name, sizeof(if_name));
#endif /* End Of CONFIG_P2P */
		} else {
			indigo_logger(LOG_LEVEL_ERROR, "DHCP only supports in P2PUT");
			goto done;
		}
	} else {
		indigo_logger(LOG_LEVEL_ERROR, "Missed TLV: TLV_ROLE");
		goto done;
	}

	/* TLV: TLV_STATIC_IP */
	memset(ip_addr, 0, sizeof(ip_addr));
	tlv = find_wrapper_tlv_by_id(req, TLV_STATIC_IP);
	if (tlv) { /* DHCP Server */
		memcpy(ip_addr, tlv->value, tlv->len);
		if (!strcmp("0.0.0.0", ip_addr)) {
			snprintf(ip_addr, sizeof(ip_addr), DHCP_SERVER_IP);
		}
		snprintf(buffer, sizeof(buffer), "%s/24", ip_addr);
		set_interface_ip(if_name, buffer);
		start_dhcp_server(if_name, ip_addr);
	} else { /* DHCP Client */
		start_dhcp_client(if_name);
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}

static int stop_dhcp_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_NOT_OK;
	char role[8];
	struct tlv_hdr *tlv = NULL;
#ifdef CONFIG_P2P
	char if_name[32];
#endif /* End Of CONFIG_P2P */

	memset(role, 0, sizeof(role));
	tlv = find_wrapper_tlv_by_id(req, TLV_ROLE);
	if (tlv) {
		memcpy(role, tlv->value, tlv->len);
		if (atoi(role) == DUT_TYPE_P2PUT) {
#ifdef CONFIG_P2P
			if (!get_p2p_group_if(if_name, sizeof(if_name))) {
				reset_interface_ip(if_name);
			}
#endif /* End Of CONFIG_P2P */
		} else {
			indigo_logger(LOG_LEVEL_ERROR, "DHCP only supports in P2PUT");
			goto done;
		}
	} else {
		indigo_logger(LOG_LEVEL_ERROR, "Missed TLV: TLV_ROLE");
		goto done;
	}

	/* TLV: TLV_STATIC_IP */
	tlv = find_wrapper_tlv_by_id(req, TLV_STATIC_IP);
	if (tlv) { /* DHCP Server */
		stop_dhcp_server();
	} else { /* DHCP Client */
		stop_dhcp_client();
	}

	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);

	return 0;
}

#ifdef CONFIG_WPS
static int get_wsc_pin_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_NOT_OK;
	char buffer[64], response[S_BUFFER_LEN];
	struct tlv_hdr *tlv = NULL;
	char value[16];
	int role = 0;
	struct wpa_ctrl *w = NULL;
	size_t resp_len;

	memset(value, 0, sizeof(value));
	tlv = find_wrapper_tlv_by_id(req, TLV_ROLE);
	if (tlv) {
		memcpy(value, tlv->value, tlv->len);
		role = atoi(value);
	} else {
		indigo_logger(LOG_LEVEL_ERROR, "Missed TLV: TLV_ROLE");
		goto done;
	}

	if (role == DUT_TYPE_APUT) {
#ifdef CONFIG_AP
		/* TODO */
		sprintf(buffer, "WPS_AP_PIN get");
		w = wpa_ctrl_open(get_hapd_ctrl_path());
		if (!w) {
			indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to hostapd");
			status = TLV_VALUE_STATUS_NOT_OK;
			message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
			goto done;
#endif /* End Of CONFIG_AP */
		}
#ifdef CONFIG_P2P
	} else if (role == DUT_TYPE_STAUT || role == DUT_TYPE_P2PUT) {
#else
	} else if (role == DUT_TYPE_STAUT) {
#endif /* End Of CONFIG_P2P */
		sprintf(buffer, "WPS_PIN get");
		w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
		if (!w) {
			indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
			status = TLV_VALUE_STATUS_NOT_OK;
			message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
			goto done;
		}
	} else {
		indigo_logger(LOG_LEVEL_ERROR, "Invalid value in TLV_ROLE");
	}

	memset(response, 0, sizeof(response));
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	if (strncmp(response, WPA_CTRL_FAIL, strlen(WPA_CTRL_FAIL)) == 0) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to execute the command(%s).", buffer);
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	if (status == TLV_VALUE_STATUS_OK) {
		fill_wrapper_tlv_bytes(resp, TLV_WSC_PIN_CODE, strlen(response), response);
	}
	if (w) {
		wpa_ctrl_close(w);
	}
	return 0;
}

#ifdef CONFIG_AP
static int start_wps_ap_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char buffer[S_BUFFER_LEN], response[BUFFER_LEN];
	char pin_code[64];
	size_t resp_len;
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_AP_START_WPS_NOT_OK;
	struct tlv_hdr *tlv = NULL;

	memset(buffer, 0, sizeof(buffer));
	tlv = find_wrapper_tlv_by_id(req, TLV_PIN_CODE);
	if (tlv) {
		memset(pin_code, 0, sizeof(pin_code));
		memcpy(pin_code, tlv->value, tlv->len);

		/* Please implement the wsc pin validation function to
		 * identify the invalid PIN code and DONOT start wps.
		 */
		#define WPS_PIN_VALIDATION_FILE "/tmp/pin_checksum.sh"
		int len = 0;
		char pipebuf[S_BUFFER_LEN];
		static const char * const parameter[] = {"sh",
							 WPS_PIN_VALIDATION_FILE,
							 pin_code, NULL};

		memset(pipebuf, 0, sizeof(pipebuf));
		if (access(WPS_PIN_VALIDATION_FILE, F_OK) == 0) {
			len = pipe_command(pipebuf, sizeof(pipebuf), "/bin/sh", parameter);
			if (len && atoi(pipebuf)) {
				indigo_logger(LOG_LEVEL_INFO, "Valid PIN Code: %s", pin_code);
			} else {
				indigo_logger(LOG_LEVEL_INFO, "Invalid PIN Code: %s", pin_code);
				message = TLV_VALUE_AP_WSC_PIN_CODE_NOT_OK;
				goto done;
			}
		}
		/*
		 * End of wsc pin validation function
		 */
		sprintf(buffer, "WPS_PIN any %s", pin_code);
	} else {
		sprintf(buffer, "WPS_PBC");
	}

	/* Open hostapd UDS socket */
	w = wpa_ctrl_open(get_hapd_ctrl_path());
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to hostapd");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}

	memset(response, 0, sizeof(response));
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	if (strncmp(response, WPA_CTRL_FAIL, strlen(WPA_CTRL_FAIL)) == 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command(%s).", buffer);
		goto done;
	}

	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	if (w) {
		wpa_ctrl_close(w);
	}
	return 0;
}
#endif /* End Of CONFIG_AP */

static int start_wps_sta_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char buffer[S_BUFFER_LEN], response[BUFFER_LEN];
	char pin_code[64];
	size_t resp_len;
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_AP_START_WPS_NOT_OK;
	struct tlv_hdr *tlv = NULL;
	int use_dynamic_pin = 0;
	struct wpa_supplicant *wpa_s = NULL;

	memset(buffer, 0, sizeof(buffer));
	tlv = find_wrapper_tlv_by_id(req, TLV_PIN_CODE);
	if (tlv) {
		memset(pin_code, 0, sizeof(pin_code));
		memcpy(pin_code, tlv->value, tlv->len);
		if (strlen(pin_code) == 1 && atoi(pin_code) == 0) {
			sprintf(buffer, "WPS_PIN any");
			use_dynamic_pin = 1;
		} else if (strlen(pin_code) == 4 || strlen(pin_code) == 8) {
			sprintf(buffer, "WPS_PIN any %s", pin_code);
		} else {
			/* Please implement the function to strip the extraneous
			 *  hyphen(dash) attached with 4 or 8-digit PIN code, then
			 *  start WPS PIN Registration with stripped PIN code.
			 */
			indigo_logger(LOG_LEVEL_ERROR, "Unrecognized PIN: %s", pin_code);
			goto done;
		}
	} else {
		sprintf(buffer, "WPS_PBC");
	}

	/* Open wpa_supplicant UDS socket */
	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}

	memset(response, 0, sizeof(response));
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	if (strncmp(response, WPA_CTRL_FAIL, strlen(WPA_CTRL_FAIL)) == 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command(%s).", buffer);
		goto done;
	}

	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	if (status == TLV_VALUE_STATUS_OK && use_dynamic_pin) {
		fill_wrapper_tlv_bytes(resp, TLV_WSC_PIN_CODE, strlen(response), response);
	}
	return 0;
}

struct _cfg_cred {
	char *key;
	char *tok;
	char val[S_BUFFER_LEN];
	unsigned short tid;
};

static int get_wsc_cred_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_NOT_OK;
	char *pos = NULL, *data = NULL, value[16];
	int i, len, count = 0, role = 0;
	struct tlv_hdr *tlv = NULL;
	struct _cfg_cred *p_cfg = NULL;

	memset(value, 0, sizeof(value));
	tlv = find_wrapper_tlv_by_id(req, TLV_ROLE);
	if (tlv) {
		memcpy(value, tlv->value, tlv->len);
		role = atoi(value);
	} else {
		indigo_logger(LOG_LEVEL_ERROR, "Missed TLV: TLV_ROLE");
		goto done;
	}

	if (role == DUT_TYPE_APUT) {
#ifdef CONFIG_AP
	    /* APUT */
		struct _cfg_cred cfg_creds[] = {
			{"ssid", "ssid=", {0}, TLV_WSC_SSID},
			{"wpa_passphrase", "wpa_passphrase=", {0}, TLV_WSC_WPA_PASSPHRASE},
			{"wpa_key_mgmt", "wpa_key_mgmt=", {0}, TLV_WSC_WPA_KEY_MGMT}
		};
		count = sizeof(cfg_creds)/sizeof(struct _cfg_cred);
		p_cfg = cfg_creds;
		tlv = find_wrapper_tlv_by_id(req, TLV_BSS_IDENTIFIER);
		struct interface_info *wlan = NULL;

		if (tlv) {
			/* mbss: TBD */
		} else {
			/* single wlan  */
			wlan = get_first_configured_wireless_interface_info();
		}
		if (!wlan) {
			goto done;
		}
		data = read_file(wlan->hapd_conf_file);
		if (!data) {
			indigo_logger(LOG_LEVEL_ERROR,
				      "Fail to read file: %s", wlan->hapd_conf_file);
			goto done;
		}
#endif /* End Of CONFIG_AP */
	} else if (role == DUT_TYPE_STAUT) {
		/* STAUT */
		struct _cfg_cred cfg_creds[] = {
			{"ssid", "ssid=", {0}, TLV_WSC_SSID},
			{"psk", "psk=", {0}, TLV_WSC_WPA_PASSPHRASE},
			{"key_mgmt", "key_mgmt=", {0}, TLV_WSC_WPA_KEY_MGMT}
		};
		count = sizeof(cfg_creds)/sizeof(struct _cfg_cred);
		p_cfg = cfg_creds;
		data = read_file(get_wpas_conf_file());
		if (!data) {
			indigo_logger(LOG_LEVEL_ERROR,
				      "Fail to read file: %s", get_wpas_conf_file());
			goto done;
		}
	} else {
		indigo_logger(LOG_LEVEL_ERROR, "Invalid value in TLV_ROLE");
		goto done;
	}

	for (i = 0; i < count; i++) {
		pos = strstr(data, p_cfg[i].tok);
		if (pos) {
			pos += strlen(p_cfg[i].tok);
			if (*pos == '"') {
				/* Handle with the format aaaaa="xxxxxxxx" */
				pos++;
				len = strchr(pos, '"') - pos;
			} else {
				/* Handle with the format bbbbb=yyyyyyyy */
				len = strchr(pos, '\n') - pos;
			}
			memcpy(p_cfg[i].val, pos, len);
			indigo_logger(LOG_LEVEL_INFO,
				      "Get %s: %s\n", p_cfg[i].key, p_cfg[i].val);
		} else {
			indigo_logger(LOG_LEVEL_INFO,
				      "Cannot find the setting: %s\n", p_cfg[i].key);
			/* goto done; */
		}
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	if (data) {
		free(data);
	}
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	if (status == TLV_VALUE_STATUS_OK) {
		for (i = 0; i < count; i++) {
			fill_wrapper_tlv_bytes(resp, p_cfg[i].tid,
					       strlen(p_cfg[i].val), p_cfg[i].val);
		}
	}
	return 0;
}

static int enable_wsc_sta_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	char *message = TLV_VALUE_WPA_S_START_UP_NOT_OK;
	char buffer[L_BUFFER_LEN];
	char value[S_BUFFER_LEN], cfg_item[2*S_BUFFER_LEN], buf[S_BUFFER_LEN];
	int i, len = 0, status = TLV_VALUE_STATUS_NOT_OK;
	struct tlv_hdr *tlv = NULL;
	struct tlv_to_config_name *cfg = NULL;

	/* TODO: Add functionality to stop Supplicant */

	/* Generate configuration */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "ctrl_interface=%s\nap_scan=1\npmf=1\n", WPAS_CTRL_PATH_DEFAULT);

	for (i = 0; i < req->tlv_num; i++) {
		cfg = find_wpas_global_config_name(req->tlv[i]->id);
		if (cfg) {
			memset(value, 0, sizeof(value));
			memcpy(value, req->tlv[i]->value, req->tlv[i]->len);
			sprintf(cfg_item, "%s=%s\n", cfg->config_name, value);
			strcat(buffer, cfg_item);
		}
	}

	/* wps settings */
	tlv = find_wrapper_tlv_by_id(req, TLV_WPS_ENABLE);
	if (tlv) {
		memset(value, 0, sizeof(value));
		memcpy(value, tlv->value, tlv->len);
		/* To get STA wps vendor info */
		wps_setting *s = get_vendor_wps_settings(WPS_STA);

		if (!s) {
			indigo_logger(LOG_LEVEL_WARNING, "Failed to get STAUT WPS settings");
		} else if (atoi(value) == WPS_ENABLE_NORMAL) {
			for (i = 0; i < STA_SETTING_NUM; i++) {
				memset(cfg_item, 0, sizeof(cfg_item));
				sprintf(cfg_item, "%s=%s\n", s[i].wkey, s[i].value);
				strcat(buffer, cfg_item);
			}
			indigo_logger(LOG_LEVEL_INFO, "STAUT Configure WPS");
		} else {
			indigo_logger(LOG_LEVEL_ERROR,
				      "Invalid WPS TLV value: %d (TLV ID 0x%04x)",
				      atoi(value), tlv->id);
		}
	} else {
		indigo_logger(LOG_LEVEL_WARNING,
			      "No WSC TLV found. Failed to append STA WSC data");
	}

	len = strlen(buffer);

	if (len) {
		write_file(get_wpas_conf_file(), buffer, len);
	}

	/* Start wpa supplicant */
	/* TODO: Add fucntionality to start Supplicant */

	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_WPA_S_START_UP_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}
#endif /* End Of CONFIG_WPS */

#ifdef CONFIG_P2P
static int set_p2p_serv_disc_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char buffer[BUFFER_LEN], response[BUFFER_LEN];
	char addr[32], p2p_dev_if[32];
	size_t resp_len;
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_P2P_SET_SERV_DISC_NOT_OK;
	struct tlv_hdr *tlv = NULL;
	struct wpa_supplicant *wpa_s = NULL;

	memset(addr, 0, sizeof(addr));
	tlv = find_wrapper_tlv_by_id(req, TLV_ADDRESS);
	if (tlv) {
		/* Send Service Discovery Req */
		memcpy(addr, tlv->value, tlv->len);
	} else {
		/* Set Services case */
		/* Add bonjour and upnp Service */
	}

	/* Can use global ctrl if global ctrl is initialized */
	get_p2p_dev_if(p2p_dev_if, sizeof(p2p_dev_if));
	indigo_logger(LOG_LEVEL_DEBUG, "P2P Dev IF: %s", p2p_dev_if);
	/* Open wpa_supplicant UDS socket */
	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}

	memset(buffer, 0, sizeof(buffer));
	memset(response, 0, sizeof(response));
	if (addr[0] != 0) {
		sprintf(buffer, "P2P_SERV_DISC_REQ %s 02000001", addr);
		indigo_logger(LOG_LEVEL_DEBUG, "Command: %s", buffer);
	} else {
		sprintf(buffer,
			"P2P_SERVICE_ADD bonjour 096d797072696e746572045f697070c00c001001 "
			"09747874766572733d311a70646c3d6170706c69636174696f6e2f706f7374736372797074");
	}
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);

	if (addr[0] == 0) {
		if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
			indigo_logger(LOG_LEVEL_ERROR,
				      "Failed to execute the command. Response: %s", response);
			goto done;
		}
		sprintf(buffer, "P2P_SERVICE_ADD upnp 10 uuid:5566d33e-9774-09ab-4822-333456785632"
			"::urn:schemas-upnp-org:service:ContentDirectory:2");
		wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
		if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
			indigo_logger(LOG_LEVEL_ERROR,
				      "Failed to execute the command. Response: %s", response);
			goto done;
		}
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}

static int set_p2p_ext_listen_handler(struct packet_wrapper *req, struct packet_wrapper *resp)
{
	struct wpa_ctrl *w = NULL;
	char buffer[S_BUFFER_LEN], response[BUFFER_LEN];
	size_t resp_len;
	int status = TLV_VALUE_STATUS_NOT_OK;
	char *message = TLV_VALUE_P2P_SET_EXT_LISTEN_NOT_OK;
	struct wpa_supplicant *wpa_s = NULL;

	/* Open wpa_supplicant UDS socket */
	wpa_s = z_wpas_get_handle_by_ifname(get_wireless_interface());
	if (!wpa_s) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "%s: Unable to get wpa_s handle for %s\n",
			      __func__, WIRELESS_INTERFACE_DEFAULT);
		goto done;
	}

	w = wpa_ctrl_open(wpa_s->ctrl_iface->sock_pair[0]);
	if (!w) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to connect to wpa_supplicant");
		status = TLV_VALUE_STATUS_NOT_OK;
		message = TLV_VALUE_WPA_S_CTRL_NOT_OK;
		goto done;
	}
	memset(buffer, 0, sizeof(buffer));
	memset(response, 0, sizeof(response));
	sprintf(buffer, "P2P_EXT_LISTEN 1000 4000");
	resp_len = sizeof(response) - 1;
	wpa_ctrl_request(w, buffer, strlen(buffer), response, &resp_len, NULL);
	/* Check response */
	if (strncmp(response, WPA_CTRL_OK, strlen(WPA_CTRL_OK)) != 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "Failed to execute the command. Response: %s", response);
		goto done;
	}
	status = TLV_VALUE_STATUS_OK;
	message = TLV_VALUE_OK;

done:
	fill_wrapper_message_hdr(resp, API_CMD_RESPONSE, req->hdr.seq);
	fill_wrapper_tlv_byte(resp, TLV_STATUS, status);
	fill_wrapper_tlv_bytes(resp, TLV_MESSAGE, strlen(message), message);
	return 0;
}
#endif /* End OF CONFIG_P2P */
