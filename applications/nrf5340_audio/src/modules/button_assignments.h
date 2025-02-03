/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Button assignments
 *
 * Button mappings are listed here.
 *
 */

#ifndef _BUTTON_ASSIGNMENTS_H_
#define _BUTTON_ASSIGNMENTS_H_

#include <zephyr/drivers/gpio.h>

#define BUTTON_NOT_ASSIGNED 0xFF

/** @brief List of buttons assignments.
 *
 *  @note If the switch node existed the button gpio pin is
 *        assigned to the selected button constant. Any none
 *        existent switch node is set as unassigned.
 *
 */
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
#define BUTTON_VOLUME_DOWN DT_GPIO_PIN(DT_ALIAS(sw0), gpios)
#else
#define BUTTON_VOLUME_DOWN BUTTON_NOT_ASSIGNED

#endif

#if DT_NODE_EXISTS(DT_ALIAS(sw1))
#define BUTTON_VOLUME_UP DT_GPIO_PIN(DT_ALIAS(sw1), gpios)
#else
#define BUTTON_VOLUME_UP BUTTON_NOT_ASSIGNED

#endif

#if DT_NODE_EXISTS(DT_ALIAS(sw2))
#define BUTTON_PLAY_PAUSE DT_GPIO_PIN(DT_ALIAS(sw2), gpios)
#else
#define BUTTON_PLAY_PAUSE BUTTON_NOT_ASSIGNED
#endif

/** @note The audio DK has an additional switch (sw4) compared
 *        to the vanilla DK. Here if the sw4 node exists it is
 *        used to configure button 5. Also if a node for sw3
 *        exists it is used to configure button 4. However if
 *        button sw4 does not exists then button sw3 is used
 *        to configure either button 4 or 5 (here it is 5)
 *        depending on the required action of this button.
 */
#if DT_NODE_EXISTS(DT_ALIAS(sw4))
#define BUTTON_5 DT_GPIO_PIN(DT_ALIAS(sw4), gpios)
#if DT_NODE_EXISTS(DT_ALIAS(sw3))
#define BUTTON_4 DT_GPIO_PIN(DT_ALIAS(sw3), gpios)
#else
#define BUTTON_4 BUTTON_NOT_ASSIGNED
#endif
#elif DT_NODE_EXISTS(DT_ALIAS(sw3))
#define BUTTON_4 BUTTON_NOT_ASSIGNED
#define BUTTON_5 DT_GPIO_PIN(DT_ALIAS(sw3), gpios)
#else
#define BUTTON_4 BUTTON_NOT_ASSIGNED
#define BUTTON_5 BUTTON_NOT_ASSIGNED
#endif

#endif /* _BUTTON_ASSIGNMENTS_H_ */
