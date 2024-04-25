/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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
#include <update_magic_values.h>

#if DT_NODE_EXISTS(DT_NODELABEL(cpuapp_suit_storage))
#define SUIT_STORAGE_OFFSET FIXED_PARTITION_OFFSET(cpuapp_suit_storage)
#define SUIT_STORAGE_SIZE   FIXED_PARTITION_SIZE(cpuapp_suit_storage)
#else
#define SUIT_STORAGE_OFFSET FIXED_PARTITION_OFFSET(suit_storage)
#define SUIT_STORAGE_SIZE   FIXED_PARTITION_SIZE(suit_storage)
#endif
#define SUIT_STORAGE_ADDRESS suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_OFFSET)

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

/* Valid application envelope */
extern const uint8_t manifest_valid_app_buf[];
extern const size_t manifest_valid_app_len;

/* Manifest generated using component id with size field set to zero */
extern const uint8_t manifest_zero_size_buf[];
extern const size_t manifest_zero_size_len;

/* Originally valid envelope with manipulated single byte */
extern const uint8_t manifest_manipulated_buf[];
extern const size_t manifest_manipulated_len;

/* Envelope generated with "UNSUPPORTED!" as a component type */
extern const uint8_t manifest_unsupported_component_buf[];
extern const size_t manifest_unsupported_component_len;

/* Envelope with manifest version set to 2 */
extern const uint8_t manifest_wrong_version_buf[];
extern const size_t manifest_wrong_version_len;

/* Envelope signed with a different private key */
extern const uint8_t manifest_different_key_buf[];
extern const size_t manifest_different_key_len;

/* Empty root envelope with unsupported command (invoke on CAND_MFST) inside install sequence */
extern const uint8_t manifest_root_unsupported_command_buf[];
extern const size_t manifest_root_unsupported_command_len;

/* Empty root envelope with install and failing candidate-verification sequences */
extern const uint8_t manifest_root_candidate_verification_fail_buf[];
extern const size_t manifest_root_candidate_verification_fail_len;

/* Empty root envelope with candidate-verification sequence */
extern const uint8_t manifest_root_candidate_verification_no_install_buf[];
extern const size_t manifest_root_candidate_verification_no_install_len;

/* Empty root envelope with candidate-verification and failing install sequences */
extern const uint8_t manifest_root_candidate_verification_install_fail_buf[];
extern const size_t manifest_root_candidate_verification_install_fail_len;

/* Empty root envelope with candidate-verification and install sequences */
extern const uint8_t manifest_root_candidate_verification_install_buf[];
extern const size_t manifest_root_candidate_verification_install_len;

static void setup_erased_flash(void)
{
	/* Execute SUIT storage init, so the MPI area is copied into a backup region. */
	int err = suit_storage_init();

	zassert_equal(SUIT_PLAT_SUCCESS, err, "Failed to init and backup suit storage (%d)", err);

	/* Clear the whole application area */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to erase storage area");

	err = flash_erase(fdev, SUIT_STORAGE_OFFSET, SUIT_STORAGE_SIZE);
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

	setup_erased_flash();

	int err = suit_storage_update_cand_set(update_candidate, ARRAY_SIZE(update_candidate));

	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "Unable to set update candidate before test execution (0x%x, %d)", buf, len);
}

static void assert_post_install_state(void)
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
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(), "Execution mode modified");
}

static void orchestrator_tests_cleanup(void *fixture)
{
	(void)fixture;
	suit_dfu_cache_deinitialize();
}

ZTEST_SUITE(orchestrator_update_tests, NULL, NULL, NULL, orchestrator_tests_cleanup, NULL);

