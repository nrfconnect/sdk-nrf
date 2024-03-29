/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_plat_fetch_domain_specific.h>
#include <suit_plat_error_convert.h>
#include <suit_plat_memptr_size_update.h>

#ifdef CONFIG_SUIT_STREAM

#ifdef CONFIG_SUIT_STREAM_SOURCE_CACHE
#include <suit_dfu_cache_streamer.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_CACHE */
#ifdef CONFIG_SUIT_STREAM_IPC_REQUESTOR
#include <suit_ipc_streamer.h>
#endif /* CONFIG_SUIT_STREAM_IPC_REQUESTOR */

LOG_MODULE_REGISTER(suit_plat_fetch_sdfw, CONFIG_SUIT_LOG_LEVEL);

static bool is_type_supported(suit_component_type_t component_type)
{
	if ((component_type == SUIT_COMPONENT_TYPE_CAND_IMG) ||
	    (component_type == SUIT_COMPONENT_TYPE_CAND_MFST) ||
	    (component_type == SUIT_COMPONENT_TYPE_SOC_SPEC) ||
	    (component_type == SUIT_COMPONENT_TYPE_MEM)) {
		return true;
	}

	return false;
}

static bool is_type_supported_by_ipc(suit_component_type_t component_type)
{
	if (component_type == SUIT_COMPONENT_TYPE_MEM) {
		return true;
	}

	return false;
}

bool suit_plat_fetch_domain_specific_is_type_supported(suit_component_type_t component_type)
{
	if (IS_ENABLED(CONFIG_SUIT_STREAM_SOURCE_CACHE) ||
	    (IS_ENABLED(CONFIG_SUIT_STREAM_IPC_REQUESTOR) &&
	     is_type_supported_by_ipc(component_type))) {
		return is_type_supported(component_type);
	}

	return false;
}

bool suit_plat_fetch_integrated_domain_specific_is_type_supported(
	suit_component_type_t component_type)
{
	return is_type_supported(component_type);
}

int suit_plat_fetch_domain_specific(suit_component_t dst_handle,
				    suit_component_type_t dst_component_type,
				    struct stream_sink *dst_sink, struct zcbor_string *uri)
{
	/* If cache is disabled, act as thou uri was not found in cache */
	suit_plat_err_t ret = SUIT_PLAT_ERR_NOT_FOUND;

#ifdef CONFIG_SUIT_STREAM_SOURCE_CACHE
	/* Check if requested uri exists in cache and get streamer */
	ret = suit_dfu_cache_streamer_stream(uri->value, uri->len, dst_sink);
#endif /* CONFIG_SUIT_STREAM_SOURCE_CACHE */

#ifdef CONFIG_SUIT_STREAM_IPC_REQUESTOR
	if ((ret == SUIT_PLAT_ERR_NOT_FOUND) &&		    /* URI was not found in cache */
	    is_type_supported_by_ipc(dst_component_type)) { /* component type is supported */
		/* Request uri through ipc streamer */
		ret = suit_ipc_streamer_stream(uri->value, uri->len, dst_sink,
					       CONFIG_SUIT_STREAM_IPC_STREAMER_CHUNK_TIMEOUT,
					       CONFIG_SUIT_STREAM_IPC_STREAMER_REQUESTING_PERIOD);
	}
#endif /* CONFIG_SUIT_STREAM_IPC_REQUESTOR */

	if (ret == SUIT_PLAT_SUCCESS) {
		/* Update size in memptr for MEM component */
		if (dst_component_type == SUIT_COMPONENT_TYPE_MEM) {
			size_t new_size = 0;

			ret = dst_sink->used_storage(dst_sink->ctx, &new_size);
			if (ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Getting used storage on destination sink failed");

				return suit_plat_err_to_processor_err_convert(ret);
			}

			ret = suit_plat_memptr_size_update(dst_handle, new_size);
			if (ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Failed to update destination MEM component size: %i", ret);

				return suit_plat_err_to_processor_err_convert(ret);
				;
			}
		}
	}

	return suit_plat_err_to_processor_err_convert(ret);
}

int suit_plat_fetch_integrated_domain_specific(suit_component_t dst_handle,
					       suit_component_type_t dst_component_type,
					       struct stream_sink *dst_sink)
{
	suit_plat_err_t ret = SUIT_SUCCESS;

	/* Update size in memptr for MEM component */
	if (dst_component_type == SUIT_COMPONENT_TYPE_MEM) {
		size_t new_size = 0;

		ret = dst_sink->used_storage(dst_sink->ctx, &new_size);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Getting used storage on destination sink failed");

			return suit_plat_err_to_processor_err_convert(ret);
		}

		ret = suit_plat_memptr_size_update(dst_handle, new_size);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to update destination MEM component size: %i", ret);

			return suit_plat_err_to_processor_err_convert(ret);
		}
	}

	return suit_plat_err_to_processor_err_convert(ret);
}

#endif /* CONFIG_SUIT_STREAM */
