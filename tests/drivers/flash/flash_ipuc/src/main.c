/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <mock_suit_ipuc.h>
#include <drivers/flash/flash_ipuc.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_mem_util.h>

#define CONFIG_SUIT_IPUC_SIZE 5

struct flash_ipuc_tests_fixture {
	size_t exp_component_idx;
	struct zcbor_string *exp_encryption_info;
	struct zcbor_string *exp_compression_info;
	size_t count;
	struct zcbor_string component_ids[CONFIG_SUIT_IPUC_SIZE];
	suit_manifest_role_t roles[CONFIG_SUIT_IPUC_SIZE];
};

static struct flash_ipuc_tests_fixture test_fixture = {0};

/* clang-format off */
static const uint8_t component_invalid[] = {
	'I', 'n', 'v', 'a', 'l', 'i', 'd', ' ',
	'c', 'o', 'm', 'p', 'o', 'n', 'e', 'n', 't', ' ',
	'I', 'D', '\n',
};
#ifndef CONFIG_SOC_SERIES_NRF54HX
static const uint8_t component_radio_0x54000_0x1000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x03, /* Radio core - executable */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x05, 0x40, 0x00,
		0x43, /* bstr(3) */
			0x19, /* uint16 */
				0x10, 0x00,
};
static const uint8_t component_radio_0x55000_0x3f000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x20, /* Radio core - data */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x05, 0x50, 0x00,
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x03, 0xf0, 0x00,
};
static const uint8_t component_app_0x94000_0x6d000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x02, /* Application core - executable */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x09, 0x40, 0x00,
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x06, 0xd0, 0x00,
};
static const uint8_t component_app_0x101000_0x60000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x20, /* Application core - data */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x10, 0x10, 0x00,
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x06, 0x00, 0x00,
};
#else /* CONFIG_SOC_SERIES_NRF54HX */
static const uint8_t component_radio_0x54000_0x1000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x03, /* Radio core - executable */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x0e, 0x05, 0x40, 0x00,
		0x43, /* bstr(3) */
			0x19, /* uint16 */
				0x10, 0x00,
};
static const uint8_t component_radio_0x55000_0x3f000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x20, /* Radio core - data */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x0e, 0x05, 0x50, 0x00,
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x03, 0xf0, 0x00,
};
static const uint8_t component_app_0x94000_0x6d000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x02, /* Application core - executable */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x0e, 0x09, 0x40, 0x00,
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x06, 0xd0, 0x00,
};
static const uint8_t component_app_0x101000_0x60000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x20, /* Application core - data */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x0e, 0x10, 0x10, 0x00,
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x06, 0x00, 0x00,
};
#endif /* CONFIG_SOC_SERIES_NRF54HX */
/* clang-format on */

static void setup_sample_ipucs(struct flash_ipuc_tests_fixture *f)
{
	/* Existence of an invalid component ID should not affect the usage of the IPUC APIs. */
	f->component_ids[0].value = component_invalid;
	f->component_ids[0].len = sizeof(component_invalid);
	f->roles[0] = SUIT_MANIFEST_APP_ROOT;

	f->component_ids[1].value = component_radio_0x54000_0x1000;
	f->component_ids[1].len = sizeof(component_radio_0x54000_0x1000);
	f->roles[1] = SUIT_MANIFEST_RAD_LOCAL_1;

	f->component_ids[2].value = component_radio_0x55000_0x3f000;
	f->component_ids[2].len = sizeof(component_radio_0x55000_0x3f000);
	f->roles[2] = SUIT_MANIFEST_RAD_LOCAL_2;

	f->component_ids[3].value = component_app_0x94000_0x6d000;
	f->component_ids[3].len = sizeof(component_app_0x94000_0x6d000);
	f->roles[3] = SUIT_MANIFEST_APP_LOCAL_1;

	f->component_ids[4].value = component_app_0x101000_0x60000;
	f->component_ids[4].len = sizeof(component_app_0x101000_0x60000);
	f->roles[4] = SUIT_MANIFEST_APP_LOCAL_2;

	f->count = 5;
}

