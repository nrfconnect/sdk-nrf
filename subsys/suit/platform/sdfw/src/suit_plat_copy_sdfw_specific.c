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
#include <suit_memory_layout.h>
#include <suit_plat_copy_domain_specific.h>

#ifdef CONFIG_SUIT_IPUC
#include <suit_ipuc_sdfw.h>
#include <suit_flash_sink.h>
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

LOG_MODULE_DECLARE(suit_plat_copy, CONFIG_SUIT_LOG_LEVEL);

#ifdef CONFIG_SUIT_STREAM
static bool
decode_supported_soc_specific_component(suit_component_t handle,
					suit_secure_domain_component_number_t *soc_component)
{
	struct zcbor_string *component_id = NULL;
	uint32_t component_number = 0;
	suit_plat_err_t plat_ret;
	int ret;

	ret = suit_plat_component_id_get(handle, &component_id);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to get component ID: %d", ret);
		return false;
	}

	plat_ret = suit_plat_decode_component_number(component_id, &component_number);
	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to decode SOC specific component number: %d", plat_ret);
		ret = suit_plat_err_to_processor_err_convert(plat_ret);
		return false;
	}

	*soc_component = (suit_secure_domain_component_number_t)component_number;

	if ((*soc_component) != SUIT_SECDOM_COMPONENT_NUMBER_SDFW &&
	    (*soc_component) != SUIT_SECDOM_COMPONENT_NUMBER_SDFW_RECOVERY) {
		LOG_ERR("Unsupported SOC specific component type: %d", *soc_component);
		return false;
	}

	return true;
}
#endif /* CONFIG_SUIT_STREAM */

bool suit_plat_copy_domain_specific_is_type_supported(suit_component_type_t dst_component_type,
						      suit_component_type_t src_component_type)
{
#ifdef CONFIG_SUIT_STREAM
	/* Check if destination component type is supported */
	if ((dst_component_type != SUIT_COMPONENT_TYPE_MEM) &&
	    (dst_component_type != SUIT_COMPONENT_TYPE_SOC_SPEC)) {
		return false;
	}

	/* Check if source component type is supported */
	if ((src_component_type != SUIT_COMPONENT_TYPE_MEM) &&
	    (src_component_type != SUIT_COMPONENT_TYPE_CAND_IMG)) {
		return false;
	}

	return true;
#else  /* CONFIG_SUIT_STREAM */
	return false;
#endif /* CONFIG_SUIT_STREAM */
}

