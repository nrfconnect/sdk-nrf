/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/bos.h>
#include <zephyr/usb/msos_desc.h>
#include <zephyr/net_buf.h>
#include <usb_descriptor.h>

#define MODULE bulk_interface
#include "module_state_event.h"
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_BRIDGE_BULK_LOG_LEVEL);

/* needs to be included after defining logging config */
#include "usb_bulk_msosv2.h"

#define USB_BULK_PACKET_SIZE  64
#define USB_BULK_PACKET_COUNT 4

/* placeholder for IN endpoint number, will be updated during USB init */
#define DAP_USB_EP_IN  0x81
/* placeholder for OUT endpoint number, will be updated during USB init */
#define DAP_USB_EP_OUT 0x01

/* position of OUT endpoint in configuration array */
#define DAP_USB_EP_OUT_IDX 0
/* position of IN endpoint in configuration array */
#define DAP_USB_EP_IN_IDX  1

/* bulk RX FIFO and associated NET buf pool */
static K_FIFO_DEFINE(dap_rx_queue);
NET_BUF_POOL_FIXED_DEFINE(dapusb_rx_pool, USB_BULK_PACKET_COUNT, USB_BULK_PACKET_SIZE, 0, NULL);

/* Execute CMSIS-DAP command and write reply into output buffer */
size_t dap_execute_cmd(uint8_t *in, uint8_t *out);

/* string descriptor for the interface */
#define DAP_IFACE_STR_DESC "CMSIS-DAP v2"

struct dap_iface_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bString[USB_BSTRING_LENGTH(DAP_IFACE_STR_DESC)];
} __packed;

USBD_STRING_DESCR_USER_DEFINE(primary)
struct dap_iface_descriptor dap_iface_desc = {
	.bLength = USB_STRING_DESCRIPTOR_LENGTH(DAP_IFACE_STR_DESC),
	.bDescriptorType = USB_DESC_STRING,
	.bString = DAP_IFACE_STR_DESC};

/* USB Class descriptor for custom interface */
USBD_CLASS_DESCR_DEFINE(primary, 0) struct {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
} __packed dapusb_desc = {
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = USB_BCC_VENDOR,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},
	.if0_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = DAP_USB_EP_OUT,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(USB_BULK_PACKET_SIZE),
		.bInterval = 0,
	},
	.if0_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = DAP_USB_EP_IN,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(USB_BULK_PACKET_SIZE),
		.bInterval = 0,
	},
};

static struct usb_ep_cfg_data dapusb_ep_data[] = {
	[DAP_USB_EP_OUT_IDX] = {.ep_cb = usb_transfer_ep_callback, .ep_addr = DAP_USB_EP_OUT},
	[DAP_USB_EP_IN_IDX]  = {.ep_cb = usb_transfer_ep_callback, .ep_addr = DAP_USB_EP_IN},
};

/* helper function to connect string descriptor to the interface during USB init */
static void iface_string_desc_init(struct usb_cfg_data *bulk_cfg)
{
	struct usb_if_descriptor *bulk_if = bulk_cfg->interface_descriptor;

	bulk_if->iInterface = usb_get_str_descriptor_idx(&dap_iface_desc);
}

void dapusb_interface_config(struct usb_desc_header *head, uint8_t bInterfaceNumber)
{
	ARG_UNUSED(head);

	dapusb_desc.if0.bInterfaceNumber = bInterfaceNumber;
#if defined(CONFIG_USB_COMPOSITE_DEVICE)
	msosv2_cmsis_dap_desc.subset_header.bFirstInterface = bInterfaceNumber;
#endif
}

static void dapusb_read_cb(uint8_t ep, int size, void *priv)
{
	struct usb_cfg_data *cfg = priv;
	struct net_buf *buf;
	static uint8_t rx_buf[USB_BULK_PACKET_SIZE];

	LOG_DBG("cfg %p ep %x size %u", cfg, ep, size);

	if (size <= 0) {
		goto read_cb_done;
	}

	buf = net_buf_alloc(&dapusb_rx_pool, K_FOREVER);
	net_buf_add_mem(buf, rx_buf, MIN(size, USB_BULK_PACKET_SIZE));
	k_fifo_put(&dap_rx_queue, buf);

read_cb_done:
	usb_transfer(ep, rx_buf, sizeof(rx_buf), USB_TRANS_READ, dapusb_read_cb, cfg);
}

/* when USB device is configured, start reading RX endpoint */
static void dapusb_dev_status_cb(struct usb_cfg_data *cfg, enum usb_dc_status_code status,
				 const uint8_t *param)
{
	ARG_UNUSED(param);

	LOG_WRN("USB status %d", status);

	if (status == USB_DC_CONFIGURED) {
		dapusb_read_cb(cfg->endpoint[DAP_USB_EP_OUT_IDX].ep_addr, 0, cfg);
	}
}

/* add interface to USB configuration */
USBD_DEFINE_CFG_DATA(dapusb_config) = {
	.usb_device_description = NULL,
	.interface_config = dapusb_interface_config,
	.interface_descriptor = &dapusb_desc.if0,
	.cb_usb_status = dapusb_dev_status_cb,
	.interface = {
		.class_handler = NULL,
		.custom_handler = NULL,
		.vendor_handler = msosv2_vendor_handle_req,
	},
	.num_endpoints = ARRAY_SIZE(dapusb_ep_data),
	.endpoint = dapusb_ep_data
};

static int dap_usb_process(void)
{
	size_t len;
	int err;
	static uint8_t tx_buf[USB_BULK_PACKET_SIZE];
	struct net_buf *buf = k_fifo_get(&dap_rx_queue, K_FOREVER);
	uint8_t ep = dapusb_config.endpoint[DAP_USB_EP_IN_IDX].ep_addr;

	len = dap_execute_cmd(buf->data, tx_buf);
	LOG_DBG("response length %u, starting with [0x%02X, 0x%02X]", len, tx_buf[0], tx_buf[1]);
	net_buf_unref(buf);

	err = usb_transfer_sync(ep, tx_buf, len, USB_TRANS_WRITE | USB_TRANS_NO_ZLP);
	if (err < 0 || err != len) {
		LOG_ERR("usb_transfer_sync failed, %d", err);
		return -EIO;
	}

	return 0;
}

static int dap_usb_thread_fn(const struct device *dev)
{
	ARG_UNUSED(dev);

	while (1) {
		dap_usb_process();
	}

	return 0;
}

K_THREAD_DEFINE(dap_usb_thread,
		CONFIG_BULK_USB_THREAD_STACK_SIZE, dap_usb_thread_fn, NULL, NULL, NULL, 5, 0, 0);

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			/* Add MS OS 2.0 BOS descriptor to BOS structure */
			usb_bos_register_cap((void *)&bos_cap_msosv2);
			/* Point interface index to string descriptor */
			iface_string_desc_init(&dapusb_config);

			/* tell the usb_cdc_handler we are done */
			module_set_state(MODULE_STATE_STANDBY);
		}
		return false;
	}
	/* we should not reach this point */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
