/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   MPSL coexistence driver for the nRF71 Coexistence Controller (COEXC).
 *
 *   COEXC arbitrates radio access between Wi-Fi and the short-range radio. The short-range
 *   radio is an RX and a TX client of COEXC, each with a request and a release IPCT channel.
 *   The client sends REQUEST or RELEASE, and COEXC answers with GRANT or REVOKE on the same
 *   channels. Arbitration uses the client priorities in CCCONF, which are set from the ranges
 *   provided by the nRF71 coexistence driver.
 */

#include <mpsl_cx_abstract_interface.h>

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <hal/nrf_ipct.h>
#include <nrfx.h>

#include <drivers/nrf71_sr_coex/nrf71_sr_coex_sr.h>

#if DT_NODE_EXISTS(DT_NODELABEL(nrf_radio_coex))
#define CX_NODE DT_NODELABEL(nrf_radio_coex)
#else
#error No enabled coex nodes registered in DTS.
#endif

#if !(DT_NODE_HAS_COMPAT(CX_NODE, nordic_nrf71_coex))
#error Selected coex node is not compatible with nordic,nrf71-coex.
#endif

#define COEXC_IPCT_NODE DT_PHANDLE(CX_NODE, ipct)
#define COEXC_IPCT	((NRF_IPCT_Type *)DT_REG_ADDR(COEXC_IPCT_NODE))

#define COEXC_IPCT_INT_GROUP 0
#define COEXC_IPCT_IRQN	     DT_IRQN(COEXC_IPCT_NODE)
#define COEXC_IPCT_IRQ_PRIO  DT_IRQ(COEXC_IPCT_NODE, priority)

#define RX_REQUEST_IPCT_CH DT_PROP_BY_IDX(CX_NODE, rx_channels, 0)
#define RX_RELEASE_IPCT_CH DT_PROP_BY_IDX(CX_NODE, rx_channels, 1)
#define TX_REQUEST_IPCT_CH DT_PROP_BY_IDX(CX_NODE, tx_channels, 0)
#define TX_RELEASE_IPCT_CH DT_PROP_BY_IDX(CX_NODE, tx_channels, 1)

#define IPCT_NUM_CHANNELS DT_PROP(COEXC_IPCT_NODE, channels)

BUILD_ASSERT(DT_PROP_LEN(CX_NODE, rx_channels) == 2,
	     "rx-channels must be <request-channel release-channel>");
BUILD_ASSERT(DT_PROP_LEN(CX_NODE, tx_channels) == 2,
	     "tx-channels must be <request-channel release-channel>");

BUILD_ASSERT(RX_REQUEST_IPCT_CH < IPCT_NUM_CHANNELS && RX_RELEASE_IPCT_CH < IPCT_NUM_CHANNELS &&
		     TX_REQUEST_IPCT_CH < IPCT_NUM_CHANNELS &&
		     TX_RELEASE_IPCT_CH < IPCT_NUM_CHANNELS,
	     "COEXC IPCT channel index out of range for the referenced IPCT");

BUILD_ASSERT(RX_REQUEST_IPCT_CH != RX_RELEASE_IPCT_CH && TX_REQUEST_IPCT_CH != TX_RELEASE_IPCT_CH &&
		     RX_REQUEST_IPCT_CH != TX_REQUEST_IPCT_CH &&
		     RX_REQUEST_IPCT_CH != TX_RELEASE_IPCT_CH &&
		     RX_RELEASE_IPCT_CH != TX_REQUEST_IPCT_CH &&
		     RX_RELEASE_IPCT_CH != TX_RELEASE_IPCT_CH,
	     "COEXC IPCT channels must be distinct");

#define RX_IPCT_CH_MASK	   (BIT(RX_REQUEST_IPCT_CH) | BIT(RX_RELEASE_IPCT_CH))
#define TX_IPCT_CH_MASK	   (BIT(TX_REQUEST_IPCT_CH) | BIT(TX_RELEASE_IPCT_CH))
#define COEXC_IPCT_CH_MASK (RX_IPCT_CH_MASK | TX_IPCT_CH_MASK)

#define REQUEST_LEAD_TIME 0U

#define SR_RX_CLIENT   DT_PROP(CX_NODE, rx_client)
#define SR_TX_CLIENT   DT_PROP(CX_NODE, tx_client)
#define CCCONF_SR_MODE 0U

