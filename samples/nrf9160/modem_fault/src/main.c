/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <net/socket.h>
#include <net/socket.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <nrf_modem.h>
#include <nrf_socket.h>

LOG_MODULE_REGISTER(app);

#define HTTP_HOSTNAME "speedtest.ftp.otenet.gr"
#define HTTP_PORT "80"
#define HTTP_GET                                                                                   \
	"GET /files/test100k.db HTTP/1.1\r\n"                                                      \
	"Host: speedtest.ftp.otenet.gr\r\n"                                                        \
	"Connection: close\r\n"                                                                    \
	"\r\n"

#define HTTP_GET_LEN (sizeof(HTTP_GET) - 1)
#define HTTP_HDR_END "\r\n\r\n"

static const char send_buf[] = HTTP_GET;
static char recv_buf[2048];

/* Modem library initialization semaphore */
static K_SEM_DEFINE(modem_sem, 0, 1);
/* Network registration semaphore, given by main thread */
static K_SEM_DEFINE(cereg_sem, 0, 1);

/* Workqueue task crashing the modem by shutting it down
 * while a download is in progress, without first de-registring from the network.
 *
 * In "normal" crash scenarions, the modem library remains initialized
 * until it is shutdown by the default fault handler or by the application.
 * As long as it remains initialized, it is possible to read out any
 * pending network data that was delivered by modem before it crashed.
 */
static void crash_task(struct k_work *unused)
{
	(void) nrf_modem_shutdown();
}

static K_WORK_DELAYABLE_DEFINE(crash_work, crash_task);

/* Modem library initialization hook */
NRF_MODEM_LIB_ON_INIT(on_init, on_modem_init, NULL);

static void on_modem_init(int err, void *ctx)
{
	if (err) {
		LOG_INF("on_init: initialization failed, err %d", err);
		return;
	}

	LOG_INF("on_init: modem library initialized");

	/* Let main thread continue and register the device
	 * to the LTE network upon modem re-initialization.
	 */
	k_sem_give(&modem_sem);
}

static void network_thread(void *p1, void *p2, void *p3)
{
	int err;
	int fd;
	size_t off;
	ssize_t bytes;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP,
	};

	bool has_faulted = false;

wait_for_cereg:
	LOG_INF("net: waiting for network registration");
	k_sem_take(&cereg_sem, K_FOREVER);

	err = getaddrinfo(HTTP_HOSTNAME, HTTP_PORT, &hints, &res);
	if (err) {
		LOG_ERR("net: getaddrinfo() failed, err %d", errno);
		return;
	}

	fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fd == -1) {
		LOG_ERR("net: failed to open socket!");
		return;
	}

	LOG_INF("net: connecting to %s", HTTP_HOSTNAME);
	err = connect(fd, res->ai_addr, res->ai_addrlen);
	if (err) {
		LOG_ERR("net: connect() failed, err: %d", errno);
		return;
	}

	(void)freeaddrinfo(res);

	off = 0;
	do {
		bytes = send(fd, &send_buf[off], HTTP_GET_LEN - off, 0);
		if (bytes < 0) {
			LOG_ERR("net: send() failed, err %d", errno);
			return;
		}
		off += bytes;
	} while (off < HTTP_GET_LEN);

	LOG_INF("net: sent %d bytes", off);

	/* Schedule a crash during the download */
	if (!has_faulted) {
		has_faulted = true;
		LOG_INF("net: crashing in two seconds");
		k_work_schedule(&crash_work, K_SECONDS(2));
	}

	LOG_INF("net: downloading..");
	off = 0;
	do {
		bytes = recv(fd, &recv_buf, sizeof(recv_buf), 0);
		if (bytes < 0) {
			LOG_WRN("net: recv() failed, err %d %s", errno, strerror(errno));
			(void)close(fd);
			/* Loop and wait for CEREG */
			goto wait_for_cereg;
		}
		if ((off + bytes) / sizeof(recv_buf) > (off / sizeof(recv_buf))) {
			LOG_INF("net: downloaded %d bytes", off + bytes);
		}
		off += bytes;
	} while (bytes != 0 /* peer closed connection */);

	LOG_INF("net: received %d bytes", off);
	LOG_INF("net: bye");

	(void)close(fd);
	(void)lte_lc_power_off();
}

/* Networking thread, retrieving a file from an HTTP server */
K_THREAD_DEFINE(networking, KB(1), network_thread, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

void main(void)
{
	int err;

	LOG_INF("main: modem fault sample started");

	/* In this sample, the application main thread:
	 * - initializes modem library and
	 * - registers the device to the network
	 */

	/* Modem library is initialized manually once, on boot.
	 * When the modem crashes, the fault handler
	 * will automatically re-initialize the library if
	 * CONFIG_NRF_MODEM_LIB_ON_FAULT_RESET_MODEM is set.
	 */
	LOG_INF("main: initializing modem library");
	err = nrf_modem_lib_init(NORMAL_MODE);
	if (err) {
		LOG_INF("main: failed to initialize, err %d", err);
		return;
	}

	while (true) {
		/* Wait forever for modem library to become
		 * re-initialized after a fault, or other reason.
		 */
		k_sem_take(&modem_sem, K_FOREVER);

		LOG_INF("main: registering to network.. ");
		err = lte_lc_init_and_connect();
		if (err) {
			LOG_INF("main: failed to connect to the LTE network, err %d", err);
			return;
		}

		LOG_INF("main: registered");
		/* Let the networking thread run */
		k_sem_give(&cereg_sem);
	}
}
