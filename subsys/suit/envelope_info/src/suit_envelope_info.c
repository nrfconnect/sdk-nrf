/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zcbor_decode.h>

#include "suit_envelope_info.h"

LOG_MODULE_REGISTER(suit_envelope_info, CONFIG_SUIT_LOG_LEVEL);

#if (!(DT_NODE_EXISTS(DT_NODELABEL(dfu_partition))))
#error DFU Partition not defined in devicetree
#endif

#define FIXED_PARTITION_ERASE_BLOCK_SIZE(label)                                                    \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), erase_block_size)
#define FIXED_PARTITION_WRITE_BLOCK_SIZE(label)                                                    \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), write_block_size)

#define DFU_PARTITION_LABEL   dfu_partition
#define DFU_PARTITION_OFFSET  FIXED_PARTITION_OFFSET(DFU_PARTITION_LABEL)
#define DFU_PARTITION_SIZE    FIXED_PARTITION_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_EB_SIZE FIXED_PARTITION_ERASE_BLOCK_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_WB_SIZE FIXED_PARTITION_WRITE_BLOCK_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_DEVICE  FIXED_PARTITION_DEVICE(DFU_PARTITION_LABEL)
#define EMPTY_STORAGE_VALUE   0xFFFFFFFF

suit_plat_err_t suit_dfu_partition_envelope_info_get(const uint8_t **address, size_t *size)
{

	zcbor_state_t states[3];

	struct nvm_address device_offset = {
		.fdev = DFU_PARTITION_DEVICE,
		.offset = DFU_PARTITION_OFFSET,
	};

	if (address == NULL || size == NULL) {
		LOG_ERR("Wrong parameters");
		return SUIT_PLAT_ERR_INVAL;
	}

	uintptr_t mapped_address = 0;

	if (!suit_memory_nvm_address_to_global_address(&device_offset, &mapped_address)) {
		LOG_ERR("Cannot obtain memory-mapped DFU partition address");
		return SUIT_PLAT_ERR_IO;
	}

	const uint8_t *envelope_address = (uint8_t *)mapped_address;
	size_t envelope_size = DFU_PARTITION_SIZE;

	if (*((uint32_t *)envelope_address) == EMPTY_STORAGE_VALUE) {
		LOG_DBG("DFU partition empty");
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), envelope_address,
			envelope_size, 1, NULL, 0);

	/* Expect SUIT envelope tag (107) and skip until the end of the envelope */
	if (!zcbor_tag_expect(states, 107) || !zcbor_any_skip(states, NULL)) {
		LOG_DBG("Malformed envelope");
		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	envelope_size = (size_t)states[0].payload - (size_t)envelope_address;

	if (envelope_size > DFU_PARTITION_SIZE) {
		LOG_DBG("Malformed envelope");
		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	*address = envelope_address;
	*size = envelope_size;

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_dfu_partition_device_info_get(struct suit_nvm_device_info *device_info)
{
	if (device_info == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	device_info->fdev = DFU_PARTITION_DEVICE;
	device_info->mapped_address = 0;
	device_info->erase_block_size = DFU_PARTITION_EB_SIZE;
	device_info->write_block_size = DFU_PARTITION_WB_SIZE;
	device_info->partition_offset = DFU_PARTITION_OFFSET;
	device_info->partition_size = DFU_PARTITION_SIZE;

	struct nvm_address device_offset = {
		.fdev = DFU_PARTITION_DEVICE,
		.offset = DFU_PARTITION_OFFSET,
	};

	uintptr_t mapped_address = 0;

	if (suit_memory_nvm_address_to_global_address(&device_offset, &mapped_address)) {
		device_info->mapped_address = (uint8_t *)mapped_address;
	}

	return SUIT_PLAT_SUCCESS;
}

bool suit_dfu_partition_is_empty(void)
{
	struct nvm_address device_offset = {
		.fdev = DFU_PARTITION_DEVICE,
		.offset = DFU_PARTITION_OFFSET,
	};

	uintptr_t mapped_address = 0;

	if (!suit_memory_nvm_address_to_global_address(&device_offset, &mapped_address)) {
		LOG_ERR("Cannot obtain memory-mapped DFU partition address");
		return false;
	}

	for (size_t i = 0; i < DFU_PARTITION_SIZE / sizeof(uint32_t); i++) {
		if (*(((uint32_t *)mapped_address) + i) != EMPTY_STORAGE_VALUE) {
			return false;
		}
	}

	return true;
}
