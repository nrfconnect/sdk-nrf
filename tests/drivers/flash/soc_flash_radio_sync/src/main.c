/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define TEST_DATA_PARTITION	   storage_partition
#define TEST_DATA_PARTITION_OFFSET PARTITION_OFFSET(TEST_DATA_PARTITION)
#define TEST_DATA_PARTITION_DEVICE PARTITION_DEVICE(TEST_DATA_PARTITION)

#define FLASH_PAGE_SIZE 4096
#define FLASH_WORD_SIZE 16
#define FLASH_PAGE_ID	0

const struct device *const flash_dev = TEST_DATA_PARTITION_DEVICE;

uint8_t test_pattern[FLASH_WORD_SIZE] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
					 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10};

uint8_t test_buffer[FLASH_WORD_SIZE] = {0};

/*
 * Set Advertisement data. Based on the Eddystone specification:
 * https://github.com/google/eddystone/blob/master/protocol-specification.md
 * https://github.com/google/eddystone/tree/master/eddystone-url
 */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16, 0xaa, 0xfe, /* Eddystone UUID */
		      0x10,			      /* Eddystone-URL frame type */
		      0x00,			      /* Calibrated Tx power at 0m */
		      0x00,			      /* URL Scheme Prefix http://www. */
		      'z', 'e', 'p', 'h', 'y', 'r', 'p', 'r', 'o', 'j', 'e', 'c', 't',
		      0x08) /* .org */
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void bt_ready(int err)
{
	char addr_s[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr = {0};
	size_t count = 1;

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	/* For connectable advertising you would use
	 * bt_le_oob_get_local().  For non-connectable non-identity
	 * advertising an non-resolvable private address is used;
	 * there is no API to retrieve that.
	 */

	bt_id_get(&addr, &count);
	bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

	printk("Beacon started, advertising as %s\n", addr_s);
}

int main(void)
{
	int err;
	int status = 0;

	printk("Starting Beacon\n");

	err = bt_enable(bt_ready);
	status += err;
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	k_msleep(1000);

	err = flash_write(flash_dev, TEST_DATA_PARTITION_OFFSET, test_pattern, FLASH_WORD_SIZE);
	status += err;
	printk("Flash write status: %d\n", err);

	err = flash_read(flash_dev, TEST_DATA_PARTITION_OFFSET, test_buffer, FLASH_WORD_SIZE);
	status += err;
	printk("Flash read status: %d\n", err);

	err = memcmp(test_buffer, test_pattern, FLASH_WORD_SIZE);
	if (err) {
		status += -1;
		printk("Written and read data does not match\n");
	}

	printk("Stopping Beacon\n");
	err = bt_disable();
	status += err;
	if (err) {
		printk("Bluetooth stop failed (err %d)\n", err);
	}

	if (status) {
		printk("Test status: FAIL\n");
	} else {
		printk("Test status: PASS\n");
	}

	return 0;
}
