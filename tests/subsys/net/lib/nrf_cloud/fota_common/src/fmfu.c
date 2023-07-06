/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "main.c"

/* Verify that nrf_cloud_fota_fmfu_dev_set returns failure when the function parameter is NULL. */
ZTEST(nrf_cloud_fota_common_test, test_00_nrf_cloud_fota_fmfu_dev_set_null_fdev)
{
	Z_TEST_SKIP_IFDEF(CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION);

	int ret = nrf_cloud_fota_fmfu_dev_set(NULL);

	zassert_equal(-EINVAL, ret, "return should be -EINVAL when NULL parameter is provided");
}

/* Verify that nrf_cloud_fota_fmfu_dev_set returns failure when the flash device is NULL
 * and CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION=n
 */
ZTEST(nrf_cloud_fota_common_test, test_01_nrf_cloud_fota_fmfu_dev_set_null_device)
{
	Z_TEST_SKIP_IFDEF(CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION);

	/* Provide a NULL flash device */
	struct dfu_target_fmfu_fdev fmfu_dev_inf = { .dev = NULL };
	int ret = nrf_cloud_fota_fmfu_dev_set(&fmfu_dev_inf);

	zassert_equal(-ENODEV, ret, "return should be -ENODEV when NULL device is provided");
}

/* Verify that nrf_cloud_fota_fmfu_dev_set returns failure when the flash device is not ready
 * and CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION=n
 */
ZTEST(nrf_cloud_fota_common_test, test_02_nrf_cloud_fota_fmfu_dev_set_fail_dev_not_ready)
{
	Z_TEST_SKIP_IFDEF(CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION);

	int ret;

	/* Provide a fake flash device that is not ready */
	struct device_state state = { .init_res = -1, .initialized = false };
	struct device flash_dev = { .state = &state };
	struct dfu_target_fmfu_fdev fmfu_dev_inf = { .dev = &flash_dev };

	dfu_target_full_modem_cfg_fake.custom_fake =
		fake_dfu_target_full_modem_cfg__succeeds;
	dfu_target_full_modem_fdev_get_fake.custom_fake =
		fake_dfu_target_full_modem_fdev_get__succeeds;

	ret = nrf_cloud_fota_fmfu_dev_set(&fmfu_dev_inf);
	zassert_not_equal(0, ret,
			  "return should not be 0 when flash device is not ready");
}

/* Verify that nrf_cloud_fota_fmfu_dev_set returns failure when dfu_target_full_modem_cfg fails. */
ZTEST(nrf_cloud_fota_common_test, test_03_nrf_cloud_fota_fmfu_dev_set_cfg_fail)
{
	int ret;

	/* Provide a fake flash device in the ready state */
	struct device_state state = { .init_res = 0, .initialized = true };
	struct device flash_dev = { .state = &state };
	struct dfu_target_fmfu_fdev fmfu_dev_inf = { .dev = &flash_dev };

	dfu_target_full_modem_cfg_fake.custom_fake =
		fake_dfu_target_full_modem_cfg__fails;
	dfu_target_full_modem_fdev_get_fake.custom_fake =
		fake_dfu_target_full_modem_fdev_get__succeeds;

	ret = nrf_cloud_fota_fmfu_dev_set(&fmfu_dev_inf);
	zassert_not_equal(0, ret, "return should not be 0 when dfu_target_full_modem_cfg fails");
}

/* Verify that nrf_cloud_fota_fmfu_dev_set returns failure when dfu_target_full_modem_fdev_get
 * fails.
 */
ZTEST(nrf_cloud_fota_common_test, test_04_nrf_cloud_fota_fmfu_dev_set_fail_fdev_get_fail)
{
	int ret;

	/* Provide a fake flash device in the ready state */
	struct device_state state = { .init_res = 0, .initialized = true };
	struct device flash_dev = { .state = &state };
	struct dfu_target_fmfu_fdev fmfu_dev_inf = { .dev = &flash_dev };

	dfu_target_full_modem_cfg_fake.custom_fake =
		fake_dfu_target_full_modem_cfg__succeeds;
	dfu_target_full_modem_fdev_get_fake.custom_fake =
		fake_dfu_target_full_modem_fdev_get__fails;

	ret = nrf_cloud_fota_fmfu_dev_set(&fmfu_dev_inf);
	zassert_not_equal(0, ret,
			  "return should not be 0 when dfu_target_full_modem_fdev_get fails");
}

