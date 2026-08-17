/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief IPC bus layer for the nRF71 Wi-Fi driver.
 *
 * Combines Zephyr ICMsg service binding, host TX/RX IPC protocol, and the
 * HAL-facing IPC bus API. Design: single IPC endpoint (ipc0) for both TX and RX.
 * - Host sends (addr, size, ack_addr) for TX.
 * - RPU writes completion to ack_addr directly, no IPC ACK.
 * - UMAC sends (event addr, size, ring) for RX; Host processes then frees
 *   via tail write.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <common/log_cfg.h>
#include <common/mem_mgmt.h>

#include <common/hal_structs_common.h>
#include <common/ipc_bus.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

/* ---- Zephyr ICMsg service binding ---- */

static K_SEM_DEFINE(wifi_ipc_bind_sem, 0, 1);

static void wifi_ipc_signal_bound(void)
{
	k_sem_give(&wifi_ipc_bind_sem);
}

static int wifi_ipc_wait_bound(wifi_ipc_t *context)
{
	if (context->busy_q.ipc_ready) {
		return 0;
	}

	if (k_sem_take(&wifi_ipc_bind_sem,
		       K_MSEC(CONFIG_NRF71_IPC_BIND_TIMEOUT_MS)) != 0 &&
	    !context->busy_q.ipc_ready) {
		LOG_ERR("IPC endpoint not bound after %d ms (Wi-Fi core up? "
			"CONFIG_SOC_NRF71_WIFI_BOOT=y for TF-M builds)",
			CONFIG_NRF71_IPC_BIND_TIMEOUT_MS);
		return -ETIMEDOUT;
	}

	return 0;
}

static void wifi_ipc_ep_bound(void *priv)
{
	wifi_ipc_t *context = (wifi_ipc_t *)priv;

	context->busy_q.ipc_ready = true;
	wifi_ipc_signal_bound();
}

/* For single-endpoint TX+RX bind: set both contexts ready when endpoint is bound */
static wifi_ipc_t *wifi_ipc_rx_ctx_shared;

static void wifi_ipc_ep_bound_tx_rx(void *priv)
{
	wifi_ipc_t *tx_ctx = (wifi_ipc_t *)priv;

	tx_ctx->busy_q.ipc_ready = true;
	if (wifi_ipc_rx_ctx_shared != NULL) {
		wifi_ipc_rx_ctx_shared->busy_q.ipc_ready = true;
	}
	wifi_ipc_signal_bound();
}

static void wifi_ipc_recv_callback(const void *data, size_t len, void *priv)
{
	wifi_ipc_t *context = (wifi_ipc_t *)priv;

	if (context->busy_q.recv_cb != NULL) {
		context->busy_q.recv_cb(data, len, context->busy_q.priv);
	}

	if (context->send_ack) {
		while (!ipc_service_send(&context->busy_q.ipc_ep, data, len)) {
			/* Retry until success */
		}
	}
}

static void wifi_ipc_busyq_init(wifi_ipc_busyq_t *busyq, const ipc_device_wrapper_t *ipc_inst,
				wifi_ipc_rx_cb_t rx_cb, void *priv)
{
	busyq->ipc_inst = ipc_inst;
	busyq->ipc_ep_cfg.cb.bound = wifi_ipc_ep_bound;
	busyq->recv_cb = rx_cb;
	/* Never clear a bind that already completed: the ICMsg endpoint stays
	 * bound for the lifetime of the Wi-Fi core, and its bound callback only
	 * fires once. Clearing this on a re-bind would latch the endpoint as
	 * not-ready forever and wedge every subsequent send.
	 */
	if (!busyq->ipc_bound) {
		busyq->ipc_ready = false;
	}
	busyq->priv = priv;
	busyq->ipc_ep_cfg.cb.received = wifi_ipc_recv_callback;
}

