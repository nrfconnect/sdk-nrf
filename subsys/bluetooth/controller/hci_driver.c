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
#include <misc/byteorder.h>
#include <stdbool.h>

#include <ble_controller.h>
#include <ble_controller_hci.h>
#include "multithreading_lock.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_hci_driver
#include "common/log.h"

#define BLE_CONTROLLER_IRQ_PRIO_LOW  4
#define BLE_CONTROLLER_IRQ_PRIO_HIGH 0

#if IS_ENABLED(CONFIG_SOC_SERIES_NRF52X)
	#define BLE_CONTROLLER_PROCESS_IRQn SWI5_IRQn
#elif IS_ENABLED(CONFIG_SOC_SERIES_NRF53X)
	#define BLE_CONTROLLER_PROCESS_IRQn EGU0_IRQn
#endif

static K_SEM_DEFINE(sem_recv, 0, 1);
static K_SEM_DEFINE(sem_signal, 0, UINT_MAX);

static struct k_thread recv_thread_data;
static struct k_thread signal_thread_data;
static K_THREAD_STACK_DEFINE(recv_thread_stack, CONFIG_BLECTLR_RX_STACK_SIZE);
static K_THREAD_STACK_DEFINE(signal_thread_stack,
			     CONFIG_BLECTLR_SIGNAL_STACK_SIZE);


/* It should not be possible to set CONFIG_BLECTRL_SLAVE_COUNT larger than
 * CONFIG_BT_MAX_CONN. Kconfig should make sure of that, this assert is to
 * verify that assumption.
 */
BUILD_ASSERT(CONFIG_BLECTRL_SLAVE_COUNT <= CONFIG_BT_MAX_CONN);

#define BLECTRL_MASTER_COUNT (CONFIG_BT_MAX_CONN - CONFIG_BLECTRL_SLAVE_COUNT)

BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_CENTRAL) ||
			 (BLECTRL_MASTER_COUNT > 0));

BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_PERIPHERAL) ||
			 (CONFIG_BLECTRL_SLAVE_COUNT > 0));

#ifdef CONFIG_BT_CTLR_DATA_LENGTH_MAX
	#define MAX_TX_PACKET_SIZE CONFIG_BT_CTLR_DATA_LENGTH_MAX
	#define MAX_RX_PACKET_SIZE CONFIG_BT_CTLR_DATA_LENGTH_MAX
#else
	#define MAX_TX_PACKET_SIZE BLE_CONTROLLER_DEFAULT_TX_PACKET_SIZE
	#define MAX_RX_PACKET_SIZE BLE_CONTROLLER_DEFAULT_RX_PACKET_SIZE
#endif

#define MASTER_MEM_SIZE (BLE_CONTROLLER_MEM_PER_MASTER_LINK( \
	MAX_TX_PACKET_SIZE, \
	MAX_RX_PACKET_SIZE, \
	BLE_CONTROLLER_DEFAULT_TX_PACKET_COUNT, \
	BLE_CONTROLLER_DEFAULT_RX_PACKET_COUNT) \
	+ BLE_CONTROLLER_MEM_MASTER_LINKS_SHARED)

#define SLAVE_MEM_SIZE (BLE_CONTROLLER_MEM_PER_SLAVE_LINK( \
	MAX_TX_PACKET_SIZE, \
	MAX_RX_PACKET_SIZE, \
	BLE_CONTROLLER_DEFAULT_TX_PACKET_COUNT, \
	BLE_CONTROLLER_DEFAULT_RX_PACKET_COUNT) \
	+ BLE_CONTROLLER_MEM_SLAVE_LINKS_SHARED)

#define MEMPOOL_SIZE ((CONFIG_BLECTRL_SLAVE_COUNT * SLAVE_MEM_SIZE) + \
		      (BLECTRL_MASTER_COUNT * MASTER_MEM_SIZE))

static u8_t ble_controller_mempool[MEMPOOL_SIZE];

#if IS_ENABLED(CONFIG_BT_CTLR_ASSERT_HANDLER)
extern void bt_ctlr_assert_handle(char *file, u32_t line);

