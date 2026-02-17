/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <dfu/fmfu_fdev.h>
#include <nrf_modem_bootloader.h>
#include <psa/crypto.h>
#include <stdio.h>
#include <modem_update_decode.h>

LOG_MODULE_REGISTER(fmfu_fdev, CONFIG_FMFU_FDEV_LOG_LEVEL);

/* The size of the cbor metadata structure will not exceed this value. */
#define MAX_META_LEN 1024

static uint8_t meta_buf[MAX_META_LEN];

static int get_hash_from_flash(const struct device *fdev, size_t offset, size_t data_len,
			       uint8_t hash[static 32], uint8_t *buffer, size_t buffer_len)
{
	int err;
	psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
	psa_status_t status;
	size_t hash_len;
	size_t end = offset + data_len;
	BUILD_ASSERT(PSA_SUCCESS == 0);

	status = psa_hash_setup(&operation, PSA_ALG_SHA_256);
	if (status != PSA_SUCCESS) {
		return status;
	}

	for (size_t tmp_offs = offset; tmp_offs < end; tmp_offs += buffer_len) {
		size_t part_len = MIN(buffer_len, (end - tmp_offs));

		err = flash_read(fdev, tmp_offs, buffer, part_len);
		if (err != 0) {
			psa_hash_abort(&operation);
			return err;
		}

		status = psa_hash_update(&operation, buffer, part_len);
		if (status != PSA_SUCCESS) {
			psa_hash_abort(&operation);
			return status;
		}
	}

	status = psa_hash_finish(&operation, hash, 32, &hash_len);
	if (status != PSA_SUCCESS) {
		psa_hash_abort(&operation);
	}
	return status;
}

static int write_chunk(uint8_t *buf, size_t buf_len, uint32_t address, bool is_bootloader)
{
	int err;

	if (is_bootloader) {
		err = nrf_modem_bootloader_bl_write(buf, buf_len);
		if (err != 0) {
			LOG_ERR("nrf_modem_bootloader_bl_write failed, err: %d", err);
			return err;
		}
	} else {
		err = nrf_modem_bootloader_fw_write(address, buf, buf_len);
		if (err != 0) {
			LOG_ERR("nrf_modem_bootloader_fw_write failed, err: %d", err);
			return err;
		}
	}

	return 0;
}

static int load_segment(const struct device *fdev, size_t seg_size, uint32_t seg_target_addr,
			uint32_t seg_offset, uint8_t *buf, size_t buf_len, bool is_bootloader)
{
	int err;
	uint32_t read_addr = seg_offset;
	size_t bytes_left = seg_size;

	while (bytes_left) {
		uint32_t read_len = MIN(buf_len, bytes_left);

		err = flash_read(fdev, read_addr, buf, read_len);
		if (err != 0) {
			LOG_ERR("flash_read failed: %d", err);
			return err;
		}

		err = write_chunk(buf, read_len, seg_target_addr, is_bootloader);
		if (err != 0) {
			LOG_ERR("write_chunk failed: %d", err);
			return err;
		}

		LOG_DBG("Wrote chunk: offset 0x%x target addr 0x%x size 0x%x", read_addr,
			seg_target_addr, read_len);

		seg_target_addr += read_len;
		bytes_left -= read_len;
		read_addr += read_len;
	}

	if (is_bootloader) {
		/* We need to explicitly call _apply() once all chunks of the
		 * bootloader has been written.
		 */
		err = nrf_modem_bootloader_update();
		if (err != 0) {
			LOG_ERR("nrf_modem_bootloader_update (bl) failed, err: %d", err);
			return err;
		}
	}

	return 0;
}

