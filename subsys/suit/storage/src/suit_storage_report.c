/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_internal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_storage_report, CONFIG_SUIT_LOG_LEVEL);

/* Magic value, indicating that the report area is erased. */
#define REPORT_SLOT_MAGIC_EMPTY	 0xFFFFFFFF
/* Magic value, indicating that an empty report was saved.
 * This value may be used as a recovery flag indication.
 */
#define REPORT_SLOT_MAGIC_FLAG	 0x2a17644c
/* Magic value, indicating that the report slot contains report data.
 * In such case, a pointer to the whole area is returned.
 */
#define REPORT_SLOT_MAGIC_BINARY 0x713cf9c6

suit_plat_err_t suit_storage_report_internal_init(void)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	if (!device_is_ready(fdev)) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_report_internal_clear(uint8_t *area_addr, size_t area_size)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	if (area_addr == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (!device_is_ready(fdev)) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	/* Override report entry. */
	int err = flash_erase(fdev, suit_plat_mem_nvm_offset_get(area_addr), area_size);

	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_storage_report_internal_read(const uint8_t *area_addr, size_t area_size,
						  const uint8_t **buf, size_t *len)
{
	uint32_t *slot_magic = (uint32_t *)area_addr;

	if ((area_addr == NULL) || (buf == NULL) || (len == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (*slot_magic == REPORT_SLOT_MAGIC_FLAG) {
		*buf = NULL;
		*len = 0;

		return SUIT_PLAT_SUCCESS;
	} else if (*slot_magic == REPORT_SLOT_MAGIC_BINARY) {
		*buf = (const uint8_t *)&slot_magic[1];
		*len = area_size - sizeof(*slot_magic);
		;

		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_NOT_FOUND;
}

suit_plat_err_t suit_storage_report_internal_save(uint8_t *area_addr, size_t area_size,
						  const uint8_t *buf, size_t len)
{
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;
	uint32_t magic = REPORT_SLOT_MAGIC_EMPTY;
	uint8_t report_magic_buf[WRITE_ALIGN(sizeof(magic))];
	size_t buffered_size = MIN(len, sizeof(report_magic_buf) - sizeof(magic));

	if ((area_addr == NULL) || ((buf == NULL) && (len != 0)) ||
	    (len > (area_size - sizeof(magic)))) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (!device_is_ready(fdev)) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	memset(report_magic_buf, 0xFF, sizeof(report_magic_buf));

	if (len > 0) {
		magic = REPORT_SLOT_MAGIC_BINARY;
		if (buffered_size > 0) {
			memcpy(&report_magic_buf[sizeof(magic)], buf, buffered_size);
		}
	} else {
		magic = REPORT_SLOT_MAGIC_FLAG;
	}

	memcpy(report_magic_buf, &magic, sizeof(magic));

	/* Override report entry. */
	int err = flash_erase(fdev, suit_plat_mem_nvm_offset_get(area_addr), area_size);

	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	err = flash_write(fdev, suit_plat_mem_nvm_offset_get(area_addr), report_magic_buf,
			  sizeof(report_magic_buf));
	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	if (len > buffered_size) {
		size_t remaining_size = len - buffered_size;
		size_t write_size =
			(WRITE_ALIGN(remaining_size) == remaining_size
				 ? remaining_size
				 : WRITE_ALIGN(remaining_size) - SUIT_STORAGE_WRITE_SIZE);

		if (write_size > 0) {
			err = flash_write(fdev,
					  suit_plat_mem_nvm_offset_get(area_addr) +
						  sizeof(report_magic_buf),
					  &buf[buffered_size], write_size);
			if (err != 0) {
				return SUIT_PLAT_ERR_IO;
			}
		}

		if (remaining_size > write_size) {
			memset(report_magic_buf, 0xFF, SUIT_STORAGE_WRITE_SIZE);
			memcpy(report_magic_buf, &buf[buffered_size + write_size],
			       remaining_size - write_size);

			err = flash_write(fdev,
					  suit_plat_mem_nvm_offset_get(area_addr) +
						  sizeof(report_magic_buf) + write_size,
					  report_magic_buf, SUIT_STORAGE_WRITE_SIZE);
			if (err != 0) {
				return SUIT_PLAT_ERR_IO;
			}
		}
	}

	return SUIT_PLAT_SUCCESS;
}
