/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic ESL Service Client Application
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/slist.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <dk_buttons_and_leds.h>

#include "esl_client.h"

LOG_MODULE_REGISTER(central_esl, CONFIG_CENTRAL_ESL_LOG_LEVEL);

static struct bt_esl_client esl_client;

#ifdef CONFIG_MCUMGR_GRP_FS
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>

#define PARTITION_NODE DT_NODELABEL(lfs1)

#if DT_NODE_EXISTS(PARTITION_NODE)
/* Internal flash use DTS partition */
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
#else
/* External flash use PM partition */
#define STORAGE_PARTITION_LABEL littlefs_storage
#define STORAGE_PARTITION_ID	FIXED_PARTITION_ID(STORAGE_PARTITION_LABEL)

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);
static struct fs_mount_t pm_littlefs_mnt = {.type = FS_LITTLEFS,
					    .fs_data = &cstorage,
					    .storage_dev = (void *)STORAGE_PARTITION_ID,
					    .mnt_point = MNT_POINT};
#endif /* (DT_NODE_EXISTS(PARTITION_NODE)) */
struct fs_mount_t *mp =
#if DT_NODE_EXISTS(PARTITION_NODE)
	&FS_FSTAB_ENTRY(PARTITION_NODE)
#else
	&pm_littlefs_mnt
#endif /* (DT_NODE_EXISTS(PARTITION_NODE)*/
	;
#endif /* (CONFIG_MCUMGR_GRP_FS) */
int ap_image_storage_init(void)
{
	int err = 0;
#ifdef CONFIG_MCUMGR_GRP_FS
	err = fs_mount(mp);
	if (err < 0) {
		LOG_ERR("Error mounting littlefs [%d]", err);
	}
#if defined(CONFIG_BT_ESL_TAG_STORAGE)
	struct fs_dir_t dirp;

	load_all_tags_in_storage(BIT(ESL_GROUPID_RFU_BIT));

	/* Check if TAG_BLE_ADDR_ROOT exists */
	fs_dir_t_init(&dirp);
	err = fs_opendir(&dirp, TAG_BLE_ADDR_ROOT);
	if (err) {
		err = fs_mkdir(TAG_BLE_ADDR_ROOT);
		if (err) {
			LOG_ERR("Create TAG_BLE_ADDR_ROOT dir failed (ret %d)", err);
			err = -EIO;
		}
	}

	fs_closedir(&dirp);

	/* Check if TAG_ESL_ADDR_ROOT exists */
	fs_dir_t_init(&dirp);
	err = fs_opendir(&dirp, TAG_ESL_ADDR_ROOT);
	if (err) {
		err = fs_mkdir(TAG_ESL_ADDR_ROOT);
		if (err) {
			LOG_ERR("Create TAG_ESL_ADDR_ROOT dir failed (ret %d)", err);
			err = -EIO;
		}
	}

	fs_closedir(&dirp);

#endif /* (CONFIG_BT_ESL_TAG_STORAGE) */

#else
	LOG_ERR("No valid image storage");
	err = -ENXIO;
#endif /* (CONFIG_MCUMGR_GRP_FS) */
	return err;
}

ssize_t ap_read_img_size_from_storage(uint8_t *img_path)
{
	int ret = 0;
#ifdef CONFIG_MCUMGR_GRP_FS
	/* Here demostrates how to retrieve image size from littlefs */
	struct fs_dirent dirent;
	struct fs_file_t file;
	int rc;

	fs_file_t_init(&file);
	rc = fs_open(&file, img_path, FS_O_CREATE | FS_O_RDWR);
	if (rc < 0) {
		LOG_ERR("FAIL: open %s: %d", img_path, rc);
		return 0;
	}

	rc = fs_stat(img_path, &dirent);
	if (rc < 0) {
		LOG_ERR("FAIL: stat %s: %d", img_path, rc);
		return 0;
	}

	LOG_INF("%s img_path size %d", img_path, dirent.size);
	ret = dirent.size;
	(void)fs_close(&file);
#else
	LOG_ERR("No valid image");
#endif
	return ret;
}

int ap_read_img_from_storage(uint8_t *img_path, void *data, size_t *len)
{
	int ret;
#ifdef CONFIG_MCUMGR_GRP_FS
	/* Here demostrates how to retrieve image from littlefs */
	struct fs_dirent dirent;
	struct fs_file_t file;
	int rc;

	fs_file_t_init(&file);
	rc = fs_open(&file, img_path, FS_O_CREATE | FS_O_RDWR);
	if (rc < 0) {
		LOG_ERR("FAIL: open %s: %d", img_path, rc);
		return rc;
	}

	rc = fs_stat(img_path, &dirent);
	if (rc < 0) {
		LOG_ERR("FAIL: stat %s: %d", img_path, rc);
		goto out;
	}

	if (rc == 0 && dirent.type == FS_DIR_ENTRY_FILE && dirent.size == 0) {
		LOG_ERR("Image file: %s not found", img_path);
		ret = -ENXIO;
	}

	LOG_INF("%s img_path size %d", img_path, dirent.size);
	rc = fs_read(&file, data, dirent.size);
	if (rc < 0) {
		LOG_ERR("FAIL: read %s: [rd:%d]", img_path, rc);
		goto out;
	}

	*len = rc;
out:
	ret = fs_close(&file);
	if (ret < 0) {
		LOG_ERR("FAIL: close %s: %d", img_path, ret);
		return ret;
	}

	return (rc < 0 ? rc : 0);

#else
	LOG_ERR("No valid image");
	ret = -ENXIO;
	return ret;
#endif /* (CONFIG_MCUMGR_GRP_FS) */
}

