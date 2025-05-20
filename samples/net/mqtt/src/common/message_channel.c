/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"

ZBUS_CHAN_DEFINE(TRIGGER_CHAN,			/* Name */
		 int,				/* Message type */
		 NULL,				/* Validator */
		 NULL,				/* User data */
		 ZBUS_OBSERVERS(sampler),	/* Observers */
		 ZBUS_MSG_INIT(0)		/* Initial value {0} */
);

ZBUS_CHAN_DEFINE(PAYLOAD_CHAN,
		 struct payload,
		 NULL,
		 NULL,
		 ZBUS_OBSERVERS(transport),
		 ZBUS_MSG_INIT(0)
);

ZBUS_CHAN_DEFINE(NETWORK_CHAN,
		 enum network_status,
		 NULL,
		 NULL,
		 ZBUS_OBSERVERS(transport IF_ENABLED(CONFIG_MQTT_SAMPLE_LED, (, led)), sampler),
		 ZBUS_MSG_INIT(0)
);

ZBUS_CHAN_DEFINE(FATAL_ERROR_CHAN,
		 int,
		 NULL,
		 NULL,
		 ZBUS_OBSERVERS(error),
		 ZBUS_MSG_INIT(0)
);
