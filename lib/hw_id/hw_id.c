/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hw_id.h>
#include <zephyr/kernel.h>

/* includes for the different HW ID sources */
#if defined(CONFIG_HW_ID_LIBRARY_SOURCE_BLE_MAC)
#include <zephyr/bluetooth/bluetooth.h>
#endif /* defined(CONFIG_HW_ID_LIBRARY_SOURCE_BLE_MAC) */
#if defined(CONFIG_HW_ID_LIBRARY_SOURCE_DEVICE_ID)
#include <zephyr/drivers/hwinfo.h>
#endif /* defined(CONFIG_HW_ID_LIBRARY_SOURCE_DEVICE_ID) */
#if defined(CONFIG_HW_ID_LIBRARY_SOURCE_UUID)
#include <modem/modem_jwt.h>
#endif /* defined(CONFIG_HW_ID_LIBRARY_SOURCE_UUID) */
#if defined(CONFIG_HW_ID_LIBRARY_SOURCE_IMEI)
#include <nrf_modem_at.h>
#endif /* defined(CONFIG_HW_ID_LIBRARY_SOURCE_IMEI) */
#if defined(CONFIG_HW_ID_LIBRARY_SOURCE_NET_MAC)
#include <zephyr/net/net_if.h>
#endif /* defined(CONFIG_HW_ID_LIBRARY_SOURCE_NET_MAC) */

#define IMEI_LEN 15

#if defined(CONFIG_HW_ID_LIBRARY_SOURCE_BLE_MAC)
/* Ask BLE stack for default MAC address */

int hw_id_get(char *buf, size_t buf_len)
{
	if (buf == NULL || buf_len < HW_ID_LEN) {
		return -EINVAL;
	}

	struct bt_le_oob oob;
	int ret = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);

	if (ret) {
		return -EIO;
	}

	snprintk(buf, buf_len, "%02X%02X%02X%02X%02X%02X",
		oob.addr.a.val[5],
		oob.addr.a.val[4],
		oob.addr.a.val[3],
		oob.addr.a.val[2],
		oob.addr.a.val[1],
		oob.addr.a.val[0]);
	return 0;
}
#endif /* defined(CONFIG_HW_ID_LIBRARY_SOURCE_BLE_MAC) */

#if defined(CONFIG_HW_ID_LIBRARY_SOURCE_DEVICE_ID)
/* Directly read Device ID from registers */

int hw_id_get(char *buf, size_t buf_len)
{
	if (buf == NULL || buf_len < HW_ID_LEN) {
		return -EINVAL;
	}

	uint8_t device_id[8];
	ssize_t ret = hwinfo_get_device_id(device_id, ARRAY_SIZE(device_id));

	if (ret != ARRAY_SIZE(device_id)) {
		return -EIO;
	}

	snprintk(buf, buf_len, "%02X%02X%02X%02X%02X%02X%02X%02X",
		device_id[0],
		device_id[1],
		device_id[2],
		device_id[3],
		device_id[4],
		device_id[5],
		device_id[6],
		device_id[7]);
	return 0;
}
#endif /* defined(CONFIG_HW_ID_LIBRARY_SOURCE_DEVICE_ID) */

#if defined(CONFIG_HW_ID_LIBRARY_SOURCE_UUID)
/* Request a UUID from the modem */

int hw_id_get(char *buf, size_t buf_len)
{
	if (buf == NULL || buf_len < HW_ID_LEN) {
		return -EINVAL;
	}

	struct nrf_device_uuid dev = {0};

	int err = modem_jwt_get_uuids(&dev, NULL);

	if (err) {
		return -EIO;
	}

	snprintk(buf, buf_len, "%s", dev.str);
	return 0;
}
#endif /* defined(CONFIG_HW_ID_LIBRARY_SOURCE_UUID) */

#if defined(CONFIG_HW_ID_LIBRARY_SOURCE_IMEI)
/* Ask the modem for an IMEI */

int hw_id_get(char *buf, size_t buf_len)
{
	if (buf == NULL || buf_len < HW_ID_LEN) {
		return -EINVAL;
	}

	char imei_buf[IMEI_LEN + 6 + 1]; /* Add 6 for \r\nOK\r\n and 1 for \0 */

	/* Retrieve device IMEI from modem. */
	int err = nrf_modem_at_cmd(imei_buf, ARRAY_SIZE(imei_buf), "AT+CGSN");

	if (err) {
		return -EIO;
	}

	/* Set null character at the end of the device IMEI. */
	imei_buf[IMEI_LEN] = 0;

	snprintk(buf, buf_len, "%s", imei_buf);
	return 0;
}
#endif /* defined(CONFIG_HW_ID_LIBRARY_SOURCE_IMEI) */

#if defined(CONFIG_HW_ID_LIBRARY_SOURCE_NET_MAC)
/* Use MAC address from default network interface */

int hw_id_get(char *buf, size_t buf_len)
{
	if (buf == NULL || buf_len < HW_ID_LEN) {
		return -EINVAL;
	}

	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		return -EIO;
	}

	struct net_linkaddr *linkaddr = net_if_get_link_addr(iface);

	snprintk(buf, buf_len, "%02X%02X%02X%02X%02X%02X",
		linkaddr->addr[0],
		linkaddr->addr[1],
		linkaddr->addr[2],
		linkaddr->addr[3],
		linkaddr->addr[4],
		linkaddr->addr[5]);
	return 0;
}
#endif /* defined(CONFIG_HW_ID_LIBRARY_SOURCE_NET_MAC) */
