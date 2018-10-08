/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file rtt.c
 *
 * Ethernet interface driver that transfers frames using SEGGER J-Link RTT.
 * This is driver is meant to be used for debugging and testing purpose.
 * Additional software is required on PC side that will be able to correctly
 * handle frames that are transferred via dedicated RTT channel.
 *
 * Before frame goes to RTT this driver calculates CRC of entire frame and adds
 * two bytes of CRC at the end (big endian order). CRC is calculated using
 * CRC-16/CCITT with initial seed 0xFFFF and no final xoring. RTT requires
 * stream transfer, so frames are serialized using SLIP encoding. SLIP END
 * byte (300 octal) is send before and after the frame, so empty frames
 * produced during SLIP decoding should be ignored.
 *
 * Specific RTT channel number is not assigned to transfer ethernet frames,
 * so software on PC side have to search for channels named "ETH_RTT".
 * PC side may want to know when device was reset. Driver sends one special
 * frame during driver initialization. See @a reset_frame_data.
 *
 * MTU for this driver is configurable. Longer frames received from PC will be
 * discarted, so make sure that software on PC side is configured with the same
 * MTU.
 */

#define SYS_LOG_DOMAIN "dev/eth_rtt"
#define SYS_LOG_LEVEL CONFIG_ETH_RTT_LOG_LEVEL
#include <logging/sys_log.h>
#include <stdio.h>
#include <kernel.h>
#include <stdbool.h>
#include <stddef.h>
#include <net/ethernet.h>
#include <net/buf.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/ethernet.h>
#include <crc16.h>
#include <rtt/SEGGER_RTT.h>

/** RTT channel name used to identify Ethernet transfer channel. */
#define CHANNEL_NAME "ETH_RTT"

/* SLIP special bytes definitions. */
#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

/** Size of the buffer used to gather data received from RTT. */
#define RX_BUFFER_SIZE (CONFIG_ETH_RTT_MTU + 36)

/** Number of poll periods that must pass to assume that transfer is
 *  not running for now.
 */
#define ACTIVE_POLL_COUNT (CONFIG_ETH_POLL_PERIOD_MS / \
			   CONFIG_ETH_POLL_ACTIVE_PERIOD_MS)

BUILD_ASSERT_MSG(CONFIG_ETH_RTT_CHANNEL < SEGGER_RTT_MAX_NUM_UP_BUFFERS,
		 "RTT channel number used in RTT network driver "
		 "must be lower than SEGGER_RTT_MAX_NUM_UP_BUFFERS");

/** Context structure that holds current state of the driver. */
struct eth_rtt_context {

	/** Initialization is done and context is ready to be used. */
	bool init_done;

	/** Network interface associated with this driver. */
	struct net_if *iface;

	/** Counter that counts down number of polls that must be done before
	 *  assuming that transfer is not running for now.
	 */
	u16_t active_poll_counter;

	/** CRC of currently sending frame to RTT. */
	u16_t crc;

	/** MAC address of this interface. */
	u8_t mac_addr[6];

	/** Buffer that contains currently receiving frame from RTT. Frame
	 *  contained in the buffer is partial and as soon as it gets finalized
	 *  it will be send to the network stack and removed from the buffer.
	 *  Data in the buffer are already SLIP decoded.
	 */
	u8_t rx_buffer[RX_BUFFER_SIZE];

	/** Number of bytes currently occupied in rx_buffer. */
	size_t rx_buffer_length;

	/** Up buffer used by RTT library */
	u8_t rtt_up_buffer[CONFIG_ETH_RTT_UP_BUFFER_SIZE];

	/** Down buffer used by RTT library */
	u8_t rtt_down_buffer[CONFIG_ETH_RTT_DOWN_BUFFER_SIZE];
};

/** Frame send when the interface is initialized to inform other end point
 *  about board reset.
 */
