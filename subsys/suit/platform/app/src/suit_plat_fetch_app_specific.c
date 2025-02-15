/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <zephyr/logging/log.h>

#include <suit_platform_internal.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_fetch_domain_specific.h>
#include <suit_plat_err.h>
#include <suit_plat_error_convert.h>

#ifdef CONFIG_SUIT_STREAM_SINK_FLASH
#include <suit_flash_sink.h>
#endif /* CONFIG_SUIT_STREAM_SINK_FLASH */

#ifdef CONFIG_FLASH_IPUC
#include <drivers/flash/flash_ipuc.h>
#endif /* CONFIG_FLASH_IPUC */

#ifdef CONFIG_SUIT_STREAM_SINK_CACHE
#include <suit_dfu_cache_sink.h>
#endif /* CONFIG_SUIT_STREAM_SINK_CACHE */

#ifdef CONFIG_SUIT_STREAM_FETCH_SOURCE_MGR
#include "suit_fetch_source_streamer.h"
#endif /* CONFIG_SUIT_STREAM_FETCH_SOURCE_MGR */

LOG_MODULE_REGISTER(suit_plat_fetch_app, CONFIG_SUIT_LOG_LEVEL);

#ifdef CONFIG_SUIT_STREAM_SINK_CACHE
static int suit_plat_fetch_cache(suit_component_t dst_handle, struct zcbor_string *uri,
				 struct zcbor_string *manifest_component_id,
				 struct suit_encryption_info *enc_info, bool dry_run)
{
#ifndef CONFIG_SUIT_STREAM_FETCH_SOURCE_MGR
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#else
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	struct stream_sink dst_sink = {0};
	struct zcbor_string *component_id = NULL;
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
		LOG_ERR("Missing component id number in cache pool component");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (enc_info != NULL) {
		LOG_ERR("Decryption while streaming to cache pool is not supported");
		(void)manifest_component_id;
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	/*
	 * Construct the stream.
	 */

	plat_ret = suit_dfu_cache_sink_get(&dst_sink, number, uri->value, uri->len, !dry_run);
	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to create cache sink: %d", plat_ret);
		ret = suit_plat_err_to_processor_err_convert(plat_ret);
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
			plat_ret = suit_fetch_source_stream(uri->value, uri->len, &dst_sink);
			if (plat_ret == SUIT_PLAT_SUCCESS) {
				suit_dfu_cache_sink_commit(dst_sink.ctx);
			} else {
				LOG_ERR("Streaming to cache failed: %d", plat_ret);
			}

			/* Failures of fetch_source streamer are treated as "unavailable payload"
			 * failures. These are cases where suit-condition-image-match will detect
			 * the failure, however suit-plat-fetch should return success to allow
			 * soft failures.
			 */
			ret = SUIT_SUCCESS;
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
#endif /* CONFIG_SUIT_STREAM_FETCH_SOURCE_MGR */
}
#endif /* CONFIG_SUIT_STREAM_SINK_CACHE */

#ifdef CONFIG_FLASH_IPUC
static int suit_plat_fetch_mem(suit_component_t dst_handle, struct zcbor_string *uri,
			       struct zcbor_string *manifest_component_id,
			       struct suit_encryption_info *enc_info, bool dry_run)
{
#ifndef CONFIG_SUIT_STREAM_FETCH_SOURCE_MGR
	return SUIT_ERR_UNSUPPORTED_COMMAND;
#else
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	struct stream_sink dst_sink = {0};
	struct zcbor_string *component_id = NULL;
	int ret = SUIT_SUCCESS;
	struct device *ipuc_dev = NULL;
	struct zcbor_string enc_info_buf;
	intptr_t run_address = 0;
	size_t size = 0;

