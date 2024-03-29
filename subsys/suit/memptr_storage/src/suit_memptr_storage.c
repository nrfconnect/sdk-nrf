/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_memptr_storage.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_memptr_storage, CONFIG_SUIT_LOG_LEVEL);

struct memptr_storage {
	const uint8_t *payload_ptr;
	size_t payload_size;
	bool in_use;
};

static struct memptr_storage records[CONFIG_SUIT_MAX_NUMBER_OF_INTEGRATED_PAYLOADS];

suit_memptr_storage_err_t suit_memptr_storage_get(memptr_storage_handle_t *handle)
{
	if (handle != NULL) {
		for (size_t i = 0; i < CONFIG_SUIT_MAX_NUMBER_OF_INTEGRATED_PAYLOADS; i++) {
			if (!records[i].in_use) {
				records[i].in_use = true;
				records[i].payload_size = 0;
				records[i].payload_ptr = NULL;

				*handle = &records[i];

				return SUIT_PLAT_SUCCESS;
			};
		}

		LOG_ERR("No free records where found.");
		return SUIT_PLAT_ERR_NOMEM;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

suit_memptr_storage_err_t suit_memptr_storage_ptr_store(memptr_storage_handle_t handle,
							const uint8_t *payload_ptr,
							size_t payload_size)
{
	if (handle != NULL) {
		struct memptr_storage *record = (struct memptr_storage *)handle;

		if (record->in_use) {
			record->payload_ptr = payload_ptr;
			record->payload_size = payload_size;

			return SUIT_PLAT_SUCCESS;
		}

		LOG_ERR("Write to unallocated record");
		return SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

suit_memptr_storage_err_t suit_memptr_storage_ptr_get(memptr_storage_handle_t handle,
						      const uint8_t **payload_ptr,
						      size_t *payload_size)
{
	if ((handle != NULL) && (payload_ptr != NULL) && (payload_size != NULL)) {
		struct memptr_storage *record = (struct memptr_storage *)handle;

		if (record->in_use) {
			*payload_ptr = record->payload_ptr;
			*payload_size = record->payload_size;

			return SUIT_PLAT_SUCCESS;
		}

		LOG_ERR("Invalid record.");
		return SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

suit_memptr_storage_err_t suit_memptr_storage_release(memptr_storage_handle_t handle)
{
	if (handle != NULL) {
		struct memptr_storage *record = (struct memptr_storage *)handle;

		record->in_use = false;
		record->payload_size = 0;
		record->payload_ptr = NULL;

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}
