/*
 * Copyright (c) 2018 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <drivers/bluetooth/hci_driver.h>
#include <bluetooth/controller.h>
#include <bluetooth/hci_vs.h>
#include <init.h>
#include <irq.h>
#include <kernel.h>
#include <soc.h>
#include <sys/byteorder.h>
#include <stdbool.h>

#include <sdc.h>
#include <sdc_hci.h>
#include <sdc_hci_vs.h>
#include "multithreading_lock.h"
#include "hci_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME sdc_hci_driver
#include "common/log.h"

/* As per the section "SoftDevice Controller/Integration with applications"
 * in the nrfxlib documentation, the controller uses the following channels:
 */
#if defined(PPI_PRESENT)
	/* PPI channels 17 - 31, for the nRF52 Series */
	#define PPI_CHANNELS_USED_BY_CTLR (BIT_MASK(15) << 17)
#else
	/* DPPI channels 0 - 13, for the nRF53 Series */
	#define PPI_CHANNELS_USED_BY_CTLR BIT_MASK(14)
#endif

/* Additionally, MPSL requires the following channels (as per the section
 * "Multiprotocol Service Layer/Integration notes"):
 */
#if defined(PPI_PRESENT)
	/* PPI channel 19, 30, 31, for the nRF52 Series */
	#define PPI_CHANNELS_USED_BY_MPSL (BIT(19) | BIT(30) | BIT(31))
#else
	/* DPPI channels 0 - 2, for the nRF53 Series */
	#define PPI_CHANNELS_USED_BY_MPSL BIT_MASK(3)
#endif

/* The following two constants are used in nrfx_glue.h for marking these PPI
 * channels and groups as occupied and thus unavailable to other modules.
 */
const uint32_t z_bt_ctlr_used_nrf_ppi_channels =
	PPI_CHANNELS_USED_BY_CTLR | PPI_CHANNELS_USED_BY_MPSL;
const uint32_t z_bt_ctlr_used_nrf_ppi_groups;

static K_SEM_DEFINE(sem_recv, 0, 1);

static struct k_thread recv_thread_data;
static K_THREAD_STACK_DEFINE(recv_thread_stack, CONFIG_SDC_RX_STACK_SIZE);

#if defined(CONFIG_BT_CONN)
/* It should not be possible to set CONFIG_SDC_SLAVE_COUNT larger than
 * CONFIG_BT_MAX_CONN. Kconfig should make sure of that, this assert is to
 * verify that assumption.
 */
BUILD_ASSERT(CONFIG_SDC_SLAVE_COUNT <= CONFIG_BT_MAX_CONN);

#define SDC_MASTER_COUNT (CONFIG_BT_MAX_CONN - CONFIG_SDC_SLAVE_COUNT)

#else

#define SDC_MASTER_COUNT 0

#endif /* CONFIG_BT_CONN */

BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_CENTRAL) ||
			 (SDC_MASTER_COUNT > 0));

BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_PERIPHERAL) ||
			 (CONFIG_SDC_SLAVE_COUNT > 0));

#ifdef CONFIG_BT_CTLR_DATA_LENGTH_MAX
	#define MAX_TX_PACKET_SIZE CONFIG_BT_CTLR_DATA_LENGTH_MAX
	#define MAX_RX_PACKET_SIZE CONFIG_BT_CTLR_DATA_LENGTH_MAX
#else
	#define MAX_TX_PACKET_SIZE SDC_DEFAULT_TX_PACKET_SIZE
	#define MAX_RX_PACKET_SIZE SDC_DEFAULT_RX_PACKET_SIZE
#endif

#define MASTER_MEM_SIZE (SDC_MEM_PER_MASTER_LINK( \
	MAX_TX_PACKET_SIZE, \
	MAX_RX_PACKET_SIZE, \
	SDC_DEFAULT_TX_PACKET_COUNT, \
	SDC_DEFAULT_RX_PACKET_COUNT) \
	+ SDC_MEM_MASTER_LINKS_SHARED)

