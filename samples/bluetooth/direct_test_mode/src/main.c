/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_raw.h>

#include <bluetooth/dtm_twowire/dtm_twowire_to_hci.h>

#include "dtm_twowire_transport.h"

static K_FIFO_DEFINE(hci_c2h_queue);

int main(void)
{
	printk("Starting DTM sample\n");

	int err;

	err = dtm_tw_transport_init();
	if (err) {
		return err;
	}

	err = bt_enable_raw(&hci_c2h_queue);
	if (err) {
		return err;
	}

	for (;;) {
		const uint16_t tw_cmd = dtm_tw_transport_read();

		printk("Received 2-wire command 0x%04X\n", tw_cmd);

		uint16_t tw_event;
		struct net_buf *p_hci_pkt = bt_buf_get_tx(BT_BUF_CMD, K_FOREVER, NULL, 0);

		dtm_tw_to_hci_status_t status = dtm_tw_to_hci_process_tw_cmd(tw_cmd,
									     p_hci_pkt,
									     &tw_event);
		switch (status) {
		case DTM_TW_TO_HCI_STATUS_ERROR:
			net_buf_unref(p_hci_pkt);
			printk("Error processing 2-wire command 0x%04X\n", tw_cmd);
			break;

		case DTM_TW_TO_HCI_STATUS_TW_EVENT:
			net_buf_unref(p_hci_pkt);
			printk("Sending 2-wire event 0x%04X\n", tw_event);
			dtm_tw_transport_write(tw_event);
			break;

		case DTM_TW_TO_HCI_STATUS_HCI_CMD:
			printk("Sending HCI command to SDC\n");
			err = bt_send(p_hci_pkt);
			/* Note: bt_send unrefs the buffer on success */
			if (err) {
				net_buf_unref(p_hci_pkt);
				return err;
			}

			p_hci_pkt = k_fifo_get(&hci_c2h_queue, K_FOREVER);
			status = dtm_tw_to_hci_process_hci_event(tw_cmd,
								 p_hci_pkt,
								 &tw_event);
			net_buf_unref(p_hci_pkt);
			if (status == DTM_TW_TO_HCI_STATUS_TW_EVENT) {
				dtm_tw_transport_write(tw_event);
			}
			break;

		default:
			net_buf_unref(p_hci_pkt);
			printk("Unexpected status %d processing 2-wire command 0x%04X\n",
			       status, tw_cmd);
			break;
		}
	}
}
