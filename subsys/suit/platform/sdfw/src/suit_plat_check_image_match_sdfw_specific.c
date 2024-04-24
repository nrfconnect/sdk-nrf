/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_plat_check_image_match_domain_specific.h>
#include <suit_plat_check_image_match_common.h>
#include <suit_platform.h>

#include <suit_plat_digest_cache.h>
#include <suit_plat_decode_util.h>
#include <psa/crypto.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_plat_check_image_match_sdfw, CONFIG_SUIT_LOG_LEVEL);

static int suit_plat_check_image_match_soc_spec_sdfw(struct zcbor_string *component_id,
						     enum suit_cose_alg alg_id,
						     struct zcbor_string *digest)
{
#ifdef CONFIG_SOC_SERIES_NRF54HX
	if (suit_cose_sha512 != alg_id) {
		LOG_ERR("Unsupported digest algorithm: %d", alg_id);
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	uint8_t *current_sdfw_digest = (uint8_t *)(NRF_SICR->UROT.SM.TBS.FW.DIGEST);

	if (PSA_HASH_LENGTH(PSA_ALG_SHA_512) != digest->len) {
		LOG_ERR("Digest length mismatch: %d instead of %d", digest->len,
			PSA_HASH_LENGTH(PSA_ALG_SHA_512));
		return SUIT_FAIL_CONDITION;
	}

	if (memcmp((void *)current_sdfw_digest, (void *)digest->value,
		   PSA_HASH_LENGTH(PSA_ALG_SHA_512))) {
		LOG_INF("Digest mismatch");
		return SUIT_FAIL_CONDITION;
	}

	return SUIT_SUCCESS;
#else  /* CONFIG_SOC_SERIES_NRF54HX */
	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
#endif /* CONFIG_SOC_SERIES_NRF54HX */
}

static int suit_plat_check_image_match_soc_spec(struct zcbor_string *component_id,
						enum suit_cose_alg alg_id,
						struct zcbor_string *digest)
{
	uint32_t number = 0;

	if (suit_plat_decode_component_number(component_id, &number) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Missing component id number");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	LOG_DBG("Component id number: %d", number);

	int err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;

	if (number == 1) {
		/* SDFW */
		err = suit_plat_check_image_match_soc_spec_sdfw(component_id, alg_id, digest);
	} else if (number == 2) {
		/* SDFW recovery */
		err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	} else {
		/* Unsupported */
		err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	return err;
}

int suit_plat_check_image_match_domain_specific(suit_component_t component,
						enum suit_cose_alg alg_id,
						struct zcbor_string *digest,
						struct zcbor_string *component_id,
						suit_component_type_t component_type)
{
	int err = SUIT_SUCCESS;

	switch (component_type) {
	case SUIT_COMPONENT_TYPE_MEM:
		err = suit_plat_check_image_match_mem_mapped(component, alg_id, digest);
		break;
	case SUIT_COMPONENT_TYPE_SOC_SPEC: {
		err = suit_plat_check_image_match_soc_spec(component_id, alg_id, digest);
		break;
	}
	case SUIT_COMPONENT_TYPE_INSTLD_MFST:
		err = suit_plat_check_image_match_mfst(component, alg_id, digest);
		break;

	default: {
		/* Should never get here */
		err = SUIT_ERR_TAMP;
		break;
	}
	}

#if CONFIG_SUIT_DIGEST_CACHE
	if (err == SUIT_SUCCESS) {
		int ret;

		switch (component_type) {
		case SUIT_COMPONENT_TYPE_MEM:
		case SUIT_COMPONENT_TYPE_SOC_SPEC: {
			ret = suit_plat_digest_cache_add(component_id, alg_id, digest);

			if (ret != SUIT_SUCCESS) {
				LOG_WRN("Failed to cache digest for component type %d, err %d",
					component_type, ret);
			}
		}
		default: {
			break;
		}
		}
	}
#endif /* CONFIG_SUIT_DIGEST_CACHE */

	return err;
}
