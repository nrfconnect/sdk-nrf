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

#include "wpa_supplicant_i.h"
#include "bss.h"

#ifdef CONFIG_WPA_SUPP
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
					    struct img_umac_event_trigger_scan *scan_done_event,
					    unsigned int event_len,
					    int aborted);

void wifi_nrf_wpa_supp_event_proc_scan_res(void *if_priv,
					   struct img_umac_event_new_scan_results *scan_res,
					   unsigned int event_len,
					   bool more_res);

void wifi_nrf_wpa_supp_event_proc_auth_resp(void *if_priv,
					    struct img_umac_event_mlme *auth_resp,
					    unsigned int event_len);

void wifi_nrf_wpa_supp_event_proc_assoc_resp(void *if_priv,
					     struct img_umac_event_mlme *assoc_resp,
					     unsigned int event_len);

void wifi_nrf_wpa_supp_event_proc_deauth(void *if_priv,
					 struct img_umac_event_mlme *deauth,
					 unsigned int event_len);

void wifi_nrf_wpa_supp_event_proc_disassoc(void *if_priv,
					   struct img_umac_event_mlme *disassoc,
					   unsigned int event_len);
#endif /* CONFIG_WPA_SUPP */
#endif /*  __ZEPHYR_WPA_SUPP_IF_H__ */
