/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <zephyr/logging/log.h>

#include <suit_platform.h>
#include <suit_platform_internal.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_fetch_domain_specific.h>
#include <suit_plat_err.h>
#include <suit_plat_error_convert.h>

#ifdef CONFIG_SUIT_STREAM
#include <suit_sink.h>
#endif /* CONFIG_SUIT_STREAM */

#ifdef CONFIG_SUIT_STREAM_SINK_MEMPTR
#include <suit_memptr_storage.h>
#include <suit_memptr_sink.h>
#endif /* CONFIG_SUIT_STREAM_SINK_MEMPTR */

#ifdef CONFIG_SUIT_STREAM_FILTER_DECRYPT
#include <suit_decrypt_filter.h>
#endif /* CONFIG_SUIT_STREAM_FILTER_DECRYPT */

#ifdef CONFIG_SUIT_STREAM_SOURCE_CACHE
#include <suit_dfu_cache_streamer.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_CACHE */

#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
#include <suit_memptr_streamer.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

LOG_MODULE_REGISTER(suit_plat_fetch, CONFIG_SUIT_LOG_LEVEL);

#ifdef CONFIG_SUIT_STREAM_SINK_MEMPTR
/** @brief A common data streamer type for both integrated and non-integrated fetch.
 *
 * @details The streamer API may receive either URI or the raw payload, depending on the
 *          context. This type is defined only to simplify/shorten the implementation.
 *          It is possible to use it here because the only part of the fetch logic,
 *          that depends on the input source (URI/payload) is the streamer.
 *
 * @param[in]  src   Pointer to the URI/payload to stream.
 * @param[in]  lin   Length of the URI/payload.
 * @param[in]  sink  Output sink to write streamed data.
 *
 * @returns SUIT_SUCCESS on success, streamer-specific error code otherwise.
 */
typedef int (*data_streamer)(const uint8_t *src, size_t len, struct stream_sink *sink);

/** @brief Streamer function for non-integrated fetches. */
static int external_streamer(const uint8_t *uri, size_t uri_len, struct stream_sink *sink)
{
#ifndef CONFIG_SUIT_STREAM_SOURCE_CACHE
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#else
	suit_plat_err_t plat_ret = suit_dfu_cache_streamer_stream(uri, uri_len, sink);

	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Streaming from cache failed: %d", plat_ret);
	}

	/* Failures of dfu_cache streamer are treated as "unavailable payload"
	 * failures. These are cases where suit-condition-image-match will detect
	 * the failure, however suit-plat-fetch should return success to allow
	 * soft failures.
	 */
	return SUIT_SUCCESS;
#endif
}

/** @brief Streamer function for integrated payload fetches. */
static inline int internal_streamer(const uint8_t *payload, size_t payload_len,
				    struct stream_sink *sink)
{
#ifndef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#else
	return suit_memptr_streamer_stream(payload, payload_len, sink);
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */
}

/** @brief Common implementation for fetching data into candidate components. */
static int suit_plat_fetch_memptr(suit_component_t dst_handle, struct zcbor_string *uri,
				  struct zcbor_string *manifest_component_id,
				  struct suit_encryption_info *enc_info, data_streamer streamer,
				  bool dry_run)
{
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	struct stream_sink dst_sink = {0};
	struct zcbor_string *component_id = NULL;
	memptr_storage_handle_t handle;
	int ret = SUIT_SUCCESS;
	uint32_t number;

	/*
	 * Validate streaming operation.
	 */

	ret = suit_plat_component_id_get(dst_handle, &component_id);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to parse component ID from handle: %d", ret);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_component_number(component_id, &number) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Missing component id number in candidate component");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	ret = suit_plat_component_impl_data_get(dst_handle, &handle);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Unable to get component data for candidate component: %d", ret);
		return ret;
	}

	/*
	 * Construct the stream.
	 */

	plat_ret = suit_memptr_sink_get(&dst_sink, handle);
	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to create memptr sink: %d", plat_ret);
		ret = suit_plat_err_to_processor_err_convert(plat_ret);
		return ret;
	}

	/* Append decryption filter if encryption info is provided. */
	if (enc_info != NULL) {
#ifdef CONFIG_SUIT_STREAM_FILTER_DECRYPT
		suit_manifest_class_id_t *class_id = NULL;

		if (suit_plat_decode_manifest_class_id(manifest_component_id, &class_id) !=
		    SUIT_PLAT_SUCCESS) {
			LOG_ERR("Manifest component ID is not a manifest class");
			ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		if (ret == SUIT_SUCCESS) {
			plat_ret =
				suit_decrypt_filter_get(&dst_sink, enc_info, class_id, &dst_sink);
			if (plat_ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Selecting decryption filter failed: %d", plat_ret);
				ret = suit_plat_err_to_processor_err_convert(plat_ret);
			}
		}
#else
		LOG_ERR("Decryption while streaming to candidate component is not supported");
		(void)manifest_component_id;
		ret = SUIT_ERR_UNSUPPORTED_PARAMETER;
#endif /* CONFIG_SUIT_STREAM_FILTER_DECRYPT */
	}

	if (!dry_run) {
		/*
		 * Stream the data.
		 */

		/* Erase the destination memory area. */
		if ((ret == SUIT_SUCCESS) && (dst_sink.erase != NULL)) {
			plat_ret = dst_sink.erase(dst_sink.ctx);
			if (plat_ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Sink erase failed: %d", plat_ret);
				ret = suit_plat_err_to_processor_err_convert(plat_ret);
			}
		}

		/* Start streaming the data. */
		if (ret == SUIT_SUCCESS) {
			ret = streamer(uri->value, uri->len, &dst_sink);
		}

		if ((ret == SUIT_SUCCESS) && (dst_sink.flush != NULL)) {
			plat_ret = dst_sink.flush(dst_sink.ctx);
			if (plat_ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Sink flush failed: %d", plat_ret);
				ret = suit_plat_err_to_processor_err_convert(plat_ret);
			}
		}
	}

	/*
	 * Destroy the stream.
	 */

	plat_ret = release_sink(&dst_sink);
	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Sink release failed: %d", plat_ret);
	}

	if (ret == SUIT_SUCCESS) {
		ret = suit_plat_err_to_processor_err_convert(plat_ret);
	}

	return ret;
}
#endif /* CONFIG_SUIT_STREAM_SINK_MEMPTR */

