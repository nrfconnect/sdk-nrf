/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include "key_wrap_test_logic.h"

#define KEY_SIZE 16
#define KEY_BITS 128
#define WRAPPED_SIZE 24

static uint8_t wrapped_key[WRAPPED_SIZE];
static uint8_t unwrapped_key[KEY_SIZE];
static size_t wrapped_length;
static size_t unwrapped_length;
static psa_key_id_t wrapping_key_id;
static psa_key_id_t source_key_id;
static psa_key_id_t unwrapped_key_id;
static bool wrapping_key_created;
static bool source_key_created;
static bool unwrapped_key_created;

/* The attributes of the key that is wrapped and then recovered. */
static void set_source_key_attributes(psa_key_attributes_t *attributes,
				      psa_algorithm_t algorithm)
{
	psa_set_key_usage_flags(attributes, PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(attributes, algorithm);
	psa_set_key_type(attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(attributes, KEY_BITS);
}

static psa_status_t generate_wrapping_key(const void *context)
{
	const struct key_wrap_test_data *test = context;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_WRAP | PSA_KEY_USAGE_UNWRAP);
	psa_set_key_algorithm(&attributes, test->algorithm);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, test->key_bits);

	status = psa_generate_key(&attributes, &wrapping_key_id);
	psa_reset_key_attributes(&attributes);

	wrapping_key_created = status == PSA_SUCCESS;

	return status;
}

static psa_status_t generate_source_key(const void *context)
{
	const struct key_wrap_test_data *test = context;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	set_source_key_attributes(&attributes, test->algorithm);

	status = psa_generate_key(&attributes, &source_key_id);
	psa_reset_key_attributes(&attributes);

	source_key_created = status == PSA_SUCCESS;

	return status;
}

static psa_status_t wrap(const void *context)
{
	const struct key_wrap_test_data *test = context;

	return psa_wrap_key(wrapping_key_id, test->algorithm, source_key_id, wrapped_key,
			    sizeof(wrapped_key), &wrapped_length);
}

static psa_status_t unwrap(const void *context)
{
	const struct key_wrap_test_data *test = context;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	set_source_key_attributes(&attributes, test->algorithm);

	status = psa_unwrap_key(&attributes, wrapping_key_id, test->algorithm, wrapped_key,
				wrapped_length, &unwrapped_key_id);
	psa_reset_key_attributes(&attributes);
	if (status != PSA_SUCCESS) {
		return status;
	}

	unwrapped_key_created = true;

	return psa_export_key(unwrapped_key_id, unwrapped_key, sizeof(unwrapped_key),
			      &unwrapped_length);
}

int key_wrap_check(void)
{
	uint8_t original_key[KEY_SIZE];
	size_t original_length;

	if (wrapped_length < sizeof(original_key) || unwrapped_length != sizeof(original_key)) {
		return APP_ERROR;
	}

	/* Generated, so its bytes are only knowable here, while it is still alive. */
	if (psa_export_key(source_key_id, original_key, sizeof(original_key), &original_length) !=
	    PSA_SUCCESS) {
		return APP_ERROR;
	}

	if (original_length != unwrapped_length ||
	    memcmp(unwrapped_key, original_key, original_length) != 0) {
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

void key_wrap_cleanup(void)
{
	if (unwrapped_key_created) {
		(void)psa_destroy_key(unwrapped_key_id);
		unwrapped_key_created = false;
	}

	if (source_key_created) {
		(void)psa_destroy_key(source_key_id);
		source_key_created = false;
	}

	if (wrapping_key_created) {
		(void)psa_destroy_key(wrapping_key_id);
		wrapping_key_created = false;
	}
}

const struct op key_wrap_keysetup_ops[] = {
	{"generate_kek", generate_wrapping_key, NULL},
	{"generate_key", generate_source_key, NULL},
};
const struct op key_wrap_operations[] = {{"wrap", wrap, NULL}, {"unwrap", unwrap, NULL}};
