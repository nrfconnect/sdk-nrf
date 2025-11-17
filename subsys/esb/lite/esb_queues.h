/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ESB_QUEUES_H_
#define ESB_QUEUES_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/sys/barrier.h>


/** Single item for a queue based on linked list: esb_fifo_list or esb_lifo_list.
 */
struct esb_list_item {
	/* Pointer to the next item or NULL if queue ends here. */
	struct esb_list_item *next;
};

/** FIFO based on cyclic linked list.
 *  The linked list is in form of a cycle, where the last item's next pointer
 *  points back to the first item, so knowing last item allows access to both
 *  ends of the queue.
 */
struct esb_fifo_list {
	/* Pointer to the last item or NULL if queue is empty. */
	struct esb_list_item *last;
};

/** LIFO based on linked list ended with NULL.
 */
struct esb_lifo_list {
	/* Pointer to the first item or NULL if queue is empty. */
	struct esb_list_item *first;
};

/** Special kind of FIFO using ring buffer.
 *  It allows usage of all array elements, not as in traditional FIFO where
 *  one element must remain unused to distinguish between full and empty states.
 *  When any of the index is increased, it is wrapped using modulo of 2 * array size.
 *  This way when FIFO is full, the difference is never zero, but equal to array size.
 *  Disadvantage is that when accessing elements using those indexes, we need to
 *  do extra modulo array size operation.
 *
 *  Actual data is stored outside of this structure allowing usage of different
 *  data types.
 *
 *  It allows lock-free usage as long as there is a single producer and a single consumer.
 *
 *  Using following macros and two stage operations (prepare + commit) reduces need of unnecessary
 *  data copying.
 */
struct esb_fifo_ring {
	volatile uint16_t begin;
	volatile uint16_t end;
};


/** Initialize esb_fifo_ring. */
#define ESB_FIFO_INIT(ring)                                                                        \
	do {                                                                                       \
		(ring)->begin = 0;                                                                 \
		(ring)->end = 0;                                                                   \
	} while (0)

/** Prepare for pushing to the FIFO. It returns pointer to array item where you can put new data.
 *  If FIFO is full, it returns NULL.
 */
#define ESB_FIFO_PREPARE_PUSH(ring, array)                                                         \
	(ESB_FIFO_FULL(ring, array) ? NULL :                                                       \
		(&((array)[(ring)->end % ARRAY_SIZE(array)])))

/** Push newly prepared data to the FIFO.
 */
#define ESB_FIFO_PUSH(ring, array)                                                                 \
	do {                                                                                       \
		barrier_dmem_fence_full();                                                         \
		(ring)->end = ((ring)->end + 1) % (2 * ARRAY_SIZE(array));                         \
	} while (0)

/** Prepare for popping data from the FIFO. It returns pointer to array item which is about to
 *  be popped. You can read data from that location before actually popping it.
 *  If FIFO is empty, it returns NULL.
 */
#define ESB_FIFO_PREPARE_POP(ring, array)                                                          \
	(ESB_FIFO_COUNT(ring, array) == 0 ? NULL :                                                 \
		(&((array)[(ring)->begin % ARRAY_SIZE(array)])))

/** Pop from the FIFO.
 */
#define ESB_FIFO_POP(ring, array)                                                                  \
	do {                                                                                       \
		(ring)->begin = ((ring)->begin + 1) % (2 * ARRAY_SIZE(array));                     \
		barrier_dmem_fence_full();                                                         \
	} while (0)

/** Clear content of the FIFO. If it is used outside consumer, ensure proper locking.
 */
#define ESB_FIFO_CLEAR(ring)                                                                       \
	do {                                                                                       \
		uint16_t end = (ring)->end;                                                        \
		(ring)->begin = end;                                                               \
	} while (0)

/** Get number of items in the FIFO.
 */
#define ESB_FIFO_COUNT(ring, array) (((ring)->end - (ring)->begin) % (2 * ARRAY_SIZE(array)))

/** Check if the FIFO is full.
 */
#define ESB_FIFO_FULL(ring, array) (ESB_FIFO_COUNT(ring, array) == ARRAY_SIZE(array))

/** Clear list-based LIFO queue. */
static inline void esb_lifo_clear(struct esb_lifo_list *queue)
{
	queue->first = NULL;
}

/** Push to list-based LIFO queue. */
static inline void esb_lifo_push(struct esb_lifo_list *queue, struct esb_list_item *item)
{
	item->next = queue->first;
	queue->first = item;
}

/** Pop item from list-based LIFO queue. Returns NULL if empty. */
static inline void *esb_lifo_pop(struct esb_lifo_list *queue)
{
	struct esb_list_item *result;

	result = queue->first;
	if (result != NULL) {
		queue->first = result->next;
	}

	return result;
}

/** Check if list-based LIFO queue is empty. */
static inline bool esb_lifo_is_empty(struct esb_lifo_list *queue)
{
	return (queue->first == NULL);
}

/** Clear list-based FIFO queue. */
void esb_fifo_clear(struct esb_fifo_list *queue);

/** Push item to list-based FIFO queue. */
void esb_fifo_push(struct esb_fifo_list *queue, struct esb_list_item *item);

/** Pop item from list-based FIFO queue. Returns NULL if empty. */
void *esb_fifo_pop(struct esb_fifo_list *queue);


#endif /* ESB_QUEUES_H_ */
