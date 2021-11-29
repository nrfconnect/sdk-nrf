/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Dummy file included to enable execution on native posix platform */
#ifndef NRFX_H__

#ifndef CONFIG_BOARD_NATIVE_POSIX
#error "This file should only be included for native posix boards"
#endif

/* Stub of a function to enable execution on native posix */
static inline bool nrfx_is_in_ram(void const *p_object)
{
	return true;
}

#endif /* NRFX_H__ */
