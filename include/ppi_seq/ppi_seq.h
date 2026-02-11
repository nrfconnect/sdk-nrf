/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_PPI_SEQ_H__
#define NRF_PPI_SEQ_H__

/**
 * @defgroup ppi_seq PPI Sequencer
 * @{
 *
 * @brief Module for autonomously triggering periodic tasks.
 *
 */

#include <string.h>
#include <nrfx_timer.h>
#include <nrfx_grtc.h>
#include <helpers/nrfx_gppi.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration. */
struct ppi_seq_config;

/** @brief PPI sequencer structure. */
struct ppi_seq {
	/** A pointer to the configuration structure used during the initialization. */
	const struct ppi_seq_config *config;

	/** Timer instance which may be used for triggering tasks. */
	nrfx_timer_t timer;

	/** Pool of GPPI connection handles. */
	nrfx_gppi_handle_t ppi_pool[CONFIG_PPI_SEQ_MAX_PPI_HANDLES];

	/** Internally used variable. */
	atomic_t repeat;

	/** Current batch count. */
	uint16_t batch_cnt;

	/** GRTC channel used by the sequencer. */
	uint8_t grtc_chan;

	/** Number of GPPI connection handles. */
	uint8_t ppi_cnt;
};

/** @brief Callback called after completion of each cycle.
 *
 * @param ppi_seq Sequencer instance.
 * @param last True if this is the last callback after which the sequencer is stopped.
 */
typedef void (*ppi_seq_cb_t)(struct ppi_seq *ppi_seq, bool last);

/** @brief Type of notifier used for the requested number of sequences completion. */
enum ppi_seq_notifier_type {
	/** k_timer callback is called when requested number of sequences is completed. */
	PPI_SEQ_NOTIFIER_SYS_TIMER,

	/** nrfx_timer as counter counts events and triggers after reaching a certain number.*/
	PPI_SEQ_NOTIFIER_NRFX_TIMER,
};

/** @brief System timer notifier structure. Low power and less resources. */
struct ppi_seq_notifier_sys_timer {
	/** Timer. */
	struct k_timer timer;

	/** User configured length of the last operation in the sequence in microseconds. */
	uint16_t offset;

	/** For internal use. */
	uint64_t timestamp;

	/** For internal use. */
	uint32_t period;
};

/** @brief nrfx_timer notifier structure. Useful for short periods. */
struct ppi_seq_notifier_nrfx_timer {
	/** TIMER driver instance used for counting events. */
	nrfx_timer_t timer;

	/** Address of an EVENT register which should be used to count completion events. */
	uint32_t end_seq_event;

	/* Number of additional main operations in a cycle. For example if there
	 * are 2 SPI transfers in each cycle then it should be set to 1.
	 */
	uint16_t extra_main_ops;
};

/** @brief Notifier structure. */
struct ppi_seq_notifier {
	/** Notifier type. */
	enum ppi_seq_notifier_type type;
	union {
		/** Notifier using k_timer. */
		struct ppi_seq_notifier_sys_timer sys_timer;

		/** Notifier using TIMER peripheral in the counter mode. */
		struct ppi_seq_notifier_nrfx_timer nrfx_timer;
	};
};

/** @brief Operation descriptor. */
struct ppi_seq_extra_op {
	/** Address of a TASK register which should be triggered by the sequencer. */
	uint32_t task;

	/** Delay relative to the start of the sequence (in microseconds). */
	uint32_t offset;
};

/** @brief Sequencer configuration structure. */
struct ppi_seq_config {
	/** Notifier. */
	struct ppi_seq_notifier *notifier;

	/** TIMER peripheral register. Can be NULL if sequencer does not use TIMER. */
	NRF_TIMER_Type *timer_reg;

#ifdef RTC_PRESENT
	/** RTC peripheral register. Can be NULL if sequencer does not use RTC. */
	NRF_RTC_Type * rtc_reg;
#endif

	/** Callback called on completion of each cycle. */
	ppi_seq_cb_t callback;

	/** Address of a TASK register that should be triggered on the start of the sequence. */
	uint32_t task;

	/** Extra operations in the sequence.
	 * Extra operations requires TIMER or RTC. If the TIMER is provided then main interval is
	 * using GRTC interval features and extra tasks are triggered using the TIMER.
	 * If the RTC is provided then RTC is used for triggering all tasks in the sequence.
	 */
	const struct ppi_seq_extra_op *extra_ops;

	/** Number of extra operations in the sequence. */
	size_t extra_ops_count;

	bool skip_gppi;
};

/** @brief Initialize the sequencer.
 *
 * @param seq Sequencer instance.
 * @param config Sequencer configuration. Must be persistent as it is used by the sequencer.
 *
 * @retval 0 on successful initialization.
 * @retval negative on initialization failure.
 */
int ppi_seq_init(struct ppi_seq *seq, const struct ppi_seq_config *config);

/** @brief Uninitialize the sequencer.
 *
 * Function releases all resources.
 *
 * @param seq Sequencer instance.
 */
void ppi_seq_uninit(struct ppi_seq *seq);

/** @brief Start the sequencer.
 *
 * When started, sequence will be repeated requested number of timers and callback will be
 * called after @p batch_cnt periods. If sequence consist of a single operation
 * (for example, a SPI transfer) then there will be callback after @p batch_cnt transfers and
 * sequencer will stop after @p repeat callbacks. Sequence can be forced to stop by
 * @ref ppi_seq_stop.
 *
 * Sequence is started as fast as possible. In case of RTC it can be delayed by a single low
 * frequency tick (approx. 30 us).
 *
 * @param seq Sequencer instance.
 * @param period Distance (in microseconds) between triggering the main task.
 * @param batch_cnt Number of periods after which the callback is called.
 * @param repeat Number of callbacks after which the sequencer is stopped. UINT32_MAX for continuous
 * operation.
 */
int ppi_seq_start(struct ppi_seq *seq, size_t period, size_t batch_cnt, int repeat);

/** @brief Stop the sequencer.
 *
 * @param seq Sequencer instance.
 * @param immediate Sequence is stopped immediately if true. If false, sequence will be stopped
 * after the next callback.
 *
 * @retval 0 Operation is successful.
 * @retval -EALREADY Sequencer is already stopped.
 */
int ppi_seq_stop(struct ppi_seq *seq, bool immediate);

/** @brief RTC interrupt handler.
 *
 * RTC on some devices starts with a delay. If system timer is used as notifier it makes it
 * impossible to correctly calculate the time of batch completion. Compare event of the first
 * tick is used as the known starting point of system timer notifier. User need to register
 * this interrupt handler if RTC is used as the timing source.
 *
 * @param seq Sequencer instance.
 */
void ppi_seq_rtc_irq_handler(struct ppi_seq *seq);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NRF_PPI_SEQ_H__ */
