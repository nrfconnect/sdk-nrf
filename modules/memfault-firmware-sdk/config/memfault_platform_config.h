/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Platform overrides for the default configuration settings in the
 * memfault-firmware-sdk. Default configuration settings can be found in
 * "<NCS folder>/modules/lib/memfault-firmware-sdk/components/include/memfault/default_config.h"
 */

#ifdef CONFIG_MEMFAULT_CDR_ENABLE
#define MEMFAULT_CDR_ENABLE 1
#endif

#ifdef CONFIG_MEMFAULT_NCS_ETB_CAPTURE
#define MEMFAULT_PLATFORM_FAULT_HANDLER_CUSTOM 1
#endif
