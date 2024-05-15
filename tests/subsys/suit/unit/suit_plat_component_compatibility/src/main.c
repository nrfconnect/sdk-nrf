/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_plat_component_compatibility.h>
#include <mocks.h>

static suit_manifest_class_id_t sample_class_id = {{0xca, 0xd8, 0x52, 0x3a, 0xf8, 0x29, 0x5a, 0x9a,
						    0xba, 0x85, 0x2e, 0xa0, 0xb2, 0xf5, 0x77,
						    0xc9}};
static int valid_component_number = 1;
static int valid_cpu_id = 2;
intptr_t valid_address = 0x80000;
size_t valid_size = 256;

static struct zcbor_string valid_component_id = {
	.value = (const uint8_t *)0x4321,
	.len = 321,
};

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

static suit_plat_err_t
suit_plat_decode_manifest_class_id_correct_fake_func(struct zcbor_string *component_id,
						     suit_manifest_class_id_t **class_id)
{
	zassert_equal(&valid_component_id, component_id, "Invalid manifest component ID value");
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	*class_id = &sample_class_id;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_manifest_class_id_invalid_fake_func(struct zcbor_string *component_id,
						     suit_manifest_class_id_t **class_id)
{
	zassert_equal(&valid_component_id, component_id, "Invalid manifest component ID value");
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	*class_id = NULL;

	return SUIT_PLAT_ERR_CRASH;
}

static int
suit_mci_manifest_class_id_validate_correct_fake_func(const suit_manifest_class_id_t *class_id)
{
	zassert_equal(class_id, &sample_class_id, "Invalid manifest class ID value");

	return SUIT_PLAT_SUCCESS;
}

static int suit_mci_manifest_parent_child_declaration_validate_correct_fake_func(
	const suit_manifest_class_id_t *parent_class_id,
	const suit_manifest_class_id_t *child_class_id)
{
	zassert_equal(parent_class_id, &sample_class_id, "Invalid parent manifest class ID value");
	zassert_equal(child_class_id, &sample_class_id, "Invalid child manifest class ID value");

	return SUIT_PLAT_SUCCESS;
}

static int suit_mci_manifest_parent_child_declaration_validate_invalid_fake_func(
	const suit_manifest_class_id_t *parent_class_id,
	const suit_manifest_class_id_t *child_class_id)
{
	zassert_equal(parent_class_id, &sample_class_id, "Invalid parent manifest class ID value");
	zassert_equal(child_class_id, &sample_class_id, "Invalid child manifest class ID value");

	return SUIT_ERR_UNAUTHORIZED_COMPONENT;
}

static int
suit_mci_manifest_class_id_validate_invalid_fake_func(const suit_manifest_class_id_t *class_id)
{
	zassert_equal(class_id, &sample_class_id, "Invalid manifest class ID value");

	return SUIT_PLAT_ERR_CRASH;
}

static suit_plat_err_t
suit_plat_decode_component_id_invalid_fake_func(struct zcbor_string *component_id, uint8_t *cpu_id,
						intptr_t *run_address, size_t *size)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(cpu_id, NULL,
			  "The API must provide a valid cpu_id pointer, to decode component ID");
	zassert_not_equal(
		run_address, NULL,
		"The API must provide a valid run_address pointer, to decode component ID");
	zassert_not_equal(size, NULL,
			  "The API must provide a valid size pointer, to decode component ID");

	return SUIT_PLAT_ERR_CRASH;
}

static suit_plat_err_t
suit_plat_decode_component_id_correct_fake_func(struct zcbor_string *component_id, uint8_t *cpu_id,
						intptr_t *run_address, size_t *size)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(cpu_id, NULL,
			  "The API must provide a valid cpu_id pointer, to decode component ID");
	zassert_not_equal(
		run_address, NULL,
		"The API must provide a valid run_address pointer, to decode component ID");
	zassert_not_equal(size, NULL,
			  "The API must provide a valid size pointer, to decode component ID");

	*cpu_id = valid_cpu_id;
	*run_address = valid_address;
	*size = valid_size;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_invalid_fake_func(struct zcbor_string *component_id,
						  suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");

	*type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	return SUIT_PLAT_ERR_CRASH;
}