static const u8_t reset_frame_data[] = {
	0, 0, 0, 0, 0, 0,            /* dummy destination MAC address */
	0, 0, 0, 0, 0, 0,            /* dummy source MAC address */
	254, 255,                    /* custom eth type */
	216, 33, 105, 148, 78, 111,  /* randomly generated magic payload */
	203, 53, 32, 137, 247, 122,  /* randomly generated magic payload */
	100, 72, 129, 255, 204, 173, /* randomly generated magic payload */
	};

/** Static context for this driver. */
static struct eth_rtt_context context_data;

#if (SYS_LOG_LEVEL >= SYS_LOG_LEVEL_DEBUG) && \
	defined(CONFIG_ETH_RTT_DEBUG_HEX_DUMP)

/** Utility function to dump all transferred data. */
void DBG_HEX_DUMP(const char *prefix, const u8_t *data, int length)
{
	int i;
	char line[64];
	char *ptr;

	while (length > 0) {
		ptr = line;
		for (i = 0; i < 16 && length > 0; i++) {
			ptr += sprintf(ptr, " %02X", *data);
			data++;
			length--;
		}
		SYS_LOG_DBG("%s%s", prefix, line);
	}
}

/** Utility macro to indicate start of frame before dumping its contents */
#define DBG_HEX_DUMP_BEGIN(prefix) SYS_LOG_DBG(prefix " begin")

/** Utility macro to indicate end of frame after dumping its contents */
#define DBG_HEX_DUMP_END(prefix) SYS_LOG_DBG(prefix " end")

#else

#define DBG_HEX_DUMP(...)
#define DBG_HEX_DUMP_BEGIN(...)
#define DBG_HEX_DUMP_END(...)

#endif

/*********** OUTPUT PART OF THE DRIVER (from network stack to RTT) ***********/

/** Sends start of frame (SLIP_END) to RTT up channel.
 *  @param context   Driver context.
 */
static void rtt_send_begin(struct eth_rtt_context *context)
{
	u8_t data = SLIP_END;

	SEGGER_RTT_Write(CONFIG_ETH_RTT_CHANNEL, &data, sizeof(data));
	DBG_HEX_DUMP_BEGIN("RTT<");
	DBG_HEX_DUMP("RTT<", &data, sizeof(data));
	context->crc = 0xFFFF;
}

/** Encodes fragment of frame using SLIP and sends it to RTT up channel.
 *  @param context   Driver context.
 *  @param ptr       Points data to send.
 *  @param len       Number of bytes to send.
 */
static void rtt_send_fragment(struct eth_rtt_context *context, const u8_t *ptr,
			      int len)
{
	static const u8_t end_stuffed[2] = { SLIP_ESC, SLIP_ESC_END };
	static const u8_t esc_stuffed[2] = { SLIP_ESC, SLIP_ESC_ESC };
	const u8_t *end = ptr + len;
	const u8_t *plain_begin = ptr;

	context->crc = crc16_ccitt(context->crc, ptr, len);

	while (ptr < end) {
		if (*ptr == SLIP_END || *ptr == SLIP_ESC) {
			if (ptr > plain_begin) {
				SEGGER_RTT_Write(CONFIG_ETH_RTT_CHANNEL,
						 plain_begin,
						 ptr - plain_begin);
				DBG_HEX_DUMP("RTT<", plain_begin,
					     ptr - plain_begin);
			}

			if (*ptr == SLIP_END) {
				SEGGER_RTT_Write(CONFIG_ETH_RTT_CHANNEL,
						 end_stuffed,
						 sizeof(end_stuffed));
				DBG_HEX_DUMP("RTT<", end_stuffed,
					     sizeof(end_stuffed));
			} else if (*ptr == SLIP_ESC) {
				SEGGER_RTT_Write(CONFIG_ETH_RTT_CHANNEL,
						 esc_stuffed,
						 sizeof(esc_stuffed));
				DBG_HEX_DUMP("RTT<", esc_stuffed,
					     sizeof(esc_stuffed));
			}
			plain_begin = ptr + 1;
		}
		ptr++;
	}

	if (ptr > plain_begin) {
		SEGGER_RTT_Write(CONFIG_ETH_RTT_CHANNEL, plain_begin,
				 ptr - plain_begin);
		DBG_HEX_DUMP("RTT<", plain_begin, ptr - plain_begin);
	}
}

