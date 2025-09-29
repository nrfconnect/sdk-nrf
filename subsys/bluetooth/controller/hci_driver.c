/*
 * Copyright (c) 2018 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/bluetooth.h>
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

#define SDC_USE_NEW_MEM_API
#include <sdc.h>
#include <sdc_soc.h>
#include <sdc_hci.h>
#include <sdc_hci_vs.h>
#include <mpsl.h>
#include <mpsl/mpsl_work.h>
#include <mpsl/mpsl_lib.h>

#include "multithreading_lock.h"
#include "hci_internal.h"
#include "radio_nrf5_txp.h"
#include "cs_antenna_switch.h"

#define DT_DRV_COMPAT nordic_bt_hci_sdc

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_sdc_hci_driver);


#if defined(CONFIG_BT_BUF_EVT_DISCARDABLE_COUNT)
#define HCI_RX_BUF_SIZE MAX(BT_BUF_RX_SIZE, \
			BT_BUF_EVT_SIZE(CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE))
#else
#define HCI_RX_BUF_SIZE BT_BUF_RX_SIZE
#endif

#if defined(CONFIG_BT_CONN) && defined(CONFIG_BT_CENTRAL)

#if CONFIG_BT_MAX_CONN > 1
#define SDC_CENTRAL_COUNT (CONFIG_BT_MAX_CONN - CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT)
#else
/* Allow the case where BT_MAX_CONN, central and peripheral counts are 1. This
 * way we avoid wasting memory in the host if the device will only use one role
 * at a time.
 */
#define SDC_CENTRAL_COUNT 1
#endif	/* CONFIG_BT_MAX_CONN > 1 */

#else
#define SDC_CENTRAL_COUNT 0
#endif /* defined(CONFIG_BT_CONN) && defined(CONFIG_BT_CENTRAL) */

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
	#if defined(CONFIG_BT_CTLR_SDC_PAWR_ADV)
		#define SDC_PERIODIC_ADV_COUNT \
			(CONFIG_BT_EXT_ADV_MAX_ADV_SET - CONFIG_BT_CTLR_SDC_PAWR_ADV_COUNT)
	#else
		#define SDC_PERIODIC_ADV_COUNT CONFIG_BT_EXT_ADV_MAX_ADV_SET
	#endif
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
		SDC_MEM_PER_PERIODIC_ADV_RSP_SET(CONFIG_BT_CTLR_ADV_DATA_LEN_MAX, \
				CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_BUFFER_COUNT, \
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
		#define SDC_SCAN_SIZE \
			SDC_MEM_SCAN_EXT(CONFIG_BT_CTLR_SDC_SCAN_BUFFER_COUNT)
	#else
		#define SDC_SCAN_SIZE \
			SDC_MEM_SCAN(CONFIG_BT_CTLR_SDC_SCAN_BUFFER_COUNT)
	#endif
#else
	#define SDC_SCAN_SIZE 0
#endif

#if defined(CONFIG_BT_CTLR_SDC_ALLOW_PARALLEL_SCANNING_AND_INITIATING)
	#define SDC_INITIATOR_SIZE SDC_MEM_INITIATOR
#else
	#define SDC_INITIATOR_SIZE 0
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

#define SDC_FAL_MEM_SIZE SDC_MEM_FAL(CONFIG_BT_CTLR_FAL_SIZE)

#if defined(CONFIG_BT_CTLR_LE_POWER_CONTROL)
#define SDC_LE_POWER_CONTROL_MEM_SIZE SDC_MEM_LE_POWER_CONTROL(SDC_CENTRAL_COUNT + PERIPHERAL_COUNT)
#else
#define SDC_LE_POWER_CONTROL_MEM_SIZE 0
#endif

#if defined(CONFIG_BT_CTLR_SUBRATING)
#define SDC_SUBRATING_MEM_SIZE SDC_MEM_SUBRATING(SDC_CENTRAL_COUNT + PERIPHERAL_COUNT)
#else
#define SDC_SUBRATING_MEM_SIZE 0
#endif

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER) || defined(CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER)
#define SDC_SYNC_TRANSFER_MEM_SIZE SDC_MEM_SYNC_TRANSFER(SDC_CENTRAL_COUNT + PERIPHERAL_COUNT)
#else
#define SDC_SYNC_TRANSFER_MEM_SIZE 0
#endif

#if defined(CONFIG_BT_CTLR_FRAME_SPACE_UPDATE)
#define SDC_FRAME_SPACE_UPDATE_MEM_SIZE \
	SDC_MEM_FRAME_SPACE_UPDATE(SDC_CENTRAL_COUNT + PERIPHERAL_COUNT)
#else
#define SDC_FRAME_SPACE_UPDATE_MEM_SIZE 0
#endif