#define SLAVE_MEM_SIZE (SDC_MEM_PER_SLAVE_LINK( \
	MAX_TX_PACKET_SIZE, \
	MAX_RX_PACKET_SIZE, \
	SDC_DEFAULT_TX_PACKET_COUNT, \
	SDC_DEFAULT_RX_PACKET_COUNT) \
	+ SDC_MEM_SLAVE_LINKS_SHARED)

#define MEMPOOL_SIZE ((CONFIG_SDC_SLAVE_COUNT * SLAVE_MEM_SIZE) + \
		      (SDC_MASTER_COUNT * MASTER_MEM_SIZE))

static uint8_t sdc_mempool[MEMPOOL_SIZE];

#if IS_ENABLED(CONFIG_BT_CTLR_ASSERT_HANDLER)
extern void bt_ctlr_assert_handle(char *file, uint32_t line);

void sdc_assertion_handler(const char *const file, const uint32_t line)
{
	bt_ctlr_assert_handle((char *) file, line);
}

#else /* !IS_ENABLED(CONFIG_BT_CTLR_ASSERT_HANDLER) */
void sdc_assertion_handler(const char *const file, const uint32_t line)
{
	BT_ERR("SoftDevice Controller ASSERT: %s, %d", log_strdup(file), line);
	k_oops();
}
#endif /* IS_ENABLED(CONFIG_BT_CTLR_ASSERT_HANDLER) */


