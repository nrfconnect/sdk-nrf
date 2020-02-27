/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <sys/byteorder.h>
#include <errno.h>

#include "hid_report_desc.h"
#include "config_channel.h"

#define MODULE config_channel

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_CONFIG_CHANNEL_LOG_LEVEL);

static int frame_size_check(const struct config_channel_frame *frame,
			    const size_t length, bool usb)
{
	const size_t min_size = sizeof(frame->recipient) +
				sizeof(frame->event_id) +
				sizeof(frame->status) +
				sizeof(frame->event_data_len);
	const size_t max_size = REPORT_SIZE_USER_CONFIG +
				(usb ? sizeof(frame->report_id) : 0);

	__ASSERT_NO_MSG(min_size < max_size);

	if ((length < min_size) || (length > max_size)) {
		LOG_WRN("Unsupported report length %zu", length);
		return -ENOTSUP;
	}

	return 0;
}

static int data_len_check(const struct config_channel_frame *frame,
			  u8_t event_data_len, bool usb)
{
	const size_t min_size = sizeof(frame->recipient) +
				sizeof(frame->event_id) +
				sizeof(frame->status) +
				sizeof(frame->event_data_len);
	const size_t max_size = REPORT_SIZE_USER_CONFIG +
				(usb ? sizeof(frame->report_id) : 0);

	__ASSERT_NO_MSG(min_size < max_size);

	if (event_data_len > max_size - min_size) {
		LOG_WRN("Unsupported event data length %" PRIu8,
			event_data_len);
		return -ENOTSUP;
	}

	return 0;
}

static void timeout_handler(struct k_work *work)
{
	struct config_channel_state *cfg_chan =
		CONTAINER_OF(work, struct config_channel_state, timeout);

	atomic_set(&cfg_chan->status, CONFIG_STATUS_TIMEOUT);
	LOG_WRN("Config channel transaction timed out");

	cfg_chan->transaction_active = false;
}

void config_channel_init(struct config_channel_state *cfg_chan)
{
	k_delayed_work_init(&cfg_chan->timeout, timeout_handler);

	cfg_chan->transaction_active = false;
	cfg_chan->disconnected = false;
}

int config_channel_report_parse(const u8_t *buffer, size_t length,
				struct config_channel_frame *frame, bool usb)
{
	int err = frame_size_check(frame, length, usb);
	if (err) {
		return err;
	}

	size_t pos = 0;
	if (usb) {
		frame->report_id = buffer[pos];
		pos += sizeof(frame->report_id);
	}

	frame->recipient = sys_get_le16(&buffer[pos]);
	pos += sizeof(frame->recipient);

	frame->event_id = buffer[pos];
	pos += sizeof(frame->event_id);

	frame->status = buffer[pos];
	pos += sizeof(frame->status);

	frame->event_data_len = buffer[pos];
	pos += sizeof(frame->event_data_len);

	err = data_len_check(frame, frame->event_data_len, usb);
	if (err) {
		return err;
	}

	/* Event data should be copied by the caller. */
	return pos;
}

int config_channel_report_fill(u8_t *buffer, const size_t length,
			       const struct config_channel_frame *frame, bool usb)
{
	int err = frame_size_check(frame, length, usb);
	if (!err) {
		err = data_len_check(frame, frame->event_data_len, usb);
	}

	if (err) {
		return err;
	}

	size_t pos = 0;

	/* BLE HID service removes report ID, according to HOGP_SPEC_V10 */
	if (usb) {
		buffer[pos] = REPORT_ID_USER_CONFIG;
		pos += sizeof(frame->report_id);
	}

	sys_put_le16(frame->recipient, &buffer[pos]);
	pos += sizeof(frame->recipient);

	buffer[pos] = frame->event_id;
	pos += sizeof(frame->event_id);

	buffer[pos] = frame->status;
	pos += sizeof(frame->status);

	buffer[pos] = frame->event_data_len;
	pos += sizeof(frame->event_data_len);

	__ASSERT_NO_MSG(pos + frame->event_data_len <= length);

	if (frame->event_data_len > 0) {
		memcpy(&buffer[pos], frame->event_data, frame->event_data_len);
	}

	return pos;
}

