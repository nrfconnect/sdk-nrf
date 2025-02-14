/** @file
 *  @brief Bluetooth Mesh DFU Target role handler
 */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>

#include <zephyr/storage/flash_map.h>

#include <zephyr/dfu/mcuboot.h>

static struct bt_mesh_blob_io_flash *blob_flash_stream;
static enum bt_mesh_dfu_effect img_effect = BT_MESH_DFU_EFFECT_NONE;
struct mcuboot_img_sem_ver img_ver;

static struct {
	uint16_t company_id;
	uint8_t major;
	uint8_t minor;
	uint16_t revision;
	uint32_t build_num;
} __packed fwid;

static struct bt_mesh_dfu_img dfu_imgs[] = { {
	.fwid = &fwid,
	.fwid_len = sizeof(fwid),
} };

#if defined(CONFIG_BT_MESH_DFU_METADATA)
static size_t flash_area_size_get(uint8_t area_id)
{
	const struct flash_area *area;
	size_t fa_size;
	int err;

	err = flash_area_open(area_id, &area);
	if (err) {
		return 0;
	}

	fa_size = area->fa_size;
	flash_area_close(area);

	return fa_size;
}
#endif

static bool is_firmware_newer(struct mcuboot_img_sem_ver *new, struct mcuboot_img_sem_ver *cur)
{
	if (new->major > cur->major) {
		return true;
	} else if (new->major < cur->major) {
		return false;
	}

	if (new->minor > cur->minor) {
		return true;
	} else if (new->minor < cur->minor) {
		return false;
	}

	if (new->revision > cur->revision) {
		return true;
	} else if (new->revision < cur->revision) {
		return false;
	}

	if (new->build_num > cur->build_num) {
		return true;
	}

	return false;
}

static int dfu_meta_check(struct bt_mesh_dfu_srv *srv,
			  const struct bt_mesh_dfu_img *img,
			  struct net_buf_simple *metadata_raw,
			  enum bt_mesh_dfu_effect *effect)
{
#if defined(CONFIG_BT_MESH_DFU_METADATA)
	struct bt_mesh_dfu_metadata metadata;
	uint8_t key[16] = {};
	uint32_t hash;
	int err;

	err = bt_mesh_dfu_metadata_decode(metadata_raw, &metadata);
	if (err) {
		printk("Unable to decode metadata: %d\n", err);
		return -EINVAL;
	}

	printk("Received firmware metadata:\n");
	printk("\tVersion: %u.%u.%u+%u\n", metadata.fw_ver.major, metadata.fw_ver.minor,
	       metadata.fw_ver.revision, metadata.fw_ver.build_num);
	printk("\tSize: %u\n", metadata.fw_size);
	printk("\tCore Type: 0x%x\n", metadata.fw_core_type);

	if (metadata.fw_core_type & BT_MESH_DFU_FW_CORE_TYPE_APP) {
		printk("\tComposition data hash: 0x%x\n", metadata.comp_hash);
		printk("\tElements: %u\n", metadata.elems);
	}

	if (metadata.user_data_len > 0) {
		size_t len;
		uint8_t user_data[2 * (CONFIG_BT_MESH_DFU_METADATA_MAXLEN - 18) + 1];

		len = bin2hex(metadata.user_data, metadata.user_data_len, user_data,
			      (sizeof(user_data) - 1));
		user_data[len] = '\0';
		printk("\tUser data: %s\n", user_data);
	}

	printk("\tUser data length: %u\n", metadata.user_data_len);

	if (!is_firmware_newer((struct mcuboot_img_sem_ver *) &metadata.fw_ver, &img_ver)) {
		printk("New firmware version is older\n");
		return -EINVAL;
	}

	if (!(metadata.fw_core_type & BT_MESH_DFU_FW_CORE_TYPE_APP)) {
		printk("Only application core firmware is supported by the sample\n");
		return -EINVAL;
	}

	if (flash_area_size_get(FIXED_PARTITION_ID(slot0_partition)) < metadata.fw_size ||
	    flash_area_size_get(FIXED_PARTITION_ID(slot1_partition)) < metadata.fw_size) {
		printk("New firmware won't fit into flash.");
		return -EINVAL;
	}

	err = bt_mesh_dfu_metadata_comp_hash_local_get(key, &hash);
	if (err) {
		printk("Failed to compute composition data hash: %d\n", err);
		return -EINVAL;
	}

