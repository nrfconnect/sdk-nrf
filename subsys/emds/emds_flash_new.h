/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef EMDS_FLASH_H__
#define EMDS_FLASH_H__

#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if two regions overlap
 *
 * This macro checks if two memory regions overlap.
 *
 * @param a Start address of the first region.
 * @param len_a Length of the first region.
 * @param b Start address of the second region.
 * @param len_b Length of the second region.
 *
 * @return true if regions overlap, false otherwise.
 */
#define REGIONS_OVERLAP(a, len_a, b, len_b) (((a) < ((b) + (len_b))) && ((b) < ((a) + (len_a))))

/**
 * @brief Emergency data storage partition descriptor
 *
 * This structure describes a partition in the emergency data storage.
 * It contains the flash area and its parameters.
 *
 * @param fa Flash area for the partition.
 * @param fp Flash parameters for the partition.
 */
struct emds_partition {
	const struct flash_area *fa;
	const struct flash_parameters *fp;
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
 * EMDS storage API can be called maximum of 4B times, due to width of the fresh_cnt,
 * in the lifetime of devices. This will never happen in the lifetime of the device
 * as flash endurance will run out much before this count is reached.
 *
 * @param marker Constant value to follow the end of the table.
 * @param fresh_cnt Increment counter for every data instance.
 * @param data_instance_off The start offset of the data instance area.
 * @param data_instance_len The data instance area length.
 * @param metadata_crc The metadata structure CRC.
 * @param snapshot_crc The snapshot area CRC.
 */
struct emds_snapshot_metadata {
	uint32_t marker;
	uint32_t fresh_cnt;
	off_t data_instance_off;
	uint32_t data_instance_len;
	uint32_t metadata_crc;
	uint32_t snapshot_crc;
	uint32_t reserved[2];
} __packed;

/**
 * @brief Emergency data storage snapshot candidate structure
 *
 * This structure is used to hold a candidate snapshot during the scanning
 * of the emergency data storage partition. It contains the metadata address
 * and the metadata itself.
 *
 * @param node Node for the system list.
 * @param partition_index Belonged partition index.
 * @param metadata_off Offset of the metadata within the partition.
 * @param metadata The snapshot metadata.
 */
struct emds_snapshot_candidate {
	sys_snode_t node;
	int partition_index;
	off_t metadata_off;
	struct emds_snapshot_metadata metadata;
};

/**
 * @brief Initialize the emergency data storage flash partition.
 *
 * @param partition Pointer to the emergency data storage partition structure.
 *
 * @retval 0 on success.
 * @retval -ENXIO if no valid flash device is found.
 * @retval -EINVAL if unable to obtain flash parameters or wrong partition format.
 */
int emds_flash_init(struct emds_partition *partition);

/**
 * @brief Scan the emergency data storage partition for valid snapshots.
 *
 * This function scans the specified partition for valid snapshots and populates
 * the candidate structure with the found snapshot metadata with the biggest
 * fresh_cnt value and valid metadata and snapshot crc values.
 *
 * @param partition Pointer to the emergency data storage partition structure.
 * @param candidate Pointer to the emergency data storage snapshot candidate structure
 *
 * @retval 0 on success.
 * @retval -EINVAL if the partition is not initialized or if an error occurs during reading.
 */
int emds_flash_scan_partition(const struct emds_partition *partition,
			      struct emds_snapshot_candidate *candidate);

/** * @brief Allocate a new snapshot in the emergency data storage partition.
 *
 * This function allocates a new snapshot in the specified partition based on the
 * freshest snapshot found in the partition.
 *
 * @param partition Pointer to the emergency data storage partition structure.
 * @param freshest_snapshot Pointer to the freshest snapshot candidate structure.
 * @param allocated_snapshot Pointer to the emergency data storage snapshot candidate structure
 * that will be filled with the allocated snapshot metadata.
 * @param data_size The size of the data to be allocated  including the size of the metadata.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the data size is invalid.
 * @retval -EADDRINUSE if the metadata or data area is not empty.
 * @retval -ENOMEM if the metadata area overlaps with the data area.
 */
int emds_flash_allocate_snapshot(const struct emds_partition *partition,
				 const struct emds_snapshot_candidate *freshest_snapshot,
				 struct emds_snapshot_candidate *allocated_snapshot,
				 size_t data_size);

/** * @brief Write data to the emergency data storage partition.
 *
 * @param partition Pointer to the emergency data storage partition structure.
 * @param data_off Offset in the partition where the data chunk should be written.
 * @param data_chunk Pointer to data chunk.
 * @param data_size Size of the data chunk.
 */
void emds_flash_write_data(const struct emds_partition *partition, off_t data_off, void *data_chunk,
			   size_t data_size);

/**
 * @brief Erase the specified emergency data storage partition.
 *
 * This function erases the entire partition, preparing it for new data storage.
 *
 * @param partition Pointer to the emergency data storage partition structure.
 *
 * @retval 0 on success, negative errno code on fail.
 */
int emds_flash_erase_partition(const struct emds_partition *partition);

#ifdef __cplusplus
}
#endif

#endif /* EMDS_FLASH_H__ */
