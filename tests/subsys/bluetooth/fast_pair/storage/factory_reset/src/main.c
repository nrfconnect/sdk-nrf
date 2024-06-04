/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_main);

#include "fp_storage.h"
#include "fp_storage_ak.h"
#include "fp_storage_ak_priv.h"
#include "fp_storage_clock.h"
#include "fp_storage_clock_priv.h"
#include "fp_storage_eik.h"
#include "fp_storage_eik_priv.h"
#include "fp_storage_pn.h"
#include "fp_storage_pn_priv.h"
#include "fp_storage_manager.h"
#include "fp_storage_manager_priv.h"

#include "common_utils.h"


#define ACCOUNT_KEY_MAX_CNT CONFIG_BT_FAST_PAIR_STORAGE_ACCOUNT_KEY_MAX

#define SETTINGS_RESET_TEST_SUBTREE_NAME "fp_reset_test"
#define SETTINGS_STAGE_KEY_NAME "stage"
#define SETTINGS_NAME_SEPARATOR_STR "/"
#define SETTINGS_RESET_TEST_STAGE_FULL_NAME \
	(SETTINGS_RESET_TEST_SUBTREE_NAME SETTINGS_NAME_SEPARATOR_STR SETTINGS_STAGE_KEY_NAME)

#define DELETE_READOUT_MAX_NAME_LEN	64

BUILD_ASSERT(SETTINGS_NAME_SEPARATOR == '/');

enum reset_test_stage {
	RESET_TEST_STAGE_NORMAL,
	RESET_TEST_STAGE_INTERRUPTED,
};

enum reset_test_finished {
	RESET_TEST_FINISHED_STORE_BIT_POS,
	RESET_TEST_FINISHED_NORMAL_RESET_BIT_POS,
	RESET_TEST_FINISHED_INTERRUPTED_RESET_BEGIN_BIT_POS,

	RESET_TEST_FINISHED_COUNT,
};

static enum reset_test_stage test_stage = RESET_TEST_STAGE_NORMAL;
static uint8_t test_finished_mask;

BUILD_ASSERT(__CHAR_BIT__ * sizeof(test_finished_mask) >= RESET_TEST_FINISHED_COUNT);

static int settings_set_err;

static bool storage_reset_pass;


static int settings_load_stage(size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;

	if (len != sizeof(test_stage)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, &test_stage, len);
	if (rc < 0) {
		return rc;
	}

	if (rc != len) {
		return -EINVAL;
	}

	return 0;
}

static int fp_reset_test_settings_set(const char *name, size_t len, settings_read_cb read_cb,
				      void *cb_arg)
{
	int err = 0;

	if (!strncmp(name, SETTINGS_STAGE_KEY_NAME, sizeof(SETTINGS_STAGE_KEY_NAME))) {
		err = settings_load_stage(len, read_cb, cb_arg);
	} else {
		err = -ENOENT;
	}

	/* The first reported settings set error will be remembered by the module.
	 * The error will then be propagated by the commit callback.
	 * Errors returned in the settings set callback are not propagated further.
	 */
	if (err && !settings_set_err) {
		settings_set_err = err;
	}

	return err;
}

static int fp_reset_test_settings_commit(void)
{
	return settings_set_err;
}

SETTINGS_STATIC_HANDLER_DEFINE(fp_reset_test, SETTINGS_RESET_TEST_SUBTREE_NAME, NULL,
			       fp_reset_test_settings_set, fp_reset_test_settings_commit, NULL);

static void mark_test_as_finished(enum reset_test_finished bit_pos)
{
	WRITE_BIT(test_finished_mask, bit_pos, 1);
}

static int test_fp_storage_reset_perform(void)
{
	if (storage_reset_pass) {
		mark_test_as_finished(RESET_TEST_FINISHED_INTERRUPTED_RESET_BEGIN_BIT_POS);
		storage_reset_pass = false;
		ztest_test_pass();
	}

	return 0;
}

static void test_fp_storage_reset_prepare(void) {}

/* The name starts with '_0_' so that this module's struct would be first in section (linker sorts
 * entries by name) and its callbacks would be call before other's modules callbacks.
 */
FP_STORAGE_MANAGER_MODULE_REGISTER(_0_test_reset, test_fp_storage_reset_perform,
				   test_fp_storage_reset_prepare, NULL, NULL);

static void *setup_fn(void)
{
	struct fp_storage_manager_module *module;

	STRUCT_SECTION_GET(fp_storage_manager_module, 0, &module);

	zassert_equal_ptr(test_fp_storage_reset_perform, module->module_reset_perform,
			  "Invalid order of elements in section");
	zassert_equal_ptr(test_fp_storage_reset_prepare, module->module_reset_prepare,
			  "Invalid order of elements in section");

	return NULL;
}

