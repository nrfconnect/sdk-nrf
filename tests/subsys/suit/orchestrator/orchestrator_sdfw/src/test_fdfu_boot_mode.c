/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <suit_orchestrator.h>
#include <suit_storage.h>
#include <suit_execution_mode.h>

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
#define SUIT_STORAGE_ADDRESS suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_OFFSET)
#define ZEPHYR_FLASH_EB_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size)

/* RFC4122 uuid5(nordic_vid, 'test_sample_recovery')
 */
static const suit_manifest_class_id_t recovery_class_id = {{0x74, 0xa0, 0xc6, 0xe7, 0xa9, 0x2a,
							    0x56, 0x00, 0x9c, 0x5d, 0x30, 0xee,
							    0x87, 0x8b, 0x06, 0xba}};

/* Empty recovery envelope with invoke sequence */
extern const uint8_t manifest_recovery_no_validate_buf[];
extern const size_t manifest_recovery_no_validate_len;

/* Empty recovery envelope with invoke and failig validate sequences */
extern const uint8_t manifest_recovery_validate_fail_buf[];
extern const size_t manifest_recovery_validate_fail_len;

/* Empty recovery envelope with validate, invoke and failing load sequences */
extern const uint8_t manifest_recovery_validate_load_fail_buf[];
extern const size_t manifest_recovery_validate_load_fail_len;

/* Empty recovery envelope with validate and load sequences */
extern const uint8_t manifest_recovery_validate_load_no_invoke_buf[];
extern const size_t manifest_recovery_validate_load_no_invoke_len;

/* Empty recovery envelope with validate, load and failing invoke sequences */
extern const uint8_t manifest_recovery_validate_load_invoke_fail_buf[];
extern const size_t manifest_recovery_validate_load_invoke_fail_len;

/* Empty recovery envelope with validate, load and invoke sequences */
extern const uint8_t manifest_recovery_validate_load_invoke_buf[];
extern const size_t manifest_recovery_validate_load_invoke_len;

/* Valid recovery envelope */
extern uint8_t manifest_valid_recovery_buf[];
extern size_t manifest_valid_recovery_len;

/* Originally valid recovery envelope with manipulated single byte */
extern const uint8_t manifest_manipulated_recovery_buf[];
extern const size_t manifest_manipulated_recovery_len;

/* Random bytes, attached to the envelopes as FW. */
extern const uint8_t sample_fw_buf[];
extern const size_t sample_fw_len;
extern const uintptr_t sample_recovery_fw_address;

static void *setup_install_recovery_fw(void)
{
	int err = 0;
	uint8_t *sample_recovery_fw_ptr = suit_plat_mem_ptr_get(sample_recovery_fw_address);
	uintptr_t sample_recovery_fw_offset = suit_plat_mem_nvm_offset_get(sample_recovery_fw_ptr);

	/* Clear the whole application area */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to erase storage area");

	if (sample_fw_len > ZEPHYR_FLASH_EB_SIZE) {
		err = flash_erase(fdev, sample_recovery_fw_offset, sample_fw_len);
	} else {
		err = flash_erase(fdev, sample_recovery_fw_offset, ZEPHYR_FLASH_EB_SIZE);
	}
	zassert_equal(0, err, "Unable to erase test FW area before test execution");

	err = flash_write(fdev, sample_recovery_fw_offset, sample_fw_buf, sample_fw_len);
	zassert_equal(0, err, "Unable to write test FW area before test execution");

	return NULL;
}

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

	suit_plat_err_t ret = suit_storage_flags_clear(SUIT_FLAG_RECOVERY);

	zassert_equal(SUIT_PLAT_SUCCESS, ret,
		      "Unable to clear recovery flag before test execution");

	ret = suit_storage_flags_clear(SUIT_FLAG_FOREGROUND_DFU);
	zassert_equal(SUIT_PLAT_SUCCESS, ret,
		      "Unable to clear foreground DFU flag before test execution");

	/* Recover MPI area from the backup region. */
	err = suit_storage_init();
	zassert_equal(SUIT_PLAT_SUCCESS, err, "Failed to recover MPI area from backup (%d)", err);
}

static void setup_install_envelope(const suit_manifest_class_id_t *class_id, const uint8_t *buf,
				   size_t len)
{
	suit_plat_err_t err = suit_storage_install_envelope(class_id, (uint8_t *)buf, len);

	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "Unable to install envelope before test execution (0x%x, %d)", buf, len);
}

static void setup_fdfu_flag(void)
{
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_flags_set(SUIT_FLAG_FOREGROUND_DFU),
		      "Unable to set foreground DFU flag before test execution");
}

ZTEST_SUITE(orchestrator_fdfu_boot_tests, NULL, setup_install_recovery_fw, NULL, NULL, NULL);