#if defined(CONFIG_BT_CTLR_EXTENDED_FEAT_SET)
#if defined(SDC_MEM_EXTENDED_FEATURE_SET_NEW)
#define SDC_EXTENDED_FEAT_SET_MEM_SIZE                                                             \
	SDC_MEM_EXTENDED_FEATURE_SET_NEW(SDC_CENTRAL_COUNT + PERIPHERAL_COUNT,                     \
					 CONFIG_BT_CTLR_SDC_EXTENDED_FEAT_MAX_REMOTE_PAGE)
#else
#define SDC_EXTENDED_FEAT_SET_MEM_SIZE                                                             \
	SDC_MEM_EXTENDED_FEATURE_SET(SDC_CENTRAL_COUNT + PERIPHERAL_COUNT,                         \
				     CONFIG_BT_CTLR_SDC_EXTENDED_FEAT_MAX_REMOTE_PAGE)
#endif
#else
#define SDC_EXTENDED_FEAT_SET_MEM_SIZE 0
#endif

#if defined(CONFIG_BT_CTLR_CONN_ISO)
#define SDC_MEM_CIG SDC_MEM_PER_CIG(CONFIG_BT_CTLR_CONN_ISO_GROUPS)
#define SDC_MEM_CIS \
	SDC_MEM_PER_CIS(CONFIG_BT_CTLR_CONN_ISO_STREAMS) + \
	SDC_MEM_ISO_RX_SDU_POOL_SIZE(CONFIG_BT_CTLR_CONN_ISO_STREAMS, \
				     CONFIG_BT_ISO_RX_MTU)
#define SDC_CIS_COUNT CONFIG_BT_CTLR_CONN_ISO_STREAMS
#else
#define SDC_MEM_CIG   0
#define SDC_MEM_CIS   0
#define SDC_CIS_COUNT 0
#endif

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
#define SDC_MEM_BIS_SINK \
	SDC_MEM_PER_BIG(CONFIG_BT_CTLR_SCAN_SYNC_ISO_SET) +					\
		SDC_MEM_PER_BIS(CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT) +				\
		SDC_MEM_ISO_RX_SDU_POOL_SIZE(CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT,              \
					     CONFIG_BT_ISO_RX_MTU)
#define SDC_BIS_SINK_COUNT CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT
#else
#define SDC_MEM_BIS_SINK   0
#define SDC_BIS_SINK_COUNT 0
#endif

#if defined(CONFIG_BT_CTLR_ADV_ISO)
#define SDC_MEM_BIS_SOURCE									\
	SDC_MEM_PER_BIG(CONFIG_BT_CTLR_ADV_ISO_SET) +			\
	SDC_MEM_PER_BIS(CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT)
#define SDC_BIS_SOURCE_COUNT CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT
#else
#define SDC_MEM_BIS_SOURCE   0
#define SDC_BIS_SOURCE_COUNT 0
#endif

#if defined(CONFIG_BT_CTLR_SDC_ISO_RX_PDU_BUFFER_PER_STREAM_COUNT)
#define SDC_MEM_ISO_RX_PDU_POOL                                                            \
	SDC_MEM_ISO_RX_PDU_POOL_PER_STREAM_SIZE(                                               \
		CONFIG_BT_CTLR_SDC_ISO_RX_PDU_BUFFER_PER_STREAM_COUNT,                             \
		SDC_CIS_COUNT, SDC_BIS_SINK_COUNT)
#else
#define SDC_MEM_ISO_RX_PDU_POOL 0
#endif

#if defined(CONFIG_BT_CTLR_SDC_ISO_TX_HCI_BUFFER_COUNT) && \
	defined(CONFIG_BT_CTLR_SDC_ISO_TX_PDU_BUFFER_PER_STREAM_COUNT)
#define SDC_MEM_ISO_TX_POOL                                                            \
	SDC_MEM_ISO_TX_PDU_POOL_SIZE(                                                          \
		CONFIG_BT_CTLR_SDC_ISO_TX_PDU_BUFFER_PER_STREAM_COUNT,                         \
		SDC_CIS_COUNT,                                                                 \
		SDC_BIS_SOURCE_COUNT) +							       \
	SDC_MEM_ISO_TX_SDU_POOL_SIZE(CONFIG_BT_CTLR_SDC_ISO_TX_HCI_BUFFER_COUNT,               \
		CONFIG_BT_ISO_TX_MTU)
#else
#define SDC_MEM_ISO_TX_POOL 0
#endif

#if defined(CONFIG_BT_CTLR_SDC_QOS_CHANNEL_SURVEY)
#define SDC_MEM_CHAN_SURV SDC_MEM_QOS_CHANNEL_SURVEY
#else
#define SDC_MEM_CHAN_SURV 0
#endif

