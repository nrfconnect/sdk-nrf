/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/bos.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(audio_usb);

/* By default, do not register the USB DFU class DFU mode instance. */
static const char *const blocklist[] = {
	"dfu_dfu",
	NULL,
};

/*
 * Instantiate a context named audio_usbd using the default USB device
 * controller, the project vendor ID, and the project product ID.
 * Project vendor ID must not be used outside of this project.
 */
USBD_DEVICE_DEFINE(audio_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), CONFIG_USBD_VID,
		   CONFIG_USBD_PID);

USBD_DESC_LANG_DEFINE(audio_lang);
USBD_DESC_MANUFACTURER_DEFINE(audio_mfr, CONFIG_USBD_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(audio_product, CONFIG_USBD_PRODUCT);

IF_ENABLED(CONFIG_HWINFO, (USBD_DESC_SERIAL_NUMBER_DEFINE(audio_sn)));

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");

static const uint8_t attributes =
	(IS_ENABLED(CONFIG_USBD_SELF_POWERED) ? USB_SCD_SELF_POWERED : 0) |
	(IS_ENABLED(CONFIG_USBD_REMOTE_WAKEUP) ? USB_SCD_REMOTE_WAKEUP : 0);

/* Full speed configuration */
USBD_CONFIGURATION_DEFINE(audio_fs_config, attributes, CONFIG_USBD_MAX_POWER, &fs_cfg_desc);

static struct usbd_context *audio_usbd_setup_device(usbd_msg_cb_t msg_cb)
{
	int ret;

	ret = usbd_add_descriptor(&audio_usbd, &audio_lang);
	if (ret) {
		LOG_ERR("Failed to initialize language descriptor (%d)", ret);
		return NULL;
	}

	ret = usbd_add_descriptor(&audio_usbd, &audio_mfr);
	if (ret) {
		LOG_ERR("Failed to initialize manufacturer descriptor (%d)", ret);
		return NULL;
	}

	ret = usbd_add_descriptor(&audio_usbd, &audio_product);
	if (ret) {
		LOG_ERR("Failed to initialize product descriptor (%d)", ret);
		return NULL;
	}

	IF_ENABLED(CONFIG_HWINFO, (ret = usbd_add_descriptor(&audio_usbd, &audio_sn);))
	if (ret) {
		LOG_ERR("Failed to initialize SN descriptor (%d)", ret);
		return NULL;
	}

	ret = usbd_add_configuration(&audio_usbd, USBD_SPEED_FS, &audio_fs_config);
	if (ret) {
		LOG_ERR("Failed to add Full-Speed configuration");
		return NULL;
	}

	ret = usbd_register_all_classes(&audio_usbd, USBD_SPEED_FS, 1, blocklist);
	if (ret) {
		LOG_ERR("Failed to add register classes");
		return NULL;
	}

	/* Interface Association Descriptor */
	usbd_device_set_code_triple(&audio_usbd, USBD_SPEED_FS, USB_BCC_MISCELLANEOUS, 0x02, 0x01);
	usbd_self_powered(&audio_usbd, attributes & USB_SCD_SELF_POWERED);

	if (msg_cb != NULL) {
		ret = usbd_msg_register_cb(&audio_usbd, msg_cb);
		if (ret) {
			LOG_ERR("Failed to register message callback");
			return NULL;
		}
	}

	return &audio_usbd;
}

struct usbd_context *audio_usbd_init_device(usbd_msg_cb_t msg_cb)
{
	int ret;

	if (audio_usbd_setup_device(msg_cb) == NULL) {
		return NULL;
	}

	ret = usbd_init(&audio_usbd);
	if (ret) {
		LOG_ERR("Failed to initialize device support");
		return NULL;
	}

	return &audio_usbd;
}
