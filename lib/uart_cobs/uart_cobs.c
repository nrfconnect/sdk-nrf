/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#define UART_COBS_DT DT_CHOSEN(nordic_cobs_uart_controller)

/* Compile-time checks for correct configuration. */
#if !DT_NODE_EXISTS(UART_COBS_DT)
#error "Missing /chosen devicetree node: nordic,cobs-uart-controller"

#elif !DT_NODE_HAS_STATUS(UART_COBS_DT, okay)
#error "nordic,cobs-uart-controller not enabled"

#elif !DT_PROP(UART_COBS_DT, hw_flow_control)
#error "Hardware flow control not enabled for nordic,cobs-uart-controller"

#else

#include <sys/time_units.h>
#include <sys/atomic.h>
#include <logging/log.h>
#include <drivers/uart.h>

#include <uart_cobs.h>
#include "cobs.h"

/* Compile-time check that max payload size is set correctly. */
BUILD_ASSERT(UART_COBS_MAX_PAYLOAD_LEN == COBS_MAX_DATA_BYTES,
	     "Max payload size should be equal to max COBS size.");

#define LOG_DBG_DEV(...) LOG_DBG(DT_LABEL(UART_COBS_DT) ": " __VA_ARGS__)

LOG_MODULE_REGISTER(uart_cobs, CONFIG_UART_COBS_LOG_LEVEL);


static void sw_evt_handle(const struct uart_cobs_user *user,
			  const struct uart_cobs_evt *evt);
static void no_evt_handle(const struct uart_cobs_user *user,
			  const struct uart_cobs_evt *evt);


/* States for RX and TX operation. */
enum op_status {
	STATUS_OFF      = 0,
	STATUS_ON       = 1,
	STATUS_PROCESS  = 2
};

struct {
	const struct device *dev;
	struct {
		atomic_t status;
		struct cobs_enc_buf buf;
		size_t len;
	} tx;
	struct {
		atomic_t status;
		struct k_timer timer;
		bool timeout_pending;
		struct cobs_dec decoder;
		uint8_t decoded_buf[COBS_MAX_DATA_BYTES];
		struct {
			uint8_t buf[COBS_MAX_BYTES];
			size_t offset;
			size_t len;
		} raw;
		size_t expected_len;
	} rx;
	struct {
		atomic_ptr_t current;
		const struct uart_cobs_user *idle;
		struct {
			const struct uart_cobs_user *from;
			const struct uart_cobs_user *to;
			int err;
			bool pending;
		} pending;
	} user;
	struct {
		struct k_work rx_dis;
		struct k_work tx_dis;
		struct k_work_q q;
	} work;
} state;

K_THREAD_STACK_DEFINE(work_q_stack_area,
		      CONFIG_UART_COBS_THREAD_STACK_SIZE);

UART_COBS_USER_DEFINE(sw_user, sw_evt_handle);
UART_COBS_USER_DEFINE(no_user, no_evt_handle);


static bool status_error_set(atomic_t *status, int err)
{
	return atomic_cas(status, STATUS_ON, err);
}

static inline bool rx_error_set(int err)
{
	return status_error_set(&state.rx.status, err);
}

static inline bool tx_error_set(int err)
{
	return status_error_set(&state.tx.status, err);
}

static inline void send_evt(const struct uart_cobs_evt *evt)
{
	struct uart_cobs_user *user;

	user = (struct uart_cobs_user *) atomic_ptr_get(&state.user.current);
	user->cb(user, evt);
}

static void send_evt_err(enum uart_cobs_evt_type type, int err)
{
	struct uart_cobs_evt evt;

	memset(&evt, 0, sizeof(evt));
	evt.type = type;
	evt.data.err = err;
	send_evt(&evt);
}

static void send_evt_user_start(void)
{
	struct uart_cobs_evt evt;

	memset(&evt, 0, sizeof(evt));
	evt.type = UART_COBS_EVT_USER_START;
	send_evt(&evt);
}

static void send_evt_user_end(const struct uart_cobs_user *from, int err)
{
	if (from != NULL) {
		struct uart_cobs_evt evt;

		memset(&evt, 0, sizeof(evt));
		evt.type = UART_COBS_EVT_USER_END;
		evt.data.err = err;
		from->cb(from, &evt);
	}
}