#if defined(CONFIG_BT_CTLR_SDC_CS_COUNT)
#define SDC_MEM_CS_POOL							\
	SDC_MEM_CS( \
		CONFIG_BT_CTLR_SDC_CS_COUNT, \
		CONFIG_BT_CTLR_SDC_CS_MAX_ANTENNA_PATHS, \
		IS_ENABLED(CONFIG_BT_CTLR_SDC_CS_STEP_MODE3)) + \
	SDC_MEM_CS_SETUP_PHASE_LINKS(SDC_CENTRAL_COUNT + PERIPHERAL_COUNT)
#else
#define SDC_MEM_CS_POOL 0
#endif

#define MEMPOOL_SIZE ((PERIPHERAL_COUNT * PERIPHERAL_MEM_SIZE) + \
		      (SDC_CENTRAL_COUNT * CENTRAL_MEM_SIZE) + \
		      (SDC_ADV_SET_MEM_SIZE) + \
		      (SDC_LE_POWER_CONTROL_MEM_SIZE) + \
		      (SDC_SUBRATING_MEM_SIZE) + \
		      (SDC_SYNC_TRANSFER_MEM_SIZE) + \
		      (SDC_FRAME_SPACE_UPDATE_MEM_SIZE) + \
		      (SDC_EXTENDED_FEAT_SET_MEM_SIZE) + \
		      (SDC_PERIODIC_ADV_MEM_SIZE) + \
		      (SDC_PERIODIC_ADV_RSP_MEM_SIZE) + \
		      (SDC_PERIODIC_SYNC_MEM_SIZE) + \
		      (SDC_PERIODIC_ADV_LIST_MEM_SIZE) + \
		      (SDC_SCAN_SIZE) + \
		      (SDC_INITIATOR_SIZE) + \
		      (SDC_FAL_MEM_SIZE) + \
		      (SDC_MEM_CHAN_SURV) + \
		      (SDC_MEM_CIG) + \
		      (SDC_MEM_CIS) + \
		      (SDC_MEM_BIS_SINK) + \
		      (SDC_MEM_BIS_SOURCE) + \
		      (SDC_MEM_ISO_RX_PDU_POOL) + \
		      (SDC_MEM_ISO_TX_POOL) + \
		      (SDC_MEM_CS_POOL))

#if defined(CONFIG_BT_SDC_ADDITIONAL_MEMORY)
__aligned(8) uint8_t sdc_mempool[MEMPOOL_SIZE + CONFIG_BT_SDC_ADDITIONAL_MEMORY];
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
#if defined(CONFIG_ASSERT) && defined(CONFIG_ASSERT_VERBOSE) && !defined(CONFIG_ASSERT_NO_MSG_INFO)
	__ASSERT(false, "SoftDevice Controller ASSERT: %s, %d\n", file, line);
#elif defined(CONFIG_LOG)
	LOG_ERR("SoftDevice Controller ASSERT: %s, %d", file, line);
	k_oops();
#elif defined(CONFIG_PRINTK)
	printk("SoftDevice Controller ASSERT: %s, %d\n", file, line);
	printk("\n");
	k_oops();
#else
	k_oops();
#endif
}
#endif /* IS_ENABLED(CONFIG_BT_CTLR_ASSERT_HANDLER) */

static struct k_work receive_work;
static inline void receive_signal_raise(void)
{
	mpsl_work_submit(&receive_work);
}

/** Storage for HCI packets from controller to host */
static struct {
	/* Buffer for the HCI packet. */
	uint8_t buf[HCI_RX_BUF_SIZE];
	/* Type of the HCI packet the buffer contains. */
	sdc_hci_msg_type_t type;
} rx_hci_msg;