static suit_plat_err_t
suit_plat_decode_component_type_mem_correct_fake_func(struct zcbor_string *component_id,
						      suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(type, NULL,
			  "The API must provide a valid type pointer, to decode component ID");

	*type = SUIT_COMPONENT_TYPE_MEM;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_special_correct_fake_func(struct zcbor_string *component_id,
							  suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(type, NULL,
			  "The API must provide a valid type pointer, to decode component ID");

	*type = SUIT_COMPONENT_TYPE_SOC_SPEC;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_unsupported_correct_fake_func(struct zcbor_string *component_id,
							      suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(type, NULL,
			  "The API must provide a valid type pointer, to decode component ID");

	*type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_cand_mfst_correct_fake_func(struct zcbor_string *component_id,
							    suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(type, NULL,
			  "The API must provide a valid type pointer, to decode component ID");

	*type = SUIT_COMPONENT_TYPE_CAND_MFST;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_instld_mfst_correct_fake_func(struct zcbor_string *component_id,
							      suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(type, NULL,
			  "The API must provide a valid type pointer, to decode component ID");

	*type = SUIT_COMPONENT_TYPE_INSTLD_MFST;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_number_invalid_fake_func(struct zcbor_string *component_id,
						    uint32_t *number)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(number, NULL,
			  "The API must provide a valid number pointer, to decode component ID");

	return SUIT_PLAT_ERR_CRASH;
}

static suit_plat_err_t
suit_plat_decode_component_number_correct_fake_func(struct zcbor_string *component_id,
						    uint32_t *number)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(number, NULL,
			  "The API must provide a valid number pointer, to decode component ID");

	*number = valid_component_number;

	return SUIT_PLAT_SUCCESS;
}

static int suit_mci_platform_specific_component_rights_validate_invalid_fake_func(
	const suit_manifest_class_id_t *class_id, int platform_specific_component_number)
{
	zassert_equal(&sample_class_id, class_id, "Invalid class ID value");
	zassert_equal(valid_component_number, platform_specific_component_number,
		      "Invalid component number value");

	return MCI_ERR_NOACCESS;
}

static int suit_mci_platform_specific_component_rights_validate_correct_fake_func(
	const suit_manifest_class_id_t *class_id, int platform_specific_component_number)
{
	zassert_equal(&sample_class_id, class_id, "Invalid class ID value");
	zassert_equal(valid_component_number, platform_specific_component_number,
		      "Invalid component number value");

	return SUIT_PLAT_SUCCESS;
}

static int
suit_mci_processor_start_rights_validate_invalid_fake_func(const suit_manifest_class_id_t *class_id,
							   int processor_id)
{
	zassert_equal(&sample_class_id, class_id, "Invalid class ID value");
	zassert_equal(valid_cpu_id, processor_id, "Invalid processor_id value");

	return MCI_ERR_NOACCESS;
}

int suit_mci_processor_start_rights_validate_correct_fake_func(
	const suit_manifest_class_id_t *class_id, int processor_id)
{
	zassert_equal(&sample_class_id, class_id, "Invalid class ID value");
	zassert_equal(valid_cpu_id, processor_id, "Invalid processor_id value");

	return SUIT_PLAT_SUCCESS;
}

static int
suit_mci_memory_access_rights_validate_invalid_fake_func(const suit_manifest_class_id_t *class_id,
							 void *address, size_t mem_size)
{
	zassert_equal(&sample_class_id, class_id, "Invalid class ID value");
	zassert_equal(valid_address, (intptr_t)address, "Invalid address value: 0x%X - 0x%X",
		      valid_address, address);
	zassert_equal(valid_size, mem_size, "Invalid processor_id value");

	return MCI_ERR_NOACCESS;
}

static int
suit_mci_memory_access_rights_validate_correct_fake_func(const suit_manifest_class_id_t *class_id,
							 void *address, size_t mem_size)
{
	zassert_equal(&sample_class_id, class_id, "Invalid class ID value");
	zassert_equal(valid_address, (intptr_t)address, "Invalid address value: 0x%X - 0x%X",
		      valid_address, address);
	zassert_equal(valid_size, mem_size, "Invalid processor_id value");

	return SUIT_PLAT_SUCCESS;
}

ZTEST_SUITE(suit_plat_component_compatibility_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_plat_component_compatibility_tests, test_null_manifest_class_id)
{
	int ret = suit_plat_component_compatibility_check(NULL, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_null_component_id)
{
	int ret = suit_plat_component_compatibility_check(&sample_class_id, NULL);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_invalid_component_id_null_value)
{
	struct zcbor_string invalid_component_id = {
		.value = NULL,
		.len = 8,
	};

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &invalid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_invalid_component_id_0_len)
{
	struct zcbor_string invalid_component_id = {
		.value = (const uint8_t *)0x1234,
		.len = 0,
	};

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &invalid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_invalid_validate_manifest_class_id)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_invalid_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_invalid_decode_component_type)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_invalid_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_mem_type_invalid_decode_component_id)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_mem_correct_fake_func;
	suit_plat_decode_component_id_fake.custom_fake =
		suit_plat_decode_component_id_invalid_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_mem_type_invalid_processor_start_rights)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_mem_correct_fake_func;
	suit_plat_decode_component_id_fake.custom_fake =
		suit_plat_decode_component_id_correct_fake_func;
	suit_mci_processor_start_rights_validate_fake.custom_fake =
		suit_mci_processor_start_rights_validate_invalid_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_mem_type_invalid_memory_access_rights)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_mem_correct_fake_func;
	suit_plat_decode_component_id_fake.custom_fake =
		suit_plat_decode_component_id_correct_fake_func;
	suit_mci_processor_start_rights_validate_fake.custom_fake =
		suit_mci_processor_start_rights_validate_correct_fake_func;
	suit_mci_memory_access_rights_validate_fake.custom_fake =
		suit_mci_memory_access_rights_validate_invalid_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_mem_type_OK)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_mem_correct_fake_func;
	suit_plat_decode_component_id_fake.custom_fake =
		suit_plat_decode_component_id_correct_fake_func;
	suit_mci_processor_start_rights_validate_fake.custom_fake =
		suit_mci_processor_start_rights_validate_correct_fake_func;
	suit_mci_memory_access_rights_validate_fake.custom_fake =
		suit_mci_memory_access_rights_validate_correct_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication succeeds */
	zassert_equal(SUIT_SUCCESS, ret, "Authorization should have succeeded");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_special_type_invalid_decode_component_number)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_special_correct_fake_func;
	suit_plat_decode_component_number_fake.custom_fake =
		suit_plat_decode_component_number_invalid_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests,
	test_special_type_invalid_validate_platform_specific_component_rights)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_special_correct_fake_func;
	suit_plat_decode_component_number_fake.custom_fake =
		suit_plat_decode_component_number_correct_fake_func;
	suit_mci_platform_specific_component_rights_validate_fake.custom_fake =
		suit_mci_platform_specific_component_rights_validate_invalid_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 1,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_special_type_OK)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_special_correct_fake_func;
	suit_plat_decode_component_number_fake.custom_fake =
		suit_plat_decode_component_number_correct_fake_func;
	suit_mci_platform_specific_component_rights_validate_fake.custom_fake =
		suit_mci_platform_specific_component_rights_validate_correct_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication succeeds */
	zassert_equal(SUIT_SUCCESS, ret, "Authorization should have succeeded");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 1,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_cand_mfst_type_invalid_decode_component_number)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_cand_mfst_correct_fake_func;
	suit_plat_decode_component_number_fake.custom_fake =
		suit_plat_decode_component_number_invalid_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_cand_mfst_type_OK)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_cand_mfst_correct_fake_func;
	suit_plat_decode_component_number_fake.custom_fake =
		suit_plat_decode_component_number_correct_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication succeeds */
	zassert_equal(SUIT_SUCCESS, ret, "Authorization should have succeeded");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}

