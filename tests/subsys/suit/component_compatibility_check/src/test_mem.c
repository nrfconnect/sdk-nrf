/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <mocks_sdfw.h>
#include <suit_plat_component_compatibility.h>
#include <suit_storage.h>
#include <suit_execution_mode.h>
#include <suit_types.h>
#include <suit_plat_mem_util.h>
#include "test_common.h"

static void *test_suite_setup(void)
{
	/* Assuming the device is provisioned with application MPI with root config and sample radio
	 * MPI entry.
	 */
	erase_area_nordic();
	erase_area_rad();
	erase_area_app();
	write_area_app();

	/* ... and SUIT storage is successfully initialized */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_init(), "Failed to initialize SUIT storage");

	/* ... and MPIs are correctly loaded */
	assert_nordic_classes();
	assert_sample_app_classes();

	return NULL;
}

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_sdfw_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();

	/* Set execution mode to the final boot state */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_execution_mode_set(EXECUTION_MODE_POST_INVOKE),
		      "Failed to update execution mode");
}

ZTEST_SUITE(suit_component_compatibility_check_tests, NULL, test_suite_setup, test_before, NULL,
	    NULL);

ZTEST(suit_component_compatibility_check_tests, test_app_local_positive)
{
	/* Sample component:
	 *  - type: MEM
	 *  - processor: Application (2)
	 *  - address: 0x80000
	 *  - size: 0x1000
	 */
	uint8_t test_mem_app[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',  0x41, 0x02, 0x45,
				  0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string test_id = {
		.value = test_mem_app,
		.len = sizeof(test_mem_app),
	};

	/* GIVEN  the sample compoenent memory is allowed to be modified */
	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;

	/* WHEN a sample MEM component, covering application area is validated for application
	 * manifest
	 */
	int err = suit_plat_component_compatibility_check(&app_local_cid, &test_id);

	/* THEN the component ID is successfully verified... */
	zassert_equal(
		SUIT_PLAT_SUCCESS, err,
		"Failed to validate application MEM component for application local manifest");

	/* ... and the arbiter was used to check for memory addresses */
	zassert_equal(arbiter_mem_access_check_fake.call_count, 1,
		      "Incorrect number of arbiter_mem_access_check() calls");
}

ZTEST(suit_component_compatibility_check_tests, test_app_radio_exec)
{
	/* Sample component:
	 *  - type: MEM
	 *  - processor: Radio (3)
	 *  - address: 0x80000
	 *  - size: 0x1000
	 */
	uint8_t test_mem_app[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',  0x41, 0x03, 0x45,
				  0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string test_id = {
		.value = test_mem_app,
		.len = sizeof(test_mem_app),
	};

	/* GIVEN  the sample compoenent memory is allowed to be modified */
	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;

	/* WHEN a sample MEM component, covering radio area is validated for application manifest */
	int err = suit_plat_component_compatibility_check(&app_local_cid, &test_id);

	/* THEN the component ID is denied... */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, err,
		      "Failed to block radio MEM component for application local manifest");

	/* ... and the arbiter was not used to check for memory addresses */
	zassert_equal(arbiter_mem_access_check_fake.call_count, 0,
		      "Incorrect number of arbiter_mem_access_check() calls");
}

ZTEST(suit_component_compatibility_check_tests, test_app_local_blocked)
{
	/* Sample component:
	 *  - type: MEM
	 *  - processor: Application (2)
	 *  - address: 0x80000
	 *  - size: 0x1000
	 */
	uint8_t test_mem_app[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',  0x41, 0x02, 0x45,
				  0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string test_id = {
		.value = test_mem_app,
		.len = sizeof(test_mem_app),
	};

	/* GIVEN  the sample compoenent memory is not allowed to be modified */
	arbiter_mem_access_check_fake.return_val = -1;

	/* WHEN a sample MEM component, covering radio area is validated for application manifest */
	int err = suit_plat_component_compatibility_check(&app_local_cid, &test_id);

	/* THEN the component ID is denied... */
	zassert_equal(
		SUIT_ERR_UNAUTHORIZED_COMPONENT, err,
		"Failed to block MEM component for application local manifest using arbiter API");

	/* ... and the arbiter was used to check for memory addresses */
	zassert_equal(arbiter_mem_access_check_fake.call_count, 1,
		      "Incorrect number of arbiter_mem_access_check() calls");
}

ZTEST(suit_component_compatibility_check_tests, test_app_local_envelope_address_boot)
{
	/* Sample component:
	 *  - type: MEM
	 *  - processor: Application (2)
	 *  - address: 0x80000
	 *  - size: 0x1000
	 */
	uint8_t test_mem_app[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',  0x41, 0x02, 0x45,
				  0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string test_id = {
		.value = test_mem_app,
		.len = sizeof(test_mem_app),
	};

	/* Update candidate info (1 region):
	 *  - address: 0x80000 size: 0x10
	 */
	suit_plat_mreg_t regions[] = {{
		.mem = suit_plat_mem_ptr_get(0x80000),
		.size = 0x10,
	}};

	/* GIVEN  the sample compoenent memory is allowed to be modified */
	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;

	/* ... and it overlaps with the current update candiadate */
	zassert_equal(
		SUIT_PLAT_SUCCESS,
		suit_storage_update_cand_set((suit_plat_mreg_t *)regions, ARRAY_SIZE(regions)),
		"Unable to configure update candidate info");

	/* ... and the orchestrator is currently booting from local manifests */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_execution_mode_set(EXECUTION_MODE_INVOKE),
		      "Failed to update execution mode");

	/* WHEN a sample MEM component, covering update candidate area is validated for application
	 * manifest
	 */
	int err = suit_plat_component_compatibility_check(&app_local_cid, &test_id);

	/* THEN the component ID is successfully verified... */
	zassert_equal(
		SUIT_PLAT_SUCCESS, err,
		"Failed to validate application MEM component for application local manifest");

	/* ... and the arbiter was used to check for memory addresses */
	zassert_equal(arbiter_mem_access_check_fake.call_count, 1,
		      "Incorrect number of arbiter_mem_access_check() calls");
}

ZTEST(suit_component_compatibility_check_tests, test_app_local_envelope_address)
{
	/* Sample component:
	 *  - type: MEM
	 *  - processor: Application (2)
	 *  - address: 0x80000
	 *  - size: 0x1000
	 */
	uint8_t test_mem_app[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',  0x41, 0x02, 0x45,
				  0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string test_id = {
		.value = test_mem_app,
		.len = sizeof(test_mem_app),
	};

	/* Update candidate info (1 region):
	 *  - address: 0x80000 size: 0x10
	 */
	suit_plat_mreg_t regions[] = {{
		.mem = suit_plat_mem_ptr_get(0x80000),
		.size = 0x10,
	}};

	/* GIVEN  the sample compoenent memory is allowed to be modified */
	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;

	/* ... and it overlaps with the current update candiadate */
	zassert_equal(
		SUIT_PLAT_SUCCESS,
		suit_storage_update_cand_set((suit_plat_mreg_t *)regions, ARRAY_SIZE(regions)),
		"Unable to configure update candidate info");

	/* ... and the orchestrator is currently processing the candidate */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_execution_mode_set(EXECUTION_MODE_INSTALL),
		      "Failed to update execution mode");

	/* WHEN a sample MEM component, covering update candidate area is validated for application
	 * manifest
	 */
	int err = suit_plat_component_compatibility_check(&app_local_cid, &test_id);

	/* THEN the component ID is denied... */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, err,
		      "Failed to block MEM component addressing dfu partition during update for "
		      "application local manifest");

	/* ... and the arbiter was not used to check for memory addresses */
	zassert_equal(arbiter_mem_access_check_fake.call_count, 0,
		      "Incorrect number of arbiter_mem_access_check() calls");
}

ZTEST(suit_component_compatibility_check_tests, test_app_local_before_envelope_address)
{
	/* Sample component:
	 *  - type: MEM
	 *  - processor: Application (2)
	 *  - address: 0x80000
	 *  - size: 0x1000
	 */
	uint8_t test_mem_app[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',  0x41, 0x02, 0x45,
				  0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string test_id = {
		.value = test_mem_app,
		.len = sizeof(test_mem_app),
	};

	/* Update candidate info (1 region):
	 *  - address: 0x7f000 size: 0x1001
	 */
	suit_plat_mreg_t regions[] = {{
		.mem = suit_plat_mem_ptr_get(0x7f000),
		.size = 0x1001,
	}};

	/* GIVEN  the sample compoenent memory is allowed to be modified */
	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;

	/* ... and it overlaps with the current update candiadate */
	zassert_equal(
		SUIT_PLAT_SUCCESS,
		suit_storage_update_cand_set((suit_plat_mreg_t *)regions, ARRAY_SIZE(regions)),
		"Unable to configure update candidate info");

	/* ... and the orchestrator is currently processing the candidate */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_execution_mode_set(EXECUTION_MODE_INSTALL),
		      "Failed to update execution mode");

	/* WHEN a sample MEM component, covering update candidate area is validated for application
	 * manifest
	 */
	int err = suit_plat_component_compatibility_check(&app_local_cid, &test_id);

	/* THEN the component ID is denied... */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, err,
		      "Failed to block MEM component addressing dfu partition during update for "
		      "application local manifest");

	/* ... and the arbiter was not used to check for memory addresses */
	zassert_equal(arbiter_mem_access_check_fake.call_count, 0,
		      "Incorrect number of arbiter_mem_access_check() calls");
}

