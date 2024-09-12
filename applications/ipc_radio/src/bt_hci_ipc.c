/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/net_buf.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_raw.h>

#include <zephyr/ipc/ipc_service.h>

#if defined(CONFIG_BT_HCI_VS_FATAL_ERROR)
#include <zephyr/logging/log_ctrl.h>
#endif /* CONFIG_BT_HCI_VS_FATAL_ERROR */

#include <zephyr/logging/log.h>

#include "ipc_bt.h"

LOG_MODULE_DECLARE(ipc_radio, CONFIG_IPC_RADIO_LOG_LEVEL);

#if defined(CONFIG_BT_CTLR_ASSERT_HANDLER) || defined(CONFIG_BT_HCI_VS_FATAL_ERROR)
static bool ipc_ept_ready;
#endif /* CONFIG_BT_CTLR_ASSERT_HANDLER || CONFIG_BT_HCI_VS_FATAL_ERROR */

static K_SEM_DEFINE(ipc_bound_sem, 0, 1);

static struct ipc_ept hci_ept;

static K_FIFO_DEFINE(tx_queue);
static K_FIFO_DEFINE(rx_queue);

enum hci_h4_type {
	HCI_H4_CMD = 0x01, /* rx */
	HCI_H4_ACL = 0x02, /* rx */
	HCI_H4_EVT = 0x04, /* tx */
	HCI_H4_ISO = 0x05  /* rx */
};

#define HCI_FATAL_MSG true
#define HCI_REGULAR_MSG false

static void recv_cmd(const uint8_t *data, size_t len)
{
	const struct bt_hci_cmd_hdr *hdr = (const struct bt_hci_cmd_hdr *)data;
	struct net_buf *buf;

	if (len < sizeof(*hdr)) {
		LOG_ERR("Not enough data for command header.");
		return;
	}

	if ((len - sizeof(*hdr)) != hdr->param_len) {
		LOG_ERR("Command param_len does not match the remaining data length.");
		return;
	}

	buf = bt_buf_get_tx(BT_BUF_CMD, K_NO_WAIT, hdr, sizeof(*hdr));
	if (!buf) {
		LOG_ERR("No available command buffers.");
		return;
	}

	data += sizeof(*hdr);
	len -= sizeof(*hdr);

	if (len > net_buf_tailroom(buf)) {
		LOG_ERR("Not enough space in buffer.");
		net_buf_unref(buf);
		return;
	}

	LOG_DBG("Received HCI CMD packet (opcode: %#x, len: %u).",
						sys_le16_to_cpu(hdr->opcode), hdr->param_len);

	net_buf_add_mem(buf, data, len);
	net_buf_put(&tx_queue, buf);
}

static void recv_acl(const uint8_t *data, size_t len)
{
	const struct bt_hci_acl_hdr *hdr = (const struct bt_hci_acl_hdr *)data;
	struct net_buf *buf;

	if (len < sizeof(*hdr)) {
		LOG_ERR("Not enough data for ACL header.");
		return;
	}

	if ((len - sizeof(*hdr)) != sys_le16_to_cpu(hdr->len)) {
		LOG_ERR("ACL payload length does not match the remaining data length.");
		return;
	}

	buf = bt_buf_get_tx(BT_BUF_ACL_OUT, K_NO_WAIT, hdr, sizeof(*hdr));
	if (!buf) {
		LOG_ERR("No available ACL buffers.");
		return;
	}

	data += sizeof(*hdr);
	len -= sizeof(*hdr);

	if (len > net_buf_tailroom(buf)) {
		LOG_ERR("Not enough space in buffer.");
		net_buf_unref(buf);
		return;
	}

	LOG_DBG("Received HCI ACL packet (handle: %u, len: %u).",
					sys_le16_to_cpu(hdr->handle), sys_le16_to_cpu(hdr->len));

	net_buf_add_mem(buf, data, len);
	net_buf_put(&tx_queue, buf);
}

static void recv_iso(const uint8_t *data, size_t len)
{
	const struct bt_hci_iso_hdr *hdr = (const struct bt_hci_iso_hdr *)data;
	struct net_buf *buf;

	if (len < sizeof(*hdr)) {
		LOG_ERR("Not enough data for ISO header.");
		return;
	}

	if ((len - sizeof(*hdr)) != bt_iso_hdr_len(sys_le16_to_cpu(hdr->len))) {
		LOG_ERR("ISO payload length does not match the remaining data length.");
		return;
	}

	buf = bt_buf_get_tx(BT_BUF_ISO_OUT, K_NO_WAIT, hdr, sizeof(*hdr));
	if (!buf) {
		LOG_ERR("No available ISO buffers.");
		return;
	}

	data += sizeof(*hdr);
	len -= sizeof(*hdr);

	if (len > net_buf_tailroom(buf)) {
		LOG_ERR("Not enough space in buffer.");
		net_buf_unref(buf);
		return;
	}

	LOG_DBG("Received HCI ISO packet (handle: %u, len: %u).",
					sys_le16_to_cpu(hdr->handle), sys_le16_to_cpu(hdr->len));

	net_buf_add_mem(buf, data, len);
	net_buf_put(&tx_queue, buf);
}