static void bt_buf_rx_freed_cb(enum bt_buf_type type_mask)
{
	if (((rx_hci_msg.type == SDC_HCI_MSG_TYPE_EVT && (type_mask & BT_BUF_EVT) != 0u) ||
	     (rx_hci_msg.type == SDC_HCI_MSG_TYPE_DATA && (type_mask & BT_BUF_ACL_IN) != 0u) ||
	     (rx_hci_msg.type == SDC_HCI_MSG_TYPE_ISO && (type_mask & BT_BUF_ISO_IN) != 0u))) {
		receive_signal_raise();
	}
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

#if defined(CONFIG_BT_CTLR_ISO_TX_BUFFERS)
static int iso_handle(struct net_buf *acl)
{
	LOG_DBG("");

	int errcode = MULTITHREADING_LOCK_ACQUIRE();

	if (!errcode) {
		errcode = sdc_hci_iso_data_put(acl->data);
		MULTITHREADING_LOCK_RELEASE();

		if (errcode) {
			/* Likely buffer overflow event */
			receive_signal_raise();
		}
	}

	return errcode;
}
#endif

static int hci_driver_send(const struct device *dev, struct net_buf *buf)
{
	int err;
	uint8_t type;

	LOG_DBG("");

	if (!buf->len) {
		LOG_DBG("Empty HCI packet");
		return -EINVAL;
	}

	type = net_buf_pull_u8(buf);
	switch (type) {
#if defined(CONFIG_BT_CONN)
	case BT_HCI_H4_ACL:
		err = acl_handle(buf);
		break;
#endif          /* CONFIG_BT_CONN */
	case BT_HCI_H4_CMD:
		err = cmd_handle(buf);
		break;
#if defined(CONFIG_BT_CTLR_ISO_TX_BUFFERS)
	case BT_HCI_H4_ISO:
		err = iso_handle(buf);
		break;
#endif
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

static int data_packet_process(const struct device *dev, uint8_t *hci_buf)
{
	struct net_buf *data_buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	struct bt_hci_acl_hdr *hdr = (void *)hci_buf;
	uint16_t hf, handle, len;
	uint8_t flags, pb, bc;

	if (!data_buf) {
		LOG_DBG("No data buffer available");
		return -ENOBUFS;
	}

	len = sys_le16_to_cpu(hdr->len);
	hf = sys_le16_to_cpu(hdr->handle);
	handle = bt_acl_handle(hf);
	flags = bt_acl_flags(hf);
	pb = bt_acl_flags_pb(flags);
	bc = bt_acl_flags_bc(flags);

	if (len + sizeof(*hdr) > HCI_RX_BUF_SIZE) {
		LOG_ERR("Event buffer too small. %zu > %u",
			len + sizeof(*hdr),
			HCI_RX_BUF_SIZE);
		return -ENOMEM;
	}

	LOG_DBG("Data: handle (0x%02x), PB(%01d), BC(%01d), len(%u)", handle,
	       pb, bc, len);

	net_buf_add_mem(data_buf, &hci_buf[0], len + sizeof(*hdr));

	struct hci_driver_data *driver_data = dev->data;

	driver_data->recv_func(dev, data_buf);

	return 0;
}

static int iso_data_packet_process(const struct device *dev, uint8_t *hci_buf)
{
	struct net_buf *data_buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
	struct bt_hci_iso_hdr *hdr = (void *)hci_buf;

	uint16_t len = sys_le16_to_cpu(hdr->len);

	if (!data_buf) {
		LOG_DBG("No data buffer available");
		return -ENOBUFS;
	}

	if (len + sizeof(*hdr) > HCI_RX_BUF_SIZE) {
		LOG_ERR("Event buffer too small. %zu > %u",
			len + sizeof(*hdr),
			HCI_RX_BUF_SIZE);
		return -ENOMEM;
	}

	net_buf_add_mem(data_buf, &hci_buf[0], len + sizeof(*hdr));

	struct hci_driver_data *driver_data = dev->data;

	(void)driver_data->recv_func(dev, data_buf);

	return 0;
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
		case SDC_HCI_SUBEVENT_VS_CONN_ANCHOR_POINT_UPDATE_REPORT:
			return true;
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

static int event_packet_process(const struct device *dev, uint8_t *hci_buf)
{
	bool discardable = event_packet_is_discardable(hci_buf);
	struct bt_hci_evt_hdr *hdr = (void *)hci_buf;
	struct net_buf *evt_buf;

	if (hdr->len + sizeof(*hdr) > HCI_RX_BUF_SIZE) {
		LOG_ERR("Event buffer too small. %zu > %u",
			hdr->len + sizeof(*hdr),
			HCI_RX_BUF_SIZE);
		return -ENOMEM;
	}

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

	evt_buf = bt_buf_get_evt(hdr->evt, discardable, K_NO_WAIT);

	if (!evt_buf) {
		if (discardable) {
			LOG_DBG("Discarding event");
			return 0;
		}

		LOG_DBG("No event buffer available");
		return -ENOBUFS;
	}

	net_buf_add_mem(evt_buf, &hci_buf[0], hdr->len + sizeof(*hdr));

	struct hci_driver_data *driver_data = dev->data;

	(void)driver_data->recv_func(dev, evt_buf);

	return 0;
}

static int fetch_hci_msg(uint8_t *p_hci_buffer, sdc_hci_msg_type_t *msg_type)
{
	int errcode;

	errcode = MULTITHREADING_LOCK_ACQUIRE();
	if (!errcode) {
		errcode = hci_internal_msg_get(p_hci_buffer, msg_type);
		MULTITHREADING_LOCK_RELEASE();
	}

	return errcode;
}

static int process_hci_msg(const struct device *dev, uint8_t *p_hci_buffer,
			    sdc_hci_msg_type_t msg_type)
{
	int err;

	if (msg_type == SDC_HCI_MSG_TYPE_EVT) {
		err = event_packet_process(dev, p_hci_buffer);
	} else if (msg_type == SDC_HCI_MSG_TYPE_DATA) {
		err = data_packet_process(dev, p_hci_buffer);
	} else if (msg_type == SDC_HCI_MSG_TYPE_ISO) {
		err = iso_data_packet_process(dev, p_hci_buffer);
	} else {
		if (!IS_ENABLED(CONFIG_BT_CTLR_SDC_SILENCE_UNEXPECTED_MSG_TYPE)) {
			LOG_ERR("Unexpected msg_type: %u. This if-else needs a new branch",
				msg_type);
		}
		err = 0;
	}

	return err;
}

void hci_driver_receive_process(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	int err;

	if (rx_hci_msg.type == SDC_HCI_MSG_TYPE_NONE &&
	    fetch_hci_msg(&rx_hci_msg.buf[0], &rx_hci_msg.type) != 0) {
		return;
	}

	err = process_hci_msg(dev, &rx_hci_msg.buf[0], rx_hci_msg.type);
	if (err == -ENOBUFS) {
		/* If we got -ENOBUFS, wait for the signal from the host. */
		return;
	} else if (err) {
		LOG_ERR("Unknown error when processing hci message %d", err);
		k_panic();
	}

	rx_hci_msg.type = SDC_HCI_MSG_TYPE_NONE;

	/* Let other threads of same priority run in between. */
	receive_signal_raise();
}

static void receive_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	hci_driver_receive_process();
}

static const struct device *entropy_source = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));

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

	if (IS_ENABLED(CONFIG_BT_CTLR_DF_ADV_CTE_TX)) {
		err = sdc_support_le_connectionless_cte_transmitter();
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

		if (IS_ENABLED(CONFIG_BT_CTLR_LE_PATH_LOSS_MONITORING)) {
			err = sdc_support_le_path_loss_monitoring();
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

	if (IS_ENABLED(CONFIG_BT_CTLR_SDC_QOS_CHANNEL_SURVEY)) {
		err = sdc_support_qos_channel_survey();
		if (err) {
			return -ENOTSUP;
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

	if (IS_ENABLED(CONFIG_BT_CTLR_PERIPHERAL_ISO)) {
		err = sdc_support_cis_peripheral();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_CENTRAL_ISO)) {
		err = sdc_support_cis_central();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_ISO)) {
		err = sdc_support_bis_source();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_SDC_IGNORE_HCI_ISO_DATA_TS_FROM_HOST)) {
		err = sdc_iso_host_timestamps_ignore(true);
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_ISO)) {
		err = sdc_support_bis_sink();
		if (err) {
			return -ENOTSUP;
		}
	}

#if defined(CONFIG_BT_CTLR_SDC_ALLOW_PARALLEL_SCANNING_AND_INITIATING)
	err = sdc_support_parallel_scanning_and_initiating();
	if (err) {
		return -ENOTSUP;
	}
#endif

	if (IS_ENABLED(CONFIG_BT_CTLR_SUBRATING)) {
		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			err = sdc_support_connection_subrating_central();
			if (err) {
				return -ENOTSUP;
			}
		}
		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			err = sdc_support_connection_subrating_peripheral();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_FRAME_SPACE_UPDATE)) {
		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			err = sdc_support_frame_space_update_central();
			if (err) {
				return -ENOTSUP;
			}
		}
		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			err = sdc_support_frame_space_update_peripheral();
			if (err) {
				return -ENOTSUP;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_EXTENDED_FEAT_SET)) {
		err = sdc_support_extended_feature_set();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_CHANNEL_SOUNDING_TEST)) {
		err = sdc_support_channel_sounding_test();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_CHANNEL_SOUNDING)) {
#if defined(CONFIG_BT_CTLR_SDC_CS_MULTIPLE_ANTENNA_SUPPORT)
		err = sdc_support_channel_sounding(cs_antenna_switch_func);
		cs_antenna_switch_init();
#else
		err = sdc_support_channel_sounding(NULL);
#endif
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_SDC_CS_STEP_MODE3)) {
		err = sdc_support_channel_sounding_mode3();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_SDC_CS_ROLE_INITIATOR_ONLY) ||
		IS_ENABLED(CONFIG_BT_CTLR_SDC_CS_ROLE_BOTH)) {
		err = sdc_support_channel_sounding_initiator_role();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_SDC_CS_ROLE_REFLECTOR_ONLY) ||
		IS_ENABLED(CONFIG_BT_CTLR_SDC_CS_ROLE_BOTH)) {
		err = sdc_support_channel_sounding_reflector_role();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_SDC_LE_POWER_CLASS_1)) {
		err = sdc_support_le_power_class_1();
		if (err) {
			return -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PRIVACY)) {
		err = sdc_support_le_privacy();
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

#if defined(CONFIG_BT_CTLR_SDC_ISO_RX_PDU_BUFFER_PER_STREAM_COUNT)
	uint8_t iso_rx_paths = 0;
#endif

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

	cfg.fal_size = CONFIG_BT_CTLR_FAL_SIZE;
	required_memory =
		sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
					   SDC_CFG_TYPE_FAL_SIZE,
					   &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

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

#if defined(CONFIG_BT_OBSERVER)
	cfg.scan_buffer_cfg.count = CONFIG_BT_CTLR_SDC_SCAN_BUFFER_COUNT;

	required_memory =
	sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
			SDC_CFG_TYPE_SCAN_BUFFER_CFG,
			&cfg);
	if (required_memory < 0) {
		return required_memory;
	}
