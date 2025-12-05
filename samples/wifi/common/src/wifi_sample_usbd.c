/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/bos.h>

USBD_DEVICE_DEFINE(sample_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   CONFIG_SAMPLE_WIFI_USBD_VID, CONFIG_SAMPLE_WIFI_USBD_PID);

USBD_DESC_LANG_DEFINE(sample_lang);
USBD_DESC_MANUFACTURER_DEFINE(sample_mfr, CONFIG_SAMPLE_WIFI_USBD_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(sample_product, CONFIG_SAMPLE_WIFI_USBD_PRODUCT);

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");

static const uint8_t attributes =
	(IS_ENABLED(CONFIG_SAMPLE_WIFI_USBD_SELF_POWERED) ? USB_SCD_SELF_POWERED : 0) |
	(IS_ENABLED(CONFIG_SAMPLE_WIFI_USBD_REMOTE_WAKEUP) ? USB_SCD_REMOTE_WAKEUP : 0);

USBD_CONFIGURATION_DEFINE(sample_fs_config,
			  attributes,
			  CONFIG_SAMPLE_WIFI_USBD_MAX_POWER, &fs_cfg_desc);

USBD_CONFIGURATION_DEFINE(sample_hs_config,
			  attributes,
			  CONFIG_SAMPLE_WIFI_USBD_MAX_POWER, &hs_cfg_desc);

int wifi_usbd_init(void)
{
	int ret;

	ret = usbd_add_descriptor(&sample_usbd, &sample_lang);
	if (ret) {
		printk("Cannot add USB language descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_descriptor(&sample_usbd, &sample_mfr);
	if (ret) {
		printk("Cannot add USB manufacturer descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_descriptor(&sample_usbd, &sample_product);
	if (ret) {
		printk("Cannot add USB product descriptor (%d)", ret);
		return ret;
	}

	if (IS_ENABLED(USBD_SUPPORTS_HIGH_SPEED) &&
	    usbd_caps_speed(&sample_usbd) == USBD_SPEED_HS) {
		ret = usbd_add_configuration(&sample_usbd, USBD_SPEED_HS,
					     &sample_hs_config);
		if (ret) {
			printk("Cannot add USB HS configuration (%d)", ret);
			return ret;
		}

		ret = usbd_register_all_classes(&sample_usbd, USBD_SPEED_HS, 1, NULL);
		if (ret) {
			printk("Cannot register USB HS classes (%d)", ret);
			return ret;
		}
	}

	ret = usbd_add_configuration(&sample_usbd, USBD_SPEED_FS,
				     &sample_fs_config);
	if (ret) {
		printk("Cannot add USB FS configuration (%d)", ret);
		return ret;
	}

	ret = usbd_register_all_classes(&sample_usbd, USBD_SPEED_FS, 1, NULL);
	if (ret) {
		printk("Cannot register USB FS classes (%d)", ret);
		return ret;
	}

	ret = usbd_init(&sample_usbd);
	if (ret) {
		printk("Cannot init USB (%d)", ret);
		return ret;
	}

	return usbd_enable(&sample_usbd);
}