int suit_plat_check_copy_domain_specific(suit_component_t dst_handle,
					 suit_component_type_t dst_component_type,
					 suit_component_t src_handle,
					 suit_component_type_t src_component_type,
					 struct zcbor_string *manifest_component_id,
					 struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM
	suit_secure_domain_component_number_t soc_component = 0;
	struct stream_sink dst_sink;
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	const uint8_t *payload_ptr;
	size_t payload_size;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	int ret = SUIT_SUCCESS;

	/*
	 * Validate streaming operation.
	 */

	if (!suit_plat_copy_domain_specific_is_type_supported(dst_component_type,
							      src_component_type)) {
		LOG_ERR("Unsupported component type pair: (dst: %d, src: %d)", dst_component_type,
			src_component_type);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (dst_component_type == SUIT_COMPONENT_TYPE_SOC_SPEC) {
		if (!decode_supported_soc_specific_component(dst_handle, &soc_component)) {
			LOG_ERR("Failed to decode SOC specific component");
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}
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

int suit_plat_copy_domain_specific(suit_component_t dst_handle,
				   suit_component_type_t dst_component_type,
				   suit_component_t src_handle,
				   suit_component_type_t src_component_type,
				   struct zcbor_string *manifest_component_id,
				   struct suit_encryption_info *enc_info)
{
#ifdef CONFIG_SUIT_STREAM
	suit_secure_domain_component_number_t soc_component = 0;
	struct stream_sink dst_sink;
#ifdef CONFIG_SUIT_STREAM_SOURCE_MEMPTR
	const uint8_t *payload_ptr;
	size_t payload_size;
#endif /* CONFIG_SUIT_STREAM_SOURCE_MEMPTR */
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	int ret = SUIT_SUCCESS;

	/*
	 * Validate streaming operation.
	 */

	if (!suit_plat_copy_domain_specific_is_type_supported(dst_component_type,
							      src_component_type)) {
		LOG_ERR("Unsupported component type pair: (dst: %d, src: %d)", dst_component_type,
			src_component_type);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (dst_component_type == SUIT_COMPONENT_TYPE_SOC_SPEC) {
		if (!decode_supported_soc_specific_component(dst_handle, &soc_component)) {
			LOG_ERR("Failed to decode SOC specific component");
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}
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

	if (ret == SUIT_SUCCESS && dst_component_type == SUIT_COMPONENT_TYPE_SOC_SPEC &&
	    (soc_component == SUIT_SECDOM_COMPONENT_NUMBER_SDFW ||
	     soc_component == SUIT_SECDOM_COMPONENT_NUMBER_SDFW_RECOVERY)) {
		uintptr_t sdfw_update_area_addr = 0;
		size_t sdfw_update_area_size = 0;

		suit_memory_sdfw_update_area_info_get(&sdfw_update_area_addr,
						      &sdfw_update_area_size);

		if (sdfw_update_area_size > 0) {
			/* SoC enforces constrains on update candidate location
			 */
			if ((uintptr_t)payload_ptr < sdfw_update_area_addr ||
			    (uintptr_t)payload_ptr + payload_size >
				    sdfw_update_area_addr + sdfw_update_area_size) {

				LOG_WRN("SDFW or SDFW_RECOVERY update - candidate mirror required "
					"(%d bytes)",
					payload_size);
#ifdef CONFIG_SUIT_IPUC
				struct stream_sink mirror_sink;
				intptr_t mirror_addr =
					suit_ipuc_sdfw_mirror_addr(payload_size);

				if (mirror_addr == 0) {
					LOG_ERR("SDFW or SDFW_RECOVERY update - candidate mirror "
						"not found");
					ret = suit_plat_err_to_processor_err_convert(
						SUIT_PLAT_ERR_NOMEM);
				}

				if (ret == SUIT_SUCCESS) {
					plat_ret = suit_flash_sink_get(
						&mirror_sink, (uint8_t *)mirror_addr, payload_size);
					if (plat_ret != SUIT_PLAT_SUCCESS) {
						LOG_ERR("Could not acquire SDFW or SDFW_RECOVERY "
							"mirror sink: %d",
							plat_ret);
						ret = suit_plat_err_to_processor_err_convert(
							plat_ret);
					}
				}

				if (ret == SUIT_SUCCESS && mirror_sink.erase != NULL) {
					plat_ret = mirror_sink.erase(mirror_sink.ctx);
					if (plat_ret != SUIT_PLAT_SUCCESS) {
						LOG_ERR("SDFW or SDFW_RECOVERY mirror sink erase "
							"failed: %d",
							plat_ret);
					}
				}

				if (ret == SUIT_SUCCESS) {
					LOG_INF("Streaming to SDFW or SDFW_RECOVERY mirror sink, "
						"%d bytes",
						payload_size);
					plat_ret = suit_generic_address_streamer_stream(
						payload_ptr, payload_size, &mirror_sink);

					if (mirror_sink.flush != NULL) {
						mirror_sink.flush(mirror_sink.ctx);
					}

					if (mirror_sink.release != NULL) {
						mirror_sink.release(mirror_sink.ctx);
					}

					if (plat_ret != SUIT_PLAT_SUCCESS) {
						LOG_ERR("Streaming to SDFW or SDFW_RECOVERY mirror "
							"sink failed: %d",
							plat_ret);
						ret = suit_plat_err_to_processor_err_convert(
							plat_ret);
					}
				}

				if (ret == SUIT_SUCCESS) {
					payload_ptr = (uint8_t *)mirror_addr;
				}
#else
				ret = suit_plat_err_to_processor_err_convert(SUIT_PLAT_ERR_NOMEM);
#endif
			}
		}
	}

	if (ret == SUIT_SUCCESS) {
		plat_ret =
			suit_generic_address_streamer_stream(payload_ptr, payload_size, &dst_sink);
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
