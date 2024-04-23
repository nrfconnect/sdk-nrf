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

/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
static const suit_manifest_class_id_t root_class_id = {{0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59,
							0xa1, 0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1,
							0x4b, 0x0a}};

/* RFC4122 uuid5(nordic_vid, 'test_sample_app') */
static const suit_manifest_class_id_t app_class_id = {{0x5b, 0x46, 0x9f, 0xd1, 0x90, 0xee, 0x53,
						       0x9c, 0xa3, 0x18, 0x68, 0x1b, 0x03, 0x69,
						       0x5e, 0x36}};

/* Empty root envelope with invoke sequence */
extern const uint8_t manifest_root_no_validate_buf[];
extern const size_t manifest_root_no_validate_len;

/* Empty root envelope with invoke and failig validate sequences */
extern const uint8_t manifest_root_validate_fail_buf[];
extern const size_t manifest_root_validate_fail_len;

/* Empty root envelope with validate, invoke and failing load sequences */
extern const uint8_t manifest_root_validate_load_fail_buf[];
extern const size_t manifest_root_validate_load_fail_len;

/* Empty root envelope with validate and load sequences */
extern const uint8_t manifest_root_validate_load_no_invoke_buf[];
extern const size_t manifest_root_validate_load_no_invoke_len;

/* Empty root envelope with validate, load and failing invoke sequences */
extern const uint8_t manifest_root_validate_load_invoke_fail_buf[];
extern const size_t manifest_root_validate_load_invoke_fail_len;

/* Empty root envelope with validate, load and invoke sequences */
extern const uint8_t manifest_root_validate_load_invoke_buf[];
extern const size_t manifest_root_validate_load_invoke_len;

/* Valid root envelope */
extern const uint8_t manifest_valid_buf[];
extern const size_t manifest_valid_len;

/* Valid application envelope */
extern const uint8_t manifest_valid_app_buf[];
extern const size_t manifest_valid_app_len;

/* Originally valid envelope with manipulated single byte */
extern const uint8_t manifest_manipulated_buf[];
extern const size_t manifest_manipulated_len;

/* Random bytes, attached to the envelopes as FW. */
extern const uint8_t sample_fw_buf[];
extern const size_t sample_fw_len;
extern const uintptr_t sample_fw_address;

static void *setup_install_fw(void)
{
	int err = 0;
	uint8_t *sample_fw_ptr = suit_plat_mem_ptr_get(sample_fw_address);
	uintptr_t sample_fw_offset = suit_plat_mem_nvm_offset_get(sample_fw_ptr);

	/* Clear the whole application area */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to erase storage area");

	if (sample_fw_len > ZEPHYR_FLASH_EB_SIZE) {
		err = flash_erase(fdev, sample_fw_offset, sample_fw_len);
	} else {
		err = flash_erase(fdev, sample_fw_offset, ZEPHYR_FLASH_EB_SIZE);
	}
	zassert_equal(0, err, "Unable to erase test FW area before test execution");

	err = flash_write(fdev, sample_fw_offset, sample_fw_buf, sample_fw_len);
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

	suit_plat_err_t ret = suit_storage_report_clear(0);

	zassert_equal(SUIT_PLAT_SUCCESS, ret,
		      "Unable to clear recovery flag before test execution");

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

ZTEST_SUITE(orchestrator_boot_tests, NULL, setup_install_fw, NULL, NULL, NULL);

ZTEST(orchestrator_boot_tests, test_invalid_exec_mode)
{
	const uint8_t *buf;
	size_t len;

	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Unexpected emergency recovery flag before test execution");
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to post mode */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_execution_mode_set(EXECUTION_MODE_POST_INVOKE),
		      "Unable to override execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails... */
	zassert_equal(-EINVAL, err, "Orchestrator did not fail");
	/* ... and the execution mode remains unchanged */
	zassert_equal(EXECUTION_MODE_POST_INVOKE, suit_execution_mode_get(),
		      "Execution mode modified");
}

ZTEST(orchestrator_boot_tests, test_no_root_envelope)
{
	const uint8_t *buf;
	size_t len;

	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Unexpected emergency recovery flag before test execution");
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to regular boot */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (no support for reboots)... */
	zassert_equal(-ENOTSUP, err, "Orchestrator did not fail");
	/* ... and the emergency flag is set... */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_report_read(0, &buf, &len),
		      "Emergency flag not set");
	/* ... and the execution mode remains unchanged */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(), "Execution mode modified");
}

