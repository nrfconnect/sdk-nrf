/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <modem/pdn.h>
#include <modem/lte_lc.h>
#include <nrf_modem_at.h>

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
		printk("Event: PDP context %d, %s\n", cid, pdn_esm_strerror(reason));
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

	/* Setup a callback for the default PDP context (zero).
	 * Do this before switching to function mode 1 (CFUN=1)
	 * to receive the first activation event.
	 */
	err = pdn_default_ctx_cb_reg(pdn_event_handler);
	if (err) {
		printk("pdn_default_ctx_cb_reg() failed, err %d\n", err);
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
	err = pdn_activate(cid, &esm, NULL);
	if (err) {
		printk("pdn_activate() failed, err %d esm %d %s\n",
			err, esm, pdn_esm_strerror(err));
		return;
	}

	printk("PDP Context %d, PDN ID %d\n", 0, pdn_id_get(0));
	printk("PDP Context %d, PDN ID %d\n", cid, pdn_id_get(cid));

	lte_lc_power_off();
	printk("Bye\n");
}
