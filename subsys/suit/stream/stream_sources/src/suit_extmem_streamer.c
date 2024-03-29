/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_memptr_streamer.h>
#include <zephyr/logging/log.h>
#include <sdfw/sdfw_services/extmem_service.h>
#include <suit_memory_layout.h>

LOG_MODULE_REGISTER(suit_extmem_streamer, CONFIG_SUIT_LOG_LEVEL);

struct extmem_streamer_ctx {
	struct stream_sink *sink;
	suit_plat_err_t result;
};

static void read_cb(void *data, uintptr_t offset, size_t data_size, void *ctx)
{
	if (ctx == NULL) {
		LOG_ERR("read_cb failed: ctx == NULL");
		return;
	}

	struct extmem_streamer_ctx *extmem_ctx = ctx;
	struct stream_sink *sink = extmem_ctx->sink;

	if (extmem_ctx->result != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Write skipped due to previous errors");
		return;
	}

	extmem_ctx->result = sink->write(sink->ctx, data, data_size);

	if (extmem_ctx->result != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Sink error: %d", extmem_ctx->result);
	}
}

static bool is_address_supported(uintptr_t address, struct extmem_capabilities *capabilities)
{
	if (!suit_memory_global_address_range_is_in_external_memory(capabilities->base_addr,
								    capabilities->capacity) ||
	    !suit_memory_global_address_is_in_external_memory(address)) {
		LOG_INF("Address does not belong to extmem area: %p", (void *)address);
		return false;
	}

	return true;
}

bool suit_extmem_streamer_address_in_range(const uint8_t *address)
{
	struct extmem_capabilities capabilities;

	int err = extmem_capabilities_get(&capabilities);

	if (err != EXTMEM_RESULT_SUCCESS) {
		return false;
	}

	return is_address_supported((uintptr_t)address, &capabilities);
}

suit_plat_err_t suit_extmem_streamer_stream(const uint8_t *payload,
					    size_t payload_size,
					    struct stream_sink *sink)
{
	struct extmem_capabilities capabilities;

	int err = extmem_capabilities_get(&capabilities);

	if (err != EXTMEM_RESULT_SUCCESS) {
		return SUIT_PLAT_ERR_IO;
	}

	uintptr_t offset = (uintptr_t)payload - capabilities.base_addr;

	if ((payload != NULL) && (sink != NULL) && (sink->write != NULL) && (payload_size > 0)) {
		struct extmem_streamer_ctx ctx = {
			.sink = sink,
			.result = SUIT_PLAT_SUCCESS,
		};

		int ret = extmem_read(offset, payload_size, read_cb, &ctx);

		if (ret != EXTMEM_RESULT_SUCCESS) {
			LOG_ERR("Read from external memory failed: %d", ret);
			return SUIT_PLAT_ERR_IO;
		}

		return ctx.result;
	}

	LOG_ERR("Invalid arguments");
	return SUIT_PLAT_ERR_INVAL;
}
