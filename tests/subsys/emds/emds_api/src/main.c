/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/ztest.h>
#include <emds/emds.h>
#include <emds_flash.h>
#if CONFIG_SETTINGS
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#endif
#if defined(CONFIG_BT) && !defined(CONFIG_BT_LL_SW_SPLIT)
#include <zephyr/bluetooth/bluetooth.h>
#include <sdc.h>
#include <mpsl.h>
#endif

enum test_states {
	EMDS_TS_EMPTY_FLASH,
	EMDS_TS_STORE_DATA,
	EMDS_TS_CLEAR_FLASH,
	EMDS_TS_NO_STORE,
	EMDS_TS_SEVERAL_STORE
};

static int iteration;

/* test scenario */
static enum test_states state[] = {
	EMDS_TS_EMPTY_FLASH,
	EMDS_TS_STORE_DATA,
	EMDS_TS_SEVERAL_STORE,
	EMDS_TS_CLEAR_FLASH,
	EMDS_TS_EMPTY_FLASH,
	EMDS_TS_NO_STORE,
	EMDS_TS_EMPTY_FLASH,
	EMDS_TS_CLEAR_FLASH,
};

static const uint8_t expect_d_data[3][3][10] = {
	{{[0 ... 9] = 0x01}, {[0 ... 9] = 0x02}, {[0 ... 9] = 0x03}},
	{{[0 ... 9] = 0x11}, {[0 ... 9] = 0x12}, {[0 ... 9] = 0x13}},
	{{[0 ... 9] = 0x21}, {[0 ... 9] = 0x22}, {[0 ... 9] = 0x23}}
};

static uint8_t d_data[3][10];
static struct emds_dynamic_entry d_entries[3] = {
	{{0x1001, &d_data[0][0], 10}},
	{{0x1002, &d_data[1][0], 10}},
	{{0x1003, &d_data[2][0], 10}},
};

static const uint8_t expect_s_data[3][1024] = {
	{[0 ... 1023] = 0xAA},
	{[0 ... 1023] = 0xBB},
	{[0 ... 1023] = 0xCC},
};
static uint8_t s_data[1024];

EMDS_STATIC_ENTRY_DEFINE(s_entry, 0x100, s_data, sizeof(s_data));

static char *print_state(enum test_states s)
{
	switch (s) {
	case EMDS_TS_EMPTY_FLASH:
		return "EMPTY_FLASH";
	case EMDS_TS_STORE_DATA:
		return "STORE_DATA";
	case EMDS_TS_CLEAR_FLASH:
		return "CLEAR_FLASH";
	case EMDS_TS_SEVERAL_STORE:
		return "SEVERAL_STORE";
	case EMDS_TS_NO_STORE:
		return "NO_STORE";
	default:
		return "UNKNOWN";
	}
}

static void app_store_cb(void)
{
#if defined(CONFIG_BT) && !defined(CONFIG_BT_LL_SW_SPLIT)
	(void)bt_enable(NULL);
#endif
}

/** Mocks ******************************************/


/** End Mocks **************************************/

static void init(void)
{
	int err;

	err = emds_entry_add(&d_entries[0]);
	zassert_equal(err, -ECANCELED, "List not initialized");

	err = emds_prepare();
	zassert_equal(err, -ECANCELED, "Flash not initialized");

	err = emds_clear();
	zassert_equal(err, -ECANCELED, "Flash not initialized");

	zassert_false(emds_init(&app_store_cb), "Initializing failed");
	zassert_equal(emds_init(NULL), -EALREADY, "Initialization did not fail");
}

static void add_d_entries(void)
{
	int err;
	uint32_t store_expected = sizeof(s_data) + sizeof(struct emds_data_entry);
	uint32_t store_used;

	for (int i = 0; i < ARRAY_SIZE(d_entries); i++) {
		err = emds_entry_add(&d_entries[i]);
		store_expected += d_entries[i].entry.len + sizeof(struct emds_data_entry);
		zassert_equal(err, 0, "Add entry failed");

		err = emds_entry_add(&d_entries[i]);
		zassert_equal(err, -EINVAL, "Entry duplicated");
	}

	err = emds_store_size_get(&store_used);
	zassert_equal(err, 0, "Getting store size failed");

	zassert_equal(store_used, store_expected, "Wrong storage size: expected: %i, got: %i",
		      store_expected, store_used);
}

static void load_empty_flash(void)
{
	uint8_t test_d_data[sizeof(d_data)] = { 0xAC };
	uint8_t test_s_data[sizeof(s_data)] = { 0xAC };

	memcpy(d_data, test_d_data, sizeof(d_data));
	memcpy(s_data, test_s_data, sizeof(s_data));

	zassert_equal(emds_load(), -ENOENT, "Load failed");

	zassert_mem_equal(d_data, test_d_data, sizeof(d_data),
			  "Data has changed");
	zassert_mem_equal(s_data, test_s_data, sizeof(s_data),
			  "Data has changed");
}

static void load_flash(int idx)
{
	memset(d_data, 0, sizeof(d_data));
	memset(s_data, 0, sizeof(s_data));

	zassert_equal(emds_load(), 0, "Load failed");

	zassert_mem_equal(d_data, &expect_d_data[idx][0][0], sizeof(d_data),
			  "Data has changed");
	zassert_mem_equal(s_data, &expect_s_data[idx][0], sizeof(s_data),
			  "Data has changed");
}

