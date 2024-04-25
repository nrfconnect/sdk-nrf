/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <suit_orchestrator.h>
#include <suit_storage.h>
#include <suit_execution_mode.h>
#include <suit_dfu_cache.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <suit_plat_mem_util.h>

#if DT_NODE_EXISTS(DT_NODELABEL(cpuapp_suit_storage))
#define SUIT_STORAGE_OFFSET FIXED_PARTITION_OFFSET(cpuapp_suit_storage)
#define SUIT_STORAGE_SIZE   FIXED_PARTITION_SIZE(cpuapp_suit_storage)
#else
#define SUIT_STORAGE_OFFSET FIXED_PARTITION_OFFSET(suit_storage)
#define SUIT_STORAGE_SIZE   FIXED_PARTITION_SIZE(suit_storage)
#endif

/* RFC4122 uuid5(nordic_vid, 'test_sample_app') */
static const suit_manifest_class_id_t supported_class_id = {{0x5b, 0x46, 0x9f, 0xd1, 0x90, 0xee,
							     0x53, 0x9c, 0xa3, 0x18, 0x68, 0x1b,
							     0x03, 0x69, 0x5e, 0x36}};

/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
static const suit_manifest_class_id_t root_class_id = {{0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59,
							0xa1, 0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1,
							0x4b, 0x0a}};

/* Valid root envelope */
extern const uint8_t manifest_valid_buf[];
extern const size_t manifest_valid_len;

/* Valid recovery envelope */
extern const uint8_t manifest_valid_recovery_buf[];
extern const size_t manifest_valid_recovery_len;

static void setup_erased_flash(void)
{
	/* Clear the whole application area */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to erase storage area");

	int err = flash_erase(fdev, SUIT_STORAGE_OFFSET, SUIT_STORAGE_SIZE);

	zassert_equal(0, err, "Unable to erase storage before test execution");

	suit_plat_err_t ret = suit_storage_report_clear(0);

	zassert_equal(SUIT_PLAT_SUCCESS, ret,
		      "Unable to clear recovery flag before test execution");

	/* Recover MPI area from the backup region. */
	err = suit_storage_init();
	zassert_equal(SUIT_PLAT_SUCCESS, err, "Failed to recover MPI area from backup (%d)", err);
}

static void setup_update_candidate(const uint8_t *buf, size_t len)
{
	zassert_not_null(buf, "NULL buf");

	suit_plat_mreg_t update_candidate[1] = {{
		.mem = buf,
		.size = len,
	}};

	int err = suit_storage_update_cand_set(update_candidate, ARRAY_SIZE(update_candidate));

	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "Unable to set update candidate before test execution (0x%x, %d)", buf, len);
}

static void assert_post_recovery_install_state(void)
{
	const suit_plat_mreg_t *regions = NULL;
	const uint8_t *buf = NULL;
	size_t len = 0;

	/* Each install attempt must at the end: */
	/* - clear the candidate availability flag */
	suit_plat_err_t ret = suit_storage_update_cand_get(&regions, &len);

	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, ret, "Update candidate presence not cleared");
	/* - do not modify the emergency flag */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Emergency flag changed");
	/* - do not modify the execution mode */
	zassert_equal(EXECUTION_MODE_INSTALL_RECOVERY, suit_execution_mode_get(),
		      "Execution mode modified");
}

static void assert_post_recovery_install_failed_state(void)
{
	const suit_plat_mreg_t *regions = NULL;
	const uint8_t *buf = NULL;
	size_t len = 0;

	/* Each install attempt must at the end: */
	/* - clear the candidate availability flag */
	suit_plat_err_t ret = suit_storage_update_cand_get(&regions, &len);

	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, ret, "Update candidate presence not cleared");
	/* - do not modify the emergency flag */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_report_read(0, &buf, &len),
		      "Emergency flag changed");
	/* - do not modify the execution mode */
	zassert_equal(EXECUTION_MODE_INSTALL_RECOVERY, suit_execution_mode_get(),
		      "Execution mode modified");
}

static void orchestrator_tests_cleanup(void *fixture)
{
	(void)fixture;
	suit_dfu_cache_deinitialize();
}