static int settings_validate(settings_load_direct_cb cb)
{
	int err;
	int err_cb = 0;

	err = settings_load_subtree_direct(NULL, cb, &err_cb);
	if (err) {
		return err;
	}

	if (err_cb) {
		return err_cb;
	}

	return 0;
}

static int settings_validate_empty_cb(const char *key, size_t len, settings_read_cb read_cb,
				      void *cb_arg, void *param)
{
	int *err = param;

	*err = -EFAULT;
	return *err;
}

static int fp_settings_data_validate_empty_cb(const char *key, size_t len, settings_read_cb read_cb,
					      void *cb_arg, void *param)
{
	int *err = param;

	if (!strncmp(key, SETTINGS_RESET_TEST_STAGE_FULL_NAME,
		     sizeof(SETTINGS_RESET_TEST_STAGE_FULL_NAME))) {
		return 0;
	}

	*err = -EFAULT;
	return *err;
}

static void fp_ram_clear(void)
{
	fp_storage_ak_ram_clear();
	fp_storage_clock_ram_clear();
	fp_storage_eik_ram_clear();
	fp_storage_pn_ram_clear();
	fp_storage_manager_ram_clear();
}

static void fp_storage_validate_unloaded_eik(void)
{
	static const uint8_t eik_store[FP_STORAGE_EIK_LEN] = {
		0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
		0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
		0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
		0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
	};

	int err;
	int ret;
	uint8_t eik[FP_STORAGE_EIK_LEN];

	err = fp_storage_eik_save(eik_store);
	zassert_equal(err, -EACCES, "Expected error before settings load");

	err = fp_storage_eik_delete();
	zassert_equal(err, -EACCES, "Expected error before settings load");

	ret = fp_storage_eik_is_provisioned();
	zassert_equal(ret, -EACCES, "Expected error before settings load");

	err = fp_storage_eik_get(eik);
	zassert_equal(err, -EACCES, "Expected error before settings load");
}

static void fp_storage_validate_unloaded(void)
{
	int err;
	char read_name[FP_STORAGE_PN_BUF_LEN];
	uint32_t clock_checkpoint;

	cu_account_keys_validate_uninitialized();

	err = fp_storage_pn_get(read_name);
	zassert_equal(err, -EACCES, "Expected error before settings load");

	err = fp_storage_clock_boot_checkpoint_get(&clock_checkpoint);
	zassert_equal(err, -EACCES, "Expected error before settings load");
	err = fp_storage_clock_current_checkpoint_get(&clock_checkpoint);
	zassert_equal(err, -EACCES, "Expected error before settings load");

	fp_storage_validate_unloaded_eik();
}

static void before_fn(void *f)
{
	ARG_UNUSED(f);

	int err;

	err = settings_validate(settings_validate_empty_cb);
	zassert_ok(err, "Settings have not been fully deleted");

	fp_storage_validate_unloaded();

	err = settings_load();
	zassert_ok(err, "Unexpected error in settings load");

	err = fp_storage_init();
	zassert_ok(err, "Unexpected error in modules init");
}

static int settings_get_name_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
				void *param)
{
	char *name = param;

	__ASSERT(strnlen(key, DELETE_READOUT_MAX_NAME_LEN) < DELETE_READOUT_MAX_NAME_LEN,
		 "Unexpected length of settings name");
	strcpy(name, key);

	/* Error value is returned to stop iterating over settings records. */
	return -EALREADY;
}

static int all_settings_delete(void)
{
	char name[DELETE_READOUT_MAX_NAME_LEN];
	char prev_name[DELETE_READOUT_MAX_NAME_LEN] = "";

	while (true) {
		int err;
		int ret;

		strcpy(name, "");

		err = settings_load_subtree_direct(NULL, settings_get_name_cb, name);
		if (err) {
			return err;
		}

		ret = strcmp(name, "");
		if (!ret) {
			break;
		}

		ret = strcmp(name, prev_name);
		if (!ret) {
			return -EDEADLK;
		}

		err = settings_delete(name);
		if (err) {
			return err;
		}

		strcpy(prev_name, name);
	}

	return 0;
}

static void after_fn(void *f)
{
	ARG_UNUSED(f);

	int err;

	err = all_settings_delete();
	zassert_ok(err, "Unable to delete settings");

	fp_ram_clear();
}

static void personalized_name_store(const char *name)
{
	int err;

	err = fp_storage_pn_save(name);
	zassert_ok(err, "Unexpected error in name save");
}

static void personalized_name_validate_loaded(const char *name)
{
	int err;
	int ret;
	char read_name[FP_STORAGE_PN_BUF_LEN];

	err = fp_storage_pn_get(read_name);
	zassert_ok(err, "Unexpected error in name get");
	ret = strcmp(read_name, name);
	zassert_equal(ret, 0, "Invalid Personalized Name");
}

