/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "psa_crypto_service_encode.h"
#include "psa_crypto_service_decode.h"

#include <sdfw/sdfw_services/ssf_client.h>
#include <sdfw/sdfw_services/crypto_service.h>

#include <zephyr/cache.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(psa_crypto_srvc, CONFIG_SSF_PSA_CRYPTO_SERVICE_LOG_LEVEL);

SSF_CLIENT_SERVICE_DEFINE(psa_crypto_srvc, PSA_CRYPTO, cbor_encode_psa_crypto_req,
			  cbor_decode_psa_crypto_rsp);

#if defined(CONFIG_SSF_PSA_CRYPTO_SERVICE_OUT_BOUNCE_BUFFERS)

#define CACHE_DATA_UNIT_SIZE (DCACHEDATA_DATAWIDTH * 4)

/* k_heap_alloc allocated memory is aligned on a multiple of pointer sizes. The HW's DataUnit size
 * must match this Zephyr behaviour.
 */
BUILD_ASSERT(CACHE_DATA_UNIT_SIZE == sizeof(uintptr_t));

static K_HEAP_DEFINE(out_buffer_heap,
		     ROUND_UP(CONFIG_SSF_PSA_CRYPTO_SERVICE_OUT_HEAP_SIZE, CACHE_DATA_UNIT_SIZE));

/**
 * @brief Prepare an out buffer in case the original buffer is not aligned
 *
 * If the original buffer is not aligned, a new buffer is allocated and the data is copied to it.
 * This is needed to achieve DCache DataUnit alignment.
 *
 * @param original_buffer Original buffer
 * @param size Size of the buffer
 * @return void* NULL if the buffer could not be allocated, original_buffer if it was aligned, else
 * a new buffer from the heap
 *
 */
static void *prepare_out_buffer(void *original_buffer, size_t size)
{
	void *out_buffer;

	if ((IS_ALIGNED(original_buffer, CACHE_DATA_UNIT_SIZE)) &&
	    (IS_ALIGNED(size, CACHE_DATA_UNIT_SIZE))) {
		out_buffer = original_buffer;
	} else {
		out_buffer = k_heap_alloc(&out_buffer_heap, size, K_NO_WAIT);
		if (out_buffer != NULL) {
			memcpy(out_buffer, original_buffer, size);
		}
	}

	return out_buffer;
}

/**
 * @brief Release an out buffer if it was allocated
 *
 * If the out buffer was allocated, the data is copied back to the original buffer and the out
 * buffer is first zeroed and then freed.
 *
 * @param original_buffer The original buffer
 * @param out_buffer The buffer to release
 * @param size Size of the buffer
 */
static void release_out_buffer(void *original_buffer, void *out_buffer, size_t size)
{
	if (out_buffer == NULL) {
		return;
	}

	if (original_buffer != out_buffer) {
		memcpy(original_buffer, out_buffer, size);
		/* Clear buffer before returning it to not leak sensitive data */
		memset(out_buffer, 0, size);
		sys_cache_data_flush_range(out_buffer, size);
		k_heap_free(&out_buffer_heap, out_buffer);
	}
}

#else

static inline void *prepare_out_buffer(void *original_buffer, size_t size)
{
	ARG_UNUSED(size);
	return original_buffer;
}

static inline void release_out_buffer(void *original_buffer, void *out_buffer, size_t size)
{
	ARG_UNUSED(original_buffer);
	ARG_UNUSED(out_buffer);
	ARG_UNUSED(size);
}

#endif

psa_status_t ssf_psa_get_key_attributes(mbedtls_svc_key_id_t key, psa_key_attributes_t *attributes)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *attributes_tmp = NULL;

	struct psa_get_key_attributes_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_get_key_attributes_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_get_key_attributes_req_m;

	req_data->psa_get_key_attributes_req_key = key;
	attributes_tmp = prepare_out_buffer(attributes, sizeof(*attributes));
	if (attributes_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_get_key_attributes_req_p_attributes = (uint32_t)attributes_tmp;
	sys_cache_data_flush_range((void *)attributes_tmp, sizeof(*attributes));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)attributes_tmp, sizeof(*attributes));

fail:
	release_out_buffer(attributes, attributes_tmp, sizeof(*attributes));
	return result;
}

psa_status_t ssf_psa_reset_key_attributes(psa_key_attributes_t *attributes)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *attributes_tmp = NULL;

	struct psa_reset_key_attributes_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_reset_key_attributes_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_reset_key_attributes_req_m;

	attributes_tmp = prepare_out_buffer(attributes, sizeof(*attributes));
	if (attributes_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_reset_key_attributes_req_p_attributes = (uint32_t)attributes_tmp;
	sys_cache_data_flush_range((void *)attributes_tmp, sizeof(*attributes));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)attributes_tmp, sizeof(*attributes));

fail:
	release_out_buffer(attributes, attributes_tmp, sizeof(*attributes));
	return result;
}

psa_status_t ssf_psa_purge_key(mbedtls_svc_key_id_t key)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_purge_key_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_purge_key_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_purge_key_req_m;

	req_data->psa_purge_key_req_key = key;

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}

fail:
	return result;
}

psa_status_t ssf_psa_copy_key(mbedtls_svc_key_id_t source_key,
			      const psa_key_attributes_t *attributes,
			      mbedtls_svc_key_id_t *target_key)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *target_key_tmp = NULL;

	struct psa_copy_key_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_copy_key_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_copy_key_req_m;

	req_data->psa_copy_key_req_source_key = source_key;
	req_data->psa_copy_key_req_p_attributes = (uint32_t)attributes;
	target_key_tmp = prepare_out_buffer(target_key, sizeof(*target_key));
	if (target_key_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_copy_key_req_p_target_key = (uint32_t)target_key_tmp;
	sys_cache_data_flush_range((void *)attributes, sizeof(*attributes));
	sys_cache_data_flush_range((void *)target_key_tmp, sizeof(*target_key));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)target_key_tmp, sizeof(*target_key));

fail:
	release_out_buffer(target_key, target_key_tmp, sizeof(*target_key));
	return result;
}

psa_status_t ssf_psa_destroy_key(mbedtls_svc_key_id_t key)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_destroy_key_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_destroy_key_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_destroy_key_req_m;

	req_data->psa_destroy_key_req_key = key;

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}

fail:
	return result;
}

psa_status_t ssf_psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				size_t data_length, mbedtls_svc_key_id_t *key)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *key_tmp = NULL;

	struct psa_import_key_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_import_key_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_import_key_req_m;

	req_data->psa_import_key_req_p_attributes = (uint32_t)attributes;
	req_data->psa_import_key_req_p_data = (uint32_t)data;
	req_data->psa_import_key_req_data_length = data_length;
	key_tmp = prepare_out_buffer(key, sizeof(*key));
	if (key_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_import_key_req_p_key = (uint32_t)key_tmp;
	sys_cache_data_flush_range((void *)attributes, sizeof(*attributes));
	sys_cache_data_flush_range((void *)data, data_length);
	sys_cache_data_flush_range((void *)key_tmp, sizeof(*key));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)key_tmp, sizeof(*key));

fail:
	release_out_buffer(key, key_tmp, sizeof(*key));
	return result;
}

