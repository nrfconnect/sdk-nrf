/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 *
 * @ingroup nfc_writable_ndef_msg_example_ndef_file_m ndef_file_m.c
 * @{
 * @ingroup nfc_writable_ndef_msg_example
 * @brief FLASH service for the NFC writable NDEF message example.
 *
 */

#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <string.h>
#include <fs/nvs.h>
#include <nfc/ndef/nfc_uri_msg.h>

#include "ndef_file_m.h"

#define FLASH_URL_ADDRESS_ID 1 /**< Address of URL message in FLASH */

static const u8_t m_url[] = /**< Default NDEF message: URL "nordicsemi.com". */
	{'n', 'o', 'r', 'd', 'i', 'c', 's', 'e', 'm', 'i', '.', 'c', 'o', 'm'};

 /* Flash block size in bytes */
#define NVS_SECTOR_SIZE    (DT_FLASH_WRITE_BLOCK_SIZE * 1024)
#define NVS_SECTOR_COUNT   2
 /* Start address of the filesystem in flash */
#define NVS_STORAGE_OFFSET DT_FLASH_AREA_STORAGE_OFFSET_0

static struct nvs_fs fs = {
	.sector_size = NVS_SECTOR_SIZE,
	.sector_count = NVS_SECTOR_COUNT,
	.offset = NVS_STORAGE_OFFSET,
};

int ndef_file_setup(void)
{
	int err;

	err = nvs_init(&fs, DT_FLASH_DEV_NAME);
	if (err < 0) {
		printk("Cannot initialize NVS!\n");
	}

	return err;
}

int ndef_file_update(u8_t const *buff, u32_t size)
{
	/* Update FLASH file with new NDEF message. */
	return nvs_write(&fs, FLASH_URL_ADDRESS_ID, buff, size);
}

int ndef_file_default_message(u8_t *buff, u32_t *size)
{
	/* Encode URI message into buffer. */
	return nfc_uri_msg_encode(NFC_URI_HTTP_WWW,
				  m_url,
				  sizeof(m_url),
				  buff,
				  size);
}

int ndef_restore_default(u8_t *buff, u32_t size)
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

int ndef_file_load(u8_t *buff, u32_t size)
{
	int err;

	/* If there is no record with given ID, create default message and store
	 * in FLASH. FLASH_URL_ADDRESS_ID is used to store an address, lets see
	 * if we can read it from flash, since we don't know the size read the
	 * maximum possible
	 */
	err = nvs_read(&fs, FLASH_URL_ADDRESS_ID, buff, size);
	if (err > 0) { /* Item was found, show it */
		printk("Found NDEF file record.\n");
	} else {
		printk("NDEF file record not found, creating default NDEF.\n");
		/* Create default NDEF message. */
		err = ndef_restore_default(buff, size);
	}

	return err;
}

/** @} */
