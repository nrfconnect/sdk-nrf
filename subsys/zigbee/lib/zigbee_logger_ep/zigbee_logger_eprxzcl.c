/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <inttypes.h>

#include <zephyr/logging/log.h>

#include <zigbee/zigbee_app_utils.h>
#include <zigbee/zigbee_logger_eprxzcl.h>

LOG_LEVEL_SET(CONFIG_ZIGBEE_LOGGER_EP_LOG_LEVEL);

/**@brief Name of the log module related to Zigbee. */
#define LOG_MODULE_NAME zigbee

/** @brief Name of the submodule used for logger messaging. */
#define LOG_SUBMODULE_NAME eprxzcl

LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, LOG_SUBMODULE_NAME,
		      CONFIG_ZIGBEE_LOGGER_EP_LOG_LEVEL);

/* This structure keeps reference to the logger instance used by this module. */
struct log_ctx {
	LOG_INSTANCE_PTR_DECLARE(inst);
};

/* Logger instance used by this module. */
static struct log_ctx logger = { LOG_INSTANCE_PTR_INIT(
	inst, LOG_MODULE_NAME, LOG_SUBMODULE_NAME) };

/**@brief Number of logs that can be simultaneously buffered before processing
 * by a backend
 */
#define PRV_LOG_CIRC_BUFFER_MAX_LOGS 8U

/**@brief Maximum length of log line. */
#define PRV_LOG_CIRC_BUFFER_MAX_MESSAGE_LENGTH 512U

typedef struct {
	size_t idx;
	char buffers[PRV_LOG_CIRC_BUFFER_MAX_LOGS]
		    [PRV_LOG_CIRC_BUFFER_MAX_MESSAGE_LENGTH];
} prv_log_circ_buffer_t;

/**@brief ZCL received frames log counter.
 * @details Successive calls to @ref zigbee_logger_eprxzcl_ep_handler
 *          increment this value.
 * @ref zigbee_logger_eprxzcl_ep_handler puts this value in every log line
 */
static uint32_t log_counter;

/**@brief Circular buffer of logs
 *        produced by @ref zigbee_logger_eprxzcl_ep_handler
 * @details Successive calls to @ref prv_log_circ_buffer_get_next_buffer
 *          return successive buffers to be filled in and that may be put
 *          into LOG_INF with deferred processing.
 */
static prv_log_circ_buffer_t log_circ_buffer;

/**@brief Function returning next log buffer to be filled in.
 *
 * This function returns a buffer, which may be filled up with log message
 * and passed to LOG_INF.
 * User must be aware that actually no 'free' operation happens anywhere.
 * Buffer may get overwritten if the backend processes logs slower than
 * they are produced. Keep PRV_LOG_CIRC_BUFFER_MAX_LOGS big enough.
 *
 * @return Pointer to the log message buffer capable of storing
 *         LOG_CIRC_BUFFER_MAX_MESSAGE_LENGTH bytes (including null terminator).
 */
static char *prv_log_circ_buffer_get_next_buffer(void)
{
	char *result;
	size_t new_idx;

	new_idx = log_circ_buffer.idx;
	result = log_circ_buffer.buffers[new_idx];

	/* Increment modulo now */
	new_idx++;
	if (new_idx >= PRV_LOG_CIRC_BUFFER_MAX_LOGS) {
		new_idx = 0U;
	}
	log_circ_buffer.idx = new_idx;

	memset(result, 0, PRV_LOG_CIRC_BUFFER_MAX_MESSAGE_LENGTH);

	return result;
}

/**@brief Macro used within zigbee_logger_eprxzcl_ep_handler
 *        to perform guarded log_message_curr advance
 * @note To be used within @ref zigbee_logger_eprxzcl_ep_handler only
 */
#define PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(s)			\
	do {							\
		if ((s) < 0) {					\
			LOG_INST_ERR(				\
				logger.inst,			\
				"Received ZCL command but encoding error occurred during log producing"); \
			return ZB_FALSE;			\
		}						\
		log_message_curr += (s);			\
		if (log_message_curr >= log_message_end) {	\
			LOG_INST_ERR(				\
				logger.inst,			\
				"Received ZCL command but produced log is too long"); \
			return ZB_FALSE;			\
		}						\
	} while (0)

