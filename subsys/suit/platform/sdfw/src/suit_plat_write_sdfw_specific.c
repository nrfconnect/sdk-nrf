/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <stdbool.h>
#include <suit_plat_write_domain_specific.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_error_convert.h>
#include <suit_platform_internal.h>
#include <suit_plat_digest_cache.h>
#include <suit_plat_memptr_size_update.h>

#ifdef CONFIG_SUIT_IPUC
#include <suit_ipuc_sdfw.h>
#endif /* CONFIG_SUIT_IPUC */

#ifdef CONFIG_SUIT_STREAM
#include <suit_sink.h>
#include <suit_sink_selector.h>
#endif /* CONFIG_SUIT_STREAM */

#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
#include <suit_memptr_streamer.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

#ifdef CONFIG_SUIT_STREAM_FILTER_DECRYPT
#include <suit_decrypt_filter.h>
#endif /* CONFIG_SUIT_STREAM_FILTER_DECRYPT */

LOG_MODULE_DECLARE(suit_plat_write, CONFIG_SUIT_LOG_LEVEL);

bool suit_plat_write_domain_specific_is_type_supported(suit_component_type_t component_type)
{
#ifdef CONFIG_SUIT_STREAM
	/* Check if destination component type is supported */
	if (component_type == SUIT_COMPONENT_TYPE_MEM) {
		return true;
	}
#endif /* CONFIG_SUIT_STREAM */

	return false;
}

int suit_plat_check_write_domain_specific(suit_component_t dst_handle,
					  suit_component_type_t dst_component_type,
					  struct zcbor_string *content,
					  struct zcbor_string *manifest_component_id,
					  struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	int ret = SUIT_SUCCESS;

	/*
	 * Validate streaming operation.
	 */

	if (content == NULL) {
		return suit_plat_err_to_processor_err_convert(SUIT_PLAT_ERR_INVAL);
	}

	/* Check if destination component type is supported */
	if (dst_component_type != SUIT_COMPONENT_TYPE_MEM) {
		LOG_ERR("Unsupported destination component type");
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
		suit_manifest_class_id_t *class_id = NULL;

		if (suit_plat_decode_manifest_class_id(manifest_component_id, &class_id) !=
		    SUIT_PLAT_SUCCESS) {
			LOG_ERR("Component ID is not a manifest class");
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		ret = suit_decrypt_filter_get(&dst_sink, enc_info, class_id, &dst_sink);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Selecting decryption filter failed: %i", ret);
		}
#else
		ret = SUIT_ERR_UNSUPPORTED_PARAMETER;
#endif /* CONFIG_SUIT_STREAM_FILTER_DECRYPT */
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

int suit_plat_write_domain_specific(suit_component_t dst_handle,
				    suit_component_type_t dst_component_type,
				    struct zcbor_string *content,
				    struct zcbor_string *manifest_component_id,
				    struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	int ret = SUIT_SUCCESS;

	/*
	 * Validate streaming operation.
	 */

	if (content == NULL) {
		return suit_plat_err_to_processor_err_convert(SUIT_PLAT_ERR_INVAL);
	}

	/* Check if destination component type is supported */
	if (dst_component_type != SUIT_COMPONENT_TYPE_MEM) {
		LOG_ERR("Unsupported destination component type");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

#ifndef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

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
		suit_manifest_class_id_t *class_id = NULL;

		if (suit_plat_decode_manifest_class_id(manifest_component_id, &class_id) !=
		    SUIT_PLAT_SUCCESS) {
			LOG_ERR("Component ID is not a manifest class");
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		ret = suit_decrypt_filter_get(&dst_sink, enc_info, class_id, &dst_sink);
		if (ret != SUIT_SUCCESS) {
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
	suit_ipuc_sdfw_revoke(dst_handle);
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
	/* Start streaming the data. */
	if (ret == SUIT_SUCCESS) {
		plat_ret = suit_memptr_streamer_stream(content->value, content->len, &dst_sink);
		if (plat_ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("memptr_streamer failed - error %i", plat_ret);
			ret = suit_plat_err_to_processor_err_convert(plat_ret);
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
