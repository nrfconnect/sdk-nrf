/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include "esl_client.h"
#include "esl_internal.h"
#include "esl_client_tag_storage.h"

LOG_MODULE_DECLARE(esl_c);

int save_tag_in_storage(const struct bt_esl_chrc_data *tag)
{
	struct fs_dirent dirent;
	struct fs_file_t file;
	int rc, ret;
	char fname[CONFIG_MCUMGR_GRP_FS_PATH_LEN];
	char addr[BT_ADDR_LE_STR_LEN];

	/* Save tag with ble addr */
	bt_addr_to_str(&tag->ble_addr.a, addr, BT_ADDR_LE_STR_LEN);
	snprintk(fname, CONFIG_MCUMGR_GRP_FS_PATH_LEN, "%s/%s", TAG_BLE_ADDR_ROOT, addr);
	fs_file_t_init(&file);
	rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (rc < 0) {
		LOG_ERR("FAIL: open %s: %d", fname, rc);
		ret = -ENXIO;
		goto out;
	}

	rc = fs_stat(fname, &dirent);
	if (rc < 0) {
		LOG_ERR("FAIL: stat %s: %d", fname, rc);
		ret = -ENOENT;
		goto out;
	}

	if (rc == 0 && dirent.type == FS_DIR_ENTRY_FILE && dirent.size == 0) {
		LOG_INF("Tag file: %s not found, create one", fname);
	}

	fs_write(&file, &tag->esl_addr, ESL_ADDR_LEN);
	fs_write(&file, tag->esl_rsp_key.key_v, EAD_KEY_MATERIAL_LEN);
	fs_write(&file, &tag->ble_addr.type, sizeof(uint8_t));
	fs_close(&file);

	/* Save tag with esl addr */
	snprintk(fname, CONFIG_MCUMGR_GRP_FS_PATH_LEN, "%s/%04x", TAG_ESL_ADDR_ROOT, tag->esl_addr);
	fs_file_t_init(&file);
	rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (rc < 0) {
		LOG_ERR("FAIL: open %s: %d", fname, rc);
		ret = -ENXIO;
		goto out;
	}

	rc = fs_stat(fname, &dirent);
	if (rc < 0) {
		LOG_ERR("FAIL: stat %s: %d", fname, rc);
		ret = -ENOENT;
		goto out;
	}

	if (rc == 0 && dirent.type == FS_DIR_ENTRY_FILE && dirent.size == 0) {
		LOG_INF("Tag file: %s not found, create one", fname);
	}

	fs_write(&file, tag->ble_addr.a.val, BT_ADDR_SIZE);
	fs_write(&file, &tag->ble_addr.type, sizeof(uint8_t));
	fs_write(&file, tag->esl_rsp_key.key_v, EAD_KEY_MATERIAL_LEN);
	fs_write(&file, &tag->dis_pnp_id.pnp_vid, sizeof(uint16_t));
	fs_write(&file, &tag->dis_pnp_id.pnp_pid, sizeof(uint16_t));
out:
	ret = fs_close(&file);
	if (ret < 0) {
		LOG_ERR("FAIL: close %s: %d", fname, ret);
	}

	return ret;
}

int load_tag_in_storage(const bt_addr_le_t *ble_addr, struct bt_esl_chrc_data *tag)
{
	struct fs_dirent dirent;
	struct fs_file_t file;
	int rc;
	char fname[CONFIG_MCUMGR_GRP_FS_PATH_LEN];
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_to_str(&ble_addr->a, addr, BT_ADDR_LE_STR_LEN);
	snprintk(fname, CONFIG_MCUMGR_GRP_FS_PATH_LEN, "%s/%s", TAG_BLE_ADDR_ROOT, addr);
	fs_file_t_init(&file);
	rc = fs_open(&file, fname, FS_O_READ);
	if (rc < 0) {
		LOG_WRN("FAIL: open %s: %d", fname, rc);
		goto out;
	}

	rc = fs_stat(fname, &dirent);
	if (rc < 0) {
		LOG_WRN("FAIL: stat %s: %d", fname, rc);
		goto out;
	}

	if (rc == 0 && dirent.type == FS_DIR_ENTRY_FILE && dirent.size == 0) {
		LOG_INF("Tag file: %s empty found. New tag", fname);
		rc = -ENOENT;
		goto out;
	}

	fs_read(&file, &tag->esl_addr, ESL_ADDR_LEN);
	fs_read(&file, tag->esl_rsp_key.key_v, EAD_KEY_MATERIAL_LEN);
	if (tag->connect_from == ACL_FROM_SCAN) {
		tag->past_needed = true;
	}

	printk("#TAG:0x%04x,%s\n#RSP_KEY:0x", tag->esl_addr, addr);
	print_hex(tag->esl_rsp_key.key_v, EAD_KEY_MATERIAL_LEN);
out:
	rc = fs_close(&file);
	if (rc < 0) {
		LOG_ERR("FAIL: close %s: %d", fname, rc);
	}

	return rc;
}

