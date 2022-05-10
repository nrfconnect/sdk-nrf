/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zboss_api.h>
#include <zephyr/sys/ring_buffer.h>

#if defined ZB_NRF_TRACE

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zboss_osif, CONFIG_ZBOSS_OSIF_LOG_LEVEL);

RING_BUF_DECLARE(logger_buf, CONFIG_ZBOSS_TRACE_LOGGER_BUFFER_SIZE);

void zb_osif_logger_put_bytes(const zb_uint8_t *buf, zb_short_t len)
{
	zb_uint8_t *buf_dest = NULL;
	zb_uint32_t allocated = 0;

	if (IS_ENABLED(CONFIG_ZBOSS_TRACE_LOG_LEVEL_OFF)) {
		return;
	}

	allocated = ring_buf_put_claim(&logger_buf, &buf_dest, len);
	while (allocated != 0) {
		ZB_MEMCPY(buf_dest, buf, allocated);
		ring_buf_put_finish(&logger_buf, allocated);
		len -= allocated;
		buf += allocated;
		allocated = ring_buf_put_claim(&logger_buf, &buf_dest, len);
	}

	if (len) {
		LOG_DBG("Dropping %u bytes, ring buffer is full", len);
	}
}

/* Is called when complete Trace message is put in the ring buffer.
 * Triggers sending buffered data through UART API.
 */
void zb_trace_msg_port_do(void)
{
	zb_uint32_t data_len;
	int ret_val;
	zb_uint8_t *data_ptr = NULL;

	if (IS_ENABLED(CONFIG_ZBOSS_TRACE_LOG_LEVEL_OFF)) {
		return;
	}

	if (ring_buf_is_empty(&logger_buf)) {
		return;
	}

	data_len = ring_buf_get_claim(&logger_buf,
				      &data_ptr,
				      CONFIG_ZBOSS_TRACE_LOGGER_BUFFER_SIZE);

	LOG_HEXDUMP_DBG(data_ptr, data_len, "");

	ret_val = ring_buf_get_finish(&logger_buf, data_len);

	if (ret_val != 0) {
		LOG_ERR("%u exceeds valid bytes in the logger ring buffer", data_len);
	}
}

/* Prints remaining bytes. */
void zb_osif_logger_flush(void)
{
	while (!ring_buf_is_empty(&logger_buf)) {
		zb_trace_msg_port_do();
	}
}

#if defined(CONFIG_ZB_NRF_TRACE_RX_ENABLE)
/* Function set UART RX callback function */
void zb_osif_logger_set_uart_byte_received_cb(zb_callback_t cb)
{
	LOG_ERR("Command reception is not available through logger");
}
#endif /* CONFIG_ZB_NRF_TRACE_RX_ENABLE */

#endif /* defined ZB_NRF_TRACE */
