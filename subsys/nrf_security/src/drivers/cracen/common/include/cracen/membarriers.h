/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrfx.h>

/** Compiler memory barrier.
 *
 * The compiler shall not reorder memory access memory across a call to cmb().
 */
static inline void cmb(void)
{
	/* The Data Memory Barrier (DMB) instruction ensures that
	 * outstanding memory transactions complete before subsequent
	 * memory transactions. It is expected to be defined by cmsis.h.
	 */
	__DMB();
}

/** CPU write memory barrier.
 *
 * All write instructions before this call must have been completed by the
 * CPU before the end of this function. No write instruction after this
 * function may be executed by the CPU before this call.
 * This also implies that the compiler may not reorder memory acceses
 * as with cmb().
 */
static inline void wmb(void)
{
#ifdef __aarch64__
	__asm__ volatile("dsb sy;" ::: "memory");
#elif defined(__amd64__)
	__asm__ volatile("sfence;" ::: "memory");
#else
	/* CUSTOMIZATION: implement CPU specific memory barrier if it has any.
	 * Use at least a compiler memory barrier.
	 */

	/* Cortex-M33 does not support wmb, but cmb/DMB is more
	 * restrictive and therefore safe to use.
	 */
	cmb();
#endif
}

/** CPU read memory barrier.
 *
 * All read instructions before this call must have been completed by the
 * CPU before the end of this function. No read instruction after this
 * function may be executed by the CPU before this call.
 * This also implies that the compiler may not reorder memory acceses
 * as with cmb().
 */
static inline void rmb(void)
{
#ifdef __aarch64__
	__asm__ volatile("dsb sy;" ::: "memory");
#elif defined(__amd64__)
	__asm__ volatile("lfence;" ::: "memory");
#else
	/* CUSTOMIZATION: implement CPU specific memory barrier if it has any.
	 * Use at least a compiler memory barrier.
	 */

	/* Cortex-M33 does not support rmb, but cmb/DMB is more
	 * restrictive and therefore safe to use.
	 */
	cmb();
#endif
}
