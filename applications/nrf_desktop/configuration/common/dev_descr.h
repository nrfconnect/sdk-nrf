/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DEV_DESCR_H_
#define _DEV_DESCR_H_

/**
 * @file
 * @defgroup dev_descr Device description
 * @{
 * @brief Custom GATT Service containing device description.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 * The header file is included only by dev_descr module for BT_PERIPHERAL or
 * by ble_discovery module for BT_CENTRAL.
 */
const struct {} dev_descr_include_once;

#define DEV_DESCR_LLPM_SUPPORT_POS	0
#define DEV_DESCR_LLPM_SUPPORT_SIZE	1
#define DEV_DESCR_LEN			(DEV_DESCR_LLPM_SUPPORT_POS + \
					DEV_DESCR_LLPM_SUPPORT_SIZE)

static struct bt_uuid_128 custom_desc_uuid = BT_UUID_INIT_128(
	0x25, 0x1d, 0x09, 0x1c, 0x78, 0x2e, 0x46, 0x98,
	0xac, 0x13, 0x44, 0x75, 0x01, 0x00, 0x00, 0x00);

const static struct bt_uuid_128 custom_desc_chrc_uuid = BT_UUID_INIT_128(
	0x25, 0x1d, 0x09, 0x1c, 0x78, 0x2e, 0x46, 0x98,
	0xac, 0x13, 0x44, 0x75, 0x02, 0x00, 0x00, 0x00);

const static struct bt_uuid_128 hwid_chrc_uuid = BT_UUID_INIT_128(
	0x25, 0x1d, 0x09, 0x1c, 0x78, 0x2e, 0x46, 0x98,
	0xac, 0x13, 0x44, 0x75, 0x03, 0x00, 0x00, 0x00);

/**
 * @}
 */

#endif /*_DEV_DESCR_H_ */
