/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdlib.h>
#include <stdint.h>

#include "nrf_errno.h"
#include "bt_rpc_gatt_common.h"

static struct bt_gatt_service_static *services[CONFIG_BT_RPC_GATT_SRV_MAX];
static size_t service_count = 0;

int bt_rpc_gatt_add_service(const struct bt_gatt_service_static *service)
{
	int service_index;

	if (service_count >= CONFIG_BT_RPC_GATT_SRV_MAX) {
		LOG_ERR("Too many services send by BT_RPC. Increase CONFIG_BT_RPC_GATT_SRV_MAX.");
		return -NRF_ENOMEM;
	}

	service_index = service_count;
	services[service_index] = service;
	service_count++;

	return service_index;
}

int bt_rpc_gatt_attr_to_index(struct bt_gatt_attr *attr)
{
	int attr_index;
	int service_index;
	const struct bt_gatt_service_static *service;

	service_index = 0;
	do
	{
		if (service_index >= service_count) {
			return -NRF_EINVAL;
		}
		service = services[service_index];
		if (attr >= service->attrs && attr < &service->attrs[service->attr_count]) {
			break;
		}
		service_index++;
	} while (true);

	attr_index = attr - service->attrs;

	return (service_index << 16) | (attr_index & 0xFFFF);
}

const struct bt_gatt_attr *bt_rpc_gatt_index_to_attr(int index)
{
	int service_index = index >> 16;
	int attr_index = index & 0xFFFF;
	struct bt_gatt_service_static *service = services[service_index];

	if (service_index >= service_count || attr_index >= service->attr_count) {
		return NULL;
	}

	return &service->attrs[attr_index];
}

int bt_rpc_gatt_srv_attr_to_index(uint8_t service_index, struct bt_gatt_attr *attr)
{
	size_t attr_index;
	struct bt_gatt_service_static *service;
	
	if (service_index >= service_count) {
		return -NRF_EINVAL;
	}

	service = services[service_index];

	attr_index = attr - service->attrs;

	if (attr_index >= service->attr_count) {
		return -NRF_EINVAL;
	}

	return (service_index << 16) | (attr_index & 0xFFFF);
}

BUILD_ASSERT(, "BT_RPC does not support BR/EDR");

/** @brief GATT Service structure */
struct bt_gatt_service_static {
	/** Service Attributes */
	const struct bt_gatt_attr *attrs;
	/** Service Attribute count */
	size_t			attr_count;
};

/** @brief GATT Service structure */
struct bt_gatt_service {
	/** Service Attributes */
	struct bt_gatt_attr	*attrs;
	/** Service Attribute count */
	size_t			attr_count;
	sys_snode_t		node;
};
