/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Test-side DECT_MDM_DRIVER_MOCK HAL (linked only from DECT NR+ RPC server ztests).
 */

#include "dect_mdm_mock_test_seam.h"

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#include <net/dect/dect_net_l2.h>
#include <net/dect/dect_net_l2_mgmt.h>
#include <net/dect/dect_utils.h>

#if defined(CONFIG_NET_CONNECTION_MANAGER)
#include <net/dect/dect_net_l2_conn_mgr.h>
#endif

LOG_MODULE_REGISTER(dect_mdm_mock, CONFIG_DECT_MDM_LOG_LEVEL);

struct dect_mdm_mock_ctx {
	struct net_if *iface;
	bool activated;
	struct dect_settings settings;
};

static struct dect_mdm_mock_ctx mock_ctx;
static bool mock_iface_initialized;
static bool mock_status_override;
static struct dect_status_info mock_status_override_info;

int dect_mdm_mock_send_call_count;
int dect_mdm_mock_network_join_call_count;
int dect_mdm_mock_network_unjoin_call_count;
uint32_t dect_mdm_mock_last_send_long_rd_id;
size_t dect_mdm_mock_last_send_len;
uint8_t dect_mdm_mock_last_send_data[1280];

static void mock_default_settings(struct dect_settings *s)
{
	memset(s, 0, sizeof(*s));
	s->device_type = DECT_DEVICE_TYPE_PT;
	s->identities.network_id = 0x87654321;
	s->identities.transmitter_long_rd_id = 0x11112222;
}

static void mock_iface_init(struct net_if *iface)
{
	static uint8_t link_addr[8] = { 0x02, 0, 0, 0, 0, 0, 0, 0x01 };
	int err;

	if (mock_iface_initialized) {
		return;
	}

	mock_ctx.iface = iface;
	mock_default_settings(&mock_ctx.settings);

	if (net_if_get_link_addr(iface)->len == 0) {
		if (net_if_flag_is_set(iface, NET_IF_RUNNING)) {
			(void)net_if_down(iface);
		}
		err = net_if_set_link_addr(iface, link_addr, sizeof(link_addr), NET_LINK_UNKNOWN);
		if (err != 0) {
			LOG_ERR("set_link_addr failed: %d", err);
			return;
		}
	}

	err = net_if_set_name(iface, CONFIG_DECT_MDM_DEVICE_NAME);
	if (err != 0) {
		LOG_ERR("set_name failed: %d", err);
		return;
	}

	dect_net_l2_init(iface, &mock_ctx.settings);
	net_if_dormant_on(iface);
	mock_iface_initialized = true;
}

void dect_mdm_mock_status_info_set(const struct dect_status_info *status)
{
	mock_status_override_info = *status;
	mock_status_override = true;
}

void dect_mdm_mock_status_info_clear(void)
{
	mock_status_override = false;
}

static int mock_status_info_get(const struct device *dev, struct dect_status_info *status_out)
{
	ARG_UNUSED(dev);

	if (mock_status_override) {
		*status_out = mock_status_override_info;
		return 0;
	}

	memset(status_out, 0, sizeof(*status_out));
	status_out->mdm_activated = mock_ctx.activated;
	strncpy(status_out->fw_version_str, DECT_MDM_MOCK_FW_VERSION,
		sizeof(status_out->fw_version_str) - 1);
	return 0;
}

static int mock_activate_req(const struct device *dev)
{
	struct net_if *iface = mock_ctx.iface;

	ARG_UNUSED(dev);

	if (mock_ctx.activated) {
		return 0;
	}

	mock_ctx.activated = true;
	net_if_carrier_on(iface);
	dect_mgmt_activate_done_evt(iface, DECT_STATUS_OK);
	return 0;
}

static int mock_settings_read(const struct device *dev, struct dect_settings *settings_out)
{
	ARG_UNUSED(dev);

	memcpy(settings_out, &mock_ctx.settings, sizeof(*settings_out));
	return 0;
}

static int mock_network_join_req(const struct device *dev)
{
	ARG_UNUSED(dev);

	dect_mdm_mock_network_join_call_count++;
	return 0;
}

static int mock_network_unjoin_req(const struct device *dev)
{
	ARG_UNUSED(dev);

	dect_mdm_mock_network_unjoin_call_count++;
	return 0;
}

static int mock_deactivate_req(const struct device *dev)
{
	struct net_if *iface = mock_ctx.iface;

	ARG_UNUSED(dev);

	if (!mock_ctx.activated) {
		return 0;
	}

	mock_ctx.activated = false;
	net_if_carrier_off(iface);
	dect_mgmt_deactivate_done_evt(iface, DECT_STATUS_OK);
	return 0;
}

static int mock_send(const struct device *dev, struct net_pkt *pkt)
{
	uint32_t target_long_rd_id;
	int data_len;

	ARG_UNUSED(dev);
	__ASSERT_NO_MSG(pkt != NULL);

	if (net_pkt_family(pkt) != AF_INET6) {
		return -EINVAL;
	}

	target_long_rd_id = dect_utils_lib_dst_long_rd_id_get_from_pkt_dst_addr(pkt);
	if (!target_long_rd_id) {
		return -EINVAL;
	}

	data_len = net_pkt_get_len(pkt);
	if (data_len > (int)sizeof(dect_mdm_mock_last_send_data)) {
		return -EMSGSIZE;
	}

	if (net_pkt_is_empty(pkt)) {
		return -EIO;
	}

	memcpy(dect_mdm_mock_last_send_data, net_pkt_ip_data(pkt), data_len);

	dect_mdm_mock_send_call_count++;
	dect_mdm_mock_last_send_long_rd_id = target_long_rd_id;
	dect_mdm_mock_last_send_len = (size_t)data_len;

	return 0;
}

static int mock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static const struct dect_nr_hal_api dect_mdm_mock_api = {
	.iface_api.init = mock_iface_init,
	.status_info_get = mock_status_info_get,
	.settings_read = mock_settings_read,
	.activate_req = mock_activate_req,
	.deactivate_req = mock_deactivate_req,
	.network_join_req = mock_network_join_req,
	.network_unjoin_req = mock_network_unjoin_req,
	.send = mock_send,
};

NET_DEVICE_INIT(dect_mdm_mock, CONFIG_DECT_MDM_DEVICE_NAME, mock_driver_init, NULL, &mock_ctx,
		NULL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dect_mdm_mock_api,
		DECT_L2, NET_L2_GET_CTX_TYPE(DECT_L2), DECT_MTU);

#if defined(CONFIG_NET_CONNECTION_MANAGER)
CONNECTIVITY_DECT_MGMT_BIND(dect_mdm_mock);
#endif
