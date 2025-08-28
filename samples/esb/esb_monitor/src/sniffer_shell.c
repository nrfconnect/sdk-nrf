/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <ctype.h>
#include <esb.h>

#include "sniffer.h"

static struct esb_sniffer_cfg *cfg;
static bool was_running;
static bool is_init;

static const char * const br_str[] = {
	"1MBPS",
	"2MBPS",
	"1MBPS_BLE",

#if defined(RADIO_MODE_MODE_Nrf_250Kbit)
	"250KBPS",
#endif /* defined(RADIO_MODE_MODE_Nrf_250Kbit) */

#if defined(RADIO_MODE_MODE_Ble_2Mbit)
	"2MBPS_BLE",
#endif /* defined(RADIO_MODE_MODE_Ble_2Mbit) */

#if defined(RADIO_MODE_MODE_Nrf_4Mbit_0BT6)
	"4MBPS",
#endif /* defined(RADIO_MODE_MODE_Nrf_4Mbit_0BT6) */

	NULL
};

static const char *bitrate_to_str(enum esb_bitrate br)
{
	switch (br) {
	case ESB_BITRATE_1MBPS:
		return br_str[0];
	case ESB_BITRATE_2MBPS:
		return br_str[1];
	case ESB_BITRATE_1MBPS_BLE:
		return br_str[2];

#if defined(RADIO_MODE_MODE_Nrf_250Kbit)
	case ESB_BITRATE_250KBPS:
		return "250KBPS";
#endif /* defined(RADIO_MODE_MODE_Nrf_250Kbit) */

#if defined(RADIO_MODE_MODE_Ble_2Mbit)
	case ESB_BITRATE_2MBPS_BLE:
		return "2MBPS_BLE";
#endif /* defined(RADIO_MODE_MODE_Ble_2Mbit) */

#if defined(RADIO_MODE_MODE_Nrf_4Mbit_0BT6)
	case ESB_BITRATE_4MBPS:
		return "4MBPS";
#endif /* defined(RADIO_MODE_MODE_Nrf_4Mbit_0BT6) */
	};

	return "";
}

static int user_input_to_bitrate(char *str, enum esb_bitrate *br)
{
	char *ptr;

	ptr = str;
	while (*ptr) {
		*ptr = toupper(*ptr);
		ptr++;
	}

	if (strcmp(br_str[0], str) == 0) {
		*br = ESB_BITRATE_1MBPS;
		return 0;
	}

	if (strcmp(br_str[1], str) == 0) {
		*br = ESB_BITRATE_2MBPS;
		return 0;
	}

	if (strcmp(br_str[2], str) == 0) {
		*br = ESB_BITRATE_1MBPS_BLE;
		return 0;
	}

#if defined(RADIO_MODE_MODE_Nrf_250Kbit)
	if (strcmp("250KBPS", str) == 0) {
		*br = ESB_BITRATE_250KBPS;
		return 0;
	}
#endif /* defined(RADIO_MODE_MODE_Nrf_250Kbit) */

#if defined(RADIO_MODE_MODE_Ble_2Mbit)
	if (strcmp("2MBPS_BLE", str) == 0) {
		*br = ESB_BITRATE_2MBPS_BLE;
		return 0;
	}
#endif /* defined(RADIO_MODE_MODE_Ble_2Mbit) */

#if defined(RADIO_MODE_MODE_Nrf_4Mbit_0BT6)
	if (strcmp("4MBPS", str) == 0) {
		*br = ESB_BITRATE_4MBPS;
		return 0;
	}
#endif /* defined(RADIO_MODE_MODE_Nrf_4Mbit_0BT6) */

	/* Bitrate not found */
	return -1;
}

static int user_input_to_addr(char *str, uint8_t *addr)
{
	char *ptr;
	int i;

	ptr = strtok(str, ".");
	for (i = 0; i < 4; i++) {
		if (ptr != NULL) {
			addr[i] = (uint8_t)strtol(ptr, (char **)NULL, 16);
			ptr = strtok(NULL, ".");
		} else {
			break;
		}
	}

	return (i == 4) ? 0 : -1;
}

