/* Full-stack IoT client example. */

/*
 * Copyright (c) 2018 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include "protocol.h"

#include <net/net_config.h>
#include <net/net_event.h>

#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/modem_key_mgmt.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>

LOG_MODULE_REGISTER(net_google_iot_mqtt, LOG_LEVEL_INF);

/* This comes from newlib. */
#include <inttypes.h>

/* Initialize AT communications */
static int at_comms_init(void)
{
	int err;

	err = at_cmd_init();
	if (err) {
		printk("Failed to initialize AT commands, err %d\n", err);
		return err;
	}

	err = at_notif_init();
	if (err) {
		printk("Failed to initialize AT notifications, err %d\n", err);
		return err;
	}

	return 0;
}

/*
 * Things that make sense in a demo app that would need to be more
 * robust in a real application:
 *
 */
void main(void)
{
	LOG_INF("Main entered");

	int err;

	printk("Sample started\n");

	err = nrf_modem_lib_init(NORMAL_MODE);
	if (err) {
		printk("Failed to initialize modem library!");
		return;
	}

	/* TODO: lte_lc_init_and_connect fails without this step */
	/* Initialize AT comms in order to provision the certificate */
	err = at_comms_init();
	if (err) {
		return;
	}

	printk("Waiting for network.. ");
	err = lte_lc_init_and_connect();
	if (err) {
		printk("Failed to connect to the LTE network, err %d\n", err);
		return;
	}

	printk("OK\n");

	mqtt_startup("mqtt.2030.ltsapis.goog", 8883);
}
