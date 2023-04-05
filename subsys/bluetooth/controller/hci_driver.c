/*
 * Copyright (c) 2018 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>
#include <zephyr/bluetooth/controller.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <stdbool.h>
#include <zephyr/sys/__assert.h>

#include <sdc.h>
#include <sdc_soc.h>
#include <sdc_hci.h>
#include <sdc_hci_vs.h>
#include <mpsl/mpsl_work.h>
#include <mpsl/mpsl_lib.h>

#include "multithreading_lock.h"
#include "hci_internal.h"
#include "ecdh.h"
#include "radio_nrf5_txp.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_sdc_hci_driver);

#if defined(CONFIG_BT_CONN)
/* It should not be possible to set CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT larger than
 * CONFIG_BT_MAX_CONN. Kconfig should make sure of that, this assert is to
 * verify that assumption.
 */
BUILD_ASSERT(CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT <= CONFIG_BT_MAX_CONN);

#define SDC_CENTRAL_COUNT (CONFIG_BT_MAX_CONN - CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT)

#else

#define SDC_CENTRAL_COUNT 0

#endif /* CONFIG_BT_CONN */

BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_CENTRAL) ||
			 (SDC_CENTRAL_COUNT > 0));

BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_PERIPHERAL) ||
			 (CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT > 0));

#if defined(CONFIG_BT_BROADCASTER)
	#if defined(CONFIG_BT_CTLR_ADV_EXT)
		#define SDC_ADV_SET_COUNT CONFIG_BT_CTLR_ADV_SET
		#define SDC_ADV_BUF_SIZE  CONFIG_BT_CTLR_ADV_DATA_LEN_MAX
	#else
		#define SDC_ADV_SET_COUNT 1
		#define SDC_ADV_BUF_SIZE SDC_DEFAULT_ADV_BUF_SIZE
	#endif
	#define SDC_ADV_SET_MEM_SIZE \
		(SDC_ADV_SET_COUNT * SDC_MEM_PER_ADV_SET(SDC_ADV_BUF_SIZE))
#else
	#define SDC_ADV_SET_COUNT 0
	#define SDC_ADV_SET_MEM_SIZE 0
#endif

#if defined(CONFIG_BT_PER_ADV)
	#define SDC_PERIODIC_ADV_COUNT CONFIG_BT_EXT_ADV_MAX_ADV_SET
	#define SDC_PERIODIC_ADV_MEM_SIZE \
		(SDC_PERIODIC_ADV_COUNT * \
		 SDC_MEM_PER_PERIODIC_ADV_SET(CONFIG_BT_CTLR_ADV_DATA_LEN_MAX))
#else
	#define SDC_PERIODIC_ADV_COUNT 0
	#define SDC_PERIODIC_ADV_MEM_SIZE 0
#endif

#if defined(CONFIG_BT_CTLR_SDC_PAWR_ADV)
#define PERIODIC_ADV_RSP_ENABLE_FAILURE_REPORTING \
		IS_ENABLED(CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_RX_FAILURE_REPORTING)
#define SDC_PERIODIC_ADV_RSP_MEM_SIZE \
	(CONFIG_BT_CTLR_SDC_PAWR_ADV_COUNT * \
	 SDC_MEM_PER_PERIODIC_ADV_RSP_SET(CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_BUFFER_COUNT, \
				CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_RX_BUFFER_COUNT, \
				CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_MAX_DATA_SIZE, \
				PERIODIC_ADV_RSP_ENABLE_FAILURE_REPORTING))
#else
#define SDC_PERIODIC_ADV_RSP_MEM_SIZE 0
#endif

#if defined(CONFIG_BT_CTLR_SDC_PAWR_SYNC) || defined(CONFIG_BT_PER_ADV_SYNC)
	#define SDC_PERIODIC_ADV_SYNC_COUNT CONFIG_BT_PER_ADV_SYNC_MAX
	#define SDC_PERIODIC_ADV_LIST_MEM_SIZE \
		SDC_MEM_PERIODIC_ADV_LIST(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST_SIZE)