static void enter_recovery_mode(void *fixture)
{
	int err = suit_storage_init();

	zassert_equal(SUIT_PLAT_SUCCESS, err, "Failed to init and backup suit storage (%d)", err);

	setup_erased_flash();

	suit_plat_err_t ret = suit_storage_report_save(0, NULL, 0);

	zassert_equal(SUIT_PLAT_SUCCESS, ret,
		      "Unable to set boot report (emergency flag) before test execution");
}

ZTEST_SUITE(orchestrator_recovery_update_tests, NULL, NULL, enter_recovery_mode,
	    orchestrator_tests_cleanup, NULL);

ZTEST(orchestrator_recovery_update_tests, test_successful_update)
{
	const uint8_t *addr;
	size_t size;

	/* GIVEN valid update candidate in suit storage... */
	setup_update_candidate(manifest_valid_buf, manifest_valid_len);
	/* ... and suit orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install recovery mode */
	zassert_equal(EXECUTION_MODE_INSTALL_RECOVERY, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN installation is performed successfully... */
	/* ... and orchestrator exits without an error... */
	zassert_equal(0, err, "Unexpected error code");
	/* ... and the application envelope get installed... */
	err = suit_storage_installed_envelope_get(&supported_class_id, &addr, &size);
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The application envelope was not installed after successful update");
	/* ... and the root envelope get installed... */
	err = suit_storage_installed_envelope_get(&root_class_id, &addr, &size);
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The root envelope was not installed after successful update");
	/* ... and the candidate availability flag is cleared */
	assert_post_recovery_install_state();
}

ZTEST(orchestrator_recovery_update_tests, test_successful_update_and_boot)
{
	const uint8_t *addr_boot;
	size_t size_boot;
	const uint8_t *addr_update;
	size_t size_update;

	/* GIVEN valid update candidate in suit storage... */
	setup_update_candidate(manifest_valid_buf, manifest_valid_len);
	/* ... and suit orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized first time");
	/* ... and the execution mode is set to install recovery mode... */
	zassert_equal(EXECUTION_MODE_INSTALL_RECOVERY, suit_execution_mode_get(),
		      "Unexpected execution mode before test preparation");
	/* ... and orchestrator is launched first time to perform successful update... */
	zassert_equal(0, suit_orchestrator_entry(), "Unsuccessful first orchestrator launch");
	/* ... and the root envelope is installed... */
	int err = suit_storage_installed_envelope_get(&root_class_id, &addr_boot, &size_boot);

	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The root envelope was not installed after successful update");
	/* ... and the application envelope is installed... */
	err = suit_storage_installed_envelope_get(&supported_class_id, &addr_boot, &size_boot);
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The application envelope was not installed after successful update");
	/* ... and the candidate availability flag is cleared */
	assert_post_recovery_install_state();

	/* WHEN when orchestrator is initialized again (reboot simulation)... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized second time");
	/* ... and the execution mode is set to invoke mode */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");
	/* ... and orchestrator is launched again */
	err = suit_orchestrator_entry();

	/* THEN installed SW is invoked successfully (orchestrator returns no error)... */
	zassert_equal(0, err, "Unexpected error code");
	/* ... and the root envelope remains installed... */
	err = suit_storage_installed_envelope_get(&root_class_id, &addr_update, &size_update);
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The root envelope was not installed after successful update");
	/* ... and the envelope remains installed... */
	err = suit_storage_installed_envelope_get(&supported_class_id, &addr_update, &size_update);
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The application envelope was not installed after successful update");
	/* .. and the same index in SUIT storage is used... */
	zassert_equal(addr_boot, addr_update,
		      "The envelope was not installed in the same SUIT storage slot");
	/* ... and the execution mode is set to post invoke mode */
	zassert_equal(EXECUTION_MODE_POST_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode after test execution");
}

ZTEST(orchestrator_recovery_update_tests, test_invalid_exec_mode)
{
	/* GIVEN suit storage does not indicate presence of update candidate... */
	setup_erased_flash();
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is forced to install recovery mode */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_execution_mode_set(EXECUTION_MODE_INSTALL_RECOVERY),
		      "Unable to override execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag remains cleared */
	assert_post_recovery_install_state();
}

ZTEST(orchestrator_recovery_update_tests, test_independent_updates_denied)
{
	/* GIVEN suit storage contains recovery FW update... */
	setup_update_candidate(manifest_valid_recovery_buf, manifest_valid_recovery_len);

	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install recovery mode */
	zassert_equal(EXECUTION_MODE_INSTALL_RECOVERY, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_recovery_install_failed_state();
}