psa_status_t ssf_psa_export_key(mbedtls_svc_key_id_t key, uint8_t *data, size_t data_size,
				size_t *data_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *data_tmp = NULL;
	void *data_length_tmp = NULL;

	struct psa_export_key_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_export_key_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_export_key_req_m;

	req_data->psa_export_key_req_key = key;
	data_tmp = prepare_out_buffer(data, data_size);
	if (data_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_export_key_req_p_data = (uint32_t)data_tmp;
	req_data->psa_export_key_req_data_size = data_size;
	data_length_tmp = prepare_out_buffer(data_length, sizeof(*data_length));
	if (data_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_export_key_req_p_data_length = (uint32_t)data_length_tmp;
	sys_cache_data_flush_range((void *)data_tmp, data_size);
	sys_cache_data_flush_range((void *)data_length_tmp, sizeof(*data_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)data_tmp, data_size);
	sys_cache_data_flush_and_invd_range((void *)data_length_tmp, sizeof(*data_length));

fail:
	release_out_buffer(data, data_tmp, data_size);
	release_out_buffer(data_length, data_length_tmp, sizeof(*data_length));
	return result;
}

psa_status_t ssf_psa_export_public_key(mbedtls_svc_key_id_t key, uint8_t *data, size_t data_size,
				       size_t *data_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *data_tmp = NULL;
	void *data_length_tmp = NULL;

	struct psa_export_public_key_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_export_public_key_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_export_public_key_req_m;

	req_data->psa_export_public_key_req_key = key;
	data_tmp = prepare_out_buffer(data, data_size);
	if (data_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_export_public_key_req_p_data = (uint32_t)data_tmp;
	req_data->psa_export_public_key_req_data_size = data_size;
	data_length_tmp = prepare_out_buffer(data_length, sizeof(*data_length));
	if (data_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_export_public_key_req_p_data_length = (uint32_t)data_length_tmp;
	sys_cache_data_flush_range((void *)data_tmp, data_size);
	sys_cache_data_flush_range((void *)data_length_tmp, sizeof(*data_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)data_tmp, data_size);
	sys_cache_data_flush_and_invd_range((void *)data_length_tmp, sizeof(*data_length));

fail:
	release_out_buffer(data, data_tmp, data_size);
	release_out_buffer(data_length, data_length_tmp, sizeof(*data_length));
	return result;
}

psa_status_t ssf_psa_hash_compute(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				  uint8_t *hash, size_t hash_size, size_t *hash_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *hash_tmp = NULL;
	void *hash_length_tmp = NULL;

	struct psa_hash_compute_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_hash_compute_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_hash_compute_req_m;

	req_data->psa_hash_compute_req_alg = alg;
	req_data->psa_hash_compute_req_p_input = (uint32_t)input;
	req_data->psa_hash_compute_req_input_length = input_length;
	hash_tmp = prepare_out_buffer(hash, hash_size);
	if (hash_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_hash_compute_req_p_hash = (uint32_t)hash_tmp;
	req_data->psa_hash_compute_req_hash_size = hash_size;
	hash_length_tmp = prepare_out_buffer(hash_length, sizeof(*hash_length));
	if (hash_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_hash_compute_req_p_hash_length = (uint32_t)hash_length_tmp;
	sys_cache_data_flush_range((void *)input, input_length);
	sys_cache_data_flush_range((void *)hash_tmp, hash_size);
	sys_cache_data_flush_range((void *)hash_length_tmp, sizeof(*hash_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)hash_tmp, hash_size);
	sys_cache_data_flush_and_invd_range((void *)hash_length_tmp, sizeof(*hash_length));

fail:
	release_out_buffer(hash, hash_tmp, hash_size);
	release_out_buffer(hash_length, hash_length_tmp, sizeof(*hash_length));
	return result;
}

psa_status_t ssf_psa_hash_compare(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				  const uint8_t *hash, size_t hash_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_hash_compare_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_hash_compare_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_hash_compare_req_m;

	req_data->psa_hash_compare_req_alg = alg;
	req_data->psa_hash_compare_req_p_input = (uint32_t)input;
	req_data->psa_hash_compare_req_input_length = input_length;
	req_data->psa_hash_compare_req_p_hash = (uint32_t)hash;
	req_data->psa_hash_compare_req_hash_length = hash_length;
	sys_cache_data_flush_range((void *)input, input_length);
	sys_cache_data_flush_range((void *)hash, hash_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}

fail:
	return result;
}

psa_status_t ssf_psa_hash_setup(mbedtls_psa_client_handle_t *p_handle, psa_algorithm_t alg)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_hash_setup_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_hash_setup_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_hash_setup_req_m;

	req_data->psa_hash_setup_req_p_handle = (uint32_t)p_handle;
	req_data->psa_hash_setup_req_alg = alg;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_hash_update(mbedtls_psa_client_handle_t *p_handle, const uint8_t *input,
				 size_t input_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_hash_update_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_hash_update_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_hash_update_req_m;

	req_data->psa_hash_update_req_p_handle = (uint32_t)p_handle;
	req_data->psa_hash_update_req_p_input = (uint32_t)input;
	req_data->psa_hash_update_req_input_length = input_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)input, input_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_hash_finish(mbedtls_psa_client_handle_t *p_handle, uint8_t *hash,
				 size_t hash_size, size_t *hash_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *hash_tmp = NULL;
	void *hash_length_tmp = NULL;

	struct psa_hash_finish_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_hash_finish_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_hash_finish_req_m;

	req_data->psa_hash_finish_req_p_handle = (uint32_t)p_handle;
	hash_tmp = prepare_out_buffer(hash, hash_size);
	if (hash_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_hash_finish_req_p_hash = (uint32_t)hash_tmp;
	req_data->psa_hash_finish_req_hash_size = hash_size;
	hash_length_tmp = prepare_out_buffer(hash_length, sizeof(*hash_length));
	if (hash_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_hash_finish_req_p_hash_length = (uint32_t)hash_length_tmp;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)hash_tmp, hash_size);
	sys_cache_data_flush_range((void *)hash_length_tmp, sizeof(*hash_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)hash_tmp, hash_size);
	sys_cache_data_flush_and_invd_range((void *)hash_length_tmp, sizeof(*hash_length));

fail:
	release_out_buffer(hash, hash_tmp, hash_size);
	release_out_buffer(hash_length, hash_length_tmp, sizeof(*hash_length));
	return result;
}

psa_status_t ssf_psa_hash_verify(mbedtls_psa_client_handle_t *p_handle, const uint8_t *hash,
				 size_t hash_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_hash_verify_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_hash_verify_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_hash_verify_req_m;

	req_data->psa_hash_verify_req_p_handle = (uint32_t)p_handle;
	req_data->psa_hash_verify_req_p_hash = (uint32_t)hash;
	req_data->psa_hash_verify_req_hash_length = hash_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)hash, hash_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_hash_abort(mbedtls_psa_client_handle_t *p_handle)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_hash_abort_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_hash_abort_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_hash_abort_req_m;

	req_data->psa_hash_abort_req_p_handle = (uint32_t)p_handle;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_hash_clone(mbedtls_psa_client_handle_t handle,
				mbedtls_psa_client_handle_t *p_handle)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_hash_clone_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_hash_clone_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_hash_clone_req_m;

	req_data->psa_hash_clone_req_handle = (uint32_t)handle;
	req_data->psa_hash_clone_req_p_handle = (uint32_t)p_handle;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_mac_compute(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				 const uint8_t *input, size_t input_length, uint8_t *mac,
				 size_t mac_size, size_t *mac_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *mac_tmp = NULL;
	void *mac_length_tmp = NULL;

	struct psa_mac_compute_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_mac_compute_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_mac_compute_req_m;

	req_data->psa_mac_compute_req_key = key;
	req_data->psa_mac_compute_req_alg = alg;
	req_data->psa_mac_compute_req_p_input = (uint32_t)input;
	req_data->psa_mac_compute_req_input_length = input_length;
	mac_tmp = prepare_out_buffer(mac, mac_size);
	if (mac_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_mac_compute_req_p_mac = (uint32_t)mac_tmp;
	req_data->psa_mac_compute_req_mac_size = mac_size;
	mac_length_tmp = prepare_out_buffer(mac_length, sizeof(*mac_length));
	if (mac_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_mac_compute_req_p_mac_length = (uint32_t)mac_length_tmp;
	sys_cache_data_flush_range((void *)input, input_length);
	sys_cache_data_flush_range((void *)mac_tmp, mac_size);
	sys_cache_data_flush_range((void *)mac_length_tmp, sizeof(*mac_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)mac_tmp, mac_size);
	sys_cache_data_flush_and_invd_range((void *)mac_length_tmp, sizeof(*mac_length));

fail:
	release_out_buffer(mac, mac_tmp, mac_size);
	release_out_buffer(mac_length, mac_length_tmp, sizeof(*mac_length));
	return result;
}

psa_status_t ssf_psa_mac_verify(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, const uint8_t *mac, size_t mac_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_mac_verify_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_mac_verify_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_mac_verify_req_m;

	req_data->psa_mac_verify_req_key = key;
	req_data->psa_mac_verify_req_alg = alg;
	req_data->psa_mac_verify_req_p_input = (uint32_t)input;
	req_data->psa_mac_verify_req_input_length = input_length;
	req_data->psa_mac_verify_req_p_mac = (uint32_t)mac;
	req_data->psa_mac_verify_req_mac_length = mac_length;
	sys_cache_data_flush_range((void *)input, input_length);
	sys_cache_data_flush_range((void *)mac, mac_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}

fail:
	return result;
}

psa_status_t ssf_psa_mac_sign_setup(mbedtls_psa_client_handle_t *p_handle, mbedtls_svc_key_id_t key,
				    psa_algorithm_t alg)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_mac_sign_setup_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_mac_sign_setup_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_mac_sign_setup_req_m;

	req_data->psa_mac_sign_setup_req_p_handle = (uint32_t)p_handle;
	req_data->psa_mac_sign_setup_req_key = key;
	req_data->psa_mac_sign_setup_req_alg = alg;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_mac_verify_setup(mbedtls_psa_client_handle_t *p_handle,
				      mbedtls_svc_key_id_t key, psa_algorithm_t alg)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_mac_verify_setup_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_mac_verify_setup_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_mac_verify_setup_req_m;

	req_data->psa_mac_verify_setup_req_p_handle = (uint32_t)p_handle;
	req_data->psa_mac_verify_setup_req_key = key;
	req_data->psa_mac_verify_setup_req_alg = alg;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_mac_update(mbedtls_psa_client_handle_t *p_handle, const uint8_t *input,
				size_t input_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_mac_update_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_mac_update_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_mac_update_req_m;

	req_data->psa_mac_update_req_p_handle = (uint32_t)p_handle;
	req_data->psa_mac_update_req_p_input = (uint32_t)input;
	req_data->psa_mac_update_req_input_length = input_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)input, input_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_mac_sign_finish(mbedtls_psa_client_handle_t *p_handle, uint8_t *mac,
				     size_t mac_size, size_t *mac_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *mac_tmp = NULL;
	void *mac_length_tmp = NULL;

	struct psa_mac_sign_finish_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_mac_sign_finish_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_mac_sign_finish_req_m;

	req_data->psa_mac_sign_finish_req_p_handle = (uint32_t)p_handle;
	mac_tmp = prepare_out_buffer(mac, mac_size);
	if (mac_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_mac_sign_finish_req_p_mac = (uint32_t)mac_tmp;
	req_data->psa_mac_sign_finish_req_mac_size = mac_size;
	mac_length_tmp = prepare_out_buffer(mac_length, sizeof(*mac_length));
	if (mac_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_mac_sign_finish_req_p_mac_length = (uint32_t)mac_length_tmp;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)mac_tmp, mac_size);
	sys_cache_data_flush_range((void *)mac_length_tmp, sizeof(*mac_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)mac_tmp, mac_size);
	sys_cache_data_flush_and_invd_range((void *)mac_length_tmp, sizeof(*mac_length));

fail:
	release_out_buffer(mac, mac_tmp, mac_size);
	release_out_buffer(mac_length, mac_length_tmp, sizeof(*mac_length));
	return result;
}

psa_status_t ssf_psa_mac_verify_finish(mbedtls_psa_client_handle_t *p_handle, const uint8_t *mac,
				       size_t mac_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_mac_verify_finish_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_mac_verify_finish_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_mac_verify_finish_req_m;

	req_data->psa_mac_verify_finish_req_p_handle = (uint32_t)p_handle;
	req_data->psa_mac_verify_finish_req_p_mac = (uint32_t)mac;
	req_data->psa_mac_verify_finish_req_mac_length = mac_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)mac, mac_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_mac_abort(mbedtls_psa_client_handle_t *p_handle)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_mac_abort_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_mac_abort_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_mac_abort_req_m;

	req_data->psa_mac_abort_req_p_handle = (uint32_t)p_handle;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_cipher_encrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				    const uint8_t *input, size_t input_length, uint8_t *output,
				    size_t output_size, size_t *output_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *output_tmp = NULL;
	void *output_length_tmp = NULL;

	struct psa_cipher_encrypt_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_cipher_encrypt_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_cipher_encrypt_req_m;

	req_data->psa_cipher_encrypt_req_key = key;
	req_data->psa_cipher_encrypt_req_alg = alg;
	req_data->psa_cipher_encrypt_req_p_input = (uint32_t)input;
	req_data->psa_cipher_encrypt_req_input_length = input_length;
	output_tmp = prepare_out_buffer(output, output_size);
	if (output_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_cipher_encrypt_req_p_output = (uint32_t)output_tmp;
	req_data->psa_cipher_encrypt_req_output_size = output_size;
	output_length_tmp = prepare_out_buffer(output_length, sizeof(*output_length));
	if (output_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_cipher_encrypt_req_p_output_length = (uint32_t)output_length_tmp;
	sys_cache_data_flush_range((void *)input, input_length);
	sys_cache_data_flush_range((void *)output_tmp, output_size);
	sys_cache_data_flush_range((void *)output_length_tmp, sizeof(*output_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)output_tmp, output_size);
	sys_cache_data_flush_and_invd_range((void *)output_length_tmp, sizeof(*output_length));

fail:
	release_out_buffer(output, output_tmp, output_size);
	release_out_buffer(output_length, output_length_tmp, sizeof(*output_length));
	return result;
}

psa_status_t ssf_psa_cipher_decrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				    const uint8_t *input, size_t input_length, uint8_t *output,
				    size_t output_size, size_t *output_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *output_tmp = NULL;
	void *output_length_tmp = NULL;

	struct psa_cipher_decrypt_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_cipher_decrypt_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_cipher_decrypt_req_m;

	req_data->psa_cipher_decrypt_req_key = key;
	req_data->psa_cipher_decrypt_req_alg = alg;
	req_data->psa_cipher_decrypt_req_p_input = (uint32_t)input;
	req_data->psa_cipher_decrypt_req_input_length = input_length;
	output_tmp = prepare_out_buffer(output, output_size);
	if (output_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_cipher_decrypt_req_p_output = (uint32_t)output_tmp;
	req_data->psa_cipher_decrypt_req_output_size = output_size;
	output_length_tmp = prepare_out_buffer(output_length, sizeof(*output_length));
	if (output_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_cipher_decrypt_req_p_output_length = (uint32_t)output_length_tmp;
	sys_cache_data_flush_range((void *)input, input_length);
	sys_cache_data_flush_range((void *)output_tmp, output_size);
	sys_cache_data_flush_range((void *)output_length_tmp, sizeof(*output_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)output_tmp, output_size);
	sys_cache_data_flush_and_invd_range((void *)output_length_tmp, sizeof(*output_length));

fail:
	release_out_buffer(output, output_tmp, output_size);
	release_out_buffer(output_length, output_length_tmp, sizeof(*output_length));
	return result;
}

psa_status_t ssf_psa_cipher_encrypt_setup(mbedtls_psa_client_handle_t *p_handle,
					  mbedtls_svc_key_id_t key, psa_algorithm_t alg)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_cipher_encrypt_setup_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_cipher_encrypt_setup_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_cipher_encrypt_setup_req_m;

	req_data->psa_cipher_encrypt_setup_req_p_handle = (uint32_t)p_handle;
	req_data->psa_cipher_encrypt_setup_req_key = key;
	req_data->psa_cipher_encrypt_setup_req_alg = alg;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_cipher_decrypt_setup(mbedtls_psa_client_handle_t *p_handle,
					  mbedtls_svc_key_id_t key, psa_algorithm_t alg)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_cipher_decrypt_setup_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_cipher_decrypt_setup_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_cipher_decrypt_setup_req_m;

	req_data->psa_cipher_decrypt_setup_req_p_handle = (uint32_t)p_handle;
	req_data->psa_cipher_decrypt_setup_req_key = key;
	req_data->psa_cipher_decrypt_setup_req_alg = alg;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_cipher_generate_iv(mbedtls_psa_client_handle_t *p_handle, uint8_t *iv,
					size_t iv_size, size_t *iv_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *iv_tmp = NULL;
	void *iv_length_tmp = NULL;

	struct psa_cipher_generate_iv_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_cipher_generate_iv_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_cipher_generate_iv_req_m;

	req_data->psa_cipher_generate_iv_req_p_handle = (uint32_t)p_handle;
	iv_tmp = prepare_out_buffer(iv, iv_size);
	if (iv_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_cipher_generate_iv_req_p_iv = (uint32_t)iv_tmp;
	req_data->psa_cipher_generate_iv_req_iv_size = iv_size;
	iv_length_tmp = prepare_out_buffer(iv_length, sizeof(*iv_length));
	if (iv_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_cipher_generate_iv_req_p_iv_length = (uint32_t)iv_length_tmp;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)iv_tmp, iv_size);
	sys_cache_data_flush_range((void *)iv_length_tmp, sizeof(*iv_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)iv_tmp, iv_size);
	sys_cache_data_flush_and_invd_range((void *)iv_length_tmp, sizeof(*iv_length));

fail:
	release_out_buffer(iv, iv_tmp, iv_size);
	release_out_buffer(iv_length, iv_length_tmp, sizeof(*iv_length));
	return result;
}

psa_status_t ssf_psa_cipher_set_iv(mbedtls_psa_client_handle_t *p_handle, const uint8_t *iv,
				   size_t iv_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_cipher_set_iv_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_cipher_set_iv_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_cipher_set_iv_req_m;

	req_data->psa_cipher_set_iv_req_p_handle = (uint32_t)p_handle;
	req_data->psa_cipher_set_iv_req_p_iv = (uint32_t)iv;
	req_data->psa_cipher_set_iv_req_iv_length = iv_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)iv, iv_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_cipher_update(mbedtls_psa_client_handle_t *p_handle, const uint8_t *input,
				   size_t input_length, uint8_t *output, size_t output_size,
				   size_t *output_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *output_tmp = NULL;
	void *output_length_tmp = NULL;

	struct psa_cipher_update_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_cipher_update_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_cipher_update_req_m;

	req_data->psa_cipher_update_req_p_handle = (uint32_t)p_handle;
	req_data->psa_cipher_update_req_p_input = (uint32_t)input;
	req_data->psa_cipher_update_req_input_length = input_length;
	output_tmp = prepare_out_buffer(output, output_size);
	if (output_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_cipher_update_req_p_output = (uint32_t)output_tmp;
	req_data->psa_cipher_update_req_output_size = output_size;
	output_length_tmp = prepare_out_buffer(output_length, sizeof(*output_length));
	if (output_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_cipher_update_req_p_output_length = (uint32_t)output_length_tmp;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)input, input_length);
	sys_cache_data_flush_range((void *)output_tmp, output_size);
	sys_cache_data_flush_range((void *)output_length_tmp, sizeof(*output_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)output_tmp, output_size);
	sys_cache_data_flush_and_invd_range((void *)output_length_tmp, sizeof(*output_length));

fail:
	release_out_buffer(output, output_tmp, output_size);
	release_out_buffer(output_length, output_length_tmp, sizeof(*output_length));
	return result;
}

psa_status_t ssf_psa_cipher_finish(mbedtls_psa_client_handle_t *p_handle, uint8_t *output,
				   size_t output_size, size_t *output_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *output_tmp = NULL;
	void *output_length_tmp = NULL;

	struct psa_cipher_finish_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_cipher_finish_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_cipher_finish_req_m;

	req_data->psa_cipher_finish_req_p_handle = (uint32_t)p_handle;
	output_tmp = prepare_out_buffer(output, output_size);
	if (output_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_cipher_finish_req_p_output = (uint32_t)output_tmp;
	req_data->psa_cipher_finish_req_output_size = output_size;
	output_length_tmp = prepare_out_buffer(output_length, sizeof(*output_length));
	if (output_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_cipher_finish_req_p_output_length = (uint32_t)output_length_tmp;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)output_tmp, output_size);
	sys_cache_data_flush_range((void *)output_length_tmp, sizeof(*output_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)output_tmp, output_size);
	sys_cache_data_flush_and_invd_range((void *)output_length_tmp, sizeof(*output_length));

fail:
	release_out_buffer(output, output_tmp, output_size);
	release_out_buffer(output_length, output_length_tmp, sizeof(*output_length));
	return result;
}

psa_status_t ssf_psa_cipher_abort(mbedtls_psa_client_handle_t *p_handle)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_cipher_abort_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_cipher_abort_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_cipher_abort_req_m;

	req_data->psa_cipher_abort_req_p_handle = (uint32_t)p_handle;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_aead_encrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				  const uint8_t *nonce, size_t nonce_length,
				  const uint8_t *additional_data, size_t additional_data_length,
				  const uint8_t *plaintext, size_t plaintext_length,
				  uint8_t *ciphertext, size_t ciphertext_size,
				  size_t *ciphertext_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *ciphertext_tmp = NULL;
	void *ciphertext_length_tmp = NULL;

	struct psa_aead_encrypt_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_aead_encrypt_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_aead_encrypt_req_m;

	req_data->psa_aead_encrypt_req_key = key;
	req_data->psa_aead_encrypt_req_alg = alg;
	req_data->psa_aead_encrypt_req_p_nonce = (uint32_t)nonce;
	req_data->psa_aead_encrypt_req_nonce_length = nonce_length;
	req_data->psa_aead_encrypt_req_p_additional_data = (uint32_t)additional_data;
	req_data->psa_aead_encrypt_req_additional_data_length = additional_data_length;
	req_data->psa_aead_encrypt_req_p_plaintext = (uint32_t)plaintext;
	req_data->psa_aead_encrypt_req_plaintext_length = plaintext_length;
	ciphertext_tmp = prepare_out_buffer(ciphertext, ciphertext_size);
	if (ciphertext_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_encrypt_req_p_ciphertext = (uint32_t)ciphertext_tmp;
	req_data->psa_aead_encrypt_req_ciphertext_size = ciphertext_size;
	ciphertext_length_tmp = prepare_out_buffer(ciphertext_length, sizeof(*ciphertext_length));
	if (ciphertext_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_encrypt_req_p_ciphertext_length = (uint32_t)ciphertext_length_tmp;
	sys_cache_data_flush_range((void *)nonce, nonce_length);
	sys_cache_data_flush_range((void *)additional_data, additional_data_length);
	sys_cache_data_flush_range((void *)plaintext, plaintext_length);
	sys_cache_data_flush_range((void *)ciphertext_tmp, ciphertext_size);
	sys_cache_data_flush_range((void *)ciphertext_length_tmp, sizeof(*ciphertext_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)ciphertext_tmp, ciphertext_size);
	sys_cache_data_flush_and_invd_range((void *)ciphertext_length_tmp,
					    sizeof(*ciphertext_length));

fail:
	release_out_buffer(ciphertext, ciphertext_tmp, ciphertext_size);
	release_out_buffer(ciphertext_length, ciphertext_length_tmp, sizeof(*ciphertext_length));
	return result;
}

psa_status_t ssf_psa_aead_decrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				  const uint8_t *nonce, size_t nonce_length,
				  const uint8_t *additional_data, size_t additional_data_length,
				  const uint8_t *ciphertext, size_t ciphertext_length,
				  uint8_t *plaintext, size_t plaintext_size,
				  size_t *plaintext_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *plaintext_tmp = NULL;
	void *plaintext_length_tmp = NULL;

	struct psa_aead_decrypt_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_aead_decrypt_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_aead_decrypt_req_m;

	req_data->psa_aead_decrypt_req_key = key;
	req_data->psa_aead_decrypt_req_alg = alg;
	req_data->psa_aead_decrypt_req_p_nonce = (uint32_t)nonce;
	req_data->psa_aead_decrypt_req_nonce_length = nonce_length;
	req_data->psa_aead_decrypt_req_p_additional_data = (uint32_t)additional_data;
	req_data->psa_aead_decrypt_req_additional_data_length = additional_data_length;
	req_data->psa_aead_decrypt_req_p_ciphertext = (uint32_t)ciphertext;
	req_data->psa_aead_decrypt_req_ciphertext_length = ciphertext_length;
	plaintext_tmp = prepare_out_buffer(plaintext, plaintext_size);
	if (plaintext_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_decrypt_req_p_plaintext = (uint32_t)plaintext_tmp;
	req_data->psa_aead_decrypt_req_plaintext_size = plaintext_size;
	plaintext_length_tmp = prepare_out_buffer(plaintext_length, sizeof(*plaintext_length));
	if (plaintext_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_decrypt_req_p_plaintext_length = (uint32_t)plaintext_length_tmp;
	sys_cache_data_flush_range((void *)nonce, nonce_length);
	sys_cache_data_flush_range((void *)additional_data, additional_data_length);
	sys_cache_data_flush_range((void *)ciphertext, ciphertext_length);
	sys_cache_data_flush_range((void *)plaintext_tmp, plaintext_size);
	sys_cache_data_flush_range((void *)plaintext_length_tmp, sizeof(*plaintext_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)plaintext_tmp, plaintext_size);
	sys_cache_data_flush_and_invd_range((void *)plaintext_length_tmp,
					    sizeof(*plaintext_length));

fail:
	release_out_buffer(plaintext, plaintext_tmp, plaintext_size);
	release_out_buffer(plaintext_length, plaintext_length_tmp, sizeof(*plaintext_length));
	return result;
}

psa_status_t ssf_psa_aead_encrypt_setup(mbedtls_psa_client_handle_t *p_handle,
					mbedtls_svc_key_id_t key, psa_algorithm_t alg)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_aead_encrypt_setup_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_aead_encrypt_setup_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_aead_encrypt_setup_req_m;

	req_data->psa_aead_encrypt_setup_req_p_handle = (uint32_t)p_handle;
	req_data->psa_aead_encrypt_setup_req_key = key;
	req_data->psa_aead_encrypt_setup_req_alg = alg;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_aead_decrypt_setup(mbedtls_psa_client_handle_t *p_handle,
					mbedtls_svc_key_id_t key, psa_algorithm_t alg)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_aead_decrypt_setup_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_aead_decrypt_setup_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_aead_decrypt_setup_req_m;

	req_data->psa_aead_decrypt_setup_req_p_handle = (uint32_t)p_handle;
	req_data->psa_aead_decrypt_setup_req_key = key;
	req_data->psa_aead_decrypt_setup_req_alg = alg;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_aead_generate_nonce(mbedtls_psa_client_handle_t *p_handle, uint8_t *nonce,
					 size_t nonce_size, size_t *nonce_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *nonce_tmp = NULL;
	void *nonce_length_tmp = NULL;

	struct psa_aead_generate_nonce_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_aead_generate_nonce_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_aead_generate_nonce_req_m;

	req_data->psa_aead_generate_nonce_req_p_handle = (uint32_t)p_handle;
	nonce_tmp = prepare_out_buffer(nonce, nonce_size);
	if (nonce_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_generate_nonce_req_p_nonce = (uint32_t)nonce_tmp;
	req_data->psa_aead_generate_nonce_req_nonce_size = nonce_size;
	nonce_length_tmp = prepare_out_buffer(nonce_length, sizeof(*nonce_length));
	if (nonce_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_generate_nonce_req_p_nonce_length = (uint32_t)nonce_length_tmp;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)nonce_tmp, nonce_size);
	sys_cache_data_flush_range((void *)nonce_length_tmp, sizeof(*nonce_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)nonce_tmp, nonce_size);
	sys_cache_data_flush_and_invd_range((void *)nonce_length_tmp, sizeof(*nonce_length));

fail:
	release_out_buffer(nonce, nonce_tmp, nonce_size);
	release_out_buffer(nonce_length, nonce_length_tmp, sizeof(*nonce_length));
	return result;
}

psa_status_t ssf_psa_aead_set_nonce(mbedtls_psa_client_handle_t *p_handle, const uint8_t *nonce,
				    size_t nonce_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_aead_set_nonce_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_aead_set_nonce_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_aead_set_nonce_req_m;

	req_data->psa_aead_set_nonce_req_p_handle = (uint32_t)p_handle;
	req_data->psa_aead_set_nonce_req_p_nonce = (uint32_t)nonce;
	req_data->psa_aead_set_nonce_req_nonce_length = nonce_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)nonce, nonce_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_aead_set_lengths(mbedtls_psa_client_handle_t *p_handle, size_t ad_length,
				      size_t plaintext_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_aead_set_lengths_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_aead_set_lengths_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_aead_set_lengths_req_m;

	req_data->psa_aead_set_lengths_req_p_handle = (uint32_t)p_handle;
	req_data->psa_aead_set_lengths_req_ad_length = ad_length;
	req_data->psa_aead_set_lengths_req_plaintext_length = plaintext_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_aead_update_ad(mbedtls_psa_client_handle_t *p_handle, const uint8_t *input,
				    size_t input_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_aead_update_ad_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_aead_update_ad_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_aead_update_ad_req_m;

	req_data->psa_aead_update_ad_req_p_handle = (uint32_t)p_handle;
	req_data->psa_aead_update_ad_req_p_input = (uint32_t)input;
	req_data->psa_aead_update_ad_req_input_length = input_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)input, input_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_aead_update(mbedtls_psa_client_handle_t *p_handle, const uint8_t *input,
				 size_t input_length, uint8_t *output, size_t output_size,
				 size_t *output_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *output_tmp = NULL;
	void *output_length_tmp = NULL;

	struct psa_aead_update_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_aead_update_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_aead_update_req_m;

	req_data->psa_aead_update_req_p_handle = (uint32_t)p_handle;
	req_data->psa_aead_update_req_p_input = (uint32_t)input;
	req_data->psa_aead_update_req_input_length = input_length;
	output_tmp = prepare_out_buffer(output, output_size);
	if (output_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_update_req_p_output = (uint32_t)output_tmp;
	req_data->psa_aead_update_req_output_size = output_size;
	output_length_tmp = prepare_out_buffer(output_length, sizeof(*output_length));
	if (output_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_update_req_p_output_length = (uint32_t)output_length_tmp;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)input, input_length);
	sys_cache_data_flush_range((void *)output_tmp, output_size);
	sys_cache_data_flush_range((void *)output_length_tmp, sizeof(*output_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)output_tmp, output_size);
	sys_cache_data_flush_and_invd_range((void *)output_length_tmp, sizeof(*output_length));

fail:
	release_out_buffer(output, output_tmp, output_size);
	release_out_buffer(output_length, output_length_tmp, sizeof(*output_length));
	return result;
}

psa_status_t ssf_psa_aead_finish(mbedtls_psa_client_handle_t *p_handle, uint8_t *ciphertext,
				 size_t ciphertext_size, size_t *ciphertext_length, uint8_t *tag,
				 size_t tag_size, size_t *tag_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *ciphertext_tmp = NULL;
	void *ciphertext_length_tmp = NULL;
	void *tag_tmp = NULL;
	void *tag_length_tmp = NULL;

	struct psa_aead_finish_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_aead_finish_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_aead_finish_req_m;

	req_data->psa_aead_finish_req_p_handle = (uint32_t)p_handle;
	ciphertext_tmp = prepare_out_buffer(ciphertext, ciphertext_size);
	if (ciphertext_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_finish_req_p_ciphertext = (uint32_t)ciphertext_tmp;
	req_data->psa_aead_finish_req_ciphertext_size = ciphertext_size;
	ciphertext_length_tmp = prepare_out_buffer(ciphertext_length, sizeof(*ciphertext_length));
	if (ciphertext_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_finish_req_p_ciphertext_length = (uint32_t)ciphertext_length_tmp;
	tag_tmp = prepare_out_buffer(tag, tag_size);
	if (tag_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_finish_req_p_tag = (uint32_t)tag_tmp;
	req_data->psa_aead_finish_req_tag_size = tag_size;
	tag_length_tmp = prepare_out_buffer(tag_length, sizeof(*tag_length));
	if (tag_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_finish_req_p_tag_length = (uint32_t)tag_length_tmp;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)ciphertext_tmp, ciphertext_size);
	sys_cache_data_flush_range((void *)ciphertext_length_tmp, sizeof(*ciphertext_length));
	sys_cache_data_flush_range((void *)tag_tmp, tag_size);
	sys_cache_data_flush_range((void *)tag_length_tmp, sizeof(*tag_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)ciphertext_tmp, ciphertext_size);
	sys_cache_data_flush_and_invd_range((void *)ciphertext_length_tmp,
					    sizeof(*ciphertext_length));
	sys_cache_data_flush_and_invd_range((void *)tag_tmp, tag_size);
	sys_cache_data_flush_and_invd_range((void *)tag_length_tmp, sizeof(*tag_length));

fail:
	release_out_buffer(ciphertext, ciphertext_tmp, ciphertext_size);
	release_out_buffer(ciphertext_length, ciphertext_length_tmp, sizeof(*ciphertext_length));
	release_out_buffer(tag, tag_tmp, tag_size);
	release_out_buffer(tag_length, tag_length_tmp, sizeof(*tag_length));
	return result;
}

psa_status_t ssf_psa_aead_verify(mbedtls_psa_client_handle_t *p_handle, uint8_t *plaintext,
				 size_t plaintext_size, size_t *plaintext_length,
				 const uint8_t *tag, size_t tag_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *plaintext_tmp = NULL;
	void *plaintext_length_tmp = NULL;

	struct psa_aead_verify_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_aead_verify_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_aead_verify_req_m;

	req_data->psa_aead_verify_req_p_handle = (uint32_t)p_handle;
	plaintext_tmp = prepare_out_buffer(plaintext, plaintext_size);
	if (plaintext_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_verify_req_p_plaintext = (uint32_t)plaintext_tmp;
	req_data->psa_aead_verify_req_plaintext_size = plaintext_size;
	plaintext_length_tmp = prepare_out_buffer(plaintext_length, sizeof(*plaintext_length));
	if (plaintext_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_aead_verify_req_p_plaintext_length = (uint32_t)plaintext_length_tmp;
	req_data->psa_aead_verify_req_p_tag = (uint32_t)tag;
	req_data->psa_aead_verify_req_tag_length = tag_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)plaintext_tmp, plaintext_size);
	sys_cache_data_flush_range((void *)plaintext_length_tmp, sizeof(*plaintext_length));
	sys_cache_data_flush_range((void *)tag, tag_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)plaintext_tmp, plaintext_size);
	sys_cache_data_flush_and_invd_range((void *)plaintext_length_tmp,
					    sizeof(*plaintext_length));

fail:
	release_out_buffer(plaintext, plaintext_tmp, plaintext_size);
	release_out_buffer(plaintext_length, plaintext_length_tmp, sizeof(*plaintext_length));
	return result;
}

psa_status_t ssf_psa_aead_abort(mbedtls_psa_client_handle_t *p_handle)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_aead_abort_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_aead_abort_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_aead_abort_req_m;

	req_data->psa_aead_abort_req_p_handle = (uint32_t)p_handle;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_sign_message(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				  const uint8_t *input, size_t input_length, uint8_t *signature,
				  size_t signature_size, size_t *signature_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *signature_tmp = NULL;
	void *signature_length_tmp = NULL;

	struct psa_sign_message_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_sign_message_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_sign_message_req_m;

	req_data->psa_sign_message_req_key = key;
	req_data->psa_sign_message_req_alg = alg;
	req_data->psa_sign_message_req_p_input = (uint32_t)input;
	req_data->psa_sign_message_req_input_length = input_length;
	signature_tmp = prepare_out_buffer(signature, signature_size);
	if (signature_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_sign_message_req_p_signature = (uint32_t)signature_tmp;
	req_data->psa_sign_message_req_signature_size = signature_size;
	signature_length_tmp = prepare_out_buffer(signature_length, sizeof(*signature_length));
	if (signature_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_sign_message_req_p_signature_length = (uint32_t)signature_length_tmp;
	sys_cache_data_flush_range((void *)input, input_length);
	sys_cache_data_flush_range((void *)signature_tmp, signature_size);
	sys_cache_data_flush_range((void *)signature_length_tmp, sizeof(*signature_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)signature_tmp, signature_size);
	sys_cache_data_flush_and_invd_range((void *)signature_length_tmp,
					    sizeof(*signature_length));

fail:
	release_out_buffer(signature, signature_tmp, signature_size);
	release_out_buffer(signature_length, signature_length_tmp, sizeof(*signature_length));
	return result;
}

psa_status_t ssf_psa_verify_message(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				    const uint8_t *input, size_t input_length,
				    const uint8_t *signature, size_t signature_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_verify_message_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_verify_message_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_verify_message_req_m;

	req_data->psa_verify_message_req_key = key;
	req_data->psa_verify_message_req_alg = alg;
	req_data->psa_verify_message_req_p_input = (uint32_t)input;
	req_data->psa_verify_message_req_input_length = input_length;
	req_data->psa_verify_message_req_p_signature = (uint32_t)signature;
	req_data->psa_verify_message_req_signature_length = signature_length;
	sys_cache_data_flush_range((void *)input, input_length);
	sys_cache_data_flush_range((void *)signature, signature_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}

fail:
	return result;
}

psa_status_t ssf_psa_sign_hash(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
			       size_t hash_length, uint8_t *signature, size_t signature_size,
			       size_t *signature_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *signature_tmp = NULL;
	void *signature_length_tmp = NULL;

	struct psa_sign_hash_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_sign_hash_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_sign_hash_req_m;

	req_data->psa_sign_hash_req_key = key;
	req_data->psa_sign_hash_req_alg = alg;
	req_data->psa_sign_hash_req_p_hash = (uint32_t)hash;
	req_data->psa_sign_hash_req_hash_length = hash_length;
	signature_tmp = prepare_out_buffer(signature, signature_size);
	if (signature_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_sign_hash_req_p_signature = (uint32_t)signature_tmp;
	req_data->psa_sign_hash_req_signature_size = signature_size;
	signature_length_tmp = prepare_out_buffer(signature_length, sizeof(*signature_length));
	if (signature_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_sign_hash_req_p_signature_length = (uint32_t)signature_length_tmp;
	sys_cache_data_flush_range((void *)hash, hash_length);
	sys_cache_data_flush_range((void *)signature_tmp, signature_size);
	sys_cache_data_flush_range((void *)signature_length_tmp, sizeof(*signature_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)signature_tmp, signature_size);
	sys_cache_data_flush_and_invd_range((void *)signature_length_tmp,
					    sizeof(*signature_length));

fail:
	release_out_buffer(signature, signature_tmp, signature_size);
	release_out_buffer(signature_length, signature_length_tmp, sizeof(*signature_length));
	return result;
}

psa_status_t ssf_psa_verify_hash(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
				 size_t hash_length, const uint8_t *signature,
				 size_t signature_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_verify_hash_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_verify_hash_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_verify_hash_req_m;

	req_data->psa_verify_hash_req_key = key;
	req_data->psa_verify_hash_req_alg = alg;
	req_data->psa_verify_hash_req_p_hash = (uint32_t)hash;
	req_data->psa_verify_hash_req_hash_length = hash_length;
	req_data->psa_verify_hash_req_p_signature = (uint32_t)signature;
	req_data->psa_verify_hash_req_signature_length = signature_length;
	sys_cache_data_flush_range((void *)hash, hash_length);
	sys_cache_data_flush_range((void *)signature, signature_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}

fail:
	return result;
}

psa_status_t ssf_psa_asymmetric_encrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
					const uint8_t *input, size_t input_length,
					const uint8_t *salt, size_t salt_length, uint8_t *output,
					size_t output_size, size_t *output_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *output_tmp = NULL;
	void *output_length_tmp = NULL;

	struct psa_asymmetric_encrypt_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_asymmetric_encrypt_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_asymmetric_encrypt_req_m;

	req_data->psa_asymmetric_encrypt_req_key = key;
	req_data->psa_asymmetric_encrypt_req_alg = alg;
	req_data->psa_asymmetric_encrypt_req_p_input = (uint32_t)input;
	req_data->psa_asymmetric_encrypt_req_input_length = input_length;
	req_data->psa_asymmetric_encrypt_req_p_salt = (uint32_t)salt;
	req_data->psa_asymmetric_encrypt_req_salt_length = salt_length;
	output_tmp = prepare_out_buffer(output, output_size);
	if (output_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_asymmetric_encrypt_req_p_output = (uint32_t)output_tmp;
	req_data->psa_asymmetric_encrypt_req_output_size = output_size;
	output_length_tmp = prepare_out_buffer(output_length, sizeof(*output_length));
	if (output_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_asymmetric_encrypt_req_p_output_length = (uint32_t)output_length_tmp;
	sys_cache_data_flush_range((void *)input, input_length);
	sys_cache_data_flush_range((void *)salt, salt_length);
	sys_cache_data_flush_range((void *)output_tmp, output_size);
	sys_cache_data_flush_range((void *)output_length_tmp, sizeof(*output_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)output_tmp, output_size);
	sys_cache_data_flush_and_invd_range((void *)output_length_tmp, sizeof(*output_length));

fail:
	release_out_buffer(output, output_tmp, output_size);
	release_out_buffer(output_length, output_length_tmp, sizeof(*output_length));
	return result;
}

psa_status_t ssf_psa_asymmetric_decrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
					const uint8_t *input, size_t input_length,
					const uint8_t *salt, size_t salt_length, uint8_t *output,
					size_t output_size, size_t *output_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *output_tmp = NULL;
	void *output_length_tmp = NULL;

	struct psa_asymmetric_decrypt_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_asymmetric_decrypt_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_asymmetric_decrypt_req_m;

	req_data->psa_asymmetric_decrypt_req_key = key;
	req_data->psa_asymmetric_decrypt_req_alg = alg;
	req_data->psa_asymmetric_decrypt_req_p_input = (uint32_t)input;
	req_data->psa_asymmetric_decrypt_req_input_length = input_length;
	req_data->psa_asymmetric_decrypt_req_p_salt = (uint32_t)salt;
	req_data->psa_asymmetric_decrypt_req_salt_length = salt_length;
	output_tmp = prepare_out_buffer(output, output_size);
	if (output_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_asymmetric_decrypt_req_p_output = (uint32_t)output_tmp;
	req_data->psa_asymmetric_decrypt_req_output_size = output_size;
	output_length_tmp = prepare_out_buffer(output_length, sizeof(*output_length));
	if (output_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_asymmetric_decrypt_req_p_output_length = (uint32_t)output_length_tmp;
	sys_cache_data_flush_range((void *)input, input_length);
	sys_cache_data_flush_range((void *)salt, salt_length);
	sys_cache_data_flush_range((void *)output_tmp, output_size);
	sys_cache_data_flush_range((void *)output_length_tmp, sizeof(*output_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)output_tmp, output_size);
	sys_cache_data_flush_and_invd_range((void *)output_length_tmp, sizeof(*output_length));

fail:
	release_out_buffer(output, output_tmp, output_size);
	release_out_buffer(output_length, output_length_tmp, sizeof(*output_length));
	return result;
}

psa_status_t ssf_psa_key_derivation_setup(mbedtls_psa_client_handle_t *p_handle,
					  psa_algorithm_t alg)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_key_derivation_setup_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_key_derivation_setup_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_key_derivation_setup_req_m;

	req_data->psa_key_derivation_setup_req_p_handle = (uint32_t)p_handle;
	req_data->psa_key_derivation_setup_req_alg = alg;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_key_derivation_get_capacity(mbedtls_psa_client_handle_t handle,
						 size_t *capacity)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *capacity_tmp = NULL;

	struct psa_key_derivation_get_capacity_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_key_derivation_get_capacity_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_key_derivation_get_capacity_req_m;

	req_data->psa_key_derivation_get_capacity_req_handle = (uint32_t)handle;
	capacity_tmp = prepare_out_buffer(capacity, sizeof(*capacity));
	if (capacity_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_key_derivation_get_capacity_req_p_capacity = (uint32_t)capacity_tmp;
	sys_cache_data_flush_range((void *)capacity_tmp, sizeof(*capacity));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)capacity_tmp, sizeof(*capacity));

fail:
	release_out_buffer(capacity, capacity_tmp, sizeof(*capacity));
	return result;
}

psa_status_t ssf_psa_key_derivation_set_capacity(mbedtls_psa_client_handle_t *p_handle,
						 size_t capacity)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_key_derivation_set_capacity_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_key_derivation_set_capacity_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_key_derivation_set_capacity_req_m;

	req_data->psa_key_derivation_set_capacity_req_p_handle = (uint32_t)p_handle;
	req_data->psa_key_derivation_set_capacity_req_capacity = capacity;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_key_derivation_input_bytes(mbedtls_psa_client_handle_t *p_handle,
						psa_key_derivation_step_t step, const uint8_t *data,
						size_t data_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_key_derivation_input_bytes_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_key_derivation_input_bytes_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_key_derivation_input_bytes_req_m;

	req_data->psa_key_derivation_input_bytes_req_p_handle = (uint32_t)p_handle;
	req_data->psa_key_derivation_input_bytes_req_step = step;
	req_data->psa_key_derivation_input_bytes_req_p_data = (uint32_t)data;
	req_data->psa_key_derivation_input_bytes_req_data_length = data_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)data, data_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_key_derivation_input_integer(mbedtls_psa_client_handle_t *p_handle,
						  psa_key_derivation_step_t step, uint64_t value)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_key_derivation_input_integer_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_key_derivation_input_integer_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_key_derivation_input_integer_req_m;

	req_data->psa_key_derivation_input_integer_req_p_handle = (uint32_t)p_handle;
	req_data->psa_key_derivation_input_integer_req_step = step;
	req_data->psa_key_derivation_input_integer_req_value = value;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_key_derivation_input_key(mbedtls_psa_client_handle_t *p_handle,
					      psa_key_derivation_step_t step,
					      mbedtls_svc_key_id_t key)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_key_derivation_input_key_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_key_derivation_input_key_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_key_derivation_input_key_req_m;

	req_data->psa_key_derivation_input_key_req_p_handle = (uint32_t)p_handle;
	req_data->psa_key_derivation_input_key_req_step = step;
	req_data->psa_key_derivation_input_key_req_key = key;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_key_derivation_key_agreement(mbedtls_psa_client_handle_t *p_handle,
						  psa_key_derivation_step_t step,
						  mbedtls_svc_key_id_t private_key,
						  const uint8_t *peer_key, size_t peer_key_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_key_derivation_key_agreement_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_key_derivation_key_agreement_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_key_derivation_key_agreement_req_m;

	req_data->psa_key_derivation_key_agreement_req_p_handle = (uint32_t)p_handle;
	req_data->psa_key_derivation_key_agreement_req_step = step;
	req_data->psa_key_derivation_key_agreement_req_private_key = private_key;
	req_data->psa_key_derivation_key_agreement_req_p_peer_key = (uint32_t)peer_key;
	req_data->psa_key_derivation_key_agreement_req_peer_key_length = peer_key_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)peer_key, peer_key_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_key_derivation_output_bytes(mbedtls_psa_client_handle_t *p_handle,
						 uint8_t *output, size_t output_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *output_tmp = NULL;

	struct psa_key_derivation_output_bytes_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_key_derivation_output_bytes_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_key_derivation_output_bytes_req_m;

	req_data->psa_key_derivation_output_bytes_req_p_handle = (uint32_t)p_handle;
	output_tmp = prepare_out_buffer(output, output_length);
	if (output_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_key_derivation_output_bytes_req_p_output = (uint32_t)output_tmp;
	req_data->psa_key_derivation_output_bytes_req_output_length = output_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)output_tmp, output_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)output_tmp, output_length);

fail:
	release_out_buffer(output, output_tmp, output_length);
	return result;
}

psa_status_t ssf_psa_key_derivation_output_key(const psa_key_attributes_t *attributes,
					       mbedtls_psa_client_handle_t *p_handle,
					       mbedtls_svc_key_id_t *key)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *key_tmp = NULL;

	struct psa_key_derivation_output_key_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_key_derivation_output_key_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_key_derivation_output_key_req_m;

	req_data->psa_key_derivation_output_key_req_p_attributes = (uint32_t)attributes;
	req_data->psa_key_derivation_output_key_req_p_handle = (uint32_t)p_handle;
	key_tmp = prepare_out_buffer(key, sizeof(*key));
	if (key_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_key_derivation_output_key_req_p_key = (uint32_t)key_tmp;
	sys_cache_data_flush_range((void *)attributes, sizeof(*attributes));
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)key_tmp, sizeof(*key));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)key_tmp, sizeof(*key));

fail:
	release_out_buffer(key, key_tmp, sizeof(*key));
	return result;
}

psa_status_t ssf_psa_key_derivation_abort(mbedtls_psa_client_handle_t *p_handle)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_key_derivation_abort_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_key_derivation_abort_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_key_derivation_abort_req_m;

	req_data->psa_key_derivation_abort_req_p_handle = (uint32_t)p_handle;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_raw_key_agreement(psa_algorithm_t alg, mbedtls_svc_key_id_t private_key,
				       const uint8_t *peer_key, size_t peer_key_length,
				       uint8_t *output, size_t output_size, size_t *output_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *output_tmp = NULL;
	void *output_length_tmp = NULL;

	struct psa_raw_key_agreement_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_raw_key_agreement_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_raw_key_agreement_req_m;

	req_data->psa_raw_key_agreement_req_alg = alg;
	req_data->psa_raw_key_agreement_req_private_key = private_key;
	req_data->psa_raw_key_agreement_req_p_peer_key = (uint32_t)peer_key;
	req_data->psa_raw_key_agreement_req_peer_key_length = peer_key_length;
	output_tmp = prepare_out_buffer(output, output_size);
	if (output_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_raw_key_agreement_req_p_output = (uint32_t)output_tmp;
	req_data->psa_raw_key_agreement_req_output_size = output_size;
	output_length_tmp = prepare_out_buffer(output_length, sizeof(*output_length));
	if (output_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_raw_key_agreement_req_p_output_length = (uint32_t)output_length_tmp;
	sys_cache_data_flush_range((void *)peer_key, peer_key_length);
	sys_cache_data_flush_range((void *)output_tmp, output_size);
	sys_cache_data_flush_range((void *)output_length_tmp, sizeof(*output_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)output_tmp, output_size);
	sys_cache_data_flush_and_invd_range((void *)output_length_tmp, sizeof(*output_length));

fail:
	release_out_buffer(output, output_tmp, output_size);
	release_out_buffer(output_length, output_length_tmp, sizeof(*output_length));
	return result;
}

psa_status_t ssf_psa_generate_random(uint8_t *output, size_t output_size)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *output_tmp = NULL;

	struct psa_generate_random_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_generate_random_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_generate_random_req_m;

	output_tmp = prepare_out_buffer(output, output_size);
	if (output_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_generate_random_req_p_output = (uint32_t)output_tmp;
	req_data->psa_generate_random_req_output_size = output_size;
	sys_cache_data_flush_range((void *)output_tmp, output_size);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)output_tmp, output_size);

fail:
	release_out_buffer(output, output_tmp, output_size);
	return result;
}

psa_status_t ssf_psa_generate_key(const psa_key_attributes_t *attributes, mbedtls_svc_key_id_t *key)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *key_tmp = NULL;

	struct psa_generate_key_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_generate_key_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_generate_key_req_m;

	req_data->psa_generate_key_req_p_attributes = (uint32_t)attributes;
	key_tmp = prepare_out_buffer(key, sizeof(*key));
	if (key_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_generate_key_req_p_key = (uint32_t)key_tmp;
	sys_cache_data_flush_range((void *)attributes, sizeof(*attributes));
	sys_cache_data_flush_range((void *)key_tmp, sizeof(*key));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)key_tmp, sizeof(*key));

fail:
	release_out_buffer(key, key_tmp, sizeof(*key));
	return result;
}

psa_status_t ssf_psa_pake_setup(mbedtls_psa_client_handle_t *p_handle,
				mbedtls_svc_key_id_t password_key,
				const psa_pake_cipher_suite_t *cipher_suite)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_pake_setup_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_pake_setup_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_pake_setup_req_m;

	req_data->psa_pake_setup_req_p_handle = (uint32_t)p_handle;
	req_data->psa_pake_setup_req_password_key = password_key;
	req_data->psa_pake_setup_req_p_cipher_suite = (uint32_t)cipher_suite;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)cipher_suite, sizeof(*cipher_suite));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_pake_set_role(mbedtls_psa_client_handle_t *p_handle, psa_pake_role_t role)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_pake_set_role_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_pake_set_role_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_pake_set_role_req_m;

	req_data->psa_pake_set_role_req_p_handle = (uint32_t)p_handle;
	req_data->psa_pake_set_role_req_role = role;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_pake_set_user(mbedtls_psa_client_handle_t *p_handle, const uint8_t *user_id,
				   size_t user_id_len)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_pake_set_user_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_pake_set_user_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_pake_set_user_req_m;

	req_data->psa_pake_set_user_req_p_handle = (uint32_t)p_handle;
	req_data->psa_pake_set_user_req_p_user_id = (uint32_t)user_id;
	req_data->psa_pake_set_user_req_user_id_len = user_id_len;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)user_id, user_id_len);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_pake_set_peer(mbedtls_psa_client_handle_t *p_handle, const uint8_t *peer_id,
				   size_t peer_id_len)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_pake_set_peer_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_pake_set_peer_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_pake_set_peer_req_m;

	req_data->psa_pake_set_peer_req_p_handle = (uint32_t)p_handle;
	req_data->psa_pake_set_peer_req_p_peer_id = (uint32_t)peer_id;
	req_data->psa_pake_set_peer_req_peer_id_len = peer_id_len;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)peer_id, peer_id_len);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_pake_set_context(mbedtls_psa_client_handle_t *p_handle, const uint8_t *context,
				      size_t context_len)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_pake_set_context_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_pake_set_context_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_pake_set_context_req_m;

	req_data->psa_pake_set_context_req_p_handle = (uint32_t)p_handle;
	req_data->psa_pake_set_context_req_p_context = (uint32_t)context;
	req_data->psa_pake_set_context_req_context_len = context_len;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)context, context_len);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_pake_output(mbedtls_psa_client_handle_t *p_handle, psa_pake_step_t step,
				 uint8_t *output, size_t output_size, size_t *output_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *output_tmp = NULL;
	void *output_length_tmp = NULL;

	struct psa_pake_output_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_pake_output_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_pake_output_req_m;

	req_data->psa_pake_output_req_p_handle = (uint32_t)p_handle;
	req_data->psa_pake_output_req_step = step;
	output_tmp = prepare_out_buffer(output, output_size);
	if (output_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_pake_output_req_p_output = (uint32_t)output_tmp;
	req_data->psa_pake_output_req_output_size = output_size;
	output_length_tmp = prepare_out_buffer(output_length, sizeof(*output_length));
	if (output_length_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_pake_output_req_p_output_length = (uint32_t)output_length_tmp;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)output_tmp, output_size);
	sys_cache_data_flush_range((void *)output_length_tmp, sizeof(*output_length));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)output_tmp, output_size);
	sys_cache_data_flush_and_invd_range((void *)output_length_tmp, sizeof(*output_length));

fail:
	release_out_buffer(output, output_tmp, output_size);
	release_out_buffer(output_length, output_length_tmp, sizeof(*output_length));
	return result;
}

