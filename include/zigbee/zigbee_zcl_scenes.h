/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZIGBEE_ZCL_SCENES_H__
#define ZIGBEE_ZCL_SCENES_H__

#include <zboss_api.h>

/** @file zigbee_zcl_scenes.h
 *
 * @defgroup zigbee_scenes Zigbee ZCL scenes helper library.
 * @{
 * @brief  Library for handling ZCL scenes events for common clusters.
 *
 * @details Provides ZCL callback event processing function that handles ZCL scenes commands.
 */

/**
 * @brief Initialize the Zigbee ZCL scenes helper library.
 *
 * @details This function initializes the ZCL scene table as well as registers the
 * settings observer for restoring scene table entries.
 */
void zcl_scenes_init(void);

/**
 * @brief Function for passing ZCL callback events to the scenes helper library logic.
 *
 * @param[in] bufid  Reference to the Zigbee stack buffer used to pass event.
 *
 * @retval ZB_TRUE   if the event was handled by the library
 * @retval ZB_FALSE  if the event was not handled by the library
 */
zb_bool_t zcl_scenes_cb(zb_bufid_t bufid);

#endif /* ZIGBEE_ZCL_SCENES_H__ */

/**@} */