/** Sends end of frame (SLIP_END) to RTT up channel.
 *  @param context   Driver context.
 */
static void rtt_send_end(struct eth_rtt_context *context)
{
	u8_t crc_buffer[2] = { context->crc >> 8, context->crc & 0xFF };
	u8_t data = SLIP_END;

	rtt_send_fragment(context, crc_buffer, sizeof(crc_buffer));
	SEGGER_RTT_Write(CONFIG_ETH_RTT_CHANNEL, &data, sizeof(data));
	DBG_HEX_DUMP("RTT<", &data, sizeof(data));
	DBG_HEX_DUMP_END("RTT<");
}

/** Callback function called by network stack when new frame arrived to the
 *  interface.
 *  @param iface   Network interface associated with this driver.
 *  @param pkt     Frame that have to be send.
 */
static int eth_iface_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct device *dev = net_if_get_device(iface);
	struct eth_rtt_context *context = dev->driver_data;
	struct net_buf *frag;

#if SYS_LOG_LEVEL >= SYS_LOG_LEVEL_DEBUG
	int total_len = net_pkt_ll_reserve(pkt);

	for (frag = pkt->frags; frag; frag = frag->frags) {
		total_len += frag->len;
	}
	SYS_LOG_DBG("Sending %d byte(s) frame", total_len);
#endif

	if (!pkt->frags) {
		return -ENODATA;
	}

	DBG_HEX_DUMP_BEGIN("ETH>");
	rtt_send_begin(context);

	DBG_HEX_DUMP("ETH>", net_pkt_ll(pkt), net_pkt_ll_reserve(pkt));
	rtt_send_fragment(context, net_pkt_ll(pkt), net_pkt_ll_reserve(pkt));

	for (frag = pkt->frags; frag; frag = frag->frags) {
		DBG_HEX_DUMP("ETH>", frag->data, frag->len);
		rtt_send_fragment(context, frag->data, frag->len);
	};

	DBG_HEX_DUMP_END("ETH>");
	rtt_send_end(context);

	net_pkt_unref(pkt);

	return 0;
}

/*********** INPUT PART OF THE DRIVER (from RTT to network stack) ***********/

/** Function called when new frame was received from RTT. It checks CRC and
 *  passes frame to the network stack.
 *  @param context   Driver context.
 *  @param data      Frame data to pass to network stack with two bytes of CRC.
 *  @param len       Number of bytes in data parameter.
 */
static void recv_frame(struct eth_rtt_context *context, u8_t *data, int len)
{
	int res;
	struct net_pkt *pkt;
	struct net_buf *pkt_buf = NULL;
	struct net_buf *last_buf = NULL;
	u16_t crc16;

	if (len <= 2) {
		if (len > 0) {
			SYS_LOG_ERR("Invalid frame length");
		}
		return;
	}

	crc16 = crc16_ccitt(0xFFFF, data, len - 2);
	if (data[len - 2] != (crc16 >> 8) || data[len - 1] != (crc16 & 0xFF)) {
		SYS_LOG_ERR("Invalid frame CRC");
		return;
	}

	len -= 2;

	SYS_LOG_DBG("Received %d byte(s) frame", len);

	pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
	if (!pkt) {
		SYS_LOG_ERR("Could not allocate rx pkt");
		return;
	}

	DBG_HEX_DUMP_BEGIN("ETH<");

	while (len > 0) {
		pkt_buf = net_pkt_get_frag(pkt, K_NO_WAIT);
		if (!pkt_buf) {
			SYS_LOG_ERR("Could not allocate data for rx pkt");
			net_pkt_unref(pkt);
			return;
		}

		if (!last_buf) {
			net_pkt_frag_insert(pkt, pkt_buf);
		} else {
			net_buf_frag_insert(last_buf, pkt_buf);
		}

		last_buf = pkt_buf;

		size_t frag_len = net_buf_tailroom(pkt_buf);

		if (len < frag_len) {
			frag_len = len;
		}

		memcpy(pkt_buf->data, data, frag_len);

		DBG_HEX_DUMP("ETH<", data, frag_len);

		len -= frag_len;
		data += frag_len;

		net_buf_add(pkt_buf, frag_len);
	}

	DBG_HEX_DUMP_END("ETH<");

	res = net_recv_data(context->iface, pkt);

	if (res < 0) {
		LOG_ERR("Failed to enqueue frame into RX queue: %d", res);
		net_pkt_unref(pkt);
	}
}