int find_tag_in_storage_with_esl_addr(uint16_t esl_addr, bt_addr_le_t *ble_addr)
{
	struct fs_dirent dirent;
	struct fs_file_t file;
	int rc;
	char fname[CONFIG_MCUMGR_GRP_FS_PATH_LEN];

	snprintk(fname, CONFIG_MCUMGR_GRP_FS_PATH_LEN, "%s/%04x", TAG_ESL_ADDR_ROOT, esl_addr);
	fs_file_t_init(&file);
	rc = fs_open(&file, fname, FS_O_READ);
	if (rc < 0) {
		LOG_ERR("FAIL: open %s: %d", fname, rc);
		goto out;
	}

	rc = fs_stat(fname, &dirent);
	if (rc < 0) {
		LOG_ERR("FAIL: stat %s: %d", fname, rc);
		goto out;
	}

	if (rc == 0 && dirent.type == FS_DIR_ENTRY_FILE && dirent.size == 0) {
		LOG_INF("Tag file: %s empty found. New tag", fname);
		rc = -ENOENT;
		goto out;
	}

	fs_read(&file, ble_addr->a.val, BT_ADDR_SIZE);
	fs_read(&file, &ble_addr->type, sizeof(uint8_t));
out:
	(void)fs_close(&file);
	if (rc < 0) {
		LOG_ERR("FAIL: close %s: %d", fname, rc);
	}

	return rc;
}

int remove_all_tags_in_storage(void)
{
	int rc;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	char fname[CONFIG_MCUMGR_GRP_FS_PATH_LEN];
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	fs_dir_t_init(&dirp);
	/* Remove BLE_ADDR*/
	rc = fs_opendir(&dirp, TAG_BLE_ADDR_ROOT);
	if (rc) {
		LOG_WRN("Error opening dir %s (rc %d)", TAG_BLE_ADDR_ROOT, rc);
		goto esl;
	}

	LOG_INF("Recursive remove files in dir %s ...\n", TAG_BLE_ADDR_ROOT);
	for (;;) {
		/* Verify fs_readdir() */
		rc = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (rc || entry.name[0] == 0) {
			if (rc < 0) {
				LOG_INF("dir is empty could be removed (%d)", rc);
				rc = fs_unlink(TAG_BLE_ADDR_ROOT);
				if (rc < 0) {
					LOG_ERR("FAIL: remove folder %s: %d", TAG_BLE_ADDR_ROOT,
						rc);
				}
			}

			break;
		}

		if (entry.type == FS_DIR_ENTRY_FILE) {
			LOG_INF("[FILE] %s ", entry.name);
			snprintk(fname, CONFIG_MCUMGR_GRP_FS_PATH_LEN, "%s/%s", TAG_BLE_ADDR_ROOT,
				 entry.name);
			rc = fs_unlink(fname);
			if (rc < 0) {
				LOG_ERR("FAIL: remove file %s: %d", fname, rc);
			}
		}
	}

	fs_closedir(&dirp);
esl:
	fs_dir_t_init(&dirp);
	/* Remove ESL_ADDR*/
	rc = fs_opendir(&dirp, TAG_ESL_ADDR_ROOT);
	if (rc) {
		LOG_WRN("Error opening dir %s (rc %d)", TAG_ESL_ADDR_ROOT, rc);
		return rc;
	}

	LOG_INF("Recursive remove files in dir %s ...\n", TAG_ESL_ADDR_ROOT);
	for (;;) {
		/* Verify fs_readdir() */
		rc = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (rc || entry.name[0] == 0) {
			if (rc < 0) {
				LOG_INF("dir is empty could be removed (%d)", rc);
				rc = fs_unlink(TAG_ESL_ADDR_ROOT);
				if (rc < 0) {
					LOG_ERR("FAIL: remove folder %s: %d", TAG_ESL_ADDR_ROOT,
						rc);
				}
			}

			break;
		}

		if (entry.type == FS_DIR_ENTRY_FILE) {
			LOG_INF("[FILE] %s ", entry.name);
			snprintk(fname, CONFIG_MCUMGR_GRP_FS_PATH_LEN, "%s/%s", TAG_ESL_ADDR_ROOT,
				 entry.name);
			rc = fs_unlink(fname);
			if (rc < 0) {
				LOG_ERR("FAIL: remove file %s: %d", fname, rc);
			}
		}
	}

	fs_closedir(&dirp);
	esl_c_obj->esl_addr_start = (CONFIG_ESL_CLIENT_DEFAULT_GROUP_ID & 0xff) << 8 |
				    (CONFIG_ESL_CLIENT_DEFAULT_ESL_ID & 0xff);
	return rc;
}

