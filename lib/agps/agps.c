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
#include <nrf_modem_gnss.h>
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
static const struct device *gps_dev;
static int gnss_fd = -1;
static int supl_fd = -1;
#endif /* CONFIG_AGPS_SRC_SUPL */

static enum gps_agps_type type_lookup_api2driver[] = {
	[NRF_MODEM_GNSS_AGPS_UTC_PARAMETERS] =
		GPS_AGPS_UTC_PARAMETERS,
	[NRF_MODEM_GNSS_AGPS_EPHEMERIDES] =
		GPS_AGPS_EPHEMERIDES,
	[NRF_MODEM_GNSS_AGPS_ALMANAC] =
		GPS_AGPS_ALMANAC,
	[NRF_MODEM_GNSS_AGPS_KLOBUCHAR_IONOSPHERIC_CORRECTION] =
		GPS_AGPS_KLOBUCHAR_CORRECTION,
	[NRF_MODEM_GNSS_AGPS_NEQUICK_IONOSPHERIC_CORRECTION] =
		GPS_AGPS_NEQUICK_CORRECTION,
	[NRF_MODEM_GNSS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS] =
		GPS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS,
	[NRF_MODEM_GNSS_AGPS_LOCATION] =
		GPS_AGPS_LOCATION,
	[NRF_MODEM_GNSS_AGPS_INTEGRITY]	=
		GPS_AGPS_INTEGRITY
};

/* Convert GNSS API A-GPS type to GPS driver type. */
static inline enum gps_agps_type type_api2driver(uint16_t type)
{
	return type_lookup_api2driver[type];
}

#if defined(CONFIG_AGPS_SRC_SUPL)
static int send_to_modem(void *data, size_t data_len, uint16_t type)
{
	int err;

	if (gps_dev) {
		/* GPS driver */
		err = gps_agps_write(gps_dev, type_api2driver(type), data, data_len);
	} else if (gnss_fd != -1) {
		/* GNSS socket */
		err = nrf_sendto(gnss_fd, data, data_len, 0, &type, sizeof(type));
		if (err < 0) {
			err = -errno;
		} else {
			err = 0;
		}
	} else {
		/* GNSS API */
		err = nrf_modem_gnss_agps_write(data, data_len, type);
	}

	return err;
}

static int inject_agps_type(void *agps,
			    size_t agps_size,
			    uint16_t type,
			    void *user_data)
{
	ARG_UNUSED(user_data);
	int err;

	err = send_to_modem(agps, agps_size, type);
	if (err) {
		LOG_ERR("Failed to send A-GPS data, type: %d (err: %d)",
			type, err);
		return err;
	}

	LOG_DBG("Injected A-GPS data, type: %d, size: %d", type, agps_size);

	return 0;
}

