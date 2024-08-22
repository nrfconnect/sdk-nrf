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
#include <suit_envelope_info.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(suitfu_mgmt, CONFIG_MGMT_SUITFU_LOG_LEVEL);

/** Represents an individual upload request. */
typedef struct {
	size_t off;  /* SIZE_MAX if unspecified. */
	size_t size; /* SIZE_MAX if unspecified. */
	bool defer_install;
	struct zcbor_string img_data;
} suitfu_mgmt_envelope_upload_req_t;

int suitfu_mgmt_suit_envelope_upload(struct smp_streamer *ctx)
{
	zcbor_state_t *zsd = ctx->reader->zs;
	zcbor_state_t *zse = ctx->writer->zs;
	static size_t image_size;
	static size_t offset_in_image;
	static bool defer_install;

	static struct suit_nvm_device_info device_info;

	size_t decoded = 0;
	suitfu_mgmt_envelope_upload_req_t req = {
		.off = SIZE_MAX,
		.size = SIZE_MAX,
		.defer_install = false,
		.img_data = {0},
	};

	struct zcbor_map_decode_key_val envelope_upload_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(data, zcbor_bstr_decode, &req.img_data),
		ZCBOR_MAP_DECODE_KEY_VAL(len, zcbor_size_decode, &req.size),
		ZCBOR_MAP_DECODE_KEY_VAL(defer_install, zcbor_bool_decode, &req.defer_install),
		ZCBOR_MAP_DECODE_KEY_VAL(off, zcbor_size_decode, &req.off)};

	if (zcbor_map_decode_bulk(zsd, envelope_upload_decode, ARRAY_SIZE(envelope_upload_decode),
				  &decoded) != 0) {
		LOG_ERR("Decoding envelope upload request failed");
		return MGMT_ERR_EINVAL;
	}

	if (req.off == 0) {
		LOG_INF("New envelope transfer started");
		image_size = 0;
		offset_in_image = 0;

		if (req.size == 0) {
			if (req.defer_install) {
				LOG_ERR("Candidate envelope empty");
				return MGMT_ERR_EINVAL;
			}

			LOG_INF("Triggering an update");
			int rc = suitfu_mgmt_candidate_envelope_process();

			if (zcbor_tstr_put_lit(zse, "rc") && zcbor_int32_put(zse, rc) &&
			    zcbor_tstr_put_lit(zse, "off") &&
			    zcbor_size_put(zse, offset_in_image)) {
				return MGMT_ERR_EOK;
			}

			return MGMT_ERR_EMSGSIZE;
		}

		suit_plat_err_t err = suit_dfu_partition_device_info_get(&device_info);

		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("DFU Partition not found");
			return MGMT_ERR_ENOENT;
		}

		/* Erases dfu_partition and dfu_cache_partition_0, leaves
		 * dfu_partition_1..dfu_partition_n intact. suitfu_mgmt_claenup() may be used
		 * instead.
		 */
		int rc = suitfu_mgmt_erase(&device_info, req.size);
		if (rc != MGMT_ERR_EOK) {
			LOG_ERR("Erasing DFU partition failed");
			return rc;
		}

		image_size = req.size;
		defer_install = req.defer_install;
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

			LOG_INF("Candidate envelope stored");
			rc = suitfu_mgmt_candidate_envelope_stored();
			if (rc == MGMT_ERR_EOK && !defer_install) {
				rc = suitfu_mgmt_candidate_envelope_process();
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

int suitfu_mgmt_suit_cleanup(struct smp_streamer *ctx)
{
	return suitfu_mgmt_cleanup();
}
