/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <stdbool.h>
#include <suit_platform.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_error_convert.h>
#include <suit_platform_internal.h>
#include <suit_plat_digest_cache.h>
#include <suit_plat_memptr_size_update.h>

#ifdef CONFIG_SUIT_STREAM
#include <suit_sink.h>
#include <suit_sink_selector.h>
#include <suit_generic_address_streamer.h>
#endif /* CONFIG_SUIT_STREAM */

#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
#include <suit_memptr_storage.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

LOG_MODULE_REGISTER(suit_plat_copy, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_check_copy(suit_component_t dst_handle, suit_component_t src_handle)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	const uint8_t *payload_ptr;
	size_t payload_size;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */
	suit_component_type_t src_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	/* Get destination component type based on component handle*/
	int ret = suit_plat_component_type_get(dst_handle, &dst_component_type);

	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode destination component type");
		return ret;
	}

	/* Check if destination component type is supported */
	if ((dst_component_type != SUIT_COMPONENT_TYPE_MEM) &&
	    (dst_component_type != SUIT_COMPONENT_TYPE_SOC_SPEC)) {
		LOG_ERR("Unsupported destination component type");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Get source component type based on component handle*/
	ret = suit_plat_component_type_get(src_handle, &src_component_type);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode source component type");
		return ret;
	}

	/* Check if source component type is supported */
	if ((src_component_type != SUIT_COMPONENT_TYPE_MEM) &&
	    (src_component_type != SUIT_COMPONENT_TYPE_CAND_IMG)) {
		LOG_ERR("Unsupported source component type");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	ret = suit_sink_select(dst_handle, &dst_sink);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("suit_sink_select failed - error %i", ret);
		return ret;
	}

	/* Here other parts of pipe will be instantiated.
	 *	Like decryption and/or decompression sinks.
	 */

	/* Select source based on component type */
	switch (src_component_type) {
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	case SUIT_COMPONENT_TYPE_MEM:
	case SUIT_COMPONENT_TYPE_CAND_IMG: {
		memptr_storage_handle_t handle = NULL;

		ret = suit_plat_component_impl_data_get(src_handle, &handle);

		if (ret != SUIT_SUCCESS) {
			LOG_ERR("suit_plat_component_impl_data_get failed - error %i", ret);
			release_sink(&dst_sink);
			return ret;
		}

		ret = suit_memptr_storage_ptr_get(handle, &payload_ptr, &payload_size);
		ret = suit_plat_err_to_processor_err_convert(ret);

		if (ret != SUIT_SUCCESS) {
			LOG_ERR("suit_memptr_storage_ptr_get failed - error %i", ret);
			release_sink(&dst_sink);
			return ret;
		}
	} break;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	default:
		ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		break;
	}

	ret = release_sink(&dst_sink);
	return suit_plat_err_to_processor_err_convert(ret);
#else  /* CONFIG_SUIT_STREAM */
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM */
}

int suit_plat_copy(suit_component_t dst_handle, suit_component_t src_handle)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	const uint8_t *payload_ptr;
	size_t payload_size;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */
	suit_component_type_t src_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	/* Get destination component type based on component handle*/
	int ret = suit_plat_component_type_get(dst_handle, &dst_component_type);

	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode destination component type");
		return ret;
	}

	/* Check if destination component type is supported */
	if ((dst_component_type != SUIT_COMPONENT_TYPE_MEM) &&
	    (dst_component_type != SUIT_COMPONENT_TYPE_SOC_SPEC)) {
		LOG_ERR("Unsupported destination component type");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Get source component type based on component handle*/
	ret = suit_plat_component_type_get(src_handle, &src_component_type);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode source component type");
		return ret;
	}

	/* Check if source component type is supported */
	if ((src_component_type != SUIT_COMPONENT_TYPE_MEM) &&
	    (src_component_type != SUIT_COMPONENT_TYPE_CAND_IMG)) {
		LOG_ERR("Unsupported source component type");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Select destination */
	ret = suit_sink_select(dst_handle, &dst_sink);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("suit_sink_select failed - error %i", ret);
		return ret;
	}

	/* Here other parts of pipe will be instantiated.
	 *	Like decryption and/or decompression sinks.
	 */

#if CONFIG_SUIT_DIGEST_CACHE
	/* Invalidate the cache entry of the digest for the destination. */
	ret = suit_plat_digest_cache_remove_by_handle(dst_handle);

	if (ret != SUIT_SUCCESS) {
		return ret;
	}
#endif

	/* Select source based on component type */
	switch (src_component_type) {
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	case SUIT_COMPONENT_TYPE_MEM:
	case SUIT_COMPONENT_TYPE_CAND_IMG: {
		memptr_storage_handle_t handle = NULL;

		ret = suit_plat_component_impl_data_get(src_handle, &handle);

		if (ret != SUIT_SUCCESS) {
			LOG_ERR("suit_plat_component_impl_data_get failed - error %i", ret);
			release_sink(&dst_sink);
			return ret;
		}

		ret = suit_memptr_storage_ptr_get(handle, &payload_ptr, &payload_size);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("suit_memptr_storage_ptr_get failed - error %i", ret);
			release_sink(&dst_sink);
			return suit_plat_err_to_processor_err_convert(ret);
		}

		if (dst_sink.erase != NULL) {
			ret = dst_sink.erase(dst_sink.ctx);
			if (ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Failed to erase destination sink: %d", ret);
				release_sink(&dst_sink);
				return suit_plat_err_to_processor_err_convert(ret);
			}

			LOG_DBG("dst_sink erased");
		}

		ret = suit_generic_address_streamer_stream(payload_ptr, payload_size, &dst_sink);

		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("memptr_streamer failed - error %i", ret);
		}

		ret = suit_plat_err_to_processor_err_convert(ret);
	} break;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	default:
		ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		break;
	}

	if (ret == SUIT_SUCCESS) {
		/* Update size in memptr for MEM component */
		if (dst_component_type == SUIT_COMPONENT_TYPE_MEM) {
			size_t new_size = 0;

			ret = dst_sink.used_storage(dst_sink.ctx, &new_size);
			if (ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Getting used storage on destination sink failed");
				release_sink(&dst_sink);

				return suit_plat_err_to_processor_err_convert(ret);
			}

			ret = suit_plat_memptr_size_update(dst_handle, new_size);
			if (ret != SUIT_SUCCESS) {
				LOG_ERR("Failed to update destination MEM component size: %i", ret);
				release_sink(&dst_sink);

				return ret;
			}
		}

		ret = release_sink(&dst_sink);
		return suit_plat_err_to_processor_err_convert(ret);
	}

	release_sink(&dst_sink);
	return suit_plat_err_to_processor_err_convert(ret);
#else  /* CONFIG_SUIT_STREAM */
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM */
}