/* Verify that nrf_cloud_fota_fmfu_apply returns failure when the external flash device is not set.
 * This test must be called before nrf_cloud_fota_fmfu_dev_set executes successfully
 */
ZTEST(nrf_cloud_fota_common_test, test_05_nrf_cloud_fota_fmfu_apply_fail_dev_not_set)
{
	int ret;

	nrf_modem_lib_shutdown_fake.custom_fake = nrf_modem_lib_shutdown__succeeds;
	nrf_modem_lib_bootloader_init_fake.custom_fake = nrf_modem_lib_bootloader_init__succeeds;
	fmfu_fdev_load_fake.custom_fake = fmfu_fdev_load__succeeds;

	ret = nrf_cloud_fota_fmfu_apply();
	zassert_equal(-EACCES, ret, "return should be -EACCES when flash device is not set");
}

/* Verify that nrf_cloud_pending_fota_job_process succeeds when a pending
 * FMFU FOTA validation FAILS because the external flash device is not set.
 * The reboot flag should become true.
 * This test must be called before nrf_cloud_fota_fmfu_dev_set executes successfully.
 */
ZTEST(nrf_cloud_fota_common_test, test_06_nrf_cloud_pending_fota_job_process_fmfu_validate_fail)
{
	int ret;
	bool reboot = false;
	struct nrf_cloud_settings_fota_job job = {
		.validate = NRF_CLOUD_FOTA_VALIDATE_PENDING,
		.type = NRF_CLOUD_FOTA_MODEM_FULL
	};

	ret = nrf_cloud_pending_fota_job_process(&job, &reboot);

	zassert_equal(0, ret, "return should be 0 after FMFU job is processed");
	zassert_equal(true, reboot, "reboot should be true when FMFU job is processed");
	/* nrf_cloud_fota_fmfu_dev_set has not yet successfully executed */
	zassert_equal(NRF_CLOUD_FOTA_VALIDATE_FAIL,
		      job.validate,
		      "validate status should be FAIL when flash device is not set");
}

/* Verify that nrf_cloud_fota_fmfu_dev_set succeeds when the flash device is
 * valid/ready and the dfu_target functions succeed.
 * Also, verify that when nrf_cloud_fota_fmfu_dev_set is called a second time, it
 * returns 1.
 * This PASS case for nrf_cloud_fota_fmfu_dev_set should be executed last due to static
 * flag which tracks if the FMFU device has been set successfully.
 */
ZTEST(nrf_cloud_fota_common_test, test_07_nrf_cloud_fota_fmfu_dev_set_pass)
{
	int ret;

	/* Provide a fake flash device in the ready state */
	struct device_state state = { .init_res = 0, .initialized = true };
	struct device flash_dev = { .state = &state };
	struct dfu_target_fmfu_fdev fmfu_dev_inf = { .dev = &flash_dev };

	dfu_target_full_modem_cfg_fake.custom_fake =
		fake_dfu_target_full_modem_cfg__succeeds;
	dfu_target_full_modem_fdev_get_fake.custom_fake =
		fake_dfu_target_full_modem_fdev_get__succeeds;

	ret = nrf_cloud_fota_fmfu_dev_set(&fmfu_dev_inf);
	zassert_equal(0, ret, "return should be 0 when device is not yet set");

	ret = nrf_cloud_fota_fmfu_dev_set(&fmfu_dev_inf);
	zassert_equal(1, ret, "return should be 1 when device is already set");
}

/* Verify that nrf_cloud_pending_fota_job_process succeeds when a pending
 * FMFU FOTA validation PASSES.
 * The reboot flag should become true.
 * Must be called after nrf_cloud_fota_fmfu_dev_set() is successful.
 */