static void clock_checkpoint_store(uint32_t clock)
{
	int err;

	err = fp_storage_clock_checkpoint_update(clock);
	zassert_ok(err, "Unexpected error in clock checkpoint update");
}

static void clock_checkpoint_validate_loaded(uint32_t clock_current, uint32_t clock_boot)
{
	int err;
	uint32_t clock_checkpoint;

	err = fp_storage_clock_boot_checkpoint_get(&clock_checkpoint);
	zassert_ok(err, "Unexpected error in clock boot checkpoint get");
	zassert_equal(clock_checkpoint, clock_boot, "Invalid clock boot checkpoint");

	err = fp_storage_clock_current_checkpoint_get(&clock_checkpoint);
	zassert_ok(err, "Unexpected error in clock current checkpoint get");
	zassert_equal(clock_checkpoint, clock_current, "Invalid clock current checkpoint");
}

static void eik_store(const uint8_t *eik)
{
	int err;

	err = fp_storage_eik_save(eik);
	zassert_ok(err, "Unexpected error in EIK store operation");
}

static void eik_validate_loaded(const uint8_t *eik)
{
	int err;
	int ret;
	uint8_t eik_loaded[FP_STORAGE_EIK_LEN];

	ret = fp_storage_eik_is_provisioned();
	zassert_equal(ret, 1, "EIK unprovisioned or unexpected error during state check");

	err = fp_storage_eik_get(eik_loaded);
	zassert_ok(err, "Unexpected error in EIK get");
	zassert_mem_equal(eik_loaded, eik, sizeof(eik_loaded), "Invalid EIK");
}

static void fp_storage_validate_empty(void)
{
	int err;
	int ret;
	char read_name[FP_STORAGE_PN_BUF_LEN];
	struct fp_account_key account_key_list[ACCOUNT_KEY_MAX_CNT];
	size_t read_cnt = ACCOUNT_KEY_MAX_CNT;
	uint32_t clock_checkpoint;
	uint8_t eik[FP_STORAGE_EIK_LEN];

	err = settings_validate(fp_settings_data_validate_empty_cb);
	zassert_ok(err, "Non-volatile Fast Pair data storage is not empty after reset");

	zassert_equal(fp_storage_ak_count(), 0, "Invalid Account Key count");

	err = fp_storage_ak_get(account_key_list, &read_cnt);
	zassert_ok(err, "Unexpected error in Account Key get");
	zassert_equal(read_cnt, 0, "Invalid Account Key count");

	err = fp_storage_clock_boot_checkpoint_get(&clock_checkpoint);
	zassert_ok(err, "Unexpected error in clock boot checkpoint get");
	zassert_equal(clock_checkpoint, 0, "Invalid clock boot checkpoint");
	err = fp_storage_clock_current_checkpoint_get(&clock_checkpoint);
	zassert_ok(err, "Unexpected error in clock current checkpoint get");
	zassert_equal(clock_checkpoint, 0, "Invalid clock current checkpoint");

	ret = fp_storage_eik_is_provisioned();
	zassert_equal(ret, 0, "Expected unprovisioned EIK");
	err = fp_storage_eik_get(eik);
	zassert_equal(err, -ENODATA, "Expected no EIK data");

	err = fp_storage_pn_get(read_name);
	zassert_ok(err, "Unexpected error in name get");
	ret = strcmp(read_name, "");
	zassert_equal(ret, 0, "Expected saved name to be empty");
}

static void fp_storage_self_test_run(void)
{
	static const uint8_t seed = 5;
	static const uint8_t key_count = ACCOUNT_KEY_MAX_CNT;
	static const char *name = "fp_storage_self_test_run";
	static const uint32_t clock_current = 0xDEADBEEF;
	static const uint32_t clock_boot_first;
	static const uint32_t clock_boot_second = clock_current;
	static const uint8_t eik[FP_STORAGE_EIK_LEN] = {
		0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
		0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
		0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
		0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
	};
	int err;

	fp_storage_validate_empty();

	cu_account_keys_generate_and_store(seed, key_count);
	personalized_name_store(name);
	clock_checkpoint_store(clock_current);
	eik_store(eik);

	cu_account_keys_validate_loaded(seed, key_count);
	personalized_name_validate_loaded(name);
	clock_checkpoint_validate_loaded(clock_current, clock_boot_first);
	eik_validate_loaded(eik);

	fp_ram_clear();
	fp_storage_validate_unloaded();
	err = settings_load();
	zassert_ok(err, "Unexpected error in settings load");
	err = fp_storage_init();
	zassert_ok(err, "Unexpected error in modules init");

	cu_account_keys_validate_loaded(seed, key_count);
	personalized_name_validate_loaded(name);
	clock_checkpoint_validate_loaded(clock_current, clock_boot_second);
	eik_validate_loaded(eik);
}

