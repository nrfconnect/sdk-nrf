/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(_ASMLANGUAGE)

#define SOC_ON_EXIT_CPU_IDLE \
	bl etb_trace_on_idle_exit;

#endif /* _ASMLANGUAGE */