ZTEST(nrf_cloud_fota_common_test, test_08_nrf_cloud_pending_fota_job_process_fmfu_pass)
{
	int ret;
	bool reboot = false;
	struct nrf_cloud_settings_fota_job job = {
		.validate = NRF_CLOUD_FOTA_VALIDATE_PENDING,
		.type = NRF_CLOUD_FOTA_MODEM_FULL
	};

	/* Set fakes so that nrf_cloud_fota_fmfu_apply() succeeds */
	nrf_modem_lib_shutdown_fake.custom_fake = nrf_modem_lib_shutdown__succeeds;
	nrf_modem_lib_bootloader_init_fake.custom_fake = nrf_modem_lib_bootloader_init__succeeds;
	fmfu_fdev_load_fake.custom_fake = fmfu_fdev_load__succeeds;

	ret = nrf_cloud_pending_fota_job_process(&job, &reboot);

	zassert_equal(0, ret, "return should be 0 when FMFU job is processed");
	zassert_equal(true, reboot, "reboot should be true when FMFU job is processed");
	zassert_equal(NRF_CLOUD_FOTA_VALIDATE_PASS,
		      job.validate,
		      "validate status should be PASS when FMFU job is processed successfully");
}

/* Verify that nrf_cloud_fota_fmfu_apply succeeds. */
ZTEST(nrf_cloud_fota_common_test, test_nrf_cloud_fota_fmfu_apply_pass)
{
	int ret;

	nrf_modem_lib_shutdown_fake.custom_fake = nrf_modem_lib_shutdown__succeeds;
	nrf_modem_lib_bootloader_init_fake.custom_fake = nrf_modem_lib_bootloader_init__succeeds;
	fmfu_fdev_load_fake.custom_fake = fmfu_fdev_load__succeeds;

	ret = nrf_cloud_fota_fmfu_apply();
	zassert_equal(0, ret, "return should be 0 when FMFU succeeds");
}

/* Verify that nrf_cloud_fota_fmfu_apply fails when nrf_modem_lib_shutdown fails. */
ZTEST(nrf_cloud_fota_common_test, test_nrf_cloud_fota_fmfu_apply_fail_shutdown_fail)
{
	int ret;

	nrf_modem_lib_shutdown_fake.custom_fake = nrf_modem_lib_shutdown__fails;

	ret = nrf_cloud_fota_fmfu_apply();
	zassert_not_equal(0, ret, "return should not be 0 when when nrf_modem_lib_shutdown fails");
}

/* Verify that nrf_cloud_fota_fmfu_apply fails when nrf_modem_lib_bootloader_init fails. */
ZTEST(nrf_cloud_fota_common_test, test_nrf_cloud_fota_fmfu_apply_fail_bl_init_fail)
{
	int ret;

	nrf_modem_lib_shutdown_fake.custom_fake = nrf_modem_lib_shutdown__succeeds;
	nrf_modem_lib_bootloader_init_fake.custom_fake = nrf_modem_lib_bootloader_init__fails;

	ret = nrf_cloud_fota_fmfu_apply();
	zassert_not_equal(0, ret,
			  "return should not be 0 when when nrf_modem_lib_bootloader_init fails");
}

/* Verify that nrf_cloud_fota_fmfu_apply fails when fmfu_fdev_load fails. */
ZTEST(nrf_cloud_fota_common_test, test_nrf_cloud_fota_fmfu_apply_fail_bl_fdev_load_fail)
{
	int ret;

	nrf_modem_lib_shutdown_fake.custom_fake = nrf_modem_lib_shutdown__succeeds;
	nrf_modem_lib_bootloader_init_fake.custom_fake = nrf_modem_lib_bootloader_init__succeeds;
	fmfu_fdev_load_fake.custom_fake = fmfu_fdev_load__fails;

	ret = nrf_cloud_fota_fmfu_apply();
	zassert_not_equal(0, ret, "return should not be 0 when when fmfu_fdev_load fails");
}
