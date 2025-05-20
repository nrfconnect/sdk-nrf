/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_ICMP_PING_SHELL_H
#define MOSH_ICMP_PING_SHELL_H

int icmp_ping_shell_th(const struct shell *shell, size_t argc, char **argv,
	char *print_buf, int print_buf_len, struct k_poll_signal *kill_signal);

#endif /* MOSH_ICMP_PING_SHELL_H */
