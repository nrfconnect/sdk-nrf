/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <init.h>

#if defined(CONFIG_BSD_LIBRARY) && defined(CONFIG_NET_SOCKETS_OFFLOAD)
#include <net/socket.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(board_nonsecure, CONFIG_BOARD_LOG_LEVEL);

#define AT_CMD_MAX_READ_LENGTH	128
#define AT_CMD_MAGPIO		"AT%XMAGPIO=1,1,1,7,1,746,803,2,698,748," \
				"2,1710,2200,3,824,894,4,880,960,5,791,849," \
				"7,1574,1577"
#define AT_CMD_MAGPIO_LEN	sizeof (AT_CMD_MAGPIO) - 1

static int pca20035_magpio_configure(void)
{
#if defined(CONFIG_BSD_LIBRARY) && defined(CONFIG_NET_SOCKETS_OFFLOAD)
	int at_socket_fd;
	int buffer;
	u8_t read_buffer[AT_CMD_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		LOG_ERR("AT socket could not be opened");
		return -EFAULT;
	}

	LOG_DBG("AT CMD: %s", AT_CMD_MAGPIO);
	buffer = send(at_socket_fd, AT_CMD_MAGPIO, AT_CMD_MAGPIO_LEN, 0);
	if (buffer != AT_CMD_MAGPIO_LEN) {
		LOG_ERR("Sending MAGPIO command failed");
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, AT_CMD_MAX_READ_LENGTH, 0);
	LOG_DBG("AT RESP: %s", read_buffer);
	if ((buffer < 2) ||
	    (memcmp("OK", read_buffer, 2 != 0))) {
		LOG_ERR("MAGPIO command failed");
		close(at_socket_fd);
		return -EIO;
	}

	LOG_DBG("MAGPIO successfully configured");

	close(at_socket_fd);
#endif
	return 0;
}

static int pca20035_board_init(struct device *dev)
{
	int err;

	err = pca20035_magpio_configure();
	if (err) {
		LOG_ERR("pca20035_magpio_configure failed with error: %d", err);
		return err;
	}

	return 0;
}

SYS_INIT(pca20035_board_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
