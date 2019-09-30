/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stddef.h>
#include <net/socket.h>
#include <nrf_socket.h>

int pdn_activate(int *fd, const char *apn)
{
	int err;
	nrf_pdn_state_t active = 0;
	nrf_socklen_t len = sizeof(active);

	if (fd == NULL || apn == NULL) {
		return -1;
	}

	if (*fd != -1) {
		/* If handle is valid, check if PDN is active */
		err = getsockopt(*fd, SOL_PDN, SO_PDN_STATE, &active, &len);
		if (err) {
			return -1;
		}

		if (active) {
			return 0;
		}

		/* PDN is not active, close socket and reactivate it */
		close(*fd);
	}

	*fd = socket(AF_LTE, SOCK_MGMT, NPROTO_PDN);
	if (*fd < 0) {
		return -1;
	}

	/* Connect to the PDN. */
	err = connect(*fd, (struct sockaddr *)apn, strlen(apn));
	if (err) {
		close(*fd);
		return -1;
	}

	/* PDN is active, but fd might have changed */
	return 1;
}
