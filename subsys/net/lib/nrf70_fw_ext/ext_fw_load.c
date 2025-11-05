/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing FW load functions for Zephyr.
 */
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP) && defined(CONFIG_NORDIC_QSPI_NOR)
#include <zephyr/drivers/flash/nrf_qspi_nor.h>
#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
#include <nrfx_clock.h>
#endif
#endif /* CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP */

#include <zephyr/logging/log.h>
#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP) && defined(CONFIG_NORDIC_QSPI_NOR)
/* For NRF QSPI NOR special handling is needed for this file as all RODATA of
 * this file is stored in external flash, so, any use of RODATA has to be protected
 * by disabling XIP and enabling it again after use. This means no LOG_* macros
 * (buffered) or buffered printk can be used in this file, else it will crash.
 */
LOG_MODULE_DECLARE(wifi_nrf, LOG_LEVEL_NONE);
#else
LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF70_LOG_LEVEL);
#endif /* CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP */

#include <fmac_main.h>

#ifdef CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP
static const char nrf70_fw_patch[] = {
	#include <nrf70_fw_patch/nrf70.bin.inc>
};
#endif /* CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP */

#ifdef CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#ifdef CONFIG_NRF70_SYSTEM_MODE
#include "system/hal_api.h"
#elif CONFIG_NRF70_RADIO_TEST
#include "radio_test/hal_api.h"
#else
#include "offload_raw_tx/hal_api.h"
#endif

#include <patch_info.h>

#if USE_PARTITION_MANAGER
#include <pm_config.h>
#define NRF70_FW_PATCH_ID PM_NRF70_WIFI_FW_ID
#else
#define NRF70_FW_PATCH_ID FIXED_PARTITION_ID(nrf70_fw_partition)
#endif
static const struct flash_area *fa;

static int nrf_wifi_read_and_download_chunk(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					    const struct flash_area *fa,
					    unsigned int image_id,
					    char *fw_chunk,
					    unsigned int offset,
					    unsigned int rpu_addr_offset,
					    unsigned int len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	const struct nrf70_fw_addr_info *addr_info;
	struct nrf_wifi_fmac_fw_chunk_info fw_chunk_info = { 0 };
	int err;

	LOG_DBG("Reading chunk of size %d from offset %d", len, offset);

	/* Read the chunk from Flash */
	err = flash_area_read(fa, offset, fw_chunk, len);
	if (err < 0) {
		LOG_ERR("Failed to read patch chunk offset:%d from flash: %d", offset, err);
		goto out;
	}

	switch (image_id) {
	case NRF70_IMAGE_LMAC_PRI:
		addr_info = &nrf70_fw_addr_info[0];
		break;
	case NRF70_IMAGE_LMAC_SEC:
		addr_info = &nrf70_fw_addr_info[1];
		break;
	case NRF70_IMAGE_UMAC_PRI:
		addr_info = &nrf70_fw_addr_info[2];
		break;
	case NRF70_IMAGE_UMAC_SEC:
		addr_info = &nrf70_fw_addr_info[3];
		break;
	default:
		LOG_ERR("Invalid image id: %d\n", image_id);
		goto out;
	}

	fw_chunk_info.dest_addr = addr_info->dest_addr + rpu_addr_offset;
	memcpy(fw_chunk_info.id_str, addr_info->name, sizeof(addr_info->name));
	fw_chunk_info.data = fw_chunk;
	fw_chunk_info.size = len;

	status = nrf_wifi_fmac_fw_chunk_load(fmac_dev_ctx,
					     addr_info->rpu_proc,
					     &fw_chunk_info);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("Failed to load patch chunk, %s", addr_info->name);
	}

out:
	return status;
}

enum nrf_wifi_status nrf_wifi_fw_load(void *rpu_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf70_fw_image_info patch_hdr;
	unsigned int image_id;
	int err;
	unsigned int offset = 0, rpu_addr_offset;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = rpu_ctx;
	unsigned int max_chunk_size = CONFIG_NRF_WIFI_FW_FLASH_CHUNK_SIZE;
	char *fw_chunk = NULL;
#ifdef CONFIG_NRF_WIFI_FW_PATCH_INTEGRITY_CHECK
	struct flash_area_check nrf70_fw_patch_check = { 0 };
	char *fw_patch_check_buf = NULL;
#endif /* NRF_WIFI_FW_PATCH_INTEGRITY_CHECK */

	err = flash_area_open(NRF70_FW_PATCH_ID, &fa);
	if (err < 0) {
		LOG_ERR("Failed to open flash area: %d", err);
		goto out;
	}

	LOG_DBG("Flash area opened with size: %d, offset: %ld", fa->fa_size, fa->fa_off);

	/* Read the Header from Flash */
	err = flash_area_read(fa, 0, &patch_hdr, sizeof(patch_hdr));
	if (err < 0) {
		LOG_ERR("Failed to read patch header from flash: %d", err);
		goto out;
	}
	offset += sizeof(patch_hdr);

	status = nrf_wifi_validate_fw_header(rpu_ctx, &patch_hdr);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("Failed to validate patch header: %d", status);
		goto out;
	}