ZTEST(orchestrator_boot_tests, test_invalid_root_envelope)
{
	const uint8_t *buf;
	size_t len;

	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and root envelope is installed... */
	setup_install_envelope(&root_class_id, manifest_manipulated_buf, manifest_manipulated_len);
	/* ... and application envelope is installed... */
	setup_install_envelope(&app_class_id, manifest_valid_app_buf, manifest_valid_app_len);
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Unexpected emergency recovery flag before test execution");
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to regular boot */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (no support for reboots)... */
	zassert_equal(-ENOTSUP, err, "Orchestrator did not fail");
	/* ... and the emergency flag is set... */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_report_read(0, &buf, &len),
		      "Emergency flag not set");
	/* ... and the execution mode remains unchanged */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(), "Execution mode modified");
}

ZTEST(orchestrator_boot_tests, test_valid_root_without_app_envelope)
{
	const uint8_t *buf;
	size_t len;

	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and root envelope is installed... */
	setup_install_envelope(&root_class_id, manifest_valid_buf, manifest_valid_len);
	/* ... and application envelope is not installed... */
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Unexpected emergency recovery flag before test execution");
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to regular boot */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (no support for reboots)... */
	zassert_equal(-ENOTSUP, err, "Orchestrator did not fail");
	/* ... and the emergency flag is set... */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_report_read(0, &buf, &len),
		      "Emergency flag not set");
	/* ... and the execution mode remains unchanged */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(), "Execution mode modified");
}

ZTEST(orchestrator_boot_tests, test_valid_root_app_envelope)
{
	const uint8_t *buf;
	size_t len;

	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and root envelope is installed... */
	setup_install_envelope(&root_class_id, manifest_valid_buf, manifest_valid_len);
	/* ... and application envelope is installed... */
	setup_install_envelope(&app_class_id, manifest_valid_app_buf, manifest_valid_app_len);
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Unexpected emergency recovery flag before test execution");
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to regular boot */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator succeeds... */
	zassert_equal(0, err, "Orchestrator not initialized");
	/* ... and the emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Emergency flag set");
	/* ... and the execution mode is set to the POST INVOKE */
	zassert_equal(EXECUTION_MODE_POST_INVOKE, suit_execution_mode_get(),
		      "Execution mode modified");
}

ZTEST(orchestrator_boot_tests, test_seq_no_validate)
{
	const uint8_t *buf;
	size_t len;

	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and root envelope without suit-validate sequence is installed... */
	setup_install_envelope(&root_class_id, manifest_root_no_validate_buf,
			       manifest_root_no_validate_len);
	/* ... and application envelope is not installed... */
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Unexpected emergency recovery flag before test execution");
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to regular boot */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (no support for reboots)... */
	zassert_equal(-ENOTSUP, err, "Orchestrator did not fail");
	/* ... and the emergency flag is set... */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_report_read(0, &buf, &len),
		      "Emergency flag not set");
	/* ... and the execution mode remains unchanged */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(), "Execution mode modified");
}

ZTEST(orchestrator_boot_tests, test_seq_validate_fail)
{
	const uint8_t *buf;
	size_t len;

	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and root envelope with failing suit-validate sequence is installed... */
	setup_install_envelope(&root_class_id, manifest_root_validate_fail_buf,
			       manifest_root_validate_fail_len);
	/* ... and application envelope is not installed... */
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Unexpected emergency recovery flag before test execution");
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to regular boot */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (no support for reboots)... */
	zassert_equal(-ENOTSUP, err, "Orchestrator did not fail");
	/* ... and the emergency flag is set... */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_report_read(0, &buf, &len),
		      "Emergency flag not set");
	/* ... and the execution mode remains unchanged */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(), "Execution mode modified");
}