static wifi_ipc_status_t wifi_ipc_busyq_register(wifi_ipc_t *context)
{
	int ret;
	const struct device *ipc_instance = GET_IPC_INSTANCE(context->busy_q.ipc_inst);

	if (context->busy_q.ipc_bound) {
		/* Already bound, only the RX consumer was re-armed. This holds while the
		 * Wi-Fi core stays powered; interface down/up never crosses it.
		 *
		 * TODO(WZN-10457): a power cycle reboots the core and resets the peer
		 * ICMsg state, so the bind goes stale. Once power management can power
		 * the core down, clear ipc_bound (and ipc_ready) on that leg so the next
		 * bring-up re-opens the instance and re-runs the handshake here.
		 */
		LOG_DBG("IPC busy queue already registered");
		return WIFI_IPC_STATUS_OK;
	}

	k_sem_reset(&wifi_ipc_bind_sem);

	ret = ipc_service_open_instance(ipc_instance);
	if (ret < 0) {
		return WIFI_IPC_STATUS_INIT_ERR;
	}

	context->busy_q.ipc_ep_cfg.name = "ep";
	context->busy_q.ipc_ep_cfg.priv = context;

	ret = ipc_service_register_endpoint(ipc_instance, &context->busy_q.ipc_ep,
					    &context->busy_q.ipc_ep_cfg);
	if (ret < 0 && ret != -EALREADY) {
		return WIFI_IPC_STATUS_INIT_ERR;
	}

	if (wifi_ipc_wait_bound(context) != 0) {
		return WIFI_IPC_STATUS_INIT_ERR;
	}

	context->busy_q.ipc_bound = true;

	LOG_INF("IPC busy queue registered");

	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_bind_ipc_service(wifi_ipc_t *context,
					    const ipc_device_wrapper_t *ipc_inst,
					    wifi_ipc_rx_cb_t rx_cb,
					    void *priv)
{
	wifi_ipc_busyq_init(&context->busy_q, ipc_inst, rx_cb, priv);

	return wifi_ipc_busyq_register(context);
}

wifi_ipc_status_t wifi_ipc_bind_ipc_service_tx_rx(wifi_ipc_t *tx_context,
						  wifi_ipc_t *rx_context,
						  const ipc_device_wrapper_t *ipc_inst,
						  wifi_ipc_rx_cb_t rx_cb,
						  void *priv)
{
	wifi_ipc_status_t ret;

	wifi_ipc_busyq_init(&rx_context->busy_q, ipc_inst, NULL, priv);
	wifi_ipc_rx_ctx_shared = rx_context;
	wifi_ipc_busyq_init(&tx_context->busy_q, ipc_inst, rx_cb, priv);
	tx_context->busy_q.ipc_ep_cfg.cb.bound = wifi_ipc_ep_bound_tx_rx;

	ret = wifi_ipc_busyq_register(tx_context);
	wifi_ipc_rx_ctx_shared = NULL;

	if (ret == WIFI_IPC_STATUS_OK) {
		/* Only the TX context owns the endpoint registration, but both
		 * share its bound state.
		 */
		rx_context->busy_q.ipc_bound = true;
	}

	return ret;
}

static wifi_ipc_status_t wifi_ipc_busyq_send(wifi_ipc_t *context, const void *data, size_t len)
{
	wifi_ipc_busyq_t *busyq = &context->busy_q;

	if (!busyq->ipc_ready) {
		LOG_DBG("IPC service is not ready");
		return WIFI_IPC_STATUS_BUSYQ_NOTREADY;
	}

	LOG_DBG("IPC send: len %d", len);

	int ret = ipc_service_send(&busyq->ipc_ep, (const void *)data, len);

	if (ret == -ENOMEM) {
		LOG_ERR("IPC send: ENOMEM");
		return WIFI_IPC_STATUS_BUSYQ_FULL;
	} else if (ret < 0) {
		LOG_ERR("IPC send: Critical IPC failure: %d", ret);
		return WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR;
	}

	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_host_tx_init(wifi_ipc_t *context, uint32_t addr_freeq)
{
	context->send_ack = false;

	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_host_rx_init(wifi_ipc_t *context, uint32_t addr_freeq)
{
	context->send_ack = false;

	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_host_tx_send(wifi_ipc_t *context,
					const void *data,
					size_t len,
					uint32_t *ack_addr)
{
	wifi_ipc_buf_desc_t msg_info = { 0 };

	msg_info.addr = (uint32_t)data;
	msg_info.size = len;
	msg_info.ack_addr = ack_addr;

	return wifi_ipc_busyq_send(context, &msg_info, sizeof(msg_info));
}

/* ---- Host TX/RX IPC protocol ---- */

#define NUM_INSTANCES 1
#define NUM_ENDPOINTS 1

struct device *ipc_instances[NUM_INSTANCES];
struct ipc_ept ept[NUM_ENDPOINTS];
struct ipc_ept_cfg ept_cfg[NUM_ENDPOINTS];

/* Single endpoint: TX (Host->UMAC) and RX (UMAC->Host) on same channel */
static wifi_ipc_t wifi_host_tx;
static wifi_ipc_t wifi_host_rx;

static int (*callback_func)(void *data);

#define IPC_TX_ACK_SLOTS 64

static K_MUTEX_DEFINE(host_tx_ack_lock);
static uint32_t host_tx_ack_slots[IPC_TX_ACK_SLOTS];
static uintptr_t host_tx_pending_bufs[IPC_TX_ACK_SLOTS];

static void host_tx_reclaim_completed(void)
{
	int i;

	k_mutex_lock(&host_tx_ack_lock, K_FOREVER);

	for (i = 0; i < IPC_TX_ACK_SLOTS; i++) {
		uint32_t completed_addr;

		completed_addr = host_tx_ack_slots[i];

		if ((completed_addr == 0U) || (host_tx_pending_bufs[i] == 0U)) {
			continue;
		}

		if (completed_addr != (uint32_t)host_tx_pending_bufs[i]) {
			LOG_WRN("Unexpected TX completion addr 0x%x in slot %d",
				completed_addr, i);
			continue;
		}

		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL,
				  (void *)host_tx_pending_bufs[i]);
		host_tx_pending_bufs[i] = 0U;
		host_tx_ack_slots[i] = 0U;
	}

	k_mutex_unlock(&host_tx_ack_lock);
}

static uint32_t *host_tx_ack_slot_alloc(const void *data)
{
	int i;

	k_mutex_lock(&host_tx_ack_lock, K_FOREVER);

	for (i = 0; i < IPC_TX_ACK_SLOTS; i++) {
		if (host_tx_pending_bufs[i] != 0U) {
			continue;
		}

		host_tx_ack_slots[i] = 0U;
		host_tx_pending_bufs[i] = (uintptr_t)data;
		k_mutex_unlock(&host_tx_ack_lock);

		return &host_tx_ack_slots[i];
	}

	k_mutex_unlock(&host_tx_ack_lock);

	return NULL;
}

static void host_tx_ack_slot_free(uint32_t *ack_addr)
{
	int i;

	if (ack_addr == NULL) {
		return;
	}

	k_mutex_lock(&host_tx_ack_lock, K_FOREVER);

	for (i = 0; i < IPC_TX_ACK_SLOTS; i++) {
		if (&host_tx_ack_slots[i] != ack_addr) {
			continue;
		}

		host_tx_ack_slots[i] = 0U;
		host_tx_pending_bufs[i] = 0U;
		break;
	}

	k_mutex_unlock(&host_tx_ack_lock);
}

/* RX path: receive event (addr, size, ring); after handler, free by writing tail to GDRAM */
static void host_rx_recv(const void *data, size_t len, void *priv)
{
	struct nrf_wifi_ipc_dev_ctx *dev_ctx = priv;
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx;
	const wifi_ipc_buf_desc_t *desc = data;
	wifi_ipc_buf_desc_t msg_info = *desc;

	LOG_DBG("Host RX IPC received");

	/* The endpoint stays bound across device teardown, so events can arrive
	 * with no consumer attached. Drop them, but always release the ring slot
	 * or the UMAC event ring fills up and wedges the next bring-up.
	 */
	if ((callback_func == NULL) || (dev_ctx == NULL) || (dev_ctx->hal_dev_ctx == NULL)) {
		LOG_DBG("Host RX IPC event dropped, no consumer");
		wifi_ipc_host_rx_free_event(&msg_info);
		return;
	}

	hal_dev_ctx = dev_ctx->hal_dev_ctx;
	hal_dev_ctx->ipc_msg = (void *)msg_info.addr;
	callback_func(dev_ctx);
	wifi_ipc_host_rx_free_event(&msg_info);
	LOG_DBG("Host RX IPC callback completed");
}

void wifi_ipc_host_rx_free_event(const wifi_ipc_buf_desc_t *event_info)
{
	volatile uint32_t *tail = (volatile uint32_t *)event_info->ring.tail_addr;
	uint32_t new_tail;

	if (event_info->ring.padded) {
		/* Ring was padded; tail advances past the event to ring base. */
		new_tail = event_info->size;
	} else {
		new_tail = (*tail + event_info->size) % event_info->ring.size;
	}
	*tail = new_tail;
}

int ipc_init(void)
{
	static bool slots_initialized;

	wifi_ipc_host_tx_init(&wifi_host_tx, 0);
	wifi_ipc_host_rx_init(&wifi_host_rx, 0);

	/* The ack slots track buffers the UMAC may still be reading. Clearing
	 * them on a re-init would leak every in-flight buffer, so initialize
	 * them only once.
	 */
	if (!slots_initialized) {
		for (int i = 0; i < IPC_TX_ACK_SLOTS; i++) {
			host_tx_ack_slots[i] = 0U;
			host_tx_pending_bufs[i] = 0U;
		}
		slots_initialized = true;
	}

	LOG_DBG("IPC host single endpoint (ipc0) TX+RX initialized");
	return 0;
}

int ipc_deinit(void)
{
	/* Reclaim whatever the UMAC has already acknowledged. Buffers still in
	 * flight stay owned by the ack slots and are reclaimed on a later send.
	 */
	host_tx_reclaim_completed();

	/* TODO(WZN-10457): this runs on interface teardown, where the core stays
	 * powered and the IPC endpoint stays bound. When power management gains a
	 * real core power-down, tear the endpoint down here (clear the bind latch
	 * so the next power-up re-runs the handshake) rather than leaving it armed
	 * against a core that has rebooted.
	 */
	return 0;
}

int ipc_recv(ipc_ctx_t ctx, void *data, int len)
{
	return 0;
}

int ipc_send(ipc_ctx_t ctx, const void *data, int len)
{
	int ret = 0;
	wifi_ipc_status_t status;

	LOG_DBG("IPC send: inst %d, len %d", ctx.inst, len);
	switch (ctx.inst) {
	case IPC_INSTANCE_CMD_CTRL:
	case IPC_INSTANCE_CMD_TX: {
		uint32_t *ack_addr;

		host_tx_reclaim_completed();
		ack_addr = host_tx_ack_slot_alloc(data);
		if (ack_addr == NULL) {
			LOG_ERR("No free TX ack slots");
			return -1;
		}

		/* TX path: send (addr, size, ack_addr) on IPC0 */
		{
			int64_t deadline = k_uptime_get() + CONFIG_NRF71_IPC_SEND_TIMEOUT_MS;

			do {
				status = wifi_ipc_host_tx_send(&wifi_host_tx,
							       data,
							       (size_t)len,
							       ack_addr);
				if (status != WIFI_IPC_STATUS_BUSYQ_NOTREADY) {
					break;
				}

				if (k_uptime_get() >= deadline) {
					LOG_ERR("IPC host TX timed out after %d ms",
						CONFIG_NRF71_IPC_SEND_TIMEOUT_MS);
					host_tx_ack_slot_free(ack_addr);
					return -ETIMEDOUT;
				}

				k_usleep(CONFIG_NRF71_IPC_SEND_RETRY_INTERVAL_US);
			} while (true);
		}

		if (status != WIFI_IPC_STATUS_OK) {
			host_tx_ack_slot_free(ack_addr);

			if (status == WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR) {
				LOG_ERR("Critical error during IPC host TX transfer");
				return -1;
			}
		}

		ret = (status == WIFI_IPC_STATUS_OK) ? 0 : -1;
		break;
	}
	case IPC_INSTANCE_RX:
		break;
	case IPC_INSTANCE_EVT:
		break;
	default:
		break;
	}

	LOG_DBG("IPC send completed: %d", ret);
	return ret;
}

int ipc_register_rx_cb(int (*rx_handler)(void *priv), void *data)
{
	int ret;

	callback_func = rx_handler;

	ret = wifi_ipc_bind_ipc_service_tx_rx(&wifi_host_tx, &wifi_host_rx,
					      DEVICE_DT_GET(DT_NODELABEL(ipc0)),
					      host_rx_recv, data);
	if (ret != WIFI_IPC_STATUS_OK) {
		LOG_ERR("Failed to bind IPC host TX+RX (ipc0): %d", ret);
		callback_func = NULL;
		return -1;
	}

	return 0;
}

void ipc_unregister_rx_cb(void)
{
	/* Detach the consumer only. The endpoint stays bound for the lifetime of
	 * the Wi-Fi core; events arriving from now on are dropped in
	 * host_rx_recv() with their ring slot released.
	 */
	callback_func = NULL;
}

/* ---- HAL-facing IPC bus API ---- */

static int nrf_wifi_ipc_irq_handler(void *data)
{
	struct nrf_wifi_ipc_dev_ctx *dev_ctx = data;

	return dev_ctx->ipc_priv->intr_callbk_fn(dev_ctx->hal_dev_ctx);
}

struct nrf_wifi_ipc_dev_ctx *nrf_wifi_ipc_dev_add(struct nrf_wifi_ipc_priv *ipc_priv,
						  void *hal_dev_ctx)
{
	struct nrf_wifi_ipc_dev_ctx *ipc_dev_ctx;
	int ret;

	ipc_dev_ctx = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*ipc_dev_ctx));
	if (!ipc_dev_ctx) {
		LOG_ERR("%s: Unable to allocate ipc_dev_ctx", __func__);
		return NULL;
	}

	ipc_dev_ctx->ipc_priv = ipc_priv;
	ipc_dev_ctx->hal_dev_ctx = hal_dev_ctx;

	ret = ipc_init();
	if (ret) {
		LOG_ERR("%s: ipc_init failed", __func__);
		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, ipc_dev_ctx);
		return NULL;
	}

	return ipc_dev_ctx;
}

