/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zboss_api.h>

#if defined ZB_NRF_TRACE

#include <logging/log.h>

LOG_MODULE_REGISTER(zboss, LOG_LEVEL_DBG);

void zb_osif_serial_init(void)
{
}

static zb_uint8_t buf8[8];
static zb_uint_t buffered;

void zb_osif_serial_put_bytes(zb_uint8_t *buf, zb_short_t len)
{
	if (IS_ENABLED(CONFIG_ZBOSS_TRACE_LOG_LEVEL_OFF)) {
		return;
	}

	/* Try to fill hex dump by 8-bytes dumps */

	while (len) {
		zb_int_t n = 8 - buffered;

		if (n > len) {
			n = len;
		}
		ZB_MEMCPY(buf8 + buffered, buf, n);
		buffered += n;
		buf += n;
		len -= n;
		if (buffered == 8) {
			LOG_HEXDUMP_DBG(buf8, 8, "");
			buffered = 0;
		}
	} /* while */
}

#ifdef CONFIG_ZB_NRF_TRACE_RX_ENABLE
/*Function set UART RX callback function*/
void zb_osif_set_uart_byte_received_cb(zb_callback_t cb)
{
	LOG_ERR("Command reception is not available through Zephyr's logger");
}
#endif /*CONFIG_ZB_NRF_TRACE_RX_ENABLE*/

void zb_osif_serial_flush(void)
{
	if (buffered) {
		LOG_HEXDUMP_DBG(buf8, buffered, "");
		buffered = 0;
	}
}

#endif /* defined ZB_NRF_TRACE */