ZTEST(suit_plat_component_compatibility_tests,
	test_instld_mfst_type_invalid_suit_plat_decode_manifest_class_id)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_mfst_correct_fake_func;
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_invalid_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(
		suit_mci_manifest_parent_child_declaration_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_manifest_parent_child_declaration_validate() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_instld_mfst_type_invalid_relationship)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_mfst_correct_fake_func;
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_mci_manifest_parent_child_declaration_validate_fake.custom_fake =
		suit_mci_manifest_parent_child_declaration_validate_invalid_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(
		suit_mci_manifest_parent_child_declaration_validate_fake.call_count, 1,
		"Incorrect number of suit_mci_manifest_parent_child_declaration_validate() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_instld_mfst_type_OK)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_mfst_correct_fake_func;
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_mci_manifest_parent_child_declaration_validate_fake.custom_fake =
		suit_mci_manifest_parent_child_declaration_validate_correct_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication succeeds */
	zassert_equal(SUIT_SUCCESS, ret, "Authorization should have succeeded");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(
		suit_mci_manifest_parent_child_declaration_validate_fake.call_count, 1,
		"Incorrect number of suit_mci_manifest_parent_child_declaration_validate() calls");
}

ZTEST(suit_plat_component_compatibility_tests, test_unsupported_type_err)
{
	suit_mci_manifest_class_id_validate_fake.custom_fake =
		suit_mci_manifest_class_id_validate_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_unsupported_correct_fake_func;

	int ret = suit_plat_component_compatibility_check(&sample_class_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_mci_processor_start_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_mci_memory_access_rights_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_number() calls");
	zassert_equal(
		suit_mci_platform_specific_component_rights_validate_fake.call_count, 0,
		"Incorrect number of suit_mci_platform_specific_component_rights_validate() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
}
