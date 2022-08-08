/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef EMDS_FLASH_H__
#define EMDS_FLASH_H__

#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Emergency data storage file system structure
 *
 * @param offset File system offset from start of flash
 * @param ate_wra Allocation table entry write address. Addresses are stored as uint32_t:
 * high 2 bytes correspond to the sector, low 2 bytes are the offset in the sector
 * @param ate_size Size of allocation table entry
 * @param data_wra_offset Data write address offset
 * @param sector_size File system is split into sectors, each sector must be multiple of pagesize
 * @param sector_cnt Number of sectors in the file systems
 * @param is_initialized Initialized flag
 * @param is_prepeared Prepared for write flag
 * @param emds_lock Mutex
 * @param flash_dev Pointer to flash device runtime structure
 * @param flash_params Pointer to flash memory parameters structure
 * @param force_erase Force erase flag
 */
struct emds_fs {
	off_t offset;
	uint32_t ate_wra;
	size_t ate_size;
	uint32_t data_wra_offset;
	uint16_t sector_size;
	uint16_t sector_cnt;
	bool is_initialized;
	bool is_prepeared;
	struct k_mutex emds_lock;
	const struct device *flash_dev;
	const struct flash_parameters *flash_params;
	bool force_erase;
};

/**
 * @brief Initialize emergency data storage flash.
 *
 * @param fs Pointer to file system
 *
 * @retval 0 on success or negative error code
 */
int emds_flash_init(struct emds_fs *fs);

/**
 * @brief Clears the emergency data storage file system from flash.
 *
 * @param fs Pointer to file system
 *
 * @note Calling this function will make any subsequent read attempts fail. Be sure to
 * restore all necessary entries before using this function.
 *
 * @retval 0 on success or negative error code
 */
int emds_flash_clear(struct emds_fs *fs);

/**
 * @brief Write an entry to the EMDS file system.
 *
 * @param fs Pointer to file system
 * @param id Id of the entry to be written
 * @param data Pointer to the data to be written
 * @param len Number of bytes to be written
 *
 * @return Number of bytes written. On success, it will be equal to the number of bytes requested
 * to be written. When a rewrite of the same data already stored is attempted, nothing is written
 * to flash, thus 0 is returned. On error, returns negative value of errno.h defined error codes.
 */
ssize_t emds_flash_write(struct emds_fs *fs, uint16_t id, const void *data, size_t len);

/**
 * @brief Read an entry from the EMDS file system.
 *
 * Read an entry from the file system.
 *
 * @param fs Pointer to file system
 * @param id Id of the entry to be read
 * @param data Pointer to data buffer
 * @param len Number of bytes in data buffer
 *
 * @return Number of bytes read. On success, it will be equal to the number of bytes requested
 * to be read. When the return value is larger than the number of bytes requested to read this
 * indicates not all bytes were read, and more data is available. On error, returns negative
 * value of errno.h defined error codes.
 */
ssize_t emds_flash_read(struct emds_fs *fs, uint16_t id, void *data, size_t len);

/**
 * @brief Prepare EMDS file system for next write events.
 *
 * This function should be called at the moment when the user has restored the desired data
 * entries from flash. It will invalidate all prior entries, and potentially clear the flash
 * area.
 *
 * @note Calling this function will make any subsequent read attempts fail. Be sure to
 * restore all necessary entries before using this function.
 *
 * @note Should only be called once.
 *
 * @param fs Pointer to file system
 * @param byte_size Total number of bytes
 *
 * @retval 0 on success or negative error code
 */
int emds_flash_prepare(struct emds_fs *fs, int byte_size);

/**
 * @brief Get remaining raw space on the flash device.
 *
 * @param fs Pointer to file system
 *
 * @retval Remaining free space of the flash device in bytes
 */
ssize_t emds_flash_free_space_get(struct emds_fs *fs);


#ifdef __cplusplus
}
#endif

#endif /* EMDS_FLASH_H__ */
