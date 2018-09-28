/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/hci_driver.h>
#include <init.h>
#include <irq.h>
#include <kernel.h>
#include <soc.h>

#include <blectlr.h>
#include <blectlr_hci.h>
#include <blectlr_util.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#include "common/log.h"

#define SIGNAL_HANDLER_STACK_SIZE 1024

static K_SEM_DEFINE(sem_recv, 0, UINT_MAX);
static K_SEM_DEFINE(sem_signal, 0, UINT_MAX);

static struct k_thread recv_thread_data;
static struct k_thread signal_thread_data;
static BT_STACK_NOINIT(recv_thread_stack, CONFIG_BT_RX_STACK_SIZE);
static BT_STACK_NOINIT(signal_thread_stack, SIGNAL_HANDLER_STACK_SIZE);

void blectlr_assertion_handler(const char *const file, const u32_t line)
{
#ifdef CONFIG_BT_CTLR_ASSERT_HANDLER
	bt_ctlr_assert_handle(file, line);
#else
	BT_ERR("BleCtlr ASSERT: %s, %d", file, line);
	k_oops();
#endif
}

static int cmd_handle(struct net_buf *cmd)
{
	const bool pkt_put = hci_cmd_packet_put(cmd->data);

	if (!pkt_put) {
		return -ENOBUFS;
	}

	k_sem_give(&sem_recv);

	return 0;
}

static int acl_handle(struct net_buf *acl)
{
	const bool pkt_put = hci_data_packet_put(acl->data);

	if (!pkt_put) {
		/* Likely buffer overflow event */
		k_sem_give(&sem_recv);
		return -ENOBUFS;
	}

	return 0;
}

static int hci_driver_send(struct net_buf *buf)
{
	int err;
	u8_t type;

	BT_DBG("Enter");

	if (!buf->len) {
		BT_DBG("Empty HCI packet");
		return -EINVAL;
	}

	type = bt_buf_get_type(buf);
	switch (type) {
#if defined(CONFIG_BT_CONN)
	case BT_BUF_ACL_OUT:
		BT_DBG("ACL_OUT");
		err = acl_handle(buf);
		break;
#endif /* CONFIG_BT_CONN */
	case BT_BUF_CMD:
		BT_DBG("CMD");
		err = cmd_handle(buf);
		break;
	default:
		BT_DBG("Unknown HCI type %u", type);
		return -EINVAL;
	}

	if (!err) {
		net_buf_unref(buf);
	}

	BT_DBG("Exit");
	return err;
}

static void data_packet_process(u8_t *hci_buf)
{
	struct net_buf *data_buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);

	if (!data_buf) {
		BT_ERR("No data buffer available");
		return;
	}

	u16_t handle = hci_buf[0] | (hci_buf[1] & 0xF) << 8;
	u16_t data_length = hci_buf[2] | hci_buf[3] << 8;
	u8_t pb_flag = (hci_buf[1] >> 4) & 0x3;
	u8_t bc_flag = (hci_buf[1] >> 6) & 0x3;

	BT_DBG("Data: Handle(%02x), PB(%01d), "
	       "BC(%01d), Length(%02x)",
	       handle, pb_flag, bc_flag, data_length);

	net_buf_add_mem(data_buf, &hci_buf[0], data_length + 4);
	bt_recv(data_buf);
}

static void event_packet_process(u8_t *hci_buf)
{
	struct bt_hci_evt_hdr *hdr = (void *)hci_buf;
	struct net_buf *evt_buf;

	if (hdr->evt == BT_HCI_EVT_CMD_COMPLETE ||
	    hdr->evt == BT_HCI_EVT_CMD_STATUS) {
		u16_t opcode = hci_buf[3] | hci_buf[4] << 8;

		if (opcode == 0xC03) {
			BT_DBG("Reset command complete");
			cal_init();
			blectlr_set_default_evt_length();
		}

		evt_buf = bt_buf_get_cmd_complete(K_FOREVER);
	} else {
		evt_buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);
	}

	if (!evt_buf) {
		BT_ERR("No event buffer available");
		return;
	}

	if (hdr->evt == 0x3E) {
		BT_DBG("LE Meta Event: subevent code "
		       "(%02x), length (%d)",
		       hci_buf[2], hci_buf[1]);
	} else {
		BT_DBG("Event: event code (%02x), "
		       "length (%d)",
		       hci_buf[0], hci_buf[1]);
	}

	net_buf_add_mem(evt_buf, &hci_buf[0], hdr->len + 2);
	if (bt_hci_evt_is_prio(hdr->evt)) {
		bt_recv_prio(evt_buf);
	} else {
		bt_recv(evt_buf);
	}
}