#else
	#define SDC_PERIODIC_ADV_SYNC_COUNT 0
	#define SDC_PERIODIC_ADV_LIST_MEM_SIZE 0
#endif

#if defined(CONFIG_BT_CTLR_SDC_PAWR_SYNC)
	#define SDC_PERIODIC_SYNC_MEM_SIZE \
		(SDC_PERIODIC_ADV_SYNC_COUNT * \
		 SDC_MEM_PER_PERIODIC_SYNC_RSP( \
					CONFIG_BT_CTLR_SDC_PERIODIC_SYNC_RSP_TX_BUFFER_COUNT, \
					CONFIG_BT_CTLR_SDC_PERIODIC_SYNC_BUFFER_COUNT))
#elif defined(CONFIG_BT_PER_ADV_SYNC)
	#define SDC_PERIODIC_SYNC_MEM_SIZE \
		(SDC_PERIODIC_ADV_SYNC_COUNT * \
		 SDC_MEM_PER_PERIODIC_SYNC(CONFIG_BT_CTLR_SDC_PERIODIC_SYNC_BUFFER_COUNT))
#else
	#define SDC_PERIODIC_SYNC_MEM_SIZE 0
#endif

#if defined(CONFIG_BT_OBSERVER)
	#if defined(CONFIG_BT_CTLR_ADV_EXT)
		#define SDC_SCAN_BUF_SIZE \
			SDC_MEM_SCAN_BUFFER_EXT(CONFIG_BT_CTLR_SDC_SCAN_BUFFER_COUNT)
	#else
		#define SDC_SCAN_BUF_SIZE \
			SDC_MEM_SCAN_BUFFER(CONFIG_BT_CTLR_SDC_SCAN_BUFFER_COUNT)
	#endif
#else
	#define SDC_SCAN_BUF_SIZE 0
#endif

#ifdef CONFIG_BT_CTLR_DATA_LENGTH_MAX
	#define MAX_TX_PACKET_SIZE CONFIG_BT_CTLR_DATA_LENGTH_MAX
	#define MAX_RX_PACKET_SIZE CONFIG_BT_CTLR_DATA_LENGTH_MAX
#else
	#define MAX_TX_PACKET_SIZE SDC_DEFAULT_TX_PACKET_SIZE
	#define MAX_RX_PACKET_SIZE SDC_DEFAULT_RX_PACKET_SIZE
#endif

#define CENTRAL_MEM_SIZE (SDC_MEM_PER_CENTRAL_LINK( \
	MAX_TX_PACKET_SIZE, \
	MAX_RX_PACKET_SIZE, \
	CONFIG_BT_CTLR_SDC_TX_PACKET_COUNT, \
	CONFIG_BT_CTLR_SDC_RX_PACKET_COUNT) \
	+ SDC_MEM_CENTRAL_LINKS_SHARED)

#define PERIPHERAL_MEM_SIZE (SDC_MEM_PER_PERIPHERAL_LINK( \
	MAX_TX_PACKET_SIZE, \
	MAX_RX_PACKET_SIZE, \
	CONFIG_BT_CTLR_SDC_TX_PACKET_COUNT, \
	CONFIG_BT_CTLR_SDC_RX_PACKET_COUNT) \
	+ SDC_MEM_PERIPHERAL_LINKS_SHARED)

#define PERIPHERAL_COUNT CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT

#define SDC_EXTRA_MEMORY CONFIG_BT_SDC_ADDITIONAL_MEMORY

#define MEMPOOL_SIZE ((PERIPHERAL_COUNT * PERIPHERAL_MEM_SIZE) + \
		      (SDC_CENTRAL_COUNT * CENTRAL_MEM_SIZE) + \
		      (SDC_ADV_SET_MEM_SIZE) + \
		      (SDC_PERIODIC_ADV_MEM_SIZE) + \
		      (SDC_PERIODIC_ADV_RSP_MEM_SIZE) + \
		      (SDC_PERIODIC_SYNC_MEM_SIZE) + \
		      (SDC_PERIODIC_ADV_LIST_MEM_SIZE) + \
		      (SDC_SCAN_BUF_SIZE) + \
		      (SDC_EXTRA_MEMORY))

