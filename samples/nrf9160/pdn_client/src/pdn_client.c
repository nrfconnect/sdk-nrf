/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <pdn.h>
#include <net/bsdlib.h>
#include <at_cmd.h>
#include <at_notif.h>
#include <lte_lc.h>

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static void modem_configure(void)
{
	int err;

	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	} else {
		printk("LTE Link Connecting ...\n");
		err = lte_lc_init_and_connect();
		__ASSERT(err == 0, "LTE link could not be established.");
		printk("LTE Link Connected!\n");
	}
}

void main(void)
{
	int error;
	const char *const address = CONFIG_PDN_SAMPLE_APN;

	printk("The PDN client sample started\n");

	modem_configure();

	printk("Connecting to APN '%s'... ", address);

	const int pdn_handle = pdn_connect(address, PDN_INET_IPV4V6);

	if (pdn_handle < 0) {
		printk("FAIL! Error: %d\n", pdn_handle);
		return;
	}

	printk("OK\nActivating the PDN... ");

	error = pdn_activate(pdn_handle);
	if (error < 0) {
		printk("FAIL! Error=%d\n", error);

		printk("Try to activate default PDN... ");

		error = pdn_activate(0);
		if (error < 0) {
			printk("FAIL! Error=%d\n", error);
			return;
		}
	}

	printk("OK\nPDN state=%d and ID=%d\n", pdn_state_get(pdn_handle),
	       pdn_id_get(pdn_handle));

	printk("Deactivate and disconnect the PDN... ");
	pdn_disconnect(pdn_handle);

	printk("OK\nSample completed, exiting...\n");
}
