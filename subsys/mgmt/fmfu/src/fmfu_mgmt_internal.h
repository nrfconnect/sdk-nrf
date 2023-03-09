/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FMFU_MGMT_INTERNAL_H__
#define FMFU_MGMT_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Define full modem update MCUMgr commands */
#define FMFU_MGMT_ID_GET_HASH 0
#define FMFU_MGMT_ID_UPLOAD 1

/*
 * Calculate the MTU through buffer size of mcumgr and subtracting overhead
 * of the RX buffer.
 */
#define SMP_PACKET_MTU\
	(CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE - \
	((CONFIG_UART_MCUMGR_RX_BUF_SIZE/CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE) + 1) * 4)
/*
 * Define the buffer sizes used for full modem update SMP server
 * SMP uart buffer is the same as the buffer configured as the RX buffer for
 * MCUMgr
 */
#define SMP_UART_BUFFER_SIZE CONFIG_UART_MCUMGR_RX_BUF_SIZE

#ifdef __cplusplus
}
#endif

#endif /* FMFU_MGMT_INTERNAL_H__ */
