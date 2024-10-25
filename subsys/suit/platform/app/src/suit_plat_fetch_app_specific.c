/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_plat_error_convert.h>
#include <suit_plat_fetch_domain_specific.h>

#ifdef CONFIG_SUIT_STREAM

#include <suit_dfu_cache_sink.h>
#include <suit_dfu_cache_streamer.h>
#include <suit_generic_address_streamer.h>

#ifdef CONFIG_SUIT_STREAM_FETCH_SOURCE_MGR
#include "suit_fetch_source_streamer.h"
#endif /* CONFIG_SUIT_STREAM_FETCH_SOURCE_MGR */
#ifdef CONFIG_SUIT_STREAM_SOURCE_CACHE
#include <suit_dfu_cache_streamer.h>
#endif /* CONFIG_SUIT_STREAM_SOURCE_CACHE */

LOG_MODULE_REGISTER(suit_plat_fetch_app, CONFIG_SUIT_LOG_LEVEL);

bool suit_plat_fetch_domain_specific_is_type_supported(suit_component_type_t component_type)
{
	if ((component_type == SUIT_COMPONENT_TYPE_CAND_IMG) ||
	    (component_type == SUIT_COMPONENT_TYPE_CAND_MFST) ||
	    (component_type == SUIT_COMPONENT_TYPE_CACHE_POOL)) {
		return true;
	}

	return false;
}

bool suit_plat_fetch_integrated_domain_specific_is_type_supported(
	suit_component_type_t component_type)
{
	if ((component_type == SUIT_COMPONENT_TYPE_CAND_IMG) ||
	    (component_type == SUIT_COMPONENT_TYPE_CAND_MFST)) {
		return true;
	}

	return false;
}

int suit_plat_fetch_domain_specific(suit_component_t dst_handle,
				    suit_component_type_t dst_component_type,
				    struct stream_sink *dst_sink, struct zcbor_string *uri)
{
	int ret = SUIT_SUCCESS;
	bool hard_failure = false;

	/* Select streamer */
	switch (dst_component_type) {
#ifdef CONFIG_SUIT_STREAM_FETCH_SOURCE_MGR
	case SUIT_COMPONENT_TYPE_CACHE_POOL:
	case SUIT_COMPONENT_TYPE_MEM: {
		ret = suit_fetch_source_stream(uri->value, uri->len, dst_sink);
		ret = suit_plat_err_to_processor_err_convert(ret);
	} break;
#endif /* SUIT_STREAM_FETCH_SOURCE_MGR */
#if defined(CONFIG_SUIT_CACHE_RW) || defined(SUIT_CACHE)
	case SUIT_COMPONENT_TYPE_CAND_MFST:
	case SUIT_COMPONENT_TYPE_CAND_IMG: {
		ret = suit_dfu_cache_streamer_stream(uri->value, uri->len, dst_sink);
		ret = suit_plat_err_to_processor_err_convert(ret);
	} break;
#endif
	default:
		ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		hard_failure = true;
		break;
	}

	if (ret == SUIT_SUCCESS && dst_component_type == SUIT_COMPONENT_TYPE_CACHE_POOL) {
		suit_dfu_cache_sink_commit(dst_sink->ctx);
	}

	if (!hard_failure) {
		/* Failures without hard_failure flag set are treated as "unavailable payload"
		 * failures. These are cases where suit-condition-image-match will detect
		 * the failure, however suit-plat-fetch should return success to allow
		 * soft failures.
		 */
		ret = SUIT_SUCCESS;
	}

	return ret;
}

int suit_plat_fetch_integrated_domain_specific(suit_component_t dst_handle,
					       suit_component_type_t dst_component_type,
					       struct stream_sink *dst_sink,
					       struct zcbor_string *payload)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;

	(void)dst_handle;
	(void)dst_component_type;

	if (payload == NULL) {
		return SUIT_ERR_UNAVAILABLE_PAYLOAD;
	}

	ret = suit_generic_address_streamer_stream(payload->value, payload->len, dst_sink);

	return suit_plat_err_to_processor_err_convert(ret);
}

#endif /* CONFIG_SUIT_STREAM */
