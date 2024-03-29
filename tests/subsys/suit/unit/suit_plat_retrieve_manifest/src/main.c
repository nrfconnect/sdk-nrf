/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_platform.h>
#include <mocks.h>

#define TEST_COMPONENT_HANDLE	   ((suit_component_t)0x123)
#define TEST_COMPONENT_ID	   ((struct zcbor_string *)0x456)
#define TEST_CLASS_ID		   ((suit_manifest_class_id_t *)0x789)
#define TEST_ENVELOPE_ADDRESS	   ((uint8_t *)0xabcd)
#define TEST_ENVELOPE_SIZE	   ((size_t)0xef01)
#define TEST_MEMPTR_STORAGE_HANDLE ((memptr_storage_handle_t)0x2345)

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(suit_platform_retrieve_manifest_tests, NULL, NULL, test_before, NULL, NULL);

static int
suit_plat_component_id_get_invalid_component_fake_func(suit_component_t handle,
						       struct zcbor_string **component_id)
{
	zassert_equal(handle, TEST_COMPONENT_HANDLE, "Unexpected component handle value");
	zassert_not_equal(component_id, NULL,
			  "API must provide valid address to get component ID value");

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

static int suit_plat_component_id_get_valid_component_fake_func(suit_component_t handle,
								struct zcbor_string **component_id)
{
	zassert_equal(handle, TEST_COMPONENT_HANDLE, "Unexpected component handle value");
	zassert_not_equal(component_id, NULL,
			  "API must provide valid address to get component ID value");

	*component_id = TEST_COMPONENT_ID;