#endif /* CONFIG_BT_OBSERVER */

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

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	cfg.cig_count.count = CONFIG_BT_CTLR_CONN_ISO_GROUPS;
	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
					  SDC_CFG_TYPE_CIG_COUNT, &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

	iso_rx_paths = CONFIG_BT_CTLR_CONN_ISO_STREAMS;
	cfg.cis_count.count = CONFIG_BT_CTLR_CONN_ISO_STREAMS;
	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
					  SDC_CFG_TYPE_CIS_COUNT, &cfg);
	if (required_memory < 0) {
		return required_memory;
	}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_BROADCAST_ISO)
	cfg.big_count.count = 0;
#if defined(CONFIG_BT_CTLR_ADV_ISO)
	cfg.big_count.count += CONFIG_BT_CTLR_ADV_ISO_SET;
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	cfg.big_count.count += CONFIG_BT_CTLR_SCAN_SYNC_ISO_SET;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
					  SDC_CFG_TYPE_BIG_COUNT, &cfg);
	if (required_memory < 0) {
		return required_memory;
	}

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	cfg.bis_source_count.count = CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT;
	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
					  SDC_CFG_TYPE_BIS_SOURCE_COUNT, &cfg);
	if (required_memory < 0) {
		return required_memory;
	}
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	cfg.bis_sink_count.count = CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT;
	iso_rx_paths += CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT;
	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
					  SDC_CFG_TYPE_BIS_SINK_COUNT, &cfg);
	if (required_memory < 0) {
		return required_memory;
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
#endif /* CONFIG_BT_CTLR_BROADCAST_ISO */

#if defined(CONFIG_BT_CTLR_SDC_ISO_RX_PDU_BUFFER_PER_STREAM_COUNT) ||                             \
	(defined(CONFIG_BT_CTLR_SDC_ISO_TX_HCI_BUFFER_COUNT) &&                                   \
	defined(CONFIG_BT_CTLR_SDC_ISO_TX_PDU_BUFFER_PER_STREAM_COUNT))
	memset(&cfg.iso_buffer_cfg, 0, sizeof(cfg.iso_buffer_cfg));
#endif

#if defined(CONFIG_BT_CTLR_SDC_ISO_RX_PDU_BUFFER_PER_STREAM_COUNT)
	cfg.iso_buffer_cfg.rx_pdu_buffer_per_stream_count =
		CONFIG_BT_CTLR_SDC_ISO_RX_PDU_BUFFER_PER_STREAM_COUNT;

	cfg.iso_buffer_cfg.rx_sdu_buffer_count = iso_rx_paths;
	cfg.iso_buffer_cfg.rx_sdu_buffer_size = CONFIG_BT_ISO_RX_MTU;
#endif

#if defined(CONFIG_BT_CTLR_SDC_ISO_TX_HCI_BUFFER_COUNT) &&                                         \
	defined(CONFIG_BT_CTLR_SDC_ISO_TX_PDU_BUFFER_PER_STREAM_COUNT)
	cfg.iso_buffer_cfg.tx_sdu_buffer_count = CONFIG_BT_CTLR_SDC_ISO_TX_HCI_BUFFER_COUNT;
	cfg.iso_buffer_cfg.tx_sdu_buffer_size = CONFIG_BT_ISO_TX_MTU;
	cfg.iso_buffer_cfg.tx_pdu_buffer_per_stream_count =
		CONFIG_BT_CTLR_SDC_ISO_TX_PDU_BUFFER_PER_STREAM_COUNT;
#endif

#if defined(CONFIG_BT_CTLR_SDC_ISO_RX_PDU_BUFFER_PER_STREAM_COUNT) ||                             \
	(defined(CONFIG_BT_CTLR_SDC_ISO_TX_HCI_BUFFER_COUNT) &&                                   \
	defined(CONFIG_BT_CTLR_SDC_ISO_TX_PDU_BUFFER_PER_STREAM_COUNT))
	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
					  SDC_CFG_TYPE_ISO_BUFFER_CFG, &cfg);
	if (required_memory < 0) {
		return required_memory;
	}
