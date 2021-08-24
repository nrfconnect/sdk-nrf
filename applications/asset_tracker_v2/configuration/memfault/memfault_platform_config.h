#pragma once

/*
 * Platform overrides for the default configuration settings in the
 * memfault-firmware-sdk. Default configuration settings can be found in
 * "<NCS folder>/modules/lib/memfault-firmware-sdk/components/include/memfault/default_config.h"
 */

 /* Prepare captured metric data for upload to Memfault cloud every configured interval. */
#define MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS CONFIG_DEBUG_MODULE_MEMFAULT_HEARTBEAT_INTERVAL_SEC
#define MEMFAULT_DATA_EXPORT_CHUNK_MAX_LEN CONFIG_DEBUG_MODULE_MEMFAULT_CHUNK_SIZE_MAX
