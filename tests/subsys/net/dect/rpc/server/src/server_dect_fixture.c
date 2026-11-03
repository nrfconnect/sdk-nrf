/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "server_dect_fixture.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>

#include <net/dect/dect_net_l2.h>
#include <net/dect/dect_net_l2_mgmt.h>

LOG_MODULE_REGISTER(server_dect_fixture, LOG_LEVEL_INF);

static struct net_if *server_dect_iface;
static bool booted;

struct net_if *server_dect_fixture_ensure_booted(void)
{
	int err;

	if (booted) {
		return server_dect_iface;
	}

	const struct device *dev = device_get_binding(CONFIG_DECT_MDM_DEVICE_NAME);

	if (dev == NULL) {
		LOG_ERR("No device \"%s\"", CONFIG_DECT_MDM_DEVICE_NAME);
		return NULL;
	}

	server_dect_iface = net_if_lookup_by_dev(dev);
	if (server_dect_iface == NULL) {
		LOG_ERR("No net_if for \"%s\"", CONFIG_DECT_MDM_DEVICE_NAME);
		return NULL;
	}

	err = net_mgmt(NET_REQUEST_DECT_ACTIVATE, server_dect_iface, NULL, 0);
	if (err != 0) {
		LOG_ERR("DECT activate failed: %d", err);
		return NULL;
	}

	err = net_if_up(server_dect_iface);
	if (err != 0 && err != -EALREADY) {
		LOG_ERR("net_if_up failed: %d", err);
		return NULL;
	}

	booted = true;
	return server_dect_iface;
}

int server_dect_fixture_associate(void)
{
	struct dect_net_ipv6_prefix_config no_prefix = { 0 };

	if (!server_dect_iface) {
		return -ENODEV;
	}

	if (!net_if_is_dormant(server_dect_iface)) {
		return 0;
	}

	dect_net_l2_parent_association_created(server_dect_iface,
					       SERVER_DECT_FIXTURE_PARENT_LONG_RD_ID, &no_prefix);
	return 0;
}

int server_dect_fixture_release(void)
{
	if (!server_dect_iface) {
		return -ENODEV;
	}

	if (net_if_is_dormant(server_dect_iface)) {
		return 0;
	}

	dect_net_l2_association_removed(server_dect_iface, SERVER_DECT_FIXTURE_PARENT_LONG_RD_ID,
					DECT_RELEASE_CAUSE_CONNECTION_TERMINATION, false);
	return 0;
}