ZTEST(orchestrator_fdfu_boot_tests, test_fdfu_no_recovery_envelope)
{
	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and update candidate flag is not set... */
	/* ... and foreground DFU flag is set */
	setup_fdfu_flag();
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to foreground DFU boot */
	zassert_equal(EXECUTION_MODE_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (hard)... */
	zassert_equal(-ENOENT, err, "Orchestrator did not fail");
	/* ... and the foreground DFU flag is set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_flags_check(SUIT_FLAG_RECOVERY),
		      "Recovery flag set");
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_flags_check(SUIT_FLAG_FOREGROUND_DFU),
		      "Foreground DFU flag not set");
	/* ... and the execution mode is set to the FAIL INVOKE FDFU */
	zassert_equal(EXECUTION_MODE_FAIL_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Execution mode not changed to the FAIL INVOKE FDFU");
	/* ... and execution mode indicates a failed state */
	zassert_equal(true, suit_execution_mode_failed(), "The device does not enter failed mode");
	/* ... and execution mode does not indicate boot mode */
	zassert_equal(false, suit_execution_mode_booting(), "The device did not left boot mode");
	/* ... and execution mode does not indicate update mode */
	zassert_equal(false, suit_execution_mode_updating(), "The device entered update mode");
}

ZTEST(orchestrator_fdfu_boot_tests, test_fdfu_invalid_recovery_envelope)
{
	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and recovery envelope is installed... */
	setup_install_envelope(&recovery_class_id, manifest_manipulated_recovery_buf,
			       manifest_manipulated_recovery_len);
	/* ... and update candidate flag is not set... */
	/* ... and foreground DFU flag is set */
	setup_fdfu_flag();
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to foreground DFU boot */
	zassert_equal(EXECUTION_MODE_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (hard)... */
	zassert_equal(-ENOEXEC, err, "Orchestrator did not fail");
	/* ... and the foreground DFU flag is set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_flags_check(SUIT_FLAG_RECOVERY),
		      "Recovery flag set");
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_flags_check(SUIT_FLAG_FOREGROUND_DFU),
		      "Foreground DFU flag not set");
	/* ... and the execution mode is set to the FAIL INVOKE FDFU */
	zassert_equal(EXECUTION_MODE_FAIL_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Execution mode not changed to the FAIL INVOKE FDFU");
}

ZTEST(orchestrator_fdfu_boot_tests, test_fdfu_valid_recovery_envelope)
{
	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and recovery envelope is installed... */
	setup_install_envelope(&recovery_class_id, manifest_valid_recovery_buf,
			       manifest_valid_recovery_len);
	/* ... and update candidate flag is not set... */
	/* ... and foreground DFU flag is set */
	setup_fdfu_flag();
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to foreground DFU boot */
	zassert_equal(EXECUTION_MODE_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator succeeds... */
	zassert_equal(0, err, "Orchestrator not initialized");
	/* ... and the foreground DFU flag is set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_flags_check(SUIT_FLAG_RECOVERY),
		      "Recovery flag set");
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_flags_check(SUIT_FLAG_FOREGROUND_DFU),
		      "Foreground DFU flag not set");
	/* ... and the execution mode is set to the POST INVOKE FDFU */
	zassert_equal(EXECUTION_MODE_POST_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Execution mode not changed to the POST INVOKE FDFU");
	/* ... and execution mode does not indicate a failed state */
	zassert_equal(false, suit_execution_mode_failed(), "The device entered failed mode");
	/* ... and execution mode does not indicate boot mode */
	zassert_equal(false, suit_execution_mode_booting(), "The device did not left boot mode");
	/* ... and execution mode does not indicate update mode */
	zassert_equal(false, suit_execution_mode_updating(), "The device entered update mode");
}

ZTEST(orchestrator_fdfu_boot_tests, test_fdfu_seq_no_validate)
{
	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and recovery envelope is installed... */
	setup_install_envelope(&recovery_class_id, manifest_recovery_no_validate_buf,
			       manifest_recovery_no_validate_len);
	/* ... and update candidate flag is not set... */
	/* ... and foreground DFU flag is set */
	setup_fdfu_flag();
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to foreground DFU boot */
	zassert_equal(EXECUTION_MODE_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator succeeds... */
	zassert_equal(0, err, "Envelope without validate sequence not accepted (err: %d)", err);
	/* ... and the foreground DFU flag is set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_flags_check(SUIT_FLAG_RECOVERY),
		      "Recovery flag set");
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_flags_check(SUIT_FLAG_FOREGROUND_DFU),
		      "Foreground DFU flag not set");
	/* ... and the execution mode is set to the POST INVOKE RECOVERY */
	zassert_equal(EXECUTION_MODE_POST_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Execution mode not changed to the POST INVOKE FDFU");
}

ZTEST(orchestrator_fdfu_boot_tests, test_fdfu_seq_validate_fail)
{
	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and recovery envelope with failing suit-validate sequence is installed... */
	setup_install_envelope(&recovery_class_id, manifest_recovery_validate_fail_buf,
			       manifest_recovery_validate_fail_len);
	/* ... and update candidate flag is not set... */
	/* ... and foreground DFU flag is set */
	setup_fdfu_flag();
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to foreground DFU boot */
	zassert_equal(EXECUTION_MODE_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (hard)... */
	zassert_equal(-EILSEQ, err, "Orchestrator did not fail");
	/* ... and the foreground DFU flag is set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_flags_check(SUIT_FLAG_RECOVERY),
		      "Recovery flag set");
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_flags_check(SUIT_FLAG_FOREGROUND_DFU),
		      "Foreground DFU flag not set");
	/* ... and the execution mode is set to the FAIL INVOKE FDFU */
	zassert_equal(EXECUTION_MODE_FAIL_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Execution mode not changed to the FAIL INVOKE FDFU");
}

ZTEST(orchestrator_fdfu_boot_tests, test_fdfu_seq_validate_load_fail)
{
	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and recovery envelope with failing suit-load sequence is installed... */
	setup_install_envelope(&recovery_class_id, manifest_recovery_validate_load_fail_buf,
			       manifest_recovery_validate_load_fail_len);
	/* ... and update candidate flag is not set... */
	/* ... and foreground DFU flag is set */
	setup_fdfu_flag();
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to foreground DFU boot */
	zassert_equal(EXECUTION_MODE_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (hard)... */
	zassert_equal(-EILSEQ, err, "Orchestrator did not fail");
	/* ... and the foreground DFU flag is set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_flags_check(SUIT_FLAG_RECOVERY),
		      "Recovery flag set");
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_flags_check(SUIT_FLAG_FOREGROUND_DFU),
		      "Foreground DFU flag not set");
	/* ... and the execution mode is set to the FAIL INVOKE FDFU */
	zassert_equal(EXECUTION_MODE_FAIL_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Execution mode not changed to the FAIL INVOKE FDFU");
}

ZTEST(orchestrator_fdfu_boot_tests, test_fdfu_seq_validate_load_no_invoke)
{
	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and recovery envelope without suit-invoke sequence is installed... */
	setup_install_envelope(&recovery_class_id, manifest_recovery_validate_load_no_invoke_buf,
			       manifest_recovery_validate_load_no_invoke_len);
	/* ... and update candidate flag is not set... */
	/* ... and foreground DFU flag is set */
	setup_fdfu_flag();
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to foreground DFU boot */
	zassert_equal(EXECUTION_MODE_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (hard)... */
	zassert_equal(-EILSEQ, err, "Orchestrator did not fail");
	/* ... and the foreground DFU flag is set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_flags_check(SUIT_FLAG_RECOVERY),
		      "Recovery flag set");
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_flags_check(SUIT_FLAG_FOREGROUND_DFU),
		      "Foreground DFU flag not set");
	/* ... and the execution mode is set to the FAIL INVOKE FDFU */
	zassert_equal(EXECUTION_MODE_FAIL_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Execution mode not changed to the FAIL INVOKE FDFU");
}

ZTEST(orchestrator_fdfu_boot_tests, test_fdfu_seq_validate_load_invoke_fail)
{
	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and recovery envelope with failing suit-invoke sequence is installed... */
	setup_install_envelope(&recovery_class_id, manifest_recovery_validate_load_invoke_fail_buf,
			       manifest_recovery_validate_load_invoke_fail_len);
	/* ... and update candidate flag is not set... */
	/* ... and foreground DFU flag is set */
	setup_fdfu_flag();
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to foreground DFU boot */
	zassert_equal(EXECUTION_MODE_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (hard)... */
	zassert_equal(-EILSEQ, err, "Orchestrator did not fail");
	/* ... and the foreground DFU flag is set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_flags_check(SUIT_FLAG_RECOVERY),
		      "Recovery flag set");
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_flags_check(SUIT_FLAG_FOREGROUND_DFU),
		      "Foreground DFU flag not set");
	/* ... and the execution mode is set to the FAIL INVOKE FDFU */
	zassert_equal(EXECUTION_MODE_FAIL_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Execution mode not changed to the FAIL INVOKE FDFU");
}

ZTEST(orchestrator_fdfu_boot_tests, test_fdfu_seq_validate_load_invoke)
{
	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and recovery envelope without dependencies is installed... */
	setup_install_envelope(&recovery_class_id, manifest_recovery_validate_load_invoke_buf,
			       manifest_recovery_validate_load_invoke_len);
	/* ... and update candidate flag is not set... */
	/* ... and foreground DFU flag is set */
	setup_fdfu_flag();
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to foreground DFU boot */
	zassert_equal(EXECUTION_MODE_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails succeeds... */
	zassert_equal(0, err, "Orchestrator not initialized");
	/* ... and the foreground DFU flag is set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_flags_check(SUIT_FLAG_RECOVERY),
		      "Recovery flag set");
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_flags_check(SUIT_FLAG_FOREGROUND_DFU),
		      "Foreground DFU flag not set");
	/* ... and the execution mode is set to the POST INVOKE FDFU */
	zassert_equal(EXECUTION_MODE_POST_INVOKE_FOREGROUND_DFU, suit_execution_mode_get(),
		      "Execution mode not changed to the POST INVOKE FDFU");
}
