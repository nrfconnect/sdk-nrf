/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* {CDB3B5AD-293B-4663-AA36-1AAE46463776} */
#define CMSIS_DAP_V2_DEVICE_INTERFACE_GUID \
	'{', 0x00, 'C', 0x00, 'D', 0x00, 'B', 0x00, '3', 0x00, 'B', 0x00, \
	'5', 0x00, 'A', 0x00, 'D', 0x00, '-', 0x00, '2', 0x00, '9', 0x00, \
	'3', 0x00, 'B', 0x00, '-', 0x00, '4', 0x00, '6', 0x00, '6', 0x00, \
	'3', 0x00, '-', 0x00, 'A', 0x00, 'A', 0x00, '3', 0x00, '6', 0x00, \
	'-', 0x00, '1', 0x00, 'A', 0x00, 'A', 0x00, 'E', 0x00, '4', 0x00, \
	'6', 0x00, '4', 0x00, '6', 0x00, '3', 0x00, '7', 0x00, '7', 0x00, \
	'6', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00

#define COMPATIBLE_ID_WINUSB \
	'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00

#define WINUSB_VENDOR_CODE	0x20

static struct msosv2_descriptor {
	struct msosv2_descriptor_set_header header;
#if defined(CONFIG_USB_COMPOSITE_DEVICE)
	struct msosv2_function_subset_header subset_header;
#endif
	struct msosv2_compatible_id compatible_id;
	struct msosv2_guids_property guids_property;
} __packed msosv2_cmsis_dap_desc = {
	/*
	 * Microsoft OS 2.0 descriptor set. This tells Windows what kind
	 * of device this is and to install the WinUSB driver.
	 */
	.header = {
		.wLength = sizeof(struct msosv2_descriptor_set_header),
		.wDescriptorType = MS_OS_20_SET_HEADER_DESCRIPTOR,
		.dwWindowsVersion = 0x06030000,
		.wTotalLength = sizeof(struct msosv2_descriptor),
	},
#if defined(CONFIG_USB_COMPOSITE_DEVICE)
	.subset_header = {
		.wLength = sizeof(struct msosv2_function_subset_header),
		.wDescriptorType = MS_OS_20_SUBSET_HEADER_FUNCTION,
		.wSubsetLength = sizeof(struct msosv2_function_subset_header)
			+ sizeof(struct msosv2_compatible_id)
			+ sizeof(struct msosv2_guids_property),
	},
#endif
	.compatible_id = {
		.wLength = sizeof(struct msosv2_compatible_id),
		.wDescriptorType = MS_OS_20_FEATURE_COMPATIBLE_ID,
		.CompatibleID = {COMPATIBLE_ID_WINUSB},
	},
	.guids_property = {
		.wLength = sizeof(struct msosv2_guids_property),
		.wDescriptorType = MS_OS_20_FEATURE_REG_PROPERTY,
		.wPropertyDataType = MS_OS_20_PROPERTY_DATA_REG_MULTI_SZ,
		.wPropertyNameLength = 42,
		.PropertyName = {DEVICE_INTERFACE_GUIDS_PROPERTY_NAME},
		.wPropertyDataLength = 80,
		.bPropertyData = {CMSIS_DAP_V2_DEVICE_INTERFACE_GUID},
	},
};

USB_DEVICE_BOS_DESC_DEFINE_CAP struct usb_bos_msosv2_desc {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_msos cap;
} __packed bos_cap_msosv2 = {
	/* Microsoft OS 2.0 Platform Capability Descriptor */
	.platform = {
		.bLength = sizeof(struct usb_bos_platform_descriptor)
			+ sizeof(struct usb_bos_capability_msos),
		.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
		.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
		.bReserved = 0,
		.PlatformCapabilityUUID = {
			/**
			 * MS OS 2.0 Platform Capability ID
			 * D8DD60DF-4589-4CC7-9CD2-659D9E648A9F
			 */
			0xDF, 0x60, 0xDD, 0xD8,
			0x89, 0x45,
			0xC7, 0x4C,
			0x9C, 0xD2,
			0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
		},
	},
	.cap = {
		/* Windows version (8.1) (0x06030000) */
		.dwWindowsVersion = sys_cpu_to_le32(0x06030000),
		.wMSOSDescriptorSetTotalLength =
			sys_cpu_to_le16(sizeof(msosv2_cmsis_dap_desc)),
		.bMS_VendorCode = WINUSB_VENDOR_CODE,
		.bAltEnumCode = 0x00
	},
};

static int msosv2_vendor_handle_req(struct usb_setup_packet *setup,
				    int32_t *len, uint8_t **data)
{
	if (usb_reqtype_is_to_device(setup)) {
		return -ENOTSUP;
	}

	if (setup->bRequest == WINUSB_VENDOR_CODE &&
	    setup->wIndex == MS_OS_20_DESCRIPTOR_INDEX) {
		*data = (uint8_t *)(&msosv2_cmsis_dap_desc);
		*len = sizeof(msosv2_cmsis_dap_desc);

		LOG_DBG("Get MS OS Descriptors v2");

		return 0;
	}

	return -ENOTSUP;
}
