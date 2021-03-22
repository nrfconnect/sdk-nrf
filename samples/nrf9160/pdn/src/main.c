/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <zephyr.h>
#include <modem/pdn.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>

extern const char *esm_strerr(int reason);

static const char * const fam_str[] = {
	[PDN_FAM_IPV4V6] = "IPV4V6",
	[PDN_FAM_IPV6] = "IPV6",
	[PDN_FAM_IPV4] = "IPV4",
};

static const char * const event_str[] = {
	[PDN_EVENT_CNEC_ESM] = "ESM",
	[PDN_EVENT_ACTIVATED] = "activated",
	[PDN_EVENT_DEACTIVATED] = "deactivated",
	[PDN_EVENT_IPV6_UP] = "IPv6 up",
	[PDN_EVENT_IPV6_DOWN] = "IPv6 down",
};

void pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	switch (event) {
	case PDN_EVENT_CNEC_ESM:
		printk("Event: PDP context %d, %s\n", cid, esm_strerr(reason));
		break;
	default:
		printk("Event: PDP context %d %s\n", cid, event_str[event]);
		break;
	}
}

void main(void)
{
	int err;
	int esm;
	uint8_t cid;
	char apn[32];

	printk("PDN sample started\n");

	/* Register to the necessary packet domain AT notifications */
	err = at_cmd_write("AT+CNEC=16", NULL, 0, NULL);
	if (err) {
		printk("AT+CNEC=16 failed, err %d\n", err);
		return;
	}

	err = at_cmd_write("AT+CGEREP=1", NULL, 0, NULL);
	if (err) {
		printk("AT+CGEREP=1 failed, err %d\n", err);
		return;
	}

	err = pdn_init();
	if (err) {
		return;
	}

	/* Setup a callback for the default PDP context (zero).
	 * Do this before switching to function mode 1 (CFUN=1)
	 * to receive the first activation event.
	 */
	err = pdn_default_callback_set(pdn_event_handler);
	if (err) {
		printk("pdn_default_callback_set() failed, err %d\n", err);
		return;
	}

	err = lte_lc_init_and_connect();
	if (err) {
		return;
	}

	err = pdn_default_apn_get(apn, sizeof(apn));
	if (err) {
		printk("pdn_default_apn_get() failed, err %d\n", err);
		return;
	}

	printk("Default APN is %s\n", apn);

	/* Create a PDP context and assign an event handler to receive events */
	err = pdn_ctx_create(&cid, pdn_event_handler);
	if (err) {
		printk("pdn_ctx_create() failed, err %d\n", err);
		return;
	}

	printk("Created new PDP context %d\n", cid);

	/* Configure a PDP context with APN and Family */
	err = pdn_ctx_configure(cid, apn, PDN_FAM_IPV4V6, NULL);
	if (err) {
		printk("pdn_ctx_configure() failed, err %d\n", err);
		return;
	}

	printk("PDP context %d configured: APN %s, Family %s\n",
	       cid, apn, fam_str[PDN_FAM_IPV4V6]);

	/* Activate a PDN connection */
	err = pdn_activate(cid, &esm);
	if (err) {
		printk("pdn_activate() failed, err %d esm %d %s\n",
			err, esm, esm_strerr(esm));
		return;
	}

	printk("PDP Context %d, PDN ID %d\n", 0, pdn_id_get(0));
	printk("PDP Context %d, PDN ID %d\n", cid, pdn_id_get(cid));

	printk("Bye\n");
}