psa_status_t ssf_psa_pake_input(mbedtls_psa_client_handle_t *p_handle, psa_pake_step_t step,
				const uint8_t *input, size_t input_length)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_pake_input_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_pake_input_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_pake_input_req_m;

	req_data->psa_pake_input_req_p_handle = (uint32_t)p_handle;
	req_data->psa_pake_input_req_step = step;
	req_data->psa_pake_input_req_p_input = (uint32_t)input;
	req_data->psa_pake_input_req_input_length = input_length;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)input, input_length);

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}

psa_status_t ssf_psa_pake_get_shared_key(mbedtls_psa_client_handle_t *p_handle,
					 const psa_key_attributes_t *attributes,
					 mbedtls_svc_key_id_t *key)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };
	void *key_tmp = NULL;

	struct psa_pake_get_shared_key_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_pake_get_shared_key_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_pake_get_shared_key_req_m;

	req_data->psa_pake_get_shared_key_req_p_handle = (uint32_t)p_handle;
	req_data->psa_pake_get_shared_key_req_p_attributes = (uint32_t)attributes;
	key_tmp = prepare_out_buffer(key, sizeof(*key));
	if (key_tmp == NULL) {
		result = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto fail;
	}
	req_data->psa_pake_get_shared_key_req_p_key = (uint32_t)key_tmp;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_range((void *)attributes, sizeof(*attributes));
	sys_cache_data_flush_range((void *)key_tmp, sizeof(*key));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));
	sys_cache_data_flush_and_invd_range((void *)key_tmp, sizeof(*key));

fail:
	release_out_buffer(key, key_tmp, sizeof(*key));
	return result;
}

psa_status_t ssf_psa_pake_abort(mbedtls_psa_client_handle_t *p_handle)
{
	int err;
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	struct psa_crypto_req req = { 0 };
	struct psa_crypto_rsp rsp = { 0 };

	struct psa_pake_abort_req *req_data;
	req.psa_crypto_req_msg_choice = psa_crypto_req_msg_psa_pake_abort_req_m_c;
	req_data = &req.psa_crypto_req_msg_psa_pake_abort_req_m;

	req_data->psa_pake_abort_req_p_handle = (uint32_t)p_handle;
	sys_cache_data_flush_range((void *)p_handle, sizeof(*p_handle));

	err = ssf_client_send_request(&psa_crypto_srvc, &req, &rsp, NULL);
	if (err != 0) {
		result = PSA_ERROR_COMMUNICATION_FAILURE;
		goto fail;
	} else {
		result = rsp.psa_crypto_rsp_status;
	}
	sys_cache_data_flush_and_invd_range((void *)p_handle, sizeof(*p_handle));

fail:
	return result;
}
