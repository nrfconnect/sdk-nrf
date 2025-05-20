/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <zephyr/logging/log.h>

#include <suit_plat_decode_util.h>
#include <suit_plat_fetch_domain_specific.h>
#include <suit_plat_err.h>
#include <suit_plat_error_convert.h>
#include <suit_plat_memptr_size_update.h>

#ifdef CONFIG_SUIT_STREAM
#include <suit_sink.h>
#endif /* CONFIG_SUIT_STREAM */

#ifdef CONFIG_SUIT_SINK_SELECTOR
#include <suit_sink_selector.h>
#endif /* CONFIG_SUIT_SINK_SELECTOR */

#ifdef CONFIG_SUIT_STREAM_FILTER_DECRYPT
#include <suit_decrypt_filter.h>
#endif /* CONFIG_SUIT_STREAM_FILTER_DECRYPT */

#ifdef CONFIG_SUIT_STREAM_SOURCE_CACHE
#include <suit_dfu_cache_streamer.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_CACHE */

#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
#include <suit_memptr_streamer.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

#ifdef CONFIG_SUIT_STREAM_IPC_REQUESTOR
#include <suit_ipc_streamer.h>
#endif /* CONFIG_SUIT_STREAM_IPC_REQUESTOR */

#if CONFIG_SUIT_DIGEST_CACHE
#include <suit_plat_digest_cache.h>
#endif /* CONFIG_SUIT_DIGEST_CACHE */

#if defined(CONFIG_SUIT_IPUC)
#include <suit_ipuc_sdfw.h>
#endif /* CONFIG_SUIT_IPUC */

LOG_MODULE_REGISTER(suit_plat_fetch_sdfw, CONFIG_SUIT_LOG_LEVEL);

#ifdef CONFIG_SUIT_STREAM
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

/** @brief Streamer function for non-integrated fetches into MEM components. */
static int external_streamer(const uint8_t *uri, size_t uri_len, struct stream_sink *sink)
{
	int ret = SUIT_PLAT_ERR_NOT_FOUND;

#ifdef CONFIG_SUIT_STREAM_SOURCE_CACHE
	/* Check if requested uri exists in cache and get streamer */
	ret = suit_dfu_cache_streamer_stream(uri, uri_len, sink);
#endif /* CONFIG_SUIT_STREAM_SOURCE_CACHE */

#ifdef CONFIG_SUIT_STREAM_IPC_REQUESTOR
	if (ret == SUIT_PLAT_ERR_NOT_FOUND) { /* URI was not found in cache */
		/* Request uri through ipc streamer */
		ret = suit_ipc_streamer_stream(uri, uri_len, sink,
					       CONFIG_SUIT_STREAM_IPC_STREAMER_CHUNK_TIMEOUT,
					       CONFIG_SUIT_STREAM_IPC_STREAMER_REQUESTING_PERIOD);
		if (ret != SUIT_PLAT_SUCCESS) {
			ret = SUIT_PLAT_ERR_NOT_FOUND;
		}
	}
#endif /* CONFIG_SUIT_STREAM_IPC_REQUESTOR */

	if (ret == SUIT_PLAT_ERR_NOT_FOUND) {
		/* If we arrived here we can treat the source data as unavailable.
		 * This is a case where suit-condition-image-match will detect
		 * the failure, however suit-plat-fetch should return success to
		 * allow soft failures.
		 */
		return SUIT_SUCCESS;
	}

	return ret;
}

/** @brief Streamer function for non-integrated fetches into SoC-specific components. */
static int external_sdfw_streamer(const uint8_t *uri, size_t uri_len, struct stream_sink *sink)
{
	int ret = SUIT_PLAT_ERR_NOT_FOUND;

#ifdef CONFIG_SUIT_STREAM_SOURCE_CACHE
	/* Check if requested uri exists in cache and get streamer */
	ret = suit_dfu_cache_streamer_stream(uri, uri_len, sink);
#endif /* CONFIG_SUIT_STREAM_SOURCE_CACHE */

	if (ret == SUIT_PLAT_ERR_NOT_FOUND) {
		/* If we arrived here we can treat the source data as unavailable.
		 * This is a case where suit-condition-image-match will detect
		 * the failure, however suit-plat-fetch should return success to
		 * allow soft failures.
		 */
		return SUIT_SUCCESS;
	}

	return ret;
}

/** @brief Streamer function for integrated payload fetches. */
#define internal_streamer suit_memptr_streamer_stream