ZTEST(suit_component_compatibility_check_tests, test_app_local_after_envelope_address)
{
	/* Sample component:
	 *  - type: MEM
	 *  - processor: Application (2)
	 *  - address: 0x80000
	 *  - size: 0x1000
	 */
	uint8_t test_mem_app[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',  0x41, 0x02, 0x45,
				  0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string test_id = {
		.value = test_mem_app,
		.len = sizeof(test_mem_app),
	};

	/* Update candidate info (1 region):
	 *  - address: 0x80fff size: 0x10
	 */
	suit_plat_mreg_t regions[] = {{
		.mem = suit_plat_mem_ptr_get(0x80fff),
		.size = 0x10,
	}};

	/* GIVEN  the sample compoenent memory is allowed to be modified */
	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;

	/* ... and it overlaps with the current update candiadate */
	zassert_equal(
		SUIT_PLAT_SUCCESS,
		suit_storage_update_cand_set((suit_plat_mreg_t *)regions, ARRAY_SIZE(regions)),
		"Unable to configure update candidate info");

	/* ... and the orchestrator is currently processing the candidate */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_execution_mode_set(EXECUTION_MODE_INSTALL),
		      "Failed to update execution mode");

	/* WHEN a sample MEM component, covering update candidate area is validated for application
	 * manifest
	 */
	int err = suit_plat_component_compatibility_check(&app_local_cid, &test_id);

	/* THEN the component ID is denied... */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, err,
		      "Failed to block MEM component addressing dfu partition during update for "
		      "application local manifest");

	/* ... and the arbiter was not used to check for memory addresses */
	zassert_equal(arbiter_mem_access_check_fake.call_count, 0,
		      "Incorrect number of arbiter_mem_access_check() calls");
}

