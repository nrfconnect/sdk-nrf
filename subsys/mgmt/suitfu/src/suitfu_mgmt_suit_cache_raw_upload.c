/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
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
#include <suit_dfu_cache_rw.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(suitfu_mgmt, CONFIG_MGMT_SUITFU_LOG_LEVEL);

/** Represents an individual upload request. */
typedef struct {
	size_t off;  /* SIZE_MAX if unspecified. */
	size_t size; /* SIZE_MAX if unspecified. */
	uint32_t target_id;
	struct zcbor_string img_data;
} suitfu_mgmt_envelope_upload_req_t;

int suitfu_mgmt_suit_cache_raw_upload(struct smp_streamer *ctx)
{
	zcbor_state_t *zsd = ctx->reader->zs;
	zcbor_state_t *zse = ctx->writer->zs;
	static uint32_t target_id;
	static size_t image_size;
	static size_t offset_in_image;

	static struct suit_nvm_device_info device_info;

	size_t decoded = 0;
	suitfu_mgmt_envelope_upload_req_t req = {
		.off = SIZE_MAX,
		.size = SIZE_MAX,
		.target_id = ~0,
		.img_data = {0},
	};

	struct zcbor_map_decode_key_val envelope_upload_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(data, zcbor_bstr_decode, &req.img_data),
		ZCBOR_MAP_DECODE_KEY_VAL(len, zcbor_size_decode, &req.size),
		ZCBOR_MAP_DECODE_KEY_VAL(off, zcbor_size_decode, &req.off),
		ZCBOR_MAP_DECODE_KEY_VAL(target_id, zcbor_uint32_decode, &req.target_id)};

	if (zcbor_map_decode_bulk(zsd, envelope_upload_decode, ARRAY_SIZE(envelope_upload_decode),
				  &decoded) != 0) {
		LOG_ERR("Decoding envelope upload request failed");
		return MGMT_ERR_EINVAL;
	}

	if (req.off == 0) {
		LOG_INF("New Cache RAW upload, cache pool %d, image size: %d bytes", req.target_id,
			req.size);
		image_size = 0;
		offset_in_image = 0;

		if (req.size == 0) {
			LOG_ERR("New Cache RAW upload, cache pool %d, empty image", req.target_id);
			return MGMT_ERR_EINVAL;
		}

		suit_plat_err_t err =
			suit_dfu_cache_rw_device_info_get(req.target_id, &device_info);
		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Cache pool %d not found", req.target_id);
			return MGMT_ERR_ENOENT;
		}

		if (req.size > device_info.partition_size) {
			LOG_ERR("Image too large: %d bytes. Cache pool %d size: %d bytes", req.size,
				req.target_id, device_info.partition_size);
			return MGMT_ERR_ENOMEM;
		}

		int rc = suitfu_mgmt_erase(&device_info, req.size);

		if (rc != MGMT_ERR_EOK) {
			LOG_ERR("Cache pool %d, erasing partition failed", req.target_id);
			return MGMT_ERR_EBADSTATE;
		}

		image_size = req.size;
		target_id = req.target_id;
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

		req.off += req.img_data.len;
		offset_in_image += req.img_data.len;
		if (last) {
			LOG_INF("Cache pool %d updated with RAW image", target_id);
			image_size = 0;
		}
	}

	if (zcbor_tstr_put_lit(zse, "rc") && zcbor_int32_put(zse, rc) &&
	    zcbor_tstr_put_lit(zse, "off") && zcbor_size_put(zse, offset_in_image)) {
		return MGMT_ERR_EOK;
	}

	return MGMT_ERR_EMSGSIZE;
}
