/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Internal HCI interface
 */

#include <stdint.h>
#include <stdbool.h>

#ifndef HCI_INTERNAL_H__
#define HCI_INTERNAL_H__


/** @brief Send an HCI command packet to the SoftDevice Controller.
 *
 * @param[in] cmd_in  HCI Command packet. The first byte in the buffer should correspond to
 *                    OpCode, as specified by the Bluetooth Core Specification.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int hci_internal_cmd_put(uint8_t *cmd_in);

/** @brief Retrieve an HCI event packet from the SoftDevice Controller.
 *
 * This API is non-blocking.
 *
 * @note The application should ensure that the size of the provided buffer is at least
 *       @ref HCI_EVENT_PACKET_MAX_SIZE bytes.
 *
 * @param[in,out] evt_out Buffer where the HCI event will be stored.
 *                        If an event is retrieved, the first byte corresponds to Event Code,
 *                        as specified by the Bluetooth Core Specification.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int hci_internal_evt_get(uint8_t *evt_out);

#endif
