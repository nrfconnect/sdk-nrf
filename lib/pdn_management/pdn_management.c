/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <net/socket.h>

int pdn_init_and_connect(const char *apn_name)
{
	int pdn_fd = socket(AF_LTE, SOCK_MGMT, NPROTO_PDN);

	if (pdn_fd >= 0) {
		/* Connect to the APN. */
		int err = connect(pdn_fd,
				  (struct sockaddr *)apn_name,
				  strlen(apn_name));

		if (err != 0) {
			close(pdn_fd);
			return -1;
		}
	}

	return pdn_fd;
}


void pdn_disconnect(int pdn_fd)
{
	close(pdn_fd);
}