ZTEST(suit_component_compatibility_check_tests, test_app_local_inside_envelope)
{
	/* Sample component:
	 *  - type: MEM
	 *  - processor: Application (2)
	 *  - address: 0x80000
	 *  - size: 0x1000
	 */
	uint8_t test_mem_app[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',  0x41, 0x02, 0x45,
				  0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string test_id = {
		.value = test_mem_app,
		.len = sizeof(test_mem_app),
	};

	/* Update candidate info (1 region):
	 *  - address: 0x7f000 size: 0x3000
	 */
	suit_plat_mreg_t regions[] = {{
		.mem = suit_plat_mem_ptr_get(0x7f000),
		.size = 0x3000,
	}};

	/* GIVEN  the sample compoenent memory is allowed to be modified */
	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;

	/* ... and it overlaps with the current update candiadate */
	zassert_equal(
		SUIT_PLAT_SUCCESS,
		suit_storage_update_cand_set((suit_plat_mreg_t *)regions, ARRAY_SIZE(regions)),
		"Unable to configure update candidate info");

	/* ... and the orchestrator is currently processing the candidate */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_execution_mode_set(EXECUTION_MODE_INSTALL),
		      "Failed to update execution mode");

	/* WHEN a sample MEM component, covering update candidate area is validated for application
	 * manifest
	 */
	int err = suit_plat_component_compatibility_check(&app_local_cid, &test_id);

	/* THEN the component ID is denied... */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, err,
		      "Failed to block MEM component addressing dfu partition during update for "
		      "application local manifest");

	/* ... and the arbiter was not used to check for memory addresses */
	zassert_equal(arbiter_mem_access_check_fake.call_count, 0,
		      "Incorrect number of arbiter_mem_access_check() calls");
}

ZTEST(suit_component_compatibility_check_tests, test_app_local_include_envelope)
{
	/* Sample component:
	 *  - type: MEM
	 *  - processor: Application (2)
	 *  - address: 0x80000
	 *  - size: 0x1000
	 */
	uint8_t test_mem_app[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',  0x41, 0x02, 0x45,
				  0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string test_id = {
		.value = test_mem_app,
		.len = sizeof(test_mem_app),
	};

	/* Update candidate info (1 region):
	 *  - address: 0x80f00 size: 0x10
	 */
	suit_plat_mreg_t regions[] = {{
		.mem = suit_plat_mem_ptr_get(0x80f00),
		.size = 0x10,
	}};

	/* GIVEN  the sample compoenent memory is allowed to be modified */
	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;

	/* ... and it overlaps with the current update candiadate */
	zassert_equal(
		SUIT_PLAT_SUCCESS,
		suit_storage_update_cand_set((suit_plat_mreg_t *)regions, ARRAY_SIZE(regions)),
		"Unable to configure update candidate info");

	/* ... and the orchestrator is currently processing the candidate */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_execution_mode_set(EXECUTION_MODE_INSTALL),
		      "Failed to update execution mode");

	/* WHEN a sample MEM component, covering update candidate area is validated for application
	 * manifest
	 */
	int err = suit_plat_component_compatibility_check(&app_local_cid, &test_id);

	/* THEN the component ID is denied... */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, err,
		      "Failed to block MEM component addressing dfu partition during update for "
		      "application local manifest");

	/* ... and the arbiter was not used to check for memory addresses */
	zassert_equal(arbiter_mem_access_check_fake.call_count, 0,
		      "Incorrect number of arbiter_mem_access_check() calls");
}