static bool user_sw_actions_abort(void)
{
	const struct uart_cobs_user *user;

	user = (struct uart_cobs_user *) atomic_ptr_get(&state.user.current);
	bool process = false;
	bool ready = true;

	/* Check current RX status. */
	switch (atomic_get(&state.rx.status)) {
	case STATUS_OFF:
		uart_cobs_rx_timeout_stop(user);
		break;
	case STATUS_PROCESS:
		uart_cobs_rx_timeout_stop(user);
		process = true;
		break;
	case STATUS_ON:
		(void) uart_cobs_rx_stop(user);
		ready = false;
		break;
	default:
		/* RX error state. */
		ready = false;
		break;
	}

	/* Check current TX status. */
	switch (atomic_get(&state.tx.status)) {
	case STATUS_OFF:
		break;
	case STATUS_ON:
		(void) uart_cobs_tx_stop(user);
		ready = false;
		break;
	default:
		/* TX error state. */
		ready = false;
		break;
	}

	state.user.pending.pending = ready;

	/* Switch should be finished synchronously if we are ready and
	 * not processing.
	 */
	return ready && !process;
}

static void user_sw_finish(void)
{
	state.user.pending.pending = false;

	/* Clear remaining state. */
	cobs_dec_reset(&state.rx.decoder);
	state.tx.len = 0;

	LOG_DBG_DEV("User end: %lu",
		    (unsigned long) state.user.pending.from);
	send_evt_user_end(state.user.pending.from, state.user.pending.err);

	LOG_DBG_DEV("User start: %lu",
		    (unsigned long) state.user.pending.to);
	LOG_DBG_DEV("RX: %d, TX: %d", atomic_get(&state.rx.status),
		    atomic_get(&state.tx.status));
	__ASSERT(atomic_get(&state.rx.status) == STATUS_OFF &&
		 atomic_get(&state.tx.status) == STATUS_OFF,
		 "Expected RX and TX to be off before switching user");
	(void) atomic_ptr_set(&state.user.current,
			      (atomic_ptr_t) state.user.pending.to);
	send_evt_user_start();
}

static int user_sw_prepare(const struct uart_cobs_user *from,
			   const struct uart_cobs_user *to, int err)
{
	/* Switch between sessions by passing through a temporary "switch"
	 * session where RX/TX can be stopped and residual events can be safely
	 * caught without interfering with normal sessions.
	 */

	if (!atomic_ptr_cas((atomic_ptr_t) &state.user.current,
			    (atomic_ptr_t) from, &sw_user)) {
		return -EBUSY;
	}

	LOG_DBG_DEV("User switch from %lu to %lu",
		    (unsigned long) from, (unsigned long) to);

	state.user.pending.from = from;
	state.user.pending.to = to;
	state.user.pending.err = err;

	if (user_sw_actions_abort()) {
		/* Finish synchronously. */
		user_sw_finish();
		return 0;
	}

	/* Finish asynchronously (via sw_evt_handle()). */
	return -EINPROGRESS;
}

static void no_evt_handle(const struct uart_cobs_user *user,
			  const struct uart_cobs_evt *evt)
{
	ARG_UNUSED(user);
	ARG_UNUSED(evt);

	/* Do nothing. */
}

static void sw_evt_handle(const struct uart_cobs_user *user,
			  const struct uart_cobs_evt *evt)
{
	ARG_UNUSED(user);
	ARG_UNUSED(evt);

	(void) user_sw_actions_abort();
}

static int rx_start(size_t len)
{
	state.rx.raw.offset = 0;
	state.rx.raw.len = 0;

	int err = uart_rx_enable(state.dev, state.rx.raw.buf,
				 len, SYS_FOREVER_MS);

	if (!err) {
		LOG_DBG_DEV("RX started");
	} else {
		(void) atomic_set(&state.rx.status, STATUS_OFF);
		LOG_DBG_DEV("RX failed to start (drv err %d)", err);
	}

	return err;
}

static void rx_resume(void)
{
	size_t len = state.rx.expected_len;

	if (cobs_dec_in_frame(&state.rx.decoder)) {
		if (cobs_dec_current_len(&state.rx.decoder) < len) {
			len -= cobs_dec_current_len(&state.rx.decoder);
		} else {
			len = 1;
		}
	}
	state.rx.expected_len = len;
	(void) rx_start(len);
}

