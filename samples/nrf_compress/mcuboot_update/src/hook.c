/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>
#include <zephyr/logging/log.h>
#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <bootutil/image.h>

LOG_MODULE_REGISTER(mcuboot_update_sample, LOG_LEVEL_DBG);

#define SECONDARY_SLOT 1

#ifndef CONFIG_MCUMGR_GRP_IMG_FRUGAL_LIST
#define ZCBOR_ENCODE_FLAG(zse, label, value) \
	(zcbor_tstr_put_lit(zse, label) && zcbor_bool_put(zse, value))
#else
/* In "frugal" lists flags are added to response only when they evaluate to true */
/* Note that value is evaluated twice! */
#define ZCBOR_ENCODE_FLAG(zse, label, value) (!(value) || \
	 (zcbor_tstr_put_lit(zse, label) && zcbor_bool_put(zse, (value))))
#endif

static struct mgmt_callback image_slot_callback;
static struct k_work slot_output_work;

static void slot_output_handler(struct k_work *work)
{
	uint32_t slot_flags;
	int rc;

	rc = img_mgmt_read_info(SECONDARY_SLOT, NULL, NULL, &slot_flags);

	if (!rc) {
		slot_flags &= IMAGE_F_COMPRESSED_LZMA2 | IMAGE_F_COMPRESSED_ARM_THUMB_FLT;

		if (slot_flags == (IMAGE_F_COMPRESSED_LZMA2 | IMAGE_F_COMPRESSED_ARM_THUMB_FLT)) {
			LOG_INF(
			"Secondary slot image is LZMA2 compressed with ARM thumb filter applied");
		} else if (slot_flags & IMAGE_F_COMPRESSED_LZMA2) {
			LOG_INF("Secondary slot image is LZMA2 compressed");
		} else {
			LOG_INF("Secondary slot image is uncompressed");
		}
	} else {
		LOG_ERR("Failed to read slot info: %d", rc);
	}
}

static enum mgmt_cb_return image_slot_state_cb(uint32_t event, enum mgmt_cb_return prev_status,
					       int32_t *rc, uint16_t *group, bool *abort_more,
					       void *data, size_t data_size)
{
	if (event == MGMT_EVT_OP_IMG_MGMT_IMAGE_SLOT_STATE) {
		/* Append a field indicating if the image in the slot is compressed */
		struct img_mgmt_state_slot_encode *slot_data =
						(struct img_mgmt_state_slot_encode *)data;

		*slot_data->ok = ZCBOR_ENCODE_FLAG(slot_data->zse, "compressed",
						  (slot_data->flags & IMAGE_F_COMPRESSED_LZMA2));
	} else if (event == MGMT_EVT_OP_IMG_MGMT_DFU_PENDING) {
		(void)k_work_submit(&slot_output_work);
	}

	return MGMT_CB_OK;
}

static int setup_slot_hook(void)
{
	/* Setup hook for when img mgmt image slot state is used and DFU pending callback */
	image_slot_callback.callback = image_slot_state_cb;
	image_slot_callback.event_id = MGMT_EVT_OP_IMG_MGMT_IMAGE_SLOT_STATE |
				       MGMT_EVT_OP_IMG_MGMT_DFU_PENDING;
	mgmt_callback_register(&image_slot_callback);
	k_work_init(&slot_output_work, slot_output_handler);

#if defined(CONFIG_OUTPUT_BOOT_MESSAGE)
	LOG_INF("Dummy message indicating new firmware is running...");
#endif

	return 0;
}

SYS_INIT(setup_slot_hook, APPLICATION, 0);
