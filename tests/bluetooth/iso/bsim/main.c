/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/shell/shell_backend.h>
#include <zephyr/sys/iterable_sections.h>

#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>

#include <babblekit/testcase.h>
#include "bstests.h"
#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "weak_stubs.h"

#include "iso_broadcast_src.h"
#include "iso_broadcast_sink.h"
#include "acl_central.h"
#include "acl_peripheral.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_ISO_TEST_LOG_LEVEL);

extern enum bst_result_t bst_result;

#define MAX_ARGS	40
#define MAX_ARG_LEN	40
#define CENTRAL_SOURCE	1
#define PERIPHERAL_SINK 2

#define COLOR_RED   "\x1B[0;31m"
#define COLOR_GREEN "\x1B[0;32m"
#define COLOR_BLUE  "\x1B[0;34m"
#define COLOR_RESET "\x1b[0m"

#define COMPANY_ID_NORDIC 0x0059

static uint8_t role;
extern enum bst_result_t bst_result;
static int argc_copy;
static char argv_copy[MAX_ARGS][MAX_ARG_LEN];

enum transport {
	ACL,
	ISO,
};

/* This code is executed by the test framework before Zephyr has booted */
static void test_args(int argc, char *argv[])
{
	argc_copy = argc;

	for (int i = 0; i < argc; i++) {
		if (i > MAX_ARGS) {
			TEST_FAIL("ENOMEM");
		}
		if (strlen(argv[i]) > MAX_ARG_LEN) {
			TEST_FAIL("ENOMEM");
		}
		memcpy(argv_copy[i], argv[i], strlen(argv[i]) + 1);
	}
}

struct test_report {
	uint32_t last_count;
	uint32_t counts_fail;
	uint32_t counts_success;
	double fail_percentage;
	bool passed;
};

/* Report structure for ACL and ISO results */
static struct test_report report[2];
static uint8_t fail_limit_percent[2];
static uint32_t packets_limit_min[] = {UINT32_MAX, UINT32_MAX};

static void result_check(uint32_t last_count, uint32_t counts_fail, uint32_t counts_success,
			 enum transport trans)
{

	if (role == CENTRAL_SOURCE) {
		return;
	}

	report[trans].last_count = last_count;
	report[trans].counts_fail = counts_fail;
	report[trans].counts_success = counts_success;

	if (counts_success == 0) {
		report[trans].fail_percentage = 100;
	} else {
		report[trans].fail_percentage =
			(counts_fail * 100) / (double)(counts_success + counts_fail);
	}

	/* Ensure enough packets have been transferred */
	if (report[ACL].counts_success < packets_limit_min[ACL] ||
	    report[ISO].counts_success < packets_limit_min[ISO]) {
		return;
	}

	if (report[ACL].fail_percentage > fail_limit_percent[ACL]) {
		TEST_FAIL("FAIL percentage %05.2f > %d", report[ACL].fail_percentage,
			  fail_limit_percent[ACL]);
	} else {
		report[ACL].passed = true;
	}

	if (report[ISO].fail_percentage > fail_limit_percent[ISO]) {
		TEST_FAIL("FAIL percentage %05.2f > %d", report[ISO].fail_percentage,
			  fail_limit_percent[ISO]);
	} else {
		report[ISO].passed = true;
	}

	if (report[ACL].passed && report[ISO].passed) {
		bst_result = 0; /* Set as passed */
	}
}

static void sim_acl_recv_main_cb(uint32_t last_count, uint32_t counts_fail, uint32_t counts_success)
{
	result_check(last_count, counts_fail, counts_success, ACL);
}

static void sim_iso_recv_main_cb(uint32_t last_count, uint32_t counts_fail, uint32_t counts_success)
{
	result_check(last_count, counts_fail, counts_success, ISO);
}

static bool is_number(const char *str)
{
	if (*str == '\0' || str == NULL) {
		return false;
	}

	uint8_t len = strlen(str);
	/* Iterate through each character of the string */
	for (uint8_t i = 0; i < len; i++) {
		/* Check if the character is not a digit */
		if (!isdigit((int)*str)) {
			return false;
		}
		str++;
	}

	return true;
}