static void rx_process(void)
{
	const uint8_t *buf = &state.rx.raw.buf[state.rx.raw.offset];
	struct uart_cobs_evt evt;

	evt.type = UART_COBS_EVT_RX;
	evt.data.rx.buf = state.rx.decoded_buf;

	for (size_t i = 0; i < state.rx.raw.len; i++) {
		int ret = cobs_decode_step(&state.rx.decoder, buf[i]);

		if (ret < 0) {
			/* Frame incomplete or erroneous. */
			continue;
		}

		/* Clear the RX state in the case that uart_cobs_rx_start()
		 * was called on a previous iteration.
		 */
		atomic_set(&state.rx.status, STATUS_PROCESS);

		evt.data.rx.len = ret;
		send_evt(&evt);
	}
}

static void rx_dis_process(struct k_work *work)
{
	ARG_UNUSED(work);

	/* While processing, set the status to STATUS_PROCESS to indicate
	 * that RX can be set pending but should not be turned on.
	 */
	atomic_val_t status = atomic_set(&state.rx.status, STATUS_PROCESS);
	bool send_err = true;

	if (status == STATUS_ON) {
		/* RX disabled due to full buffer or timeout. */
		rx_process();
	} else if (status == UART_COBS_ERR_TIMEOUT) {
		/* RX disabled due to timeout. Parse the data and allow the
		 * user to cancel the timeout with uart_cobs_rx_timeout_stop()
		 * before triggering an error.
		 */
		state.rx.timeout_pending = true;
		rx_process();
		send_err = state.rx.timeout_pending;
		state.rx.timeout_pending = false;
	} else if (status == -EAGAIN) {
		/* Bit-level error or buffer overrun.
		 * Reset and resume reception.
		 */
		cobs_dec_reset(&state.rx.decoder);
		atomic_set(&state.rx.status, STATUS_ON);
		send_err = false;
	}

	if (status < 0 && send_err) {
		/* Fatal error. */
		LOG_DBG_DEV("RX stopped with error: %d", status);
		send_evt_err(UART_COBS_EVT_RX_ERR, status);
	}

	status = atomic_get(&state.rx.status);
	if (status == STATUS_ON) {
		rx_resume();
	} else {
		(void) atomic_set(&state.rx.status, STATUS_OFF);
	}

	if (state.user.pending.pending) {
		/* Finish switch. */
		user_sw_finish();
	}
}

static void tx_dis_process(struct k_work *work)
{
	ARG_UNUSED(work);

	atomic_val_t status = atomic_set(&state.tx.status, STATUS_OFF);
	size_t len = state.tx.len;

	state.tx.len = 0;

	if (status < 0) {
		LOG_DBG_DEV("TX stopped with error %d", status);
		send_evt_err(UART_COBS_EVT_TX_ERR, status);
	} else {
		LOG_DBG_DEV("TX complete event");
		struct uart_cobs_evt evt;

		evt.type = UART_COBS_EVT_TX;
		evt.data.tx.len = len;
		send_evt(&evt);
	}

	if (state.user.pending.pending) {
		/* Finish switch. */
		user_sw_finish();
	}
}

static void rx_timer_expired(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	if (rx_error_set(UART_COBS_ERR_TIMEOUT)) {
		(void) uart_rx_disable(state.dev);
	}
}

static void uart_tx_handle(struct uart_event_tx *evt, bool aborted)
{
	LOG_DBG_DEV("TX %s (len=%u)", (aborted ? "aborted" : ""), evt->len);

	if (aborted) {
		/* This will not overwrite the user abort error if it is set. */
		(void) tx_error_set(UART_COBS_ERR_TIMEOUT);
	}
	k_work_submit_to_queue(&state.work.q, &state.work.tx_dis);
}

static void uart_rx_rdy_handle(struct uart_event_rx *evt)
{
	__ASSERT(evt->buf == state.rx.raw.buf,
		 "Expected event buf and local buf to be the same");

	LOG_DBG_DEV("RX (offset=%u, len=%u)", evt->offset, evt->len);

	state.rx.raw.offset = evt->offset;
	state.rx.raw.len = evt->len;
}

static void uart_rx_stopped_handle(struct uart_event_rx_stop *evt)
{
	LOG_DBG_DEV("RX stopped (reason=%d)", evt->reason);

	switch (evt->reason) {
	case UART_ERROR_OVERRUN:
	case UART_ERROR_PARITY:
	case UART_ERROR_FRAMING:
		(void) rx_error_set(-EAGAIN);
		break;
	case UART_BREAK:
	default:
		(void) rx_error_set(UART_COBS_ERR_BREAK);
		break;
	}
}

