/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

/** @file
 *  @brief LED Button Service (LBS) sample
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

#define LBS_UUID_SERVICE 0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, \
			 0xDE, 0xEF, 0x12, 0x12, 0x23, 0x15, 0x00, 0x00

#define LBS_UUID_BUTTON_CHAR 0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, \
			     0xDE, 0xEF, 0x12, 0x12, 0x24, 0x15, 0x00, 0x00

#define LBS_UUID_LED_CHAR    0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, \
			     0xDE, 0xEF, 0x12, 0x12, 0x25, 0x15, 0x00, 0x00

/** @brief Callback type for when a LED state change is received */
typedef void (*led_cb_t)(const bool led_state);

/** @brief Callback type for when the button state is pulled */
typedef bool (*button_cb_t)(void);

/** @brief Callback struct used by the LBS service */
struct bt_lbs_cb {
	led_cb_t    led_cb;
	button_cb_t button_cb;
};

/** @brief Initialization
 *
 * @details This function will register a BLE with two characteristics, BUTTON
 *          and LED. Notification can be enabled for the BUTTON characteristics
 *          to let a connected BLE unit know when the button state changes.
 *          The LED characteristics can be written to change the state of the
 *          LED on the board.
 *
 * @param[in] callbacks Struct containing pointers to callback functions
 *			used by the service. This pointer can be NULL
 *			if no callback functions are defined.
 *
 *
 * @return Returns 0 if initialization was successful,
 *         otherwise negative value
 */
int lbs_init(struct bt_lbs_cb *callbacks);

/** @brief Send button state
 *
 * @details This function will send a binary state, typically the state of a
 *          button, to the connected BLE unit.
 *
 * @param[in] button_state The state of the button
 *
 * @return Returns 0 if the operation was successful,
 *         otherwise negative value.
 */
int lbs_send_button_state(bool button_state);

#ifdef __cplusplus
}
#endif
