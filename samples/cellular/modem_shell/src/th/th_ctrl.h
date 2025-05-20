/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TH_CTRL_H
#define TH_CTRL_H
#include <zephyr/shell/shell.h>

#define TH_BG_THREADS_MAX_AMOUNT 2

void th_ctrl_init(void);
void th_ctrl_result_print(int nbr);
void th_ctrl_status_print(void);
void th_ctrl_start(const struct shell *shell,
		   size_t argc,
		   char **argv,
		   bool bg_thread,
		   bool pipeline);
void th_ctrl_kill(int nbr);
void th_ctrl_kill_em_all(void);

#endif /* TH_CTRL_H */
