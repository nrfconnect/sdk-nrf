/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <logging/log.h>
#include <net/socket.h>
#include <nrf_socket.h>
#include <device.h>
#include <drivers/gps.h>

#if defined(CONFIG_AGPS_SRC_SUPL)
#include <supl_session.h>
#include <supl_os_client.h>
#elif defined(CONFIG_AGPS_SRC_NRF_CLOUD)
#include <net/nrf_cloud_agps.h>
#endif

LOG_MODULE_REGISTER(agps, CONFIG_AGPS_LOG_LEVEL);

#if defined(CONFIG_AGPS_SRC_SUPL)
/* Number of DNS lookup attempts */
#define DNS_ATTEMPT_COUNT  3

static const struct device *gps_dev;
static int gnss_fd;
static int supl_fd;
#endif /* CONFIG_AGPS_SRC_SUPL */

static enum gps_agps_type type_lookup_socket2gps[] = {
	[NRF_GNSS_AGPS_UTC_PARAMETERS]	= GPS_AGPS_UTC_PARAMETERS,
	[NRF_GNSS_AGPS_EPHEMERIDES]	= GPS_AGPS_EPHEMERIDES,
	[NRF_GNSS_AGPS_ALMANAC]		= GPS_AGPS_ALMANAC,
	[NRF_GNSS_AGPS_KLOBUCHAR_IONOSPHERIC_CORRECTION]
					= GPS_AGPS_KLOBUCHAR_CORRECTION,
	[NRF_GNSS_AGPS_NEQUICK_IONOSPHERIC_CORRECTION]
					= GPS_AGPS_NEQUICK_CORRECTION,
	[NRF_GNSS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS]
					= GPS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS,
	[NRF_GNSS_AGPS_LOCATION]	= GPS_AGPS_LOCATION,
	[NRF_GNSS_AGPS_INTEGRITY]	= GPS_AGPS_INTEGRITY,
};

/* Convert nrf_socket A-GPS type to GPS API type. */
static inline enum gps_agps_type type_socket2gps(nrf_gnss_agps_data_type_t type)
{
	return type_lookup_socket2gps[type];
}

#if defined(CONFIG_AGPS_SRC_SUPL)
static int send_to_modem(void *data, size_t data_len,
			 nrf_gnss_agps_data_type_t type)
{
	int err;

	/* At this point, GPS driver or app-provided socket is assumed. */
	if (gps_dev) {
		return gps_agps_write(gps_dev, type_socket2gps(type), data,
				      data_len);
	}

	err = nrf_sendto(gnss_fd, data, data_len, 0, &type, sizeof(type));
	if (err < 0) {
		LOG_ERR("Failed to send AGPS data to modem, errno: %d", errno);
		err = -errno;
	} else {
		err = 0;
	}

	LOG_DBG("A-GPS data sent to modem");

	return err;
}

static int inject_agps_type(void *agps,
			    size_t agps_size,
			    nrf_gnss_agps_data_type_t type,
			    void *user_data)
{
	ARG_UNUSED(user_data);
	int err;

	err = send_to_modem(agps, agps_size, type);
	if (err) {
		LOG_ERR("Failed to send AGNSS data, type: %d (err: %d)",
			type, err);
		return err;
	}

	LOG_DBG("Injected AGPS data, type: %d, size: %d", type, agps_size);

	return 0;
}

static int open_supl_socket(void)
{
	int err;
	int proto;
	uint16_t port;
	struct addrinfo *addr;
	struct addrinfo *info;

	proto = IPPROTO_TCP;
	port = htons(CONFIG_AGPS_SUPL_PORT);

	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = proto,
		/* Either a valid,
		 * NULL-terminated access point name or NULL.
		 */
		.ai_canonname = NULL,
	};

	err = getaddrinfo(CONFIG_AGPS_SUPL_HOST_NAME, NULL, &hints,
			  &info);

	if (err) {
		LOG_ERR("Failed to resolve IPv4 hostname %s, errno: %d",
			CONFIG_AGPS_SUPL_HOST_NAME, err);
		return -ECHILD;
	}

	/* Create socket */
	supl_fd = socket(AF_INET, SOCK_STREAM, proto);
	if (supl_fd < 0) {
		LOG_ERR("Failed to create socket, errno %d", errno);
		err = -errno;
		goto cleanup;
	}

	struct timeval timeout = {
		.tv_sec = 1,
		.tv_usec = 0,
	};

	err = setsockopt(supl_fd,
			 SOL_SOCKET,
			 SO_RCVTIMEO,
			 &timeout,
			 sizeof(timeout));
	if (err) {
		LOG_ERR("Failed to setup socket timeout, errno %d", errno);
		err = -errno;
		goto cleanup;
	}

	/* Not connected */
	err = -1;

	for (addr = info; addr != NULL; addr = addr->ai_next) {
		struct sockaddr *const sa = addr->ai_addr;

		switch (sa->sa_family) {
		case AF_INET6:
			continue;
		case AF_INET:
			((struct sockaddr_in *)sa)->sin_port = port;
			char ip[INET_ADDRSTRLEN];

			inet_ntop(AF_INET,
				  &((struct sockaddr_in *)sa)->sin_addr, ip,
				  sizeof(ip));

			LOG_DBG("ip %s (%x) port %d", log_strdup(ip),
				((struct sockaddr_in *)sa)->sin_addr.s_addr,
				ntohs(port));
			break;
		}

		err = connect(supl_fd, sa, addr->ai_addrlen);
		if (err) {
			/* Try next address */
			LOG_ERR("Unable to connect, errno %d", errno);
			err = -errno;
		} else {
			/* Connected */
			break;
		}
	}

