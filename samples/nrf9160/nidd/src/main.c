/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>

K_SEM_DEFINE(lte_connected, 0, 1);

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		    (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		printk("Network registration status: %s\n",
		       evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
		       "Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		printk("RRC mode: %s\n",
		       evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
		       "Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		printk("LTE cell changed: Cell ID: %d, Tracking area: %d\n",
		       evt->cell.id, evt->cell.tac);
		break;
	case LTE_LC_EVT_MODEM_EVENT:
		printk("Modem event: %d\n", evt->modem_evt);
		break;
	default:
		break;
	}
}

static void modem_init(void)
{
	int err;

	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library initialization failed, error: %d\n", err);
		return;
	}

	err = lte_lc_init();
	if (err) {
		printk("Modem LTE connection initialization failed, error: %d\n", err);
		return;
	}

	(void)nrf_modem_at_printf("AT+CMEE=1");
}

static void modem_connect(void)
{
	int err = lte_lc_connect_async(lte_handler);

	if (err) {
		printk("Connecting to LTE network failed, error: %d\n", err);
		return;
	}
}

static void modem_power_off(void)
{
	lte_lc_power_off();
}

static int nidd_pdn_setup(void)
{
	int cid = 0;
	int err;

	if (IS_ENABLED(CONFIG_NIDD_ALLOC_NEW_CID)) {
		err = nrf_modem_at_scanf("AT%XNEWCID?", "%%XNEWCID: %d", &cid);
		if (err == 1) {
			printf("Created CID %d\n", cid);
		} else {
			printf("Create CID failed, error: %d\n", err);
			return -1;
		}
	}

	err = nrf_modem_at_printf("AT+CGDCONT=%d,\"Non-IP\",\"%s\"", cid, CONFIG_NIDD_APN);
	if (err == 0) {
		printk("Configured Non-IP for APN \"%s\"\n", CONFIG_NIDD_APN);
	} else {
		printf("Configure Non-IP failed, type: %d, error: %d\n",
		       nrf_modem_at_err_type(err), nrf_modem_at_err(err));
		return -1;
	}

	return cid;
}

static int nidd_pdn_activate(int cid)
{
	char xgetpdnid[18];
	int pdn_id;
	int err;

	if (IS_ENABLED(CONFIG_NIDD_ALLOC_NEW_CID)) {
		err = nrf_modem_at_printf("AT+CGACT=1,%d", cid);
		if (err == 0) {
			printk("Activated CID %d\n", cid);
		} else {
			printk("Activate CID failed, type: %d, error: %d\n",
			       nrf_modem_at_err_type(err), nrf_modem_at_err(err));
			return -1;
		}
	}

	(void)snprintf(xgetpdnid, sizeof(xgetpdnid), "AT%%XGETPDNID=%u", cid);
	err = nrf_modem_at_scanf(xgetpdnid, "%%XGETPDNID: %d", &pdn_id);
	if (err == 1) {
		printk("Get PDN ID %d\n", pdn_id);
	} else {
		printk("Get PDN ID failed, error: %d\n", err);
		return -1;
	}

	return pdn_id;
}

static void nidd_socket_close(int fd)
{
	int err;

	err = close(fd);
	if (err == 0) {
		printk("Closed socket %d\n", fd);
	} else {
		printk("Close socket failed, error: %d, errno: %d\n", err, errno);
	}
}

static int nidd_socket_setup(int pdn_id)
{
	char pdn[8];
	int fd, err;

	fd = socket(AF_PACKET, SOCK_RAW, 0);
	if (fd >= 0) {
		printk("Created socket %d\n", fd);
	} else {
		printk("Create socket failed, error: %d, errno: %d\n", fd, errno);
		return -1;
	}

	if (IS_ENABLED(CONFIG_NIDD_ALLOC_NEW_CID)) {
		(void)snprintf(pdn, sizeof(pdn), "pdn%d", pdn_id);
		err = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, pdn, strlen(pdn));
		if (err == 0) {
			printk("Bound to PDN ID %d\n", pdn_id);
		} else {
			printk("Bind to PDN failed, error: %d, errno: %d\n", err, errno);
			goto error_close_socket;
		}
	}

	struct timeval recv_timeout = {
		.tv_sec = 5
	};

	err = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
	if (err != 0) {
		printk("Set receive timeout failed, error: %d, errno: %d\n", err, errno);
		goto error_close_socket;
	}

	return fd;

error_close_socket:
	nidd_socket_close(fd);

	return -1;
}

static void nidd_send_and_recv(int fd)
{
	char buffer[256];
	ssize_t len;

	len = send(fd, CONFIG_NIDD_PAYLOAD, sizeof(CONFIG_NIDD_PAYLOAD)-1, 0);
	if (len == sizeof(CONFIG_NIDD_PAYLOAD)-1) {
		printk("Sent %d bytes\n", len);
	} else {
		printk("Send failed, error: %d, errno: %d\n", len, errno);
		return;
	}

	len = recv(fd, buffer, sizeof(buffer), 0);
	if (len > 0) {
		printk("Received %d bytes: %s\n", len, buffer);
	} else if (len < 0) {
		printk("Receive failed, error: %d, errno: %d\n", len, errno);
		return;
	}
}

void main(void)
{
	int cid, pdn_id, fd;

	printk("NIDD sample started\n");
	modem_init();

	cid = nidd_pdn_setup();
	if (cid == -1) {
		goto power_off;
	}

	modem_connect();
	k_sem_take(&lte_connected, K_FOREVER);

	pdn_id = nidd_pdn_activate(cid);
	if (pdn_id == -1) {
		goto power_off;
	}

	fd = nidd_socket_setup(pdn_id);
	if (fd == -1) {
		goto power_off;
	}

	nidd_send_and_recv(fd);
	nidd_socket_close(fd);

power_off:
	modem_power_off();
	printk("NIDD sample done\n");
}