/** @brief Common implementation for fetching data into MEM components. */
static int suit_plat_fetch_mem(suit_component_t dst_handle, struct zcbor_string *uri,
			       struct zcbor_string *manifest_component_id,
			       struct suit_encryption_info *enc_info, data_streamer streamer,
			       bool dry_run)
{
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	struct stream_sink dst_sink = {0};
	int ret = SUIT_SUCCESS;

	/*
	 * Validate streaming operation.
	 */

	/*
	 * Construct the stream.
	 */

	/* Select sink */
	ret = suit_sink_select(dst_handle, &dst_sink);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Selecting sink failed: %i", ret);
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
		LOG_ERR("Decryption while streaming to MEM component is not supported");
		(void)manifest_component_id;
		ret = SUIT_ERR_UNSUPPORTED_PARAMETER;
#endif /* CONFIG_SUIT_STREAM_FILTER_DECRYPT */
	}

	if (!dry_run) {
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

		/* Update size in memptr for MEM component */
		if (ret == SUIT_SUCCESS) {
			size_t new_size = 0;

			plat_ret = dst_sink.used_storage(dst_sink.ctx, &new_size);
			if (plat_ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Getting used storage on destination sink failed: %d",
					plat_ret);
				ret = suit_plat_err_to_processor_err_convert(plat_ret);
			}

			if (ret == SUIT_SUCCESS) {
				plat_ret = suit_plat_memptr_size_update(dst_handle, new_size);
				if (plat_ret != SUIT_PLAT_SUCCESS) {
					LOG_ERR("Failed to update destination MEM component size: "
						"%i",
						plat_ret);
					ret = suit_plat_err_to_processor_err_convert(plat_ret);
				}
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

/** @brief Common implementation for fetching data into SoC specific components. */
static int suit_plat_fetch_soc(suit_component_t dst_handle, struct zcbor_string *uri,
			       struct zcbor_string *manifest_component_id,
			       struct suit_encryption_info *enc_info, data_streamer streamer,
			       bool dry_run)
{
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	struct stream_sink dst_sink = {0};
	int ret = SUIT_SUCCESS;

	/*
	 * Validate streaming operation.
	 */

	/*
	 * Construct the stream.
	 */

	/* Create destination sink. */
	ret = suit_sink_select(dst_handle, &dst_sink);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Selecting sink failed: %d", ret);
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
		LOG_ERR("Decryption while streaming to SoC specific component is not supported");
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
#endif /* CONFIG_SUIT_STREAM */

bool suit_plat_fetch_domain_specific_is_type_supported(suit_component_type_t dst_component_type)
{
	if (!IS_ENABLED(CONFIG_SUIT_STREAM_SOURCE_CACHE) &&
	    (!IS_ENABLED(CONFIG_SUIT_STREAM_IPC_REQUESTOR) ||
	     (dst_component_type != SUIT_COMPONENT_TYPE_MEM))) {
		return false;
	}

#ifdef CONFIG_SUIT_STREAM
	switch (dst_component_type) {
	case SUIT_COMPONENT_TYPE_MEM:
	case SUIT_COMPONENT_TYPE_SOC_SPEC:
		return true;
	default:
		break;
	}
#endif /* CONFIG_SUIT_STREAM */

	return false;
}

int suit_plat_check_fetch_domain_specific(suit_component_t dst_handle,
					  suit_component_type_t dst_component_type,
					  struct zcbor_string *uri,
					  struct zcbor_string *manifest_component_id,
					  struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM
	switch (dst_component_type) {
	case SUIT_COMPONENT_TYPE_MEM:
		return suit_plat_fetch_mem(dst_handle, uri, manifest_component_id, enc_info,
					   external_streamer, true);
	case SUIT_COMPONENT_TYPE_SOC_SPEC:
		return suit_plat_fetch_soc(dst_handle, uri, manifest_component_id, enc_info,
					   external_sdfw_streamer, true);
	default:
		break;
	}
#endif /* CONFIG_SUIT_STREAM */

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

int suit_plat_fetch_domain_specific(suit_component_t dst_handle,
				    suit_component_type_t dst_component_type,
				    struct zcbor_string *uri,
				    struct zcbor_string *manifest_component_id,
				    struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM
	switch (dst_component_type) {
	case SUIT_COMPONENT_TYPE_MEM:
		return suit_plat_fetch_mem(dst_handle, uri, manifest_component_id, enc_info,
					   external_streamer, false);
	case SUIT_COMPONENT_TYPE_SOC_SPEC:
		return suit_plat_fetch_soc(dst_handle, uri, manifest_component_id, enc_info,
					   external_sdfw_streamer, false);
	default:
		break;
	}
#endif /* CONFIG_SUIT_STREAM */

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

bool suit_plat_fetch_integrated_domain_specific_is_type_supported(
	suit_component_type_t dst_component_type)
{
#ifdef CONFIG_SUIT_STREAM
	switch (dst_component_type) {
	case SUIT_COMPONENT_TYPE_MEM:
	case SUIT_COMPONENT_TYPE_SOC_SPEC:
		return true;
	default:
		break;
	}
#endif /* CONFIG_SUIT_STREAM */

	return false;
}

int suit_plat_check_fetch_integrated_domain_specific(suit_component_t dst_handle,
						     suit_component_type_t dst_component_type,
						     struct zcbor_string *payload,
						     struct zcbor_string *manifest_component_id,
						     struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	switch (dst_component_type) {
	case SUIT_COMPONENT_TYPE_MEM:
		return suit_plat_fetch_mem(dst_handle, payload, manifest_component_id, enc_info,
					   internal_streamer, true);
	case SUIT_COMPONENT_TYPE_SOC_SPEC:
		return suit_plat_fetch_soc(dst_handle, payload, manifest_component_id, enc_info,
					   internal_streamer, true);
	default:
		break;
	}
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

int suit_plat_fetch_integrated_domain_specific(suit_component_t dst_handle,
					       suit_component_type_t dst_component_type,
					       struct zcbor_string *payload,
					       struct zcbor_string *manifest_component_id,
					       struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	switch (dst_component_type) {
	case SUIT_COMPONENT_TYPE_MEM:
		return suit_plat_fetch_mem(dst_handle, payload, manifest_component_id, enc_info,
					   internal_streamer, false);
	case SUIT_COMPONENT_TYPE_SOC_SPEC:
		return suit_plat_fetch_soc(dst_handle, payload, manifest_component_id, enc_info,
					   internal_streamer, false);
	default:
		break;
	}
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}
