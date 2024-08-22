/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_platform_internal.h>
#include <suit_plat_fetch_domain_specific.h>
#include <suit_plat_error_convert.h>

#if CONFIG_SUIT_DIGEST_CACHE
#include <suit_plat_digest_cache.h>
#endif /* CONFIG_SUIT_DIGEST_CACHE */

#ifdef CONFIG_SUIT_STREAM
#include <suit_sink.h>
#include <suit_sink_selector.h>
#endif /* CONFIG_SUIT_STREAM */

#ifdef CONFIG_SUIT_STREAM_SINK_MEMPTR
#include <suit_memptr_sink.h>
#endif /* CONFIG_SUIT_STREAM_SINK_MEMPTR */
#ifdef CONFIG_SUIT_CACHE_RW
#include <suit_dfu_cache_sink.h>
#endif

#ifdef CONFIG_SUIT_STREAM_FILTER_DECRYPT
#include <suit_decrypt_filter.h>
#endif /* CONFIG_SUIT_STREAM_FILTER_DECRYPT */

#include <stdbool.h>
#include <suit_platform.h>
#include <suit_memptr_storage.h>
#include <suit_plat_decode_util.h>

LOG_MODULE_REGISTER(suit_plat_fetch, CONFIG_SUIT_LOG_LEVEL);

#ifdef CONFIG_SUIT_STREAM

static int verify_and_get_sink(suit_component_t dst_handle, struct stream_sink *sink,
			       struct zcbor_string *uri, suit_component_type_t *component_type,
			       bool write_enabled)
{
	int ret = suit_plat_component_type_get(dst_handle, component_type);

	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode component type: %i", ret);
		return ret;
	}
	if (!suit_plat_fetch_domain_specific_is_type_supported(*component_type)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Select sink */
#ifdef CONFIG_SUIT_CACHE_RW
	if (*component_type == SUIT_COMPONENT_TYPE_CACHE_POOL) {
		uint32_t number;
		struct zcbor_string *component_id;

		int ret = suit_plat_component_id_get(dst_handle, &component_id);

		if (ret != SUIT_SUCCESS) {
			LOG_ERR("suit_plat_component_id_get failed: %i", ret);
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		if (suit_plat_decode_component_number(component_id, &number) != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Missing component id number in candidate image component");
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		suit_plat_err_t plat_ret =
			suit_dfu_cache_sink_get(sink, number, uri->value, uri->len, write_enabled);

		if (plat_ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Getting cache sink failed");
		}

		return suit_plat_err_to_processor_err_convert(plat_ret);
	}
#else
	(void)write_enabled;
#endif /* CONFIG_SUIT_CACHE_RW */

	return suit_sink_select(dst_handle, sink);
}
#endif /* CONFIG_SUIT_STREAM */

int suit_plat_check_fetch(suit_component_t dst_handle, struct zcbor_string *uri,
			  struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	struct stream_sink dst_sink = {0};
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	int ret = SUIT_SUCCESS;

	/*
	 * Validate streaming operation.
	 */
	ret = verify_and_get_sink(dst_handle, &dst_sink, uri, &dst_component_type, false);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to verify component and get end sink");
		return ret;
	}

	/*
	 * Try to Construct the stream.
	 */

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
	 * Destroy the stream.
	 */

	plat_ret = release_sink(&dst_sink);
	if (ret == SUIT_SUCCESS) {
		ret = suit_plat_err_to_processor_err_convert(plat_ret);
	}

	return ret;
#endif /* CONFIG_SUIT_STREAM */

	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

int suit_plat_fetch(suit_component_t dst_handle, struct zcbor_string *uri,
		    struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink = {0};
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	int ret = SUIT_SUCCESS;

	/*
	 * Validate streaming operation.
	 */

	ret = verify_and_get_sink(dst_handle, &dst_sink, uri, &dst_component_type, true);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to verify component end get end sink");
		return ret;
	}

	/*
	 * Construct the stream.
	 */

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

#if CONFIG_SUIT_DIGEST_CACHE
	/* Invalidate digest cache for the destination component. */
	ret = suit_plat_digest_cache_remove_by_handle(dst_handle);

	if (ret != SUIT_SUCCESS) {
		return ret;
	}
#endif

	/* Erase the destination memory area. */
	if (dst_sink.erase != NULL) {
		ret = dst_sink.erase(dst_sink.ctx);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Sink mem erase failed: %i", ret);
		}
	}

	/* Start streaming the data. */
	if (ret == SUIT_SUCCESS) {
		ret = suit_plat_fetch_domain_specific(dst_handle, dst_component_type, &dst_sink,
						      uri);
	}

	/*
	 * Destroy the stream.
	 */

	plat_ret = release_sink(&dst_sink);
	if (ret == SUIT_SUCCESS) {
		ret = suit_plat_err_to_processor_err_convert(plat_ret);
	}

	return ret;
#else
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM */
}

int suit_plat_check_fetch_integrated(suit_component_t dst_handle, struct zcbor_string *payload,
				     struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	int ret = SUIT_SUCCESS;

	/*
	 * Validate streaming operation.
	 */

	/* Decode destination component type based on component handle. */
	ret = suit_plat_component_type_get(dst_handle, &dst_component_type);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode component type: %i", ret);
		return ret;
	}

	/* Check if destination component is supported by the fetch operation. */
	if (!suit_plat_fetch_integrated_domain_specific_is_type_supported(dst_component_type)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

#ifndef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	/*
	 * Try to construct the stream.
	 */

	/* Create destination sink. */
	ret = suit_sink_select(dst_handle, &dst_sink);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Selecting sink failed: %i", ret);
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
	 * Destroy the stream.
	 */

	plat_ret = release_sink(&dst_sink);
	if (ret == SUIT_SUCCESS) {
		ret = suit_plat_err_to_processor_err_convert(plat_ret);
	}

	return ret;
#else
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM */
}

int suit_plat_fetch_integrated(suit_component_t dst_handle, struct zcbor_string *payload,
			       struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM
	struct stream_sink dst_sink;
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	int ret = SUIT_SUCCESS;

	/*
	 * Validate streaming operation.
	 */

	/* Decode destination component type based on component handle. */
	ret = suit_plat_component_type_get(dst_handle, &dst_component_type);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode component type: %i", ret);
		return ret;
	}

	/* Check if destination component is supported by the fetch operation. */
	if (!suit_plat_fetch_integrated_domain_specific_is_type_supported(dst_component_type)) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

#ifndef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	/*
	 * Construct the stream.
	 */

	/* Create destination sink. */
	ret = suit_sink_select(dst_handle, &dst_sink);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Selecting sink failed: %i", ret);
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

	/* Start streaming the data. */
	if (ret == SUIT_SUCCESS) {
		ret = suit_plat_fetch_integrated_domain_specific(dst_handle, dst_component_type,
								 &dst_sink, payload);
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