	/*
	 * Validate streaming operation.
	 */
	ret = suit_plat_component_id_get(dst_handle, &component_id);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to parse component ID from handle: %d", ret);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_address_size(component_id, &run_address, &size) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to decode address and size");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (!flash_component_ipuc_check(component_id)) {
		LOG_ERR("Unable to find IPUC to modify MEM component");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/*
	 * Construct the stream.
	 */

	if (!dry_run) {
		/* Create IPUC instance and erase the destination memory */
		if (enc_info != NULL) {
			enc_info_buf.value = (const uint8_t *)enc_info;
			enc_info_buf.len = sizeof(*enc_info);
			ipuc_dev = flash_component_ipuc_create(component_id, &enc_info_buf, NULL);
		} else {
			ipuc_dev = flash_component_ipuc_create(component_id, NULL, NULL);
		}
		if (ipuc_dev == NULL) {
			LOG_ERR("Unable to create IPUC to modify MEM component: %d", ret);
			ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

#ifdef CONFIG_SUIT_STREAM_SINK_FLASH
		/* Internal MRAM/Flash on single controller */
		if (ret == SUIT_SUCCESS) {
			if (!suit_flash_sink_is_address_supported((uint8_t *)run_address)) {
				LOG_ERR("IPUC address not supported by flash sink (0x%lx, 0x%x)",
					run_address, size);
				ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
			} else {
				plat_ret = suit_flash_sink_get(&dst_sink, (uint8_t *)run_address,
							       size);
				if (plat_ret != SUIT_PLAT_SUCCESS) {
					LOG_ERR("Unable to create flash sink: %d", plat_ret);
					ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
				}
			}
		}
#else  /* CONFIG_SUIT_STREAM_SINK_FLASH */
		ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
#endif /* CONFIG_SUIT_STREAM_SINK_FLASH */

		/*
		 * Stream the data.
		 */

		/* Skip memory erase - it was already cleared by the IPUC constructor. */

		/* Start streaming the data. */
		if (ret == SUIT_SUCCESS) {
			plat_ret = suit_fetch_source_stream(uri->value, uri->len, &dst_sink);

			/* Failures of fetch_source streamer are treated as "unavailable payload"
			 * failures. These are cases where suit-condition-image-match will detect
			 * the failure, however suit-plat-fetch should return success to allow
			 * soft failures.
			 */
			ret = SUIT_SUCCESS;
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

	if (dst_sink.write != NULL) {
		plat_ret = release_sink(&dst_sink);
		if (plat_ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Sink release failed: %d", plat_ret);
		}
	}

	if (ipuc_dev != NULL) {
		flash_ipuc_release(ipuc_dev);
	}

	if (ret == SUIT_SUCCESS) {
		ret = suit_plat_err_to_processor_err_convert(plat_ret);
	}

	return ret;
#endif /* CONFIG_SUIT_STREAM_FETCH_SOURCE_MGR */
}
#endif /* CONFIG_FLASH_IPUC */

bool suit_plat_fetch_domain_specific_is_type_supported(suit_component_type_t dst_component_type)
{
#ifdef CONFIG_SUIT_STREAM_SINK_CACHE
	if (dst_component_type == SUIT_COMPONENT_TYPE_CACHE_POOL) {
		return true;
	}
#endif /* CONFIG_SUIT_STREAM_SINK_CACHE */
#ifdef CONFIG_FLASH_IPUC
	if (dst_component_type == SUIT_COMPONENT_TYPE_MEM) {
		return true;
	}
#endif /* CONFIG_FLASH_IPUC */

	return false;
}

int suit_plat_check_fetch_domain_specific(suit_component_t dst_handle,
					  suit_component_type_t dst_component_type,
					  struct zcbor_string *uri,
					  struct zcbor_string *manifest_component_id,
					  struct suit_encryption_info *enc_info)
{
	switch (dst_component_type) {
#ifdef CONFIG_SUIT_STREAM_SINK_CACHE
	case SUIT_COMPONENT_TYPE_CACHE_POOL:
		return suit_plat_fetch_cache(dst_handle, uri, manifest_component_id, enc_info,
					     true);
#endif /* CONFIG_SUIT_STREAM_SINK_CACHE */
#ifdef CONFIG_FLASH_IPUC
	case SUIT_COMPONENT_TYPE_MEM:
		return suit_plat_fetch_mem(dst_handle, uri, manifest_component_id, enc_info, true);
#endif /* CONFIG_FLASH_IPUC */
	default:
		break;
	}

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

int suit_plat_fetch_domain_specific(suit_component_t dst_handle,
				    suit_component_type_t dst_component_type,
				    struct zcbor_string *uri,
				    struct zcbor_string *manifest_component_id,
				    struct suit_encryption_info *enc_info)
{
	switch (dst_component_type) {
#ifdef CONFIG_SUIT_STREAM_SINK_CACHE
	case SUIT_COMPONENT_TYPE_CACHE_POOL:
		return suit_plat_fetch_cache(dst_handle, uri, manifest_component_id, enc_info,
					     false);
#endif /* CONFIG_SUIT_STREAM_SINK_CACHE */
#ifdef CONFIG_FLASH_IPUC
	case SUIT_COMPONENT_TYPE_MEM:
		return suit_plat_fetch_mem(dst_handle, uri, manifest_component_id, enc_info, false);
#endif /* CONFIG_FLASH_IPUC */
	default:
		break;
	}

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

bool suit_plat_fetch_integrated_domain_specific_is_type_supported(
	suit_component_type_t component_type)
{
	return false;
}

int suit_plat_check_fetch_integrated_domain_specific(suit_component_t dst_handle,
						     suit_component_type_t dst_component_type,
						     struct zcbor_string *payload,
						     struct zcbor_string *manifest_component_id,
						     struct suit_encryption_info *enc_info)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

int suit_plat_fetch_integrated_domain_specific(suit_component_t dst_handle,
					       suit_component_type_t dst_component_type,
					       struct zcbor_string *payload,
					       struct zcbor_string *manifest_component_id,
					       struct suit_encryption_info *enc_info)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
