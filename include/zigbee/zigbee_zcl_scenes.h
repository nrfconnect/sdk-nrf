/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZIGBEE_ZCL_SCENES_H__
#define ZIGBEE_ZCL_SCENES_H__

#include <zboss_api.h>

/** Initialize ZCL scene table. */
void zcl_scenes_init(void);

/** Handle scene-related ZCL callbacks. */
zb_bool_t zcl_scenes_cb(zb_bufid_t bufid);

#endif /* ZIGBEE_ZCL_SCENES_H__ */