#if CONFIG_BT_SDC_ADDITIONAL_MEMORY
__aligned(8) uint8_t sdc_mempool[MEMPOOL_SIZE];
#else
static __aligned(8) uint8_t sdc_mempool[MEMPOOL_SIZE];
#endif

#if IS_ENABLED(CONFIG_BT_CTLR_ASSERT_HANDLER)
extern void bt_ctlr_assert_handle(char *file, uint32_t line);

void sdc_assertion_handler(const char *const file, const uint32_t line)
{
	bt_ctlr_assert_handle((char *) file, line);
}

#else /* !IS_ENABLED(CONFIG_BT_CTLR_ASSERT_HANDLER) */
void sdc_assertion_handler(const char *const file, const uint32_t line)
{
	LOG_ERR("SoftDevice Controller ASSERT: %s, %d", file, line);
	k_oops();
}
#endif /* IS_ENABLED(CONFIG_BT_CTLR_ASSERT_HANDLER) */

static struct k_work receive_work;
static inline void receive_signal_raise(void)
{
	mpsl_work_submit(&receive_work);
}

static int cmd_handle(struct net_buf *cmd)
{
	LOG_DBG("");

	int errcode = MULTITHREADING_LOCK_ACQUIRE();

	if (!errcode) {
		errcode = hci_internal_cmd_put(cmd->data);
		MULTITHREADING_LOCK_RELEASE();
	}
	if (errcode) {
		return errcode;
	}

	receive_signal_raise();

	return 0;
}

#if defined(CONFIG_BT_CONN)
static int acl_handle(struct net_buf *acl)
{
	LOG_DBG("");

	int errcode = MULTITHREADING_LOCK_ACQUIRE();

	if (!errcode) {
		errcode = sdc_hci_data_put(acl->data);
		MULTITHREADING_LOCK_RELEASE();

		if (errcode) {
			/* Likely buffer overflow event */
			receive_signal_raise();
		}
	}

	return errcode;
}
#endif

static int hci_driver_send(struct net_buf *buf)
{
	int err;
	uint8_t type;

	LOG_DBG("");

	if (!buf->len) {
		LOG_DBG("Empty HCI packet");
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
		LOG_DBG("Unknown HCI type %u", type);
		return -EINVAL;
	}

	if (!err) {
		net_buf_unref(buf);
	}

	LOG_DBG("Exit: %d", err);
	return err;
}

