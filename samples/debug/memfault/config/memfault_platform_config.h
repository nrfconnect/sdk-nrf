/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

/*
 * Platform overrides for the default configuration settings in the
 * memfault-firmware-sdk. Default configuration settings can be found in
 * "<NCS folder>/modules/lib/memfault-firmware-sdk/components/include/memfault/default_config.h"
 */

/* Uncomment the definition below to override the default setting for
 * heartbeat interval. This will prepare the captured metric data for upload
 * to Memfault cloud at the specified interval.
 */
/*
 * #define MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS 1800
 */
