/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef __SUPP_MAIN_H_
#define __SUPP_MAIN_H_

#include "wpa_supplicant_i.h"

struct wpa_supplicant *z_wpas_get_handle_by_ifname(const char *ifname);
struct wpa_supplicant_event_msg {
	bool global;
	void *ctx;
	unsigned int event;
	void *data;
};
int z_wpas_send_event(const struct wpa_supplicant_event_msg *msg);
extern struct k_work_q z_wpas_wq;
#endif /* __SUPP_MAIN_H_ */