static int user_input_to_pipes(char **argv, int argc, uint8_t *pipes)
{
	int pipe;

	if (strcmp(argv[1], "all") == 0) {
		*pipes = 0xFF;
		return 0;
	}

	for (int i = 1; i < argc; i++) {
		pipe = strtol(argv[i], (char **)NULL, 10);

		if (pipe == 0 && argv[1][0] != '0') {
			return -1;
		}

		if (pipe < 0 || pipe > 7) {
			return -1;
		}
		*pipes |= (1 << pipe);
	}

	return 0;
}

static int sniff_cmd_show(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t channel;

	esb_get_rf_channel(&channel);

	shell_print(shell, "Current sniffer config:");
	shell_print(shell, "Is running: %d", cfg->is_running);
	shell_print(shell, "Channel: %d", channel);
	shell_print(shell, "Bitrate: %s", bitrate_to_str(cfg->bitrate));

	shell_print(shell, "Base address 0: %02X.%02X.%02X.%02X",
						cfg->base_addr0[0], cfg->base_addr0[1],
						 cfg->base_addr0[2], cfg->base_addr0[3]);

	shell_print(shell, "Base address 1: %02X.%02X.%02X.%02X",
						cfg->base_addr1[0], cfg->base_addr1[1],
						 cfg->base_addr1[2], cfg->base_addr1[3]);

	shell_print(shell, "Pipe prefixes: %02X.%02X.%02X.%02X %02X.%02X.%02X.%02X",
				cfg->pipe_prefix[0], cfg->pipe_prefix[1], cfg->pipe_prefix[2],
				 cfg->pipe_prefix[3], cfg->pipe_prefix[4], cfg->pipe_prefix[5],
							cfg->pipe_prefix[6], cfg->pipe_prefix[7]);

	shell_print(shell, "Enabled pipes: %02X", cfg->enabled_pipes);
	return 0;
}

static int sniff_cmd_start(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;

	was_running = false;
	if (!cfg->is_running) {
		err = esb_start_rx();
	} else {
		return 0;
	}

	if (err) {
		shell_error(shell, "Failed to start sniffer, err: %d", err);
		return err;
	}

	cfg->is_running = true;
	shell_print(shell, "Sniffer started");
	return 0;
}

static int sniff_set_prev_state(const struct shell *shell, size_t argc, char **argv)
{
	if (was_running) {
		return sniff_cmd_start(shell, argc, argv);
	}

	return 0;
}

static int sniff_cmd_stop(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	if (cfg->is_running) {
		/* Called internally, not by user interaction */
		if (argc == 0) {
			was_running = true;
		}

		err = esb_stop_rx();
	} else {
		return 0;
	}

	if (err) {
		shell_error(shell, "Failed to stop sniffer, err: %d", err);
		return err;
	}

	cfg->is_running = false;
	shell_print(shell, "Sniffer stopped");
	return 0;
}

static int sniff_cmd_set_channel(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	uint32_t ch;

	ch = strtol(argv[1], (char **)NULL, 10);
	if (ch == 0 && argv[1][0] != '0') {
		shell_error(shell, "Failed to parse channel");
		return -EINVAL;
	}

	if (ch > 100) {
		shell_fprintf_impl(shell, SHELL_OPTION, "Channel range: <0, 100>\n");
		return -EINVAL;
	}

	sniff_cmd_stop(shell, 0, argv);
	err = esb_set_rf_channel(ch);
	if (err) {
		shell_error(shell, "Failed to set channel, err: %d", err);
		return err;
	}
	sniff_set_prev_state(shell, argc, argv);

	shell_print(shell, "Setting channel to %d", ch);
	return 0;
}

static int sniff_cmd_set_bitrate(const struct shell *shell, size_t argc, char **argv)
{
	enum esb_bitrate br;
	int err;

	if (argc < 2) {
		shell_help(shell);
		shell_fprintf_impl(shell, SHELL_OPTION, "Possible bitrates:\n");
		for (size_t i = 0; br_str[i] != NULL; i++) {
			shell_fprintf_impl(shell, SHELL_OPTION, "- %s\n", br_str[i]);
		}

		return 0;
	}

	err = user_input_to_bitrate(argv[1], &br);
	if (err < 0) {
		shell_error(shell, "Unknown bitrate");
		return -EINVAL;
	}

	sniff_cmd_stop(shell, 0, argv);
	err = esb_set_bitrate(br);
	if (err) {
		shell_error(shell, "Failed to set bitrate, err: %d", err);
		return err;
	}
	sniff_set_prev_state(shell, argc, argv);

	cfg->bitrate = br;
	shell_print(shell, "Setting bitrate to %s", bitrate_to_str(br));
	return 0;
}

