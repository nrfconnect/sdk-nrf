/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/zephyr.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi.h>
#include <wifi/conn_mgr.h>
#include <psa/storage_common.h>
#include <psa/protected_storage.h>

#define WIFI_CONN_MGR_SCAN_TIMEOUT K_SECONDS(45)
#define WIFI_CONN_MGR_CONN_MANAGEMENT_PERIOD K_SECONDS(10)
#define WIFI_CONN_MGR_SCAN_INTERVAL K_SECONDS(20)

LOG_MODULE_REGISTER(wifi_conn_mgr, CONFIG_WIFI_CONN_MGR_LOG_LEVEL);

struct connection_params {
	int8_t rssi_status;        /* 0 is unconfigured, 1 is configured, negative is RSSI value */
	struct wifi_connect_req_params params;
	uint8_t ssid[WIFI_SSID_MAX_LEN]; /* buffer to hold ssid */
	uint8_t psk[WIFI_PSK_MAX_LEN]; /* buffer to hold psk */
};

/**
 * @brief fixed size version of wifi credentials for saving in secure storage
 */
struct wifi_credentials_fixed_size
{
	uint8_t ssid[WIFI_SSID_MAX_LEN];
	uint8_t ssid_length;
	uint8_t psk[WIFI_PSK_MAX_LEN];
	uint8_t psk_length;
};

struct connection_params cnx_params[CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS] = { 0 };


#define WIFI_CONN_MGR_EVENTS (NET_EVENT_WIFI_SCAN_RESULT |		\
				NET_EVENT_WIFI_SCAN_DONE |		\
				NET_EVENT_WIFI_CONNECT_RESULT |		\
				NET_EVENT_WIFI_DISCONNECT_RESULT)

static struct net_mgmt_event_callback wifi_conn_mgr_cb;
struct k_work_delayable wifi_conn_mgr_work;
static k_tid_t waiting_connect = NULL;

enum state_type {
    DISCONNECTED,
    SCANNING,
    SCAN_DONE,
    CONNECTING,
    ASSOCIATED,
    CONNECTED,
    DISCONNECTING
} state;

void wifi_mgr_save_credentials(void) {
	if (IS_ENABLED(CONFIG_BUILD_WITH_TFM)) {
		struct wifi_credentials_fixed_size credential_storage[CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS] = {0};
		size_t credential_storage_idx = 0;
		for (size_t idx = 0; idx < CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS; ++idx) {
			if (cnx_params[idx].rssi_status != 0) {
				struct wifi_credentials_fixed_size *creds = &credential_storage[credential_storage_idx];
				creds->ssid_length = cnx_params[idx].params.ssid_length;
				creds->psk_length = cnx_params[idx].params.psk_length;
				memcpy(creds->ssid, cnx_params[idx].params.ssid, creds->ssid_length);
				memcpy(creds->psk, cnx_params[idx].params.psk, creds->psk_length);
				credential_storage_idx++;
			}
		}
		int ret = psa_ps_set(CONFIG_WIFI_CONN_MGR_PSA_UID, sizeof(credential_storage), credential_storage, PSA_STORAGE_FLAG_NONE);

		if (ret != PSA_SUCCESS) {
			LOG_ERR("wifi credentials could not be saved to secure storage");
		}
		LOG_ERR("wifi credentials saved successfully");
	}
}

void wifi_mgr_load_credentials(void) {
	struct wifi_credentials_fixed_size credential_storage[CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS] = {0};
	size_t data_length = 0;

	int ret = psa_ps_get(CONFIG_WIFI_CONN_MGR_PSA_UID, 0, sizeof(credential_storage), credential_storage, &data_length);

	if (ret != PSA_SUCCESS || data_length != sizeof(credential_storage)) {
		LOG_ERR("wifi credentials could not be loaded from secure storage");
		return;
	}

	for (size_t idx = 0; idx < CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS; ++idx) {
		struct wifi_connect_req_params new_param = {
			.ssid = credential_storage[idx].ssid,
			.ssid_length = credential_storage[idx].ssid_length,
			.psk = credential_storage[idx].psk,
			.psk_length = credential_storage[idx].psk_length,
		};
		wifi_mgr_add_credential(&new_param);
	}
}