void nrf_wifi_ipc_dev_rem(struct nrf_wifi_ipc_dev_ctx *ipc_dev_ctx)
{
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, ipc_dev_ctx);
}

enum nrf_wifi_status nrf_wifi_ipc_dev_init(struct nrf_wifi_ipc_dev_ctx *ipc_dev_ctx)
{
	int ret;

	ret = ipc_register_rx_cb(&nrf_wifi_ipc_irq_handler, ipc_dev_ctx);
	if (ret) {
		LOG_ERR("%s: ipc_register_rx_cb failed", __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	return NRF_WIFI_STATUS_SUCCESS;
}

void nrf_wifi_ipc_dev_deinit(struct nrf_wifi_ipc_dev_ctx *ipc_dev_ctx)
{
	ARG_UNUSED(ipc_dev_ctx);

	/* Detach the event consumer before the HAL context is torn down. The IPC
	 * endpoint itself stays bound: it is the control plane and its lifetime
	 * follows the Wi-Fi core, not the interface.
	 */
	ipc_unregister_rx_cb();
	ipc_deinit();
}

struct nrf_wifi_ipc_priv *nrf_wifi_ipc_init(
	enum nrf_wifi_status (*intr_callbk_fn)(void *hal_dev_ctx))
{
	struct nrf_wifi_ipc_priv *ipc_priv;

	ipc_priv = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*ipc_priv));
	if (!ipc_priv) {
		LOG_ERR("%s: Unable to allocate memory for ipc_priv", __func__);
		return NULL;
	}