static int sniff_cmd_set_addr0(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t new_addr[4];
	int err;

	err = user_input_to_addr(argv[1], new_addr);
	if (err) {
		shell_error(shell, "Failed to parse address");
		return -EINVAL;
	}

	sniff_cmd_stop(shell, 0, argv);
	err = esb_set_base_address_0(new_addr);
	if (err) {
		shell_error(shell, "Failed to set address 0, err: %d", err);
		return err;
	}
	sniff_set_prev_state(shell, argc, argv);

	memcpy(cfg->base_addr0, new_addr, 4);
	shell_print(shell, "Setting address 0 to %02X.%02X.%02X.%02X", new_addr[0], new_addr[1],
									new_addr[2], new_addr[3]);

	return 0;
}

static int sniff_cmd_set_addr1(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t new_addr[4];
	int err;

	err = user_input_to_addr(argv[1], new_addr);
	if (err) {
		shell_error(shell, "Failed to parse address");
		return -EINVAL;
	}

	sniff_cmd_stop(shell, 0, argv);
	err = esb_set_base_address_1(new_addr);
	if (err) {
		shell_error(shell, "Failed to set address 1, err: %d", err);
		return err;
	}
	sniff_set_prev_state(shell, argc, argv);

	memcpy(cfg->base_addr1, new_addr, 4);
	shell_print(shell, "Setting address 1 to %02X.%02X.%02X.%02X", new_addr[0], new_addr[1],
									new_addr[2], new_addr[3]);

	return 0;
}

static int sniff_cmd_set_prefix(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t new_prefix[8];
	int err, err2;

	err = user_input_to_addr(argv[1], new_prefix);
	err2 = user_input_to_addr(argv[2], &new_prefix[4]);
	if (err || err2) {
		shell_error(shell, "Failed to parse prefix");
		return -EINVAL;
	}

	sniff_cmd_stop(shell, 0, argv);
	err = esb_set_prefixes(new_prefix, 8);
	if (err) {
		shell_error(shell, "Failed to set prefixes, err: %d", err);
		return err;
	}
	sniff_set_prev_state(shell, argc, argv);

	memcpy(cfg->pipe_prefix, new_prefix, 8);
	shell_print(shell, "Setting prefixes to %02X.%02X.%02X.%02X %02X.%02X.%02X.%02X",
				new_prefix[0], new_prefix[1], new_prefix[2], new_prefix[3],
				 new_prefix[4], new_prefix[5], new_prefix[6], new_prefix[7]);
	return 0;
}

static int sniff_cmd_pipe_enable(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	uint8_t pipes_to_en;

	pipes_to_en = cfg->enabled_pipes;
	err = user_input_to_pipes(argv, argc, &pipes_to_en);
	if (err) {
		shell_error(shell, "Failed to parse pipes");
		return -EINVAL;
	}

	sniff_cmd_stop(shell, 0, argv);
	err = esb_enable_pipes(pipes_to_en);
	if (err) {
		shell_error(shell, "Failed to enable specified pipes, err: %d", err);
		return err;
	}
	sniff_set_prev_state(shell, argc, argv);

	cfg->enabled_pipes = pipes_to_en;
	shell_print(shell, "Enabled pipes: %02X", cfg->enabled_pipes);
	return 0;
}

static int sniff_cmd_pipe_disable(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	uint8_t pipes_to_en;

	pipes_to_en = 0;
	err = user_input_to_pipes(argv, argc, &pipes_to_en);
	if (err) {
		shell_error(shell, "Failed to parse pipes");
		return -EINVAL;
	}
	pipes_to_en = (cfg->enabled_pipes & ~pipes_to_en);

	sniff_cmd_stop(shell, 0, argv);
	err = esb_enable_pipes(pipes_to_en);
	if (err) {
		shell_error(shell, "Failed to disable specified pipes, err: %d", err);
		return err;
	}
	sniff_set_prev_state(shell, argc, argv);

	cfg->enabled_pipes = pipes_to_en;
	shell_print(shell, "Disabled pipes: %02X", (uint8_t)~cfg->enabled_pipes);

	return 0;
}

