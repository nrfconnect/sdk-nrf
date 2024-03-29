/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <suit_dfu_cache.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include "zcbor_encode.h"
#include <zephyr/sys/byteorder.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util_macro.h>

#include "suit_dfu_cache_internal.h"

LOG_MODULE_REGISTER(dfu_cache, CONFIG_SUIT_LOG_LEVEL);

static bool init_done;
struct dfu_cache dfu_cache;

/**
 * @brief Check if current_key is same as uri
 *
 * @param current_key Current key decoded from cache slot
 * @param uri Desired uri
 * @return true
 * @return false
 */
static bool uricmp(const struct zcbor_string *current_key, const struct zcbor_string *uri)
{
	/* Check what type of string is uri */
	if (uri->value[uri->len - 1] == '\0') {
		/* uri is null terminated string */
		if ((uri->len - 1) != current_key->len) {
			return false;
		}
	} else {
		/* uri is zcbor tstr */
		if (uri->len != current_key->len) {
			return false;
		}
	}

	return !strncmp(current_key->value, uri->value, current_key->len);
}

/* Match callback context. */
struct match_uri_ctx {
	const struct zcbor_string *uri;
	uintptr_t payload_offset;
	size_t payload_size;
	bool match;
};

/**
 * @brief Foreach callback for matching.
 */
static bool match_uri(struct dfu_cache_pool *cache_pool, zcbor_state_t *state,
		      const struct zcbor_string *uri, uintptr_t payload_offset, size_t payload_size,
		      void *ctx)
{
	struct match_uri_ctx *cb_ctx = ctx;

	cb_ctx->match = uricmp(uri, cb_ctx->uri);

	if (cb_ctx->match) {
		cb_ctx->payload_offset = payload_offset;
		cb_ctx->payload_size = payload_size;
	}

	/* Stop further iteration if URI matches. */
	return !cb_ctx->match;
}

/**
 * @brief Check if cache_pool contains slot with key equal to uri and if true get data
 *
 * @param cache_pool Cache pool to be searched
 * @param uri Desired URI
 * @param payload Output pointer to data in slot
 * @return suit_plat_err_t SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
static suit_plat_err_t search_cache_pool(struct dfu_cache_pool *cache_pool,
					 const struct zcbor_string *uri,
					 struct zcbor_string *payload)
{
	suit_plat_err_t res = SUIT_PLAT_ERR_NOT_FOUND;

	if ((cache_pool != NULL) && (uri != NULL) && (payload != NULL) &&
	    (cache_pool->address != NULL) && (uri->len <= CONFIG_SUIT_MAX_URI_LENGTH)) {
		struct match_uri_ctx ctx = {
			.match = false,
			.uri = uri,
		};
		res = suit_dfu_cache_partition_slot_foreach(cache_pool, match_uri, &ctx);

		if (ctx.match) {
			payload->value = (uint8_t *)ctx.payload_offset;
			payload->len = ctx.payload_size;
			res = SUIT_PLAT_SUCCESS;
		} else {
			res = SUIT_PLAT_ERR_NOT_FOUND;
		}

		return res;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

suit_plat_err_t suit_dfu_cache_search(const uint8_t *uri, size_t uri_size, const uint8_t **payload,
				      size_t *payload_size)
{
	if ((uri != NULL) && (uri_size != 0)) {
		struct zcbor_string tmp_payload = {.len = 0, .value = NULL};
		struct zcbor_string tmp_uri = {.len = uri_size, .value = uri};

		for (size_t i = 0; i < dfu_cache.pools_count; i++) {
			suit_plat_err_t ret =
				search_cache_pool(&dfu_cache.pools[i], &tmp_uri, &tmp_payload);

			if (ret == SUIT_PLAT_SUCCESS) {
				*payload = tmp_payload.value;
				*payload_size = tmp_payload.len;

				return ret;
			}
		}

		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	return SUIT_PLAT_ERR_INVAL;
}

void suit_dfu_cache_clear(struct dfu_cache *cache)
{
	if (cache == NULL) {
		return;
	}

	for (size_t i = 0; i < cache->pools_count; i++) {
		cache->pools[i].address = NULL;
		cache->pools[i].size = 0;
	}
}

/**
 * @brief Copy the pointers between SUIT cache structures.
 *
 * @note This operation does not make a copy of the data, pointed by the caches.
 *       It copies the pointers to the data, so the contents may be referred from both of the
 *       structures.
 *
 * @param dst_cache  Destination cache to be aligned.
 * @param src_cache  Source cache to be copied.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
static suit_plat_err_t suit_dfu_cache_copy(struct dfu_cache *dst_cache,
					   const struct dfu_cache *src_cache)
{
	if (src_cache->pools_count > CONFIG_SUIT_CACHE_MAX_CACHES) {
		return SUIT_PLAT_ERR_NOMEM;
	}

	dst_cache->pools_count = src_cache->pools_count;
	memcpy(&(dst_cache->pools), src_cache->pools,
	       src_cache->pools_count * sizeof(struct dfu_cache_pool));

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_dfu_cache_initialize(struct dfu_cache *cache)
{
	if (init_done) {
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	suit_plat_err_t ret = suit_dfu_cache_copy(&dfu_cache, cache);

	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Copying cache from storage, failed: %i", ret);
		return ret;
	}

	init_done = true;

	return SUIT_PLAT_SUCCESS;
}

void suit_dfu_cache_deinitialize(void)
{
	suit_dfu_cache_clear(&dfu_cache);
	init_done = false;
}