	ipc_priv->intr_callbk_fn = intr_callbk_fn;

	return ipc_priv;
}

void nrf_wifi_ipc_deinit(struct nrf_wifi_ipc_priv *ipc_priv)
{
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, ipc_priv);
}

enum nrf_wifi_status nrf_wifi_ipc_send_msg(struct nrf_wifi_ipc_dev_ctx *ipc_dev_ctx,
					   unsigned int msg_type,
					   void *msg,
					   unsigned int len)
{
	ipc_ctx_t ctx;
	int ret;

	ARG_UNUSED(ipc_dev_ctx);

	switch (msg_type) {
	case NRF_WIFI_IPC_MSG_CMD_CTRL:
		ctx.inst = IPC_INSTANCE_CMD_CTRL;
		ctx.ept = IPC_EPT_UMAC;
		break;
	case NRF_WIFI_IPC_MSG_CMD_DATA_TX:
		ctx.inst = IPC_INSTANCE_CMD_TX;
		ctx.ept = IPC_EPT_UMAC;
		break;
	case NRF_WIFI_IPC_MSG_CMD_DATA_RX:
		ctx.inst = IPC_INSTANCE_RX;
		ctx.ept = IPC_EPT_LMAC;
		break;
	default:
		LOG_ERR("%s: Invalid msg_type (%d)", __func__, msg_type);
		return NRF_WIFI_STATUS_FAIL;
	}

	ret = ipc_send(ctx, msg, len);
	if (ret < 0) {
		return NRF_WIFI_STATUS_FAIL;
	}

	return NRF_WIFI_STATUS_SUCCESS;
}
