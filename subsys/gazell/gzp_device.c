/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <nrf_gzll.h>
#include <gzp.h>
#include "gzp_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gzp_device, CONFIG_GAZELL_LOG_LEVEL);

#define GZP_DEVICE_DB_NAME      "gzp_device"
#define GZP_DEVICE_DB_SYS_ADDR  "sys_addr"
#define GZP_DEVICE_DB_HOST_ID   "host_id"

/*
 * Possible return values for the function gzp_tx_rx_transaction()
 */
enum gzp_tx_rx_trans_result {
	GZP_TX_RX_SUCCESS,        /* ACK received. Transaction successful. */
	GZP_TX_RX_FAILED_TO_SEND,
	GZP_TX_RX_NO_RESPONSE
};

/* GZP state group bitmask */
#define GZP_STATE_GROUP_BITMASK ~0xFU

/* GZP state */
enum gzp_state {
	/* null state */
	GZP_STATE_NULL = 0x0,

	/* gzp_address_req_send_async() processing */
	GZP_STATE_ADDR_REQ_GROUP = 0x10,
	GZP_STATE_ADDR_REQ_DISABLE,
	GZP_STATE_ADDR_REQ_TX_STEP_0,
	GZP_STATE_ADDR_REQ_TX_STEP_1,
	GZP_STATE_ADDR_REQ_UPDATE_RADIO,
	GZP_STATE_ADDR_REQ_DISABLE_2,

	/* gzp_id_req_send_async() processing */
	GZP_STATE_ID_REQ_GROUP = 0x20,
	GZP_STATE_ID_REQ_REQUEST,
	GZP_STATE_ID_REQ_FETCH,

	/* gzp_crypt_data_send_async() processing */
	GZP_STATE_CRYPT_SEND_GROUP = 0x30,
	GZP_STATE_CRYPT_SEND_TX,
	GZP_STATE_CRYPT_SEND_KEY_UPDATE,
	GZP_STATE_CRYPT_SEND_TX_2,

	/* gzp_crypt_tx_transaction_async() processing */
	GZP_STATE_CRYPT_TX_GROUP = 0x40,
	GZP_STATE_CRYPT_TX_USER_DATA,

	/* gzp_key_update_async() processing */
	GZP_STATE_KEY_UPDATE_GROUP = 0x50,
	GZP_STATE_KEY_UPDATE_PREPARE,
	GZP_STATE_KEY_UPDATE,

	/* gzp_tx_rx_transaction_async() processing */
	GZP_STATE_TX_RX_GROUP = 0x60,
	GZP_STATE_TX_RX_DISABLE,
	GZP_STATE_TX_RX_TX,
	GZP_STATE_TX_RX_SLEEP,
	GZP_STATE_TX_RX_RX,
	GZP_STATE_TX_RX_DISABLE_2,
};

/* GZP event type */
enum gzp_event_type {
	GZP_EVENT_TX_SUCCESS,
	GZP_EVENT_TX_FAILED,
	GZP_EVENT_LL_DISABLED,
	GZP_EVENT_LL_TICK_TIMER,
	GZP_EVENT_TX_RX_DONE,
	GZP_EVENT_CRYPT_TX_DONE,
	GZP_EVENT_KEY_UPDATE_DONE,
};

/* GZP event information */
struct gzp_event_info {
	enum gzp_event_type event_type;

	union {
		struct {
			uint32_t pipe;
			nrf_gzll_device_tx_info_t info;
		} tx;

		enum gzp_tx_rx_trans_result result;

		bool retval;
	};
};

/* GZP state event processing function */
typedef void (*gzp_state_event_process)(enum gzp_state state,
					void *context,
					const struct gzp_event_info *event_info);

/* GZP state group mapping to event processing function */
struct gzp_state_group_event_process {
	enum gzp_state state_group;
	gzp_state_event_process event_process;
};

/* GZP state and context pool position */
struct gzp_state_ctx {
	enum gzp_state state;
	uint8_t ctx_pos;
};

/* The context used in gzp_address_req_send_async() and gzp_addr_req_process() */
struct __packed gzp_address_req_send_ctx {
	bool retval;
	nrf_gzll_tx_power_t temp_power;
	uint32_t temp_max_tx_attempts;
	uint8_t req_send_cnt;
};

/* The context used in gzp_crypt_data_send_async() and gzp_crypt_data_send_process() */
struct __packed gzp_crypt_data_send_ctx {
	const uint8_t *src;
	uint8_t length;
};

/* The context used in gzp_tx_rx_transaction_async() and gzp_tx_rx_process() */
struct __packed gzp_tx_rx_ctx {
	enum gzp_tx_rx_trans_result retval;
	uint8_t pipe;
	uint32_t temp_lifetime;
};

/* User callback context */
struct gzp_user_callback_context {
	void *callback;
	void *context;
};

/* The context used in gzp_address_req_send() */
struct gzp_address_req_wrap_up_ctx {
	bool result;
	struct k_sem done_sem;
};

/* The context used in gzp_id_req_send() */
struct gzp_id_req_wrap_up_ctx {
	enum gzp_id_req_res result;
	struct k_sem done_sem;
};

/* The context used in gzp_crypt_data_send() */
struct gzp_crypt_data_send_wrap_up_ctx {
	bool result;
	struct k_sem done_sem;
};

/* GZP System Address */
static uint8_t gzp_system_address[GZP_SYSTEM_ADDRESS_WIDTH];

#ifdef CONFIG_GAZELL_PAIRING_CRYPT

/* GZP Host ID */
static uint8_t gzp_host_id[GZP_HOST_ID_LENGTH];
static bool gzp_id_req_pending;

#endif

#ifdef CONFIG_GAZELL_PAIRING_SETTINGS

/* Settings key for System Address */
static const char db_key_sys_addr[] = GZP_DEVICE_DB_NAME "/" GZP_DEVICE_DB_SYS_ADDR;
#ifdef CONFIG_GAZELL_PAIRING_CRYPT
/* Settings key for Host ID */
static const char db_key_host_id[] = GZP_DEVICE_DB_NAME "/" GZP_DEVICE_DB_HOST_ID;
#endif

#endif

#ifdef CONFIG_GAZELL_PAIRING_CRYPT

/*
 * Function sending the packet and a subsequent packet fetching the response.
 *
 * @param pipe is the pipe on which the packet should be sent.
 */
static void gzp_tx_rx_transaction_async(uint8_t pipe);

/*
 * Function for sending an encrypted packet. The function detects whether the correct
 * key was used, and attempts to send a "key update" to the host if the wrong key was being
 * used.

 * @param tx_packet is a pointer to the packet to be sent.
 * @param length is the length of the packet to be sent.

 * @retval true if transmission succeeded and packet was decrypted correctly by host.
 * @retval false if transmission failed or packet was not decrypted correctly by host.
 */
static void gzp_crypt_tx_transaction_async(const uint8_t *tx_packet, uint8_t length);

/*
 * Function updating the "dynamic key" and sending a "key update" to the host.
 */
static void gzp_key_update_async(void);