static int sniff_cmd_pipe_prefix(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	uint8_t pipe, prefix;

	pipe = strtol(argv[1], (char **)NULL, 10);
	if (pipe == 0 && argv[1][0] != '0') {
		shell_error(shell, "Failed to parse pipe number");
		return -EINVAL;
	}
	if (pipe < 0 || pipe > 7) {
		shell_error(shell, "Wrong pipe number");
		return -EINVAL;
	}
	prefix = (uint8_t)strtol(argv[2], (char **)NULL, 16);

	sniff_cmd_stop(shell, 0, argv);
	err = esb_update_prefix(pipe, prefix);
	if (err) {
		shell_error(shell, "Failed to update prefix on selected pipe, err: %d", err);
		return err;
	}
	sniff_set_prev_state(shell, argc, argv);

	cfg->pipe_prefix[pipe] = prefix;
	shell_print(shell, "Set pipe %u prefix to %02X", pipe, cfg->pipe_prefix[pipe]);
	return 0;
}

void sniffer_shell_init(struct esb_sniffer_cfg *sniffer_cfg)
{
	if (is_init || !sniffer_cfg) {
		return;
	}
	cfg = sniffer_cfg;
	is_init = true;

	SHELL_STATIC_SUBCMD_SET_CREATE(set_subcmds,
		SHELL_CMD_ARG(
			channel, NULL,
			"Set channel to listen on\n"
			"Ussage: sniffer set channel {ch_number}\n",
			sniff_cmd_set_channel, 2, 0
		),
		SHELL_CMD_ARG(
			bitrate, NULL,
			"Set bitrate\n"
			"Ussage: sniffer set bitrate {bitrate}\n",
			sniff_cmd_set_bitrate, 1, 1
		),
		SHELL_CMD_ARG(
			addr0, NULL,
			"Set base address 0\n"
			"Ussage: sniffer set addr0 {xx.xx.xx.xx}\n",
			sniff_cmd_set_addr0, 2, 0
		),
		SHELL_CMD_ARG(
			addr1, NULL,
			"Set base address 1\n"
			"Ussage: sniffer set addr1 {xx.xx.xx.xx}\n",
			sniff_cmd_set_addr1, 2, 0
		),
		SHELL_CMD_ARG(
			prefix, NULL,
			"Set prefix\n"
			"Ussage: sniffer set prefix {xx.xx.xx.xx} {xx.xx.xx.xx}\n",
			sniff_cmd_set_prefix, 3, 0
		),
		SHELL_SUBCMD_SET_END
	);

	SHELL_STATIC_SUBCMD_SET_CREATE(pipe_subcmds,
		SHELL_CMD_ARG(
			enable, NULL,
			"Enable selected pipes\n"
			"Ussage: sniffer pipe enable {pipe_number | all} ...\n",
			sniff_cmd_pipe_enable, 2, 7
		),
		SHELL_CMD_ARG(
			disable, NULL,
			"Disable selected pipes\n"
			"Ussage: sniffer pipe disable {pipe_number | all} ...\n",
			sniff_cmd_pipe_disable, 2, 7
		),
		SHELL_CMD_ARG(
			prefix, NULL,
			"Set prefix on selected pipe\n"
			"Ussage: sniffer pipe prefix {pipe_number} {prefix}\n",
			sniff_cmd_pipe_prefix, 3, 0
		),
		SHELL_SUBCMD_SET_END
	);

	SHELL_STATIC_SUBCMD_SET_CREATE(sniffer_subcmds,
		SHELL_CMD_ARG(
			show, NULL,
			"Show actual configuration",
			sniff_cmd_show, 1, 0
		),
		SHELL_CMD_ARG(
			start, NULL,
			"Start listening",
			sniff_cmd_start, 1, 0
		),
		SHELL_CMD_ARG(
			stop, NULL,
			"Stop listening",
			sniff_cmd_stop, 1, 0
		),
		SHELL_CMD_ARG(
			set, &set_subcmds,
			"Change parameters of sniffer",
			NULL, 0, 0
		),
		SHELL_CMD_ARG(
			pipe, &pipe_subcmds,
			"Change parameters of pipes",
			NULL, 0, 0
		),
		SHELL_SUBCMD_SET_END
	);
	SHELL_CMD_REGISTER(sniffer, &sniffer_subcmds, "Sniffer management command", NULL);
}