#define SHARED_LNA_REG  (*(volatile uint32_t *)(DT_REG_ADDR(DT_NODELABEL(radio)) + 0x740UL))
#define SHARED_LNA_MASK BIT(15)

BUILD_ASSERT(SR_RX_CLIENT != SR_TX_CLIENT, "COEXC RX and TX clients must be distinct");
BUILD_ASSERT(SR_RX_CLIENT < COEXC_CCCONF_MaxCount && SR_TX_CLIENT < COEXC_CCCONF_MaxCount,
	     "COEXC client index out of range");

enum {
	PRIO_NORMAL,
	PRIO_CRITICAL,
	PRIO_KIND_COUNT,
};

enum {
	SR_RX,
	SR_TX,
	SR_DIR_COUNT,
};

union sr_prio_table {
	atomic_val_t atomic;
	uint8_t prio[PRIO_KIND_COUNT][SR_DIR_COUNT];
};

BUILD_ASSERT(sizeof(union sr_prio_table) == sizeof(atomic_val_t));

static mpsl_cx_cb_t mpsl_cx_cb;
static atomic_t requested_clients;
static atomic_t coex_enabled;
static atomic_t coex_disabled_on_request;
static atomic_t sr_prio;
static atomic_t sr_prio_valid;

static inline void ipct_send(uint8_t channel)
{
	nrf_ipct_task_trigger(COEXC_IPCT, nrf_ipct_send_task_get(channel));
}

static inline void ipct_recv_clear(uint8_t channel)
{
	nrf_ipct_event_clear(COEXC_IPCT, nrf_ipct_receive_event_get(channel));
}

static inline bool ipct_recv_check(uint8_t channel)
{
	return nrf_ipct_event_check(COEXC_IPCT, nrf_ipct_receive_event_get(channel));
}

static uint32_t ipct_recv_events_get(uint32_t channels)
{
	uint32_t events = 0;

	for (uint8_t ch = 0; ch < IPCT_NUM_CHANNELS; ch++) {
		if ((channels & BIT(ch)) && ipct_recv_check(ch)) {
			events |= BIT(ch);
		}
	}

	return events;
}

static void clients_release(mpsl_cx_op_map_t ops)
{
	if (ops & MPSL_CX_OP_RX) {
		ipct_send(RX_RELEASE_IPCT_CH);
	}

	if (ops & MPSL_CX_OP_TX) {
		ipct_send(TX_RELEASE_IPCT_CH);
	}
}

static bool prio_resolve(const struct nrf71_sr_coex_pti_range *range, uint8_t level, uint8_t *p_out)
{
	if (level == NRF71_SR_COEX_PRIO_LEVEL_AUTO) {
		*p_out = range->end;
		return true;
	}

	if ((range->step == 0) || (level > (range->start - range->end) / range->step)) {
		return false;
	}

	*p_out = (uint8_t)(range->start - (level * range->step));

	return true;
}

static void ccconf_priority_set(uint8_t client, uint8_t pti)
{
	NRF_COEXC->CCCONF[client] =
		((pti << COEXC_CCCONF_PRIORITY_Pos) & COEXC_CCCONF_PRIORITY_Msk) |
		((CCCONF_SR_MODE << COEXC_CCCONF_MODE_Pos) & COEXC_CCCONF_MODE_Msk);
}

static void sr_priority_apply(mpsl_cx_op_map_t ops, bool critical)
{
	const uint8_t kind = critical ? PRIO_CRITICAL : PRIO_NORMAL;
	union sr_prio_table table;

	if (!atomic_get(&sr_prio_valid)) {
		return;
	}

	table.atomic = atomic_get(&sr_prio);

	if (ops & MPSL_CX_OP_RX) {
		ccconf_priority_set(SR_RX_CLIENT, table.prio[kind][SR_RX]);
	}

	if (ops & MPSL_CX_OP_TX) {
		ccconf_priority_set(SR_TX_CLIENT, table.prio[kind][SR_TX]);
	}
}

static int sr_coex_enable(bool enable)
{
	/* Use shared LNA when coex is enabled */
	if (enable) {
		SHARED_LNA_REG |= SHARED_LNA_MASK;
	} else {
		SHARED_LNA_REG &= ~SHARED_LNA_MASK;
	}

	atomic_set(&coex_enabled, enable);

	return 0;
}