#endif

/*
 * Function returning true if array contains only 1s (0xff).
 *
 * @param *src is a pointer to the array to be evaluated.
 * @param length is the length of the array to be evaluated.
 *
 * @retval true
 * @retval false
 */
static bool gzp_array_is_set(const uint8_t *src, uint8_t length);

#ifdef CONFIG_GAZELL_PAIRING_SETTINGS

/*
 * Function for storing the current "system address" and "host ID" in NV memory.
 *
 * @param store_all selects whether only "system address" or both "system address" and
 *                  "host ID" should be stored.
 * @arg true selects that both should be stored.
 * @arg false selects that only "system address" should be stored.
 *
 * @retval true
 * @retval false
 */
static bool gzp_params_store(bool store_all);

/*
 * Restore the "system address" and "host ID" from NV memory.
 * @retval true
 * @retval false
 */
static bool gzp_params_restore(void);

#endif

/*
 * Delay function. Will add a delay equal to GZLL_RX_PERIOD * rx_periods [us].
 *
 * @param rx_periods
 */
static void gzp_delay_rx_periods(uint32_t rx_periods);

static void gzp_device_worker(struct k_work *work);

static void gzp_addr_req_process(enum gzp_state state,
				 void *context,
				 const struct gzp_event_info *event_info);

#ifdef CONFIG_GAZELL_PAIRING_CRYPT

static void gzp_id_req_process(enum gzp_state state,
			       void *context,
			       const struct gzp_event_info *event_info);

static void gzp_crypt_data_send_process(enum gzp_state state,
					void *context,
					const struct gzp_event_info *event_info);

static void gzp_tx_rx_process(enum gzp_state state,
			      void *context,
			      const struct gzp_event_info *event_info);

static void gzp_crypt_tx_process(enum gzp_state state,
				 void *context,
				 const struct gzp_event_info *event_info);

static void gzp_key_update_process(enum gzp_state state,
				   void *context,
				   const struct gzp_event_info *event_info);

#endif /* CONFIG_GAZELL_PAIRING_CRYPT */

/* Work item */
static K_WORK_DEFINE(gzp_device_work, gzp_device_worker);

/* Message queue */
K_MSGQ_DEFINE(gzp_msgq,
	      sizeof(struct gzp_event_info),
	      CONFIG_GAZELL_PAIRING_DEVICE_MSGQ_LEN,
	      sizeof(uint32_t));

/* GZP state context stack */
static struct gzp_state_ctx state_stack[3];
/* GZP state stack count */
static uint8_t state_stack_cnt;

/* Context pool */
static uint8_t ctx_pool[12];
/* Context pool position */
static uint8_t ctx_pool_pos;

BUILD_ASSERT(sizeof(ctx_pool) >= sizeof(struct gzp_address_req_send_ctx),
		"The context pool is too small for system address request");

BUILD_ASSERT(sizeof(ctx_pool) >= sizeof(struct gzp_crypt_data_send_ctx) +
				 sizeof(struct gzp_tx_rx_ctx),
		"The context pool is too small for crypt data send");

/* User callback function */
static void *gzp_user_callback;
/* User callback context */
static void *gzp_user_context;

/* GZP transmit buffer */
static uint8_t gzp_tx_packet[GZP_MAX_FW_PAYLOAD_LENGTH];
/* GZP transmit buffer length */
static uint8_t gzp_tx_packet_length;

/* GZP receive buffer */
static uint8_t gzp_rx_packet[GZP_MAX_ACK_PAYLOAD_LENGTH];
/* GZP receive buffer length */
static uint32_t gzp_rx_packet_length;

/* User callback for tracing transmit results */
static gzp_tx_result_callback gzp_tx_result_user_callback;

/* Table to search for a GZP state event processing function */
static const struct gzp_state_group_event_process gzp_state_group_event_processes[] = {
#ifdef CONFIG_GAZELL_PAIRING_CRYPT
	{GZP_STATE_TX_RX_GROUP, gzp_tx_rx_process},
	{GZP_STATE_CRYPT_SEND_GROUP, gzp_crypt_data_send_process},
	{GZP_STATE_CRYPT_TX_GROUP, gzp_crypt_tx_process},
	{GZP_STATE_KEY_UPDATE_GROUP, gzp_key_update_process},
#endif
	{GZP_STATE_ADDR_REQ_GROUP, gzp_addr_req_process},
#ifdef CONFIG_GAZELL_PAIRING_CRYPT
	{GZP_STATE_ID_REQ_GROUP, gzp_id_req_process},
#endif
	{GZP_STATE_NULL, NULL}
};

/* GZP command: Host address request */
static const uint8_t gzp_cmd_host_address_req[GZP_CMD_HOST_ADDRESS_REQ_PAYLOAD_LENGTH]
					     = {GZP_CMD_HOST_ADDRESS_REQ};

/* GZP command: Host address fetch */
static const uint8_t gzp_cmd_host_address_fetch[GZP_CMD_HOST_ADDRESS_FETCH_PAYLOAD_LENGTH]
					       = {GZP_CMD_HOST_ADDRESS_FETCH};

/* GZP command: Fetch response */
static const uint8_t gzp_cmd_fetch_resp[GZP_CMD_FETCH_RESP_PAYLOAD_LENGTH]
				       = {GZP_CMD_FETCH_RESP};

/* GZP command: Key update prepare */
static const uint8_t gzp_cmd_key_update_prepare[GZP_CMD_KEY_UPDATE_PREPARE_PAYLOAD_LENGTH]
					       = {GZP_CMD_KEY_UPDATE_PREPARE};


/* Get the current GZP state.
 */
static enum gzp_state gzp_state_get(void)
{
	if (state_stack_cnt) {
		return state_stack[state_stack_cnt - 1].state;
	} else {
		return GZP_STATE_NULL;
	}
}

/* Set the current GZP state.
 */
static void gzp_state_set(enum gzp_state state)
{
	state_stack[state_stack_cnt - 1].state = state;
}

/* Get the current GZP state context.
 */
static uint8_t *gzp_state_get_ctx(void)
{
	__ASSERT_NO_MSG(state_stack_cnt);

	return ctx_pool + state_stack[state_stack_cnt - 1].ctx_pos;
}

/* Push a GZP state to stack and allocate context pool.
 *
 * @param state GZP state
 * @param ctx_size context size requested
 *
 * @retval context pointer
 */
static uint8_t *gzp_state_new(enum gzp_state state, size_t ctx_size)
{
	if (state_stack_cnt < ARRAY_SIZE(state_stack) &&
	    ctx_pool_pos + ctx_size <= sizeof(ctx_pool)) {
		state_stack_cnt++;

		gzp_state_set(state);

		state_stack[state_stack_cnt - 1].ctx_pos = ctx_pool_pos;
		ctx_pool_pos += ctx_size;

		return gzp_state_get_ctx();
	} else {
		return NULL;
	}
}

/* Remove the current GZP state from stack.
 */
static void gzp_state_remove(void)
{
	if (state_stack_cnt) {
		ctx_pool_pos = state_stack[state_stack_cnt - 1].ctx_pos;
		state_stack_cnt--;
	}
}

