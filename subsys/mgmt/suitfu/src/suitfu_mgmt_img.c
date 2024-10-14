/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>
#include <string.h>
#include <stdio.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>

#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
#include <sdfw/sdfw_services/suit_service.h>
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include "suitfu_mgmt_priv.h"
#include <suit_envelope_info.h>

#ifdef CONFIG_SUIT_CACHE_RW
#include <suit_dfu_cache_rw.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(suitfu_mgmt, CONFIG_MGMT_SUITFU_LOG_LEVEL);

#include <dfu/suit_dfu.h>

#define ENCODE_FLAG(zse, flag, value) (zcbor_tstr_put_lit(zse, flag) && zcbor_bool_put(zse, value))

/** Represents an individual upload request. */
typedef struct {
	uint32_t image; /* 0 by default. */
	size_t off;	/* SIZE_MAX if unspecified. */
	size_t size;	/* SIZE_MAX if unspecified. */
	struct zcbor_string img_data;
	struct zcbor_string data_sha;
	bool upgrade; /* Only allow greater version numbers. */
} suitfu_mgmt_image_upload_req_t;

static int suitfu_mgmt_img_upload(struct smp_streamer *ctx)
{
	zcbor_state_t *zsd = ctx->reader->zs;
	zcbor_state_t *zse = ctx->writer->zs;
	static uint32_t image_id;
	static size_t image_size;
	static size_t offset_in_image;

	static struct suit_nvm_device_info device_info;

	size_t decoded = 0;
	suitfu_mgmt_image_upload_req_t req = {
		.off = SIZE_MAX,
		.size = SIZE_MAX,
		.img_data = {0},
		.data_sha = {0},
		.upgrade = false,
		.image = 0,
	};

	struct zcbor_map_decode_key_val image_upload_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(image, zcbor_uint32_decode, &req.image),
		ZCBOR_MAP_DECODE_KEY_VAL(data, zcbor_bstr_decode, &req.img_data),
		ZCBOR_MAP_DECODE_KEY_VAL(len, zcbor_size_decode, &req.size),
		ZCBOR_MAP_DECODE_KEY_VAL(off, zcbor_size_decode, &req.off),
		ZCBOR_MAP_DECODE_KEY_VAL(sha, zcbor_bstr_decode, &req.data_sha),
		ZCBOR_MAP_DECODE_KEY_VAL(upgrade, zcbor_bool_decode, &req.upgrade)};

	if (zcbor_map_decode_bulk(zsd, image_upload_decode, ARRAY_SIZE(image_upload_decode),
				  &decoded) != 0) {
		LOG_ERR("Decoding image upload request failed");
		return MGMT_ERR_EINVAL;
	}

	if (req.off == 0) {
		LOG_INF("New image transfer started (image number: %d)", req.image);
		image_size = 0;
		offset_in_image = 0;

		if (req.size == 0) {
			LOG_ERR("Candidate image empty");
			return MGMT_ERR_EINVAL;
		}

		if (req.image == 0) {
			suit_plat_err_t err = suit_dfu_partition_device_info_get(&device_info);

			if (err != SUIT_PLAT_SUCCESS) {
				LOG_ERR("DFU Partition not found");
				return MGMT_ERR_ENOENT;
			}

		} else {
#ifdef CONFIG_SUIT_CACHE_RW
			uint32_t cache_pool_id = req.image - 1;
			suit_plat_err_t err =
				suit_dfu_cache_rw_device_info_get(cache_pool_id, &device_info);

			if (err != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Cache pool %d not found", cache_pool_id);
				return MGMT_ERR_ENOENT;
			}
#else
			return MGMT_ERR_ENOENT;
#endif
		}

		if (req.size > device_info.partition_size) {
			LOG_ERR("Image too large: %d bytes. Partition size: %d bytes", req.size,
				device_info.partition_size);
			return MGMT_ERR_ENOMEM;
		}

		int rc = suitfu_mgmt_erase(&device_info, req.size);

		if (rc != MGMT_ERR_EOK) {
			LOG_ERR("Erasing partition failed");
			return rc;
		}

		image_id = req.image;
		image_size = req.size;
	}

	bool last = false;

	if (image_size == 0) {
		LOG_ERR("No transfer in progress");
		return MGMT_ERR_EBADSTATE;
	}

	if (req.off + req.img_data.len > image_size) {
		LOG_ERR("Image boundaries reached");
		image_size = 0;
		return MGMT_ERR_ENOMEM;
	}

	if (req.off != offset_in_image) {
		LOG_WRN("Wrong offset in image, expected: %p, received: %p",
			(void *)offset_in_image, (void *)req.off);
		if (zcbor_tstr_put_lit(zse, "rc") && zcbor_int32_put(zse, MGMT_ERR_EUNKNOWN) &&
		    zcbor_tstr_put_lit(zse, "off") && zcbor_size_put(zse, offset_in_image)) {
			return MGMT_ERR_EOK;
		}
	}

	if ((req.off + req.img_data.len) == image_size) {
		last = true;
	}

	int rc = suitfu_mgmt_write(&device_info, req.off, req.img_data.value, req.img_data.len,
				   last);

	if (rc == MGMT_ERR_EOK) {

		offset_in_image += req.img_data.len;
		if (last) {

			if (image_id == 0) {
				LOG_INF("Candidate envelope stored");
				rc = suitfu_mgmt_candidate_envelope_stored();
			} else {
				uint32_t cache_pool_id = image_id - 1;

				LOG_INF("Cache pool %d updated with RAW image", cache_pool_id);
			}

			image_size = 0;
		}
	}

	if (zcbor_tstr_put_lit(zse, "rc") && zcbor_int32_put(zse, rc) &&
	    zcbor_tstr_put_lit(zse, "off") && zcbor_size_put(zse, offset_in_image)) {
		return MGMT_ERR_EOK;
	}

	return MGMT_ERR_EMSGSIZE;
}

