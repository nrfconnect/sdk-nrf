/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PEER_H_
#define PEER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include <stdlib.h>
#include <zephyr/types.h>
#include <bluetooth/addr.h>
#include <dm.h>

/** @brief Testing if the peer is supported.
 *
 *  @param peer Bluetooth LE Device Address.
 *
 *  @retval /a true peer is supported.
 *          /a false peer is not supported.
 */
bool peer_supported_test(const bt_addr_le_t *peer);

/** @brief Add a new supported peer.
 *
 *  @param peer Bluetooth LE Device Address.
 *
 *  @retval 0 if the operation was successful, otherwise a (negative) error code.
 */
int peer_supported_add(const bt_addr_le_t *peer);

/** @brief Prepare a access address.
 *
 *  @retval 0 if the operation was successful, otherwise a (negative) error code.
 */
int peer_access_address_prepare(void);

/** @brief Get the current access address.
 *
 *  @retval Access address value.
 */
uint32_t peer_access_address_get(void);

/** @brief Initialize the peer management module.
 *
 *  @param None
 *
 *  @retval 0 if the operation was successful, otherwise a (negative) error code.
 */
int peer_init(void);

/** @brief Peer measurement update.
 *
 *  @param result Measurement structure.
 */
void peer_update(struct dm_result *result);


#ifdef __cplusplus
}
#endif


#endif /* PEER_H_ */
