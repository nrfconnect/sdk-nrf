/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_platform_internal.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_error_convert.h>
#include <suit_plat_check_image_match_domain_specific.h>
#include <suit_plat_check_image_match_common.h>
#include <suit.h>

#ifdef CONFIG_SUIT_STREAM_SINK_DIGEST
#include <suit_memptr_storage.h>
#include <suit_generic_address_streamer.h>
#include <suit_digest_sink.h>

#include <psa/crypto.h>
#endif /* CONFIG_SUIT_STREAM_SINK_DIGEST */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_plat_check_image_match, CONFIG_SUIT_LOG_LEVEL);

#ifdef CONFIG_SUIT_STREAM_SINK_DIGEST
int suit_plat_check_image_match_mem_mapped(suit_component_t component,
					   enum suit_cose_alg alg_id,
					   struct zcbor_string *digest)
{
	void *impl_data = NULL;
	int err = suit_plat_component_impl_data_get(component, &impl_data);

	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to get implementation data: %d", err);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	const uint8_t *data = NULL;
	size_t size = 0;

	err = suit_memptr_storage_ptr_get((memptr_storage_handle_t)impl_data, &data, &size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to get memptr ptr: %d", err);
		return SUIT_ERR_CRASH;
	}

	psa_algorithm_t psa_alg;

	if (suit_cose_sha512 == alg_id) {
		psa_alg = PSA_ALG_SHA_512;
	} else if (suit_cose_sha256 == alg_id) {
		psa_alg = PSA_ALG_SHA_256;
	} else {
		LOG_ERR("Unsupported hash algorithm: %d", alg_id);
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	struct stream_sink digest_sink;

	err = suit_digest_sink_get(&digest_sink, psa_alg, digest->value);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to get digest sink: %d", err);
		return suit_plat_err_to_processor_err_convert(err);
	}

	err = suit_generic_address_streamer_stream(data, size, &digest_sink);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to stream to digest sink: %d", err);
		err = suit_plat_err_to_processor_err_convert(err);
	} else {
		err = suit_digest_sink_digest_match(digest_sink.ctx);
		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to check digest: %d", err);
			/* Translate error code to allow entering another branches in try-each
			 * sequence
			 */
			err = SUIT_FAIL_CONDITION;
		} else {
			err = SUIT_SUCCESS;
		}
	}

	suit_plat_err_t release_err = digest_sink.release(digest_sink.ctx);

	if (release_err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to release digest sink: %d", release_err);
		if (err != SUIT_SUCCESS) {
			err = suit_plat_err_to_processor_err_convert(release_err);
		}
	}

	return err;
}
#endif /* CONFIG_SUIT_STREAM_SINK_DIGEST */

int suit_plat_check_image_match_mfst(suit_component_t component, enum suit_cose_alg alg_id,
				     struct zcbor_string *digest)
{
	int ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;

	const uint8_t *envelope_str;
	size_t envelope_len;
	struct zcbor_string manifest_digest;
	enum suit_cose_alg alg;

	ret = suit_plat_retrieve_manifest(component, &envelope_str, &envelope_len);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to check image digest: unable to retrieve manifest contents "
			"(handle: %p)\r\n",
			(void *)component);
		return ret;
	}

	ret = suit_processor_get_manifest_metadata(envelope_str, envelope_len, false, NULL,
						   &manifest_digest, &alg, NULL);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to check image digest: unable to read manifest digest (handle: "
			"%p)\r\n",
			(void *)component);
		return ret;
	}

	if (alg_id != alg) {
		LOG_ERR("Manifest digest check failed: digest algorithm does not match (handle: "
			"%p)\r\n",
			(void *)component);
		ret = SUIT_FAIL_CONDITION;
	} else if (!suit_compare_zcbor_strings(digest, &manifest_digest)) {
		LOG_ERR("Manifest digest check failed: digest values does not match (handle: "
			"%p)\r\n",
			(void *)component);
		ret = SUIT_FAIL_CONDITION;
	}

	return ret;
}

int suit_plat_check_image_match(suit_component_t component, enum suit_cose_alg alg_id,
				struct zcbor_string *digest)
{
	struct zcbor_string *component_id = NULL;
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	int err = suit_plat_component_id_get(component, &component_id);

	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to get component id: %d", err);
		return err;
	}

	if (suit_plat_decode_component_type(component_id, &component_type) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to decode component type");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	LOG_DBG("Component type: %d", component_type);

	switch (component_type) {
	case SUIT_COMPONENT_TYPE_UNSUPPORTED:
		LOG_ERR("Unsupported component type");
		err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		break;
	case SUIT_COMPONENT_TYPE_CAND_IMG:
		err = suit_plat_check_image_match_mem_mapped(component, alg_id, digest);
		break;
	case SUIT_COMPONENT_TYPE_CAND_MFST:
		err = suit_plat_check_image_match_mfst(component, alg_id, digest);
		break;

	case SUIT_COMPONENT_TYPE_MEM:
	case SUIT_COMPONENT_TYPE_SOC_SPEC:
	case SUIT_COMPONENT_TYPE_INSTLD_MFST:
		/* Handling of these component types is domain specific */
		err = suit_plat_check_image_match_domain_specific(component, alg_id, digest,
								  component_id, component_type);
		break;
	case SUIT_COMPONENT_TYPE_CACHE_POOL:
	default: {
		LOG_ERR("Unhandled component type: %d", component_type);
		err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		break;
	}
	}

	return err;
}
