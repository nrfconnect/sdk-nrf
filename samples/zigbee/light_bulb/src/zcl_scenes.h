/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZCL_SCENES_H__
#define ZCL_SCENES_H__

#include <zboss_api.h>


#define ZCL_SCENES_ENDPOINT   10
#define ZCL_SCENES_TABLE_SIZE 3


/** Initialize ZCL scene table. */
void zcl_scenes_init(void);

/** Handle scene-related ZCL callbacks. */
zb_bool_t zcl_scenes_cb(zb_bufid_t bufid);

#endif /* ZCL_SCENES_H__ */