static int cmd_handle(struct net_buf *cmd)
{
	BT_DBG("");

	int errcode = MULTITHREADING_LOCK_ACQUIRE();

	if (!errcode) {
		errcode = hci_internal_cmd_put(cmd->data);
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
		errcode = sdc_hci_data_put(acl->data);
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
	uint8_t type;

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

static void data_packet_process(uint8_t *hci_buf)
{
	struct net_buf *data_buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	struct bt_hci_acl_hdr *hdr = (void *)hci_buf;
	uint16_t hf, handle, len;
	uint8_t flags, pb, bc;

	if (!data_buf) {
		BT_ERR("No data buffer available");
		return;
	}

	len = sys_le16_to_cpu(hdr->len);
	hf = sys_le16_to_cpu(hdr->handle);
	handle = bt_acl_handle(hf);
	flags = bt_acl_flags(hf);
	pb = bt_acl_flags_pb(flags);
	bc = bt_acl_flags_bc(flags);

	BT_DBG("Data: handle (0x%02x), PB(%01d), BC(%01d), len(%u)", handle,
	       pb, bc, len);

	net_buf_add_mem(data_buf, &hci_buf[0], len + sizeof(*hdr));
	bt_recv(data_buf);
}

static bool event_packet_is_discardable(const uint8_t *hci_buf)
{
	struct bt_hci_evt_hdr *hdr = (void *)hci_buf;

	switch (hdr->evt) {
	case BT_HCI_EVT_LE_META_EVENT: {
		struct bt_hci_evt_le_meta_event *me = (void *)&hci_buf[2];

		switch (me->subevent) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
		case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT:
			return true;
		default:
			return false;
		}
	}
	case BT_HCI_EVT_VENDOR:
	{
		uint8_t subevent = hci_buf[2];

		switch (subevent) {
		case SDC_HCI_SUBEVENT_VS_QOS_CONN_EVENT_REPORT:
			return true;
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

static void event_packet_process(uint8_t *hci_buf)
{
	bool discardable = event_packet_is_discardable(hci_buf);
	struct bt_hci_evt_hdr *hdr = (void *)hci_buf;
	struct net_buf *evt_buf;

	if (hdr->evt == BT_HCI_EVT_LE_META_EVENT) {
		struct bt_hci_evt_le_meta_event *me = (void *)&hci_buf[2];

		BT_DBG("LE Meta Event (0x%02x), len (%u)",
		       me->subevent, hdr->len);
	} else if (hdr->evt == BT_HCI_EVT_CMD_COMPLETE) {
		struct bt_hci_evt_cmd_complete *cc = (void *)&hci_buf[2];
		struct bt_hci_evt_cc_status *ccs = (void *)&hci_buf[5];
		uint16_t opcode = sys_le16_to_cpu(cc->opcode);

		BT_DBG("Command Complete (0x%04x) status: 0x%02x,"
		       " ncmd: %u, len %u",
		       opcode, ccs->status, cc->ncmd, hdr->len);
	} else if (hdr->evt == BT_HCI_EVT_CMD_STATUS) {
		struct bt_hci_evt_cmd_status *cs = (void *)&hci_buf[2];
		uint16_t opcode = sys_le16_to_cpu(cs->opcode);

		BT_DBG("Command Status (0x%04x) status: 0x%02x",
		       opcode, cs->status);
	} else {
		BT_DBG("Event (0x%02x) len %u", hdr->evt, hdr->len);
	}

	evt_buf = bt_buf_get_evt(hdr->evt, discardable,
				 discardable ? K_NO_WAIT : K_FOREVER);

	if (!evt_buf) {
		if (discardable) {
			BT_DBG("Discarding event");
			return;
		}

		BT_ERR("No event buffer available");
		return;
	}

	net_buf_add_mem(evt_buf, &hci_buf[0], hdr->len + sizeof(*hdr));
	bt_recv(evt_buf);
}

static bool fetch_and_process_hci_evt(uint8_t *p_hci_buffer)
{
	int errcode;

	errcode = MULTITHREADING_LOCK_ACQUIRE();
	if (!errcode) {
		errcode = hci_internal_evt_get(p_hci_buffer);
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
		errcode = sdc_hci_data_get(p_hci_buffer);
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

	static uint8_t hci_buffer[CONFIG_BT_RX_BUF_LEN];

	bool received_evt = false;
	bool received_data = false;

	while (true) {
		if (!received_evt && !received_data) {
			/* Wait for a signal from the controller. */
			k_sem_take(&sem_recv, K_FOREVER);
		}

		received_evt = fetch_and_process_hci_evt(&hci_buffer[0]);

		if (IS_ENABLED(CONFIG_BT_CONN)) {
			received_data = fetch_and_process_acl_data(&hci_buffer[0]);
		}

		/* Let other threads of same priority run in between. */
		k_yield();
	}
}

void host_signal(void)
{
	/* Wake up the RX event/data thread */
	k_sem_give(&sem_recv);
}

static int hci_driver_open(void)
{
	BT_DBG("Open");

	k_thread_create(&recv_thread_data, recv_thread_stack,
			K_THREAD_STACK_SIZEOF(recv_thread_stack), recv_thread,
			NULL, NULL, NULL, K_PRIO_COOP(CONFIG_SDC_RX_PRIO), 0,
			K_NO_WAIT);
	k_thread_name_set(&recv_thread_data, "SDC RX");

	uint8_t build_revision[SDC_BUILD_REVISION_SIZE];

	sdc_build_revision_get(build_revision);
	LOG_HEXDUMP_INF(build_revision, sizeof(build_revision),
			"SoftDevice Controller build revision: ");

	int err;
	int required_memory;
	sdc_cfg_t cfg;

	cfg.master_count.count = SDC_MASTER_COUNT;

	/* NOTE: sdc_cfg_set() returns a negative errno on error. */
	required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
				       SDC_CFG_TYPE_MASTER_COUNT,
				       &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

	cfg.slave_count.count = CONFIG_SDC_SLAVE_COUNT;

	required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
				       SDC_CFG_TYPE_SLAVE_COUNT,
				       &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

	cfg.buffer_cfg.rx_packet_size = MAX_RX_PACKET_SIZE;
	cfg.buffer_cfg.tx_packet_size = MAX_TX_PACKET_SIZE;
	cfg.buffer_cfg.rx_packet_count = SDC_DEFAULT_RX_PACKET_COUNT;
	cfg.buffer_cfg.tx_packet_count = SDC_DEFAULT_TX_PACKET_COUNT;

	required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
				       SDC_CFG_TYPE_BUFFER_CFG,
				       &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

	cfg.event_length.event_length_us =
		CONFIG_SDC_MAX_CONN_EVENT_LEN_DEFAULT;
	required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
				       SDC_CFG_TYPE_EVENT_LENGTH,
				       &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

	BT_DBG("BT mempool size: %u, required: %u",
	       sizeof(sdc_mempool), required_memory);

	if (required_memory > sizeof(sdc_mempool)) {
		BT_ERR("Allocated memory too low: %u < %u",
		       sizeof(sdc_mempool), required_memory);
		k_panic();
		/* No return from k_panic(). */
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_BT_BROADCASTER)) {
		if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT)) {
			err = sdc_support_ext_adv();
			if (err) {
				return -ENOTSUP;
			}
		} else {
			err = sdc_support_adv();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		err = sdc_support_slave();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
		if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT)) {
			err = sdc_support_ext_scan();
			if (err) {
				return -ENOTSUP;
			}
		} else {
			err = sdc_support_scan();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		err = sdc_support_master();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_DATA_LENGTH)) {
		err = sdc_support_dle();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_2M)) {
		err = sdc_support_le_2m_phy();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		err = sdc_support_le_coded_phy();
		if (err) {
			return -ENOTSUP;
		}
	}

	err = MULTITHREADING_LOCK_ACQUIRE();
	if (!err) {
		err = sdc_enable(host_signal, sdc_mempool);
		MULTITHREADING_LOCK_RELEASE();
	}
	if (err < 0) {
		return err;
	}

	return 0;
}

static const struct bt_hci_driver drv = {
	.name = "SoftDevice Controller",
	.bus = BT_HCI_DRIVER_BUS_VIRTUAL,
	.open = hci_driver_open,
	.send = hci_driver_send,
};

#if !defined(CONFIG_BT_HCI_VS_EXT)
uint8_t bt_read_static_addr(struct bt_hci_vs_static_addr addrs[], uint8_t size)
{
	/* only one supported */
	ARG_UNUSED(size);

	if (((NRF_FICR->DEVICEADDR[0] != UINT32_MAX) ||
	    ((NRF_FICR->DEVICEADDR[1] & UINT16_MAX) != UINT16_MAX)) &&
	     (NRF_FICR->DEVICEADDRTYPE & 0x01)) {
		sys_put_le32(NRF_FICR->DEVICEADDR[0], &addrs[0].bdaddr.val[0]);
		sys_put_le16(NRF_FICR->DEVICEADDR[1], &addrs[0].bdaddr.val[4]);

		/* The FICR value is a just a random number, with no knowledge
		 * of the Bluetooth Specification requirements for random
		 * static addresses.
		 */
		BT_ADDR_SET_STATIC(&addrs[0].bdaddr);

		/* If no public address is provided and a static address is
		 * available, then it is recommended to return an identity root
		 * key (if available) from this command.
		 */
		if ((NRF_FICR->IR[0] != UINT32_MAX) &&
		    (NRF_FICR->IR[1] != UINT32_MAX) &&
		    (NRF_FICR->IR[2] != UINT32_MAX) &&
		    (NRF_FICR->IR[3] != UINT32_MAX)) {
			sys_put_le32(NRF_FICR->IR[0], &addrs[0].ir[0]);
			sys_put_le32(NRF_FICR->IR[1], &addrs[0].ir[4]);
			sys_put_le32(NRF_FICR->IR[2], &addrs[0].ir[8]);
			sys_put_le32(NRF_FICR->IR[3], &addrs[0].ir[12]);
		} else {
			/* Mark IR as invalid */
			(void)memset(addrs[0].ir, 0x00, sizeof(addrs[0].ir));
		}

		return 1;
	}

	return 0;
}
#endif /* !defined(CONFIG_BT_HCI_VS_EXT) */

void bt_ctlr_set_public_addr(const uint8_t *addr)
{
	const sdc_hci_cmd_vs_zephyr_write_bd_addr_t *bd_addr = (void *)addr;

	(void)sdc_hci_cmd_vs_zephyr_write_bd_addr(bd_addr);
}

static int hci_driver_init(const struct device *unused)
{
	ARG_UNUSED(unused);
	int err = 0;

	bt_hci_driver_register(&drv);

	err = sdc_init(sdc_assertion_handler);
	return err;
}

SYS_INIT(hci_driver_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
