/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef EMDS_FLASH_H__
#define EMDS_FLASH_H__

#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/flash_map.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Emergency data storage partition descriptor
 *
 * @param part_off Partition offset from the beginning of the flash area
 * @param part_size Partition size in bytes. It must be aligned to flash page size
 * in case of flash memory.
 */
struct emds_partition_desc {
	off_t part_off;
	size_t part_size;
};

/**
 * @brief Emergency data storage descriptor
 *
 * @param fa Flash area pointer
 * @param fp Flash parameters pointer
 * @param part Array of two emergency data storage partitions
 */
struct emds_desc {
	const struct flash_area *fa;
	const struct flash_parameters *fp;
	struct emds_partition_desc part[2];
};

/**
 * @brief Emergency data storage data entry structure
 *
 * @param id Unique data identifier.
 * @param length Data length.
 * @param data Zero length array for data reference.
 */
struct emds_data_entry {
	uint16_t id;
	uint16_t length;
	uint8_t data[];
} __packed;

/**
 * @brief Emergency data storage metadata structure
 *
 * This structure is used to store metadata about the emergency data storage.
 * It contains information about the data instance area, its length, and CRCs
 * for both the metadata and the snapshot area.
 *
 * @param marker Constant value to follow the end of the table.
 * @param fresh_cnt Increment counter for every data instance.
 * @param data_instance_addr The start address of the data instance area.
 * @param data_instance_len The data instance area length.
 * @param metadata_crc The metadata structure CRC.
 * @param snapshot_crc The snapshot area CRC.
 */
struct emds_snapshot_metadata {
	uint32_t marker;
	uint32_t fresh_cnt;
	uint32_t data_instance_addr;
	uint32_t data_instance_len;
	uint32_t metadata_crc;
	uint32_t snapshot_crc;
} __packed;

/**
 * @brief Initialize emergency data storage flash.
 *
 * @param desc Pointer to emergency data storage descriptor
 *
 * @retval 0 on success or negative error code
 */
int emds_flash_init(struct emds_desc *desc);

#ifdef __cplusplus
}
#endif

#endif /* EMDS_FLASH_H__ */
