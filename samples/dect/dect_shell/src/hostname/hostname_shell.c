/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/settings/settings.h>

#include <zephyr/net/hostname.h>

#include "desh_print.h"

struct hostname_settings {
	char hostname_str[NET_HOSTNAME_MAX_LEN];
};
static struct hostname_settings settings_data;

#define HOSTNAME_SETT_TREE_KEY		"hostname_settings"
#define HOSTNAME_SETT_COMMON_CONFIG_KEY "common"

static int dect_common_settings_handler(const char *key, size_t len, settings_read_cb read_cb,
					void *cb_arg)
{
	int ret = 0;

	if (strcmp(key, HOSTNAME_SETT_COMMON_CONFIG_KEY) == 0) {
		ret = read_cb(cb_arg, &settings_data, sizeof(settings_data));
		if (ret < 0) {
			printk("Failed to read dect phy_settings_data, error: %d", ret);
			return ret;
		}
		return 0;
	}
	return -ENOENT;
}

static int hostname_settings_write(void)
{
	int ret = 0;

	/* Flush RAM data to settings */
	ret = settings_save_one(HOSTNAME_SETT_TREE_KEY "/" HOSTNAME_SETT_COMMON_CONFIG_KEY,
				&settings_data, sizeof(settings_data));
	if (ret) {
		return ret;
	}
	return 0;
}

static struct settings_handler cfg = {.name = HOSTNAME_SETT_TREE_KEY,
				      .h_set = dect_common_settings_handler};

int hostname_settings_init(void)
{
	int ret = 0;

	/* Set the initial defaults from the code: */
	strcpy(settings_data.hostname_str, CONFIG_NET_HOSTNAME);

	ret = settings_subsys_init();
	if (ret) {
		printk("(%s): failed to initialize settings subsystem, error: %d", (__func__), ret);
		return ret;
	}
	ret = settings_register(&cfg);
	if (ret) {
		printk("Cannot register settings handler %d", ret);
		return ret;
	}
	ret = settings_load_subtree(HOSTNAME_SETT_TREE_KEY);
	if (ret) {
		printk("(%s): cannot load settings %d", (__func__), ret);
		return ret;
	}
	/* Write hostname from settings */
	if (net_hostname_set(settings_data.hostname_str, strlen(settings_data.hostname_str)) < 0) {
		printk("%s: failed to set hostname\n", __func__);
		return -EINVAL;
	}
	return 0;
}

SYS_INIT(hostname_settings_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static int cmd_hostname_write(const struct shell *shell, size_t argc, char **argv)
{
	if (net_hostname_set(argv[1], strlen(argv[1])) < 0) {
		desh_error("Failed to set hostname");
		return -EINVAL;
	}
	/* Store also to RAM settings */
	strcpy(settings_data.hostname_str, argv[1]);

	desh_print("Hostname set to %s", argv[1]);

	/* Write to settings */
	if (hostname_settings_write() < 0) {
		desh_error("Failed to write hostname to settings");
		return -EINVAL;
	}

	return 0;
}

static void cmd_hostname_read(const struct shell *shell, size_t argc, char **argv)
{
	desh_print("Hostname: %s", net_hostname_get());
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_hostname,
			       SHELL_CMD_ARG(write, NULL,
					     "Write hostname.\n"
					     "Usage: hostname write <hostname_str>\n",
					     cmd_hostname_write, 2, 0),
			       SHELL_CMD_ARG(read, NULL,
					     "Print current hostname.\n"
					     "Usage: hostame read",
					     cmd_hostname_read, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(hostname, &sub_hostname, "Commands for setting and reading hostname.",
		   desh_print_help_shell);