/** Functions decodes SLIP data and passes each decoded frame to recv_frame
 *  function. It decodes data from rx_buffer that were recently added to it,
 *  i.e. decoding starts at rx_buffer_length. Decoding is done inplace, so
 *  decoded data are written back to the rx_buffer. If there is unfinished
 *  frame left then it will be moved to the beginning of the rx_buffer and
 *  rx_buffer_length will be updated accordingly. Otherwise rx_buffer will be
 *  cleared.
 *  @param context        Driver context.
 *  @param new_data_size  Number of bytes that have to be decoded.
 */
static void decode_new_slip_data(struct eth_rtt_context *context,
				 int new_data_size)
{
	u8_t *src = &context->rx_buffer[context->rx_buffer_length];
	u8_t *dst = &context->rx_buffer[context->rx_buffer_length];
	u8_t *end = src + new_data_size;
	u8_t *start = context->rx_buffer;
	u8_t last_byte = context->rx_buffer_length > 0 ? dst[-1] : 0;

	while (src < end) {
		u8_t byte = *src++;
		*dst++ = byte;
		if (byte == SLIP_END) {
			recv_frame(context, start, dst - start - 1);
			start = dst;
		} else if (last_byte == SLIP_ESC) {
			if (byte == SLIP_ESC_END) {
				dst--;
				dst[-1] = SLIP_END;
			} else if (byte == SLIP_ESC_ESC) {
				dst--;
				dst[-1] = SLIP_ESC;
			}
		}
		last_byte = byte;
	}

	context->rx_buffer_length = dst - start;

	if (context->rx_buffer_length > 0 && start != context->rx_buffer) {
		memmove(context->rx_buffer, start, context->rx_buffer_length);
	}
}

static void poll_timer_handler(struct k_timer *dummy);

/** Timer used to do polling RTT down channel */
K_TIMER_DEFINE(eth_rtt_poll_timer, poll_timer_handler, NULL);

/** Work handler that is submitted to system workqueue by the poll timer.
 *  It is responsible for reading all available data from RTT down buffer.
 */
static void poll_work_handler(struct k_work *work)
{
	struct eth_rtt_context *context = &context_data;
	u32_t total = 0;
	s32_t period = K_MSEC(CONFIG_ETH_POLL_PERIOD_MS);
	int num;

	do {
		if (context->rx_buffer_length >= sizeof(context->rx_buffer)) {
			SYS_LOG_ERR("RX buffer overflow. "
				    "Discarding buffer contents.\n");
			context->rx_buffer_length = 0;
		}
		num = SEGGER_RTT_Read(CONFIG_ETH_RTT_CHANNEL,
			&context->rx_buffer[context->rx_buffer_length],
			sizeof(context->rx_buffer) - context->rx_buffer_length);
		if (num > 0) {
			DBG_HEX_DUMP("RTT>",
				&context->rx_buffer[context->rx_buffer_length],
				num);
			decode_new_slip_data(context, num);
			total += num;
		}
	} while (num > 0);

	if (total > 0) {
		context->active_poll_counter = ACTIVE_POLL_COUNT;
		period = K_MSEC(CONFIG_ETH_POLL_ACTIVE_PERIOD_MS);
	} else if (context->active_poll_counter > 0) {
		context->active_poll_counter--;
		period = K_MSEC(CONFIG_ETH_POLL_ACTIVE_PERIOD_MS);
	}
	k_timer_start(&eth_rtt_poll_timer, period, K_MSEC(5000));
}

