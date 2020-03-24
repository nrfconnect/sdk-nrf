/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _CDC_CTRL_EVENT_H_
#define _CDC_CTRL_EVENT_H_

/**
 * @brief CDC Control Event
 * @defgroup cdc_ctrl_event CDC Control Event
 * @{
 */

#include <string.h>
#include <toolchain/common.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

enum cdc_ctrl_cmd {
	CDC_CTRL_ENABLE,
	CDC_CTRL_DISABLE
};

/** CDC control event. */
struct cdc_ctrl_event {
	struct event_header header;

	enum cdc_ctrl_cmd cmd;
};

EVENT_TYPE_DECLARE(cdc_ctrl_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CDC_CTRL_EVENT_H_ */