	return SUIT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_unknown_fake_func(struct zcbor_string *component_id,
						  suit_component_type_t *type)
{
	zassert_equal(component_id, TEST_COMPONENT_ID, "Unexpected component ID value");
	zassert_not_equal(type, NULL, "API must provide valid address to get component type value");

	return SUIT_PLAT_ERR_CRASH;
}

static suit_plat_err_t
suit_plat_decode_component_type_unsupported_fake_func(struct zcbor_string *component_id,
						      suit_component_type_t *type)
{
	zassert_equal(component_id, TEST_COMPONENT_ID, "Unexpected component ID value");
	zassert_not_equal(type, NULL, "API must provide valid address to get component type value");

	*type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_mem_fake_func(struct zcbor_string *component_id,
					      suit_component_type_t *type)
{
	zassert_equal(component_id, TEST_COMPONENT_ID, "Unexpected component ID value");
	zassert_not_equal(type, NULL, "API must provide valid address to get component type value");

	*type = SUIT_COMPONENT_TYPE_MEM;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_cand_img_fake_func(struct zcbor_string *component_id,
						   suit_component_type_t *type)
{
	zassert_equal(component_id, TEST_COMPONENT_ID, "Unexpected component ID value");
	zassert_not_equal(type, NULL, "API must provide valid address to get component type value");

	*type = SUIT_COMPONENT_TYPE_CAND_IMG;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_instld_fake_func(struct zcbor_string *component_id,
						 suit_component_type_t *type)
{
	zassert_equal(component_id, TEST_COMPONENT_ID, "Unexpected component ID value");
	zassert_not_equal(type, NULL, "API must provide valid address to get component type value");

	*type = SUIT_COMPONENT_TYPE_INSTLD_MFST;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_cand_fake_func(struct zcbor_string *component_id,
					       suit_component_type_t *type)
{
	zassert_equal(component_id, TEST_COMPONENT_ID, "Unexpected component ID value");
	zassert_not_equal(type, NULL, "API must provide valid address to get component type value");

	*type = SUIT_COMPONENT_TYPE_CAND_MFST;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_manifest_class_id_invalid_fake_func(struct zcbor_string *component_id,
						     suit_manifest_class_id_t **class_id)
{
	zassert_equal(component_id, TEST_COMPONENT_ID, "Unexpected component ID value");
	zassert_not_equal(class_id, NULL,
			  "API must provide valid address to get manifest class ID value");

	return SUIT_PLAT_ERR_CRASH;
}

static suit_plat_err_t
suit_plat_decode_manifest_class_id_valid_fake_func(struct zcbor_string *component_id,
						   suit_manifest_class_id_t **class_id)
{
	zassert_equal(component_id, TEST_COMPONENT_ID, "Unexpected component ID value");
	zassert_not_equal(class_id, NULL,
			  "API must provide valid address to get manifest class ID value");

	*class_id = TEST_CLASS_ID;

	return SUIT_PLAT_SUCCESS;
}

static int
suit_storage_installed_envelope_get_not_found_fake_func(const suit_manifest_class_id_t *id,
							uint8_t **addr, size_t *size)
{
	zassert_equal(id, TEST_CLASS_ID, "Unexpected manifest class ID value");
	zassert_not_equal(addr, NULL,
			  "API must provide valid address to get installed manifest address value");
	zassert_not_equal(size, NULL,
			  "API must provide valid address to get installed manifest size value");

	return SUIT_PLAT_ERR_NOT_FOUND;
}

static int
suit_storage_installed_envelope_get_invalid_address_fake_func(const suit_manifest_class_id_t *id,
							      uint8_t **addr, size_t *size)
{
	zassert_equal(id, TEST_CLASS_ID, "Unexpected manifest class ID value");
	zassert_not_equal(addr, NULL,
			  "API must provide valid address to get installed manifest address value");
	zassert_not_equal(size, NULL,
			  "API must provide valid address to get installed manifest size value");

	*addr = NULL;
	*size = TEST_ENVELOPE_SIZE;

	return SUIT_SUCCESS;
}

static int
suit_storage_installed_envelope_get_invalid_size_fake_func(const suit_manifest_class_id_t *id,
							   uint8_t **addr, size_t *size)
{
	zassert_equal(id, TEST_CLASS_ID, "Unexpected manifest class ID value");
	zassert_not_equal(addr, NULL,
			  "API must provide valid address to get installed manifest address value");
	zassert_not_equal(size, NULL,
			  "API must provide valid address to get installed manifest size value");

	*addr = TEST_ENVELOPE_ADDRESS;
	*size = 0;

	return SUIT_SUCCESS;
}

static int suit_storage_installed_envelope_get_found_fake_func(const suit_manifest_class_id_t *id,
							       uint8_t **addr, size_t *size)
{
	zassert_equal(id, TEST_CLASS_ID, "Unexpected manifest class ID value");
	zassert_not_equal(addr, NULL,
			  "API must provide valid address to get installed manifest address value");
	zassert_not_equal(size, NULL,
			  "API must provide valid address to get installed manifest size value");

	*addr = TEST_ENVELOPE_ADDRESS;
	*size = TEST_ENVELOPE_SIZE;

	return SUIT_PLAT_SUCCESS;
}

static int suit_plat_component_impl_data_get_no_data_fake_func(suit_component_t handle,
							       void **impl_data)
{
	zassert_equal(handle, TEST_COMPONENT_HANDLE, "Unexpected component handle value");
	zassert_not_equal(impl_data, NULL,
			  "API must provide valid address to get imeplementation-specific data");

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

static int suit_plat_component_impl_data_get_data_fake_func(suit_component_t handle,
							    void **impl_data)
{
	zassert_equal(handle, TEST_COMPONENT_HANDLE, "Unexpected component handle value");
	zassert_not_equal(impl_data, NULL,
			  "API must provide valid address to get imeplementation-specific data");

	*impl_data = TEST_MEMPTR_STORAGE_HANDLE;

	return SUIT_SUCCESS;
}

static int suit_memptr_storage_ptr_get_no_data_fake_func(memptr_storage_handle_t handle,
							 const uint8_t **payload_ptr,
							 size_t *payload_size)
{
	zassert_equal(handle, TEST_MEMPTR_STORAGE_HANDLE, "Unexpected memory storage handle value");
	zassert_not_equal(payload_ptr, NULL,
			  "API must provide valid address to get installed manifest address value");
	zassert_not_equal(payload_size, NULL,
			  "API must provide valid address to get installed manifest size value");

	return SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD;
}

static int suit_memptr_storage_ptr_get_invalid_addr_fake_func(memptr_storage_handle_t handle,
							      const uint8_t **payload_ptr,
							      size_t *payload_size)
{
	zassert_equal(handle, TEST_MEMPTR_STORAGE_HANDLE, "Unexpected memory storage handle value");
	zassert_not_equal(payload_ptr, NULL,
			  "API must provide valid address to get installed manifest address value");
	zassert_not_equal(payload_size, NULL,
			  "API must provide valid address to get installed manifest size value");

	*payload_ptr = NULL;
	*payload_size = TEST_ENVELOPE_SIZE;

	return SUIT_PLAT_SUCCESS;
}

static int suit_memptr_storage_ptr_get_invalid_size_fake_func(memptr_storage_handle_t handle,
							      const uint8_t **payload_ptr,
							      size_t *payload_size)
{
	zassert_equal(handle, TEST_MEMPTR_STORAGE_HANDLE, "Unexpected memory storage handle value");
	zassert_not_equal(payload_ptr, NULL,
			  "API must provide valid address to get installed manifest address value");
	zassert_not_equal(payload_size, NULL,
			  "API must provide valid address to get installed manifest size value");

	*payload_ptr = TEST_ENVELOPE_ADDRESS;
	*payload_size = 0;

	return SUIT_PLAT_SUCCESS;
}

static int suit_memptr_storage_ptr_get_valid_fake_func(memptr_storage_handle_t handle,
						       const uint8_t **payload_ptr,
						       size_t *payload_size)
{
	zassert_equal(handle, TEST_MEMPTR_STORAGE_HANDLE, "Unexpected memory storage handle value");
	zassert_not_equal(payload_ptr, NULL,
			  "API must provide valid address to get installed manifest address value");
	zassert_not_equal(payload_size, NULL,
			  "API must provide valid address to get installed manifest size value");

	*payload_ptr = TEST_ENVELOPE_ADDRESS;
	*payload_size = TEST_ENVELOPE_SIZE;

	return SUIT_PLAT_SUCCESS;
}

ZTEST(suit_platform_retrieve_manifest_tests, test_handle_null_args)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* WHEN platform is asked to return manifest and input parameters are invalid */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, NULL, NULL);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_CRASH, ret, "Invalid manifest retrieved");

	/* WHEN platform is asked to return manifest and input parameters are invalid */
	ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, NULL, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_CRASH, ret, "Invalid manifest retrieved");

	/* WHEN platform is asked to return manifest and input parameters are invalid */
	ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, NULL);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_CRASH, ret, "Invalid manifest retrieved");

	/* ... and other checks were not performed in any of those scenarios */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_handle_invalid)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is invalid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_invalid_component_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_component_unknown)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is unknown (not decoded) */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_unknown_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_component_unsupported)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is unsupported (but decoded) */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_unsupported_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_component_mem)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is memory-mapped component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_mem_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_component_cand_img)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is candidate image component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_cand_img_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_instld_invalid_class)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is installed manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_fake_func;
	/* ... and the manifest class ID for given component is invalid */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_invalid_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and manifest class ID for given component ID was fetched */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_instld_manifest_not_found)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is installed manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_fake_func;
	/* ... and the manifest class ID for given component is valid */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_valid_fake_func;
	/* ... and the manifest with given class ID is not found in SUIT storage */
	suit_storage_installed_envelope_get_fake.custom_fake =
		suit_storage_installed_envelope_get_not_found_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and manifest class ID for given component ID was fetched */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest for given class ID was fetched */
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 1,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_instld_manifest_invalid_address)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is installed manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_fake_func;
	/* ... and the manifest class ID for given component is valid */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_valid_fake_func;
	/* ... and the manifest with given class ID is not found in SUIT storage */
	suit_storage_installed_envelope_get_fake.custom_fake =
		suit_storage_installed_envelope_get_invalid_address_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and manifest class ID for given component ID was fetched */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest for given class ID was fetched */
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 1,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_instld_manifest_invalid_size)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is installed manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_fake_func;
	/* ... and the manifest class ID for given component is valid */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_valid_fake_func;
	/* ... and the manifest with given class ID is not found in SUIT storage */
	suit_storage_installed_envelope_get_fake.custom_fake =
		suit_storage_installed_envelope_get_invalid_size_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and manifest class ID for given component ID was fetched */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest for given class ID was fetched */
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 1,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_instld_manifest_found)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is installed manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_fake_func;
	/* ... and the manifest class ID for given component is valid */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_valid_fake_func;
	/* ... and the manifest with given class ID is not found in SUIT storage */
	suit_storage_installed_envelope_get_fake.custom_fake =
		suit_storage_installed_envelope_get_found_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_SUCCESS, ret, "Invalid manifest retrieved");
	zassert_equal(TEST_ENVELOPE_ADDRESS, envelope_str, "Invalid envelope address received");
	zassert_equal(TEST_ENVELOPE_SIZE, envelope_len, "Invalid envelope size received");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and manifest class ID for given component ID was fetched */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest for given class ID was fetched */
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 1,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_cand_manifest_no_data)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is candidate manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_cand_fake_func;
	/* ... and there is no implementation-specific data */
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_no_data_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and implementation data for given component handle was fetched */
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_cand_manifest_no_ptr)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is candidate manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_cand_fake_func;
	/* ... and there is implementation-specific data */
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_data_fake_func;
	/* ... and there is no data in memory pointer storage */
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_no_data_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and implementation data for given component handle was fetched */
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	/* ... and envelope was fetched from the memory pointer storage */
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_cand_manifest_invalid_addr)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is candidate manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_cand_fake_func;
	/* ... and there is implementation-specific data */
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_data_fake_func;
	/* ... and there is data in memory pointer storage */
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_invalid_addr_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and implementation data for given component handle was fetched */
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	/* ... and envelope was fetched from the memory pointer storage */
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_cand_manifest_invalid_size)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is candidate manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_cand_fake_func;
	/* ... and there is implementation-specific data */
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_data_fake_func;
	/* ... and there is data in memory pointer storage */
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_invalid_size_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, &envelope_str, &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and implementation data for given component handle was fetched */
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	/* ... and envelope was fetched from the memory pointer storage */
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_cand_manifest_valid)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is candidate manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_cand_fake_func;
	/* ... and there is implementation-specific data */
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_data_fake_func;
	/* ... and there is data in memory pointer storage */
	suit_memptr_storage_ptr_get_fake.custom_fake = suit_memptr_storage_ptr_get_valid_fake_func;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, (const uint8_t *)&envelope_str,
					      &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_SUCCESS, ret, "Invalid manifest retrieved");
	zassert_equal(TEST_ENVELOPE_ADDRESS, envelope_str, "Invalid envelope address received");
	zassert_equal(TEST_ENVELOPE_SIZE, envelope_len, "Invalid envelope size received");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and implementation data for given component handle was fetched */
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	/* ... and envelope was fetched from the memory pointer storage */
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
	zassert_equal(suit_memory_global_address_is_in_external_memory_fake.call_count, 1,
		      "Incorrect number of suit_memory_global_address_is_in_external_memory() "
		      "calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
}

ZTEST(suit_platform_retrieve_manifest_tests, test_cand_manifest_in_external_memory)
{
	uint8_t *envelope_str;
	size_t envelope_len;

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is candidate manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_cand_fake_func;
	/* ... and there is implementation-specific data */
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_data_fake_func;
	/* ... and there is data in memory pointer storage */
	suit_memptr_storage_ptr_get_fake.custom_fake = suit_memptr_storage_ptr_get_valid_fake_func;

	/* ... and the data pointer points to an area in external memory */
	suit_memory_global_address_is_in_external_memory_fake.return_val = true;

	/* WHEN platform is asked to return manifest */
	int ret = suit_plat_retrieve_manifest(TEST_COMPONENT_HANDLE, (const uint8_t *)&envelope_str,
					      &envelope_len);

	/* THEN manifest is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and implementation data for given component handle was fetched */
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	/* ... and envelope was fetched from the memory pointer storage */
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
	zassert_equal(suit_memory_global_address_is_in_external_memory_fake.call_count, 1,
		      "Incorrect number of suit_memory_global_address_is_in_external_memory() "
		      "calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
}
