/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Single point of truth for the shared-memory concurrency model of the
 * app-domain RT framework.
 *
 * The framework's execution model is fixed: the RT component is a zero-latency
 * interrupt (ZLI) running on the same application core as the Zephyr threads.
 * The control plane and data plane are therefore plain ISR <-> thread traffic
 * on one core. Because both contexts share the same data cache, that cache is
 * self-coherent and ordering barriers alone are sufficient, even on SoCs with
 * a data cache such as nRF54H20.
 *
 * Cache maintenance is required only if a *unidirectional* shared buffer is
 * additionally exposed to another bus master (a DMA engine or another core).
 * That case is opt-in (CONFIG_RT_FW_SHARED_DMA_VISIBLE) and is wired through the
 * rt_shared_dma_*() helpers below. It must NOT be used on a bidirectional
 * structure (e.g. the control block, where the ISR both reads commands and
 * writes status) because cache invalidation there is destructive.
 *
 * Keeping all barriers and cache operations here means the memory model lives
 * in exactly one place. The rest of the framework never calls __DMB() or the
 * cache API directly.
 */

#ifndef RT_SHARED_H_
#define RT_SHARED_H_

#include <stddef.h>
#include <cmsis_core.h>

#if defined(CONFIG_RT_FW_SHARED_DMA_VISIBLE)
#include <zephyr/cache.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Release barrier (producer). All prior writes to the shared region are made
 * visible before the subsequent "publish" store (a sequence number or ring
 * index). Pair with rt_shared_acquire() on the consumer.
 */
static inline void rt_shared_release(void)
{
	__DMB();
}

/*
 * Acquire barrier (consumer). The observation of a published sequence number /
 * ring index happens-before the subsequent reads of the shared payload.
 */
static inline void rt_shared_acquire(void)
{
	__DMB();
}

/*
 * Push a unidirectional payload this context just wrote out towards other
 * observers. No-op unless the region is DMA-/cross-master-visible. Call after
 * writing the payload and before the release + publish.
 *
 * @p addr must be cache-line aligned and @p len a multiple of the cache line
 * size when the opt-in is enabled (see CONFIG_RT_FW_SHARED_DMA_VISIBLE).
 */
static inline void rt_shared_dma_flush(void *addr, size_t len)
{
#if defined(CONFIG_RT_FW_SHARED_DMA_VISIBLE)
	(void)sys_cache_data_flush_range(addr, len);
#else
	(void)addr;
	(void)len;
#endif
}

/*
 * Drop any stale cache lines for a unidirectional payload another master may
 * have written, before this context reads it. No-op unless the region is
 * DMA-/cross-master-visible. Call after the acquire and before reading.
 *
 * Invalidation is destructive, so only use this on a buffer that this context
 * does not itself write (see the file header).
 */
static inline void rt_shared_dma_invalidate(void *addr, size_t len)
{
#if defined(CONFIG_RT_FW_SHARED_DMA_VISIBLE)
	(void)sys_cache_data_invd_range(addr, len);
#else
	(void)addr;
	(void)len;
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* RT_SHARED_H_ */
