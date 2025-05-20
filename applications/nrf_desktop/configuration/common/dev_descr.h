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
 * The header file is included only by dev_descr module for nRF Desktop BLE
 * Peripheral or by ble_discovery for nRF Desktop BLE Central.
 */
const struct {} dev_descr_include_once;

#define DEV_DESCR_LLPM_SUPPORT_POS	0
#define DEV_DESCR_LLPM_SUPPORT_SIZE	1
#define DEV_DESCR_LEN			(DEV_DESCR_LLPM_SUPPORT_POS + \
					DEV_DESCR_LLPM_SUPPORT_SIZE)

static struct bt_uuid_128 custom_desc_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x00000001, 0x7544, 0x13ac, 0x9846, 0x2e781c091d25));

const static struct bt_uuid_128 custom_desc_chrc_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x00000002, 0x7544, 0x13ac, 0x9846, 0x2e781c091d25));

const static struct bt_uuid_128 hwid_chrc_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x00000003, 0x7544, 0x13ac, 0x9846, 0x2e781c091d25));

/**
 * @}
 */

#endif /*_DEV_DESCR_H_ */
