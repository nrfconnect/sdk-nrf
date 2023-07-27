/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef __SUPP_MAIN_H_
#define __SUPP_MAIN_H_

struct wpa_supplicant_event_msg {
	/* Dummy messages to unblock select */
	bool ignore_msg;
	void *ctx;
	unsigned int event;
	void *data;
};
int send_wpa_supplicant_event(const struct wpa_supplicant_event_msg *msg);
#endif /* __SUPP_MAIN_H_ */