static int esl_client_init(void)
{
	int err;
	struct bt_esl_client_init_param init;

	init.cb.ap_image_storage_init = ap_image_storage_init;
	init.cb.ap_read_img_from_storage = ap_read_img_from_storage;
	init.cb.ap_read_img_size_from_storage = ap_read_img_size_from_storage;
	err = bt_esl_client_init(&esl_client, &init);
	if (err) {
		LOG_ERR("ESL Client initialization failed (err %d)", err);
		return err;
	}

	LOG_INF("ESL Client module initialized");
	return err;
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_WRN("Pairing failed conn: %s, reason %d", addr, reason);
	bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

void bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
	char addr[BT_ADDR_STR_LEN];

	bt_addr_le_to_str(peer, addr, sizeof(addr));
	LOG_INF("Bond deleted for %s, id %u", addr, id);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {.cancel = auth_cancel};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {.pairing_complete = pairing_complete,
							       .pairing_failed = pairing_failed,
							       .bond_deleted = bond_deleted

};

extern uint8_t groups_per_button;
static uint8_t btn3_toggle;
static uint8_t btn3_group_toggle;
static uint8_t btn4_toggle;
static uint8_t btn4_group_toggle;
/**
 *  type 0x07 broadcast Display Img 0
 *  type 0x08 broadcast Display Img 1
 *  type 0X12 Tag 0~10 Display Img 0
 *  type 0X13 Tag 0~10 Display img 1
 **/
static uint8_t btn3_cmd_list[] = {0x7, 0x8, 0x12, 0x13};

/**
 * type 1 led 0 broadcast flashing
 * type 4 led 0 broadcast off
 */
static uint8_t btn4_cmd_list[] = {0x1, 0x4};
static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	uint32_t button = button_state & has_changed;

	if (button & DK_BTN1_MSK) {
		esl_c_scan(true, false);
	}

	if (button & DK_BTN2_MSK) {
#if defined(CONFIG_BT_ESL_TAG_STORAGE)
		bt_c_esl_unbond(-1);
		remove_all_tags_in_storage();
#endif
	}

	if (button & DK_BTN3_MSK) {
		esl_dummy_ap_ad_data(btn3_cmd_list[btn3_toggle % sizeof(btn3_cmd_list)],
				     btn3_group_toggle);
		printk("btn3_group_toggle %d\n", btn3_group_toggle);
		btn3_toggle++;
		btn3_group_toggle = (btn3_toggle / sizeof(btn3_cmd_list)) % groups_per_button;
	}

	if (button & DK_BTN4_MSK) {
		esl_dummy_ap_ad_data(btn4_cmd_list[btn4_toggle % sizeof(btn4_cmd_list)],
				     btn4_group_toggle);
		printk("btn4_group_toggle %d\n", btn4_group_toggle);
		btn4_toggle++;
		btn4_group_toggle = (btn4_toggle / sizeof(btn4_cmd_list)) % groups_per_button;
	}
}

int main(void)
{
	int err;
	uint16_t ctrl_version = 0;

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Failed to init dk buttons. %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_USB_DEVICE_STACK)) {
		err = usb_enable(NULL);
		if (err) {
			LOG_ERR("Failed to init usb %d", err);
		}
	}

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_ERR("Failed to register authorization callbacks. %d", err);
		return err;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		LOG_ERR("Failed to register authorization callbacks. %d", err);
		return err;
	}

#if defined(BT_ESL_AP_PTS)
	uint8_t pub_addr[] = {0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB};

	bt_ctlr_set_public_addr(pub_addr);
#endif /* CONFIG_BT_ESL_AP_PTS */

	err = bt_enable(NULL);
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = ble_ctrl_version_get(&ctrl_version);
	if (err != 0) {
		LOG_ERR("Failed to get controller version");
	}

	printk("Controller version: %d\n", ctrl_version);
	esl_client_init();
	int (*module_init[])(void) = {esl_client.cb.ap_image_storage_init};

	for (size_t i = 0; i < ARRAY_SIZE(module_init); i++) {
		err = (*module_init[i])();
		if (err) {
			LOG_ERR("init module %d (err %d)", i, err);
			return err;
		}
	}

	LOG_INF("Starting Bluetooth Central ESL example\n");
	for (;;) {
		/* Wait infinitely */
		k_sleep(K_FOREVER);
	}
}