#endif

#if defined(CONFIG_BT_CTLR_SDC_CS_COUNT)
	cfg.cs_count.count = CONFIG_BT_CTLR_SDC_CS_COUNT;
	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
									SDC_CFG_TYPE_CS_COUNT,
									&cfg);
	if (required_memory < 0) {
		return required_memory;
	}
#endif

#if defined(CONFIG_BT_CTLR_SDC_CS_COUNT)
	cfg.cs_cfg.max_antenna_paths_supported = CONFIG_BT_CTLR_SDC_CS_MAX_ANTENNA_PATHS;
	cfg.cs_cfg.num_antennas_supported = CONFIG_BT_CTLR_SDC_CS_NUM_ANTENNAS;

	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
									SDC_CFG_TYPE_CS_CFG,
									&cfg);
	if (required_memory < 0) {
		return required_memory;
	}
#endif

#if defined(CONFIG_BT_CTLR_SDC_EXTENDED_FEAT_MAX_REMOTE_PAGE)
	cfg.extended_feature_page_count = CONFIG_BT_CTLR_SDC_EXTENDED_FEAT_MAX_REMOTE_PAGE;
	required_memory = sdc_cfg_set(SDC_DEFAULT_RESOURCE_CFG_TAG,
				      SDC_CFG_TYPE_EXTENDED_FEATURE_PAGE_COUNT, &cfg);
	if (required_memory < 0) {
		return required_memory;
	}