int suit_plat_check_fetch(suit_component_t dst_handle, struct zcbor_string *uri,
			  struct zcbor_string *manifest_component_id,
			  struct suit_encryption_info *enc_info)
{
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	/* Decode destination component type based on component handle. */
	int ret = suit_plat_component_type_get(dst_handle, &dst_component_type);

	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode component type: %d", ret);
		return ret;
	}

	if (uri == NULL) {
		LOG_ERR("Empty URI");
		return SUIT_ERR_UNAVAILABLE_PAYLOAD;
	}

	switch (dst_component_type) {
#ifdef CONFIG_SUIT_STREAM_SINK_MEMPTR
	case SUIT_COMPONENT_TYPE_CAND_IMG:
	case SUIT_COMPONENT_TYPE_CAND_MFST:
		return suit_plat_fetch_memptr(dst_handle, uri, manifest_component_id, enc_info,
					      external_streamer, true);
#endif /* CONFIG_SUIT_STREAM_SINK_MEMPTR */
	default:
		break;
	}

	if (suit_plat_fetch_domain_specific_is_type_supported(dst_component_type)) {
		return suit_plat_check_fetch_domain_specific(dst_handle, dst_component_type, uri,
							     manifest_component_id, enc_info);
	}

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

int suit_plat_fetch(suit_component_t dst_handle, struct zcbor_string *uri,
		    struct zcbor_string *manifest_component_id,
		    struct suit_encryption_info *enc_info)
{
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	/* Decode destination component type based on component handle. */
	int ret = suit_plat_component_type_get(dst_handle, &dst_component_type);

	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode component type: %d", ret);
		return ret;
	}

	if (uri == NULL) {
		LOG_ERR("Empty URI");
		return SUIT_ERR_UNAVAILABLE_PAYLOAD;
	}

	switch (dst_component_type) {
#ifdef CONFIG_SUIT_STREAM_SINK_MEMPTR
	case SUIT_COMPONENT_TYPE_CAND_IMG:
	case SUIT_COMPONENT_TYPE_CAND_MFST:
		return suit_plat_fetch_memptr(dst_handle, uri, manifest_component_id, enc_info,
					      external_streamer, false);
#endif /* CONFIG_SUIT_STREAM_SINK_MEMPTR */
	default:
		break;
	}

	if (suit_plat_fetch_domain_specific_is_type_supported(dst_component_type)) {
		return suit_plat_fetch_domain_specific(dst_handle, dst_component_type, uri,
						       manifest_component_id, enc_info);
	}

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

int suit_plat_check_fetch_integrated(suit_component_t dst_handle, struct zcbor_string *payload,
				     struct zcbor_string *manifest_component_id,
				     struct suit_encryption_info *enc_info)
{
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	/* Decode destination component type based on component handle. */
	int ret = suit_plat_component_type_get(dst_handle, &dst_component_type);

	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode component type: %i", ret);
		return ret;
	}

	if (payload == NULL) {
		LOG_ERR("Empty payload");
		return SUIT_ERR_UNAVAILABLE_PAYLOAD;
	}

	switch (dst_component_type) {
#ifdef CONFIG_SUIT_STREAM_SINK_MEMPTR
	case SUIT_COMPONENT_TYPE_CAND_IMG:
	case SUIT_COMPONENT_TYPE_CAND_MFST:
		return suit_plat_fetch_memptr(dst_handle, payload, manifest_component_id, enc_info,
					      internal_streamer, true);
#endif /* CONFIG_SUIT_STREAM_SINK_MEMPTR */
	default:
		break;
	}

	if (suit_plat_fetch_integrated_domain_specific_is_type_supported(dst_component_type)) {
		return suit_plat_check_fetch_integrated_domain_specific(
			dst_handle, dst_component_type, payload, manifest_component_id, enc_info);
	}

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

int suit_plat_fetch_integrated(suit_component_t dst_handle, struct zcbor_string *payload,
			       struct zcbor_string *manifest_component_id,
			       struct suit_encryption_info *enc_info)
{
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	/* Decode destination component type based on component handle. */
	int ret = suit_plat_component_type_get(dst_handle, &dst_component_type);

	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode component type: %i", ret);
		return ret;
	}

	if (payload == NULL) {
		LOG_ERR("Empty payload");
		return SUIT_ERR_UNAVAILABLE_PAYLOAD;
	}

	switch (dst_component_type) {
#ifdef CONFIG_SUIT_STREAM_SINK_MEMPTR
	case SUIT_COMPONENT_TYPE_CAND_IMG:
	case SUIT_COMPONENT_TYPE_CAND_MFST:
		return suit_plat_fetch_memptr(dst_handle, payload, manifest_component_id, enc_info,
					      internal_streamer, false);
#endif /* CONFIG_SUIT_STREAM_SINK_MEMPTR */
	default:
		break;
	}

	if (suit_plat_fetch_integrated_domain_specific_is_type_supported(dst_component_type)) {
		return suit_plat_fetch_integrated_domain_specific(
			dst_handle, dst_component_type, payload, manifest_component_id, enc_info);
	}

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}
