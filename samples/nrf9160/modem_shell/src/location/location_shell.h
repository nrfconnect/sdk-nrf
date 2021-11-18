/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LOCATION_SHELL_H
#define MOSH_LOCATION_SHELL_H

int location_shell(const struct shell *shell, size_t argc, char **argv);
void location_ctrl_init(void);

#endif /* MOSH_LOCATION_SHELL_H */
