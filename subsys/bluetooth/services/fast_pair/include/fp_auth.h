/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_AUTH_H_
#define _FP_AUTH_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup fp_auth Fast Pair authentication
 * @brief Internal API for Fast Pair Bluetooth authentication implementation
 *
 * Fast Pair authentication subsystem handles Bluetooth authentication described by Fast Pair
 * Procedure. Fast Pair Provider uses passkey confirmation relying on data written over GATT to
 * achieve MITM protection. The module must be initialized with @ref bt_fast_pair_enable before
 * using API functions.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Start Fast Pair Bluetooth authentication procedure.
 *
 * The Fast Pair authentication subsystem takes over Zephyr Bluetooth authentication callbacks to
 * perform Bluetooth pairing according to the Fast Pair procedure. The procedure can be used only
 * if connected peer is Fast Pair Seeker. Otherwise regular pairing must be used.
 *
 * @param[in] conn		Pointer to Bluetooth connection (determines Fast Pair Seeker).
 * @param[in] send_pairing_req	Boolean informing if Provider should send pairing request.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_auth_start(struct bt_conn *conn, bool send_pairing_req);

/** Compare passkeys for a given Fast Pair Seeker.
 *
 * Performs passkey comparision and sends Bluetooth pairing response. Fills the value of Bluetooth
 * authentication passkey to forward it to the Fast Pair Seeker over GATT notification. Success is
 * returned no matter if passkey match. According to the Fast Pair specification, the Bluetooth
 * authentication passkey must be sent to the Fast Pair Seeker no matter if the passkey comparison
 * is successful on Provider's side.
 *
 * @param[in] conn		Pointer to Bluetooth connection (determines Fast Pair Seeker).
 * @param[in] gatt_passkey	Value providing passkey received over GATT.
 * @param[out] bt_aut_passkey	Pointer to value used to store Bluetooth authentication passkey.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_auth_cmp_passkey(struct bt_conn *conn, uint32_t gatt_passkey, uint32_t *bt_auth_passkey);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_AUTH_H_ */