	printk("Current composition data hash: 0x%x\n", hash);

	if (hash == metadata.comp_hash) {
		img_effect = BT_MESH_DFU_EFFECT_NONE;
	} else {
		img_effect = BT_MESH_DFU_EFFECT_UNPROV;
	}

#else
	img_effect = BT_MESH_DFU_EFFECT_UNPROV;
#endif /* CONFIG_BT_MESH_DFU_METADATA */

	printk("Metadata check succeeded, effect: %d\n", img_effect);

	*effect = img_effect;

	return 0;
}

static int dfu_start(struct bt_mesh_dfu_srv *srv,
		     const struct bt_mesh_dfu_img *img,
		     struct net_buf_simple *metadata,
		     const struct bt_mesh_blob_io **io)
{
	printk("Firmware upload started\n");

	*io = &blob_flash_stream->io;

	return 0;
}

static void dfu_end(struct bt_mesh_dfu_srv *srv, const struct bt_mesh_dfu_img *img, bool success)
{
	printk("Firmware upload %s\n", success ? "succeeded" : "failed");

	if (!success) {
		return;
	}

	/* TODO: Add verification code here. */

	bt_mesh_dfu_srv_verified(srv);
}

static int dfu_recover(struct bt_mesh_dfu_srv *srv,
		       const struct bt_mesh_dfu_img *img,
		       const struct bt_mesh_blob_io **io)
{
	printk("Recovering the firmware upload\n");

	/* TODO: Need to recover the effect. */

#if defined(CONFIG_BT_MESH_DFD_SRV)
	return -ENOTSUP;
#else
	*io = &blob_flash_stream->io;

	return 0;
#endif
}

static void do_reboot(struct k_work *work)
{
	sys_reboot(SYS_REBOOT_WARM);
}

static int dfu_apply(struct bt_mesh_dfu_srv *srv, const struct bt_mesh_dfu_img *img)
{
	static struct k_work_delayable pending_reboot;

	printk("Applying the new firmware\n");

	boot_request_upgrade(BOOT_UPGRADE_TEST);

	if (img_effect == BT_MESH_DFU_EFFECT_UNPROV) {
		bt_mesh_reset();

		printk("Pending the mesh settings to cleared before rebooting...\n");

		/* Let the mesh reset its settings before rebooting the device. */
		k_work_init_delayable(&pending_reboot, do_reboot);
		k_work_schedule(&pending_reboot, K_MSEC(1));
	} else {
		/* No need to unprovision device. Reboot immediately. */
		sys_reboot(SYS_REBOOT_WARM);
	}

	return 0;
}

static const struct bt_mesh_dfu_srv_cb dfu_handlers = {
	.check = dfu_meta_check,
	.start = dfu_start,
	.end = dfu_end,
	.apply = dfu_apply,
	.recover = dfu_recover,
};

struct bt_mesh_dfu_srv dfu_srv =
	BT_MESH_DFU_SRV_INIT(&dfu_handlers, dfu_imgs, ARRAY_SIZE(dfu_imgs));

static void image_version_load(void)
{
	struct mcuboot_img_header img_header;
	int err;

	err = boot_read_bank_header(FIXED_PARTITION_ID(slot0_partition), &img_header,
				    sizeof(img_header));
	if (err) {
		printk("Failed to read image header: %d\n", err);
		return;
	}

	img_ver = img_header.h.v1.sem_ver;

	fwid.company_id = sys_cpu_to_le16(CONFIG_BT_COMPANY_ID);
	fwid.major = img_ver.major;
	fwid.minor = img_ver.minor;
	fwid.revision = sys_cpu_to_le16(img_ver.revision);
	fwid.build_num = sys_cpu_to_le32(img_ver.build_num);

	printk("Current image version: %u.%u.%u+%u\n", img_ver.major, img_ver.minor,
	       img_ver.revision, img_ver.build_num);
}

int dfu_target_init(struct bt_mesh_blob_io_flash *flash_stream)
{
	blob_flash_stream = flash_stream;

	image_version_load();

	return 0;
}

void dfu_target_image_confirm(void)
{
	int err;

	err = boot_write_img_confirmed();
	if (err) {
		printk("Failed to confirm image: %d\n", err);
	}

	/* Switch DFU Server state to the Idle state if it was in the Applying state. */
	bt_mesh_dfu_srv_applied(&dfu_srv);
}
