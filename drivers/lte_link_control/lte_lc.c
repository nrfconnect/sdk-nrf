/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <zephyr/types.h>
#include <errno.h>
#include <net/socket.h>
#include <string.h>
#include <stdio.h>
#include <device.h>

#define LC_MAX_READ_LENGTH 128
#if defined(CONFIG_LTE_AUTO_INIT_AND_CONNECT)
DEVICE_DECLARE(lte_link_control);
#endif

/* Subscribes to notifications with level 2 */
static const char subscribe[] = { 'A', 'T', '+', 'C', 'E',
				  'R', 'E', 'G', '=', '2' };
/* Set the modem to power off mode */
static const char power_off[] = { 'A', 'T', '+', 'C', 'F', 'U', 'N', '=', '0' };
/* Set the modem to Normal mode */
static const char normal[] = { 'A', 'T', '+', 'C', 'F', 'U', 'N', '=', '1' };
/* Set the modem to Offline mode */
static const char offline[] = { 'A', 'T', '+', 'C', 'F', 'U', 'N', '=', '4' };
/* Successful return from modem */
static const char success[] = { 'O', 'K' };
/* Network status read from modem */
static const char *status[4] = {
	"+CEREG: 1",
	"+CEREG:1",
	"+CEREG: 5",
	"+CEREG:5",
};

static int w_lte_lc_init_and_connect(struct device *unused)
{
	int at_socket_fd;
	int buffer;
	u8_t read_buffer[LC_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}

	buffer = send(at_socket_fd, subscribe, sizeof(subscribe), 0);
	if (buffer != sizeof(subscribe)) {
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, sizeof(read_buffer), 0);
	if ((buffer < sizeof(success)) ||
	    (memcmp(success, read_buffer, sizeof(success)) != 0)) {
		close(at_socket_fd);
		return -EIO;
	}
	buffer = send(at_socket_fd, normal, sizeof(normal), 0);
	if (buffer != sizeof(normal)) {
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, sizeof(read_buffer), 0);
	if ((buffer < sizeof(success)) ||
	    (memcmp(success, read_buffer, sizeof(success)) != 0)) {
		close(at_socket_fd);
		return -EIO;
	}

	while (true) {
		buffer = recv(at_socket_fd, read_buffer,
				  sizeof(read_buffer), MSG_DONTWAIT);
		if ((memcmp(status[0], read_buffer, strlen(status[0])) == 0) ||
		    (memcmp(status[1], read_buffer, strlen(status[1])) == 0) ||
		    (memcmp(status[2], read_buffer, strlen(status[2])) == 0) ||
		    (memcmp(status[3], read_buffer, strlen(status[3])) == 0)) {
			break;
		}
	}
	close(at_socket_fd);
	return 0;
}

/* lte lc Init and connect wrapper */
int lte_lc_init_and_connect(void)
{
	struct device *x = 0;

	int err = w_lte_lc_init_and_connect(x);

	return err;
}

int lte_lc_offline(void)
{
	int at_socket_fd;
	int buffer;
	u8_t read_buffer[LC_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}
	buffer = send(at_socket_fd, offline, sizeof(offline), 0);
	if (buffer != sizeof(offline)) {
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, LC_MAX_READ_LENGTH, 0);
	if ((buffer < sizeof(success)) ||
	    (memcmp(success, read_buffer, sizeof(success) != 0))) {
		close(at_socket_fd);
		return -EIO;
	}

	close(at_socket_fd);
	return 0;
}

int lte_lc_power_off(void)
{
	int at_socket_fd;
	int buffer;
	u8_t read_buffer[LC_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}
	buffer = send(at_socket_fd, power_off, sizeof(power_off), 0);
	if (buffer != sizeof(power_off)) {
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, LC_MAX_READ_LENGTH, 0);
	if ((buffer < sizeof(success)) ||
	    (memcmp(success, read_buffer, sizeof(success)) != 0)) {
		close(at_socket_fd);
		return -EIO;
	}
	close(at_socket_fd);
	return 0;
}

int lte_lc_normal(void)
{
	int at_socket_fd;
	int buffer;
	u8_t read_buffer[LC_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}
	buffer = send(at_socket_fd, normal, sizeof(normal), 0);
	if (buffer != sizeof(normal)) {
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, LC_MAX_READ_LENGTH, 0);
	if ((buffer < sizeof(success)) ||
	    (memcmp(success, read_buffer, sizeof(success)) != 0)) {
		close(at_socket_fd);
		return -EIO;
	}
	close(at_socket_fd);
	return 0;
}

#if defined(CONFIG_LTE_AUTO_INIT_AND_CONNECT)
DEVICE_AND_API_INIT(lte_link_control, "LTE_LINK_CONTROL",
		    w_lte_lc_init_and_connect, NULL, NULL, APPLICATION,
		    CONFIG_APPLICATION_INIT_PRIORITY, NULL);
#endif /* CONFIG_LTE_AUTO_INIT_AND_CONNECT */
