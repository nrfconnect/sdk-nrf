/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Shared-memory communication structures between the Zephyr application
 * domain and the RT component.
 *
 * Ownership / concurrency model:
 *  - The control block command fields are published with a seqlock: Zephyr
 *    makes command_seq odd (write in progress), writes the command snapshot
 *    (enable + period_ticks), then makes command_seq even again (one greater
 *    than before). The RT ISR only consumes a command when command_seq is even
 *    and differs from ack_seq, reads the snapshot, and re-checks command_seq.
 *    If it changed it retries on the next tick. This guarantees the ISR never
 *    applies a half-written command, which a plain "write fields then bump a
 *    counter" scheme cannot, because the ZLI can preempt the writer at any
 *    point and irq_lock() does not mask it.
 *  - The RT ISR is the single writer of the applied/ack/telemetry fields.
 *    applied_enable/applied_period_ticks reflect what is actually programmed in
 *    hardware, so Zephyr can distinguish a requested command (command_seq !=
 *    ack_seq) from the state in effect.
 *  - The optional event queue is a lock-free single-producer (RT ISR) /
 *    single-consumer (Zephyr) ring.
 *
 * All barriers and (optional) cache maintenance go through rt_shared.h, which
 * is the single place that knows the memory model. Because the RT component is
 * a ZLI on the same core as the Zephyr threads, the shared data cache is
 * self-coherent and ordering barriers alone are sufficient even on cached SoCs
 * (e.g. nRF54H20). Cache maintenance is only engaged for a unidirectional
 * buffer that is additionally DMA-/cross-master-visible
 * (CONFIG_RT_FW_SHARED_DMA_VISIBLE).
 */

#ifndef RT_COMM_H_
#define RT_COMM_H_

#include <stdint.h>
#include <zephyr/toolchain.h>

/*
 * Alignment of DMA-/cross-master-visible payload. Cache-line sized when the
 * opt-in is enabled (so a per-entry invalidate cannot clobber a neighbour),
 * otherwise just word aligned.
 */
#if defined(CONFIG_RT_FW_SHARED_DMA_VISIBLE)
#if defined(CONFIG_DCACHE_LINE_SIZE) && (CONFIG_DCACHE_LINE_SIZE > 0)
#define RT_SHARED_ALIGN CONFIG_DCACHE_LINE_SIZE
#else
#define RT_SHARED_ALIGN 32
#endif
#else
#define RT_SHARED_ALIGN sizeof(uint32_t)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* The RT-owned GPIOs (toggle output and optional debug pin) are selected from
 * the /zephyr,user devicetree node. See rt_platform.h.
 */

/* Control plane: Zephyr -> RT command/config, RT -> Zephyr status/telemetry. */
struct rt_control_block {
	/* Seqlock-published command (written by Zephyr, read by the RT ISR). */
	volatile uint32_t command_seq;   /* even = stable, odd = write in progress */
	volatile uint32_t enable;        /* requested: 0 = idle, 1 = toggle GPIO */
	volatile uint32_t period_ticks;  /* requested TIMER compare value (CC0) */

	/* Applied/acknowledged state (written by the RT ISR, read by Zephyr). */
	volatile uint32_t ack_seq;              /* command_seq of last applied command */
	volatile uint32_t applied_enable;       /* enable currently in effect */
	volatile uint32_t applied_period_ticks; /* CC0 currently programmed in HW */

	/* RT-owned telemetry/status. */
	volatile uint32_t tick_count;     /* ticks while enabled */
	volatile uint32_t dropped_events; /* data-plane ring overflows (saturating) */
	volatile uint32_t last_error;     /* RT-owned sticky error/status code */
};

#if defined(CONFIG_RT_FW_DATA_PLANE)

/* Must be a power of two. */
#define RT_EVENT_QUEUE_SIZE 16U

enum rt_event_type {
	RT_EVENT_TICK = 1,           /* periodic marker from the RT path */
	RT_EVENT_CONFIG_APPLIED = 2, /* RT applied a new control-plane config */
};

struct rt_event {
	uint32_t type;
	uint32_t value;
	uint32_t timestamp; /* RT tick_count when the event was produced */
} __aligned(RT_SHARED_ALIGN);

/* SPSC ring: RT ISR is the producer (head), Zephyr is the consumer (tail).
 * The entries are aligned so that, under CONFIG_RT_FW_SHARED_DMA_VISIBLE, each
 * one owns whole cache line(s) and the head/tail indices live on a separate
 * line from the payload.
 */
struct rt_event_queue {
	volatile uint32_t head;
	volatile uint32_t tail;
	struct rt_event entries[RT_EVENT_QUEUE_SIZE] __aligned(RT_SHARED_ALIGN);
};

#endif /* CONFIG_RT_FW_DATA_PLANE */

/*
 * The shared instances are defined in rt_fastpath.c, i.e. they are owned by
 * the RT component. The Zephyr side only references them through this header.
 */
extern struct rt_control_block rt_ctrl;

#if defined(CONFIG_RT_FW_DATA_PLANE)
extern struct rt_event_queue rt_evq;
#endif

#ifdef __cplusplus
}
#endif

#endif /* RT_COMM_H_ */