ZTEST(orchestrator_boot_tests, test_seq_validate_load_fail)
{
	const uint8_t *buf;
	size_t len;

	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and root envelope with failing suit-load sequence is installed... */
	setup_install_envelope(&root_class_id, manifest_root_validate_load_fail_buf,
			       manifest_root_validate_load_fail_len);
	/* ... and application envelope is not installed... */
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Unexpected emergency recovery flag before test execution");
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to regular boot */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (no support for reboots)... */
	zassert_equal(-ENOTSUP, err, "Orchestrator did not fail");
	/* ... and the emergency flag is set... */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_report_read(0, &buf, &len),
		      "Emergency flag not set");
	/* ... and the execution mode remains unchanged */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(), "Execution mode modified");
}

ZTEST(orchestrator_boot_tests, test_seq_validate_load_no_invoke)
{
	const uint8_t *buf;
	size_t len;

	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and root envelope without suit-invoke sequence is installed... */
	setup_install_envelope(&root_class_id, manifest_root_validate_load_no_invoke_buf,
			       manifest_root_validate_load_no_invoke_len);
	/* ... and application envelope is not installed... */
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Unexpected emergency recovery flag before test execution");
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to regular boot */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (no support for reboots)... */
	zassert_equal(-ENOTSUP, err, "Orchestrator did not fail");
	/* ... and the emergency flag is set... */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_report_read(0, &buf, &len),
		      "Emergency flag not set");
	/* ... and the execution mode remains unchanged */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(), "Execution mode modified");
}

ZTEST(orchestrator_boot_tests, test_seq_validate_load_invoke_fail)
{
	const uint8_t *buf;
	size_t len;

	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and root envelope with failing suit-invoke sequence is installed... */
	setup_install_envelope(&root_class_id, manifest_root_validate_load_invoke_fail_buf,
			       manifest_root_validate_load_invoke_fail_len);
	/* ... and application envelope is not installed... */
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Unexpected emergency recovery flag before test execution");
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to regular boot */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (no support for reboots)... */
	zassert_equal(-ENOTSUP, err, "Orchestrator did not fail");
	/* ... and the emergency flag is set... */
	zassert_equal(SUIT_PLAT_SUCCESS, suit_storage_report_read(0, &buf, &len),
		      "Emergency flag not set");
	/* ... and the execution mode remains unchanged */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(), "Execution mode modified");
}

ZTEST(orchestrator_boot_tests, test_seq_validate_load_invoke)
{
	const uint8_t *buf;
	size_t len;

	/* GIVEN empty flash (suit storage is erased)... */
	setup_erased_flash();
	/* ... and root envelope without dependencies is installed... */
	setup_install_envelope(&root_class_id, manifest_root_validate_load_invoke_buf,
			       manifest_root_validate_load_invoke_len);
	/* ... and application envelope is not installed... */
	/* ... and update candidate flag is not set... */
	/* ... and emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Unexpected emergency recovery flag before test execution");
	/* ... and orchestrator is initialized... */
	zassert_equal(0, suit_orchestrator_init(),
		      "Orchestrator not initialized before test execution");
	/* ... and the execution mode is set to regular boot */
	zassert_equal(EXECUTION_MODE_INVOKE, suit_execution_mode_get(),
		      "Unexpected execution mode before test execution");

	/* WHEN orchestrator is executed */
	int err = suit_orchestrator_entry();

	/* THEN orchestrator fails (no support for reboots)... */
	zassert_equal(0, err, "Orchestrator not initialized");
	/* ... and the emergency flag is not set... */
	zassert_equal(SUIT_PLAT_ERR_NOT_FOUND, suit_storage_report_read(0, &buf, &len),
		      "Emergency flag not set");
	/* ... and the execution mode is set to the POST INVOKE */
	zassert_equal(EXECUTION_MODE_POST_INVOKE, suit_execution_mode_get(),
		      "Execution mode modified");
}
