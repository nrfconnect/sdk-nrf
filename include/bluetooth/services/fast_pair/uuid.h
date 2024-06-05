/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_FAST_PAIR_UUID_H_
#define BT_FAST_PAIR_UUID_H_

#include <zephyr/bluetooth/uuid.h>

/**
 * @defgroup bt_fast_pair_uuid Fast Pair UUID API
 * @brief Fast Pair UUID API
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Fast Pair Service UUID value. */
#define BT_FAST_PAIR_UUID_FPS_VAL (0xFE2C)
/** Fast Pair Service UUID. */
#define BT_FAST_PAIR_UUID_FPS BT_UUID_DECLARE_16(BT_FAST_PAIR_UUID_FPS_VAL)

/** Fast Pair Model ID Characteristic UUID value. */
#define BT_FAST_PAIR_UUID_MODEL_ID_VAL \
	BT_UUID_128_ENCODE(0xFE2C1233, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA)
/** Fast Pair Model ID Characteristic UUID. */
#define BT_FAST_PAIR_UUID_MODEL_ID \
	BT_UUID_DECLARE_128(BT_FAST_PAIR_UUID_MODEL_ID_VAL)

/** Fast Pair Key-based Pairing Characteristic UUID value. */
#define BT_FAST_PAIR_UUID_KEY_BASED_PAIRING_VAL \
	BT_UUID_128_ENCODE(0xFE2C1234, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA)
/** Fast Pair Key-based Pairing Characteristic UUID. */
#define BT_FAST_PAIR_UUID_KEY_BASED_PAIRING \
	BT_UUID_DECLARE_128(BT_FAST_PAIR_UUID_KEY_BASED_PAIRING_VAL)

/** Fast Pair Passkey Characteristic UUID value. */
#define BT_FAST_PAIR_UUID_PASSKEY_VAL \
	BT_UUID_128_ENCODE(0xFE2C1235, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA)
/** Fast Pair Passkey Characteristic UUID. */
#define BT_FAST_PAIR_UUID_PASSKEY \
	BT_UUID_DECLARE_128(BT_FAST_PAIR_UUID_PASSKEY_VAL)

/** Fast Pair Account Key Characteristic UUID value. */
#define BT_FAST_PAIR_UUID_ACCOUNT_KEY_VAL \
	BT_UUID_128_ENCODE(0xFE2C1236, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA)
/** Fast Pair Account Key Characteristic UUID. */
#define BT_FAST_PAIR_UUID_ACCOUNT_KEY \
	BT_UUID_DECLARE_128(BT_FAST_PAIR_UUID_ACCOUNT_KEY_VAL)

/** Fast Pair Additional Data Characteristic UUID value. */
#define BT_FAST_PAIR_UUID_ADDITIONAL_DATA_VAL \
	BT_UUID_128_ENCODE(0xFE2C1237, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA)
/** Fast Pair Additional Data Characteristic UUID. */
#define BT_FAST_PAIR_UUID_ADDITIONAL_DATA \
	BT_UUID_DECLARE_128(BT_FAST_PAIR_UUID_ADDITIONAL_DATA_VAL)

/** Fast Pair Beacon Actions Characteristic UUID value (used in the FMDN extension). */
#define BT_FAST_PAIR_UUID_BEACON_ACTIONS_VAL \
	BT_UUID_128_ENCODE(0xFE2C1238, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA)
/** Fast Pair Beacon Actions Characteristic UUID (used in the FMDN extension). */
#define BT_FAST_PAIR_UUID_BEACON_ACTIONS \
	BT_UUID_DECLARE_128(BT_FAST_PAIR_UUID_BEACON_ACTIONS_VAL)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_FAST_PAIR_UUID_H_ */