int config_channel_report_get(struct config_channel_state *cfg_chan,
			      u8_t *buffer, size_t length, bool usb,
			      u16_t local_product_id)
{
	int pos;

	cfg_chan->frame.status = atomic_get(&cfg_chan->status);
	cfg_chan->frame.event_data_len = 0;

	if (cfg_chan->frame.status == CONFIG_STATUS_REJECT ||
	    cfg_chan->frame.status == CONFIG_STATUS_TIMEOUT) {
		LOG_WRN("Transaction failed, reason %d", cfg_chan->frame.status);
		pos = config_channel_report_fill(buffer, length,
						 &cfg_chan->frame, usb);
		if (pos < 0) {
			LOG_WRN("Could not set report");
			return pos;
		}

		return -EIO;
	}

	if (cfg_chan->is_fetch) {
		__ASSERT_NO_MSG(cfg_chan->frame.event_data_len == 0);

		if (cfg_chan->disconnected) {
			cfg_chan->frame.status = CONFIG_STATUS_DISCONNECTED_ERROR;
		} else if (atomic_get(&cfg_chan->fetch.done)) {
			if (cfg_chan->fetch.recipient != cfg_chan->frame.recipient ||
			    cfg_chan->fetch.event_id != cfg_chan->frame.event_id) {
				LOG_WRN("Fetched wrong item: recipient 0x%x id %d",
					cfg_chan->fetch.recipient, cfg_chan->fetch.event_id);
			}

			cfg_chan->frame.event_data_len = cfg_chan->fetch.data_len;
			cfg_chan->frame.event_data = (u8_t *) &cfg_chan->fetch.data;
			cfg_chan->frame.status = CONFIG_STATUS_SUCCESS;
		} else {
			if (cfg_chan->frame.recipient != local_product_id) {
				if (usb) {
					LOG_INF("Forwarding fetch get request to %" PRIx16, cfg_chan->frame.recipient);

					struct config_forward_get_event *event =
						new_config_forward_get_event();

					event->recipient = cfg_chan->frame.recipient;
					event->id = cfg_chan->frame.event_id;
					event->status = CONFIG_STATUS_FETCH;
					event->channel_id = cfg_chan;

					atomic_set(&cfg_chan->status, CONFIG_STATUS_PENDING);

					EVENT_SUBMIT(event);
				}
			}

			cfg_chan->frame.status = CONFIG_STATUS_PENDING;
		}
	} else {
		/* Event ID and recipient are retained from latest request from host */
		cfg_chan->transaction_active = false;
	}

	pos = config_channel_report_fill(buffer, length,
					 &cfg_chan->frame, usb);
	if (pos < 0) {
		LOG_WRN("Could not set report");
		return pos;
	}

	if (cfg_chan->frame.status != CONFIG_STATUS_PENDING) {
		atomic_set(&cfg_chan->status, cfg_chan->frame.status);
		k_delayed_work_cancel(&cfg_chan->timeout);
		cfg_chan->transaction_active = false;
	}

	return 0;
}