static void data_packet_process(uint8_t *hci_buf)
{
	struct net_buf *data_buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	struct bt_hci_acl_hdr *hdr = (void *)hci_buf;
	uint16_t hf, handle, len;
	uint8_t flags, pb, bc;

	if (!data_buf) {
		LOG_ERR("No data buffer available");
		return;
	}

	len = sys_le16_to_cpu(hdr->len);
	hf = sys_le16_to_cpu(hdr->handle);
	handle = bt_acl_handle(hf);
	flags = bt_acl_flags(hf);
	pb = bt_acl_flags_pb(flags);
	bc = bt_acl_flags_bc(flags);

	LOG_DBG("Data: handle (0x%02x), PB(%01d), BC(%01d), len(%u)", handle,
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
			return true;
#if defined(CONFIG_BT_EXT_ADV)
		case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT:
		{
			const struct bt_hci_evt_le_ext_advertising_report *ext_adv =
				(void *)&hci_buf[3];

			return (ext_adv->num_reports == 1) &&
				   ((ext_adv->adv_info->evt_type &
					 BT_HCI_LE_ADV_EVT_TYPE_LEGACY) != 0);
		}
#endif
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

		LOG_DBG("LE Meta Event (0x%02x), len (%u)",
		       me->subevent, hdr->len);
	} else if (hdr->evt == BT_HCI_EVT_CMD_COMPLETE) {
		struct bt_hci_evt_cmd_complete *cc = (void *)&hci_buf[2];
		struct bt_hci_evt_cc_status *ccs = (void *)&hci_buf[5];
		uint16_t opcode = sys_le16_to_cpu(cc->opcode);

		LOG_DBG("Command Complete (0x%04x) status: 0x%02x,"
		       " ncmd: %u, len %u",
		       opcode, ccs->status, cc->ncmd, hdr->len);
	} else if (hdr->evt == BT_HCI_EVT_CMD_STATUS) {
		struct bt_hci_evt_cmd_status *cs = (void *)&hci_buf[2];
		uint16_t opcode = sys_le16_to_cpu(cs->opcode);

		LOG_DBG("Command Status (0x%04x) status: 0x%02x",
		       opcode, cs->status);
	} else {
		LOG_DBG("Event (0x%02x) len %u", hdr->evt, hdr->len);
	}

	evt_buf = bt_buf_get_evt(hdr->evt, discardable,
				 discardable ? K_NO_WAIT : K_FOREVER);

	if (!evt_buf) {
		if (discardable) {
			LOG_DBG("Discarding event");
			return;
		}

		LOG_ERR("No event buffer available");
		return;
	}

	net_buf_add_mem(evt_buf, &hci_buf[0], hdr->len + sizeof(*hdr));
	bt_recv(evt_buf);
}

static bool fetch_and_process_hci_msg(uint8_t *p_hci_buffer)
{
	int errcode;
	sdc_hci_msg_type_t msg_type;

	errcode = MULTITHREADING_LOCK_ACQUIRE();
	if (!errcode) {
		errcode = hci_internal_msg_get(p_hci_buffer, &msg_type);
		MULTITHREADING_LOCK_RELEASE();
	}

	if (errcode) {
		return false;
	}

	if (msg_type == SDC_HCI_MSG_TYPE_EVT) {
		event_packet_process(p_hci_buffer);
	} else if (msg_type == SDC_HCI_MSG_TYPE_DATA) {
		data_packet_process(p_hci_buffer);
	} else {
		LOG_ERR("Unexpected msg_type: %u. This if-else needs a new branch", msg_type);
	}

	return true;
}

void hci_driver_receive_process(void)
{
#if defined(CONFIG_BT_BUF_EVT_DISCARDABLE_COUNT)
	static uint8_t hci_buf[MAX(BT_BUF_RX_SIZE,
				   BT_BUF_EVT_SIZE(CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE))];
#else
	static uint8_t hci_buf[BT_BUF_RX_SIZE];
#endif

	if (fetch_and_process_hci_msg(&hci_buf[0])) {
		/* Let other threads of same priority run in between. */
		receive_signal_raise();
	}
}

static void receive_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	hci_driver_receive_process();
}

static const struct device *entropy_source = DEVICE_DT_GET(DT_NODELABEL(rng));

static uint8_t rand_prio_low_vector_get(uint8_t *p_buff, uint8_t length)
{
	int ret = entropy_get_entropy_isr(entropy_source, p_buff, length, 0);

	__ASSERT(ret >= 0, "The entropy source returned an error in the low priority context");
	return ret >= 0 ? ret : 0;
}

static uint8_t rand_prio_high_vector_get(uint8_t *p_buff, uint8_t length)
{
	int ret = entropy_get_entropy_isr(entropy_source, p_buff, length, 0);

	__ASSERT(ret >= 0, "The entropy source returned an error in the high priority context");
	return ret >= 0 ? ret : 0;
}

static void rand_prio_low_vector_get_blocking(uint8_t *p_buff, uint8_t length)
{
	int err = entropy_get_entropy(entropy_source, p_buff, length);

	__ASSERT(err == 0, "The entropy source returned an error in a blocking call");
	(void) err;
}

