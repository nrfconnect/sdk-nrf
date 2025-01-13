/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hci_internal_wrappers.h"
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/buf.h>

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
/*
 * The Host will generate up to acl_pkts number of Host Number of Completed Packets command plus a
 * number of normal HCI commands, as such we need to ensure the tx command buffer count is big
 * enough to not block incoming ACKs from the host.
 *
 * When Controller to Host data flow control is supported, ensure that `CONFIG_BT_BUF_CMD_TX_COUNT`
 * is greater than or equal to `BT_BUF_ACL_RX_COUNT + Ncmd`, where Ncmd is the supported maximum
 * Num_HCI_Command_Packets.
 *
 * The SDC controller (currently) does not support Num_HCI_Command_Packets > 1, which means Ncmd
 * is always 1.
 *
 * `CONFIG_BT_BUF_CMD_TX_COUNT` is used differently depending on whether `CONFIG_BT_HCI_HOST` or
 * `BT_HCI_RAW` is defined. And as ACL packet pools are only used in connections,
 * we need to limit the build assert as such. See the comments above the asserts for more
 * information on the specific scenario.
 */
#if defined(CONFIG_BT_HCI_HOST) && (BT_BUF_ACL_RX_COUNT > 0)
/*
 * When `CONFIG_BT_HCI_HOST`, `CONFIG_BT_BUF_CMD_TX_COUNT` controls the capacity of
 * `bt_hci_cmd_create`. Which is used for all BT HCI Commands, and can be called concurrently from
 * many different application contexts, initiating various processes.
 *
 * Currently `bt_hci_cmd_create` is generally `K_FOREVER`, as such this build assert only guarantees
 * to avoid deadlocks due to missing CMD buffers, if the host is only allocating the next command
 * once the previous is completed.
 */
BUILD_ASSERT(BT_BUF_ACL_RX_COUNT < CONFIG_BT_BUF_CMD_TX_COUNT,
	     "Too low HCI command buffers compared to ACL Rx buffers.");
#else  /* controller-only build */
/*
 * On a controller-only build (`BT_HCI_RAW`) `CONFIG_BT_BUF_CMD_TX_COUNT` controls the capacity of
 * `bt_buf_get_tx(BT_BUF_CMD)`. Which is only used to receive commands from the Host at the rate the
 * command flow control dictates. Considering one buffer is needed to the Num_HCI_Command_Packet, to
 * do flow control, at least one more buffer is needed.
 *
 */
BUILD_ASSERT((CONFIG_BT_BUF_CMD_TX_COUNT - 1) > 0,
	     "We need at least two HCI command buffers to avoid deadlocks.");
#endif /* CONFIG_BT_CONN && CONFIG_BT_HCI_HOST */

/*
 * This wrapper addresses a limitation in some Zephyr HCI samples such as Zephyr hci_ipc, namely the
 * dropping of Host Number of Completed Packets (HNCP) messages when buffers are full. Dropping
 * these messages causes a resource leak.
 *
 * The buffers are full when CONFIG_BT_BUF_CMD_TX_COUNT is exhausted. This wrapper implements a
 * workaround that ensures the buffers are never exhausted, by limiting the
 * Host_Total_Num_ACL_Data_Packets value. Limiting this value naturally limits the number of HNCP to
 * one the sample buffers can handle.
 *
 * Considering the sample uses the same buffer pool for normal commands, which take up one buffer at
 * most (look up `HCI_Command_Complete` for more details), this wrapper artifically limits the
 * Host_Total_Num_ACL_Data_Packets to at most one less than the CONFIG_BT_BUF_CMD_TX_COUNT pool
 * capacity.
 * .
 */
int sdc_hci_cmd_cb_host_buffer_size_wrapper(const sdc_hci_cmd_cb_host_buffer_size_t *cmd_params)
{
	sdc_hci_cmd_cb_host_buffer_size_t ctrl_cmd_params = *cmd_params;

	ctrl_cmd_params.host_total_num_acl_data_packets = MIN(
		ctrl_cmd_params.host_total_num_acl_data_packets, (CONFIG_BT_BUF_CMD_TX_COUNT - 1));

	return sdc_hci_cmd_cb_host_buffer_size(&ctrl_cmd_params);
}
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */
