/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _CONFIG_CHANNEL_H_
#define _CONFIG_CHANNEL_H_

#include "config_event.h"

#define CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE 16

struct config_channel_frame {
	u8_t report_id;
	u16_t recipient;
	u8_t event_id;
	u8_t status;
	u8_t event_data_len;
	u8_t *event_data;
};

struct fetch {
	atomic_t done;
	u8_t data[CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE];
	size_t data_len;

	/* Recently fetched event_id and recipient. */
	u8_t event_id;
	u16_t recipient;
};

struct config_channel_state {
	atomic_t status;
	bool transaction_active;
	bool disconnected;
	bool is_fetch;
	struct k_delayed_work timeout;
	struct config_channel_frame frame;

	struct fetch fetch;
};

void config_channel_init(struct config_channel_state *cfg_chan);

int config_channel_report_get(struct config_channel_state *cfg_chan,
			      u8_t *buffer, size_t length, bool usb);
int config_channel_report_set(struct config_channel_state *cfg_chan,
			      const u8_t *buffer, size_t length, bool usb,
			      u16_t local_product_id);

int config_channel_report_parse(const u8_t *buffer, size_t length,
				struct config_channel_frame *frame, bool usb);
int config_channel_report_fill(u8_t *buffer, const size_t length,
			       const struct config_channel_frame *frame, bool usb);

void config_channel_fetch_receive(struct config_channel_state *cfg_chan,
				  const struct config_fetch_event *event);
void config_channel_forwarded_receive(struct config_channel_state *cfg_chan,
				      const struct config_forwarded_event *event);

void config_channel_disconnect(struct config_channel_state *cfg_chan);
#endif