static int load_segments(const struct device *fdev, uint8_t *meta_buf, size_t wrapper_len,
			 const struct Segments *seg, size_t blob_offset, uint8_t *buf,
			 size_t buf_len)
{
	int err;
	size_t prev_segments_len = 0;

	for (int i = 0; i < seg->Segments_Segment_m_count; i++) {
		size_t seg_size = seg->Segments_Segment_m[i].Segment_len;
		uint32_t seg_addr = seg->Segments_Segment_m[i].Segment_target_addr;
		uint32_t read_addr = blob_offset + prev_segments_len;
		bool is_bootloader = i == 0;

		LOG_INF("Writing segment %d/%d, Target addr: 0x%x, size: 0%x", i + 1,
			seg->Segments_Segment_m_count, seg_addr, seg_size);

		err = load_segment(fdev, seg_size, seg_addr, read_addr, buf, buf_len,
				   is_bootloader);
		if (err != 0) {
			LOG_ERR("load_segment failed: %d", err);
			return err;
		}

		if (i == 0) {
#ifndef CONFIG_FMFU_FDEV_SKIP_PREVALIDATION
			/* The IPC-DFU bootloader has been written, we can now
			 * perform the prevalidation.
			 */
			LOG_INF("Running prevalidation (can take minutes)");
			err = nrf_modem_bootloader_verify((void *)meta_buf, wrapper_len);
			if (err != 0) {
				LOG_ERR("nrf_fmfu_verify_signature failed, err: %d", err);
				return err;
			}
#else
			LOG_WRN("[WARNING] Skipping prevalidation, this "
				"should only be done during development");
#endif /* CONFIG_FMFU_FDEV_SKIP_PREVALIDATION */
		}
		prev_segments_len += seg_size;
	}

	err = nrf_modem_bootloader_update();
	if (err != 0) {
		LOG_ERR("nrf_modem_bootloader_update (fw) failed, err: %d", err);
		return err;
	}

	LOG_INF("FMFU finished");

	return 0;
}

int fmfu_fdev_load(uint8_t *buf, size_t buf_len, const struct device *fdev, size_t offset)
{
	const struct zcbor_string *segments_string;
	struct COSE_Sign1_Manifest wrapper;
	bool hash_len_valid = false;
	uint8_t expected_hash[32];
	bool hash_valid = false;
	struct Segments segments;
	size_t blob_offset;
	size_t wrapper_len;
	uint8_t hash[32];
	size_t blob_len;
	int err;

	if (buf == NULL || fdev == NULL) {
		return -ENOMEM;
	}

	/* Read the whole wrapper. */
	err = flash_read(fdev, offset, meta_buf, MAX_META_LEN);
	if (err != 0) {
		return err;
	}

	if (cbor_decode_Wrapper(meta_buf, MAX_META_LEN, &wrapper, &wrapper_len) != ZCBOR_SUCCESS) {
		LOG_ERR("Unable to decode wrapper");
		return -EINVAL;
	}

	/* Add the base offset of the wrapper to the blob offset to get the
	 * absolute offset to the blob in the flash device.
	 */
	blob_offset = wrapper_len + offset;

	/* Get a pointer to, and decode,  the segments as this is a cbor encoded
	 * structure in itself.
	 */
	segments_string = &wrapper.COSE_Sign1_Manifest_payload_cbor.Manifest_segments;
	if (cbor_decode_Segments(segments_string->value, segments_string->len, &segments, NULL) !=
	    ZCBOR_SUCCESS) {
		LOG_ERR("Unable to decode segments");
		return -EINVAL;
	}

	/* Extract the expected hash from the manifest */
	memcpy(expected_hash, wrapper.COSE_Sign1_Manifest_payload_cbor.Manifest_blob_hash.value,
	       sizeof(expected_hash));

	/* Calculate total length of all segments */
	blob_len = 0;
	for (int i = 0; i < segments.Segments_Segment_m_count; i++) {
		blob_len += segments.Segments_Segment_m[i].Segment_len;
	}

	if (sizeof(hash) == wrapper.COSE_Sign1_Manifest_payload_cbor.Manifest_blob_hash.len) {
		hash_len_valid = true;
	} else {
		LOG_ERR("Invalid hash length");
		return -EINVAL;
	}

	err = get_hash_from_flash(fdev, blob_offset, blob_len, hash, buf, buf_len);
	if (err != 0) {
		return err;
	}

	if (memcmp(expected_hash, hash, sizeof(hash)) == 0) {
		hash_valid = true;
	} else {
		LOG_ERR("Invalid hash");
		return -EINVAL;
	}

	if (hash_len_valid && hash_valid) {
		return load_segments(fdev, meta_buf, wrapper_len,
				     (const struct Segments *)&segments, blob_offset, buf, buf_len);
	} else {
		return -EINVAL;
	}
}