cleanup:
	freeaddrinfo(info);

	if (err) {
		/* Unable to connect, close socket */
		close(supl_fd);
		supl_fd = -1;
	}

	return err;
}

static void close_supl_socket(void)
{
	if (close(supl_fd) < 0) {
		LOG_ERR("Failed to close SUPL socket");
	}
}

static ssize_t supl_write(const void *buf, size_t nbytes, void *user_data)
{
	ARG_UNUSED(user_data);

	return send(supl_fd, buf, nbytes, 0);
}

static int supl_logger(int level, const char *fmt, ...)
{
	char buffer[256];
	va_list args;
	int ret;

	va_start(args, fmt);

	ret = vsnprintk(buffer, sizeof(buffer), fmt, args);

	va_end(args);

	if (ret < 0) {
		LOG_ERR("%s: encoding error", __func__);
		return ret;
	} else if ((size_t)ret >= sizeof(buffer)) {
		LOG_ERR("%s: too long message, it will be cut short",
			__func__);
	}

	LOG_DBG("%s", log_strdup(buffer));

	return ret;
}

static ssize_t supl_read(void *buf, size_t nbytes, void *user_data)
{
	ARG_UNUSED(user_data);
	ssize_t rc = recv(supl_fd, buf, nbytes, 0);

	if (rc < 0 && (errno == ETIMEDOUT)) {
		return 0;
	}

	return rc;
}

static int init_supl(int socket)
{
	int err;
	struct supl_api supl_api = {
		.read       = supl_read,
		.write      = supl_write,
		.handler    = inject_agps_type,
		.logger     = supl_logger,
		.counter_ms = k_uptime_get
	};

	err = supl_init(&supl_api);
	if (err) {
		LOG_ERR("Failed to initialize SUPL library, error: %d", err);
		return err;
	}

	if (socket) {
		LOG_DBG("Using user-provided socket, fd %d", socket);

		gps_dev = NULL;
		gnss_fd = socket;
	} else {
		gps_dev = device_get_binding("NRF9160_GPS");
		if (gps_dev == NULL) {
			LOG_ERR("Could not get binding to nRF9160 GPS");
			return -ENODEV;
		}

		LOG_DBG("Using GPS driver to input assistance data");
	}

	LOG_INF("SUPL is initialized");

	return 0;
}

static int supl_start(const struct gps_agps_request request)
{
	int err;
	nrf_gnss_agps_data_frame_t req = {
		.sv_mask_ephe = request.sv_mask_ephe,
		.sv_mask_alm = request.sv_mask_alm,
		.data_flags =
			(request.utc ?
			BIT(NRF_GNSS_AGPS_GPS_UTC_REQUEST) : 0) |
			(request.klobuchar ?
			BIT(NRF_GNSS_AGPS_KLOBUCHAR_REQUEST) : 0) |
			(request.nequick ?
			BIT(NRF_GNSS_AGPS_NEQUICK_REQUEST) : 0) |
			(request.system_time_tow ?
			BIT(NRF_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) : 0) |
			(request.position ?
			BIT(NRF_GNSS_AGPS_POSITION_REQUEST) : 0) |
			(request.integrity ?
			BIT(NRF_GNSS_AGPS_INTEGRITY_REQUEST) : 0),
	};

	err = open_supl_socket();
	if (err) {
		LOG_ERR("Failed to open SUPL socket");
		return err;
	}

	LOG_INF("Starting SUPL session");

	err = supl_session(&req);
	if (err) {
		LOG_ERR("SUPL session failed, error: %d", err);
		return err;
	}

	LOG_INF("SUPL session finished successfully");

	close_supl_socket();

	return err;
}

#endif /* CONFIG_AGPS_SRC_SUPL */

int gps_agps_request(struct gps_agps_request request, int socket)
{
	int err;

#if defined(CONFIG_AGPS_SRC_SUPL)
	static bool supl_is_init;

	if (!supl_is_init) {
		err = init_supl(socket);
		if (err) {
			LOG_ERR("SUPL initialization failed, error: %d", err);
			return err;
		}

		supl_is_init = true;
	}

	err = supl_start(request);
	if (err) {
		LOG_ERR("SUPL request failed, error: %d", err);
		return err;
	}

#elif defined(CONFIG_AGPS_SRC_NRF_CLOUD)
	err = nrf_cloud_agps_request(request);
	if (err) {
		LOG_ERR("nRF Cloud A-GPS request failed, error: %d", err);
		return err;
	}
#endif /* CONFIG_AGPS_SRC_SUPL */

	return 0;
}

int gps_process_agps_data(const uint8_t *buf, size_t len)
{
	int err = 0;

#if defined(CONFIG_AGPS_SRC_NRF_CLOUD) && defined(CONFIG_NRF_CLOUD_AGPS)

	err = nrf_cloud_agps_process(buf, len, NULL);
	if (err) {
		LOG_ERR("A-GPS failed, error: %d", err);
	} else {
		LOG_INF("A-GPS data successfully processed");
	}
#endif /* CONFIG_AGPS_SRC_NRF_CLOUD && CONFIG_NRF_CLOUD_AGPS */

	return err;
}