#endif

	LOG_DBG("BT mempool size: %zu, required: %u", sizeof(sdc_mempool), required_memory);

	if (required_memory > sizeof(sdc_mempool)) {
		LOG_ERR("Allocated memory too low: %zu < %u",
		       sizeof(sdc_mempool), required_memory);
		k_panic();
		/* No return from k_panic(). */
		return -ENOMEM;
	}

	return 0;
}

static int hci_driver_open(const struct device *dev, bt_hci_recv_t recv_func)
{
	LOG_DBG("Open");

	k_work_init(&receive_work, receive_work_handler);

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

	err = sdc_enable(hci_driver_receive_process, sdc_mempool);
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return err;
	}

#if defined(CONFIG_BT_PER_ADV)
	sdc_hci_cmd_vs_periodic_adv_event_length_set_t per_adv_length_params = {
		.event_length_us = CONFIG_BT_CTLR_SDC_PERIODIC_ADV_EVENT_LEN_DEFAULT
	};
	err = sdc_hci_cmd_vs_periodic_adv_event_length_set(&per_adv_length_params);
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return -ENOTSUP;
	}
#endif /* CONFIG_BT_PER_ADV */

#if defined(CONFIG_BT_CTLR_CHANNEL_SOUNDING)
	sdc_hci_cmd_vs_cs_params_set_t cs_params_set_event_length = {
		.cs_param_type = SDC_HCI_VS_CS_PARAM_TYPE_CS_EVENT_LENGTH_SET,
		.cs_param_data.cs_event_length_params.cs_event_length_us =
			CONFIG_BT_CTLR_SDC_CS_EVENT_LEN_DEFAULT
	};
	err = sdc_hci_cmd_vs_cs_params_set(&cs_params_set_event_length);
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return -ENOTSUP;
	}
#endif /* CONFIG_BT_CTLR_CHANNEL_SOUNDING */

#if defined(CONFIG_BT_CTLR_CHANNEL_SOUNDING)
	sdc_hci_cmd_vs_cs_params_set_t cs_params_set_t_pm_length = {
		.cs_param_type = SDC_HCI_VS_CS_PARAM_TYPE_CS_T_PM_SET,
		.cs_param_data.cs_t_pm_params.cs_t_pm_length_us =
			CONFIG_BT_CTLR_SDC_CS_T_PM_LEN_DEFAULT
	};
	err = sdc_hci_cmd_vs_cs_params_set(&cs_params_set_t_pm_length);
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return -ENOTSUP;
	}
#endif /* CONFIG_BT_CTLR_CHANNEL_SOUNDING */

#if defined(CONFIG_BT_CTLR_SDC_BIG_RESERVED_TIME_US)
	sdc_hci_cmd_vs_big_reserved_time_set_t big_reserved_time_params = {
		.reserved_time_us = CONFIG_BT_CTLR_SDC_BIG_RESERVED_TIME_US
	};
	err = sdc_hci_cmd_vs_big_reserved_time_set(&big_reserved_time_params);
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return -ENOTSUP;
	}
