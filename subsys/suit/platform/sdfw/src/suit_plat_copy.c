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

#if CONFIG_SUIT_IPUC
#include <suit_plat_ipuc.h>
#endif /* CONFIG_SUIT_IPUC */

#ifdef CONFIG_SUIT_STREAM
#include <suit_sink.h>
#include <suit_sink_selector.h>
#include <suit_generic_address_streamer.h>
#endif /* CONFIG_SUIT_STREAM */

#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
#include <suit_memptr_storage.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

#ifdef CONFIG_SUIT_STREAM_FILTER_DECRYPT
#include <suit_decrypt_filter.h>
#endif /* CONFIG_SUIT_STREAM_FILTER_DECRYPT */

LOG_MODULE_REGISTER(suit_plat_copy, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_check_copy(suit_component_t dst_handle, suit_component_t src_handle,
			 struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	const uint8_t *payload_ptr;
	size_t payload_size;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */
	suit_component_type_t src_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	int ret = SUIT_SUCCESS;

	/*
	 * Validate streaming operation.
	 */

	/* Get destination component type based on component handle*/
	ret = suit_plat_component_type_get(dst_handle, &dst_component_type);
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

	/*
	 * Try to construct the stream.
	 */

	/* Select destination */
	ret = suit_sink_select(dst_handle, &dst_sink);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("suit_sink_select failed - error %i", ret);
		return ret;
	}

	/* Append decryption filter if encryption info is provided. */
	if (enc_info != NULL) {
#ifdef CONFIG_SUIT_STREAM_FILTER_DECRYPT
		ret = suit_decrypt_filter_get(&dst_sink, enc_info, &dst_sink);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Selecting decryption filter failed: %i", ret);
		}
#else
		ret = SUIT_ERR_UNSUPPORTED_PARAMETER;
#endif /* CONFIG_SUIT_STREAM_FILTER_DECRYPT */
	}

#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	/* Check if it spossible to read source address and size */
	if (ret == SUIT_SUCCESS) {
		memptr_storage_handle_t handle = NULL;

		ret = suit_plat_component_impl_data_get(src_handle, &handle);
		if (ret != SUIT_SUCCESS) {
			LOG_ERR("suit_plat_component_impl_data_get failed - error %i", ret);
		} else {
			plat_ret = suit_memptr_storage_ptr_get(handle, &payload_ptr, &payload_size);
			if (plat_ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("suit_memptr_storage_ptr_get failed - error %i", plat_ret);
				ret = suit_plat_err_to_processor_err_convert(plat_ret);
			}
		}
	}
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	/*
	 * Destroy the stream.
	 */

	plat_ret = release_sink(&dst_sink);
	if (ret == SUIT_SUCCESS) {
		ret = suit_plat_err_to_processor_err_convert(plat_ret);
	}

	return ret;
#else  /* CONFIG_SUIT_STREAM */
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM */
}

int suit_plat_copy(suit_component_t dst_handle, suit_component_t src_handle,
		   struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	const uint8_t *payload_ptr;
	size_t payload_size;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */
	suit_component_type_t src_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	int ret = SUIT_SUCCESS;

	/*
	 * Validate streaming operation.
	 */

	/* Get destination component type based on component handle*/
	ret = suit_plat_component_type_get(dst_handle, &dst_component_type);
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

	/*
	 * Construct the stream.
	 */

	/* Select destination */
	ret = suit_sink_select(dst_handle, &dst_sink);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("suit_sink_select failed - error %i", ret);
		return ret;
	}

	/* Append decryption filter if encryption info is provided. */
	if (enc_info != NULL) {
#ifdef CONFIG_SUIT_STREAM_FILTER_DECRYPT
		ret = suit_decrypt_filter_get(&dst_sink, enc_info, &dst_sink);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Selecting decryption filter failed: %i", ret);
		}
#else
		ret = SUIT_ERR_UNSUPPORTED_PARAMETER;
#endif /* CONFIG_SUIT_STREAM_FILTER_DECRYPT */
	}

	/*
	 * Stream the data.
	 */

#if CONFIG_SUIT_IPUC
	suit_plat_ipuc_revoke(dst_handle);
#endif /* CONFIG_SUIT_IPUC */

#if CONFIG_SUIT_DIGEST_CACHE
	/* Invalidate digest cache for the destination component. */
	if (ret == SUIT_SUCCESS) {
		ret = suit_plat_digest_cache_remove_by_handle(dst_handle);
		if (ret != SUIT_SUCCESS) {
			LOG_ERR("Invalidating digest cache failed: %i", ret);
		}
	}
#endif

	/* Erase the destination memory area. */
	if ((ret == SUIT_SUCCESS) && (dst_sink.erase != NULL)) {
		plat_ret = dst_sink.erase(dst_sink.ctx);
		if (plat_ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Sink mem erase failed, err code: %d", plat_ret);
			ret = suit_plat_err_to_processor_err_convert(plat_ret);
		}
	}

#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	/* Currently all supported source types can be handled with generic address streamer. */
	memptr_storage_handle_t handle = NULL;

	if (ret == SUIT_SUCCESS) {
		ret = suit_plat_component_impl_data_get(src_handle, &handle);
		if (ret != SUIT_SUCCESS) {
			LOG_ERR("suit_plat_component_impl_data_get failed - error %i", ret);
		}
	}

	if (ret == SUIT_SUCCESS) {
		plat_ret = suit_memptr_storage_ptr_get(handle, &payload_ptr, &payload_size);
		if (plat_ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("suit_memptr_storage_ptr_get failed - error %i", plat_ret);
			ret = suit_plat_err_to_processor_err_convert(plat_ret);
		}
	}

	if (ret == SUIT_SUCCESS) {
		ret = suit_generic_address_streamer_stream(payload_ptr, payload_size, &dst_sink);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("memptr_streamer failed - error %i", ret);
		}
	}
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	/* Flush any remaining data before reading used storage size */
	if ((ret == SUIT_SUCCESS) && (dst_sink.flush != NULL)) {
		plat_ret = dst_sink.flush(dst_sink.ctx);
		if (plat_ret != SUIT_PLAT_SUCCESS) {
			ret = suit_plat_err_to_processor_err_convert(ret);
		}
	}

	/* Update size in memptr for MEM component */
	if ((ret == SUIT_SUCCESS) && (dst_component_type == SUIT_COMPONENT_TYPE_MEM)) {
		size_t new_size = 0;

		plat_ret = dst_sink.used_storage(dst_sink.ctx, &new_size);
		if (plat_ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Getting used storage on destination sink failed");
			ret = suit_plat_err_to_processor_err_convert(plat_ret);
		} else {
			ret = suit_plat_memptr_size_update(dst_handle, new_size);
			if (ret != SUIT_SUCCESS) {
				LOG_ERR("Failed to update destination MEM component size: %i", ret);
			}
		}
	}

	/*
	 * Destroy the stream.
	 */

	plat_ret = release_sink(&dst_sink);
	if (ret == SUIT_SUCCESS) {
		ret = suit_plat_err_to_processor_err_convert(plat_ret);
	}

	return ret;
#else  /* CONFIG_SUIT_STREAM */
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM */
}