static void prepare(void)
{
	zassert_equal(emds_store(), -ECANCELED, "Prepare must be done before store");

	zassert_false(emds_is_ready(), "EMDS should not be ready");

	zassert_equal(emds_prepare(), 0, "Prepare failed.");

	zassert_true(emds_is_ready(), "EMDS should be ready");
}

static void store(int idx)
{
	zassert_true(emds_is_ready(), "Store should be ready to execute");

	memcpy(d_data, &expect_d_data[idx][0][0], sizeof(d_data));
	memcpy(s_data, &expect_s_data[idx][0], sizeof(s_data));

#if defined(CONFIG_BT) && !defined(CONFIG_BT_LL_SW_SPLIT)
	/* Disable bluetooth and mpsl scheduler if bluetooth is enabled. */
	(void) sdc_disable(); // Replace with bt_disable when added.
	mpsl_uninit();
#endif

	int64_t start_tic = k_uptime_ticks();

	zassert_equal(emds_store(), 0, "Store failed");

	k_yield();
	int64_t store_time_ticks = k_uptime_ticks() - start_tic;

	zassert_false(emds_is_ready(), "Store not completed");

	uint64_t store_time_us = k_ticks_to_us_ceil64(store_time_ticks);

	uint32_t estimate_store_time_us = emds_store_time_get();

	printf("Store time: Actual %lldus, Worst case:  %dus\n",
	       store_time_us, estimate_store_time_us);

	zassert_true((store_time_us < estimate_store_time_us), "Store takes to long time");
}

static void clear(void)
{
	zassert_equal(emds_clear(), 0, "Clear failed");
}

static bool pragma_always(const void *s)
{
	return true;
}

static bool pragma_empty_flash(const void *s)
{
	const enum test_states *state = s;

	return *state == EMDS_TS_EMPTY_FLASH;
}

static bool pragma_store_data(const void *s)
{
	const enum test_states *state = s;

	return *state == EMDS_TS_STORE_DATA;
}

static bool pragma_clear_flash(const void *s)
{
	const enum test_states *state = s;

	return *state == EMDS_TS_CLEAR_FLASH;
}

static bool pragma_no_store(const void *s)
{
	const enum test_states *state = s;

	return *state == EMDS_TS_NO_STORE;
}

static bool pragma_several_store(const void *s)
{
	const enum test_states *state = s;

	return *state == EMDS_TS_SEVERAL_STORE;
}

#if CONFIG_SETTINGS
static int emds_test_settings_set(const char *name, size_t len,
				  settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	int rc;

	if (settings_name_steq(name, "iteration", &next) && !next) {
		if (len != sizeof(iteration)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, &iteration, sizeof(iteration));
		if (rc >= 0) {
			return 0;
		}

		return rc;
	}

	return -ENOENT;
}

struct settings_handler emds_test_conf = {
	.name = "emds_test",
	.h_set = emds_test_settings_set
};
#endif

ZTEST(_setup, test_setup)
{
	init();
	add_d_entries();
}

ZTEST(empty_flash, test_empty_flash)
{
	load_empty_flash();
	prepare();
	store(0);
	load_flash(0);
}

ZTEST(store_data, test_store_data)
{
	load_flash(0);
	prepare();
	store(1);
	load_flash(1);
}

ZTEST(clear_flash, test_clear_flash)
{
	load_flash(0);
	clear();
	load_empty_flash();
}

ZTEST(no_store, test_no_store)
{
	clear();
	load_empty_flash();
	prepare();
	load_empty_flash();
}

ZTEST(several_store, test_several_store)
{
	load_flash(1);
	prepare();
	store(2);
	load_flash(2);
	prepare();
	store(0);
	load_flash(0);
}

ZTEST_SUITE(_setup, pragma_always, NULL, NULL, NULL, NULL);
ZTEST_SUITE(empty_flash, pragma_empty_flash, NULL, NULL, NULL, NULL);
ZTEST_SUITE(store_data, pragma_store_data, NULL, NULL, NULL, NULL);
ZTEST_SUITE(clear_flash, pragma_clear_flash, NULL, NULL, NULL, NULL);
ZTEST_SUITE(no_store, pragma_no_store, NULL, NULL, NULL, NULL);
ZTEST_SUITE(several_store, pragma_several_store, NULL, NULL, NULL, NULL);

void test_main(void)
{
#if defined(CONFIG_BT) && !defined(CONFIG_BT_LL_SW_SPLIT)
	(void)bt_enable(NULL);
#endif
#if CONFIG_SETTINGS
	(void)settings_subsys_init();
	(void)settings_register(&emds_test_conf);
	(void)settings_load();
#endif
	enum test_states run_state = state[iteration];

	if (iteration == 0) {
		printk("***********************************************\n");
		printk("*************** EMDS TEST START ***************\n");
	}
	printk("********** Test run: %s (%d) **********\n",
		print_state(run_state), iteration);

	ztest_run_all(&run_state, false, 1, 1);

#if CONFIG_SETTINGS
	iteration++;
	if (iteration < ARRAY_SIZE(state)) {
		(void)settings_save_one("emds_test/iteration", &iteration, sizeof(iteration));
		sys_reboot(SYS_REBOOT_COLD);
	} else {
		(void)settings_save_one("emds_test/iteration", 0, sizeof(iteration));
	}
#else
	(void)emds_clear();
#endif
}
