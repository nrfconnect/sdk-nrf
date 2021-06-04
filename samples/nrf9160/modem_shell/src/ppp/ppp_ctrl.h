/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_PPP_CTRL_H
#define MOSH_PPP_CTRL_H

#define PPP_MODEM_DATA_RAW_SCKT_FD_NONE -1

void ppp_ctrl_init(void);
int ppp_ctrl_start(const struct shell *shell);
void ppp_ctrl_stop(void);

#endif /* MOSH_PPP_CTRL_H */
