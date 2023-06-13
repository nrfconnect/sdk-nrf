/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_PPP_CTRL_H
#define MOSH_PPP_CTRL_H

#define PPP_MODEM_DATA_RAW_SCKT_FD_NONE -1
#define PPP_MODEM_DATA_RCV_SND_BUFF_SIZE 1500

void ppp_ctrl_init(void);
int ppp_ctrl_start(void);
void ppp_ctrl_stop(void);

void ppp_ctrl_default_pdn_active(bool default_pdn_active);

#endif /* MOSH_PPP_CTRL_H */
