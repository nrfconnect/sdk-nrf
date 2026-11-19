/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Bluetooth Mesh node reset using a button.
 * @defgroup bt_mesh_node_reset_button Bluetooth Mesh node reset using a button
 * @{
 */

#ifndef BT_MESH_NODE_RESET_BUTTON_H__
#define BT_MESH_NODE_RESET_BUTTON_H__

#include <dk_buttons_and_leds.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BT_MESH_NODE_RESET_BUTTON)

/* Select which button resets the node, from CONFIG_BT_MESH_NODE_RESET_BUTTON_INDEX.
 * The index counts from 0 and is the bit position in the dk_buttons_and_leds
 * button masks.
 */
#if CONFIG_BT_MESH_NODE_RESET_BUTTON_INDEX == 0
#define BT_MESH_NODE_RESET_BUTTON_MSK DK_BTN1_MSK
#elif CONFIG_BT_MESH_NODE_RESET_BUTTON_INDEX == 1
#define BT_MESH_NODE_RESET_BUTTON_MSK DK_BTN2_MSK
#elif CONFIG_BT_MESH_NODE_RESET_BUTTON_INDEX == 2
#define BT_MESH_NODE_RESET_BUTTON_MSK DK_BTN3_MSK
#elif CONFIG_BT_MESH_NODE_RESET_BUTTON_INDEX == 3
#define BT_MESH_NODE_RESET_BUTTON_MSK DK_BTN4_MSK
#else
#error "Node reset button: unsupported CONFIG_BT_MESH_NODE_RESET_BUTTON_INDEX"
#endif

#endif /* CONFIG_BT_MESH_NODE_RESET_BUTTON */

/** @brief Connect the node reset action to a button.
 *
 * Registers a button handler. The button and the press method can differ
 * between development kits.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_mesh_node_reset_button_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_NODE_RESET_BUTTON_H__ */

/** @} */
