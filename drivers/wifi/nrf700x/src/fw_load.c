/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing FW load functions for Zephyr.
 *
 * For NRF QSPI NOR special handling is needed for this file as all RODATA of
 * this file is stored in external flash, so, any use of RODATA has to be protected
 * by disabling XIP and enabling it again after use. This means no LOG_* macros
 * (buffered) or buffered printk can be used in this file, else it will crash.
 */
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP) && defined(CONFIG_NORDIC_QSPI_NOR)
#include <zephyr/drivers/flash/nrf_qspi_nor.h>
#endif /* CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF700X_LOG_LEVEL);

#include <fmac_main.h>

#ifdef CONFIG_NRF_WIFI_PATCHES_BUILTIN
/* INCBIN macro Taken from https://gist.github.com/mmozeiko/ed9655cf50341553d282 */
#define STR2(x) #x
#define STR(x) STR2(x)

#ifdef __APPLE__
#define USTR(x) "_" STR(x)
#else
#define USTR(x) STR(x)
#endif

#ifdef _WIN32
#define INCBIN_SECTION ".rdata, \"dr\""
#elif defined __APPLE__
#define INCBIN_SECTION "__TEXT,__const"
#else
#define INCBIN_SECTION ".rodata.*"
#endif

/* this aligns start address to 16 and terminates byte array with explicit 0
 * which is not really needed, feel free to change it to whatever you want/need
 */
#define INCBIN(prefix, name, file) \
	__asm__(".section " INCBIN_SECTION "\n" \
			".global " USTR(prefix) "_" STR(name) "_start\n" \
			".balign 16\n" \
			USTR(prefix) "_" STR(name) "_start:\n" \
			".incbin \"" file "\"\n" \
			\
			".global " STR(prefix) "_" STR(name) "_end\n" \
			".balign 1\n" \
			USTR(prefix) "_" STR(name) "_end:\n" \
			".byte 0\n" \
	); \
	extern __aligned(16)    const char prefix ## _ ## name ## _start[]; \
	extern                  const char prefix ## _ ## name ## _end[];

INCBIN(_bin, nrf70_fw, STR(CONFIG_NRF_WIFI_FW_BIN));
#endif /* CONFIG_NRF_WIFI_PATCHES_BUILTIN */

#ifdef CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include "hal_api.h"

#include <patch_info.h>

/* Flash partition for NVS */
#define NRF70_FW_PATCH_ID FIXED_PARTITION_ID(nrf70_fw_partition)
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
#else
enum nrf_wifi_status nrf_wifi_fw_load(void *rpu_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_fw_info fw_info = { 0 };
#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP) && defined(CONFIG_NORDIC_QSPI_NOR)
	const struct device *flash_dev = DEVICE_DT_GET(DT_INST(0, nordic_qspi_nor));
#endif /* CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP */
	uint8_t *fw_start;
	uint8_t *fw_end;

#ifdef CONFIG_NRF_WIFI_PATCHES_BUILTIN
	fw_start = (uint8_t *)_bin_nrf70_fw_start;
	fw_end = (uint8_t *)_bin_nrf70_fw_end;
#else
	BUILD_ASSERT(0, "CONFIG_NRF_WIFI_PATCHES_BUILTIN must be enabled");
#endif /* CONFIG_NRF_WIFI_PATCHES_BUILTIN */

	status = nrf_wifi_fmac_fw_parse(rpu_ctx, fw_start, fw_end - fw_start,
					&fw_info);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_fw_parse failed", __func__);
		return status;
	}

#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP) && defined(CONFIG_NORDIC_QSPI_NOR)
	nrf_qspi_nor_xip_enable(flash_dev, true);
#endif /* CONFIG_NRF_WIFI */
	/* Load the FW patches to the RPU */
	status = nrf_wifi_fmac_fw_load(rpu_ctx, &fw_info);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_fw_load failed", __func__);
	}

#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP) && defined(CONFIG_NORDIC_QSPI_NOR)
	nrf_qspi_nor_xip_enable(flash_dev, false);
#endif /* CONFIG_NRF_WIFI */

	return status;
}
#endif /* NRF_WIFI_PATCHES_EXT_FLASH_STORE */
