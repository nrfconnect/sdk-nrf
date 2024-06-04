/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include <nrfx.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	/* Size of BLE address derived HW ID needs to be aligned with HWINFO nRF driver. */
	uint8_t ble_addr_hwid[8];

	sys_put_le32(NRF_FICR->BLE.ADDR[0], &ble_addr_hwid[0]);
	sys_put_le16((uint16_t)(NRF_FICR->BLE.ADDR[1] & UINT16_MAX), &ble_addr_hwid[4]);
	sys_put_be16(CONFIG_DESKTOP_HWINFO_BLE_ADDRESS_FICR_POSTFIX, &ble_addr_hwid[6]);

	if (length > sizeof(ble_addr_hwid)) {
		length = sizeof(ble_addr_hwid);
	}

	memcpy(buffer, ble_addr_hwid, length);

	return length;
}