static suit_plat_err_t suit_ipuc_get_count_fake_func(size_t *count)
{
	zassert_not_null(count, "Caller must provide non-Null pointer");

	*count = test_fixture.count;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t suit_ipuc_get_info_fake_func(size_t idx, struct zcbor_string *component_id,
						    suit_manifest_role_t *role)
{
	zassert_not_null(component_id, "Caller must provide non-Null pointer to read component_id");
	zassert_not_null(role, "Caller must provide non-Null pointer to read role");
	zassert_true(idx < test_fixture.count, "The idx value is larger than declared IPUC list");

	*component_id = test_fixture.component_ids[idx];
	*role = test_fixture.roles[idx];

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t suit_ipuc_write_setup_fake_func(struct zcbor_string *component_id,
						       struct zcbor_string *encryption_info,
						       struct zcbor_string *compression_info)
{
	zassert_not_null(component_id, "Caller must provide non-Null pointer to read component_id");
	zassert_equal(encryption_info, test_fixture.exp_encryption_info,
		      "Unexpected encryption info structure passed");
	zassert_equal(compression_info, test_fixture.exp_compression_info,
		      "Unexpected compression info structure passed");
	zassert_true(test_fixture.exp_component_idx < test_fixture.count,
		     "Invalid test: expected idx value is larger than declared IPUC list");

	struct zcbor_string *exp_component_id =
		&test_fixture.component_ids[test_fixture.exp_component_idx];
	zassert_equal(component_id->len, exp_component_id->len,
		      "Invalid component ID length passed");
	zassert_equal(memcmp(component_id->value, exp_component_id->value, exp_component_id->len),
		      0, "Invalid component ID value passed");

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t suit_ipuc_write_fake_func(struct zcbor_string *component_id, size_t offset,
						 uintptr_t buffer, size_t chunk_size,
						 bool last_chunk)
{
	zassert_not_null(component_id, "Caller must provide non-Null pointer to read component_id");
	zassert_true(test_fixture.exp_component_idx < test_fixture.count,
		     "Invalid test: expected idx value is larger than declared IPUC list");

	struct zcbor_string *exp_component_id =
		&test_fixture.component_ids[test_fixture.exp_component_idx];
	zassert_equal(component_id->len, exp_component_id->len,
		      "Invalid component ID length passed");
	zassert_equal(memcmp(component_id->value, exp_component_id->value, exp_component_id->len),
		      0, "Invalid component ID value passed");

	if (chunk_size == 4) {
		uint8_t exp_data[] = {0x1, 0x2, 0x3, 0x4};

		zassert_equal(memcmp((uint8_t *)buffer, exp_data, sizeof(exp_data)), 0,
			      "Unexpected data passed through IPUC write API");
	} else if (chunk_size == 1) {
		uint8_t data = *((uint8_t *)buffer);

		if ((data != 0xAB) && (data != 0xFF)) {
			zassert_true(false, "Unexpected data passed through IPUC write API");
		}
	} else if (chunk_size == 0) {
		zassert_is_null((uint8_t *)buffer, "Unexpected data passed through IPUC write API");
	} else {
		uint8_t *data = (uint8_t *)buffer;

		for (size_t i = 0; i < chunk_size; i++) {
			zassert_equal(
				data[i], 0xFF,
				"Unexpected data length passed though IPUC write API (%d, %d)",
				chunk_size, i);
		}
	}

	return SUIT_PLAT_SUCCESS;
}

static void *test_suite_setup(void)
{
	return &test_fixture;
}

static void test_before(void *f)
{
	struct flash_ipuc_tests_fixture *fixture = (struct flash_ipuc_tests_fixture *)f;

	/* Reset mocks */
	mock_suit_ipuc_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();

	/* Reset test fixture */
	memset(fixture, 0, sizeof(*fixture));
}

ZTEST_SUITE(flash_ipuc_tests, NULL, test_suite_setup, test_before, NULL, NULL);

ZTEST_F(flash_ipuc_tests, test_null_arg)
{
	uintptr_t mem_address = 0x1000;
	size_t mem_size = 0x1000;
	uintptr_t ipuc_address = 0x1000;
	size_t ipuc_size = 0x1000;
	uintptr_t min_address = 0;

	/* WHEN one of the required arguments is equal to NULL
	 */

	/* THEN all IPUC availability check functions return false... */
	zassert_equal(flash_component_ipuc_check(NULL), false,
		      "IPUC component check for NULL component ID shall fail");
	zassert_equal(flash_cache_ipuc_check(min_address, NULL, &ipuc_size), false,
		      "IPUC cache check with NULL output address pointer shall fail");
	zassert_equal(flash_cache_ipuc_check(min_address, &ipuc_address, NULL), false,
		      "IPUC cache check with NULL output size pointer shall fail");

	/* ... and all IPUC creation fails... */
	zassert_equal(flash_component_ipuc_create(NULL, NULL, NULL), NULL,
		      "IPUC component created for NULL component ID");
	zassert_equal(flash_cache_ipuc_create(min_address, NULL, &ipuc_size), NULL,
		      "IPUC cache created with NULL output address pointer");
	zassert_equal(flash_cache_ipuc_create(min_address, &ipuc_address, NULL), NULL,
		      "IPUC cache created with NULL output size pointer");

	/* ... and no IPUC is found... */
	zassert_equal(flash_ipuc_find(mem_address, mem_size, NULL, &ipuc_size), NULL,
		      "IPUC cache found with NULL output address pointer");
	zassert_equal(flash_ipuc_find(mem_address, mem_size, &ipuc_address, NULL), NULL,
		      "IPUC cache found with NULL output size pointer");

	/* ... and no IPUC SSF APIs are called... */
	zassert_equal(suit_ipuc_get_count_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_get_count() calls");
	zassert_equal(suit_ipuc_get_info_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_get_info() calls");
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");
	zassert_equal(suit_ipuc_write_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write() calls");
}

ZTEST_F(flash_ipuc_tests, test_component_check_OK)
{
	/* clang-format off */
#ifndef CONFIG_SOC_SERIES_NRF54HX
	const uint8_t declared_component_id_cbor[] = {
		0x84, /* array(4) */
			0x44, /* bstr(4) */
				0x63, /* text(3) */
					'M', 'E', 'M',
			0x41, /* bstr(1) */
				0x03, /* Radio core - executable */
			0x45, /* bstr(5) */
				0x1a, /* uint32 */
					0x00, 0x05, 0x40, 0x00,
			0x43, /* bstr(3) */
				0x19, /* uint16 */
					0x10, 0x00,
	};
#else /* CONFIG_SOC_SERIES_NRF54HX */
	const uint8_t declared_component_id_cbor[] = {
		0x84, /* array(4) */
			0x44, /* bstr(4) */
				0x63, /* text(3) */
					'M', 'E', 'M',
			0x41, /* bstr(1) */
				0x03, /* Radio core - executable */
			0x45, /* bstr(5) */
				0x1a, /* uint32 */
					0x0e, 0x05, 0x40, 0x00,
			0x43, /* bstr(3) */
				0x19, /* uint16 */
					0x10, 0x00,
	};
#endif /* CONFIG_SOC_SERIES_NRF54HX */
	/* clang-format on */
	struct zcbor_string test_component_id = {
		.value = component_radio_0x54000_0x1000,
		.len = sizeof(component_radio_0x54000_0x1000),
	};

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;

	/* GIVEN a single radio manifest component IPUC is available */
	fixture->count = 1;
	fixture->component_ids[0].value = declared_component_id_cbor;
	fixture->component_ids[0].len = sizeof(declared_component_id_cbor);
	fixture->roles[0] = SUIT_MANIFEST_RAD_LOCAL_1;

	/* WHEN the check for matching address is called */
	zassert_equal(flash_component_ipuc_check(&test_component_id), true,
		      "IPUC component check for component ID failed");

	/* THEN the API returns true */
	/* ... and the IPUC was checked for component count */
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls");
	/* ... and the IPUC with the first index was checked */
	zassert_equal(suit_ipuc_get_info_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_info() calls");
	zassert_equal(suit_ipuc_get_info_fake.arg0_val, 0,
		      "Incorrect IPUC index value in suit_ipuc_get_info() call");

	/* ... and the IPUC was not set up */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");
	/* ... and the IPUC content was not modified */
	zassert_equal(suit_ipuc_write_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write() calls");
}

ZTEST_F(flash_ipuc_tests, test_component_check_NOK_different_id)
{
	/* clang-format off */
#ifndef CONFIG_SOC_SERIES_NRF54HX
	const uint8_t declared_component_id_cbor[] = {
		0x84, /* array(4) */
			0x44, /* bstr(4) */
				0x63, /* text(3) */
					'M', 'E', 'M',
			0x41, /* bstr(1) */
				0x03, /* Radio core - executable */
			0x45, /* bstr(5) */
				0x1a, /* uint32 */
					0x00, 0x05, 0x40, 0x00,
			0x43, /* bstr(3) */
				0x19, /* uint16 */
					0x10, 0x10,
	};
#else /* CONFIG_SOC_SERIES_NRF54HX */
	const uint8_t declared_component_id_cbor[] = {
		0x84, /* array(4) */
			0x44, /* bstr(4) */
				0x63, /* text(3) */
					'M', 'E', 'M',
			0x41, /* bstr(1) */
				0x03, /* Radio core - executable */
			0x45, /* bstr(5) */
				0x1a, /* uint32 */
					0x0e, 0x05, 0x40, 0x00,
			0x43, /* bstr(3) */
				0x19, /* uint16 */
					0x10, 0x10,
	};
#endif /* CONFIG_SOC_SERIES_NRF54HX */
	/* clang-format on */
	struct zcbor_string test_component_id = {
		.value = component_radio_0x54000_0x1000,
		.len = sizeof(component_radio_0x54000_0x1000),
	};

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;

	/* GIVEN a single radio manifest component IPUC is available */
	fixture->count = 1;
	fixture->component_ids[0].value = declared_component_id_cbor;
	fixture->component_ids[0].len = sizeof(declared_component_id_cbor);
	fixture->roles[0] = SUIT_MANIFEST_RAD_LOCAL_1;

	/* WHEN the check for different address is called */
	zassert_equal(flash_component_ipuc_check(&test_component_id), false,
		      "IPUC component check for component ID did not fail");

	/* THEN the API returns false */
	/* ... and the IPUC was checked for component count */
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls");
	/* ... and the IPUC with the first index was checked */
	zassert_equal(suit_ipuc_get_info_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_info() calls");
	zassert_equal(suit_ipuc_get_info_fake.arg0_val, 0,
		      "Incorrect IPUC index value in suit_ipuc_get_info() call");

	/* ... and the IPUC was not set up */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");
	/* ... and the IPUC content was not modified */
	zassert_equal(suit_ipuc_write_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write() calls");
}

ZTEST_F(flash_ipuc_tests, test_component_create_OK)
{
	/* clang-format off */
#ifndef CONFIG_SOC_SERIES_NRF54HX
	const uint8_t declared_component_id_cbor[] = {
		0x84, /* array(4) */
			0x44, /* bstr(4) */
				0x63, /* text(3) */
					'M', 'E', 'M',
			0x41, /* bstr(1) */
				0x03, /* Radio core - executable */
			0x45, /* bstr(5) */
				0x1a, /* uint32 */
					0x00, 0x05, 0x40, 0x00,
			0x43, /* bstr(3) */
				0x19, /* uint16 */
					0x10, 0x00,
	};
#else /* CONFIG_SOC_SERIES_NRF54HX */
	const uint8_t declared_component_id_cbor[] = {
		0x84, /* array(4) */
			0x44, /* bstr(4) */
				0x63, /* text(3) */
					'M', 'E', 'M',
			0x41, /* bstr(1) */
				0x03, /* Radio core - executable */
			0x45, /* bstr(5) */
				0x1a, /* uint32 */
					0x0e, 0x05, 0x40, 0x00,
			0x43, /* bstr(3) */
				0x19, /* uint16 */
					0x10, 0x00,
	};
#endif /* CONFIG_SOC_SERIES_NRF54HX */
	/* clang-format on */
	struct zcbor_string test_component_id = {
		.value = component_radio_0x54000_0x1000,
		.len = sizeof(component_radio_0x54000_0x1000),
	};
	struct device *flash_ipuc = NULL;
	uint8_t exp_data[] = {0xAB};

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;
	suit_ipuc_write_setup_fake.custom_fake = suit_ipuc_write_setup_fake_func;
	suit_ipuc_write_fake.custom_fake = suit_ipuc_write_fake_func;

	/* GIVEN a single radio manifest component IPUC is available */
	fixture->count = 1;
	fixture->component_ids[0].value = declared_component_id_cbor;
	fixture->component_ids[0].len = sizeof(declared_component_id_cbor);
	fixture->roles[0] = SUIT_MANIFEST_RAD_LOCAL_1;
	/* ... and the test expects the first plaintext, uncompressed IPUC to be configured */
	fixture->exp_component_idx = 0;
	fixture->exp_encryption_info = NULL;
	fixture->exp_compression_info = NULL;

	/* WHEN the check for matching address is called */
	flash_ipuc = flash_component_ipuc_create(&test_component_id, NULL, NULL);

	/* THEN the API returns a valid IPUC driver */
	zassert_not_null(flash_ipuc, "IPUC component create for component ID failed");
	/* ... and the IPUC was checked for component count */
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls");
	/* ... and the IPUC with the first index was checked */
	zassert_equal(suit_ipuc_get_info_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_info() calls");
	zassert_equal(suit_ipuc_get_info_fake.arg0_val, 0,
		      "Incorrect IPUC index value in suit_ipuc_get_info() call");
	/* ... and the IPUC was not set up for writing */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	/* ... and the IPUC was not initialized over SSF */
	zassert_equal(flash_ipuc_setup_pending(flash_ipuc), true,
		      "Cache IPUC initialized immediately");
	/* ... and the IPUC content was not modified */
	zassert_equal(suit_ipuc_write_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write() calls");

	/* WHEN erase API is called for any byte */
	zassert_equal(flash_erase(flash_ipuc, 1, 1), 0, "Failed to erase cache IPUC");

	/* THEN the IPUC was not set up for writing */
	zassert_equal(flash_ipuc_setup_pending(flash_ipuc), true,
		      "Cache IPUC initialized while erasing uninitialized instance");
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	/* ... and the IPUC content was not modified */
	zassert_equal(suit_ipuc_write_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write() calls");

	/* WHEN write API is called for any byte */
	zassert_equal(flash_write(flash_ipuc, 1, exp_data, sizeof(exp_data)), 0,
		      "Failed to erase cache IPUC");

	/* THEN the IPUC was set up for writing */
	zassert_equal(flash_ipuc_setup_pending(flash_ipuc), false,
		      "Cache IPUC not initialized while erasing contents");
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	/* ... and the IPUC content was modified */
	zassert_equal(suit_ipuc_write_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_write() calls");
	zassert_equal(suit_ipuc_write_fake.arg1_val, 1, "Incorrect offset passed through IPUC API");
	zassert_equal(suit_ipuc_write_fake.arg3_val, sizeof(exp_data),
		      "Incorrect data length passed through IPUC API");
	zassert_equal(suit_ipuc_write_fake.arg4_val, false,
		      "Incorrect last_chunk flag value passed through IPUC API");

	/* Release the IPUC */
	flash_ipuc_release(flash_ipuc);
}

ZTEST_F(flash_ipuc_tests, test_component_create_NOK_different_id)
{
	/* clang-format off */
#ifndef CONFIG_SOC_SERIES_NRF54HX
	const uint8_t declared_component_id_cbor[] = {
		0x84, /* array(4) */
			0x44, /* bstr(4) */
				0x63, /* text(3) */
					'M', 'E', 'M',
			0x41, /* bstr(1) */
				0x03, /* Radio core - executable */
			0x45, /* bstr(5) */
				0x1a, /* uint32 */
					0x00, 0x05, 0x40, 0x00,
			0x45, /* bstr(5) */
				0x1a, /* uint32 */
					0x00, 0x05, 0x40, 0x00,
	};
#else /* CONFIG_SOC_SERIES_NRF54HX */
	const uint8_t declared_component_id_cbor[] = {
		0x84, /* array(4) */
			0x44, /* bstr(4) */
				0x63, /* text(3) */
					'M', 'E', 'M',
			0x41, /* bstr(1) */
				0x03, /* Radio core - executable */
			0x45, /* bstr(5) */
				0x1a, /* uint32 */
					0x0e, 0x05, 0x40, 0x00,
			0x45, /* bstr(5) */
				0x1a, /* uint32 */
					0x00, 0x05, 0x40, 0x00,
	};
#endif /* CONFIG_SOC_SERIES_NRF54HX */
	/* clang-format on */
	struct zcbor_string test_component_id = {
		.value = component_radio_0x54000_0x1000,
		.len = sizeof(component_radio_0x54000_0x1000),
	};
	struct device *flash_ipuc = NULL;

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;

	/* GIVEN a single radio manifest component IPUC is available */
	fixture->count = 1;
	fixture->component_ids[0].value = declared_component_id_cbor;
	fixture->component_ids[0].len = sizeof(declared_component_id_cbor);
	fixture->roles[0] = SUIT_MANIFEST_RAD_LOCAL_1;

	/* WHEN the check for different address is called */
	flash_ipuc = flash_component_ipuc_create(&test_component_id, NULL, NULL);

	/* THEN the API returns NULL */
	zassert_is_null(flash_ipuc, "IPUC component create for component ID failed");
	/* ... and the IPUC was checked for component count */
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls");
	/* ... and the IPUC with the first index was checked */
	zassert_equal(suit_ipuc_get_info_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_info() calls");
	zassert_equal(suit_ipuc_get_info_fake.arg0_val, 0,
		      "Incorrect IPUC index value in suit_ipuc_get_info() call");

	/* ... and the IPUC was not set up */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");
	/* ... and the IPUC content was not modified */
	zassert_equal(suit_ipuc_write_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write() calls");
}

ZTEST_F(flash_ipuc_tests, test_driver_write_read_ro)
{
	struct zcbor_string test_component_id = {
		.value = component_radio_0x54000_0x1000,
		.len = sizeof(component_radio_0x54000_0x1000),
	};
	struct device *flash_ipuc = NULL;
	uint8_t sample_data[] = {0x1, 0x2, 0x3, 0x4};
	int ret = 0;

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;
	suit_ipuc_write_setup_fake.custom_fake = suit_ipuc_write_setup_fake_func;
	suit_ipuc_write_fake.custom_fake = suit_ipuc_write_fake_func;

	/* GIVEN all sample IPUCs are available */
	setup_sample_ipucs(fixture);
	/* ... and the test expects the second plaintext, uncompressed IPUC to be configured */
	fixture->exp_component_idx = 1;
	fixture->exp_encryption_info = NULL;
	fixture->exp_compression_info = NULL;
	/* ... and the radio IPUC is correctly setup */
	flash_ipuc = flash_component_ipuc_create(&test_component_id, NULL, NULL);
	zassert_not_null(flash_ipuc, "IPUC component create for component ID failed");
	/* ... and the IPUC was checked for component count */
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls");
	/* ... and the IPUC with the first two indexes was checked */
	zassert_equal(suit_ipuc_get_info_fake.call_count, 2,
		      "Incorrect number of suit_ipuc_get_info() calls");
	zassert_equal(suit_ipuc_get_info_fake.arg0_history[0], 0,
		      "Incorrect IPUC index value in suit_ipuc_get_info() call");
	zassert_equal(suit_ipuc_get_info_fake.arg0_history[1], 1,
		      "Incorrect IPUC index value in suit_ipuc_get_info() call");
	/* ... and the IPUC was not set up for writing at this stage */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	for (size_t offset = 0; offset < 0x2000; offset += 0x300) {
		/* WHEN the flash write API is called */
		ret = flash_write(flash_ipuc, offset, sample_data, sizeof(sample_data));

		if (offset < 0x1000) {
			/* ... and the offset is within the IPUC access range */
			/* THEN the memory is modified through IPUC API */
			zassert_equal(ret, 0, "Write inside IPUC area failed");
			zassert_equal(suit_ipuc_write_fake.call_count, offset / 0x300 + 1,
				      "Incorrect number of suit_ipuc_write() calls @0x%x", offset);
			zassert_equal(suit_ipuc_write_fake.arg1_val, offset,
				      "Incorrect offset passed through IPUC API @0x%x", offset);
			zassert_equal(suit_ipuc_write_fake.arg3_val, sizeof(sample_data),
				      "Incorrect data length passed through IPUC API @0x%x",
				      offset);
			zassert_equal(
				suit_ipuc_write_fake.arg4_val, false,
				"Incorrect last_chunk flag value passed through IPUC API @0x%x",
				offset);
		} else {
			/* ... and the offset is not within the IPUC access range */
			/* THEN the memory is not modified through IPUC API */
			zassert_equal(ret, -ENOMEM, "Write outside of IPUC area did not fail");
			zassert_equal(suit_ipuc_write_fake.call_count, 0x1000 / 0x300 + 1,
				      "Incorrect number of suit_ipuc_write() calls");
		}

		/* ... and it is not possible to read back the data */
		ret = flash_read(flash_ipuc, offset, sample_data, sizeof(sample_data));
		zassert_equal(ret, -EACCES, "Read inside write-only IPUC area did not fail");
	}

	/* ... and the IPUC was set up only once for writing at the first write request */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	/* ... and it is possible to close the IPUC slot */
	ret = flash_write(flash_ipuc, 0, NULL, 0);
	zassert_equal(ret, 0, "IPUC flush failed failed");
	zassert_equal(suit_ipuc_write_fake.call_count, 0x1000 / 0x300 + 2,
		      "Incorrect number of suit_ipuc_write() calls");
	zassert_equal(suit_ipuc_write_fake.arg1_val, 0, "Incorrect offset passed through IPUC API");
	zassert_equal_ptr(suit_ipuc_write_fake.arg2_val, NULL,
			  "Incorrect data passed through IPUC API");
	zassert_equal(suit_ipuc_write_fake.arg3_val, 0,
		      "Incorrect data length passed through IPUC API");
	zassert_equal(suit_ipuc_write_fake.arg4_val, true,
		      "Incorrect last_chunk flag value passed through IPUC API");

	/* Release the IPUC */
	flash_ipuc_release(flash_ipuc);
}

ZTEST_F(flash_ipuc_tests, test_driver_write_read_rw)
{
	struct zcbor_string test_component_id = {
		.value = component_app_0x94000_0x6d000,
		.len = sizeof(component_app_0x94000_0x6d000),
	};
	struct device *flash_ipuc = NULL;
	uint8_t sample_data[] = {0x1, 0x2, 0x3, 0x4};
	uint8_t read_buf[sizeof(sample_data)] = {0x00};
	int ret = 0;

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;
	suit_ipuc_write_setup_fake.custom_fake = suit_ipuc_write_setup_fake_func;
	suit_ipuc_write_fake.custom_fake = suit_ipuc_write_fake_func;

	/* GIVEN all sample IPUCs are available */
	setup_sample_ipucs(fixture);
	/* ... and the test expects the third plaintext, uncompressed IPUC to be configured */
	fixture->exp_component_idx = 3;
	fixture->exp_encryption_info = NULL;
	fixture->exp_compression_info = NULL;
	/* ... and the application IPUC is correctly setup */
	flash_ipuc = flash_component_ipuc_create(&test_component_id, NULL, NULL);
	zassert_not_null(flash_ipuc, "IPUC component create for component ID failed");
	/* ... and the IPUC was checked for component count */
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls");
	/* ... and the IPUC with the first 4 indexes were checked */
	zassert_equal(suit_ipuc_get_info_fake.call_count, 4,
		      "Incorrect number of suit_ipuc_get_info() calls");
	zassert_equal(suit_ipuc_get_info_fake.arg0_history[0], 0,
		      "Incorrect IPUC index value in suit_ipuc_get_info() call");
	zassert_equal(suit_ipuc_get_info_fake.arg0_history[1], 1,
		      "Incorrect IPUC index value in suit_ipuc_get_info() call");
	zassert_equal(suit_ipuc_get_info_fake.arg0_history[2], 2,
		      "Incorrect IPUC index value in suit_ipuc_get_info() call");
	zassert_equal(suit_ipuc_get_info_fake.arg0_history[3], 3,
		      "Incorrect IPUC index value in suit_ipuc_get_info() call");
	/* ... and the IPUC was not set up for writing at this stage */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	for (size_t offset = 0; offset < 0x6d000; offset += 0x300) {
		/* WHEN the flash write API is called */
		ret = flash_write(flash_ipuc, offset, sample_data, sizeof(sample_data));

		if (offset < 0x6d000) {
			/* ... and the offset is within the IPUC access range */
			/* THEN the memory is modified through IPUC API */
			zassert_equal(ret, 0, "Write inside IPUC area failed");
			zassert_equal(suit_ipuc_write_fake.call_count, offset / 0x300 + 1,
				      "Incorrect number of suit_ipuc_write() calls");
			zassert_equal(suit_ipuc_write_fake.arg1_val, offset,
				      "Incorrect offset passed through IPUC API");
			zassert_equal(suit_ipuc_write_fake.arg3_val, sizeof(sample_data),
				      "Incorrect data length passed through IPUC API");
			zassert_equal(suit_ipuc_write_fake.arg4_val, false,
				      "Incorrect last_chunk flag value passed through IPUC API");
			/* ... and it is possible to read back the data */
			ret = flash_read(flash_ipuc, offset, read_buf, sizeof(read_buf));
			zassert_equal(ret, 0, "Read inside readable IPUC area failed");
		} else {
			/* ... and the offset is not within the IPUC access range */
			/* THEN the memory is not modified through IPUC API */
			zassert_equal(ret, -ENOMEM, "Write outside of IPUC area did not fail");
			zassert_equal(suit_ipuc_write_fake.call_count, 0x6d000 / 0x300 + 1,
				      "Incorrect number of suit_ipuc_write() calls");
			/* ... and it is not possible to read back the data */
			ret = flash_read(flash_ipuc, offset, sample_data, sizeof(sample_data));
			zassert_equal(ret, -ENOMEM,
				      "Read inside write-only IPUC area did not fail");
		}
	}

	/* ... and the IPUC was set up only once for writing at the first write request */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	/* ... and it is possible to close the IPUC slot */
	ret = flash_write(flash_ipuc, 0, NULL, 0);
	zassert_equal(ret, 0, "IPUC flush failed failed");
	zassert_equal(suit_ipuc_write_fake.call_count, 0x6d000 / 0x300 + 2,
		      "Incorrect number of suit_ipuc_write() calls");
	zassert_equal(suit_ipuc_write_fake.arg1_val, 0, "Incorrect offset passed through IPUC API");
	zassert_equal_ptr(suit_ipuc_write_fake.arg2_val, NULL,
			  "Incorrect data passed through IPUC API");
	zassert_equal(suit_ipuc_write_fake.arg3_val, 0,
		      "Incorrect data length passed through IPUC API");
	zassert_equal(suit_ipuc_write_fake.arg4_val, true,
		      "Incorrect last_chunk flag value passed through IPUC API");

	/* Release the IPUC */
	flash_ipuc_release(flash_ipuc);
}

ZTEST_F(flash_ipuc_tests, test_driver_erase_ro)
{
	struct zcbor_string test_component_id = {
		.value = component_radio_0x54000_0x1000,
		.len = sizeof(component_radio_0x54000_0x1000),
	};
	struct device *flash_ipuc = NULL;
	int ret = 0;
	uint8_t sample_data[] = {0xAB};

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;
	suit_ipuc_write_setup_fake.custom_fake = suit_ipuc_write_setup_fake_func;
	suit_ipuc_write_fake.custom_fake = suit_ipuc_write_fake_func;

	/* GIVEN all sample IPUCs are available */
	setup_sample_ipucs(fixture);
	/* ... and the test expects the second plaintext, uncompressed IPUC to be configured */
	fixture->exp_component_idx = 1;
	fixture->exp_encryption_info = NULL;
	fixture->exp_compression_info = NULL;
	/* ... and the radio IPUC is correctly setup */
	flash_ipuc = flash_component_ipuc_create(&test_component_id, NULL, NULL);
	zassert_not_null(flash_ipuc, "IPUC component create for component ID failed");
	/* ... and the IPUC was checked for component count */
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls");
	/* ... and the IPUC with the first two indexes was checked */
	zassert_equal(suit_ipuc_get_info_fake.call_count, 2,
		      "Incorrect number of suit_ipuc_get_info() calls");
	zassert_equal(suit_ipuc_get_info_fake.arg0_history[0], 0,
		      "Incorrect IPUC index value in suit_ipuc_get_info() call");
	zassert_equal(suit_ipuc_get_info_fake.arg0_history[1], 1,
		      "Incorrect IPUC index value in suit_ipuc_get_info() call");
	/* ... and the IPUC was set up for writing as a result of a write request */
	zassert_equal(flash_write(flash_ipuc, 0, sample_data, sizeof(sample_data)), 0,
		      "Failed to initialize IPUC through write request");
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_write_setup() calls");
	zassert_equal(suit_ipuc_write_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_write() calls");

	/* Reset fake to keep the offset/history offsets intact. */
	RESET_FAKE(suit_ipuc_write);
	suit_ipuc_write_fake.custom_fake = suit_ipuc_write_fake_func;

	/* WHEN the erase API is called */
	ret = flash_erase(flash_ipuc, 0, 0xFFF);

	size_t write_blocks_passed = suit_ipuc_write_fake.call_count;

	/* THEN the memory is modified through IPUC API */
	zassert_equal(ret, 0, "Erase inside IPUC area failed");
	zassert_true(suit_ipuc_write_fake.call_count > 0,
		     "Incorrect number of suit_ipuc_write() calls");
	for (size_t i = 0; i < suit_ipuc_write_fake.call_count && i < FFF_ARG_HISTORY_LEN; i++) {
		zassert_equal(
			suit_ipuc_write_fake.arg1_history[i], 0x1000 / write_blocks_passed * i,
			"Incorrect offset (0x%x != 0x%x) passed through IPUC API in iteration %d",
			suit_ipuc_write_fake.arg1_history[i], 0x1000 / write_blocks_passed * i, i);
		zassert_equal(suit_ipuc_write_fake.arg3_history[i], 0x1000 / write_blocks_passed,
			      "Incorrect data length (0x%x != 0x%x) passed through IPUC API @%d",
			      suit_ipuc_write_fake.arg3_history[i], 0x1000 / write_blocks_passed,
			      i);
		zassert_equal(
			suit_ipuc_write_fake.arg4_history[i], false,
			"Incorrect last_chunk flag value passed through IPUC API in iteration %d",
			i);
	}

	/* ... and it is possible to do a massive erase through write_setup */
	ret = flash_erase(flash_ipuc, 0, 0x1000);
	zassert_equal(ret, 0, "Masive erase inside IPUC area failed");
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 2,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	/* ... and it is possible to close the IPUC slot */
	ret = flash_write(flash_ipuc, 0, NULL, 0);
	zassert_equal(ret, 0, "IPUC flush failed failed");
	zassert_equal(suit_ipuc_write_fake.call_count, write_blocks_passed + 1,
		      "Incorrect number of suit_ipuc_write() calls (%d != %d)",
		      suit_ipuc_write_fake.call_count, write_blocks_passed + 1);
	zassert_equal(suit_ipuc_write_fake.arg1_val, 0, "Incorrect offset passed through IPUC API");
	zassert_equal_ptr(suit_ipuc_write_fake.arg2_val, NULL,
			  "Incorrect data passed through IPUC API");
	zassert_equal(suit_ipuc_write_fake.arg3_val, 0,
		      "Incorrect data length passed through IPUC API");
	zassert_equal(suit_ipuc_write_fake.arg4_val, true,
		      "Incorrect last_chunk flag value passed through IPUC API");

	/* Release the IPUC */
	flash_ipuc_release(flash_ipuc);
}

ZTEST_F(flash_ipuc_tests, test_cache_bank0_check_OK)
{
	uintptr_t ipuc_address = 0;
	size_t ipuc_size = 0;
	uintptr_t exp_address = 0;
	size_t exp_size = 0;
	uint8_t exp_cpu_id = 0;
	struct zcbor_string exp_component_id = {
		.value = component_app_0x94000_0x6d000,
		.len = sizeof(component_app_0x94000_0x6d000),
	};

	/* Read the expected address and size from the component ID. */
	zassert_equal(suit_plat_decode_component_id(&exp_component_id, &exp_cpu_id, &exp_address,
						    &exp_size),
		      SUIT_PLAT_SUCCESS, "Unable to decode expected cache IPUC component ID");

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;

	/* GIVEN all sample IPUCs are available */
	setup_sample_ipucs(fixture);

	/* WHEN the check for any cache IPUC is called */
	zassert_equal(flash_cache_ipuc_check(0, &ipuc_address, &ipuc_size), true,
		      "Failed to check application cache IPUC");

	/* THEN the API returns true */
	/* ... and the IPUC was checked for component count */
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls");
	/* ... and all defined IPUCs were checked */
	zassert_equal(suit_ipuc_get_info_fake.call_count, fixture->count + 1,
		      "Incorrect number of suit_ipuc_get_info() calls (%d != %d)",
		      suit_ipuc_get_info_fake.call_count, fixture->count + 1);
	for (size_t i = 0; i < fixture->count; i++) {
		zassert_equal(
			suit_ipuc_get_info_fake.arg0_history[i], i,
			"Incorrect IPUC index value in suit_ipuc_get_info() call in iteration %d",
			i);
	}

	/* ... and the information about the largest application IPUC was returned */
	zassert_equal(ipuc_address, exp_address, "unexpected IPUC address returned (0x%x != 0x%x)",
		      ipuc_address, exp_address);
	zassert_equal(ipuc_size, exp_size, "unexpected IPUC size returned (0x%x != 0x%x)",
		      ipuc_size, exp_size);

	/* ... and the IPUC was not set up */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");
	/* ... and the IPUC content was not modified */
	zassert_equal(suit_ipuc_write_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write() calls");
}

ZTEST_F(flash_ipuc_tests, test_cache_bank1_check_OK)
{
	uintptr_t ipuc_address = 0;
	size_t ipuc_size = 0;
	uintptr_t exp_address = 0;
	size_t exp_size = 0;
	uint8_t exp_cpu_id = 0;
	struct zcbor_string exp_component_id = {
		.value = component_app_0x101000_0x60000,
		.len = sizeof(component_app_0x101000_0x60000),
	};
	uintptr_t bank1_address = (uintptr_t)suit_plat_mem_nvm_ptr_get(0x100000);

	/* Read the expected address and size from the component ID. */
	zassert_equal(suit_plat_decode_component_id(&exp_component_id, &exp_cpu_id, &exp_address,
						    &exp_size),
		      SUIT_PLAT_SUCCESS, "Unable to decode expected cache IPUC component ID");

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;

	/* GIVEN all sample IPUCs are available */
	setup_sample_ipucs(fixture);

	/* WHEN the check for bank 1 cache is called */
	zassert_equal(flash_cache_ipuc_check(bank1_address, &ipuc_address, &ipuc_size), true,
		      "Failed to check application cache IPUC");

	/* THEN the API returns true */
	/* ... and the IPUC was checked for component count */
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls");
	/* ... and all defined IPUCs were checked */
	zassert_equal(suit_ipuc_get_info_fake.call_count, fixture->count + 1,
		      "Incorrect number of suit_ipuc_get_info() calls (%d != %d)",
		      suit_ipuc_get_info_fake.call_count, fixture->count + 1);
	for (size_t i = 0; i < fixture->count; i++) {
		zassert_equal(
			suit_ipuc_get_info_fake.arg0_history[i], i,
			"Incorrect IPUC index value in suit_ipuc_get_info() call in iteration %d",
			i);
	}

	/* ... and the information about the largest application IPUC on bank1 was returned */
	zassert_equal(ipuc_address, exp_address, "unexpected IPUC address returned (0x%x != 0x%x)",
		      ipuc_address, exp_address);
	zassert_equal(ipuc_size, exp_size, "unexpected IPUC size returned (0x%x != 0x%x)",
		      ipuc_size, exp_size);

	/* ... and the IPUC was not set up */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");
	/* ... and the IPUC content was not modified */
	zassert_equal(suit_ipuc_write_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write() calls");
}

ZTEST_F(flash_ipuc_tests, test_cache_bank1_partial_check_OK)
{
	uintptr_t ipuc_address = 0;
	size_t ipuc_size = 0;
	uintptr_t exp_address = 0;
	size_t exp_size = 0;
	uint8_t exp_cpu_id = 0;
	struct zcbor_string exp_component_id = {
		.value = component_app_0x94000_0x6d000,
		.len = sizeof(component_app_0x94000_0x6d000),
	};
	uintptr_t bank1_address = (uintptr_t)suit_plat_mem_nvm_ptr_get(0x100000);

	/* Read the expected address and size from the component ID. */
	zassert_equal(suit_plat_decode_component_id(&exp_component_id, &exp_cpu_id, &exp_address,
						    &exp_size),
		      SUIT_PLAT_SUCCESS, "Unable to decode expected cache IPUC component ID");

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;

	/* GIVEN all sample IPUCs except one are available */
	setup_sample_ipucs(fixture);
	fixture->count--;

	/* WHEN the check for bank 1 cache is called */
	zassert_equal(flash_cache_ipuc_check(bank1_address, &ipuc_address, &ipuc_size), true,
		      "Failed to check application cache IPUC");

	/* THEN the API returns true */
	/* ... and the IPUC was checked for component count */
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls");
	/* ... and all defined IPUCs were checked */
	zassert_equal(suit_ipuc_get_info_fake.call_count, fixture->count + 1,
		      "Incorrect number of suit_ipuc_get_info() calls (%d != %d)",
		      suit_ipuc_get_info_fake.call_count, fixture->count + 1);
	for (size_t i = 0; i < fixture->count; i++) {
		zassert_equal(
			suit_ipuc_get_info_fake.arg0_history[i], i,
			"Incorrect IPUC index value in suit_ipuc_get_info() call in iteration %d",
			i);
	}

	/* ... and the information about the largest application IPUC was returned */
	zassert_equal(ipuc_address, exp_address, "unexpected IPUC address returned (0x%x != 0x%x)",
		      ipuc_address, exp_address);
	zassert_equal(ipuc_size, exp_size, "unexpected IPUC size returned (0x%x != 0x%x)",
		      ipuc_size, exp_size);

	/* ... and the IPUC was not set up */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");
	/* ... and the IPUC content was not modified */
	zassert_equal(suit_ipuc_write_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write() calls");
}

ZTEST_F(flash_ipuc_tests, test_cache_bank1_partial_create_OK)
{
	uintptr_t ipuc_address = 0;
	size_t ipuc_size = 0;
	uintptr_t exp_address = 0;
	size_t exp_size = 0;
	uint8_t exp_cpu_id = 0;
	struct zcbor_string exp_component_id = {
		.value = component_app_0x94000_0x6d000,
		.len = sizeof(component_app_0x94000_0x6d000),
	};
	uintptr_t bank1_address = (uintptr_t)suit_plat_mem_nvm_ptr_get(0x100000);
	struct device *flash_ipuc = NULL;

	/* Read the expected address and size from the component ID. */
	zassert_equal(suit_plat_decode_component_id(&exp_component_id, &exp_cpu_id, &exp_address,
						    &exp_size),
		      SUIT_PLAT_SUCCESS, "Unable to decode expected cache IPUC component ID");

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;

	/* GIVEN all sample IPUCs except one are available */
	setup_sample_ipucs(fixture);
	fixture->count--;

	/* WHEN the bank 1 cache IPUC is requested */
	flash_ipuc = flash_cache_ipuc_create(bank1_address, &ipuc_address, &ipuc_size);

	/* THEN the API returns a valid IPUC driver */
	zassert_not_null(flash_ipuc, "Cache IPUC create failed");

	/* ... and the IPUC was checked for component count */
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls");
	/* ... and all defined IPUCs were checked */
	zassert_equal(suit_ipuc_get_info_fake.call_count, fixture->count + 1,
		      "Incorrect number of suit_ipuc_get_info() calls (%d != %d)",
		      suit_ipuc_get_info_fake.call_count, fixture->count + 1);
	for (size_t i = 0; i < fixture->count; i++) {
		zassert_equal(
			suit_ipuc_get_info_fake.arg0_history[i], i,
			"Incorrect IPUC index value in suit_ipuc_get_info() call in iteration %d",
			i);
	}

	/* ... and the information about the largest application IPUC was returned */
	zassert_equal(ipuc_address, exp_address, "unexpected IPUC address returned (0x%x != 0x%x)",
		      ipuc_address, exp_address);
	zassert_equal(ipuc_size, exp_size, "unexpected IPUC size returned (0x%x != 0x%x)",
		      ipuc_size, exp_size);
	/* ... and the IPUC was not set up for writing */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	/* ... and the IPUC was not initialized over SSF */
	zassert_equal(flash_ipuc_setup_pending(flash_ipuc), true,
		      "Cache IPUC initialized immediately");
	/* ... and the IPUC content was not modified */
	zassert_equal(suit_ipuc_write_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write() calls");

	/* Release the IPUC */
	flash_ipuc_release(flash_ipuc);
}

ZTEST_F(flash_ipuc_tests, test_cache_setup_on_erase_OK)
{
	uintptr_t ipuc_address = 0;
	size_t ipuc_size = 0;
	uintptr_t exp_address = 0;
	size_t exp_size = 0;
	uint8_t exp_cpu_id = 0;
	struct zcbor_string exp_component_id = {
		.value = component_app_0x94000_0x6d000,
		.len = sizeof(component_app_0x94000_0x6d000),
	};
	uintptr_t bank1_address = (uintptr_t)suit_plat_mem_nvm_ptr_get(0x100000);
	struct device *flash_ipuc = NULL;

	/* Read the expected address and size from the component ID. */
	zassert_equal(suit_plat_decode_component_id(&exp_component_id, &exp_cpu_id, &exp_address,
						    &exp_size),
		      SUIT_PLAT_SUCCESS, "Unable to decode expected cache IPUC component ID");

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;

	/* GIVEN all sample IPUCs are available */
	setup_sample_ipucs(fixture);

	/* WHEN the bank 1 cache IPUC is requested */
	flash_ipuc = flash_cache_ipuc_create(bank1_address, &ipuc_address, &ipuc_size);

	/* THEN the API returns a valid IPUC driver */
	zassert_not_null(flash_ipuc, "Cache IPUC create failed");

	/* ... and the IPUC was not initialized over SSF */
	zassert_equal(flash_ipuc_setup_pending(flash_ipuc), true,
		      "Cache IPUC initialized immediately");
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	/* WHEN erase API is called for any byte */
	zassert_equal(flash_erase(flash_ipuc, 1, 1), 0, "Failed to erase cache IPUC");

	/* THEN the IPUC was not set up for writing */
	zassert_equal(flash_ipuc_setup_pending(flash_ipuc), true,
		      "Cache IPUC initialized while erasing uninitialized instance");
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	/* ... and the IPUC content was not modified */
	zassert_equal(suit_ipuc_write_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write() calls");

	/* Release the IPUC */
	flash_ipuc_release(flash_ipuc);
}

ZTEST_F(flash_ipuc_tests, test_cache_setup_on_write_OK)
{
	uintptr_t ipuc_address = 0;
	size_t ipuc_size = 0;
	uintptr_t exp_address = 0;
	size_t exp_size = 0;
	uint8_t exp_cpu_id = 0;
	struct zcbor_string exp_component_id = {
		.value = component_app_0x101000_0x60000,
		.len = sizeof(component_app_0x101000_0x60000),
	};
	uintptr_t bank1_address = (uintptr_t)suit_plat_mem_nvm_ptr_get(0x100000);
	struct device *flash_ipuc = NULL;
	uint8_t exp_data[] = {0xAB};

	/* Read the expected address and size from the component ID. */
	zassert_equal(suit_plat_decode_component_id(&exp_component_id, &exp_cpu_id, &exp_address,
						    &exp_size),
		      SUIT_PLAT_SUCCESS, "Unable to decode expected cache IPUC component ID");

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;
	suit_ipuc_write_fake.custom_fake = suit_ipuc_write_fake_func;

	/* GIVEN all sample IPUCs are available */
	setup_sample_ipucs(fixture);

	/* ... and the test expects the fourth plaintext, uncompressed IPUC to be configured */
	fixture->exp_component_idx = 4;
	fixture->exp_encryption_info = NULL;
	fixture->exp_compression_info = NULL;

	/* WHEN the bank 1 cache IPUC is requested */
	flash_ipuc = flash_cache_ipuc_create(bank1_address, &ipuc_address, &ipuc_size);

	/* THEN the API returns a valid IPUC driver */
	zassert_not_null(flash_ipuc, "Cache IPUC create failed");

	/* ... and the IPUC was not initialized over SSF */
	zassert_equal(flash_ipuc_setup_pending(flash_ipuc), true,
		      "Cache IPUC initialized immediately");
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 0,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	/* WHEN write API is called for any byte */
	zassert_equal(flash_write(flash_ipuc, 1, exp_data, sizeof(exp_data)), 0,
		      "Failed to erase cache IPUC");

	/* THEN the IPUC was set up for writing */
	zassert_equal(flash_ipuc_setup_pending(flash_ipuc), false,
		      "Cache IPUC not initialized while erasing contents");
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_write_setup() calls");

	/* ... and the IPUC content was modified */
	zassert_equal(suit_ipuc_write_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_write() calls");
	zassert_equal(suit_ipuc_write_fake.arg1_val, 1, "Incorrect offset passed through IPUC API");
	zassert_equal(suit_ipuc_write_fake.arg3_val, sizeof(exp_data),
		      "Incorrect data length passed through IPUC API");
	zassert_equal(suit_ipuc_write_fake.arg4_val, false,
		      "Incorrect last_chunk flag value passed through IPUC API");

	/* Release the IPUC */
	flash_ipuc_release(flash_ipuc);
}

ZTEST_F(flash_ipuc_tests, test_find_ipuc)
{
	struct device *flash_ipuc = NULL;
	struct zcbor_string radio_component_id = {
		.value = component_radio_0x55000_0x3f000,
		.len = sizeof(component_radio_0x55000_0x3f000),
	};
	uintptr_t ipuc_address = 0;
	size_t ipuc_size = 0;
	uintptr_t cache_ipuc_address = 0;
	size_t cache_ipuc_size = 0;

	/* Set fakes, that return values from the test fixture. */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;

	/* GIVEN all sample IPUCs are available */
	setup_sample_ipucs(fixture);

	/* ... and two of them are configured */
	zassert_not_null(flash_component_ipuc_create(&radio_component_id, NULL, NULL),
			 "Failed to create radio IPUC before test execution");
	zassert_not_null(flash_cache_ipuc_create(0, &cache_ipuc_address, &cache_ipuc_size),
			 "Failed to create application cache IPUC before test execution");

	/* WHEN component IPUC for unconfigured ID is searched */
	flash_ipuc = flash_ipuc_find((uintptr_t)suit_plat_mem_nvm_ptr_get(0x54000), 0x1000,
				     &ipuc_address, &ipuc_size);
	/* THEN the API returns NULL */
	zassert_is_null(flash_ipuc, "Find for unconfigured IPUC shall fail");

	/* WHEN component IPUC for address overlapping with configured ID is searched */
	flash_ipuc = flash_ipuc_find((uintptr_t)suit_plat_mem_nvm_ptr_get(0x55000 - 1), 0x3f000,
				     &ipuc_address, &ipuc_size);
	/* THEN the API returns NULL */
	zassert_is_null(flash_ipuc, "Find for unconfigured IPUC shall fail");

	/* WHEN component IPUC for matching address and too bigger size is searched */
	flash_ipuc = flash_ipuc_find((uintptr_t)suit_plat_mem_nvm_ptr_get(0x55000), 0x3f000 + 1,
				     &ipuc_address, &ipuc_size);
	/* THEN the API returns NULL */
	zassert_is_null(flash_ipuc, "Find for unconfigured IPUC shall fail");

	/* WHEN component IPUC for matching address and size is searched */
	flash_ipuc = flash_ipuc_find((uintptr_t)suit_plat_mem_nvm_ptr_get(0x55000), 0x3f000,
				     &ipuc_address, &ipuc_size);
	/* THEN the API returns a valid IPUC driver */
	zassert_not_null(flash_ipuc, "Find for configured IPUC failed");
	/* ... and the returned address and size matches with the component ID */
	zassert_equal(ipuc_address, (uintptr_t)suit_plat_mem_nvm_ptr_get(0x55000),
		      "Component IPUC address does not match");
	zassert_equal(ipuc_size, 0x3f000, "Component IPUC size does not match");

	/* WHEN component IPUC for address within configured ID is searched */
	flash_ipuc = flash_ipuc_find((uintptr_t)suit_plat_mem_nvm_ptr_get(0x55000 + 1), 0x1000,
				     &ipuc_address, &ipuc_size);
	/* THEN the API returns a valid IPUC driver */
	zassert_not_null(flash_ipuc, "Find for configured IPUC failed");
	/* ... and the returned address and size matches with the component ID */
	zassert_equal(ipuc_address, (uintptr_t)suit_plat_mem_nvm_ptr_get(0x55000),
		      "Component IPUC address does not match");
	zassert_equal(ipuc_size, 0x3f000, "Component IPUC size does not match");

	/* WHEN component IPUC for address returned by cache constructor is searched */
	flash_ipuc =
		flash_ipuc_find(cache_ipuc_address, cache_ipuc_size, &ipuc_address, &ipuc_size);
	/* THEN the API returns a valid IPUC driver */
	zassert_not_null(flash_ipuc, "Find for configured IPUC failed");
	/* ... and the returned address and size matches with cache region */
	zassert_equal(ipuc_address, cache_ipuc_address, "Cache IPUC address does not match");
	zassert_equal(ipuc_size, cache_ipuc_size, "Cache IPUC size does not match");

	/* Release IPUCs */
	flash_ipuc = flash_ipuc_find((uintptr_t)suit_plat_mem_nvm_ptr_get(0x55000), 0x1000,
				     &ipuc_address, &ipuc_size);
	flash_ipuc_release(flash_ipuc);

	flash_ipuc =
		flash_ipuc_find(cache_ipuc_address, cache_ipuc_size, &ipuc_address, &ipuc_size);
	flash_ipuc_release(flash_ipuc);
}
