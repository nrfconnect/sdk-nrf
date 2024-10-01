/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <dfu_stream_flatten.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dfu_stream_flatten, CONFIG_DFU_TARGET_LOG_LEVEL);

int stream_flash_flatten_page(struct stream_flash_ctx *ctx, off_t off)
{
	int rc;
	struct flash_pages_info page;

	rc = flash_get_page_info_by_offs(ctx->fdev, off, &page);

	if (rc != 0) {
		LOG_ERR("Error %d while getting page info", rc);
		return rc;
	}

#if defined(CONFIG_STREAM_FLASH_ERASE)
	if (ctx->last_erased_page_start_offset == page.start_offset) {
		return 0;
	}
#else
	if (ctx->bytes_written + ctx->offset > page.start_offset) {
		return 0;
	}
#endif

	LOG_DBG("Flattening page at offset 0x%08lx", (long)page.start_offset);

	rc = flash_flatten(ctx->fdev, page.start_offset, page.size);

	if (rc != 0) {
		LOG_ERR("Error %d while flattening page", rc);
#if defined(CONFIG_STREAM_FLASH_ERASE)
	} else {
		ctx->last_erased_page_start_offset = page.start_offset;
#endif
	}

	return rc;
}