static void send(struct net_buf *buf, bool is_fatal_err)
{
	uint8_t type;
	uint8_t retries = 0;
	int ret;

	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "Controller buffer:");

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_IN:
		type = HCI_H4_ACL;
		break;
	case BT_BUF_EVT:
		type = HCI_H4_EVT;
		break;
	case BT_BUF_ISO_IN:
		type = HCI_H4_ISO;
		break;
	default:
		LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
		net_buf_unref(buf);
		return;
	}
	net_buf_push_u8(buf, type);

	LOG_HEXDUMP_DBG(buf->data, buf->len, "Final HCI buffer:");

	do {
		ret = ipc_service_send(&hci_ept, buf->data, buf->len);
		if (ret < 0) {
			retries++;
			if (retries > 10) {
				LOG_WRN("IPC send has been blocked during 10 retires.");
				retries = 0;
			}

			if (is_fatal_err) {
				LOG_ERR("IPC service send error: %d", ret);
			} else {
				k_yield();
			}
		}
	} while (ret < 0);

	LOG_INF("Sent message of %d bytes.", ret);

	net_buf_unref(buf);
}

static void bound(void *priv)
{
#if defined(CONFIG_BT_CTLR_ASSERT_HANDLER) || defined(CONFIG_BT_HCI_VS_FATAL_ERROR)
	ipc_ept_ready = true;
#endif /* CONFIG_BT_CTLR_ASSERT_HANDLER || CONFIG_BT_HCI_VS_FATAL_ERROR */

	k_sem_give(&ipc_bound_sem);
}

static void recv(const void *data, size_t len, void *priv)
{
	const uint8_t *tmp = (const uint8_t *)data;
	enum hci_h4_type type;

	LOG_INF("Received hci message of %u bytes.", len);
	LOG_HEXDUMP_DBG(data, len, "HCI data:");

	type = (enum hci_h4_type)*tmp++;
	len -= sizeof(type);

	switch (type) {
	case HCI_H4_CMD:
		recv_cmd(tmp, len);
		break;

	case HCI_H4_ACL:
		recv_acl(tmp, len);
		break;

	case HCI_H4_ISO:
		recv_iso(tmp, len);
		break;

	default:
		LOG_ERR("Unknown HCI type %u.", type);
		return;
	}
}

static void tx_thread(void)
{
	struct net_buf *buf;
	int err;

	while (1) {
		buf = net_buf_get(&tx_queue, K_FOREVER);
		err = bt_send(buf);
		if (err) {
			LOG_ERR("bt_send failed err: %d.", err);
			net_buf_unref(buf);
		}

		k_yield();
	}
}

K_THREAD_DEFINE(tx_thread_id, CONFIG_BT_HCI_TX_STACK_SIZE, tx_thread,
		NULL, NULL, NULL,
		CONFIG_BT_HCI_TX_PRIO, 0, 0);

static struct ipc_ept_cfg hci_ept_cfg = {
	.name = "nrf_bt_hci",
	.cb = {
		.bound    = bound,
		.received = recv,
	},
};

#if defined(CONFIG_BT_CTLR_ASSERT_HANDLER)
void bt_ctlr_assert_handle(char *file, uint32_t line)
{
	(void)irq_lock();

	LOG_ERR("HCI Fatal error in: %s at %d.", file, line);

#if defined(CONFIG_BT_HCI_VS_FATAL_ERROR)
	if (ipc_ept_ready) {
		struct net_buf *buf;

		buf = hci_vs_err_assert(file, line);
		if (!buf) {
			send(buf, HCI_FATAL_MSG);
		} else {
			LOG_ERR("Can't send Fatal Error HCI event.");
		}
	} else {
		LOG_ERR("HCI Fatal error before IPC endpoint is ready.");
	}

#else /* !CONFIG_BT_HCI_VS_FATAL_ERROR */
	LOG_ERR("Controller assert in: %s at %d.", file, line);

#endif /* !CONFIG_BT_HCI_VS_FATAL_ERROR */

	for (;;) {
	};
}
#endif /* CONFIG_BT_CTLR_ASSERT_HANDLER */

#if defined(CONFIG_BT_HCI_VS_FATAL_ERROR)
void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	LOG_PANIC();

	(void)irq_lock();

	if ((!esf) && (ipc_ept_ready)) {
		struct net_buf *buf;

		buf = hci_vs_err_stack_frame(reason, esf);
		if (!buf) {
			send(buf, HCI_FATAL_MSG);
		} else {
			LOG_ERR("Can't create Fatal Error HCI event.");
		}
	}

	for (;;) {
	};

	CODE_UNREACHABLE;
}
#endif /* CONFIG_BT_HCI_VS_FATAL_ERROR */

int ipc_bt_init(void)
{
	int err;
	const struct device *hci_ipc_instance = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_hci_ipc));

	LOG_INF("Initializing ipc_radio bt_hci.");

	err = bt_enable_raw(&rx_queue);
	if (err) {
		LOG_ERR("bt_enable_raw failed: %d.", err);
		return err;
	}

	err = ipc_service_open_instance(hci_ipc_instance);
	if ((err < 0) && (err != -EALREADY)) {
		LOG_ERR("IPC service instance initialization failed: %d.", err);
		return err;
	}

	err = ipc_service_register_endpoint(hci_ipc_instance, &hci_ept, &hci_ept_cfg);
	if (err) {
		LOG_ERR("Registering endpoint failed: %d.", err);
		return err;
	}

	return 0;
}

int ipc_bt_process(void)
{
	struct net_buf *buf;

	k_sem_take(&ipc_bound_sem, K_FOREVER);

	while (1) {
		buf = net_buf_get(&rx_queue, K_FOREVER);
		send(buf, HCI_REGULAR_MSG);
	}

	return 0;
}
