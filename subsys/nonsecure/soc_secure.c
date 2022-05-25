/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <soc_secure.h>
#include <errno.h>

#include <secure_services.h>

/* soc_secure_gpio_pin_mcu_select intentionally left unimplemented.
 * All uses currently coming from secure firmware.
 */

#if CONFIG_SPM_SERVICE_READ
int soc_secure_mem_read(void *dst, void *src, size_t len)
{
	return spm_request_read(dst, (uint32_t)src, len);
}
#endif