static int sr_client_priority_set(const struct nrf71_sr_coex_sr_prio *prio)
{
	union sr_prio_table resolved;

	if (!prio_resolve(&prio->rx, prio->level, &resolved.prio[PRIO_NORMAL][SR_RX]) ||
	    !prio_resolve(&prio->tx, prio->level, &resolved.prio[PRIO_NORMAL][SR_TX]) ||
	    !prio_resolve(&prio->rx_critical, prio->level, &resolved.prio[PRIO_CRITICAL][SR_RX]) ||
	    !prio_resolve(&prio->tx_critical, prio->level, &resolved.prio[PRIO_CRITICAL][SR_TX])) {
		return -EINVAL;
	}

	atomic_set(&sr_prio, resolved.atomic);
	atomic_set(&sr_prio_valid, 1);

	return 0;
}

static const struct nrf71_sr_coex_sr_driver_cb sr_driver_cb = {
	.enable = sr_coex_enable,
	.set_priority = sr_client_priority_set,
};

static int32_t request(const mpsl_cx_request_t *p_req_params)
{
	mpsl_cx_op_map_t new_clients = 0;

	if (p_req_params == NULL) {
		return -EINVAL;
	}

	mpsl_cx_op_map_t ops = p_req_params->ops & (MPSL_CX_OP_RX | MPSL_CX_OP_TX);

	const bool enabled = atomic_get(&coex_enabled);
	const mpsl_cx_op_map_t clients = enabled ? ops : 0;
	const mpsl_cx_op_map_t prev_clients = (mpsl_cx_op_map_t)atomic_get(&requested_clients);

	if ((clients & MPSL_CX_OP_RX) &&
	    (!(prev_clients & MPSL_CX_OP_RX) || ipct_recv_check(RX_RELEASE_IPCT_CH))) {
		ipct_recv_clear(RX_REQUEST_IPCT_CH);
		ipct_recv_clear(RX_RELEASE_IPCT_CH);
		new_clients |= MPSL_CX_OP_RX;
	}

	if ((clients & MPSL_CX_OP_TX) &&
	    (!(prev_clients & MPSL_CX_OP_TX) || ipct_recv_check(TX_RELEASE_IPCT_CH))) {
		ipct_recv_clear(TX_REQUEST_IPCT_CH);
		ipct_recv_clear(TX_RELEASE_IPCT_CH);
		new_clients |= MPSL_CX_OP_TX;
	}

	atomic_set(&coex_disabled_on_request, !enabled);
	atomic_set(&requested_clients, (atomic_val_t)clients);

	clients_release(prev_clients & ~clients);

	/* Split the MPSL CX priority range in half to determine the COEXC activity type. */
	sr_priority_apply(clients, p_req_params->prio > (UINT8_MAX / 2));

	if (new_clients & MPSL_CX_OP_RX) {
		nrf_ipct_int_enable(COEXC_IPCT, COEXC_IPCT_INT_GROUP, RX_IPCT_CH_MASK);
		ipct_send(RX_REQUEST_IPCT_CH);
	}

	if (new_clients & MPSL_CX_OP_TX) {
		nrf_ipct_int_enable(COEXC_IPCT, COEXC_IPCT_INT_GROUP, TX_IPCT_CH_MASK);
		ipct_send(TX_REQUEST_IPCT_CH);
	}

	return 0;
}

static int32_t release(void)
{
	clients_release((mpsl_cx_op_map_t)atomic_set(&requested_clients, 0));

	return 0;
}

static int32_t granted_ops_get(mpsl_cx_op_map_t *p_granted_ops)
{
	mpsl_cx_op_map_t granted = MPSL_CX_OP_IDLE_LISTEN; /* always granted */

	if (p_granted_ops == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&coex_disabled_on_request)) {
		*p_granted_ops = granted | MPSL_CX_OP_RX | MPSL_CX_OP_TX;
		return 0;
	}

	mpsl_cx_op_map_t clients = (mpsl_cx_op_map_t)atomic_get(&requested_clients);

	if ((clients & MPSL_CX_OP_RX) && ipct_recv_check(RX_REQUEST_IPCT_CH) &&
	    !ipct_recv_check(RX_RELEASE_IPCT_CH)) {
		granted |= MPSL_CX_OP_RX;
	}

	if ((clients & MPSL_CX_OP_TX) && ipct_recv_check(TX_REQUEST_IPCT_CH) &&
	    !ipct_recv_check(TX_RELEASE_IPCT_CH)) {
		granted |= MPSL_CX_OP_TX;
	}

	*p_granted_ops = granted;

	return 0;
}