static int configure_supported_features(void)
{
	int err;

#if defined(CONFIG_BT_BROADCASTER)
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
#endif

	if (IS_ENABLED(CONFIG_BT_PER_ADV)) {
		err = sdc_support_le_periodic_adv();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		err = sdc_support_peripheral();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
		if (!IS_ENABLED(CONFIG_BT_CENTRAL)) {
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

		if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC)) {
			err = sdc_support_le_periodic_sync();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT)) {
			err = sdc_support_ext_central();
			if (err) {
				return -ENOTSUP;
			}
		} else {
			err = sdc_support_central();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_DATA_LENGTH)) {
		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			err = sdc_support_dle_central();
			if (err) {
				return -ENOTSUP;
			}
		}
		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			err = sdc_support_dle_peripheral();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_2M)) {
		err = sdc_support_le_2m_phy();
		if (err) {
			return -ENOTSUP;
		}
		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			err = sdc_support_phy_update_central();
			if (err) {
				return -ENOTSUP;
			}
		}
		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			err = sdc_support_phy_update_peripheral();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER)) {
		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			err = sdc_support_periodic_adv_sync_transfer_sender_central();
			if (err) {
				return -ENOTSUP;
			}
		}

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			err = sdc_support_periodic_adv_sync_transfer_sender_peripheral();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)) {
		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			err = sdc_support_periodic_adv_sync_transfer_receiver_central();
			if (err) {
				return -ENOTSUP;
			}
		}

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			err = sdc_support_periodic_adv_sync_transfer_receiver_peripheral();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		err = sdc_support_le_coded_phy();
		if (err) {
			return -ENOTSUP;
		}
		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			err = sdc_support_phy_update_central();
			if (err) {
				return -ENOTSUP;
			}
		}
		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			err = sdc_support_phy_update_peripheral();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_SDC_CX_ADV_TRY_CONTINUE_ON_DENIAL)) {
		err = sdc_coex_adv_mode_configure(true);
		if (err) {
			return -ENOTSUP;
		}
	} else if (IS_ENABLED(CONFIG_BT_CTLR_SDC_CX_ADV_CLOSE_ADV_EVT_ON_DENIAL)) {
		err = sdc_coex_adv_mode_configure(false);
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_DF_CONN_CTE_RSP) && IS_ENABLED(CONFIG_BT_CENTRAL)) {
		err = sdc_support_le_conn_cte_rsp_central();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_DF_CONN_CTE_RSP) && IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		err = sdc_support_le_conn_cte_rsp_peripheral();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_LE_POWER_CONTROL)) {
		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			err = sdc_support_le_power_control_central();
			if (err) {
				return -ENOTSUP;
			}
		}

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			err = sdc_support_le_power_control_peripheral();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_SCA_UPDATE)) {
		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			err = sdc_support_sca_central();
			if (err) {
				return -ENOTSUP;
			}
		}

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			err = sdc_support_sca_peripheral();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_SDC_PAWR_ADV)) {
		err = sdc_support_le_periodic_adv_with_rsp();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_SDC_PAWR_SYNC)) {
		err = sdc_support_le_periodic_sync_with_rsp();
		if (err) {
			return -ENOTSUP;
		}
	}

	return 0;
}

static int configure_memory_usage(void)
{
	int required_memory;
	sdc_cfg_t cfg;

#if !defined(CONFIG_BT_LL_SOFTDEVICE_PERIPHERAL)
	cfg.central_count.count = SDC_CENTRAL_COUNT;

	/* NOTE: sdc_cfg_set() returns a negative errno on error. */
	required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
				       SDC_CFG_TYPE_CENTRAL_COUNT,
				       &cfg);
	if (required_memory < 0) {
		return required_memory;
	}
#endif

#if !defined(CONFIG_BT_LL_SOFTDEVICE_CENTRAL)
	cfg.peripheral_count.count = CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT;

	required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
				       SDC_CFG_TYPE_PERIPHERAL_COUNT,
				       &cfg);
	if (required_memory < 0) {
		return required_memory;
	}
