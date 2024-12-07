/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_auraconfig.h"

#include <babblekit/testcase.h>
#include <zephyr/shell/shell_backend.h>
#include <bstests.h>
#include <bs_tracing.h>
#include <bs_types.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nac_test, CONFIG_NAC_TEST_LOG_LEVEL);

extern enum bst_result_t bst_result;

#define MAX_ARGS	200
#define MAX_ARG_LEN	50
#define ARG_COMMAND	"nac"
#define MAX_COMMAND_LEN 60

static int argc_copy;
static char argv_copy[MAX_ARGS][MAX_ARG_LEN];

/* This code is executed by the test framework and not Zephyr */
static void test_args(int argc, char *argv[])
{
	argc_copy = argc;

	for (int i = 0; i < argc; i++) {
		if (i > MAX_ARGS) {
			TEST_FAIL("ENOMEM, MAX_ARG");
		}
		if (strlen(argv[i]) > MAX_ARG_LEN) {
			TEST_FAIL("ENOMEM, MAX_ARG_LEN");
		}

		memcpy(argv_copy[i], argv[i], strlen(argv[i]) + 1);
	}
}

static int modules_configure(void)
{
	int ret;
	char cmd_str[MAX_COMMAND_LEN] = {'\0'};

	if (argc_copy == 0) {
		LOG_ERR("No arguments provided");
		return -EINVAL;
	}

	if (strcmp(argv_copy[0], ARG_COMMAND) != 0) {
		LOG_ERR("First argument must be '%s'", ARG_COMMAND);
		return -EINVAL;
	}

	strcat(cmd_str, argv_copy[0]);

	/* Parse argv_copy, divide by keyword ARG_COMMAND and send commands to shell */
	for (int i = 1; i < argc_copy; i++) {
		if (strcmp(argv_copy[i], ARG_COMMAND) == 0) {
			/* Send command and reset argc */
			ret = shell_execute_cmd(NULL, cmd_str);
			if (ret) {
				LOG_ERR("Failed to send command %s, err: %d", cmd_str, ret);
				return ret;
			}
			memset(cmd_str, '\0', sizeof(cmd_str));

			strcat(cmd_str, argv_copy[i]);
		} else {
			if (strlen(cmd_str) + strlen(argv_copy[i]) > MAX_COMMAND_LEN) {
				LOG_ERR("Command too long");
				return -EINVAL;
			}

			strcat(cmd_str, " ");
			strcat(cmd_str, argv_copy[i]);
		}
	}

	/* Send last command */
	ret = shell_execute_cmd(NULL, cmd_str);
	if (ret) {
		LOG_ERR("Failed to send command %s", cmd_str);
		return ret;
	}

	return 0;
}

void test_main(void)
{
	int err;

	int backends = shell_backend_count_get();

	if (backends != 1) {
		TEST_FAIL("Only one shell backend accepted. Found: %d", backends);
	}

	nrf_auraconfig_main();

	err = modules_configure();
	if (err) {
		TEST_FAIL("Configure failed (err %d)", err);
	}
}

static void test_delete(void)
{
	if (bst_result == In_progress) {
		/* The nRF_Auraconfig sample won't stop by itself, so running == passing */
		TEST_PASS();
	} else {
		TEST_FAIL("Test aborted");
	}
}

static const struct bst_test_instance test_vector[] = {
	{
		.test_args_f = test_args,
		.test_id = "nac",
		.test_main_f = test_main,
		.test_delete_f = test_delete,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_nrf_auraconfig_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vector);
}

bst_test_install_t test_installers[] = {test_nrf_auraconfig_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