void blectlr_assertion_handler(const char *const file, const u32_t line)
{
	bt_ctlr_assert_handle((char *) file, line);
}

#else /* !IS_ENABLED(CONFIG_BT_CTLR_ASSERT_HANDLER) */
void blectlr_assertion_handler(const char *const file, const u32_t line)
{
	BT_ERR("BleCtlr ASSERT: %s, %d", log_strdup(file), line);
	k_oops();
}
#endif /* IS_ENABLED(CONFIG_BT_CTLR_ASSERT_HANDLER) */


static int cmd_handle(struct net_buf *cmd)
{
	BT_DBG("");

	int errcode = MULTITHREADING_LOCK_ACQUIRE();

	if (!errcode) {
		errcode = hci_cmd_put(cmd->data);
		MULTITHREADING_LOCK_RELEASE();
	}
	if (errcode) {
		return errcode;
	}

	k_sem_give(&sem_recv);

	return 0;
}

#if defined(CONFIG_BT_CONN)
static int acl_handle(struct net_buf *acl)
{
	BT_DBG("");

	int errcode = MULTITHREADING_LOCK_ACQUIRE();

	if (!errcode) {
		errcode = hci_data_put(acl->data);
		MULTITHREADING_LOCK_RELEASE();

		if (errcode) {
			/* Likely buffer overflow event */
			k_sem_give(&sem_recv);
		}
	}

	return errcode;
}
#endif

static int hci_driver_send(struct net_buf *buf)
{
	int err;
	u8_t type;

	BT_DBG("");

	if (!buf->len) {
		BT_DBG("Empty HCI packet");
		return -EINVAL;
	}

	type = bt_buf_get_type(buf);
	switch (type) {
#if defined(CONFIG_BT_CONN)
	case BT_BUF_ACL_OUT:
		err = acl_handle(buf);
		break;
#endif          /* CONFIG_BT_CONN */
	case BT_BUF_CMD:
		err = cmd_handle(buf);
		break;
	default:
		BT_DBG("Unknown HCI type %u", type);
		return -EINVAL;
	}

	if (!err) {
		net_buf_unref(buf);
	}

	BT_DBG("Exit: %d", err);
	return err;
}

#ifndef bt_acl_flag_pb
/* Temporary defines, missing from hci.h */
#define bt_acl_flags_pb(h)		((h) >> 12 & BIT_MASK(2))
#define bt_acl_flags_bc(h)		((h) >> 14 & BIT_MASK(2))
#endif /* bt_acl_flag_pb */

static void data_packet_process(u8_t *hci_buf)
{
	struct net_buf *data_buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	struct bt_hci_acl_hdr *hdr = (void *)hci_buf;
	u16_t hf, handle, len;
	u8_t pb, bc;

	if (!data_buf) {
		BT_ERR("No data buffer available");
		return;
	}

	len = sys_le16_to_cpu(hdr->len);
	hf = sys_le16_to_cpu(hdr->handle);
	handle = bt_acl_handle(hf);
	pb = bt_acl_flags_pb(hf);
	bc = bt_acl_flags_bc(hf);

	BT_DBG("Data: handle (0x%02x), PB(%01d), BC(%01d), len(%u)", handle,
	       pb, bc, len);

	net_buf_add_mem(data_buf, &hci_buf[0], len + sizeof(*hdr));
	bt_recv(data_buf);
}