static int suitfu_mgmt_img_state_read(struct smp_streamer *ctx)
{
	zcbor_state_t *zse = ctx->writer->zs;
	char installed_vers_str[IMG_MGMT_VER_MAX_STR_LEN] = "<\?\?\?>";
	uint8_t hash[IMG_MGMT_HASH_LEN] = {0};
	struct zcbor_string zhash = {.value = hash, .len = IMG_MGMT_HASH_LEN};
	bool ok;

	/* Let's assume we have 1 'image' with slot 0 that represents installed
	 * envelope.
	 */
	ok = zcbor_tstr_put_lit(zse, "images") && zcbor_list_start_encode(zse, 2);

#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
	unsigned int seq_num = 0;
	int alg_id = 0;
	suit_ssf_manifest_class_info_t class_info = {0};
	suit_manifest_role_t role = SUIT_MANIFEST_APP_ROOT;

	suit_plat_mreg_t hash_mreg = {.mem = hash, .size = IMG_MGMT_HASH_LEN};

	if (ok) {
		suit_plat_err_t err = suit_get_supported_manifest_info(role, &class_info);

		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Unable to read the root manifest data class info: %d", err);
			ok = false;
		}
	}

	if (ok) {
		/* Let's proceed installed envelope */
		suit_plat_err_t err = suit_get_installed_manifest_info(
			&class_info.class_id, &seq_num, NULL, NULL, &alg_id, &hash_mreg);

		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Unable to read the current manifest data: %d", err);
			ok = false;
		}

		zhash.value = hash_mreg.mem;
		zhash.len = hash_mreg.size;
	}

	if (ok) {
		int err = snprintf(installed_vers_str, sizeof(installed_vers_str), "%d", seq_num);

		if (err < 0) {
			LOG_ERR("Unable to create manifest version string: %d", err);
			ok = false;
		}
	}
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */

	if (ok) {
		ok = zcbor_map_start_encode(zse, MAX_IMG_CHARACTERISTICS) &&
		     zcbor_tstr_put_lit(zse, "slot") && zcbor_int32_put(zse, 0) &&
		     zcbor_tstr_put_lit(zse, "version");
		LOG_DBG("Manifest slot encoded: %d", ok);
	}

	if (ok) {
		installed_vers_str[sizeof(installed_vers_str) - 1] = '\0';
		ok = zcbor_tstr_put_term(zse, installed_vers_str, sizeof(installed_vers_str));
		LOG_DBG("Manifest version encoded: %d", ok);
	}

	if (ok) {
		ok = zcbor_tstr_put_lit(zse, "hash") && zcbor_bstr_encode(zse, &zhash);
		LOG_DBG("Manifest hash encoded: %d", ok);
	}

	if (ok) {
#if !CONFIG_SUIT_RECOVERY
		ok = ENCODE_FLAG(zse, "bootable", 1) && ENCODE_FLAG(zse, "confirmed", 1) &&
		     ENCODE_FLAG(zse, "active", 1) && ENCODE_FLAG(zse, "permanent", 1);
#else
		ok = ENCODE_FLAG(zse, "pending", 1);
#endif
		LOG_DBG("Manifest flags encoded: %d", ok);
	}

	if (ok) {
		ok = zcbor_map_end_encode(zse, MAX_IMG_CHARACTERISTICS);
		LOG_DBG("Image map encoded: %d", ok);
	}

	ok = ok && zcbor_list_end_encode(zse, 2);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}

static int suitfu_mgmt_img_erase(struct smp_streamer *ctx)
{
	return suitfu_mgmt_cleanup();
}

static int suitfu_mgmt_img_state_write(struct smp_streamer *ctx)
{

	zcbor_state_t *zsd = ctx->reader->zs;
	bool confirm = false;
	size_t decoded = 0;

	struct zcbor_map_decode_key_val image_upload_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(confirm, zcbor_bool_decode, &confirm)};

	if (zcbor_map_decode_bulk(zsd, image_upload_decode, ARRAY_SIZE(image_upload_decode),
				  &decoded) != 0) {
		LOG_ERR("Decoding image state write request failed");
		return MGMT_ERR_EINVAL;
	}

	if (confirm) {
		return suitfu_mgmt_candidate_envelope_process();
	}

	return MGMT_ERR_EOK;
}

static const struct mgmt_handler img_mgmt_handlers[] = {
	[IMG_MGMT_ID_STATE] = {.mh_read = suitfu_mgmt_img_state_read,
			       .mh_write = suitfu_mgmt_img_state_write},
	[IMG_MGMT_ID_UPLOAD] = {.mh_read = NULL, .mh_write = suitfu_mgmt_img_upload},
	[IMG_MGMT_ID_ERASE] = {.mh_read = NULL, .mh_write = suitfu_mgmt_img_erase},
};

static struct mgmt_group img_mgmt_group = {
	.mg_handlers = (struct mgmt_handler *)img_mgmt_handlers,
	.mg_handlers_count = ARRAY_SIZE(img_mgmt_handlers),
	.mg_group_id = MGMT_GROUP_ID_IMAGE,
};

void img_mgmt_register_group(void)
{
	mgmt_register_group(&img_mgmt_group);
}

void img_mgmt_unregister_group(void)
{
	mgmt_unregister_group(&img_mgmt_group);
}

#ifdef CONFIG_MGMT_SUITFU_AUTO_REGISTER_HANDLERS
MCUMGR_HANDLER_DEFINE(suitfu_mgmt_img, img_mgmt_register_group);
#endif
