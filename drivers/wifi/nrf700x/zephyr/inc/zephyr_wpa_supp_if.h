/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing WPA supplicant interface specific declarations for
 * the Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __ZEPHYR_WPA_SUPP_IF_H__
#define __ZEPHYR_WPA_SUPP_IF_H__


#define RPU_RESP_EVENT_TIMEOUT  5000
#ifdef CONFIG_WPA_SUPP
#include <drivers/driver_zephyr.h>

void *wifi_nrf_wpa_supp_dev_init(void *supp_drv_if_ctx, const char *iface_name,
				 struct zep_wpa_supp_dev_callbk_fns *supp_callbk_fns);

void wifi_nrf_wpa_supp_dev_deinit(void *if_priv);

int wifi_nrf_wpa_supp_scan2(void *if_priv, struct wpa_driver_scan_params *params);

int wifi_nrf_wpa_supp_scan_abort(void *if_priv);

int wifi_nrf_wpa_supp_scan_results_get(void *if_priv);

int wifi_nrf_wpa_supp_deauthenticate(void *if_priv, const char *addr, unsigned short reason_code);

int wifi_nrf_wpa_supp_authenticate(void *if_priv, struct wpa_driver_auth_params *params,
			   struct wpa_bss *curr_bss);

int wifi_nrf_wpa_supp_associate(void *if_priv, struct wpa_driver_associate_params *params);

int wifi_nrf_wpa_set_supp_port(void *if_priv, int authorized, char *bssid);

int wifi_nrf_wpa_supp_signal_poll(void *if_priv, struct wpa_signal_info *si,
				 unsigned char *bssid);

int wifi_nrf_nl80211_send_mlme(void *if_priv, const u8 *data, size_t data_len, int noack,
			       unsigned int freq, int no_cck, int offchanok, unsigned int wait_time,
			       int cookie);

int wifi_nrf_supp_get_wiphy(void *if_priv);

int wifi_nrf_supp_register_frame(void *if_priv,
			u16 type, const u8 *match, size_t match_len,
			bool multicast);

int wifi_nrf_wpa_supp_set_key(void *if_priv,
			   const unsigned char *ifname,
			   enum wpa_alg alg,
			   const unsigned char *addr,
			   int key_idx,
			   int set_tx,
			   const unsigned char *seq,
			   size_t seq_len,
			   const unsigned char *key,
			   size_t key_len);

void wifi_nrf_wpa_supp_event_proc_scan_start(void *if_priv);

void wifi_nrf_wpa_supp_event_proc_scan_done(void *if_priv,
					struct nrf_wifi_umac_event_trigger_scan *scan_done_event,
					unsigned int event_len,
					int aborted);

void wifi_nrf_wpa_supp_event_proc_scan_res(void *if_priv,
					struct nrf_wifi_umac_event_new_scan_results *scan_res,
					unsigned int event_len,
					bool more_res);

void wifi_nrf_wpa_supp_event_proc_auth_resp(void *if_priv,
					    struct nrf_wifi_umac_event_mlme *auth_resp,
					    unsigned int event_len);

void wifi_nrf_wpa_supp_event_proc_assoc_resp(void *if_priv,
					     struct nrf_wifi_umac_event_mlme *assoc_resp,
					     unsigned int event_len);

void wifi_nrf_wpa_supp_event_proc_deauth(void *if_priv,
					 struct nrf_wifi_umac_event_mlme *deauth,
					 unsigned int event_len);

void wifi_nrf_wpa_supp_event_proc_disassoc(void *if_priv,
					   struct nrf_wifi_umac_event_mlme *disassoc,
					   unsigned int event_len);

void wifi_nrf_wpa_supp_event_proc_get_sta(void *if_priv,
					   struct nrf_wifi_umac_event_new_station *info,
					   unsigned int event_len);

void wifi_nrf_wpa_supp_event_proc_get_if(void *if_priv,
					   struct nrf_wifi_interface_info *info,
					   unsigned int event_len);

void wifi_nrf_wpa_supp_event_mgmt_tx_status(void *if_priv,
						struct nrf_wifi_umac_event_mlme *mlme_event,
						unsigned int event_len);


void wifi_nrf_wpa_supp_event_proc_unprot_mgmt(void *if_priv,
						struct nrf_wifi_umac_event_mlme *unprot_mgmt,
						unsigned int event_len);

void wifi_nrf_wpa_supp_event_get_wiphy(void *if_priv,
						struct nrf_wifi_event_get_wiphy *get_wiphy,
						unsigned int event_len);

void wifi_nrf_wpa_supp_event_mgmt_rx_callbk_fn(void *if_priv,
						struct nrf_wifi_umac_event_mlme *mgmt_rx_event,
						unsigned int event_len);

int wifi_nrf_supp_get_capa(void *if_priv, struct wpa_driver_capa *capa);

void wifi_nrf_wpa_supp_event_mac_chgd(void *if_priv);
int wifi_nrf_supp_get_conn_info(void *if_priv, struct wpa_conn_info *info);

void wifi_nrf_supp_event_proc_get_conn_info(void *os_vif_ctx,
					    struct nrf_wifi_umac_event_conn_info *info,
					    unsigned int event_len);

#endif /* CONFIG_WPA_SUPP */
#endif /*  __ZEPHYR_WPA_SUPP_IF_H__ */
