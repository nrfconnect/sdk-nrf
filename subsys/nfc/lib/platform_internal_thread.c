/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <nfc_platform.h>
#include "platform_internal.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(nfc_platform, CONFIG_NFC_PLATFORM_LOG_LEVEL);

#ifdef CONFIG_NFC_LOW_LATENCY_IRQ
#define SWI_NAME(number)	SWI_NAME2(number)
#ifdef CONFIG_SOC_SERIES_NRF53X
#define SWI_NAME2(number)	EGU ## number ## _IRQn
#else
#define SWI_NAME2(number)	SWI ## number ## _IRQn
#endif

#define NFC_SWI_IRQN		SWI_NAME(CONFIG_NFC_SWI_NUMBER)
#endif /* CONFIG_NFC_LOW_LATENCY_IRQ */

#define NFC_LIB_CTX_SIZE CONFIG_NFC_LIB_CTX_MAX_SIZE

struct nfc_item_header {
	uint16_t ctx_size;
	uint16_t data_size;
};

#ifdef CONFIG_NFC_OWN_THREAD
/* By using a counting semaphore, NFC events interrupt can go faster than the processing thread.
 * All events must be processed.
 */
static K_SEM_DEFINE(cb_sem, 0, K_SEM_MAX_LIMIT);
#endif /* CONFIG_NFC_OWN_THREAD */

static bool buf_full;
static nfc_lib_cb_resolve_t nfc_cb_resolve;
RING_BUF_DECLARE(nfc_cb_ring, CONFIG_NFC_RING_SIZE);

static void work_resubmit(struct k_work *work, struct ring_buf *buf)
{
	if (!IS_ENABLED(CONFIG_NFC_OWN_THREAD)) {
		if (!ring_buf_is_empty(buf)) {
			int ret = k_work_submit(work);
			/* In this case, work can be scheduled either from IRQ or to the queue
			 * running work items during the run.
			 */
			__ASSERT_NO_MSG(((ret == 0) || (ret == 2)));
			ARG_UNUSED(ret);
		}
	}
}

static int ring_buf_get_data(struct ring_buf *buf, uint8_t **data, uint32_t size, bool *is_alloc)
{
	uint32_t tmp;
	int err;

	*is_alloc = false;

	/* Try to access data without copying.
	 * If this fails (for example, the data wraps in ring buffer), try to allocate a buffer
	 * and copy data into it.
	 */
	tmp = ring_buf_get_claim(buf, data, size);
	if (tmp < size) {
		err = ring_buf_get_finish(&nfc_cb_ring, 0);
		if (err) {
			LOG_DBG("Tried to finish a read with 0 bytes, err %i.", err);
			return err;
		}

		*data = k_malloc(size);
		if (data == NULL) {
			LOG_DBG("Could not allocate %d bytes.", size);
			return -ENOMEM;
		}

		tmp = ring_buf_get(buf, *data, size);
		if (tmp < size) {
			LOG_DBG("Tried to read %d bytes, but read %d.", size, tmp);
			k_free(*data);
			return -EBADMSG;
		}

		*is_alloc = true;
	}

	return 0;
}

static void cb_work(struct k_work *work)
{
	struct nfc_item_header header;
	uint32_t ctx[(ROUND_UP(NFC_LIB_CTX_SIZE, 4) / 4)];
	uint8_t *data;
	uint32_t size;
	bool is_alloc = false;
	int err = 0;

	/* Data from ring buffer is always processed in pairs - context + data in order
	 * to avoid double copying
	 */
	size = ring_buf_get(&nfc_cb_ring, (uint8_t *)&header, sizeof(header));
	if (size != sizeof(header)) {
		LOG_ERR("Tried to read header: %d bytes, read %d.", sizeof(header), size);
		goto error;
	}

	__ASSERT(header.ctx_size <= NFC_LIB_CTX_SIZE, "NFC context is bigger than expected.");

	size = ring_buf_get(&nfc_cb_ring, (uint8_t *)&ctx, header.ctx_size);
	if (size != header.ctx_size) {
		LOG_ERR("Tried to read ctx: %d bytes, read %d.", sizeof(header.ctx_size), size);
		goto error;
	}

	if (header.data_size == 0) {
		size = ring_buf_get(&nfc_cb_ring, (uint8_t *)&data, sizeof(data));
		if (size != sizeof(data)) {
			LOG_ERR("Tried to read data pointer: %d bytes, read %d.", sizeof(data),
											      size);
			goto error;
		}
	} else {
		err = ring_buf_get_data(&nfc_cb_ring, &data, header.data_size, &is_alloc);
		if (err) {
			LOG_ERR("Reading nfc data failed, err %i.", err);
			goto error;
		}
	}

	__ASSERT(nfc_cb_resolve != NULL, "nfc_cb_resolve is not set");
	nfc_cb_resolve(ctx, data);

	if (header.data_size == 0) {
		work_resubmit(work, &nfc_cb_ring);
		return;
	}

	if (is_alloc) {
		k_free(data);
	} else {
		err = ring_buf_get_finish(&nfc_cb_ring, header.data_size);
		if (err) {
			LOG_ERR("Tried to finish a read with %u bytes, err %i.",
									header.data_size, err);
		}
	}

	work_resubmit(work, &nfc_cb_ring);

	return;

error:
	LOG_ERR("Reading nfc ring buffer failed, resetting ring buffer.");
	ring_buf_reset(&nfc_cb_ring);
}