/** Timer used to do polling RTT down channel */
K_WORK_DEFINE(eth_rtt_poll_work, poll_work_handler);

/** Polling timer handler. It just submits work to system workqueue. */
static void poll_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&eth_rtt_poll_work);
}

/******** COMMON PART OF THE DRIVER (initialization on configuration) ********/

/** Network interface initialization. It setups driver context, MAC address,
 *  starts poll timer and sends reset packet to RTT.
 *  @param iface   Interface to initialize.
 */
static void eth_iface_init(struct net_if *iface)
{
	int mac_addr_bytes = -1;
	struct eth_rtt_context *context = net_if_get_device(iface)->driver_data;

	ethernet_init(iface);

	if (context->init_done) {
		return;
	}

	context->init_done = true;
	context->iface = iface;
	context->active_poll_counter = 0;

#if defined(CONFIG_ETH_RTT_MAC_ADDR)
	if (CONFIG_ETH_RTT_MAC_ADDR[0] != 0) {
		mac_addr_bytes = net_bytes_from_str(context->mac_addr,
						    sizeof(context->mac_addr),
						    CONFIG_ETH_RTT_MAC_ADDR);
	}
#endif

	if (mac_addr_bytes < 0) {
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = sys_rand32_get();
	}

	net_if_set_link_addr(iface, context->mac_addr,
			     sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	k_timer_start(&eth_rtt_poll_timer, K_MSEC(CONFIG_ETH_POLL_PERIOD_MS),
		      K_MSEC(5000));

	SYS_LOG_INF("Initialized '%s': "
		    "MAC addr %02X:%02X:%02X:%02X:%02X:%02X, "
		    "MTU %d, RTT channel %d, RAM consumed %d",
		    iface->if_dev->dev->config->name, context->mac_addr[0],
		    context->mac_addr[1], context->mac_addr[2],
		    context->mac_addr[3], context->mac_addr[4],
		    context->mac_addr[5], CONFIG_ETH_RTT_MTU,
		    CONFIG_ETH_RTT_CHANNEL, sizeof(*context));

	rtt_send_begin(context);
	rtt_send_fragment(context, reset_frame_data, sizeof(reset_frame_data));
	rtt_send_end(context);
}

/** Returns network driver capabilities. Currently no additional capabilities
 *  available.
 */
static enum ethernet_hw_caps eth_capabilities(struct device *dev)
{
	return (enum ethernet_hw_caps)0;
}

/** Network driver initialization. It setups RTT channels.
 *  @param dev   Device to initialize.
 */
static int eth_rtt_init(struct device *dev)
{
	struct eth_rtt_context *context = dev->driver_data;

	SEGGER_RTT_ConfigUpBuffer(CONFIG_ETH_RTT_CHANNEL, CHANNEL_NAME,
				  context->rtt_up_buffer,
				  sizeof(context->rtt_up_buffer),
				  SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
	SEGGER_RTT_ConfigDownBuffer(CONFIG_ETH_RTT_CHANNEL, CHANNEL_NAME,
				    context->rtt_down_buffer,
				    sizeof(context->rtt_down_buffer),
				    SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);

	return 0;
}

/** Network interface API callbacks implemented by this driver. */
static const struct ethernet_api if_api = {
	.iface_api.init = eth_iface_init,
	.iface_api.send = eth_iface_send,
	.get_capabilities = eth_capabilities,
};

/** Initialization of network device driver. */
ETH_NET_DEVICE_INIT(eth_rtt, CONFIG_ETH_RTT_DRV_NAME, eth_rtt_init,
		    &context_data, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &if_api, CONFIG_ETH_RTT_MTU);