ZTEST(suite_fast_pair_storage_factory_reset_01, test_01_store)
{
	fp_storage_self_test_run();

	mark_test_as_finished(RESET_TEST_FINISHED_STORE_BIT_POS);
}

static void store_data_and_factory_reset(void)
{
	static const uint8_t seed = 50;
	static const uint8_t key_count = ACCOUNT_KEY_MAX_CNT;
	static const char *name = "store_data_and_factory_reset";
	static const uint32_t clock_current = 0xDEADBEEF;
	static const uint32_t clock_boot;
	static const uint8_t eik[FP_STORAGE_EIK_LEN] = {
		0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
		0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
		0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
		0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
	};
	int err;

	cu_account_keys_generate_and_store(seed, key_count);
	personalized_name_store(name);
	clock_checkpoint_store(clock_current);
	eik_store(eik);

	cu_account_keys_validate_loaded(seed, key_count);
	personalized_name_validate_loaded(name);
	clock_checkpoint_validate_loaded(clock_current, clock_boot);
	eik_validate_loaded(eik);

	err = fp_storage_factory_reset();
	zassert_ok(err, "Unexpected error in factory reset");
}

ZTEST(suite_fast_pair_storage_factory_reset_01, test_02_normal_reset)
{
	store_data_and_factory_reset();

	fp_storage_self_test_run();

	mark_test_as_finished(RESET_TEST_FINISHED_NORMAL_RESET_BIT_POS);
}

ZTEST(suite_fast_pair_storage_factory_reset_02, test_interrupted_reset_begin)
{
	storage_reset_pass = true;

	store_data_and_factory_reset();

	/* The recent instruction should trigger test pass so this code should never be entered. */
	zassert_unreachable();
}

static void before_continue_fn(void *f)
{
	ARG_UNUSED(f);

	int err;

	err = settings_load();
	zassert_ok(err, "Unexpected error in settings load");
	err = fp_storage_init();
	zassert_ok(err, "Unexpected error in modules init");
}

ZTEST(suite_fast_pair_storage_factory_reset_03, test_interrupted_reset_finish)
{
	fp_storage_self_test_run();
}

static void teardown_fn(void *f)
{
	ARG_UNUSED(f);

	int err;

	for (size_t i = 0; i < RESET_TEST_FINISHED_COUNT; i++) {
		if (!(test_finished_mask & BIT(i))) {
			/* Do not reboot the device, clean up and abandon the test to
			 * properly report fails in the last test suite that was run.
			 */
			err = all_settings_delete();
			__ASSERT(!err, "Unable to delete settings (err %d)", err);

			return;
		}
	}

	test_stage = RESET_TEST_STAGE_INTERRUPTED;
	err = settings_save_one(SETTINGS_RESET_TEST_STAGE_FULL_NAME, &test_stage,
				sizeof(test_stage));
	__ASSERT(!err, "Unexpected error in test settings (err %d)", err);

	if (IS_ENABLED(CONFIG_REBOOT)) {
		sys_reboot(SYS_REBOOT_WARM);
	}

	fp_ram_clear();
}

static bool predicate_fn(const void *global_state)
{
	ARG_UNUSED(global_state);

	return (test_stage == RESET_TEST_STAGE_NORMAL);
}

static bool predicate_continue_fn(const void *global_state)
{
	ARG_UNUSED(global_state);

	return (test_stage == RESET_TEST_STAGE_INTERRUPTED);
}

ZTEST_SUITE(suite_fast_pair_storage_factory_reset_01,
	    predicate_fn, setup_fn, before_fn, after_fn, NULL);
ZTEST_SUITE(suite_fast_pair_storage_factory_reset_02,
	    predicate_fn, setup_fn, before_fn, NULL, teardown_fn);
ZTEST_SUITE(suite_fast_pair_storage_factory_reset_03,
	    predicate_continue_fn, setup_fn, before_continue_fn, after_fn, NULL);

void test_main(void)
{
	int err;

	err = settings_subsys_init();
	__ASSERT(!err, "Error while initializing settings subsys (err %d)", err);

	err = settings_load_subtree(SETTINGS_RESET_TEST_SUBTREE_NAME);
	__ASSERT(!err, "Error while loading test settings (err %d)", err);

	__ASSERT((test_stage == RESET_TEST_STAGE_NORMAL) ||
		 (test_stage == RESET_TEST_STAGE_INTERRUPTED),
		 "Invalid test stage");

	ztest_run_all(NULL, false, 1, 1);
}