static int open_supl_socket(void)
{
	int err;
	struct addrinfo *info;

	struct addrinfo hints = {
		.ai_family = AF_UNSPEC, /* Both IPv4 and IPv6 addresses accepted. */
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(CONFIG_AGPS_SUPL_HOST_NAME, NULL, &hints, &info);
	if (err) {
		LOG_ERR("Failed to resolve hostname %s, error: %d",
			CONFIG_AGPS_SUPL_HOST_NAME, err);

		return -1;
	}

	/* Not connected */
	err = -1;

	for (struct addrinfo *addr = info; addr != NULL; addr = addr->ai_next) {
		char ip[INET6_ADDRSTRLEN] = { 0 };
		struct sockaddr *const sa = addr->ai_addr;

		switch (sa->sa_family) {
		case AF_INET6:
			((struct sockaddr_in6 *)sa)->sin6_port = htons(CONFIG_AGPS_SUPL_PORT);
			break;
		case AF_INET:
			((struct sockaddr_in *)sa)->sin_port = htons(CONFIG_AGPS_SUPL_PORT);
			break;
		}

		supl_fd = socket(sa->sa_family, SOCK_STREAM, IPPROTO_TCP);
		if (supl_fd < 0) {
			LOG_ERR("Failed to create socket, errno %d", errno);
			goto cleanup;
		}

		/* The SUPL library expects a 1 second timeout for the read function. */
		struct timeval timeout = {
			.tv_sec = 1,
			.tv_usec = 0,
		};

		err = setsockopt(supl_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		if (err) {
			LOG_ERR("Failed to set socket timeout, errno %d", errno);
			goto cleanup;
		}

		inet_ntop(sa->sa_family,
			  (void *)&((struct sockaddr_in *)sa)->sin_addr,
			  ip,
			  INET6_ADDRSTRLEN);
		LOG_DBG("Connecting to %s port %d", ip, CONFIG_AGPS_SUPL_PORT);

		err = connect(supl_fd, sa, addr->ai_addrlen);
		if (err) {
			close(supl_fd);
			supl_fd = -1;

			/* Try the next address */
			LOG_WRN("Connecting to server failed, errno %d", errno);
		} else {
			/* Connected */
			break;
		}
	}

cleanup:
	freeaddrinfo(info);

	if (err) {
		/* Unable to connect, close socket */
		LOG_ERR("Could not connect to SUPL server");
		if (supl_fd > -1) {
			close(supl_fd);
			supl_fd = -1;
		}
		return -1;
	}

	return 0;
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

		gnss_fd = socket;
	} else {
		gps_dev = device_get_binding("NRF9160_GPS");
		if (gps_dev != NULL) {
			LOG_DBG("Using GPS driver to input assistance data");
		} else {
			LOG_DBG("Using GNSS API to input assistance data");
		}
	}

	LOG_INF("SUPL is initialized");

	return 0;
}

static int supl_start(const struct nrf_modem_gnss_agps_data_frame request)
{
	int err;

	err = open_supl_socket();
	if (err) {
		LOG_ERR("Failed to open SUPL socket");
		return err;
	}

	LOG_INF("Starting SUPL session");

	err = supl_session(&request);
	if (err) {
		LOG_ERR("SUPL session failed, error: %d", err);
		goto cleanup;
	}

	LOG_INF("SUPL session finished successfully");

cleanup:
	close_supl_socket();

	return err;
}

#endif /* CONFIG_AGPS_SRC_SUPL */

int agps_request_send(struct nrf_modem_gnss_agps_data_frame request, int socket)
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

#elif defined(CONFIG_AGPS_SRC_NRF_CLOUD) && defined(CONFIG_NRF_CLOUD_MQTT)
	/* Convert GNSS API A-GPS request to GPS driver A-GPS request. */
	struct gps_agps_request agps_request;

	agps_request.sv_mask_ephe = request.sv_mask_ephe;
	agps_request.sv_mask_alm = request.sv_mask_alm;
	agps_request.utc =
		request.data_flags & NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST ? 1 : 0;
	agps_request.klobuchar =
		request.data_flags & NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST ? 1 : 0;
	agps_request.nequick =
		request.data_flags & NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST ? 1 : 0;
	agps_request.system_time_tow =
		request.data_flags & NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST ? 1 : 0;
	agps_request.position =
		request.data_flags & NRF_MODEM_GNSS_AGPS_POSITION_REQUEST ? 1 : 0;
	agps_request.integrity =
		request.data_flags & NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST ? 1 : 0;

	err = nrf_cloud_agps_request(agps_request);
	if (err) {
		LOG_ERR("nRF Cloud A-GPS request failed, error: %d", err);
		return err;
	}
#elif defined(CONFIG_AGPS_SRC_NRF_CLOUD) && !defined(CONFIG_NRF_CLOUD_MQTT)
	LOG_ERR("CONFIG_NRF_CLOUD_MQTT must be enabled to make A-GPS requests");
	return -EOPNOTSUPP;
	(void)err;
#endif /* CONFIG_AGPS_SRC_SUPL */

	return 0;
}

int agps_cloud_data_process(const uint8_t *buf, size_t len)
{
	int err = 0;

#if defined(CONFIG_AGPS_SRC_NRF_CLOUD) && defined(CONFIG_NRF_CLOUD_AGPS)
	err = nrf_cloud_agps_process(buf, len, NULL);
	if (err) {
		LOG_ERR("A-GPS failed, error: %d", err);
		return err;
	}

	LOG_INF("A-GPS data successfully processed");

#else /* CONFIG_AGPS_SRC_NRF_CLOUD && CONFIG_NRF_CLOUD_AGPS */

	LOG_WRN("Processing of incoming A-GPS data is not supported");
	return -EOPNOTSUPP;
#endif

	return err;
}