zb_uint8_t zigbee_logger_eprxzcl_ep_handler(zb_bufid_t bufid)
{
	if (CONFIG_ZIGBEE_LOGGER_EP_LOG_LEVEL >= LOG_LEVEL_INF) {
		int status;
		char *log_message = prv_log_circ_buffer_get_next_buffer();
		char *log_message_curr = log_message;
		char *log_message_end =
			log_message_curr +
			PRV_LOG_CIRC_BUFFER_MAX_MESSAGE_LENGTH - 1U;
		zb_zcl_parsed_hdr_t *cmd_info =
			ZB_BUF_GET_PARAM(bufid, zb_zcl_parsed_hdr_t);
		size_t payload_length = zb_buf_len(bufid);
		const zb_uint8_t *payload = zb_buf_begin(bufid);
		uint32_t log_number = log_counter++;

		status = snprintf(log_message_curr,
				  log_message_end - log_message_curr,
				  "Received ZCL command (%" PRIu32
				  "): src_addr=",
				  log_number);
		PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);

		switch (ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info)
			.source.addr_type) {
		case ZB_ZCL_ADDR_TYPE_SHORT:
			status = snprintf(
				log_message_curr,
				log_message_end - log_message_curr,
				"0x%04x(short)",
				ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info)
				.source.u.short_addr);
			PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);
			break;

		case ZB_ZCL_ADDR_TYPE_IEEE_GPD:
			status = ieee_addr_to_str(
				log_message_curr,
				log_message_end - log_message_curr,
				ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info)
				.source.u.ieee_addr);
			PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);

			status =
				snprintf(log_message_curr,
					 log_message_end - log_message_curr,
					 "(ieee_gpd)");
			PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);
			break;

		case ZB_ZCL_ADDR_TYPE_SRC_ID_GPD:
			status = snprintf(
				log_message_curr,
				log_message_end - log_message_curr,
				"0x%x(src_id_gpd)",
				ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info)
				.source.u.src_id);
			PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);
			break;

		case ZB_ZCL_ADDR_TYPE_IEEE:
			status = ieee_addr_to_str(
				log_message_curr,
				log_message_end - log_message_curr,
				ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info)
				.source.u.ieee_addr);
			PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);
			status =
				snprintf(log_message_curr,
					 log_message_end - log_message_curr,
					 "(ieee)");
			PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);
			break;

		default:
			status =
				snprintf(log_message_curr,
					 log_message_end - log_message_curr,
					 "0(unknown)");
			PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);
			break;
		}

		status = snprintf(
			log_message_curr,
			log_message_end - log_message_curr,
			" src_ep=%u dst_ep=%u cluster_id=0x%04x profile_id=0x%04x",
			ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
			ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
			cmd_info->cluster_id, cmd_info->profile_id);
		PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);

		status = snprintf(
			log_message_curr,
			log_message_end - log_message_curr,
			" cmd_dir=%u common_cmd=%u cmd_id=0x%02x cmd_seq=%u disable_def_resp=%u",
			cmd_info->cmd_direction,
			cmd_info->is_common_command, cmd_info->cmd_id,
			cmd_info->seq_number,
			cmd_info->disable_default_response);
		PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);

		if (cmd_info->is_manuf_specific) {
			status =
				snprintf(log_message_curr,
					 log_message_end - log_message_curr,
					 " manuf_code=0x%04x",
					 cmd_info->manuf_specific);
		} else {
			status =
				snprintf(log_message_curr,
					 log_message_end - log_message_curr,
					 " manuf_code=void");
		}
		PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);

		status = snprintf(log_message_curr,
				  log_message_end - log_message_curr,
				  " payload=[");
		PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);

		status = to_hex_str(log_message_curr,
				    log_message_end - log_message_curr,
				    payload, payload_length, false);
		PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);

		/* Put again log_counter to be able to simple check
		 * log consistency
		 */
		status = snprintf(log_message_curr,
				  log_message_end - log_message_curr,
				  "] (%" PRIu32 ")", log_number);
		PRV_ADVANCE_CURR_LOG_MESSAGE_PTR(status);

		*log_message_curr = '\0';

		LOG_INST_INF(logger.inst, "%s", log_message);
	}

	return ZB_FALSE;
}
