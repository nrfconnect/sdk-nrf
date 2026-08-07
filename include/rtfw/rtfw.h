/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RTFW_RTFW_H_
#define RTFW_RTFW_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Command-attempt result event.
 *
 * The value field contains the signed result represented as uint32_t.
 */
#define RTFW_EVENT_COMMAND_PROCESSED 1U

/** Highest event type reserved for framework-generated events. */
#define RTFW_EVENT_TYPE_FRAMEWORK_MAX 0xffU
/** First event type available to source adapters. */
#define RTFW_EVENT_TYPE_USER_BASE     0x100U

#define RTFW_FAULT_EVENT_OVERFLOW BIT(0)
#define RTFW_FAULT_BACKEND_APPLY  BIT(1)

/**
 * @brief Fixed-size command exchanged with an RTFW backend.
 *
 * The framework treats @ref data as opaque. A backend owns command identifiers
 * and validates the payload before applying it.
 */
struct rtfw_command {
	uint32_t id;
	uint32_t data_len;
	uint8_t data[CONFIG_RTFW_COMMAND_DATA_SIZE];
};

/**
 * @brief Event produced by a backend's zero-latency fast path.
 *
 * Event types through @ref RTFW_EVENT_TYPE_FRAMEWORK_MAX are reserved by the
 * framework. Backend-defined event types must start at
 * @ref RTFW_EVENT_TYPE_USER_BASE.
 */
struct rtfw_event {
	uint32_t type;
	uint32_t value;
	uint32_t timestamp;
};

/**
 * @brief Coherent framework status snapshot.
 *
 * requested is the newest published command. attempted and apply_result
 * describe the command acknowledged by the fast path. applied is the most
 * recent command completed successfully. A failed attempt is acknowledged and
 * clears pending without changing applied.
 */
struct rtfw_status {
	struct rtfw_command requested;
	struct rtfw_command attempted;
	struct rtfw_command applied;
	int32_t apply_result;
	bool pending;
	uint32_t dropped_events;
	/** Zero when CONFIG_RTFW_QUEUE_USAGE_STATS is disabled. */
	uint32_t max_queue_depth;
	uint32_t faults;
};

/**
 * @brief Process a command in the zero-latency fast path.
 *
 * The framework calls this required callback at most once per
 * @ref rtfw_fastpath_run invocation, before the source fast-path handler.
 * Execution must be bounded and must not use kernel APIs, log, allocate, block,
 * or perform cache maintenance. Return zero only after applying the command
 * completely. On error, leave the previously applied state unchanged and
 * return a negative errno value.
 */
typedef int (*rtfw_command_handler_t)(const struct rtfw_command *command,
				      void *user_data);

/**
 * @brief Service a client-owned source in the zero-latency fast path.
 *
 * The framework calls this required callback on every
 * @ref rtfw_fastpath_run invocation, after processing any pending command.
 * Execution must be bounded and must not use kernel APIs, log, allocate, block,
 * or perform cache maintenance. The handler owns source-event acknowledgment
 * and may call @ref rtfw_event_push.
 */
typedef void (*rtfw_fastpath_handler_t)(void *user_data);

/**
 * @brief Pend the client-owned source interrupt.
 *
 * @ref rtfw_submit calls this required callback in Zephyr thread context after
 * publishing a command. It must make the source ISR run even when no hardware
 * event is pending. The callback must not process the command itself.
 */
typedef void (*rtfw_pend_source_irq_t)(void *user_data);

/**
 * @brief Consume an event outside the zero-latency fast path.
 *
 * The framework calls this optional callback from its dedicated delivery
 * workqueue. The event pointer is valid only for the duration of the call. The
 * callback may use kernel APIs but should remain bounded to avoid overflowing
 * the fixed-size event queue.
 */
typedef void (*rtfw_event_cb_t)(const struct rtfw_event *event, void *user_data);

/**
 * @brief RTFW client configuration.
 *
 * A zero-latency interrupt does not perform Zephyr's system power-management
 * resume sequence. The client must keep the source peripheral, shared RAM, and
 * required power domains accessible while the source IRQ is enabled. It must
 * mask that IRQ before entering an unsafe power state and restore the source
 * before unmasking it after resume.
 */
struct rtfw_config {
	rtfw_command_handler_t command_handler;
	rtfw_fastpath_handler_t fastpath_handler;
	rtfw_pend_source_irq_t pend_source_irq;
	rtfw_event_cb_t event_handler;
	void *fastpath_user_data;
	void *event_user_data;
};

/**
 * @brief Initialize the reusable framework.
 *
 * This must complete before the client enables its source interrupt.
 * RTFW is a singleton; a second successful initialization is rejected with
 * -EALREADY.
 *
 * @retval 0 The framework was initialized.
 * @retval -EINVAL config or a required callback is NULL.
 * @retval -EALREADY The singleton framework is already initialized.
 */
int rtfw_init(const struct rtfw_config *config);

/**
 * @brief Publish a latest-wins command and pend the backend source IRQ.
 *
 * The call is thread-safe and non-blocking with respect to the RT context.
 * It must not be called from interrupt context.
 *
 * A command stops being pending when the fast path acknowledges its processing,
 * regardless of the handler result. Consult attempted, applied, and
 * apply_result in @ref rtfw_status to distinguish success from failure.
 *
 * @retval 0 The command was published.
 * @retval -EINVAL The framework is not initialized, command is NULL, or its
 *                  payload is too large.
 * @retval -EPERM The function was called from interrupt context.
 */
int rtfw_submit(const struct rtfw_command *command);

/**
 * @brief Read a coherent requested/attempted/applied state snapshot.
 *
 * @retval 0 The snapshot was copied.
 * @retval -EINVAL The framework is not initialized or status is NULL.
 * @retval -EPERM The function was called from interrupt context.
 */
int rtfw_get_status(struct rtfw_status *status);

/**
 * @brief Enter the registered zero-latency fast path.
 *
 * A client calls this from every direct source ISR, including software-pended
 * control entries where no hardware event is asserted. The function applies
 * at most one published command and then invokes fastpath_handler().
 */
void rtfw_fastpath_run(void);

/**
 * @brief Push one event from the single producer fast path.
 *
 * The framework rings its internal doorbell when delivery changes from idle to
 * pending. Only the registered fast-path execution context may call this
 * function. Calls from threads, the delivery callback, or another interrupt
 * context violate the SPSC single-producer contract.
 *
 * @retval 0 The event was queued.
 * @retval -EINVAL event is NULL or uses a framework-reserved type.
 * @retval -EACCES The framework has not been initialized.
 * @retval -ENOSPC The fixed-size queue was full.
 */
int rtfw_event_push(const struct rtfw_event *event);

#ifdef __cplusplus
}
#endif

#endif /* RTFW_RTFW_H_ */