static uint32_t req_grant_delay_get(void)
{
	return REQUEST_LEAD_TIME;
}

static int32_t register_callback(mpsl_cx_cb_t cb)
{
	mpsl_cx_cb = cb;

	return 0;
}

static const mpsl_cx_interface_t mpsl_cx_methods = {
	.p_request = request,
	.p_release = release,
	.p_granted_ops_get = granted_ops_get,
	.p_req_grant_delay_get = req_grant_delay_get,
	.p_register_callback = register_callback,
};

static void coexc_ipct_isr(const void *arg)
{
	uint32_t pending;

	ARG_UNUSED(arg);

	/* IPCT events are kept pending and used as grant state in granted_ops_get.
	 * To ensure the callback gets called only once per change, the channel's interrupt is
	 * disabled until the next request. Events that are not pending should have IRQs enabled.
	 */
	pending = nrf_ipct_int_pending_get(COEXC_IPCT, COEXC_IPCT_INT_GROUP) & COEXC_IPCT_CH_MASK;

	nrf_ipct_int_disable(COEXC_IPCT, COEXC_IPCT_INT_GROUP, pending);

	const uint32_t cleared = pending & ~ipct_recv_events_get(pending);

	if (cleared != 0) {
		nrf_ipct_int_enable(COEXC_IPCT, COEXC_IPCT_INT_GROUP, cleared);
	}

	mpsl_cx_op_map_t changed_ops = 0;

	if (pending & RX_IPCT_CH_MASK) {
		changed_ops |= MPSL_CX_OP_RX;
	}

	if (pending & TX_IPCT_CH_MASK) {
		changed_ops |= MPSL_CX_OP_TX;
	}

	changed_ops &= (mpsl_cx_op_map_t)atomic_get(&requested_clients);

	mpsl_cx_cb_t cb = mpsl_cx_cb;

	if ((changed_ops != 0) && (cb != NULL)) {
		mpsl_cx_op_map_t granted_ops;

		(void)granted_ops_get(&granted_ops);
		cb(granted_ops);
	}
}

static int mpsl_cx_init(void)
{
	int32_t ret;

	ret = mpsl_cx_interface_set(&mpsl_cx_methods);
	if (ret != 0) {
		return ret;
	}

	nrf_ipct_int_disable(COEXC_IPCT, COEXC_IPCT_INT_GROUP, 0xFFFFFFFFUL);

	nrf_ipct_shorts_enable(COEXC_IPCT, COEXC_IPCT_CH_MASK << IPCT_SHORTS_RECEIVE0_FLUSH0_Pos);
	nrf_ipct_task_trigger(COEXC_IPCT, nrf_ipct_flush_task_get(RX_REQUEST_IPCT_CH));
	nrf_ipct_task_trigger(COEXC_IPCT, nrf_ipct_flush_task_get(RX_RELEASE_IPCT_CH));
	nrf_ipct_task_trigger(COEXC_IPCT, nrf_ipct_flush_task_get(TX_REQUEST_IPCT_CH));
	nrf_ipct_task_trigger(COEXC_IPCT, nrf_ipct_flush_task_get(TX_RELEASE_IPCT_CH));

	IRQ_CONNECT(COEXC_IPCT_IRQN, COEXC_IPCT_IRQ_PRIO, coexc_ipct_isr, NULL, 0);
	irq_enable(COEXC_IPCT_IRQN);

	ret = nrf71_sr_coex_sr_register(&sr_driver_cb);
	if (ret != 0) {
		return ret;
	}

	return nrf71_sr_coex_sr_power_notify(NRF71_SR_COEX_SR_POWERED_UP_READY);
}

SYS_INIT(mpsl_cx_init, POST_KERNEL, CONFIG_MPSL_CX_INIT_PRIORITY);