ZTEST(suit_component_compatibility_check_tests, test_app_local_surrounded)
{
	/* Sample component:
	 *  - type: MEM
	 *  - processor: Application (2)
	 *  - address: 0x80000
	 *  - size: 0x1000
	 */
	uint8_t test_mem_app[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',  0x41, 0x02, 0x45,
				  0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string test_id = {
		.value = test_mem_app,
		.len = sizeof(test_mem_app),
	};

	/* Update candidate info (2 regions):
	 *  - address: 0x7f000 size: 0x1000
	 *  - address: 0x82000 size: 0x1000
	 */
	/* clang-format off */
	suit_plat_mreg_t regions[] = {
		{
			.mem = suit_plat_mem_ptr_get(0x7f000),
			.size = 0x1000,
		},
		{
			.mem = suit_plat_mem_ptr_get(0x82000),
			.size = 0x1000,
		}
	};
	/* clang-format on */

	/* GIVEN  the sample compoenent memory is allowed to be modified */
	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;

	/* ... and it overlaps with the current update candiadate */
	zassert_equal(
		SUIT_PLAT_SUCCESS,
		suit_storage_update_cand_set((suit_plat_mreg_t *)regions, ARRAY_SIZE(regions)),
		"Unable to configure update candidate info");

	/* ... and the orchestrator is currently processing the candidate */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_execution_mode_set(EXECUTION_MODE_INSTALL),
		      "Failed to update execution mode");

	/* WHEN a sample MEM component, covering area between update candidate regions is validated
	 * for application manifest
	 */
	int err = suit_plat_component_compatibility_check(&app_local_cid, &test_id);

	/* THEN the component ID is successfully verified... */
	zassert_equal(
		SUIT_PLAT_SUCCESS, err,
		"Failed to validate application MEM component for application local manifest");

	/* ... and the arbiter was used to check for memory addresses */
	zassert_equal(arbiter_mem_access_check_fake.call_count, 1,
		      "Incorrect number of arbiter_mem_access_check() calls");
}

ZTEST(suit_component_compatibility_check_tests, test_app_root_invalid_exec_type)
{
	/* Sample component:
	 *  - type: MEM
	 *  - processor: Application (2)
	 *  - address: 0x80000
	 *  - size: 0x1000
	 */
	uint8_t test_mem_app[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',  0x41, 0x02, 0x45,
				  0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string test_id = {
		.value = test_mem_app,
		.len = sizeof(test_mem_app),
	};

	/* GIVEN  the sample compoenent memory is allowed to be modified */
	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;

	/* WHEN a sample MEM component, covering radio area is validated for application manifest */
	int err = suit_plat_component_compatibility_check(&app_root_cid, &test_id);

	/* THEN the component ID is denied... */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, err,
		      "Failed to block executable MEM component in root manifest");

	/* ... and the arbiter was not used to check for memory addresses */
	zassert_equal(arbiter_mem_access_check_fake.call_count, 0,
		      "Incorrect number of arbiter_mem_access_check() calls");
}

ZTEST(suit_component_compatibility_check_tests, test_app_root_invalid_nonexec_type)
{
	/* Sample component:
	 *  - type: MEM
	 *  - processor: non-executable (-1)
	 *  - address: 0x80000
	 *  - size: 0x1000
	 */
	uint8_t test_mem_app[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',  0x41, 0x20, 0x45,
				  0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string test_id = {
		.value = test_mem_app,
		.len = sizeof(test_mem_app),
	};

	/* GIVEN  the sample compoenent memory is allowed to be modified */
	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;

	/* WHEN a sample MEM component, covering radio area is validated for application manifest */
	int err = suit_plat_component_compatibility_check(&app_root_cid, &test_id);

	/* THEN the component ID is denied... */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, err,
		      "Failed to block non-executable MEM component in root manifest");

	/* ... and the arbiter was not used to check for memory addresses */
	zassert_equal(arbiter_mem_access_check_fake.call_count, 0,
		      "Incorrect number of arbiter_mem_access_check() calls");
}