int config_channel_report_set(struct config_channel_state *cfg_chan,
			      const u8_t *buffer, size_t length, bool usb,
			      u16_t local_product_id)
{
	if (usb && (atomic_get(&cfg_chan->status) == CONFIG_STATUS_PENDING)) {
		LOG_WRN("Busy forwarding");
		return -EBUSY;
	}

	if (cfg_chan->transaction_active) {
		LOG_WRN("Transaction already in progress");

		atomic_set(&cfg_chan->status, CONFIG_STATUS_REJECT);
		return -EALREADY;
	}

	/* Feature report set */
	int pos = config_channel_report_parse(buffer, length,
					      &cfg_chan->frame, usb);
	if (pos < 0) {
		LOG_WRN("Could not parse report");
		return pos;
	}

	if (usb && (cfg_chan->frame.report_id != REPORT_ID_USER_CONFIG)) {
		LOG_WRN("Unsupported report ID %" PRIu8, cfg_chan->frame.report_id);
		return -ENOTSUP;
	}

	/* Start transaction timeout. */
	k_delayed_work_submit(&cfg_chan->timeout,
			      K_SECONDS(CONFIG_DESKTOP_CONFIG_CHANNEL_TIMEOUT));

	cfg_chan->transaction_active = true;
	cfg_chan->disconnected = false;
	cfg_chan->is_fetch = (cfg_chan->frame.status == CONFIG_STATUS_FETCH);

	atomic_set(&cfg_chan->fetch.done, false);

	if (cfg_chan->frame.recipient == local_product_id) {
		if (cfg_chan->is_fetch) {
			struct config_fetch_request_event *event = new_config_fetch_request_event();

			event->recipient = cfg_chan->frame.recipient;
			event->id = cfg_chan->frame.event_id;
			event->channel_id = cfg_chan;

			atomic_set(&cfg_chan->status, CONFIG_STATUS_PENDING);

			EVENT_SUBMIT(event);
		} else {
			struct config_event *event = new_config_event(cfg_chan->frame.event_data_len);

			memcpy(event->dyndata.data, &(buffer[pos]), cfg_chan->frame.event_data_len);
			event->id = cfg_chan->frame.event_id;

			cfg_chan->pending_config_event = event;

			atomic_set(&cfg_chan->status, CONFIG_STATUS_PENDING);

			EVENT_SUBMIT(event);
		}
	} else {
		if (usb) {
			LOG_INF("Forwarding event to %" PRIx16, cfg_chan->frame.recipient);

			struct config_forward_event *event =
				new_config_forward_event(cfg_chan->frame.event_data_len);

			event->recipient = cfg_chan->frame.recipient;
			event->id = cfg_chan->frame.event_id;

			if (cfg_chan->is_fetch) {
				event->status = CONFIG_STATUS_FETCH;
			} else {
				event->status = CONFIG_STATUS_PENDING;
				memcpy(event->dyndata.data, &buffer[pos], cfg_chan->frame.event_data_len);
			}

			EVENT_SUBMIT(event);

			atomic_set(&cfg_chan->status, CONFIG_STATUS_PENDING);

		} else {
			LOG_WRN("Unsupported recipient %" PRIx16, cfg_chan->frame.recipient);

			atomic_set(&cfg_chan->status, CONFIG_STATUS_REJECT);

			k_delayed_work_cancel(&cfg_chan->timeout);
			cfg_chan->transaction_active = false;

			return -ENOTSUP;
		}
	}

	return 0;
}

void config_channel_fetch_receive(struct config_channel_state *cfg_chan,
				  const struct config_fetch_event *event)
{
	if (event->channel_id != cfg_chan) {
		/* Request was made by the other transport. */
		LOG_DBG("Fetch dropped, not intended for this transport");
		return;
	}

	cfg_chan->fetch.event_id = event->id;
	cfg_chan->fetch.recipient = event->recipient;

	if (cfg_chan->disconnected) {
		LOG_WRN("Transaction invalid, ignore fetched data");

		return;
	}

	if (event->dyndata.size > sizeof(cfg_chan->fetch.data)) {
		LOG_WRN("Fetched data size %zu too big", event->dyndata.size);
		return;
	}

	memcpy(cfg_chan->fetch.data, event->dyndata.data,
	       event->dyndata.size);
	cfg_chan->fetch.data_len = event->dyndata.size;

	atomic_set(&cfg_chan->fetch.done, true);
}

void config_channel_forwarded_receive(struct config_channel_state *cfg_chan,
				      const struct config_forwarded_event *event)
{
	atomic_set(&cfg_chan->status, event->status);
}

void config_channel_event_done(struct config_channel_state *cfg_chan,
			       const struct config_event *event)
{
	if (event == cfg_chan->pending_config_event) {
		atomic_set(&cfg_chan->status, CONFIG_STATUS_SUCCESS);

		k_delayed_work_cancel(&cfg_chan->timeout);
		cfg_chan->transaction_active = false;
	}
}

void config_channel_disconnect(struct config_channel_state *cfg_chan)
{
	if (cfg_chan->transaction_active) {
		cfg_chan->disconnected = true;
		cfg_chan->transaction_active = false;
		k_delayed_work_cancel(&cfg_chan->timeout);
	}
}
