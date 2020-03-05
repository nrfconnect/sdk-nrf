/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _CONFIG_CHANNEL_H_
#define _CONFIG_CHANNEL_H_

/**
 * @file
 * @defgroup config_channel Configuration channel
 * @{
 * @brief API for the nRF52 Desktop configuration channel.
 */

#include "config_event.h"

/** @brief Configuration channel data frame.
 */
struct config_channel_frame {
	/** HID report ID of the feature report used for transmitting data. */
	u8_t report_id;

	/** Product ID of the device to which the request is addressed.
	    Needed to route requests in a multi-device setup. */
	u16_t recipient;

	/** ID of the option to be set or fetched. */
	u8_t event_id;

	/** Status of the request. Also, used by sender to ask for fetch. */
	u8_t status;

	/** Length of data in the request. */
	u8_t event_data_len;

	/** Arbitrary length data connected to the request. */
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

/** @brief Configuration channel instance.
 */
struct config_channel_state {
	/** Status of the current transaction. */
	atomic_t status;

	/** @c true if there is a transaction in progress */
	bool transaction_active;

	/** @c true if the transport has been disconnected. */
	bool disconnected;

	/** @c true if the current transaction is a fetch request. */
	bool is_fetch;

	/** Work handling transaction timeout. */
	struct k_delayed_work timeout;

	/** Data frame holding the request details. */
	struct config_channel_frame frame;

	/** State of the fetch operation. */
	struct fetch fetch;

	/** Currently processed config event. */
	void *pending_config_event;
};

/** @brief Initialize the configuration channel instance.
 *
 *  @param cfg_chan Pointer to the configuration channel instance.
 */
void config_channel_init(struct config_channel_state *cfg_chan);

/**
 * @brief Handle a report get operation on the configuration channel.
 *
 * @param cfg_chan Pointer to the configuration channel instance.
 * @param buffer   Pointer to the report buffer to be filled
 *                 when handling the get request.
 * @param length   Length of the data to be filled.
 * @param usb @c true if the operation occurs for USB, @c false for Bluetooth.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int config_channel_report_get(struct config_channel_state *cfg_chan,
			      u8_t *buffer, size_t length, bool usb,
			      u16_t local_product_id);
/**
 * @brief Handle a report set operation on the configuration channel.
 *
 * @param cfg_chan Pointer to the configuration channel instance.
 * @param buffer   Pointer to the report buffer to be parsed
 *                 to handle the set request.
 * @param length   Length of the incoming data.
 * @param usb @c true if the operation occurs for USB, @c false for Bluetooth.
 * @param local_product_id Product ID (USB PID) of the targeted device.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int config_channel_report_set(struct config_channel_state *cfg_chan,
			      const u8_t *buffer, size_t length, bool usb,
			      u16_t local_product_id);

/**
 * @brief Parse the configuration channel report.
 *
 * @param buffer Pointer to the report buffer to be parsed.
 * @param length Length of the buffer.
 * @param frame  Pointer to the structure containing the parsed values.
 * @param usb @c true if the operation occurs for USB, @c false for Bluetooth.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int config_channel_report_parse(const u8_t *buffer, size_t length,
				struct config_channel_frame *frame, bool usb);
/**
 * @brief Fill the configuration channel report with
 *        values from a provided structure.
 *
 * @param buffer Pointer to the report buffer to be filled.
 * @param length Length of the buffer.
 * @param frame Pointer to a structure with values to be copied to the buffer.
 * @param usb @c true if the operation occurs for USB, @c false for Bluetooth.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int config_channel_report_fill(u8_t *buffer, const size_t length,
			       const struct config_channel_frame *frame, bool usb);

/**
 * @brief Handle the fetched data event from one of the firmware modules.
 *
 * @param cfg_chan Pointer to the configuration channel instance.
 * @param event    Pointer to the event carrying the fetched data.
 *
 */
void config_channel_fetch_receive(struct config_channel_state *cfg_chan,
				  const struct config_fetch_event *event);
/**
 * @brief Handle the confirmation that a configuration channel request
 *        has been forwarded.
 *
 * @param cfg_chan Pointer to the configuration channel instance.
 * @param event    Pointer to the event with status of the forwarded request.
 *
 */
void config_channel_forwarded_receive(struct config_channel_state *cfg_chan,
				      const struct config_forwarded_event *event);

/**
 * @brief Handle the confirmation that a configuration event is processed.
 *
 * @param cfg_chan Pointer to the configuration channel instance.
 * @param event    Pointer to the event with status of the forwarded request.
 *
 */
void config_channel_event_done(struct config_channel_state *cfg_chan,
			       const struct config_event *event);

/**
 * @brief Function for notifying a configuration channel instance that the
 * 	  connection has been lost. As a result, all configuration channel
 *	  transactions in progress are marked as invalid.
 *
 * @param cfg_chan Pointer to the configuration channel instance.
 *
 */
void config_channel_disconnect(struct config_channel_state *cfg_chan);

/**
 * @}
 */

#endif /*_CONFIG_CHANNEL_H_ */
