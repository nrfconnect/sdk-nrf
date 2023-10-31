/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <pm_config.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt_client.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt_client.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_smp.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(dfu_target_smp, CONFIG_DFU_TARGET_LOG_LEVEL);

#define MCUBOOT_HEADER_MAGIC 0x96f3b83d

struct upload_progress {
	size_t image_size;
	size_t offset;
	uint32_t image_num;
	int smp_status;
	int smp_echo;
};

static struct upload_progress upload_state;
static struct mcumgr_image_data image_info[CONFIG_DFU_TARGET_SMP_IMAGE_LIST_SIZE];
static int image_count;
static bool recovery_mode_active;
static dfu_target_reset_cb_t dfu_target_reset_cb;
static struct smp_client_object smp_client;
static struct img_mgmt_client img_gr_client;
static struct os_mgmt_client os_gr_client;

static const char dfu_smp_echo_str[] = "Recovery";

static int dfu_target_smp_recovery_enable(void);

static int img_state_res_update(struct mcumgr_image_state *res_buf)
{
	if (res_buf->status) {
		LOG_ERR("Image state response command fail, err:%d", res_buf->status);
		image_count = 0;
	} else {
		image_count = res_buf->image_list_length;
	}

	return res_buf->status;
}

static int img_upload_res_update(struct mcumgr_image_upload *res_buf)
{
	if (res_buf->status) {
		LOG_ERR("Image Upload command fail err:%d", res_buf->status);
	} else {
		upload_state.offset = res_buf->image_upload_offset;
	}
	return res_buf->status;
}

int dfu_target_smp_client_init(void)
{
	int rc;

	rc = smp_client_object_init(&smp_client, SMP_SERIAL_TRANSPORT);
	if (rc) {
		return rc;
	}

	img_mgmt_client_init(&img_gr_client, &smp_client, CONFIG_DFU_TARGET_SMP_IMAGE_LIST_SIZE,
			     image_info);
	os_mgmt_client_init(&os_gr_client, &smp_client);

	image_count = 0;

	return rc;
}

bool dfu_target_smp_identify(const void *const buf)
{
	/* MCUBoot headers starts with 4 byte magic word */
	return *((const uint32_t *)buf) == MCUBOOT_HEADER_MAGIC;
}

int dfu_target_smp_init(size_t file_size, int img_num, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);
	if (img_num < 0) {
		return -ENOENT;
	}

	if (dfu_target_reset_cb) {
		upload_state.image_num = 2;
	} else {
		upload_state.image_num = (uint32_t)img_num;
	}

	upload_state.image_size = file_size;
	upload_state.offset = 0;

	return img_mgmt_client_upload_init(&img_gr_client, file_size, upload_state.image_num, NULL);
}

int dfu_target_smp_offset_get(size_t *out)
{
	*out = upload_state.offset;
	return 0;
}

static int dfu_target_smp_enable(void)
{
	if (dfu_target_reset_cb && !recovery_mode_active) {
		if (dfu_target_reset_cb()) {
			LOG_ERR("SMP server reset fail");
			return -ENOENT;
		}

		if (dfu_target_smp_recovery_enable()) {
			LOG_ERR("SMP Server Recovery mode fail");
			dfu_target_reset_cb();
			return -ENOENT;
		}
		recovery_mode_active = true;
	}
	return 0;
}

int dfu_target_smp_write(const void *const buf, size_t len)
{
	int err = 0;
	struct mcumgr_image_upload upload_res;

	err = dfu_target_smp_enable();
	if (err) {
		return err;
	}

	err = img_mgmt_client_upload(&img_gr_client, buf, len, &upload_res);
	if (err) {
		return err;
	}

	return img_upload_res_update(&upload_res);
}

int dfu_target_smp_done(bool successful)
{
	int rc;
	struct mcumgr_image_state res_buf;

	rc = dfu_target_smp_enable();
	if (rc) {
		return rc;
	}

	if (successful) {
		/* Read Image list */
		LOG_INF("MCUBoot SMP image download ready.");
		image_count = 0;
		rc = img_mgmt_client_state_read(&img_gr_client, &res_buf);
		if (rc) {
			goto end;
		}
		rc = img_state_res_update(&res_buf);
	} else {
		LOG_INF("MCUBoot SMP image upgrade aborted %d", upload_state.image_num);
	}
end:
	upload_state.image_size = 0;
	upload_state.offset = 0;

	return rc;
}

static struct mcumgr_image_data *discover_secondary_image_info(void)
{
	if (!image_count) {
		return NULL;
	}

	for (int i = 0; i < image_count; i++) {
		if (image_info[i].slot_num == 0) {
			continue;
		}
		return &image_info[i];
	}

	return NULL;
}

int dfu_target_smp_schedule_update(int img_num)
{
	int err = 0;
	struct mcumgr_image_state res_buf;
	struct mcumgr_image_data *secondary_image;

	/* Discover always Secondary image */
	secondary_image = discover_secondary_image_info();
	/* Test functionality here */
	if (!secondary_image) {
		LOG_ERR("Secondary Image info not available");
		return -ENOENT;
	}

	err = dfu_target_smp_enable();
	if (err) {
		return err;
	}

	LOG_INF("MCUBoot image-%d upgrade scheduled. Reset device to apply",
		secondary_image->slot_num);

	err = img_mgmt_client_state_write(&img_gr_client, secondary_image->hash, false, &res_buf);
	if (err) {
		image_count = 0;
		return err;
	}

	return img_state_res_update(&res_buf);
}

int dfu_target_smp_reset(void)
{
	int rc;

	upload_state.image_size = 0;
	upload_state.offset = 0;

	rc = dfu_target_smp_enable();
	if (rc) {
		return rc;
	}

	if (dfu_target_reset_cb) {
		/* MCUBoot not support Erase yet */
		rc = 0;
	} else {
		rc = img_mgmt_client_erase(&img_gr_client, upload_state.image_num);
	}

	return rc;
}

int dfu_target_smp_reboot(void)
{
	int rc;

	upload_state.image_size = 0;
	upload_state.offset = 0;

	rc = dfu_target_smp_enable();
	if (rc) {
		return rc;
	}

	rc = os_mgmt_client_reset(&os_gr_client);
	LOG_DBG("OS Reset command status:%d", rc);
	if (rc) {
		return rc;
	}
	recovery_mode_active = false;

	return 0;
}

int dfu_target_smp_confirm_image(void)
{
	int rc;
	struct mcumgr_image_state res_buf;

	if (dfu_target_reset_cb) {
		/* Recovery mode can't confirm image  Application need to handle that */
		return -EACCES;
	}

	LOG_INF("Confirm Image");
	rc = img_mgmt_client_state_write(&img_gr_client, NULL, true, &res_buf);
	if (rc) {
		LOG_INF("Confirm fault err:%d", rc);
		return rc;
	}
	return img_state_res_update(&res_buf);
}

int dfu_target_smp_recovery_mode_enable(dfu_target_reset_cb_t cb)
{
	dfu_target_reset_cb = cb;
	return 0;
}

int dfu_target_smp_image_list_get(struct mcumgr_image_state *res_buf)
{
	int err = 0;

	err = dfu_target_smp_enable();
	if (err) {
		return err;
	}
	err = img_mgmt_client_state_read(&img_gr_client, res_buf);
	if (err == 0) {
		img_state_res_update(res_buf);
	}

	return err;
}

static int dfu_target_smp_recovery_enable(void)
{
	int rc;

	rc = os_mgmt_client_echo(&os_gr_client, dfu_smp_echo_str);
	LOG_DBG("Recovery activate status %d", rc);

	return rc;
}
