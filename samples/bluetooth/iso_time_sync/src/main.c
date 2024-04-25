/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <ctype.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/console/console.h>
#include "iso_time_sync.h"

enum role {
	CIS_CENTRAL = 'c',
	CIS_PERIPHERAL = 'p',
	BIS_TRANSMIT = 'b',
	BIS_RECEIVER = 'r',
};

enum direction {
	DIR_TX = 't',
	DIR_RX = 'r'
};

#define CONSOLE_INTEGER_INVALID 0xFFFFFFFFU

static enum role role_select(void)
{
	enum role role;

	while (true) {
		printk("Choose role - cis_central (c) / cis_peripheral (p) / bis_transmitter (b) / "
		       "bis_receiver (r) : ");
		role = console_getchar();
		printk("%c\n", role);

		switch (role) {
		case CIS_CENTRAL:
		case CIS_PERIPHERAL:
		case BIS_TRANSMIT:
		case BIS_RECEIVER:
			return role;
		default:
			printk("Invalid role selected\n");
		}
	}
}

static enum direction direction_select(void)
{
	enum direction direction;

	while (true) {
		printk("Choose direction - TX (t) / RX (r) : ");
		direction = console_getchar();
		printk("%c\n", direction);

		switch (direction) {
		case DIR_TX:
		case DIR_RX:
			return direction;
		default:
			printk("Invalid direction selected\n");
			break;
		}
	}
}

static size_t get_chars(char *buffer, size_t max_size)
{
	size_t pos = 0;

	while (pos < max_size) {
		char c = tolower(console_getchar());

		if (c == '\n' || c == '\r') {
			break;
		}
		printk("%c", c);
		buffer[pos++] = c;
	}
	printk("\n");
	buffer[pos] = '\0';

	return pos;
}

static uint32_t console_integer_get(void)
{
	char buffer[9];
	size_t char_count;

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return CONSOLE_INTEGER_INVALID;
	}

	return strtoul(buffer, NULL, 0);
}

static uint8_t bis_index_select(void)
{
	uint32_t index;

	while (true) {
		printk("Choose bis index [1..31] : ");

		index = console_integer_get();

		if (index == CONSOLE_INTEGER_INVALID) {
			/* Use default of 1. */
			return 1;
		} else if (index >= 1 && index <= 31) {
			return index;
		}

		printk("Invalid index\n");
	}
}

static uint8_t sdu_retransmission_number_get(void)
{
	uint32_t retransmission_number;

	while (true) {
		printk("Choose retransmission number [0..30] : ");

		retransmission_number = console_integer_get();

		if (retransmission_number == CONSOLE_INTEGER_INVALID) {
			/* Use default of 1. */
			return 1;
		} else if (retransmission_number <= 30) {
			return retransmission_number;
		}

		printk("Invalid retransmission number\n");
	}
}

static uint8_t sdu_max_transport_latency_get(void)
{
	uint32_t max_transport_latency_ms;

	while (true) {
		printk("Choose max transport latency in ms [5..4000] : ");

		max_transport_latency_ms = console_integer_get();

		if (max_transport_latency_ms == CONSOLE_INTEGER_INVALID) {
			/* Use default of 10. */
			return 10;
		} else if (max_transport_latency_ms >= 5 && max_transport_latency_ms <= 4000) {
			return max_transport_latency_ms;
		}

		printk("Invalid latency\n");
	}
}

int main(void)
{
	int err;
	enum role role;
	enum direction direction;
	uint8_t retransmission_number;
	uint16_t max_transport_latency_ms;

	console_init();

	printk("Bluetooth ISO Time Sync Demo\n");

	err = timed_led_toggle_init();
	if (err != 0) {
		printk("Error failed to init LED device for toggling\n");
		return err;
	}

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	role = role_select();
	if (role == BIS_TRANSMIT) {
		retransmission_number = sdu_retransmission_number_get();
		max_transport_latency_ms = sdu_max_transport_latency_get();

		printk("Starting BIS transmitter with BIS indices [1..%d], RTN: %d, max transport latency %d ms\n",
			CONFIG_BT_ISO_MAX_CHAN, retransmission_number, max_transport_latency_ms);
		bis_transmitter_start(retransmission_number, max_transport_latency_ms);
	} else if (role == BIS_RECEIVER) {
		uint8_t index = bis_index_select();

		printk("Starting BIS receiver, BIS index %d\n", index);
		bis_receiver_start(index);
	} else {
		direction = direction_select();

		if (role == CIS_CENTRAL) {
			retransmission_number = sdu_retransmission_number_get();
			max_transport_latency_ms = sdu_max_transport_latency_get();

			printk("Starting CIS central, dir: %s, RTN: %d, max transport latency %d ms\n",
			       direction == DIR_TX ? "tx" : "rx",
			       retransmission_number, max_transport_latency_ms);
			cis_central_start(direction == DIR_TX,
					  retransmission_number, max_transport_latency_ms);
		} else {
			printk("Starting CIS peripheral, dir: %s\n",
			       direction == DIR_TX ? "tx" : "rx");
			cis_peripheral_start(direction == DIR_TX);
		}
	}

	return 0;
}