/* Work item handler function.
 */
static void gzp_device_worker(struct k_work *work)
{
	struct gzp_event_info event_info;

	while (!k_msgq_get(&gzp_msgq, &event_info, K_NO_WAIT)) {
		/* Process the event with respect to the current state context */
		if (state_stack_cnt > 0) {
			enum gzp_state state = gzp_state_get();
			enum gzp_state state_group = state & GZP_STATE_GROUP_BITMASK;
			const struct gzp_state_group_event_process *event_process;

			event_process = gzp_state_group_event_processes;

			while (event_process->state_group != GZP_STATE_NULL) {
				if (event_process->state_group == state_group) {
					event_process->event_process(state,
								     gzp_state_get_ctx(),
								     &event_info);
					break;
				}

				event_process++;
			}
		}

		/* Call user function for tracing transmit results */
		if (gzp_tx_result_user_callback) {
			switch (event_info.event_type) {
			case GZP_EVENT_TX_SUCCESS:
				gzp_tx_result_user_callback(true,
							    event_info.tx.pipe,
							    &event_info.tx.info);
				break;
			case GZP_EVENT_TX_FAILED:
				gzp_tx_result_user_callback(false,
							    event_info.tx.pipe,
							    &event_info.tx.info);
				break;
			default:
				break;
			};
		}
	}
}