#endif /* BT_CTLR_SDC_BIG_RESERVED_TIME_US*/

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	sdc_hci_cmd_vs_cig_reserved_time_set_t cig_reserved_time_params = {
		.reserved_time_us = CONFIG_BT_CTLR_SDC_CIG_RESERVED_TIME_US
	};
	err = sdc_hci_cmd_vs_cig_reserved_time_set(&cig_reserved_time_params);
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return -ENOTSUP;
	}

	sdc_hci_cmd_vs_cis_subevent_length_set_t cis_subevent_length_params = {
		.cis_subevent_length_us = CONFIG_BT_CTLR_SDC_CIS_SUBEVENT_LENGTH_US
	};
	err = sdc_hci_cmd_vs_cis_subevent_length_set(&cis_subevent_length_params);
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return -ENOTSUP;
	}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */

#if defined(CONFIG_BT_CONN)
	sdc_hci_cmd_vs_event_length_set_t conn_event_length_params = {
		.event_length_us = CONFIG_BT_CTLR_SDC_MAX_CONN_EVENT_LEN_DEFAULT
	};
	err = sdc_hci_cmd_vs_event_length_set(&conn_event_length_params);
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return -ENOTSUP;
	}

	sdc_hci_cmd_vs_conn_event_extend_t event_extend_params = {
		.enable = IS_ENABLED(CONFIG_BT_CTLR_SDC_CONN_EVENT_EXTEND_DEFAULT)
	};
	err = sdc_hci_cmd_vs_conn_event_extend(&event_extend_params);
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return -ENOTSUP;
	}
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CENTRAL)
	sdc_hci_cmd_vs_central_acl_event_spacing_set_t acl_event_spacing_params = {
		.central_acl_event_spacing_us =
			CONFIG_BT_CTLR_SDC_CENTRAL_ACL_EVENT_SPACING_DEFAULT
	};
	err = sdc_hci_cmd_vs_central_acl_event_spacing_set(&acl_event_spacing_params);
	if (err) {
		MULTITHREADING_LOCK_RELEASE();
		return -ENOTSUP;
	}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_CTLR_MIN_VAL_OF_MAX_ACL_TX_PAYLOAD_DEFAULT)
	if (CONFIG_BT_CTLR_MIN_VAL_OF_MAX_ACL_TX_PAYLOAD_DEFAULT != 27) {
		sdc_hci_cmd_vs_min_val_of_max_acl_tx_payload_set_t params = {
			.min_val_of_max_acl_tx_payload =
				CONFIG_BT_CTLR_MIN_VAL_OF_MAX_ACL_TX_PAYLOAD_DEFAULT
		};
		err = sdc_hci_cmd_vs_min_val_of_max_acl_tx_payload_set(&params);
		if (err) {
			MULTITHREADING_LOCK_RELEASE();
			return -ENOTSUP;
		}
	}
#endif

	MULTITHREADING_LOCK_RELEASE();

	struct hci_driver_data *driver_data = dev->data;

	driver_data->recv_func = recv_func;

	bt_buf_rx_freed_cb_set(bt_buf_rx_freed_cb);

	return 0;
}

static int hci_driver_close(const struct device *dev)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_CTLR_SDC_CS_MULTIPLE_ANTENNA_SUPPORT)) {
		cs_antenna_switch_clear();
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

	bt_buf_rx_freed_cb_set(NULL);

	return err;
}

static const struct bt_hci_driver_api hci_driver_api = {
	.open = hci_driver_open,
	.close = hci_driver_close,
	.send = hci_driver_send,
};

void bt_ctlr_set_public_addr(const uint8_t *addr)
{
	const sdc_hci_cmd_vs_zephyr_write_bd_addr_t *bd_addr = (void *)addr;

	(void)sdc_hci_cmd_vs_zephyr_write_bd_addr(bd_addr);
}

static int hci_driver_init(const struct device *dev)
{
	int err = 0;

	err = sdc_init(sdc_assertion_handler);
	if (err) {
		return err;
	}

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

#if defined(CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL)
BUILD_ASSERT(CONFIG_BT_LL_SOFTDEVICE_INIT_PRIORITY > CONFIG_MPSL_INIT_PRIORITY,
			 "MPSL must be initialized before SoftDevice Controller");
#endif /* CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL */

#define BT_HCI_CONTROLLER_INIT(inst) \
	static struct hci_driver_data data_##inst; \
	DEVICE_DT_INST_DEFINE(inst, hci_driver_init, NULL, &data_##inst, NULL, POST_KERNEL, \
			      CONFIG_BT_LL_SOFTDEVICE_INIT_PRIORITY, &hci_driver_api)

/* Only a single instance is supported */
BT_HCI_CONTROLLER_INIT(0)
