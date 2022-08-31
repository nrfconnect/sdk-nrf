/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_wifi_mgmt, CONFIG_NET_L2_NRF_WIFI_MGMT_LOG_LEVEL);

#include <errno.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include "supp_api.h"

static int wifi_connect(uint32_t mgmt_request, struct net_if *iface,
			void *data, size_t len)
{
	struct wifi_connect_req_params *params =
		(struct wifi_connect_req_params *)data;
	const struct device *dev = net_if_get_device(iface);
#ifndef CONFIG_WPA_SUPP
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;


	if (off_api == NULL || off_api->connect == NULL) {
		return -ENOTSUP;
	}
#endif

	LOG_HEXDUMP_DBG(params->ssid, params->ssid_length, "ssid");
	LOG_HEXDUMP_DBG(params->psk, params->psk_length, "psk");
	if (params->sae_password) {
		LOG_HEXDUMP_DBG(params->sae_password, params->sae_password_length, "sae");
	}
	NET_DBG("ch %u sec %u", params->channel, params->security);

	if ((params->security > WIFI_SECURITY_TYPE_MAX) ||
	    (params->ssid_length > WIFI_SSID_MAX_LEN) ||
	    (params->ssid_length == 0U) ||
	    ((params->security == WIFI_SECURITY_TYPE_PSK ||
		  params->security == WIFI_SECURITY_TYPE_PSK_SHA256) &&
	     ((params->psk_length < 8) || (params->psk_length > 64) ||
	      (params->psk_length == 0U) || !params->psk)) ||
	    ((params->security == WIFI_SECURITY_TYPE_SAE) &&
	      ((params->psk_length == 0U) || !params->psk) &&
		  ((params->sae_password_length == 0U) || !params->sae_password)) ||
	    ((params->channel != WIFI_CHANNEL_ANY) &&
	     (params->channel > WIFI_CHANNEL_MAX)) ||
	    !params->ssid) {
		return -EINVAL;
	}
#ifdef CONFIG_WPA_SUPP
	return zephyr_supp_connect(dev, params);
#else
	return off_api->connect(dev, params);
#endif
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT, wifi_connect);

static void scan_result_cb(struct net_if *iface, int status,
			    struct wifi_scan_result *entry)
{
	if (!iface) {
		return;
	}

	if (!entry) {
		struct wifi_status scan_status = {
			.status = status,
		};

		net_mgmt_event_notify_with_info(NET_EVENT_WIFI_SCAN_DONE,
						iface, &scan_status,
						sizeof(struct wifi_status));
		return;
	}

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_SCAN_RESULT, iface,
					entry, sizeof(struct wifi_scan_result));
}

static int wifi_scan(uint32_t mgmt_request, struct net_if *iface,
		     void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);

	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;
	if (off_api == NULL || off_api->scan == NULL) {
		return -ENOTSUP;
	}
	return off_api->scan(dev, scan_result_cb);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_SCAN, wifi_scan);


static int wifi_disconnect(uint32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);

#ifdef CONFIG_WPA_SUPP
	return zephyr_supp_disconnect(dev);
#else
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;
	if (off_api == NULL || off_api->disconnect == NULL) {
		return -ENOTSUP;
	}
	return off_api->disconnect(dev);
#endif
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_DISCONNECT, wifi_disconnect);

void wifi_mgmt_raise_connect_result_event(struct net_if *iface, int status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_CONNECT_RESULT,
					iface, &cnx_status,
					sizeof(struct wifi_status));
}

void wifi_mgmt_raise_disconnect_result_event(struct net_if *iface, int status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_DISCONNECT_RESULT,
					iface, &cnx_status,
					sizeof(struct wifi_status));
}

static int wifi_ap_enable(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	struct wifi_connect_req_params *params =
		(struct wifi_connect_req_params *)data;
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;

	if (off_api == NULL || off_api->ap_enable == NULL) {
		return -ENOTSUP;
	}

	return off_api->ap_enable(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_AP_ENABLE, wifi_ap_enable);

static int wifi_ap_disable(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;

	if (off_api == NULL || off_api->ap_enable == NULL) {
		return -ENOTSUP;
	}

	return off_api->ap_disable(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_AP_DISABLE, wifi_ap_disable);

static int wifi_iface_status(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	int ret;
	const struct device *dev = net_if_get_device(iface);
	struct wifi_iface_status status = { 0 };

	if (!data || len != sizeof(struct wifi_iface_status)) {
		return -EINVAL;
	}

#ifdef CONFIG_WPA_SUPP
	ret = zephyr_supp_status(dev, &status);
#else
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;

	if (off_api == NULL || off_api->iface_status == NULL) {
		return -ENOTSUP;
	}

	ret = off_api->iface_status(dev, &status);
#endif

	if (ret) {
		return ret;
	}

	memcpy(data, &status, len);

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_IFACE_STATUS, wifi_iface_status);

void wifi_mgmt_raise_iface_status_event(struct net_if *iface,
	struct wifi_iface_status *iface_status)
{
	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_IFACE_STATUS,
					iface, &iface_status,
					sizeof(struct wifi_iface_status));
}

#ifdef CONFIG_NET_STATISTICS_WIFI
static int wifi_iface_stats(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	int ret;
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;
	struct net_stats_wifi stats = { 0 };

	if (off_api == NULL || off_api->get_stats == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(struct net_stats_wifi)) {
		return -EINVAL;
	}

	ret = off_api->get_stats(dev, &stats);

	if (ret) {
		return ret;
	}

	memcpy(data, &stats, len);

	return 0;
}
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_WIFI, wifi_iface_stats);
#endif