static void event_packet_process(u8_t *hci_buf)
{
	struct bt_hci_evt_hdr *hdr = (void *)hci_buf;
	struct net_buf *evt_buf;

	if (hdr->evt == BT_HCI_EVT_CMD_COMPLETE ||
	    hdr->evt == BT_HCI_EVT_CMD_STATUS) {
		evt_buf = bt_buf_get_cmd_complete(K_FOREVER);
	} else {
		evt_buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);
	}

	if (!evt_buf) {
		BT_ERR("No event buffer available");
		return;
	}

	if (hdr->evt == BT_HCI_EVT_LE_META_EVENT) {
		struct bt_hci_evt_le_meta_event *me = (void *)&hci_buf[2];

		BT_DBG("LE Meta Event (0x%02x), len (%u)",
		       me->subevent, hdr->len);
	} else if (hdr->evt == BT_HCI_EVT_CMD_COMPLETE) {
		struct bt_hci_evt_cmd_complete *cc = (void *)&hci_buf[2];
		struct bt_hci_evt_cc_status *ccs = (void *)&hci_buf[5];
		u16_t opcode = sys_le16_to_cpu(cc->opcode);

		BT_DBG("Command Complete (0x%04x) status: 0x%02x,"
		       " ncmd: %u, len %u",
		       opcode, ccs->status, cc->ncmd, hdr->len);
	} else if (hdr->evt == BT_HCI_EVT_CMD_STATUS) {
		struct bt_hci_evt_cmd_status *cs = (void *)&hci_buf[2];
		u16_t opcode = sys_le16_to_cpu(cs->opcode);

		BT_DBG("Command Status (0x%04x) status: 0x%02x",
		       opcode, cs->status);
	} else {
		BT_DBG("Event (0x%02x) len %u", hdr->evt, hdr->len);
	}

	net_buf_add_mem(evt_buf, &hci_buf[0], hdr->len + sizeof(*hdr));
	if (bt_hci_evt_is_prio(hdr->evt)) {
		bt_recv_prio(evt_buf);
	} else {
		bt_recv(evt_buf);
	}
}

static bool fetch_and_process_hci_evt(uint8_t *p_hci_buffer)
{
	int errcode;

	errcode = MULTITHREADING_LOCK_ACQUIRE();
	if (!errcode) {
		errcode = hci_evt_get(p_hci_buffer);
		MULTITHREADING_LOCK_RELEASE();
	}

	if (errcode) {
		return false;
	}

	event_packet_process(p_hci_buffer);
	return true;
}

static bool fetch_and_process_acl_data(uint8_t *p_hci_buffer)
{
	int errcode;

	errcode = MULTITHREADING_LOCK_ACQUIRE();
	if (!errcode) {
		errcode = hci_data_get(p_hci_buffer);
		MULTITHREADING_LOCK_RELEASE();
	}

	if (errcode) {
		return false;
	}

	data_packet_process(p_hci_buffer);
	return true;
}

static void recv_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	static u8_t hci_buffer[HCI_MSG_BUFFER_MAX_SIZE];

	bool received_evt = false;
	bool received_data = false;

	while (true) {
		if (!received_evt && !received_data) {
			/* Wait for a signal from the controller. */
			k_sem_take(&sem_recv, K_FOREVER);
		}

		received_evt = fetch_and_process_hci_evt(&hci_buffer[0]);

		received_data = fetch_and_process_acl_data(&hci_buffer[0]);

		/* Let other threads of same priority run in between. */
		k_yield();
	}
}

static void signal_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&sem_signal, K_FOREVER);
		ble_controller_low_prio_tasks_process();
	}
}

