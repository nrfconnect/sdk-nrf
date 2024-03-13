/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <ctype.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/console/console.h>
#include <sdc_hci_vs.h>
#include "iso_combined_bis_and_cis.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_main, LOG_LEVEL_INF);

void print_intro(void)
{
	LOG_INF("\r\nBluetooth ISO Combined BIS and CIS sample\r\n\r\n"
		"The sample demonstrates data transfer over Bluetooth ISO\r\n"
		"CIS and BIS using the following topology:\r\n\r\n"
		"┌------┐     ┌-------┐      ┌--------┐\r\n"
		"|      |ISO  |  CIS  | ISO  |        |\r\n"
		"| CIS  ├----►|Central├-----►|BIS Sink|\r\n"
		"|Periph|CIS  |+ BIS  | BIS  |        |\r\n"
		"|      |     |Source |      |        |\r\n"
		"└------┘     └-------┘      └--------┘\r\n"
		"  (1)           (2)            (3)   \r\n"
		"The sample only operates as a device 2: Combined CIS Central + BIS Source\r\n"
		"Please, use other samples as a CIS Peripheral (TX) and BIS Sink (RX) "
		"devices.\r\n\r\n");
}

int main(void)
{
	int err;

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return 0;
	}

	print_intro();

	LOG_INF("Starting combined CIS Central + BIS Source");
	combined_cis_bis_start();

	return 0;
}
