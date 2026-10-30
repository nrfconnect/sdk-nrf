/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <psa/crypto.h>

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include <net/downloader.h>

#include "ota_download.h"

#include "ota_bundle_decode.h"

LOG_MODULE_REGISTER(ota_download, LOG_LEVEL_INF);

static const char *ota_target_name(int target)
{
	switch (target) {
	case ota_element_target_nrf53_app_m_c:
		return "nrf53_app";
	case ota_element_target_nrf53_net_m_c:
		return "nrf53_net";
	case ota_element_target_nrf53_bl_m_c:
		return "nrf53_bl";
	case ota_element_target_nrf91_app_m_c:
		return "nrf91_app";
	case ota_element_target_nrf91_bl_m_c:
		return "nrf91_bl";
	case ota_element_target_nrf91_mdm_delta_m_c:
		return "nrf91_mdm_delta";
	case ota_element_target_nrf91_mdm_full_m_c:
		return "nrf91_mdm_full";
	default:
		return "unknown";
	}
}

static struct downloader dl;
static struct fs_file_t ota_file;
static char dl_buf[2048];
static K_SEM_DEFINE(dl_done, 0, 1);
static int dl_result;

static int dl_callback(const struct downloader_evt *evt)
{
	int err;

	switch (evt->id) {
	case DOWNLOADER_EVT_FRAGMENT:
		err = fs_write(&ota_file, evt->fragment.buf, evt->fragment.len);
		if (err < 0) {
			LOG_ERR("fs_write failed: %d", err);
			dl_result = err;
			return 1; /* non-zero stops the download */
		}
		return 0;

	case DOWNLOADER_EVT_ERROR:
		LOG_ERR("Download error: %d", evt->error);
		dl_result = evt->error;
		return 1; /* stop; zero would let the downloader reconnect and resume */

	case DOWNLOADER_EVT_DONE:
		dl_result = 0;
		k_sem_give(&dl_done);
		return 0;

	case DOWNLOADER_EVT_STOPPED:
		if (dl_result == 0) {
			dl_result = -ECANCELED;
		}
		k_sem_give(&dl_done);
		return 0;

	default:
		return 0;
	}
}

int ota_bundle_download(const char *url, const int *sec_tags, uint8_t sec_tag_count)
{
	struct downloader_cfg cfg = {
		.callback = dl_callback,
		.buf = dl_buf,
		.buf_size = sizeof(dl_buf),
	};
	struct downloader_host_cfg host_cfg = {
		.sec_tag_list = sec_tags,
		.sec_tag_count = sec_tag_count,
		.range_override = sizeof(dl_buf),
	};
	struct fs_statvfs sv;
	int err;

	/* Free the whole partition for the new bundle */
	(void)fs_unlink(OTA_BUNDLE_PATH);

	fs_file_t_init(&ota_file);
	err = fs_open(&ota_file, OTA_BUNDLE_PATH, FS_O_CREATE | FS_O_WRITE);
	if (err) {
		LOG_ERR("fs_open failed: %d", err);
		return err;
	}

	err = downloader_init(&dl, &cfg);
	if (err) {
		LOG_ERR("downloader_init failed: %d", err);
		goto out_close;
	}

	k_sem_reset(&dl_done);
	dl_result = 0;

	err = downloader_get(&dl, &host_cfg, url, 0);
	if (err) {
		LOG_ERR("downloader_get failed: %d", err);
		goto out_deinit;
	}

	k_sem_take(&dl_done, K_FOREVER);
	err = dl_result;

out_deinit:
	downloader_deinit(&dl);
out_close:
	fs_sync(&ota_file);
	fs_close(&ota_file);

	if (err) {
		(void)fs_unlink(OTA_BUNDLE_PATH); /* don't keep a partial bundle */
	} else if (fs_statvfs("/ota", &sv) == 0) {
		LOG_INF("Bundle stored, %lu of %lu blocks free", sv.f_bfree, sv.f_blocks);
	}

	return err;
}

