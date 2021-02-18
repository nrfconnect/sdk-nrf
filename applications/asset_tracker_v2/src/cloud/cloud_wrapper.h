/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>

enum cloud_wrap_event_type {
	/** Cloud integration layer is connecting. */
	CLOUD_WRAP_EVT_CONNECTING,
	/** Cloud integration layer iss connected. */
	CLOUD_WRAP_EVT_CONNECTED,
	/** Cloud integration layer is disconnected. */
	CLOUD_WRAP_EVT_DISCONNECTED,
	/** Data received from cloud integration layer. */
	CLOUD_WRAP_EVT_DATA_RECEIVED,
	/** Cloud integration layer has successfully perform a FOTA update.
	 * Device should now be rebooted.
	 */
	CLOUD_WRAP_EVT_FOTA_DONE,
	/** The cloud integration layer has started a FOTA update. */
	CLOUD_WRAP_EVT_FOTA_START,
	/** An image erase is pending. */
	CLOUD_WRAP_EVT_FOTA_ERASE_PENDING,
	/** Image erase done. */
	CLOUD_WRAP_EVT_FOTA_ERASE_DONE,
	/** An error occurred during FOTA. */
	CLOUD_WRAP_EVT_FOTA_ERROR,
	/** Irrecoverable error has occurred in the cloud integration layer. */
	CLOUD_WRAP_EVT_ERROR,

};

struct cloud_wrap_event_data {
	char *buf;
	size_t len;
};

struct cloud_wrap_event {
	enum cloud_wrap_event_type type;

	union {
		struct cloud_wrap_event_data data;
		int err;
	};
};

typedef void (*cloud_wrap_evt_handler_t)(const struct cloud_wrap_event *evt);

/** Setup and initialize the configured cloud integration layer. */
int cloud_wrap_init(cloud_wrap_evt_handler_t event_handler);

/** Connect to cloud. */
int cloud_wrap_connect(void);

/** Disconnect from cloud. */
int cloud_wrap_disconnect(void);

/** Request device state from cloud. The device state contains the device
 * configuration.
 */
int cloud_wrap_state_get(void);

/** Send data to the device state. */
int cloud_wrap_state_send(char *buf, size_t len);

/** Send data to cloud. */
int cloud_wrap_data_send(char *buf, size_t len);

/** Send batched data to cloud. */
int cloud_wrap_batch_send(char *buf, size_t len);

/** Send UI data to cloud. Button presses. */
int cloud_wrap_ui_send(char *buf, size_t len);