int wifi_mgr_add_credential(struct wifi_connect_req_params *new_param)
{	
	size_t dest_idx = CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS;

	/* look if ssid is already listed */
	for (size_t idx = 0; idx < CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS; ++idx) {
		if (strcmp(cnx_params[idx].params.ssid, new_param->ssid) == 0) {
			dest_idx = idx;
			break;
		}
	}

	/* if ssid is not found, try to find empty new spot */
	if (dest_idx == CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS) {
		for (size_t idx = 0; idx < CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS; ++idx) {
			if (cnx_params[idx].rssi_status == 0) {
				dest_idx = idx;
				break;
			}
		}
	}
	if (dest_idx < CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS) {
		memcpy(&cnx_params[dest_idx].params, new_param, sizeof(struct wifi_connect_req_params));
		cnx_params[dest_idx].rssi_status = 1;
		/* copy strings to internal buffers */
		strncpy(cnx_params[dest_idx].ssid, new_param->ssid, WIFI_SSID_MAX_LEN);
		strncpy(cnx_params[dest_idx].psk, new_param->psk, WIFI_PSK_MAX_LEN);
		cnx_params[dest_idx].params.ssid = cnx_params[dest_idx].ssid;
		cnx_params[dest_idx].params.psk = cnx_params[dest_idx].psk;
		wifi_mgr_save_credentials();
		return 0;
	}
	LOG_WRN("No more credential storage available");
	return -ENOMEM;
}

int wifi_mgr_wait_connection(k_timeout_t wait)
{
    if (state == CONNECTED) {
        return 0;
    }
    if (waiting_connect != NULL) {
        return -EAGAIN;
    }
    waiting_connect = k_current_get();
    int err = k_sleep(wait);
    waiting_connect = NULL;
    return (err) ? 0 : -EIO; /* Return error if the timeout expired */
}

static void handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_scan_result *entry =
        (const struct wifi_scan_result *)cb->info;
    
    /* Find if the scanned network is:
        - In the list of credentials
        - The rssi_status is either 1 (not scanned yet) or scanned RSSI is higher
            than previously scanned
        If yes, then save this new RSSI to rssi_status.

        This will help later, when we're looking for the strongest signal.
    */
    for (size_t idx = 0; idx < CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS; ++idx) {
        if (cnx_params[idx].rssi_status != 0 &&
            cnx_params[idx].params.ssid_length == entry->ssid_length &&
            !strncmp(cnx_params[idx].params.ssid, entry->ssid, entry->ssid_length) &&
            entry->rssi < 0 &&
            (cnx_params[idx].rssi_status == 1 || entry->rssi > cnx_params[idx].rssi_status)) {
                cnx_params[idx].rssi_status = entry->rssi;
                LOG_DBG("SSID: %.*s RSSI: %d, Channel: %u", cnx_params[idx].params.ssid_length, 
                        cnx_params[idx].params.ssid, entry->rssi, entry->channel);
                break;
            }
    }
}


static void wifi_mgr_event_handler(struct net_mgmt_event_callback *cb,
                    uint32_t mgmt_event, struct net_if *iface)
{
    switch (mgmt_event) {
        case NET_EVENT_WIFI_CONNECT_RESULT:
        {
            const struct wifi_status *status = 
                (const struct wifi_status *) cb->info;
            if (status->status) {
                LOG_WRN("Connection request failed (%d)", status->status);
                state = DISCONNECTED;
                k_work_schedule(&wifi_conn_mgr_work, K_NO_WAIT);
            } else {
                LOG_DBG("Association successful");
                state = ASSOCIATED;
                k_work_schedule(&wifi_conn_mgr_work, K_MSEC(10));
            }
            break;
        }
        case NET_EVENT_WIFI_DISCONNECT_RESULT:
            LOG_DBG("Disconnect event");
            state = DISCONNECTED;
            k_work_schedule(&wifi_conn_mgr_work, K_NO_WAIT);
            break;
        case NET_EVENT_WIFI_SCAN_RESULT:
		    handle_wifi_scan_result(cb);
		    break;
	    case NET_EVENT_WIFI_SCAN_DONE:
		    state = SCAN_DONE;
            k_work_reschedule(&wifi_conn_mgr_work, K_NO_WAIT);
		    break;
        default:
            LOG_INF("Other network event");
            break;
    }
}