static int cmd_shell_send(size_t argc, ...)
{
	int err;
	char cmd_str[MAX_ARG_LEN + 20] = {'\0'};
	va_list argv;

	va_start(argv, argc);

	if (argc == 0) {
		LOG_ERR("No arguments provided");
		return -EINVAL;
	}

	for (int i = 0; i < argc; i++) {
		const char *arg = va_arg(argv, const char *);

		if (strlen(cmd_str) + strlen(arg) + 1 > ARRAY_SIZE(cmd_str)) {
			LOG_ERR("Agument %u too long", i);
			return -ENOMEM;
		}

		LOG_DBG("Adding %s argc is %d", arg, argc);

		strcat(cmd_str, arg);
		strcat(cmd_str, " ");
	}

	err = shell_execute_cmd(NULL, cmd_str);
	if (err) {
		LOG_ERR("Failed to send command %s", cmd_str);
		return err;
	}

	va_end(argv);

	return 0;
}

static int modules_configure(void)
{
	int ret;
	char *arg_dev;
	char *arg_command;
	char *arg_value;

	if (argc_copy % 3 != 0) {
		TEST_FAIL("Arguments must be in sets of 3");
	}

	for (int i = 0; i < argc_copy; i = i + 3) {
		/* Argument meant for tester framework */
		if (strcmp(argv_copy[i], "tester") == 0) {
			continue;
		}

		arg_dev = argv_copy[i];
		arg_command = argv_copy[i + 1];

		if (!is_number(argv_copy[i + 2])) {
			TEST_FAIL("Argument: %s must be a number [%s]", argv_copy[i + 1],
				  argv_copy[i + 2]);
		}
		arg_value = argv_copy[i + 2];

		LOG_INF("Sending to: %s command: %s value: %s", arg_dev, arg_command, arg_value);
		ret = cmd_shell_send(3, arg_dev, arg_command, arg_value);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

int test_params_set(void)
{
	for (int i = 0; i < argc_copy; i++) {
		if (strcmp(argv_copy[i], "tester") != 0) {
			continue;
		}

		if (strcmp(argv_copy[i + 1], "dev") == 0) {
			if (strcmp(argv_copy[i + 2], "CENTRAL_SOURCE") == 0) {
				LOG_INF("Role set to Central/Source");
				role = CENTRAL_SOURCE;
			} else if (strcmp(argv_copy[i + 2], "PERIPHERAL_SINK") == 0) {
				LOG_INF("Role set to Peripheral/Sink");
				role = PERIPHERAL_SINK;
			}
		} else if (strcmp(argv_copy[i + 1], "acl_fail_limit_percent") == 0) {
			if (is_number(argv_copy[i + 2])) {
				fail_limit_percent[ACL] = strtol(argv_copy[i + 2], NULL, 10);
				LOG_INF("Var set: %d", fail_limit_percent[ACL]);
			} else {
				LOG_ERR("Argument %s must be a number [%s]", argv_copy[i],
					argv_copy[i + 2]);
				return -EINVAL;
			}
		} else if (strcmp(argv_copy[i + 1], "iso_fail_limit_percent") == 0) {
			if (is_number(argv_copy[i + 2])) {
				fail_limit_percent[ISO] = strtol(argv_copy[i + 2], NULL, 10);
				LOG_INF("Var set: %d", fail_limit_percent[ISO]);
			} else {
				LOG_ERR("Argument %s must be a number [%s]", argv_copy[i],
					argv_copy[i + 2]);
				return -EINVAL;
			}
		} else if (strcmp(argv_copy[i + 1], "acl_packets_limit_min") == 0) {
			if (is_number(argv_copy[i + 2])) {
				packets_limit_min[ACL] = strtol(argv_copy[i + 2], NULL, 10);
				LOG_INF("Var set: %d", packets_limit_min[ACL]);
			} else {
				LOG_ERR("Argument: %s must be a number [%s]", argv_copy[i],
					argv_copy[i + 2]);
				return -EINVAL;
			}
		} else if (strcmp(argv_copy[i + 1], "iso_packets_limit_min") == 0) {
			if (is_number(argv_copy[i + 2])) {
				packets_limit_min[ISO] = strtol(argv_copy[i + 2], NULL, 10);
				LOG_INF("Var set: %d", packets_limit_min[ISO]);
			} else {
				LOG_ERR("Argument: %s must be a number [%s]", argv_copy[i],
					argv_copy[i + 2]);
				return -EINVAL;
			}
		}
	}

	return 0;
}

static int modules_init(void)
{
	int err;

	if (role == PERIPHERAL_SINK) {
		TEST_START("ACL peripheral and ISO Broadcast Sink");

		err = iso_broadcast_sink_init(sim_iso_recv_main_cb);
		if (err) {
			LOG_ERR("iso_broadcaster_sink_init failed (err %d)", err);
			return err;
		}

		err = acl_peripheral_init(sim_acl_recv_main_cb);
		if (err) {
			LOG_ERR("central_init failed (err %d)", err);
			return err;
		}

	} else if (role == CENTRAL_SOURCE) {
		TEST_START("ACL central and ISO Broadcast source");

		err = iso_broadcast_src_init();
		if (err) {
			LOG_ERR("iso_broadcast_src_init failed (err %d)", err);
			return err;
		}

		err = acl_central_init();
		if (err) {
			LOG_ERR("central_init failed (err %d)", err);
			return err;
		}

	} else {
		LOG_ERR("Role not set");
		return -EINVAL;
	}

	return 0;
}

static int modules_start(void)
{
	int err;

	if (role == PERIPHERAL_SINK) {
		TEST_START("ACL peripheral + ISO Broadcast Sink");

		err = cmd_shell_send(2, "acl_peripheral", "start");
		if (err) {
			return err;
		}

		err = cmd_shell_send(2, "iso_brcast_sink", "start");
		if (err) {
			return err;
		}

	} else if (role == CENTRAL_SOURCE) {
		TEST_START("ACL central + ISO Broadcast source");

		err = cmd_shell_send(2, "acl_central", "start");
		if (err) {
			return err;
		}

		err = cmd_shell_send(2, "iso_brcast_src", "start");
		if (err) {
			return err;
		}

	} else {
		return -EINVAL;
	}

	return 0;
}

int ctlr_manufacturer_get(void)
{
	int ret;
	struct net_buf *rsp;

	ret = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, &rsp);
	if (ret) {
		return ret;
	}

	struct bt_hci_rp_read_local_version_info *rp = (void *)rsp->data;

	if (rp->manufacturer == COMPANY_ID_NORDIC) {
		/* NOTE: The string below is used by the Nordic CI system */
		TEST_PRINT("Controller: SoftDevice: Version %s (0x%02x), Revision %d",
			   bt_hci_get_ver_str(rp->hci_version), rp->hci_version, rp->hci_revision);
	} else {
		return -EPERM;
	}

	net_buf_unref(rsp);

	return 0;
}

void test_main(void)
{
	int err;

	report[ACL].passed = false;
	report[ISO].passed = false;
	report[ACL].fail_percentage = 100;
	report[ISO].fail_percentage = 100;

	int shells = shell_backend_count_get();

	if (shells != 1) {
		TEST_FAIL("Only one shell accepted. Found: %d", shells);
	}

	err = test_params_set();
	if (err) {
		TEST_FAIL("test_params_set failed (err %d)", err);
	}

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Bluetooth enable failed (err %d)", err);
	}

	err = ctlr_manufacturer_get();
	if (err) {
		TEST_FAIL("Unsupported controller (err %d)", err);
	}

	err = modules_init();
	if (err) {
		TEST_FAIL("Modules init failed (err %d)", err);
	}

	err = modules_configure();
	if (err) {
		TEST_FAIL("Configure failed (err %d)", err);
	}

	err = modules_start();
	if (err) {
		TEST_FAIL("Modules start failed (err %d)", err);
	}

	if (role == CENTRAL_SOURCE) {
		/* There are no test cases for this role.*/
		TEST_PASS("Central/source passed");
	}
}