/* group_id RFU bit on means read all id */
int load_all_tags_in_storage(uint8_t group_id)
{
	int rc;
	struct fs_dir_t dirp;
	struct fs_dirent dirent;
	struct fs_file_t file;
	static struct fs_dirent entry;
	char fname[CONFIG_MCUMGR_GRP_FS_PATH_LEN];
	uint16_t esl_addr;
	uint8_t esl_id_offset;
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	fs_dir_t_init(&dirp);
	/* Verify fs_opendir() */
	rc = fs_opendir(&dirp, TAG_ESL_ADDR_ROOT);
	if (rc) {
		LOG_DBG("Error opening dir %s (rc %d)", TAG_ESL_ADDR_ROOT, rc);
		return rc;
	}

	LOG_INF("Recursive open files in dir %s ...\n", TAG_ESL_ADDR_ROOT);
	for (;;) {
		/* Verify fs_readdir() */
		rc = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (rc || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_FILE) {
			snprintk(fname, CONFIG_MCUMGR_GRP_FS_PATH_LEN, "%s/%s", TAG_ESL_ADDR_ROOT,
				 entry.name);
			esl_addr = strtol(entry.name, NULL, 16);
			esl_c_obj->esl_addr_start = MAX(esl_c_obj->esl_addr_start, esl_addr + 1);
			esl_id_offset =
				MIN(LOW_BYTE(esl_addr) - CONFIG_ESL_CLIENT_DEFAULT_ESL_ID, 0);
			/* Skip if not in group_id */
			if (!CHECK_BIT(group_id, ESL_GROUPID_RFU_BIT) ||
			    (GROUPID_BYTE(esl_addr) != group_id)) {
				continue;
			}

			LOG_INF("[FILE] %s ", entry.name);
			fs_file_t_init(&file);
			rc = fs_open(&file, fname, FS_O_READ);
			if (rc < 0) {
				LOG_DBG("FAIL: open %s: %d", fname, rc);
			}

			rc = fs_stat(fname, &dirent);
			if (rc < 0) {
				LOG_DBG("FAIL: stat %s: %d", fname, rc);
			}

			if (rc == 0 && dirent.type == FS_DIR_ENTRY_FILE && dirent.size == 0) {
				LOG_INF("Tag file: %s empty found. New tag", fname);
			}

			fs_seek(&file, sizeof(bt_addr_le_t), FS_SEEK_SET);
			fs_read(&file,
				esl_c_obj->sync_buf[GROUPID_BYTE(esl_addr)]
					.rsp_buffer[esl_id_offset]
					.rsp_key.key_v,
				EAD_KEY_MATERIAL_LEN);
			esl_c_obj->esl_addr_start = MAX(esl_c_obj->esl_addr_start, esl_addr + 1);
			fs_close(&file);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return rc;
}

/* type 0 list esl, type 1 list ble*/
int list_tags_in_storage(uint8_t type)
{
	int rc;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	uint8_t path[CONFIG_MCUMGR_GRP_FS_PATH_LEN];

	if (type == 0) {
		snprintk(path, CONFIG_MCUMGR_GRP_FS_PATH_LEN, "%s", TAG_ESL_ADDR_ROOT);
	} else {
		snprintk(path, CONFIG_MCUMGR_GRP_FS_PATH_LEN, "%s", TAG_BLE_ADDR_ROOT);
	}

	fs_dir_t_init(&dirp);
	/* Verify fs_opendir() */
	rc = fs_opendir(&dirp, path);
	if (rc) {
		LOG_WRN("Error opening dir %s (rc %d)", path, rc);
		return rc;
	}

	if (type == 0) {
		printk("#TAG_IN_ESL:\n");
	} else {
		printk("#TAG_IN_BLE:\n");
	}

	LOG_INF("Recursive open files in dir %s ...\n", path);
	for (;;) {
		/* Verify fs_readdir() */
		rc = fs_readdir(&dirp, &entry);
		/* entry.name[0] == 0 means end-of-dir */
		if (rc || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_FILE) {
			LOG_INF("[FILE] %s ", entry.name);
			printk("%s\n", entry.name);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return rc;
}

int remove_tag_in_storage(uint16_t esl_addr, const bt_addr_le_t *peer_addr)
{
	int rc;
	struct fs_file_t file;
	char fname[CONFIG_MCUMGR_GRP_FS_PATH_LEN];
	char addr[BT_ADDR_STR_LEN];
	bt_addr_le_t ble_addr;

	if (peer_addr == NULL) {
		/* No address assign use esl_addr*/
		rc = find_tag_in_storage_with_esl_addr(esl_addr, &ble_addr);
		if (rc) {
			LOG_WRN("Tag is not found in BLE_ADDR storage");
			return rc;
		}

		bt_addr_to_str(&ble_addr.a, addr, BT_ADDR_STR_LEN);
		LOG_INF("find addr %s", addr);
	}

	fs_file_t_init(&file);
	/* Remove BLE_ADDR*/
	snprintk(fname, CONFIG_MCUMGR_GRP_FS_PATH_LEN, "%s/%s", TAG_BLE_ADDR_ROOT, addr);
	rc = fs_unlink(fname);
	if (rc < 0) {
		LOG_ERR("FAIL: remove file %s: %d", fname, rc);
	}

	/* Remove ESL_ADDR*/
	snprintk(fname, CONFIG_MCUMGR_GRP_FS_PATH_LEN, "%s/%04x", TAG_ESL_ADDR_ROOT, esl_addr);
	rc = fs_unlink(fname);
	if (rc < 0) {
		LOG_ERR("FAIL: remove file %s: %d", fname, rc);
	}

	return rc;
}
