/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @ingroup nfc_writable_ndef_msg_example_ndef_file_m ndef_file_m.c
 * @{
 * @ingroup nfc_writable_ndef_msg_example
 * @brief FLASH service for the NFC writable NDEF message example.
 *
 */

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/device.h>
#include <nfc/t4t/ndef_file.h>
#include <nfc/ndef/uri_msg.h>
#include "ndef_file_m.h"

#if !defined(CONFIG_NDEF_FILE_STORAGE_RAM)
#include <zephyr/storage/flash_map.h>
#endif /* !defined(CONFIG_NDEF_FILE_STORAGE_RAM) */
#if defined(CONFIG_NVS)
#include <zephyr/kvss/nvs.h>
#elif defined(CONFIG_ZMS)
#include <zephyr/kvss/zms.h>
#endif /* defined(CONFIG_NVS) */

#define FLASH_URL_ADDRESS_ID 1 /**< Address of URL message in FLASH */

static const uint8_t m_url[] = /**< Default NDEF message: URL "nordicsemi.com". */
	{'n', 'o', 'r', 'd', 'i', 'c', 's', 'e', 'm', 'i', '.', 'c', 'o', 'm'};

#if defined(CONFIG_NDEF_FILE_STORAGE_RAM)

static uint8_t ram_ndef[CONFIG_NDEF_FILE_SIZE];
static uint32_t ram_ndef_len;
static bool ram_ndef_valid;

#else /* CONFIG_NDEF_FILE_STORAGE_RAM */

/* Flash partition for storage*/
#define FLASH_DEVICE PARTITION_DEVICE(storage_partition)
/* Flash block size in bytes */
#define SECTOR_SIZE  (DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size))
#define SECTOR_COUNT 2
/* Start address of the filesystem in flash */
#define STORAGE_OFFSET PARTITION_OFFSET(storage_partition)

#if defined(CONFIG_NVS)
static struct nvs_fs fs = {
	.sector_size = SECTOR_SIZE,
	.sector_count = SECTOR_COUNT,
	.offset = STORAGE_OFFSET,
};
#elif defined(CONFIG_ZMS)
static struct zms_fs fs = {
	.sector_size = SECTOR_SIZE,
	.sector_count = SECTOR_COUNT,
	.offset = STORAGE_OFFSET,
};
#else
#error "Please select file system."
#endif /* defined(CONFIG_NVS) */

#endif /* CONFIG_NDEF_FILE_STORAGE_RAM */

int ndef_file_setup(void)
{
#if defined(CONFIG_NDEF_FILE_STORAGE_RAM)
	return 0;
#else /* CONFIG_NDEF_FILE_STORAGE_RAM */
	int err = 0;

	fs.flash_device = FLASH_DEVICE;
	if (fs.flash_device == NULL) {
		return -ENODEV;
	}

#if defined(CONFIG_NVS)
	err = nvs_mount(&fs);
#elif defined(CONFIG_ZMS)
	err = zms_mount(&fs);
#endif /* defined(CONFIG_NVS) */

	if (err < 0) {
		printk("Cannot initialize storage!\n");
	}

	return err;
#endif /* CONFIG_NDEF_FILE_STORAGE_RAM */
}

int ndef_file_update(uint8_t const *buff, uint32_t size)
{
	/* Update storage with new NDEF message. */
#if defined(CONFIG_NVS)
	return nvs_write(&fs, FLASH_URL_ADDRESS_ID, buff, size);
#elif defined(CONFIG_ZMS)
	return zms_write(&fs, FLASH_URL_ADDRESS_ID, buff, size);
#elif defined(CONFIG_NDEF_FILE_STORAGE_RAM)
	if (size > CONFIG_NDEF_FILE_SIZE) {
		return -ENOMEM;
	}

	memcpy(ram_ndef, buff, size);
	ram_ndef_len = size;
	ram_ndef_valid = true;

	return size;
#endif /* defined(CONFIG_NVS) */
}

/** .. include_startingpoint_ndef_file_rst */
int ndef_file_default_message(uint8_t *buff, uint32_t *size)
{
	int err;
	uint32_t ndef_size = nfc_t4t_ndef_file_msg_size_get(*size);

	/* Encode URI message into buffer. */
	err = nfc_ndef_uri_msg_encode(NFC_URI_HTTP_WWW,
				      m_url,
				      sizeof(m_url),
				      nfc_t4t_ndef_file_msg_get(buff),
				      &ndef_size);
	if (err) {
		return err;
	}

	err = nfc_t4t_ndef_file_encode(buff, &ndef_size);
	if (err) {
		return err;
	}

	*size = ndef_size;

	return 0;
}
/** .. include_endpoint_ndef_file_rst */

int ndef_restore_default(uint8_t *buff, uint32_t size)
{
	int err;

	err = ndef_file_default_message(buff, &size);
	if (err < 0) {
		printk("Cannot create default message!\n");
		return err;
	}

	/* Save record with default NDEF message. */
	err = ndef_file_update(buff, size);
	if (err < 0) {
		printk("Cannot flash NDEF message!\n");
	}
	return err;
}

int ndef_file_load(uint8_t *buff, uint32_t size)
{
#if defined(CONFIG_NDEF_FILE_STORAGE_RAM)
	if (ram_ndef_valid && ram_ndef_len <= size) {
		memcpy(buff, ram_ndef, ram_ndef_len);
		printk("Found NDEF file in RAM.\n");
		return ram_ndef_len;
	}

	printk("NDEF file not in RAM, creating default NDEF.\n");
	return ndef_restore_default(buff, size);
#else /* CONFIG_NDEF_FILE_STORAGE_RAM */
	int err = 0;

	/* If there is no record with given ID, create default message and store
	 * in FLASH. FLASH_URL_ADDRESS_ID is used to store an address, lets see
	 * if we can read it from flash, since we don't know the size read the
	 * maximum possible
	 */
#if defined(CONFIG_NVS)
	err = nvs_read(&fs, FLASH_URL_ADDRESS_ID, buff, size);
#elif defined(CONFIG_ZMS)
	err = zms_read(&fs, FLASH_URL_ADDRESS_ID, buff, size);
#endif /* defined(CONFIG_NVS) */

	if (err > 0) { /* Item was found, show it */
		printk("Found NDEF file record.\n");
	} else {
		printk("NDEF file record not found, creating default NDEF.\n");
		/* Create default NDEF message. */
		err = ndef_restore_default(buff, size);
	}

	return err;
#endif /* CONFIG_NDEF_FILE_STORAGE_RAM */
}

/** @} */