static void test_delete(void)
{
	if (role == CENTRAL_SOURCE) {
		return;
	}

	const char text_color[][sizeof(COLOR_GREEN)] = {COLOR_GREEN, COLOR_RED};
	uint8_t selected_color = 1;
	if (bst_result == 0) {
		selected_color = 0;
	}

	bs_trace_info_time(0,
			   "%sReport ACL: Compl: %5u, Limit: %5u, fails: %4u = %05.2f, %% "
			   "limit: %u %%\n" COLOR_RESET,
			   text_color[selected_color], report[ACL].counts_success,
			   packets_limit_min[ACL], report[ACL].counts_fail,
			   report[ACL].fail_percentage, fail_limit_percent[ACL]);

	bs_trace_info_time(0,
			   "%sReport ISO: Compl: %5u, Limit: %5u, fails: %4u = %05.2f, %% "
			   "limit: %u %%\n" COLOR_RESET,
			   text_color[selected_color], report[ISO].counts_success,
			   packets_limit_min[ISO], report[ISO].counts_fail,
			   report[ISO].fail_percentage, fail_limit_percent[ISO]);
}

static const struct bst_test_instance test_vector[] = {
	{
		.test_args_f = test_args,
		.test_id = "bis_and_acl",
		.test_main_f = test_main,
		.test_delete_f = test_delete,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_bis_and_acl_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vector);
}

bst_test_install_t test_installers[] = {
	test_bis_and_acl_install,
	NULL,
};

int main(void)
{
	bst_main();
	return 0;
}