static void uart_rx_disabled_handle(void)
{
	k_work_submit_to_queue(&state.work.q, &state.work.rx_dis);
}

static void uart_async_cb(const struct device *dev,
			  struct uart_event *evt,
			  void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_TX_DONE:
		LOG_DBG_DEV("UART_TX_DONE");
		uart_tx_handle(&evt->data.tx, false);
		break;
	case UART_TX_ABORTED:
		LOG_DBG_DEV("UART_TX_ABORTED");
		uart_tx_handle(&evt->data.tx, true);
		break;
	case UART_RX_RDY:
		LOG_DBG_DEV("UART_RX_RDY");
		uart_rx_rdy_handle(&evt->data.rx);
		break;
	case UART_RX_DISABLED:
		LOG_DBG_DEV("UART_RX_DISABLED");
		uart_rx_disabled_handle();
		break;
	case UART_RX_STOPPED:
		LOG_DBG_DEV("UART_RX_STOPPED");
		uart_rx_stopped_handle(&evt->data.rx_stop);
		break;
	case UART_RX_BUF_REQUEST:
	case UART_RX_BUF_RELEASED:
	default:
		break;
	}
}

static int uart_cobs_sys_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Driver init. */
	state.dev = device_get_binding(DT_LABEL(UART_COBS_DT));
	if (state.dev == NULL) {
		return -EACCES;
	}

	int err = uart_callback_set(state.dev, uart_async_cb, NULL);

	if (err) {
		state.dev = NULL;
		return -EIO;
	}

	/* Module init. */
	(void) atomic_set(&state.rx.status, STATUS_OFF);
	(void) atomic_set(&state.tx.status, STATUS_OFF);

	k_timer_init(&state.rx.timer, rx_timer_expired, NULL);
	k_timer_user_data_set(&state.rx.timer, NULL);

	cobs_dec_init(&state.rx.decoder, state.rx.decoded_buf);

	state.user.idle = &no_user;
	(void) atomic_ptr_set(&state.user.current,
			      (atomic_ptr_t) state.user.idle);

	k_work_init(&state.work.rx_dis, rx_dis_process);
	k_work_init(&state.work.tx_dis, tx_dis_process);

	k_work_q_start(&state.work.q, work_q_stack_area,
		       K_THREAD_STACK_SIZEOF(work_q_stack_area),
		       CONFIG_UART_COBS_THREAD_PRIO);
	return 0;
}

int uart_cobs_init(void)
{
	if (state.dev != NULL) {
		/* Already initialized. */
		return 0;
	}

	return uart_cobs_sys_init(NULL);
}

int uart_cobs_user_start(const struct uart_cobs_user *user)
{
	if (user == NULL) {
		return -EINVAL;
	}
	if (atomic_ptr_get(&state.user.current) == (atomic_ptr_t) user) {
		return -EALREADY;
	} else {
		return user_sw_prepare(state.user.idle, user, 0);
	}
}

int uart_cobs_user_end(const struct uart_cobs_user *user, int err)
{
	if (user == NULL) {
		return -EINVAL;
	}
	return user_sw_prepare(user, state.user.idle, err);
}

bool uart_cobs_user_active(const struct uart_cobs_user *user)
{
	return atomic_ptr_get(&state.user.current) == (atomic_ptr_t) user;
}

int uart_cobs_idle_user_set(const struct uart_cobs_user *user)
{
	if (user == NULL) {
		/* Switch to the unset idle state if NULL is passed */
		user = &no_user;
	}

	/* Switch to empty idle handler first to lock the user state,
	 * preventing other switches from messing up the state.
	 */
	if (!atomic_ptr_cas(&state.user.current, (atomic_ptr_t) state.user.idle,
			    &no_user)) {
		return -EBUSY;
	}

	const struct uart_cobs_user *prev_user = state.user.idle;

	state.user.idle = user;
	if (prev_user == user) {
		return -EALREADY;
	}

	(void) user_sw_prepare(&no_user, user, 0);
	return 0;
}