static void recv_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	static u8_t hci_buffer[256 + 4];
	bool pkt;

	BT_DBG("Started");
	while (1) {
		k_sem_take(&sem_recv, K_FOREVER);

		pkt = hci_data_packet_get(hci_buffer);
		if (pkt) {
			data_packet_process(hci_buffer);
		}

		pkt = hci_event_packet_get(hci_buffer);
		if (pkt) {
			event_packet_process(hci_buffer);
		}

		/* Let other threads of same priority run in between. */
		k_yield();
	}
}

void _signal_handler_irq(void)
{
	k_sem_give(&sem_recv);
}

static void signal_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&sem_signal, K_FOREVER);
		blectlr_signal();
	}
}

static int hci_driver_open(void)
{
	BT_DBG("Open");

	k_thread_create(&recv_thread_data, recv_thread_stack,
			K_THREAD_STACK_SIZEOF(recv_thread_stack), recv_thread,
			NULL, NULL, NULL, K_PRIO_COOP(CONFIG_BT_RX_PRIO), 0,
			K_NO_WAIT);

	k_thread_create(&signal_thread_data, signal_thread_stack,
			K_THREAD_STACK_SIZEOF(signal_thread_stack),
			signal_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_RX_PRIO), 0, K_NO_WAIT);

	return 0;
}

static const struct bt_hci_driver drv = {
	.name = "Controller",
	.bus = BT_HCI_DRIVER_BUS_VIRTUAL,
	.open = hci_driver_open,
	.send = hci_driver_send,
};

void host_signal(void)
{
	/* Wake up the RX event/data thread */
	k_sem_give(&sem_recv);
}

void SIGNALLING_Handler(void)
{
	k_sem_give(&sem_signal);
}

static int hci_driver_init(struct device *unused)
{
	ARG_UNUSED(unused);

	u32_t err = blectlr_init(host_signal);

	if (err) {
		/* Probably memory */
		return -ENOMEM;
	}

	bt_hci_driver_register(&drv);

	IRQ_DIRECT_CONNECT(NRF5_IRQ_RADIO_IRQn, 0,
			   C_RADIO_Handler, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(NRF5_IRQ_RTC0_IRQn, 0,
			   C_RTC0_Handler, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(NRF5_IRQ_TIMER0_IRQn, 0,
			   C_TIMER0_Handler, IRQ_ZERO_LATENCY);
	IRQ_CONNECT(NRF5_IRQ_SWI5_IRQn, 4, SIGNALLING_Handler, NULL, 0);
	IRQ_DIRECT_CONNECT(NRF5_IRQ_RNG_IRQn, 4, C_RNG_Handler, 0);
	IRQ_DIRECT_CONNECT(NRF5_IRQ_POWER_CLOCK_IRQn, 4,
			   C_POWER_CLOCK_Handler, 0);

	irq_enable(NRF5_IRQ_RADIO_IRQn);
	irq_enable(NRF5_IRQ_RTC0_IRQn);
	irq_enable(NRF5_IRQ_TIMER0_IRQn);
	irq_enable(NRF5_IRQ_SWI5_IRQn);
	irq_enable(NRF5_IRQ_RNG_IRQn);
	irq_enable(NRF5_IRQ_POWER_CLOCK_IRQn);

	return 0;
}

SYS_INIT(hci_driver_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
