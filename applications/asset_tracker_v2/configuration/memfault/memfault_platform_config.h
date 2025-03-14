/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

/*
 * Platform overrides for the default configuration settings in the
 * memfault-firmware-sdk. Default configuration settings can be found in
 * "<NCS folder>/modules/lib/memfault-firmware-sdk/components/include/memfault/default_config.h"
 */

#if defined(CONFIG_DEBUG_MODULE)
#define MEMFAULT_DATA_EXPORT_CHUNK_MAX_LEN CONFIG_DEBUG_MODULE_MEMFAULT_CHUNK_SIZE_MAX
#endif /* defined(CONFIG_DEBUG_MODULE) */