ZTEST(orchestrator_update_tests, test_successful_update)
{
	const uint8_t *addr;
	size_t size;

	/* GIVEN valid update candidate in suit storage... */
	setup_update_candidate(manifest_valid_buf, manifest_valid_len);
	/* ... and suit orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN installation is performed successfully... */
	/* ... and orchestrator exits without an error... */
	zassert_equal(0, err, "Unexpected error code");
	/* ... and the envelope get installed... */
	err = suit_storage_installed_envelope_get(&supported_class_id, &addr, &size);
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The envelope was not installed after successful update");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_successful_update_and_boot)
{
	const uint8_t *addr_boot;
	size_t size_boot;
	const uint8_t *addr_update;
	size_t size_update;

	/* GIVEN valid update candidate in suit storage... */
	setup_update_candidate(manifest_valid_buf, manifest_valid_len);
	/* ... and suit orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized first time");
	/* ... and the execution mode is set to install mode... */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test preparation");
	/* ... and orchestrator is launched first time to perform successful update... */
	zassert_equal(0, suit_orchestrator_entry(), "Unsuccessful first orchestrator launch");
	/* ... and the bootable envelope is installed... */
	int err = suit_storage_installed_envelope_get(&supported_class_id, &addr_boot, &size_boot);

	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The envelope was not installed after successful update");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();

	/* WHEN when orchestrator is initialized again (reboot simulation)... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized second time");
	/* ... and the execution mode is set to invoke mode */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");
	/* ... and orchestrator is launched again */
	err = suit_orchestrator_entry();

	/* THEN installed SW is invoked successfully (orchestrator returns no error)... */
	zassert_equal(0, err, "Unexpected error code");
	/* ... and the envelope remains installed... */
	err = suit_storage_installed_envelope_get(&supported_class_id, &addr_update, &size_update);
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The envelope was not installed after successful update");
	/* .. and the same index in SUIT storage is used... */
	zassert_equal(addr_boot, addr_update,
		      "The envelope was not installed in the same SUIT storage slot");
	/* ... and the execution mode is set to post invoke mode */
	zassert_equal(EXECUTION_MODE_POST_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode after test execution");
}

ZTEST(orchestrator_update_tests, test_envelope_size_zero)
{
	/* GIVEN suit storage contains envelope with component id's size field set to zero... */
	setup_update_candidate(manifest_zero_size_buf, manifest_zero_size_len);
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Failed to setup test");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_manipulated_envelope)
{
	/* GIVEN suit storage contains envelope with one of its byte changed to different value...
	 */
	setup_update_candidate(manifest_manipulated_buf, manifest_manipulated_len);
	/* ... and suit orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_invalid_exec_mode)
{
	/* GIVEN suit storage does not indicate presence of update candidate... */
	setup_erased_flash();
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is forced to install mode */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_execution_mode_set(EXECUTION_MODE_INSTALL),
		      "Unable to override execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag remains cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_invalid_address_and_size)
{
	/* GIVEN suit storage indicates presence of update candidate... */
	/* ... but has empty address field...*/
	const uint8_t *buf = (const uint8_t *)UPDATE_MAGIC_VALUE_EMPTY;
	/* ... and has empty size field... */
	size_t len = (size_t)UPDATE_MAGIC_VALUE_EMPTY;

	setup_update_candidate(buf, len);

	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EFAULT, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_candidate_empty_size)
{
	/* GIVEN suit storage has update candidate flag set... */
	/* ...and some non-empty and non-zero address field...*/
	const uint8_t *buf = (const uint8_t *)0xDEADBEEF;
	/* ... but empty size field... */
	size_t len = (size_t)UPDATE_MAGIC_VALUE_EMPTY;

	setup_update_candidate(buf, len);

	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EFAULT, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_candidate_too_many_caches)
{
	suit_plat_mreg_t update_candidate[CONFIG_SUIT_STORAGE_N_UPDATE_REGIONS] = {0};

	for (size_t i = 0; i < ARRAY_SIZE(update_candidate); i++) {
		update_candidate[i].mem = (const uint8_t *)i;
		update_candidate[i].size = 0;
	}

	/* GIVEN suit storage has update candidate flag set... */
	setup_erased_flash();
	/* ...and contains a valid envelope address and size...*/
	update_candidate[0].mem = manifest_valid_buf;
	update_candidate[0].size = manifest_valid_len;

	int err = suit_storage_update_cand_set(update_candidate, ARRAY_SIZE(update_candidate));

	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "Unable to set update candidate before test execution (length: %d)",
		      ARRAY_SIZE(update_candidate));

	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EINVAL, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_unsupported_component)
{
	/* GIVEN suit storage contains update candidate with unsupported component... */
	setup_update_candidate(manifest_unsupported_component_buf,
			       manifest_unsupported_component_len);
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_wrong_manifest_version)
{
	/* GIVEN suit storage contains update candidate with invalid manifest version... */
	setup_update_candidate(manifest_wrong_version_buf, manifest_wrong_version_len);

	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_signed_with_different_key)
{
	/* GIVEN suit storage contains update candidate with invalid manifest version... */
	setup_update_candidate(manifest_different_key_buf, manifest_different_key_len);

	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_independent_updates_denied)
{
	/* GIVEN suit storage contains update candidate that cannot be independently updated... */
	setup_update_candidate(manifest_valid_app_buf, manifest_valid_app_len);

	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_unsupported_command)
{
	/* GIVEN suit storage contains update candidate with unsupported command... */
	setup_update_candidate(manifest_root_unsupported_command_buf,
			       manifest_root_unsupported_command_len);

	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_seq_cand_varification_fail)
{
	/* GIVEN suit storage contains update candidate with failing candidate-verification
	 * sequence...
	 */
	setup_update_candidate(manifest_root_candidate_verification_fail_buf,
			       manifest_root_candidate_verification_fail_len);

	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_seq_cand_varification_no_install)
{
	/* GIVEN suit storage contains update candidate without suit-install sequence... */
	setup_update_candidate(manifest_root_candidate_verification_no_install_buf,
			       manifest_root_candidate_verification_no_install_len);

	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_seq_cand_varification_install_fail)
{
	/* GIVEN suit storage contains update candidate with failing suit-install sequence... */
	setup_update_candidate(manifest_root_candidate_verification_install_fail_buf,
			       manifest_root_candidate_verification_install_fail_len);

	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator returns error code... */
	zassert_equal(-EACCES, err, "Unexpected error code");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}

ZTEST(orchestrator_update_tests, test_seq_cand_varification_install)
{
	const uint8_t *addr;
	size_t size;

	/* GIVEN suit storage contains update candidate and suit-install sequence... */
	setup_update_candidate(manifest_root_candidate_verification_install_buf,
			       manifest_root_candidate_verification_install_len);

	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(), "Orchestrator not initialized");
	/* ... and the execution mode is set to install mode */
	zassert_equal(EXECUTION_MODE_INSTALL, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is launched */
	int err = suit_orchestrator_entry();

	/* THEN installation is performed successfully... */
	/* ... and orchestrator exits without an error... */
	zassert_equal(0, err, "Unexpected error code");
	/* ... and the envelope get installed... */
	err = suit_storage_installed_envelope_get(&root_class_id, &addr, &size);
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "The envelope was not installed after successful update");
	/* ... and the candidate availability flag is cleared */
	assert_post_install_state();
}
