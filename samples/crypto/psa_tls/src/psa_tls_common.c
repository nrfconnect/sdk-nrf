/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>

/** @brief Function for sending all data in a given buffer to a connected
 * socket.
 *
 * @param   sock        The socket identifier to send data to.
 * @param   buf         Pointer to the data buffer.
 * @param   len         Number of bytes to send from buf.
 *
 * @returns 0 on success.
 */
int psa_tls_send_buffer(int sock, const uint8_t *buf, size_t len)
{
	ssize_t sent_len;

	while (len > 0) {
		sent_len = send(sock, buf, len, 0);
		if (sent_len < 0) {
			return -errno;
		}

		buf += sent_len;
		len -= sent_len;
	}

	return 0;
}