int ota_bundle_apply(void)
{
	uint8_t digest[32];
	uint8_t expected_digest[32];
	struct ota_bundle bundle = {0};
	psa_hash_operation_t sha_ctx = {0};
	size_t header_len = 0;
	size_t digest_len = 0;
	size_t payload_offset;
	uint64_t elements_size = 0;
	uint32_t remaining;
	ssize_t bytes_read;
	int err;
	bool sha_initialized = false;

	fs_file_t_init(&ota_file);
	err = fs_open(&ota_file, OTA_BUNDLE_PATH, FS_O_READ);
	if (err) {
		LOG_ERR("fs_open failed: %d", err);
		return err;
	}

	bytes_read = fs_read(&ota_file, dl_buf, sizeof(dl_buf));
	if (bytes_read <= 0) {
		err = bytes_read < 0 ? (int)bytes_read : -EBADMSG;
		LOG_ERR("Failed to read OTA bundle header: %d", err);
		goto out_close;
	}

	err = cbor_decode_ota_bundle(dl_buf, (size_t)bytes_read, &bundle, &header_len);
	if (err != ZCBOR_SUCCESS || header_len == 0 || header_len > (size_t)bytes_read) {
		LOG_ERR("Invalid OTA bundle header: %d", err);
		err = -EBADMSG;
		goto out_close;
	}

	memcpy(expected_digest, bundle.bundle_sha256.value, sizeof(expected_digest));

	err = fs_seek(&ota_file, (off_t)header_len, FS_SEEK_SET);
	if (err) {
		LOG_ERR("Failed to seek to OTA payload: %d", err);
		goto out_close;
	}

	psa_status_t psa_err = psa_crypto_init();
	if (psa_err != PSA_SUCCESS) {
		LOG_ERR("Failed to initialize PSA Crypto: %d", psa_err);
		err = -EIO;
		goto out_close;
	}
	psa_err = psa_hash_setup(&sha_ctx, PSA_ALG_SHA_256);
	if (psa_err != PSA_SUCCESS) {
		LOG_ERR("Failed to initialize SHA-256: %d", psa_err);
		err = -EIO;
		goto out_close;
	}
	sha_initialized = true;

	remaining = bundle.bundle_size;
	while (remaining > 0) {
		size_t read_size = remaining < sizeof(dl_buf) ? remaining : sizeof(dl_buf);

		bytes_read = fs_read(&ota_file, dl_buf, read_size);
		if (bytes_read <= 0) {
			err = bytes_read < 0 ? (int)bytes_read : -EBADMSG;
			LOG_ERR("Failed to read OTA payload: %d", err);
			goto out_free_sha;
		}
		psa_err = psa_hash_update(&sha_ctx, dl_buf, (size_t)bytes_read);
		if (psa_err != PSA_SUCCESS) {
			LOG_ERR("Failed to update SHA-256: %d", psa_err);
			err = -EIO;
			goto out_free_sha;
		}
		remaining -= (uint32_t)bytes_read;
	}

	bytes_read = fs_read(&ota_file, dl_buf, 1);
	if (bytes_read != 0) {
		LOG_ERR("OTA bundle has trailing data or could not be read");
		err = bytes_read < 0 ? (int)bytes_read : -EBADMSG;
		goto out_free_sha;
	}

	psa_err = psa_hash_finish(&sha_ctx, digest, sizeof(digest), &digest_len);
	sha_initialized = false;
	if (psa_err != PSA_SUCCESS || digest_len != sizeof(digest)) {
		LOG_ERR("Failed to finish SHA-256: %d", psa_err);
		err = -EIO;
		goto out_free_sha;
	}
	if (memcmp(digest, expected_digest, sizeof(digest)) != 0) {
		LOG_ERR("OTA bundle SHA-256 mismatch");
		err = -EBADMSG;
		goto out_free_sha;
	}
	err = 0;

	payload_offset = header_len;
	LOG_INF("OTA bundle valid: %u payload bytes, %zu elements; SHA-256 verified",
		bundle.bundle_size, bundle.ota_element_m_count);
	for (size_t i = 0; i < bundle.ota_element_m_count; i++) {
		const struct ota_element *element = &bundle.ota_element_m[i];

		LOG_INF("  %s: offset %zu, size %u bytes", ota_target_name(element->target_choice),
			payload_offset, element->size);
		payload_offset += element->size;
	}

	LOG_INF("expected size: %u, actual size: %u", bundle.bundle_size + header_len, payload_offset);

out_free_sha:
	if (sha_initialized) {
		(void)psa_hash_abort(&sha_ctx);
	}
out_close:
	fs_close(&ota_file);
	return err;
}