static int hci_driver_open(void)
{
	BT_DBG("Open");

	k_thread_create(&recv_thread_data, recv_thread_stack,
			K_THREAD_STACK_SIZEOF(recv_thread_stack), recv_thread,
			NULL, NULL, NULL, K_PRIO_COOP(CONFIG_BLECTLR_PRIO), 0,
			K_NO_WAIT);

	u8_t build_revision[BLE_CONTROLLER_BUILD_REVISION_SIZE];

	ble_controller_build_revision_get(build_revision);
	LOG_HEXDUMP_INF(build_revision, sizeof(build_revision),
			"BLE controller build revision: ");

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

u8_t bt_read_static_addr(bt_addr_le_t *addr)
{
	if (((NRF_FICR->DEVICEADDR[0] != UINT32_MAX) ||
	     ((NRF_FICR->DEVICEADDR[1] & UINT16_MAX) != UINT16_MAX)) &&
	    (NRF_FICR->DEVICEADDRTYPE & 0x01)) {
		sys_put_le32(NRF_FICR->DEVICEADDR[0], &addr->a.val[0]);
		sys_put_le16(NRF_FICR->DEVICEADDR[1], &addr->a.val[4]);

		/* The FICR value is a just a random number, with no knowledge
		 * of the Bluetooth Specification requirements for random
		 * static addresses.
		 */
		BT_ADDR_SET_STATIC(&addr->a);

		addr->type = BT_ADDR_LE_RANDOM;
		return 1;
	}
	return 0;
}

static int ble_init(struct device *unused)
{
	int err = 0;
	nrf_lf_clock_cfg_t clock_cfg;

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
	clock_cfg.lf_clk_source = NRF_LF_CLOCK_SRC_RC;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL
	clock_cfg.lf_clk_source = NRF_LF_CLOCK_SRC_XTAL;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH
	clock_cfg.lf_clk_source = NRF_LF_CLOCK_SRC_SYNTH;
#else
#error "Clock source is not defined"
#endif

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_500PPM
	clock_cfg.accuracy = NRF_LF_CLOCK_ACCURACY_500_PPM;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_250PPM
	clock_cfg.accuracy = NRF_LF_CLOCK_ACCURACY_250_PPM;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_150PPM
	clock_cfg.accuracy = NRF_LF_CLOCK_ACCURACY_150_PPM;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_100PPM
	clock_cfg.accuracy = NRF_LF_CLOCK_ACCURACY_100_PPM;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_75PPM
	clock_cfg.accuracy = NRF_LF_CLOCK_ACCURACY_75_PPM;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_50PPM
	clock_cfg.accuracy = NRF_LF_CLOCK_ACCURACY_50_PPM;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_30PPM
	clock_cfg.accuracy = NRF_LF_CLOCK_ACCURACY_30_PPM;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_20PPM
	clock_cfg.accuracy = NRF_LF_CLOCK_ACCURACY_20_PPM;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_10PPM
	clock_cfg.accuracy = NRF_LF_CLOCK_ACCURACY_10_PPM;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_5PPM
	clock_cfg.accuracy = NRF_LF_CLOCK_ACCURACY_5_PPM;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_2PPM
	clock_cfg.accuracy = NRF_LF_CLOCK_ACCURACY_2_PPM;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_1PPM
	clock_cfg.accuracy = NRF_LF_CLOCK_ACCURACY_1_PPM;
#else
#error "Clock accuracy is not defined"
#endif
	clock_cfg.rc_ctiv = BLE_CONTROLLER_RECOMMENDED_RC_CTIV;
	clock_cfg.rc_temp_ctiv = BLE_CONTROLLER_RECOMMENDED_RC_TEMP_CTIV;

	err = ble_controller_init(blectlr_assertion_handler,
				  &clock_cfg,
				  BLE_CONTROLLER_PROCESS_IRQn
		);
	return err;
}

static int ble_enable(void)
{
	int err;
	int required_memory;
	ble_controller_cfg_t cfg;

	cfg.master_count.count = BLECTRL_MASTER_COUNT;

	/* NOTE: ble_controller_cfg_set() returns a negative errno on error. */
	required_memory =
		ble_controller_cfg_set(BLE_CONTROLLER_DEFAULT_RESOURCE_CFG_TAG,
				       BLE_CONTROLLER_CFG_TYPE_MASTER_COUNT,
				       &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

	cfg.slave_count.count = CONFIG_BLECTRL_SLAVE_COUNT;

	required_memory =
		ble_controller_cfg_set(BLE_CONTROLLER_DEFAULT_RESOURCE_CFG_TAG,
				       BLE_CONTROLLER_CFG_TYPE_SLAVE_COUNT,
				       &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

	cfg.buffer_cfg.rx_packet_size = MAX_RX_PACKET_SIZE;
	cfg.buffer_cfg.tx_packet_size = MAX_TX_PACKET_SIZE;
	cfg.buffer_cfg.rx_packet_count = BLE_CONTROLLER_DEFAULT_RX_PACKET_COUNT;
	cfg.buffer_cfg.tx_packet_count = BLE_CONTROLLER_DEFAULT_TX_PACKET_COUNT;

	required_memory =
		ble_controller_cfg_set(BLE_CONTROLLER_DEFAULT_RESOURCE_CFG_TAG,
				       BLE_CONTROLLER_CFG_TYPE_BUFFER_CFG,
				       &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

	cfg.event_length.event_length_us =
		CONFIG_BLECTRL_MAX_CONN_EVENT_LEN_DEFAULT;
	required_memory =
		ble_controller_cfg_set(BLE_CONTROLLER_DEFAULT_RESOURCE_CFG_TAG,
				       BLE_CONTROLLER_CFG_TYPE_EVENT_LENGTH,
				       &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

	BT_DBG("BT mempool size: %u, required: %u",
	       sizeof(ble_controller_mempool), required_memory);

	if (required_memory > sizeof(ble_controller_mempool)) {
		BT_ERR("Allocated memory too low: %u < %u",
		       sizeof(ble_controller_mempool), required_memory);
		k_panic();
		/* No return from k_panic(). */
		return -ENOMEM;
	}

	err = MULTITHREADING_LOCK_ACQUIRE();
	if (!err) {
		err = ble_controller_enable(host_signal,
					    ble_controller_mempool);
		MULTITHREADING_LOCK_RELEASE();
	}
	if (err < 0) {
		return err;
	}

	/* Start processing software interrupts. This enables, e.g., the flash
	 * API to work without having to call bt_enable(), which in turn calls
	 * hci_driver_open().
	 *
	 * FIXME: Here we possibly start dynamic behavior during initialization,
	 * which in general is a bad thing.
	 */
	k_thread_create(&signal_thread_data, signal_thread_stack,
			K_THREAD_STACK_SIZEOF(signal_thread_stack),
			signal_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BLECTLR_PRIO), 0, K_NO_WAIT);

	return 0;
}

ISR_DIRECT_DECLARE(ble_controller_radio_isr_wrapper)
{
	ble_controller_RADIO_IRQHandler();

	ISR_DIRECT_PM();

	/* We may need to reschedule in case a radio timeslot callback
	 * accesses zephyr primitives.
	 */
	return 1;
}

ISR_DIRECT_DECLARE(ble_controller_rtc0_isr_wrapper)
{
	ble_controller_RTC0_IRQHandler();

	ISR_DIRECT_PM();

	/* No need for rescheduling, because the interrupt handler
	 * does not access zephyr primitives.
	 */
	return 0;
}

ISR_DIRECT_DECLARE(ble_controller_timer0_isr_wrapper)
{
	ble_controller_TIMER0_IRQHandler();

	ISR_DIRECT_PM();

	/* We may need to reschedule in case a radio timeslot callback
	 * accesses zephyr primitives.
	 */
	return 1;
}

static int hci_driver_init(struct device *unused)
{
	ARG_UNUSED(unused);

	bt_hci_driver_register(&drv);

	int err = 0;

	err = ble_enable();

	if (err < 0) {
		return err;
	}

	IRQ_DIRECT_CONNECT(RADIO_IRQn, BLE_CONTROLLER_IRQ_PRIO_HIGH,
			   ble_controller_radio_isr_wrapper, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(RTC0_IRQn, BLE_CONTROLLER_IRQ_PRIO_HIGH,
			   ble_controller_rtc0_isr_wrapper, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(TIMER0_IRQn, BLE_CONTROLLER_IRQ_PRIO_HIGH,
			   ble_controller_timer0_isr_wrapper, IRQ_ZERO_LATENCY);

	IRQ_CONNECT(BLE_CONTROLLER_PROCESS_IRQn, BLE_CONTROLLER_IRQ_PRIO_LOW,
		    SIGNALLING_Handler, NULL, 0);


	return 0;
}

SYS_INIT(hci_driver_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
SYS_INIT(ble_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