#ifdef CONFIG_NRF_WIFI_FW_PATCH_INTEGRITY_CHECK
	fw_patch_check_buf = k_malloc(max_chunk_size);
	if (!fw_patch_check_buf) {
		LOG_ERR("Failed to allocate memory for patch data size: %d", patch_hdr.len);
		goto out;
	}
	nrf70_fw_patch_check.match = (uint8_t *)patch_hdr.hash;
	nrf70_fw_patch_check.clen = patch_hdr.len;
	nrf70_fw_patch_check.off = offset;
	nrf70_fw_patch_check.rbuf = fw_patch_check_buf;
	nrf70_fw_patch_check.rblen = max_chunk_size;
	/* Check the integrity of the patch */
	err = flash_area_check_int_sha256(fa, &nrf70_fw_patch_check);
	if (err < 0) {
		LOG_ERR("Patch integrity check failed: %d", err);
		status = NRF_WIFI_STATUS_FAIL;
		k_free(fw_patch_check_buf);
		goto out;
	}
	k_free(fw_patch_check_buf);
#endif /* NRF_WIFI_FW_PATCH_INTEGRITY_CHECK */

	status = nrf_wifi_fmac_fw_reset(rpu_ctx);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("Failed to reset FMAC: %d", status);
		goto out;
	}

	fw_chunk = k_malloc(max_chunk_size);
	if (!fw_chunk) {
		LOG_ERR("Failed to allocate memory for patch chunk size: %d", max_chunk_size);
		goto out;
	}

	for (image_id = 0; image_id < patch_hdr.num_images; image_id++) {
		struct nrf70_fw_image image;
		unsigned int num_chunks;
		unsigned int chunk_id;

		rpu_addr_offset = 0;

		/* Read sub-header */
		err = flash_area_read(fa, offset, &image, sizeof(image));
		if (err < 0) {
			LOG_ERR("Failed to read patch image from flash: %d", err);
			goto out;
		}
		offset += sizeof(image);

		num_chunks = image.len / max_chunk_size +
			     (image.len % max_chunk_size ? 1 : 0);
		LOG_DBG("Processing image %d, len: %d, num_chunks: %d",
			image_id, image.len, num_chunks);
		for (chunk_id = 0; chunk_id < num_chunks; chunk_id++) {
			unsigned int chunk_size = image.len - chunk_id * max_chunk_size;

			if (chunk_size > max_chunk_size) {
				chunk_size = max_chunk_size;
			}

			LOG_DBG("Processing chunk %d-%d, size: %d", image_id, chunk_id, chunk_size);
			status = nrf_wifi_read_and_download_chunk(fmac_dev_ctx,
						fa, image_id, fw_chunk, offset, rpu_addr_offset,
						chunk_size);
			if (status != NRF_WIFI_STATUS_SUCCESS) {
				LOG_ERR("Failed to read and download patch %d-%d",
					image_id, chunk_id);
				goto out;
			}
			offset += chunk_size;
			rpu_addr_offset += chunk_size;
		}
	}

	status = nrf_wifi_fmac_fw_boot(rpu_ctx);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("Failed to boot FMAC: %d", status);
		goto out;
	}

out:
	if (fw_chunk) {
		k_free(fw_chunk);
	}
	flash_area_close(fa);
	return status;
}
#elif CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP
#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
static nrf_clock_hfclk_div_t saved_divider = NRF_CLOCK_HFCLK_DIV_1;
#endif
static void enable_xip_and_set_cpu_freq(void)
{
#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
	/* Save the current divider */
	saved_divider = nrfx_clock_divider_get(NRF_CLOCK_DOMAIN_HFCLK);

	if (saved_divider == NRF_CLOCK_HFCLK_DIV_2) {
		LOG_DBG("CPU frequency is already set to 64 MHz (DIV_2)");
	} else {
		/* Set CPU frequency to 64MHz (DIV_2) */
		int ret = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_2);

		if (ret != 0) {
			LOG_ERR("Failed to set CPU frequency: %d", ret);
			return;
		}

		LOG_DBG("CPU frequency set to 64 MHz");
	}
#endif

#if defined(CONFIG_NORDIC_QSPI_NOR)
	const struct device *flash_dev = DEVICE_DT_GET(DT_INST(0, nordic_qspi_nor));

	nrf_qspi_nor_xip_enable(flash_dev, true);

	LOG_DBG("XIP enabled");
#endif
}

static void disable_xip_and_restore_cpu_freq(void)
{
#if defined(CONFIG_NORDIC_QSPI_NOR)
	const struct device *flash_dev = DEVICE_DT_GET(DT_INST(0, nordic_qspi_nor));

	nrf_qspi_nor_xip_enable(flash_dev, false);

	LOG_DBG("XIP disabled");
#endif

#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
	/* Restore CPU frequency to the saved value */
	nrf_clock_hfclk_div_t current_divider = nrfx_clock_divider_get(NRF_CLOCK_DOMAIN_HFCLK);

	if (current_divider != saved_divider) {
		int ret = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, saved_divider);

		if (ret != 0) {
			LOG_ERR("Failed to restore CPU frequency: %d", ret);
		} else {
			LOG_DBG("CPU frequency restored to original value");
		}
	} else {
		LOG_DBG("CPU frequency is already at the saved value");
	}
#endif
}

enum nrf_wifi_status nrf_wifi_fw_load(void *rpu_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_fw_info fw_info = { 0 };

	enable_xip_and_set_cpu_freq();

	status = nrf_wifi_fmac_fw_parse(rpu_ctx,
			  nrf70_fw_patch,
			  sizeof(nrf70_fw_patch),
			  &fw_info);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_fw_parse failed", __func__);
		goto out;
	}

	/* Load the FW patches to the RPU */
	status = nrf_wifi_fmac_fw_load(rpu_ctx, &fw_info);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_fw_load failed", __func__);
	}

out:
	disable_xip_and_restore_cpu_freq();
	return status;
}
#endif /* NRF_WIFI_PATCHES_EXT_FLASH_STORE */