#endif

	cfg.buffer_cfg.rx_packet_size = MAX_RX_PACKET_SIZE;
	cfg.buffer_cfg.tx_packet_size = MAX_TX_PACKET_SIZE;
	cfg.buffer_cfg.rx_packet_count = CONFIG_BT_CTLR_SDC_RX_PACKET_COUNT;
	cfg.buffer_cfg.tx_packet_count = CONFIG_BT_CTLR_SDC_TX_PACKET_COUNT;

	required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
				       SDC_CFG_TYPE_BUFFER_CFG,
				       &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

	cfg.event_length.event_length_us =
		CONFIG_BT_CTLR_SDC_MAX_CONN_EVENT_LEN_DEFAULT;
	required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
				       SDC_CFG_TYPE_EVENT_LENGTH,
				       &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

#if defined(CONFIG_BT_BROADCASTER) || defined(CONFIG_BT_LL_SOFTDEVICE_MULTIROLE)
	cfg.adv_count.count = SDC_ADV_SET_COUNT;

	required_memory =
	sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
		    SDC_CFG_TYPE_ADV_COUNT,
		    &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

#if defined(CONFIG_BT_CTLR_ADV_DATA_LEN_MAX)
	cfg.adv_buffer_cfg.max_adv_data = CONFIG_BT_CTLR_ADV_DATA_LEN_MAX;
#else
	cfg.adv_buffer_cfg.max_adv_data = SDC_DEFAULT_ADV_BUF_SIZE;
#endif

	required_memory =
	sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
		    SDC_CFG_TYPE_ADV_BUFFER_CFG,
		    &cfg);
	if (required_memory < 0) {
		return required_memory;
	}
#endif

	if (IS_ENABLED(CONFIG_BT_PER_ADV)) {
		cfg.periodic_adv_count.count = SDC_PERIODIC_ADV_COUNT;
		required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
			    SDC_CFG_TYPE_PERIODIC_ADV_COUNT,
			    &cfg);
		if (required_memory < 0) {
			return required_memory;
		}
	}

	if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
		cfg.scan_buffer_cfg.count = CONFIG_BT_CTLR_SDC_SCAN_BUFFER_COUNT;

		required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
			    SDC_CFG_TYPE_SCAN_BUFFER_CFG,
			    &cfg);
		if (required_memory < 0) {
			return required_memory;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC)) {
		cfg.periodic_sync_count.count = SDC_PERIODIC_ADV_SYNC_COUNT;
		required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
			    SDC_CFG_TYPE_PERIODIC_SYNC_COUNT,
			    &cfg);
		if (required_memory < 0) {
			return required_memory;
		}

		cfg.periodic_sync_buffer_cfg.count = CONFIG_BT_CTLR_SDC_PERIODIC_SYNC_BUFFER_COUNT;
		required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
			    SDC_CFG_TYPE_PERIODIC_SYNC_BUFFER_CFG,
			    &cfg);
		if (required_memory < 0) {
			return required_memory;
		}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST_SIZE)
		cfg.periodic_adv_list_size = CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST_SIZE;
		required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
			    SDC_CFG_TYPE_PERIODIC_ADV_LIST_SIZE,
			    &cfg);
		if (required_memory < 0) {
			return required_memory;
		}
#endif
	}

#if defined(CONFIG_BT_CTLR_SDC_PAWR_ADV)
	cfg.periodic_adv_rsp_count.count = CONFIG_BT_CTLR_SDC_PAWR_ADV_COUNT;
	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
				      SDC_CFG_TYPE_PERIODIC_ADV_RSP_COUNT, &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

	cfg.periodic_adv_rsp_buffer_cfg.tx_buffer_count =
		CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_BUFFER_COUNT;
	cfg.periodic_adv_rsp_buffer_cfg.max_tx_data_size =
		CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_MAX_DATA_SIZE;
	cfg.periodic_adv_rsp_buffer_cfg.rx_buffer_count =
		CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_RX_BUFFER_COUNT;

	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
				      SDC_CFG_TYPE_PERIODIC_ADV_RSP_BUFFER_CFG, &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

	cfg.periodic_adv_rsp_failure_reporting_cfg =
		PERIODIC_ADV_RSP_ENABLE_FAILURE_REPORTING ? 1 : 0;

	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
		SDC_CFG_TYPE_PERIODIC_ADV_RSP_FAILURE_REPORTING_CFG, &cfg);

	if (required_memory < 0) {
		return required_memory;
	}
