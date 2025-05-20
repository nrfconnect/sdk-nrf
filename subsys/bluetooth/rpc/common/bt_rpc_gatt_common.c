/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <zephyr/bluetooth/gatt.h>

#include "bt_rpc_gatt_common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(BT_RPC, CONFIG_BT_RPC_LOG_LEVEL);

#define ATTR_INDEX_POS 16
#define ATTR_INDEX_MASK 0xFFFF

static struct bt_gatt_svc_cache {
	const struct bt_gatt_service *services[CONFIG_BT_RPC_GATT_SRV_MAX];
	size_t count;
} svc_cache = {
	.count = 0,
};

int bt_rpc_gatt_add_service(const struct bt_gatt_service *svc, uint32_t *svc_index)
{
	uint32_t index;

	if (svc_cache.count >= CONFIG_BT_RPC_GATT_SRV_MAX) {
		LOG_ERR("Too many GATT services used by BT_RPC. %s",
			"Increase CONFIG_BT_RPC_GATT_SRV_MAX.");
		return -ENOMEM;
	}

	index = svc_cache.count;
	svc_cache.services[index] = svc;

	svc_cache.count++;

	*svc_index = index;

	return 0;
}

int bt_rpc_gatt_attr_to_index(const struct bt_gatt_attr *attr, uint32_t *index)
{
	const struct bt_gatt_service *service;
	uint32_t attr_index;
	uint32_t service_index = 0;

	if (!attr) {
		return -EINVAL;
	}

	do {
		if (service_index >= svc_cache.count) {
			return -EINVAL;
		}

		service = svc_cache.services[service_index];

		if ((attr >= service->attrs) && (attr < &service->attrs[service->attr_count])) {
			break;
		}

		service_index++;
	} while (true);

	attr_index = attr - service->attrs;

	*index = (service_index << ATTR_INDEX_POS) | (attr_index & ATTR_INDEX_MASK);

	return 0;
}

const struct bt_gatt_attr *bt_rpc_gatt_index_to_attr(uint32_t index)
{
	uint32_t service_index = index >> ATTR_INDEX_POS;
	uint32_t attr_index = index & ATTR_INDEX_MASK;
	const struct bt_gatt_service *service = svc_cache.services[service_index];

	if ((service_index >= svc_cache.count) || (attr_index >= service->attr_count)) {
		return NULL;
	}

	return &service->attrs[attr_index];
}

const struct bt_gatt_service *bt_rpc_gatt_get_service_by_index(uint16_t svc_index)
{
	if (svc_index > svc_cache.count) {
		return NULL;
	}

	return svc_cache.services[svc_index];
}

int bt_rpc_gatt_service_to_index(const struct bt_gatt_service *svc, uint16_t *svc_index)
{
	int err = -EFAULT;

	if (!svc_index || !svc) {
		return -EINVAL;
	}

	for (size_t i = 0; i < svc_cache.count; i++) {
		if (svc == svc_cache.services[i]) {
			*svc_index = i;
			err = 0;
			break;
		}
	}

	return err;
}

int bt_rpc_gatt_remove_service(const struct bt_gatt_service *svc)
{
	if (!svc) {
		return -EINVAL;
	}

	for (size_t i = 0; i < svc_cache.count; i++) {
		if (svc == svc_cache.services[i]) {
			if (i == ARRAY_SIZE(svc_cache.services)) {
				svc_cache.services[i - 1] = NULL;
			} else {
				memmove(&svc_cache.services[i], &svc_cache.services[i + 1],
					(sizeof(svc_cache.services[i]) * (svc_cache.count - i)));
			}

			svc_cache.count--;

			break;
		}
	}

	return 0;
}

static uint8_t gatt_foreach_iter(const struct bt_gatt_attr *attr,
				 uint16_t handle, uint16_t start_handle,
				 uint16_t end_handle,
				 const struct bt_uuid *uuid,
				 const void *attr_data, uint16_t *num_matches,
				 bt_gatt_attr_func_t func, void *user_data)
{
	uint8_t result;

	/* Stop if over the requested range */
	if (handle > end_handle) {
		return BT_GATT_ITER_STOP;
	}

	/* Check if attribute handle is within range */
	if (handle < start_handle) {
		return BT_GATT_ITER_CONTINUE;
	}

	/* Match attribute UUID if set */
	if (uuid && bt_uuid_cmp(uuid, attr->uuid)) {
		return BT_GATT_ITER_CONTINUE;
	}

	/* Match attribute user_data if set */
	if (attr_data && attr_data != attr->user_data) {
		return BT_GATT_ITER_CONTINUE;
	}

	*num_matches -= 1;

	result = func(attr, handle, user_data);

	if (!*num_matches) {
		return BT_GATT_ITER_STOP;
	}

	return result;
}

void bt_rpc_gatt_foreach_attr_type(uint16_t start_handle, uint16_t end_handle,
				   const struct bt_uuid *uuid,
				   const void *attr_data, uint16_t num_matches,
				   bt_gatt_attr_func_t func,
				   void *user_data)
{
	uint16_t handle;

	if (!num_matches) {
		num_matches = UINT16_MAX;
	}

	for (size_t i = 0; i < svc_cache.count; i++) {
		const struct bt_gatt_service *svc = svc_cache.services[i];

		handle = bt_gatt_attr_get_handle(&svc->attrs[0]);
		if (handle + svc->attr_count < start_handle) {
			continue;
		}

		for (size_t j = 0; j < svc->attr_count; j++) {
			struct bt_gatt_attr *attr = &svc->attrs[j];

			handle = bt_gatt_attr_get_handle(attr);

			if (gatt_foreach_iter(attr, handle,
					      start_handle,
					      end_handle,
					      uuid, attr_data,
					      &num_matches,
					      func, user_data) ==
			    BT_GATT_ITER_STOP) {
				return;
			}
		}
	}
}