#ifdef CONFIG_NFC_OWN_THREAD
static void nfc_cb_wrapper(void)
{
	while (1) {
		k_sem_take(&cb_sem, K_FOREVER);
		cb_work(NULL);
	}
}

K_THREAD_DEFINE(nfc_cb_thread, CONFIG_NFC_THREAD_STACK_SIZE, nfc_cb_wrapper,
		 NULL, NULL, NULL, CONFIG_NFC_THREAD_PRIORITY, 0, 0);
#else
static K_WORK_DEFINE(nfc_cb_work, cb_work);
#endif /* CONFIG_NFC_OWN_THREAD */

static void schedule_callback(void)
{
	__ASSERT(buf_full == false, "NFC ring buffer overflow");
#ifdef CONFIG_NFC_OWN_THREAD
	k_sem_give(&cb_sem);
#else
	k_work_submit(&nfc_cb_work);
#endif /* CONFIG_NFC_OWN_THREAD */
}

#ifdef CONFIG_NFC_LOW_LATENCY_IRQ
ISR_DIRECT_DECLARE(nfc_swi_handler)
{
	schedule_callback();

	ISR_DIRECT_PM();

	return 1;
}
#endif /* CONFIG_NFC_LOW_LATENCY_IRQ */

int nfc_platform_internal_init(nfc_lib_cb_resolve_t cb_rslv)
{
	if (!cb_rslv) {
		return -EINVAL;
	}

#ifdef CONFIG_NFC_LOW_LATENCY_IRQ
	IRQ_DIRECT_CONNECT(NFC_SWI_IRQN, CONFIG_NFCT_IRQ_PRIORITY,
			   nfc_swi_handler, 0);
	irq_enable(NFC_SWI_IRQN);
#endif /* CONFIG_NFC_LOW_LATENCY_IRQ */

	nfc_cb_resolve = cb_rslv;

	return 0;
}

void nfc_platform_cb_request(const void *ctx,
			     size_t ctx_len,
			     const uint8_t *data,
			     size_t data_len,
			     bool copy_data)
{
	struct nfc_item_header header;
	uint32_t size;
	uint32_t exp_size;

	header.ctx_size = ctx_len;
	header.data_size = copy_data ? data_len : 0;

	size = ring_buf_put(&nfc_cb_ring, (uint8_t *)&header, sizeof(header));
	if (size != sizeof(header)) {
		buf_full = true;
		goto end;
	}

	size = ring_buf_put(&nfc_cb_ring, (uint8_t *)ctx, header.ctx_size);
	if (size != header.ctx_size) {
		buf_full = true;
		goto end;
	}

	if (copy_data && (data_len > 0)) {
		size = ring_buf_put(&nfc_cb_ring, data, header.data_size);
		exp_size = header.data_size;
	} else {
		size = ring_buf_put(&nfc_cb_ring, (uint8_t *)&data, sizeof(data));
		exp_size = sizeof(data);
	}

	if (size != exp_size) {
		buf_full = true;
	}

end:
#ifdef CONFIG_NFC_LOW_LATENCY_IRQ
	NVIC_SetPendingIRQ(NFC_SWI_IRQN);
#else
	schedule_callback();
#endif /* CONFIG_NFC_LOW_LATENCY_IRQ */
}