#endif

#if defined(CONFIG_BT_CTLR_SDC_PAWR_SYNC)
	cfg.periodic_sync_rsp_tx_buffer_cfg.count =
		CONFIG_BT_CTLR_SDC_PERIODIC_SYNC_RSP_TX_BUFFER_COUNT;
	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
					  SDC_CFG_TYPE_PERIODIC_SYNC_RSP_TX_BUFFER_CFG, &cfg);
	if (required_memory < 0) {
		return required_memory;
	}
#endif

	LOG_DBG("BT mempool size: %u, required: %u",
	       sizeof(sdc_mempool), required_memory);

	if (required_memory > sizeof(sdc_mempool)) {
		LOG_ERR("Allocated memory too low: %u < %u",
		       sizeof(sdc_mempool), required_memory);
		k_panic();
		/* No return from k_panic(). */
		return -ENOMEM;
	}

	return 0;
}

static int hci_driver_open(void)
{
	LOG_DBG("Open");

	k_work_init(&receive_work, receive_work_handler);

	if (IS_ENABLED(CONFIG_BT_CTLR_ECDH)) {
		hci_ecdh_init();
	}

	uint8_t build_revision[SDC_BUILD_REVISION_SIZE];

	sdc_build_revision_get(build_revision);
	LOG_HEXDUMP_INF(build_revision, sizeof(build_revision),
			"SoftDevice Controller build revision: ");

	int err;

	if (!device_is_ready(entropy_source)) {
		LOG_ERR("Entropy source device not ready");
		return -ENODEV;
	}

	sdc_rand_source_t rand_functions = {
		.rand_prio_low_get = rand_prio_low_vector_get,
		.rand_prio_high_get = rand_prio_high_vector_get,
		.rand_poll = rand_prio_low_vector_get_blocking
	};

	err = sdc_rand_source_register(&rand_functions);
	if (err) {
		LOG_ERR("Failed to register rand source (%d)", err);
		return -EINVAL;
	}

	err = MULTITHREADING_LOCK_ACQUIRE();
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_UNINIT_MPSL_ON_DISABLE)) {
		if (!mpsl_is_initialized()) {
			err = mpsl_lib_init();
			if (err) {
				MULTITHREADING_LOCK_RELEASE();
				return err;
			}
		}
	}

#if RADIO_TXP_DEFAULT != 0
	err = sdc_default_tx_power_set(RADIO_TXP_DEFAULT);
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return -ENOTSUP;
	}
#endif

	err = sdc_enable(receive_signal_raise, sdc_mempool);
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return err;
	}

	MULTITHREADING_LOCK_RELEASE();

	return 0;
}

static int hci_driver_close(void)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_CTLR_ECDH)) {
		hci_ecdh_uninit();
	}

	err = MULTITHREADING_LOCK_ACQUIRE();
	if (err) {
		return err;
	}

	err = sdc_disable();
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_UNINIT_MPSL_ON_DISABLE)) {
		err = mpsl_lib_uninit();
		if (err) {
			MULTITHREADING_LOCK_RELEASE();
			return err;
		}
	}

	MULTITHREADING_LOCK_RELEASE();

	return err;
}

static const struct bt_hci_driver drv = {
	.name = "SoftDevice Controller",
	.bus = BT_HCI_DRIVER_BUS_VIRTUAL,
	.open = hci_driver_open,
	.close = hci_driver_close,
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

	err = configure_supported_features();
	if (err) {
		return err;
	}

	err = configure_memory_usage();
	if (err) {
		return err;
	}

	return err;
}

SYS_INIT(hci_driver_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