#ifdef CONFIG_GAZELL_PAIRING_SETTINGS

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	if (!strcmp(key, GZP_DEVICE_DB_SYS_ADDR)) {
		uint8_t tmp_addr[GZP_SYSTEM_ADDRESS_WIDTH];

		ssize_t len = read_cb(cb_arg, tmp_addr,
				      sizeof(tmp_addr));

		if (len == sizeof(tmp_addr)) {
			memcpy(gzp_system_address, tmp_addr, len);
		} else {
			LOG_WRN("Sys Addr data invalid length (%u)", len);

			return -EINVAL;
		}
#ifdef CONFIG_GAZELL_PAIRING_CRYPT
	} else if (!strcmp(key, GZP_DEVICE_DB_HOST_ID)) {
		uint8_t tmp_id[GZP_HOST_ID_LENGTH];

		ssize_t len = read_cb(cb_arg, tmp_id,
				      sizeof(tmp_id));

		if (len == sizeof(tmp_id)) {
			memcpy(gzp_host_id, tmp_id, len);
		} else {
			LOG_WRN("Host ID data invalid length (%u)", len);

			return -EINVAL;
		}
#endif
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(nrf_gzp_device, GZP_DEVICE_DB_NAME, NULL, settings_set, NULL,
			       NULL);

#endif

#ifdef CONFIG_GAZELL_PAIRING_CRYPT

static void gzp_clear_host_id(void)
{
	memset(gzp_host_id, 0xff, GZP_HOST_ID_LENGTH);
}

#endif

static void gzp_clear_sys_addr(void)
{
	memset(gzp_system_address, 0xff, GZP_SYSTEM_ADDRESS_WIDTH);
}

void gzp_init(void)
{
	gzp_clear_sys_addr();
#ifdef CONFIG_GAZELL_PAIRING_CRYPT
	gzp_clear_host_id();
#endif

	gzp_init_internal();

#ifdef CONFIG_GAZELL_PAIRING_SETTINGS
	(void)gzp_params_restore();
#endif

	/* Update radio parameters from gzp_system_address */
	(void)gzp_update_radio_params(gzp_system_address);
}

void gzp_erase_pairing_data(void)
{
#ifdef CONFIG_GAZELL_PAIRING_SETTINGS
	settings_delete(db_key_sys_addr);
#ifdef CONFIG_GAZELL_PAIRING_CRYPT
	settings_delete(db_key_host_id);
#endif /* CONFIG_GAZELL_PAIRING_CRYPT */
#endif /* CONFIG_GAZELL_PAIRING_SETTINGS */
	gzp_clear_sys_addr();
#ifdef CONFIG_GAZELL_PAIRING_CRYPT
	gzp_clear_host_id();
#endif /* CONFIG_GAZELL_PAIRING_CRYPT */
}

/* Set user callback context.
 *
 * @param callback Callback function
 * @param context Callback context
 *
 * @retval true
 * @retval false
 */
static bool gzp_set_user_callback(void *callback, void *context)
{
	if (!gzp_user_callback) {
		gzp_user_callback = callback;
		gzp_user_context = context;

		return true;
	} else {
		return false;
	}
}

/* Get user callback context.
 */
static struct gzp_user_callback_context gzp_get_user_callback(void)
{
	struct gzp_user_callback_context retval;

	retval.callback = gzp_user_callback;
	retval.context = gzp_user_context;

	return retval;
}

/* Clear user callback context.
 */
static void gzp_clear_user_callback(void)
{
	gzp_user_callback = NULL;
	gzp_user_context = NULL;
}

static void gzp_address_req_wrap_up(bool result, void *context)
{
	struct gzp_address_req_wrap_up_ctx *ctx = (struct gzp_address_req_wrap_up_ctx *)context;

	ctx->result = result;

	k_sem_give(&ctx->done_sem);
}

bool gzp_address_req_send(void)
{
	struct gzp_address_req_wrap_up_ctx ctx;

	k_sem_init(&ctx.done_sem, 0, 1);

	gzp_address_req_send_async(gzp_address_req_wrap_up, &ctx);

	k_sem_take(&ctx.done_sem, K_FOREVER);

	return ctx.result;
}

void gzp_address_req_send_async(gzp_address_req_callback callback, void *context)
{
	struct gzp_address_req_send_ctx *ctx;

	if (!gzp_set_user_callback(callback, context)) {
		callback(false, context);
		return;
	}

	ctx = (struct gzp_address_req_send_ctx *)gzp_state_new(GZP_STATE_ADDR_REQ_DISABLE,
							sizeof(struct gzp_address_req_send_ctx));
	if (!ctx) {
		gzp_clear_user_callback();
		callback(false, context);
		return;
	}

	ctx->retval = false;

	/* Store parameters that are temporarily changed */
	ctx->temp_max_tx_attempts = nrf_gzll_get_max_tx_attempts();
	ctx->temp_power = nrf_gzll_get_tx_power();

	(void)nrf_gzll_disable();
}

static bool gzp_send_host_addr_req(void)
{
	return nrf_gzll_add_packet_to_tx_fifo(GZP_PAIRING_PIPE,
					      gzp_cmd_host_address_req,
					      GZP_CMD_HOST_ADDRESS_REQ_PAYLOAD_LENGTH);
}

static void gzp_addr_req_to_finish(void)
{
	gzp_state_set(GZP_STATE_ADDR_REQ_DISABLE_2);

	nrf_gzll_disable();
}

static void gzp_addr_req_process_ack_payload(struct gzp_address_req_send_ctx *ctx)
{
	if (nrf_gzll_get_rx_fifo_packet_count(GZP_PAIRING_PIPE) > 0) {
		gzp_rx_packet_length = GZP_MAX_ACK_PAYLOAD_LENGTH;
		if (nrf_gzll_fetch_packet_from_rx_fifo(GZP_PAIRING_PIPE,
						       gzp_rx_packet,
						       &gzp_rx_packet_length)) {
			if (gzp_rx_packet_length == GZP_CMD_HOST_ADDRESS_RESP_PAYLOAD_LENGTH &&
			    gzp_rx_packet[0] == (uint8_t)GZP_CMD_HOST_ADDRESS_RESP) {
				memcpy(gzp_system_address,
				       &gzp_rx_packet[GZP_CMD_HOST_ADDRESS_RESP_ADDRESS],
				       GZP_SYSTEM_ADDRESS_WIDTH);
#ifdef CONFIG_GAZELL_PAIRING_SETTINGS
				(void)gzp_params_store(false);
#endif
				ctx->retval = true;
			}
		}
	}
}

static void gzp_addr_req_process(enum gzp_state state,
				 void *context,
				 const struct gzp_event_info *event_info)
{
	struct gzp_address_req_send_ctx *ctx = (struct gzp_address_req_send_ctx *)context;

	switch (state) {
	case GZP_STATE_ADDR_REQ_DISABLE:
		if (event_info->event_type == GZP_EVENT_LL_DISABLED) {
			/* Modify parameters */
			nrf_gzll_set_max_tx_attempts(GZP_REQ_TX_TIMEOUT);
			nrf_gzll_set_tx_power(GZP_POWER);
			__ASSERT_NO_MSG(nrf_gzll_get_error_code() == NRF_GZLL_ERROR_CODE_NO_ERROR);

			/* Flush RX FIFO */
			nrf_gzll_flush_rx_fifo(GZP_PAIRING_PIPE);
			__ASSERT_NO_MSG(nrf_gzll_get_error_code() == NRF_GZLL_ERROR_CODE_NO_ERROR);
			nrf_gzll_enable();
			__ASSERT_NO_MSG(nrf_gzll_get_error_code() == NRF_GZLL_ERROR_CODE_NO_ERROR);

			gzp_state_set(GZP_STATE_ADDR_REQ_TX_STEP_0);

			ctx->req_send_cnt = 0;

			if (!gzp_send_host_addr_req()) {
				gzp_addr_req_to_finish();
			}
		}
		break;
	case GZP_STATE_ADDR_REQ_TX_STEP_0:
		if (event_info->event_type == GZP_EVENT_TX_SUCCESS &&
		    event_info->tx.pipe == GZP_PAIRING_PIPE) {
			nrf_gzp_flush_rx_fifo(GZP_PAIRING_PIPE);

			ctx->req_send_cnt++;
			if (ctx->req_send_cnt >= GZP_MAX_BACKOFF_PACKETS) {
				gzp_state_set(GZP_STATE_ADDR_REQ_TX_STEP_1);
			}
		} else if (event_info->event_type == GZP_EVENT_TX_FAILED &&
			   event_info->tx.pipe == GZP_PAIRING_PIPE) {
			gzp_state_set(GZP_STATE_ADDR_REQ_TX_STEP_1);
		} else {
			break;
		}

		if (gzp_state_get() == GZP_STATE_ADDR_REQ_TX_STEP_0) {
			if (!gzp_send_host_addr_req()) {
				gzp_addr_req_to_finish();
			}
		} else {
			gzp_delay_rx_periods(GZP_TX_ACK_WAIT_TIMEOUT);

			if (!nrf_gzll_add_packet_to_tx_fifo(
				GZP_PAIRING_PIPE,
				gzp_cmd_host_address_fetch,
				GZP_CMD_HOST_ADDRESS_FETCH_PAYLOAD_LENGTH)) {
				gzp_addr_req_to_finish();
			}
		}
		break;
	case GZP_STATE_ADDR_REQ_TX_STEP_1:
		if (event_info->event_type == GZP_EVENT_TX_SUCCESS &&
		    event_info->tx.pipe == GZP_PAIRING_PIPE) {
			if (event_info->tx.info.payload_received_in_ack) {
				/* If pairing response received */
				gzp_addr_req_process_ack_payload(ctx);
			}
		} else if (event_info->event_type == GZP_EVENT_TX_FAILED &&
			   event_info->tx.pipe == GZP_PAIRING_PIPE) {
			gzp_delay_rx_periods(GZP_NOT_PROXIMITY_BACKOFF_RX_TIMEOUT -
					     GZP_TX_ACK_WAIT_TIMEOUT);
		} else {
			break;
		}

		gzp_delay_rx_periods(GZP_STEP1_RX_TIMEOUT);
		gzp_addr_req_to_finish();
		break;
	case GZP_STATE_ADDR_REQ_DISABLE_2:
		if (event_info->event_type == GZP_EVENT_LL_DISABLED) {
			struct gzp_user_callback_context callback_context;
			bool retval = ctx->retval;

			if (retval) {
				if (!gzp_update_radio_params(gzp_system_address)) {
					LOG_ERR("Cannot update radio parameters");
				}
			}
			nrf_gzll_flush_rx_fifo(GZP_PAIRING_PIPE);
			__ASSERT_NO_MSG(nrf_gzll_get_error_code() == NRF_GZLL_ERROR_CODE_NO_ERROR);
			nrf_gzll_flush_tx_fifo(GZP_PAIRING_PIPE);
			__ASSERT_NO_MSG(nrf_gzll_get_error_code() == NRF_GZLL_ERROR_CODE_NO_ERROR);
			nrf_gzll_set_max_tx_attempts(ctx->temp_max_tx_attempts);
			nrf_gzll_set_tx_power(ctx->temp_power);
			__ASSERT_NO_MSG(nrf_gzll_get_error_code() == NRF_GZLL_ERROR_CODE_NO_ERROR);
			nrf_gzll_enable();
			__ASSERT_NO_MSG(nrf_gzll_get_error_code() == NRF_GZLL_ERROR_CODE_NO_ERROR);

			gzp_state_remove();

			callback_context = gzp_get_user_callback();
			gzp_clear_user_callback();
			((gzp_address_req_callback)callback_context.callback)(
							retval,
							callback_context.context);
		}
		break;
	default:
		break;
	}
}

#ifdef CONFIG_GAZELL_PAIRING_CRYPT

/* Report a TX RX transaction event for subsequent processing.
 *
 * @param event_type GZP event type
 * @param result TX RX transaction result
 */
static void gzp_device_report_tx_rx(enum gzp_event_type event_type,
				    enum gzp_tx_rx_trans_result result)
{
	int err;
	struct gzp_event_info event_info;

	event_info.event_type = event_type;
	event_info.result = result;
	err = k_msgq_put(&gzp_msgq, &event_info, K_NO_WAIT);
	if (!err) {
		k_work_submit(&gzp_device_work);
	} else {
		LOG_ERR("Cannot put TX RX event to message queue");
	}
}

static void gzp_id_req_to_finish(enum gzp_id_req_res result)
{
	struct gzp_user_callback_context callback_context;

	gzp_state_remove();

	callback_context = gzp_get_user_callback();
	gzp_clear_user_callback();
	((gzp_id_req_callback)callback_context.callback)(
				result,
				callback_context.context);
}

void gzp_id_req_send_async(gzp_id_req_callback callback, void *context)
{
	if (!gzp_set_user_callback(callback, context)) {
		callback(GZP_ID_RESP_FAILED, context);
		return;
	}

	/* If no ID request is pending, send new "ID request" */
	if (!gzp_id_req_pending) {
		if (!gzp_state_new(GZP_STATE_ID_REQ_REQUEST, 0)) {
			gzp_clear_user_callback();
			callback(GZP_ID_RESP_FAILED, context);
			return;
		}

		/* Build "Host ID request packet" */
		gzp_tx_packet[0] = (uint8_t)GZP_CMD_HOST_ID_REQ;

		/* Generate new session token */
		gzp_random_numbers_generate(&gzp_tx_packet[GZP_CMD_HOST_ID_REQ_SESSION_TOKEN],
					    GZP_SESSION_TOKEN_LENGTH);

		/* Send "Host ID request" */
		if (!nrf_gzll_add_packet_to_tx_fifo(
				GZP_DATA_PIPE,
				gzp_tx_packet,
				GZP_CMD_HOST_ID_REQ_PAYLOAD_LENGTH)) {
			gzp_id_req_to_finish(GZP_ID_RESP_FAILED);
		}
	} else {
		/* If "ID request is pending" send "fetch ID" packet */

		if (!gzp_state_new(GZP_STATE_ID_REQ_FETCH, 0)) {
			gzp_clear_user_callback();
			callback(GZP_ID_RESP_FAILED, context);
			return;
		}

		/* Build "host ID fetch" packet */
		gzp_tx_packet[0] = (uint8_t)GZP_CMD_HOST_ID_FETCH;
		gzp_add_validation_id(&gzp_tx_packet[GZP_CMD_HOST_ID_FETCH_VALIDATION_ID]);

		/* Encrypt "host ID fetch" packet */
		gzp_crypt_select_key(GZP_ID_EXCHANGE);
		gzp_crypt(&gzp_tx_packet[1],
			  &gzp_tx_packet[1],
			  GZP_CMD_HOST_ID_FETCH_PAYLOAD_LENGTH - 1);

		gzp_tx_packet_length = GZP_CMD_HOST_ID_FETCH_PAYLOAD_LENGTH;

		gzp_tx_rx_transaction_async(GZP_DATA_PIPE);
	}
}

static bool gzp_id_req_process_tx_rx_success(void)
{
	bool is_resp_packet;
	uint8_t dyn_key[GZP_DYN_KEY_LENGTH];

	/* Validate response packet */
	if (gzp_rx_packet_length == GZP_CMD_HOST_ID_FETCH_RESP_PAYLOAD_LENGTH &&
	    gzp_rx_packet[0] == (uint8_t)GZP_CMD_HOST_ID_FETCH_RESP) {
		gzp_crypt(&gzp_rx_packet[1],
			  &gzp_rx_packet[1],
			  GZP_CMD_HOST_ID_FETCH_RESP_PAYLOAD_LENGTH - 1);
		if (gzp_validate_id(
		    &gzp_rx_packet[GZP_CMD_HOST_ID_FETCH_RESP_VALIDATION_ID])) {
			enum gzp_id_req_res result;

			result = (enum gzp_id_req_res)
				 gzp_rx_packet[GZP_CMD_HOST_ID_FETCH_RESP_STATUS];
			switch (result) {
			case GZP_ID_RESP_PENDING:
				break;
			case GZP_ID_RESP_REJECTED:
				gzp_id_req_pending = false;
				break;
			case GZP_ID_RESP_GRANTED:
				gzp_set_host_id(
					&gzp_rx_packet[GZP_CMD_HOST_ID_FETCH_RESP_HOST_ID]);
				gzp_random_numbers_generate(dyn_key, GZP_DYN_KEY_LENGTH);
				gzp_crypt_set_dyn_key(dyn_key);
#ifdef CONFIG_GAZELL_PAIRING_SETTINGS
				(void)gzp_params_store(true);
#endif
				gzp_id_req_pending = false;
				break;
			default:
				break;
			}

			gzp_id_req_to_finish(result);
		} else {
			gzp_id_req_pending = false;
			gzp_id_req_to_finish(GZP_ID_RESP_REJECTED);
		}

		is_resp_packet = true;
	} else {
		is_resp_packet = false;
	}

	return is_resp_packet;
}

static void gzp_id_req_process(enum gzp_state state,
			       void *context,
			       const struct gzp_event_info *event_info)
{
	ARG_UNUSED(context);

	switch (state) {
	case GZP_STATE_ID_REQ_REQUEST:
		if (event_info->event_type == GZP_EVENT_TX_SUCCESS &&
		    event_info->tx.pipe == GZP_DATA_PIPE) {
			/* Update session token if "Host ID request" was successfully transmitted */
			gzp_crypt_set_session_token(
				&gzp_tx_packet[GZP_CMD_HOST_ID_REQ_SESSION_TOKEN]);
			gzp_id_req_pending = true;
			gzp_id_req_to_finish(GZP_ID_RESP_PENDING);
		} else if (event_info->event_type == GZP_EVENT_TX_FAILED &&
			   event_info->tx.pipe == GZP_DATA_PIPE) {
			gzp_id_req_pending = false;
			gzp_id_req_to_finish(GZP_ID_RESP_FAILED);
		}
		break;
	case GZP_STATE_ID_REQ_FETCH:
		if (event_info->event_type == GZP_EVENT_TX_RX_DONE) {
			/* If packet was successfully sent AND a response packet was received */
			if (event_info->result == GZP_TX_RX_SUCCESS) {
				if (gzp_id_req_process_tx_rx_success()) {
					break;
				}
			}

			gzp_id_req_pending = false;
			gzp_id_req_to_finish(GZP_ID_RESP_FAILED);
		}
		break;
	default:
		break;
	}
}

static void gzp_id_req_wrap_up(enum gzp_id_req_res result, void *context)
{
	struct gzp_id_req_wrap_up_ctx *ctx = (struct gzp_id_req_wrap_up_ctx *)context;

	ctx->result = result;

	k_sem_give(&ctx->done_sem);
}

enum gzp_id_req_res gzp_id_req_send(void)
{
	struct gzp_id_req_wrap_up_ctx ctx;

	k_sem_init(&ctx.done_sem, 0, 1);

	gzp_id_req_send_async(gzp_id_req_wrap_up, &ctx);

	k_sem_take(&ctx.done_sem, K_FOREVER);

	return ctx.result;
}

void gzp_id_req_cancel(void)
{
	gzp_id_req_pending = false;
}

/* Report a boolean result event for subsequent processing.
 *
 * @param event_type GZP event type
 * @param result boolean result
 */
static void gzp_device_report_retval(enum gzp_event_type event_type,
				     bool result)
{
	int err;
	struct gzp_event_info event_info;

	event_info.event_type = event_type;
	event_info.retval = result;
	err = k_msgq_put(&gzp_msgq, &event_info, K_NO_WAIT);
	if (!err) {
		k_work_submit(&gzp_device_work);
	} else {
		LOG_ERR("Cannot put boolean event to message queue");
	}
}

static void gzp_crypt_send_wrap_up(bool result, void *context)
{
	struct gzp_crypt_data_send_wrap_up_ctx *ctx;

	ctx = (struct gzp_crypt_data_send_wrap_up_ctx *)context;

	ctx->result = result;

	k_sem_give(&ctx->done_sem);
}

bool gzp_crypt_data_send(const uint8_t *src, uint8_t length)
{
	struct gzp_crypt_data_send_wrap_up_ctx ctx;

	k_sem_init(&ctx.done_sem, 0, 1);

	gzp_crypt_data_send_async(src, length, gzp_crypt_send_wrap_up, &ctx);

	k_sem_take(&ctx.done_sem, K_FOREVER);

	return ctx.result;
}

void gzp_crypt_data_send_async(const uint8_t *src,
			       uint8_t length,
			       gzp_crypt_data_send_callback callback,
			       void *context)
{
	struct gzp_crypt_data_send_ctx *ctx;

	if (length > GZP_ENCRYPTED_USER_DATA_MAX_LENGTH ||
	    !gzp_set_user_callback(callback, context)) {
		callback(false, context);
		return;
	}

	ctx = (struct gzp_crypt_data_send_ctx *)gzp_state_new(GZP_STATE_CRYPT_SEND_TX,
							sizeof(struct gzp_crypt_data_send_ctx));
	if (!ctx) {
		gzp_clear_user_callback();
		callback(false, context);
		return;
	}

	ctx->src = src;
	ctx->length = length;

	gzp_crypt_tx_transaction_async(src, length);
}

static void gzp_crypt_data_send_to_finish(bool result)
{
	struct gzp_user_callback_context callback_context;

	gzp_state_remove();

	callback_context = gzp_get_user_callback();
	gzp_clear_user_callback();
	((gzp_crypt_data_send_callback)callback_context.callback)(
						result,
						callback_context.context);
}

static void gzp_crypt_data_send_process(enum gzp_state state,
					void *context,
					const struct gzp_event_info *event_info)
{
	struct gzp_crypt_data_send_ctx *ctx = (struct gzp_crypt_data_send_ctx *)context;

	switch (state) {
	case GZP_STATE_CRYPT_SEND_TX:
		if (event_info->event_type == GZP_EVENT_CRYPT_TX_DONE) {
			if (event_info->retval) {
				gzp_crypt_data_send_to_finish(true);
				break;
			}

			LOG_DBG("GZP_CRYPT_TX failed");

			/* Attempt key update if user data transmission failed
			 * during normal operation (!gzp_id_req_pending)
			 */
			if (!gzp_id_req_pending) {
				LOG_DBG("KEY UPDATE");
				gzp_state_set(GZP_STATE_CRYPT_SEND_KEY_UPDATE);
				gzp_key_update_async();
				break;
			}

			gzp_crypt_data_send_to_finish(false);
			break;
		}
		break;
	case GZP_STATE_CRYPT_SEND_KEY_UPDATE:
		if (event_info->event_type == GZP_EVENT_KEY_UPDATE_DONE) {
			if (event_info->retval) {
				gzp_state_set(GZP_STATE_CRYPT_SEND_TX_2);
				gzp_crypt_tx_transaction_async(ctx->src,
							       ctx->length);
			} else {
				gzp_crypt_data_send_to_finish(false);
			}
		}
		break;
	case GZP_STATE_CRYPT_SEND_TX_2:
		if (event_info->event_type == GZP_EVENT_CRYPT_TX_DONE) {
			gzp_crypt_data_send_to_finish(event_info->retval);
		}
		break;
	default:
		break;
	}
}

#endif /* CONFIG_GAZELL_PAIRING_CRYPT */

/* Report a GZLL miscellaneous event for subsequent processing.
 *
 * @param event_type GZP event type
 */
static void gzp_device_report_ll(enum gzp_event_type event_type)
{
	int err;
	struct gzp_event_info event_info;

	event_info.event_type = event_type;
	err = k_msgq_put(&gzp_msgq, &event_info, K_NO_WAIT);
	if (!err) {
		k_work_submit(&gzp_device_work);
	} else {
		LOG_ERR("Cannot put LL event to message queue");
	}
}

#ifdef CONFIG_GAZELL_PAIRING_CRYPT

static void gzp_tx_rx_transaction_async(uint8_t pipe)
{
	struct gzp_tx_rx_ctx *ctx;

	ctx = (struct gzp_tx_rx_ctx *)gzp_state_new(GZP_STATE_TX_RX_DISABLE,
						    sizeof(struct gzp_tx_rx_ctx));
	if (!ctx) {
		gzp_device_report_tx_rx(GZP_EVENT_TX_RX_DONE, GZP_TX_RX_FAILED_TO_SEND);
		return;
	}

	ctx->pipe = pipe;

	nrf_gzp_flush_rx_fifo(pipe);

	nrf_gzll_disable();
}

static void gzp_tick_timer_callback(void)
{
	gzp_device_report_ll(GZP_EVENT_LL_TICK_TIMER);
}

static void gzp_tx_rx_to_finish(enum gzp_tx_rx_trans_result retval, struct gzp_tx_rx_ctx *ctx)
{
	ctx->retval = retval;

	gzp_state_set(GZP_STATE_TX_RX_DISABLE_2);

	nrf_gzll_disable();
}

static void gzp_tx_rx_process(enum gzp_state state,
			      void *context,
			      const struct gzp_event_info *event_info)
{
	struct gzp_tx_rx_ctx *ctx = (struct gzp_tx_rx_ctx *)context;

	switch (state) {
	case GZP_STATE_TX_RX_DISABLE:
		if (event_info->event_type == GZP_EVENT_LL_DISABLED) {
			ctx->temp_lifetime = nrf_gzll_get_sync_lifetime();
			(void)nrf_gzll_set_sync_lifetime(GZP_TX_RX_TRANS_DELAY * 3);
				/* 3 = RXPERIOD * 2 + margin */
			(void)nrf_gzll_enable();

			gzp_state_set(GZP_STATE_TX_RX_TX);

			if (!nrf_gzll_add_packet_to_tx_fifo(ctx->pipe,
							    gzp_tx_packet,
							    gzp_tx_packet_length)) {
				gzp_tx_rx_to_finish(GZP_TX_RX_FAILED_TO_SEND, ctx);
			}
		}
		break;
	case GZP_STATE_TX_RX_TX:
		if (event_info->event_type == GZP_EVENT_TX_SUCCESS &&
		    event_info->tx.pipe == ctx->pipe) {
			nrf_gzp_flush_rx_fifo(ctx->pipe);

			gzp_state_set(GZP_STATE_TX_RX_SLEEP);

			nrf_gzll_set_tick_timer(2 * GZP_TX_RX_TRANS_DELAY,
						gzp_tick_timer_callback);
		} else if (event_info->event_type == GZP_EVENT_TX_FAILED &&
			   event_info->tx.pipe == ctx->pipe) {
			gzp_tx_rx_to_finish(GZP_TX_RX_FAILED_TO_SEND, ctx);
		}
		break;
	case GZP_STATE_TX_RX_SLEEP:
		if (event_info->event_type == GZP_EVENT_LL_TICK_TIMER) {
			gzp_state_set(GZP_STATE_TX_RX_RX);

			if (!nrf_gzll_add_packet_to_tx_fifo(ctx->pipe,
							    gzp_cmd_fetch_resp,
							    GZP_CMD_FETCH_RESP_PAYLOAD_LENGTH)) {
				gzp_tx_rx_to_finish(GZP_TX_RX_NO_RESPONSE, ctx);
			}
		}
		break;
	case GZP_STATE_TX_RX_RX:
		if (event_info->event_type == GZP_EVENT_TX_SUCCESS &&
		    event_info->tx.pipe == ctx->pipe) {
			enum gzp_tx_rx_trans_result retval;
			bool fetch_success;

			if (nrf_gzll_get_rx_fifo_packet_count(ctx->pipe)) {
				gzp_rx_packet_length = GZP_MAX_ACK_PAYLOAD_LENGTH;
				fetch_success = nrf_gzll_fetch_packet_from_rx_fifo(
							ctx->pipe,
							gzp_rx_packet,
							&gzp_rx_packet_length);
				if (!fetch_success) {
					nrf_gzll_flush_rx_fifo(ctx->pipe);
				}
			} else {
				fetch_success = false;
			}

			retval = (fetch_success) ? GZP_TX_RX_SUCCESS : GZP_TX_RX_NO_RESPONSE;
			gzp_tx_rx_to_finish(retval, ctx);
		} else if (event_info->event_type == GZP_EVENT_TX_FAILED &&
			   event_info->tx.pipe == ctx->pipe) {
			gzp_tx_rx_to_finish(GZP_TX_RX_NO_RESPONSE, ctx);
		}
		break;
	case GZP_STATE_TX_RX_DISABLE_2:
		if (event_info->event_type == GZP_EVENT_LL_DISABLED) {
			enum gzp_tx_rx_trans_result retval = ctx->retval;

			(void)nrf_gzll_set_sync_lifetime(ctx->temp_lifetime);
			(void)nrf_gzll_enable();

			gzp_state_remove();

			gzp_device_report_tx_rx(GZP_EVENT_TX_RX_DONE, retval);
		}
		break;
	default:
		break;
	}
}

static void gzp_crypt_tx_transaction_async(const uint8_t *src,
					   uint8_t length)
{
	if (!gzp_state_new(GZP_STATE_CRYPT_TX_USER_DATA, 0)) {
		gzp_device_report_retval(GZP_EVENT_CRYPT_TX_DONE, false);
		return;
	}

	gzp_tx_packet_length = length + (uint8_t)GZP_ENCRYPTED_USER_DATA_PACKET_OVERHEAD;

	/* Assemble tx packet */
	gzp_tx_packet[0] = (uint8_t)GZP_CMD_ENCRYPTED_USER_DATA;
	gzp_add_validation_id(&gzp_tx_packet[GZP_CMD_ENCRYPTED_USER_DATA_VALIDATION_ID]);
	memcpy(&gzp_tx_packet[GZP_CMD_ENCRYPTED_USER_DATA_PAYLOAD], src, length);

	/* Encrypt tx packet */
	if (gzp_id_req_pending) {
		gzp_crypt_select_key(GZP_ID_EXCHANGE);
	} else {
		gzp_crypt_select_key(GZP_DATA_EXCHANGE);
	}
	gzp_crypt(&gzp_tx_packet[1], &gzp_tx_packet[1], gzp_tx_packet_length - 1);

	gzp_tx_rx_transaction_async(GZP_DATA_PIPE);
}

static void gzp_crypt_tx_to_finish(bool result)
{
	gzp_state_remove();

	gzp_device_report_retval(GZP_EVENT_CRYPT_TX_DONE, result);
}

static void gzp_crypt_tx_process_tx_rx_success(void)
{
	if (gzp_rx_packet_length == GZP_CMD_ENCRYPTED_USER_DATA_RESP_PAYLOAD_LENGTH &&
	    gzp_rx_packet[0] == (uint8_t)GZP_CMD_ENCRYPTED_USER_DATA_RESP) {
		gzp_crypt(&gzp_rx_packet[GZP_CMD_ENCRYPTED_USER_DATA_RESP_VALIDATION_ID],
			  &gzp_rx_packet[GZP_CMD_ENCRYPTED_USER_DATA_RESP_VALIDATION_ID],
			  GZP_VALIDATION_ID_LENGTH);

		/* Validate response in order to know whether packet was
		 * correctly decrypted by host
		 */
		if (gzp_validate_id(
		    &gzp_rx_packet[GZP_CMD_ENCRYPTED_USER_DATA_RESP_VALIDATION_ID])) {
			/* Update session token if normal operation
			 * (!gzp_id_req_pending)
			 */
			if (!gzp_id_req_pending) {
				gzp_crypt_set_session_token(
					&gzp_rx_packet[
					GZP_CMD_ENCRYPTED_USER_DATA_RESP_SESSION_TOKEN]);
			}
			gzp_crypt_tx_to_finish(true);
		} else {
			LOG_DBG("GZP_CRYPT_TX_TRANS: Validation ID bad");
			gzp_crypt_tx_to_finish(false);
		}
	} else {
		LOG_DBG("GZP_CRYPT_TX_TRANS: Bad CMD.");
		gzp_crypt_tx_to_finish(false);
	}
}

static void gzp_crypt_tx_process(enum gzp_state state,
				 void *context,
				 const struct gzp_event_info *event_info)
{
	ARG_UNUSED(context);

	switch (state) {
	case GZP_STATE_CRYPT_TX_USER_DATA:
		if (event_info->event_type == GZP_EVENT_TX_RX_DONE) {
			/* If packet was successfully sent AND a response packet was received */
			if (event_info->result == GZP_TX_RX_SUCCESS) {
				gzp_crypt_tx_process_tx_rx_success();
			} else {
				LOG_DBG("GZP_CRYPT_TX_TRANS: gzp_tx_rx_trans not SUCCESS");
				gzp_crypt_tx_to_finish(false);
			}
		}
		break;
	default:
		break;
	}
}

static void gzp_key_update_async(void)
{
	if (!gzp_state_new(GZP_STATE_KEY_UPDATE_PREPARE, 0)) {
		gzp_device_report_retval(GZP_EVENT_KEY_UPDATE_DONE, false);
		return;
	}

	memcpy(gzp_tx_packet,
	       gzp_cmd_key_update_prepare,
	       GZP_CMD_KEY_UPDATE_PREPARE_PAYLOAD_LENGTH);
	gzp_tx_packet_length = GZP_CMD_KEY_UPDATE_PREPARE_PAYLOAD_LENGTH;

	/* Send "prepare packet" to get session token to be used for key update */
	gzp_tx_rx_transaction_async(GZP_DATA_PIPE);
}

static void gzp_key_update_to_finish(bool result)
{
	gzp_state_remove();

	gzp_device_report_retval(GZP_EVENT_KEY_UPDATE_DONE, result);
}

static void gzp_key_update_process(enum gzp_state state,
				   void *context,
				   const struct gzp_event_info *event_info)
{
	ARG_UNUSED(context);

	switch (state) {
	case GZP_STATE_KEY_UPDATE_PREPARE:
		if (event_info->event_type == GZP_EVENT_TX_RX_DONE) {
			/* If packet was successfully sent AND a response packet was received */
			if (event_info->result == GZP_TX_RX_SUCCESS) {
				if (gzp_rx_packet_length ==
				    GZP_CMD_KEY_UPDATE_PREPARE_RESP_PAYLOAD_LENGTH &&
				    gzp_rx_packet[0] == (uint8_t)GZP_CMD_KEY_UPDATE_PREPARE_RESP) {
					gzp_crypt_set_session_token(
						&gzp_rx_packet[
						GZP_CMD_KEY_UPDATE_PREPARE_RESP_SESSION_TOKEN]);

					/* Build "key update" packet */
					gzp_tx_packet[0] = (uint8_t)GZP_CMD_KEY_UPDATE;
					gzp_add_validation_id(
						&gzp_tx_packet[GZP_CMD_KEY_UPDATE_VALIDATION_ID]);
					gzp_random_numbers_generate(
						&gzp_tx_packet[GZP_CMD_KEY_UPDATE_NEW_KEY],
						GZP_DYN_KEY_LENGTH);
					gzp_crypt_set_dyn_key(
						&gzp_tx_packet[GZP_CMD_KEY_UPDATE_NEW_KEY]);

					/* Encrypt "key update packet" */
					gzp_crypt_select_key(GZP_KEY_EXCHANGE);
					gzp_crypt(&gzp_tx_packet[1],
						  &gzp_tx_packet[1],
						  GZP_CMD_KEY_UPDATE_PAYLOAD_LENGTH - 1);

					gzp_state_set(GZP_STATE_KEY_UPDATE);

					/* Send "key update" packet */
					if (nrf_gzll_add_packet_to_tx_fifo(GZP_DATA_PIPE,
						gzp_tx_packet,
						GZP_CMD_KEY_UPDATE_PAYLOAD_LENGTH)) {
						break;
					}
				}
			}

			gzp_key_update_to_finish(false);
		}
		break;
	case GZP_STATE_KEY_UPDATE:
		if (event_info->event_type == GZP_EVENT_TX_SUCCESS &&
		    event_info->tx.pipe == GZP_DATA_PIPE) {
			gzp_key_update_to_finish(true);
		} else if (event_info->event_type == GZP_EVENT_TX_FAILED &&
			   event_info->tx.pipe == GZP_DATA_PIPE) {
			gzp_key_update_to_finish(false);
		}
		break;
	default:
		break;
	}
}

void gzp_set_host_id(const uint8_t *id)
{
	memcpy(gzp_host_id, id, GZP_HOST_ID_LENGTH);
}

void gzp_get_host_id(uint8_t *dst_id)
{
	memcpy(dst_id, gzp_host_id, GZP_HOST_ID_LENGTH);
}

#endif /* CONFIG_GAZELL_PAIRING_CRYPT */

int8_t gzp_get_pairing_status(void)
{
	int8_t db_index;

	db_index = (gzp_array_is_set(gzp_system_address, GZP_SYSTEM_ADDRESS_WIDTH)) ? -2 : -1;
#ifdef CONFIG_GAZELL_PAIRING_CRYPT
	if (db_index == -1 && !gzp_array_is_set(gzp_host_id, GZP_HOST_ID_LENGTH)) {
		db_index = 0;
	}
#endif

	return db_index;
}

static bool gzp_array_is_set(const uint8_t *src, uint8_t length)
{
	uint8_t i;

	for (i = 0; i < length; i++) {
		if (*(src + i) != 0xff) {
			return false;
		}
	}
	return true;
}

#ifdef CONFIG_GAZELL_PAIRING_SETTINGS

static bool gzp_params_store(bool store_all)
{
	int err;

#ifdef CONFIG_GAZELL_PAIRING_CRYPT
	err = settings_delete(db_key_host_id);
	if (err) {
		LOG_WRN("Cannot delete host id, err %d", err);
	}
#endif

	err = settings_save_one(db_key_sys_addr,
				gzp_system_address,
				sizeof(gzp_system_address));
	if (err) {
		LOG_ERR("Cannot store system address, err %d", err);
#ifdef CONFIG_GAZELL_PAIRING_CRYPT
	} else {
		err = settings_save_one(db_key_host_id,
					gzp_host_id,
					sizeof(gzp_host_id));
		if (err) {
			LOG_ERR("Cannot store host id, err %d", err);
		}
#endif
	}

	return (!err) ? true : false;
}

static bool gzp_params_restore(void)
{
	int err;

	err = settings_load_subtree(GZP_DEVICE_DB_NAME);
	if (err) {
		LOG_ERR("Cannot load settings");
	}

	return (!err) ? true : false;
}

#endif /* CONFIG_GAZELL_PAIRING_SETTINGS */

static void gzp_delay_rx_periods(uint32_t rx_periods)
{
	k_busy_wait(rx_periods * 2 * nrf_gzll_get_timeslot_period());
}

/* Report a GZLL transmit event for subsequent processing.
 *
 * @param event_type GZP event type
 * @param pipe GZLL pipe
 * @param tx_info GZLL transmit information
 */
static void gzp_device_report_tx(enum gzp_event_type event_type,
				 uint32_t pipe,
				 nrf_gzll_device_tx_info_t *tx_info)
{
	int err;
	struct gzp_event_info event_info;

	event_info.event_type = event_type;
	event_info.tx.pipe = pipe;
	event_info.tx.info = *tx_info;
	err = k_msgq_put(&gzp_msgq, &event_info, K_NO_WAIT);
	if (!err) {
		k_work_submit(&gzp_device_work);
	} else {
		LOG_ERR("Cannot put TX event to message queue");
	}
}

void nrf_gzll_device_tx_success(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info)
{
	gzp_device_report_tx(GZP_EVENT_TX_SUCCESS, pipe, &tx_info);
}

void nrf_gzll_device_tx_failed(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info)
{
	gzp_device_report_tx(GZP_EVENT_TX_FAILED, pipe, &tx_info);
}

void nrf_gzll_disabled(void)
{
	gzp_device_report_ll(GZP_EVENT_LL_DISABLED);
}

void nrf_gzll_host_rx_data_ready(uint32_t pipe, nrf_gzll_host_rx_info_t rx_info)
{
}

void gzp_tx_result_callback_register(gzp_tx_result_callback callback)
{
	gzp_tx_result_user_callback = callback;
}