static void wifi_mgr_worker_fn(struct k_work *item)
{
    struct net_if *iface = net_if_get_default();
    switch (state) {
        case DISCONNECTED:
        {
            /* If disconnected, and there are configured credentials, start
                a network scan */
            bool start_scan = false;
            for (size_t idx = 0; idx < CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS; ++idx) {
                if (cnx_params[idx].rssi_status != 0) {
                    start_scan = true;
                    cnx_params[idx].rssi_status = 1;
                }
            }
            if (start_scan) {
                if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0)) {
                    LOG_ERR("Scan request failed");
                    k_work_schedule(&wifi_conn_mgr_work, K_SECONDS(10));
                } else {
                    state = SCANNING;
                    LOG_DBG("Scanning requested");
                    k_work_schedule(&wifi_conn_mgr_work, WIFI_CONN_MGR_SCAN_TIMEOUT);
                }
            }
            break;
        }
        case SCANNING:
        {
            /* The scanning timed out without finishing, disconnect to reset */
            if (net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0)) {
                state = DISCONNECTED;
                k_work_schedule(&wifi_conn_mgr_work, K_SECONDS(1));
            } else {
                state = DISCONNECTING;
            }
            break;
        }
        case SCAN_DONE:
        {
            /* Find the highest RSSI network to connect to */
            size_t index = CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS;
            int8_t max_rssi = -128;
            for (size_t idx = 0; idx < CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS; ++idx) {
                if (cnx_params[idx].rssi_status < 0 &&
                        cnx_params[idx].rssi_status > max_rssi) {
                            index = idx;
                            max_rssi = cnx_params[idx].rssi_status;
                        }
            }
            if (index < CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS) {
                if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &cnx_params[index].params,
                    sizeof(struct wifi_connect_req_params))) {
                        LOG_ERR("Connection request failed");
                        k_work_schedule(&wifi_conn_mgr_work, K_SECONDS(5));
                } else {
                    state = CONNECTING;
                    LOG_DBG("Connection requested");
                }
            } else {
                state = DISCONNECTED;
                LOG_INF("No configured networks found in scan");
                k_work_schedule(&wifi_conn_mgr_work, WIFI_CONN_MGR_SCAN_INTERVAL);
            }
            break;
        }
        case ASSOCIATED:
        {
            /* Continue to check until the security handshake is complete */
            struct wifi_iface_status status = { 0 };
            if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
                    sizeof(struct wifi_iface_status))) {
                        LOG_WRN("Status request failed");
                        k_work_schedule(&wifi_conn_mgr_work, K_SECONDS(1));
            } else {
                if ((enum wifi_iface_state)status.state == WIFI_STATE_COMPLETED) {
                    LOG_DBG("Connection completed");
                    state = CONNECTED;
                    if (waiting_connect != NULL) {
                        k_wakeup(waiting_connect);
                    }
                    k_work_schedule(&wifi_conn_mgr_work, WIFI_CONN_MGR_CONN_MANAGEMENT_PERIOD);
                } else {
                    k_work_schedule(&wifi_conn_mgr_work, K_MSEC(50));
                }
            }
            break;
        }
        case CONNECTED:
        {
            /* Monitor the connection periodically, in case there are 
                disconnections that do not send notifications */
            struct wifi_iface_status status = { 0 };
            if (!net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
                sizeof(struct wifi_iface_status))) {
                    if ((enum wifi_iface_state)status.state < WIFI_STATE_ASSOCIATING) {
                        if (net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0)) {
                            state = DISCONNECTED;
                            k_work_schedule(&wifi_conn_mgr_work, K_SECONDS(5));
                        } else {
                            state = DISCONNECTING;
                        }
                        break;
                    } else if ((enum wifi_iface_state)status.state < WIFI_STATE_COMPLETED) {
                        state = ASSOCIATED;
                        k_work_schedule(&wifi_conn_mgr_work, K_MSEC(100));
                        break;
                    }
            }
            k_work_schedule(&wifi_conn_mgr_work, WIFI_CONN_MGR_CONN_MANAGEMENT_PERIOD);
            break;
        }
        default:
            k_work_schedule(&wifi_conn_mgr_work, K_MINUTES(10));
            break;
    }
}

static int wifi_mgr_init(const struct device *unused)
{
    wifi_mgr_load_credentials();
    ARG_UNUSED(unused);
    net_mgmt_init_event_callback(&wifi_conn_mgr_cb,
                wifi_mgr_event_handler,
                WIFI_CONN_MGR_EVENTS);
    net_mgmt_add_event_callback(&wifi_conn_mgr_cb);

    state = DISCONNECTED;
    k_work_init_delayable(&wifi_conn_mgr_work, wifi_mgr_worker_fn);
    k_work_schedule(&wifi_conn_mgr_work, K_SECONDS(5));
    return 0;
}

SYS_INIT(wifi_mgr_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