int uart_cobs_tx_buf_write(const struct uart_cobs_user *user,
			   const uint8_t *data, size_t len)
{
	if (data == NULL) {
		return -EINVAL;
	}
	if (atomic_ptr_get(&state.user.current) != (atomic_ptr_t) user) {
		return -EACCES;
	}

	size_t new_len = state.tx.len + len;

	if (new_len > COBS_MAX_DATA_BYTES) {
		return -ENOMEM;
	}

	if (atomic_get(&state.tx.status) != STATUS_OFF) {
		return -EBUSY;
	}

	memcpy(&state.tx.buf.buf[state.tx.len], data, len);
	state.tx.len = new_len;
	return 0;
}

int uart_cobs_tx_buf_clear(const struct uart_cobs_user *user)
{
	if (atomic_ptr_get(&state.user.current) != (atomic_ptr_t) user) {
		return -EACCES;
	}
	if (atomic_get(&state.tx.status) != STATUS_OFF) {
		return -EBUSY;
	}

	state.tx.len = 0;
	return 0;
}

int uart_cobs_tx_start(const struct uart_cobs_user *user, int timeout)
{
	if (atomic_ptr_get(&state.user.current) != (atomic_ptr_t) user) {
		return -EACCES;
	}
	if (!atomic_cas(&state.tx.status, STATUS_OFF, STATUS_ON)) {
		LOG_DBG_DEV("TX was already started");
		return -EBUSY;
	}

	int err = cobs_encode(&state.tx.buf, state.tx.len);

	if (err) {
		(void) atomic_set(&state.tx.status, STATUS_OFF);
		return err;
	}

	err = uart_tx(state.dev, (uint8_t *) &state.tx.buf,
		      COBS_ENCODED_SIZE(state.tx.len), timeout);
	if (err) {
		LOG_DBG_DEV("TX failed to start (drv err %d)", err);
		(void) atomic_set(&state.tx.status, STATUS_OFF);
		return err;
	}
	return 0;
}

int uart_cobs_tx_stop(const struct uart_cobs_user *user)
{
	if (atomic_ptr_get(&state.user.current) != (atomic_ptr_t) user) {
		return -EACCES;
	}
	if (!tx_error_set(UART_COBS_ERR_ABORT)) {
		/* Already aborted or aborting. */
		return 0;
	}

	return uart_tx_abort(state.dev);
}

int uart_cobs_rx_start(const struct uart_cobs_user *user, size_t len)
{
	if (atomic_ptr_get(&state.user.current) != (atomic_ptr_t) user) {
		return -EACCES;
	}
	if (len == 0 || len > COBS_MAX_DATA_BYTES) {
		return -EINVAL;
	}

	bool start;

	if (atomic_cas(&state.rx.status, STATUS_OFF, STATUS_ON)) {
		/* RX was off and not processing. */
		start = true;
	} else if (atomic_cas(&state.rx.status, STATUS_PROCESS, STATUS_ON)) {
		/* RX is processing and will be resumed after processing is
		 * done.
		 */
		start = false;
	} else {
		LOG_DBG_DEV("RX was already started");
		return -EBUSY;
	}

	state.rx.expected_len = COBS_ENCODED_SIZE(len);
	if (!start) {
		return 0;
	}
	return rx_start(COBS_ENCODED_SIZE(len));
}

int uart_cobs_rx_stop(const struct uart_cobs_user *user)
{
	if (atomic_ptr_get(&state.user.current) != (atomic_ptr_t) user) {
		return -EACCES;
	}
	if (!rx_error_set(UART_COBS_ERR_ABORT)) {
		/* Already aborted or aborting. */
		return 0;
	}
	return uart_rx_disable(state.dev);
}

int uart_cobs_rx_timeout_start(const struct uart_cobs_user *user, int timeout)
{
	if (atomic_ptr_get(&state.user.current) != (atomic_ptr_t) user) {
		return -EACCES;
	}
	k_timer_start(&state.rx.timer, K_MSEC(timeout), K_NO_WAIT);
	return 0;
}

int uart_cobs_rx_timeout_stop(const struct uart_cobs_user *user)
{
	if (atomic_ptr_get(&state.user.current) != (atomic_ptr_t) user) {
		return -EACCES;
	}
	state.rx.timeout_pending = false;
	k_timer_stop(&state.rx.timer);
	return 0;
}

bool uart_cobs_in_work_q_thread(void)
{
	return k_current_get() == &state.work.q.thread;
}

SYS_INIT(uart_cobs_sys_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);


#endif /* DT_NODE_EXISTS(UART_COBS_DT) */
