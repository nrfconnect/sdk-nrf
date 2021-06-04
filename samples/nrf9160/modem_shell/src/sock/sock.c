/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <shell/shell.h>
#include <assert.h>
#include <strings.h>
#include <stdio.h>
#if defined(CONFIG_POSIX_API)
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/fdtable.h>
#else
#include <net/socket.h>
#endif
#include <net/tls_credentials.h>
#include <fcntl.h>
#include <modem/at_cmd.h>
#include <nrf_socket.h>

#include "sock.h"
#include "mosh_defines.h"
#include "link_api.h"
#include "net_utils.h"
#include "str_utils.h"

/* Maximum number of sockets takes into account AT command socket */
#define MAX_SOCKETS (CONFIG_POSIX_MAX_FDS - 1)
#define SOCK_SEND_BUFFER_SIZE_UDP 1200
/* This should be multiple of TCP window size (708) to make it more efficient */
#define SOCK_SEND_BUFFER_SIZE_TCP 3540
#define SOCK_RECEIVE_BUFFER_SIZE 1536
#define SOCK_RECEIVE_STACK_SIZE 1280
#define SOCK_RECEIVE_PRIORITY 5
/* Timeout (in ms) for polling socket events such as receive data,
 * permission to send more, disconnected socket etc.
 * This limits how quickly data can be received after socket creation.
 */
#define SOCK_POLL_TIMEOUT_MS 1000
#define SOCK_FD_NONE -1
#define SOCK_DATA_LEN_NONE -1

struct data_transfer_info {
	struct k_work_delayable work;
	int interval;
	void *parent; /* Type is sock_info */
	bool data_format_hex; /* Print data in hex format vs. normal string */
};

struct sock_info {
	int id;
	int fd;
	int family;
	int type;
	int port;
	int bind_port;
	int pdn_cid;
	bool in_use;
	char *send_buffer;
	uint32_t send_buffer_size;
	bool send_poll;
	uint32_t send_bytes_sent;
	int32_t send_bytes_left;
	int send_print_interval;
	bool log_receive_data;
	int64_t start_time_ms;
	int64_t recv_end_time_ms;
	uint32_t recv_data_len;
	uint32_t recv_data_len_expected;
	bool recv_start_throughput;
	enum sock_recv_print_format recv_print_format;
	struct addrinfo *addrinfo;
	struct data_transfer_info send_info;
};

K_MUTEX_DEFINE(sock_info_mutex);

K_SEM_DEFINE(sock_sem, 0, 1);

struct sock_info sockets[MAX_SOCKETS] = { 0 };
extern const struct shell *shell_global;

static void sock_info_clear(struct sock_info *socket_info)
{
	k_mutex_lock(&sock_info_mutex, K_FOREVER);

	if (k_work_delayable_busy_get(&socket_info->send_info.work) > 0) {
		k_work_cancel_delayable(&socket_info->send_info.work);
		shell_print(shell_global, "Socket data send periodic stop");
	}
	if (socket_info->send_buffer != NULL) {
		free(socket_info->send_buffer);
	}
	if (socket_info->in_use) {
		close(socket_info->fd);
	}
	freeaddrinfo(socket_info->addrinfo);

	memset(socket_info, 0, sizeof(struct sock_info));

	socket_info->id = SOCK_ID_NONE;
	socket_info->fd = SOCK_FD_NONE;
	socket_info->log_receive_data = true;
	socket_info->recv_data_len_expected = SOCK_DATA_LEN_NONE;
	socket_info->recv_print_format = SOCK_RECV_PRINT_FORMAT_STR;

	k_mutex_unlock(&sock_info_mutex);
}

static int get_socket_id_by_fd(int fd)
{
	for (int i = 0; i < MAX_SOCKETS; i++) {
		if (sockets[i].fd == fd) {
			assert(i == sockets[i].id);
			return i;
		}
	}
	return SOCK_ID_NONE;
}

static struct sock_info *get_socket_info_by_id(int socket_id)
{
	struct sock_info *socket_info = NULL;

	if (socket_id == SOCK_ID_NONE) {
		shell_error(shell_global, "Socket id not given. -i option is mandatory");
		return NULL;
	}
	if (socket_id < 0 || socket_id > MAX_SOCKETS) {
		shell_error(shell_global, "Socket id=%d must a postive number smaller than %d",
			    socket_id, MAX_SOCKETS);
		return NULL;
	}
	socket_info = &(sockets[socket_id]);
	if (!socket_info->in_use) {
		shell_error(shell_global, "Socket id=%d not available", socket_id);
		return NULL;
	}
	return socket_info;
}

static struct sock_info *reserve_socket_id(void)
{
	struct sock_info *socket_info = NULL;
	int socket_id = 0;

	while (socket_id < MAX_SOCKETS) {
		if (!sockets[socket_id].in_use) {
			socket_info = &(sockets[socket_id]);
			sock_info_clear(socket_info);
			socket_info->id = socket_id;
			socket_info->send_info.parent = socket_info;
			break;
		}
		socket_id++;
	}
	return socket_info;
}

static bool sock_get_blocking_mode(int fd)
{
	int blocking = true;
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags | (int) O_NONBLOCK) {
		blocking = false;
	}
	return blocking;
}

static void set_sock_blocking_mode(int fd, bool blocking)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if (!blocking) {
		fcntl(fd, F_SETFL, flags | (int) O_NONBLOCK);
	} else {
		fcntl(fd, F_SETFL, flags & ~(int) O_NONBLOCK);
	}
}

static void sock_all_set_nonblocking(void)
{
	for (int i = 0; i < MAX_SOCKETS; i++) {
		if (sockets[i].in_use) {
			set_sock_blocking_mode(sockets[i].fd, false);
		}
	}
}

static bool sock_send_buffer_calloc(struct sock_info *socket_info, uint32_t size)
{
	if (socket_info->send_buffer != NULL) {
		if (socket_info->send_buffer_size == size) {
			memset(socket_info->send_buffer, 0,
			       socket_info->send_buffer_size);
		} else {
			free(socket_info->send_buffer);
		}
	}
	socket_info->send_buffer_size = size;
	socket_info->send_buffer = calloc(size + 1, 1);
	if (socket_info->send_buffer == NULL) {
		shell_error(
			shell_global,
			"Out of memory while reserving send buffer of size %d bytes",
			socket_info->send_buffer_size);
		return false;
	}
	return true;
}

static void sock_send_buffer_free(struct sock_info *socket_info)
{
	if (socket_info->send_buffer != NULL) {
		free(socket_info->send_buffer);
		socket_info->send_buffer_size = 0;
	}
}

static int sock_validate_parameters(
	int family,
	int type,
	char *address,
	int port,
	int bind_port,
	int pdn_cid,
	bool secure,
	int sec_tag)
{
	/* Validate family parameter */
	if (family != AF_INET && family != AF_INET6 && family != AF_PACKET) {
		shell_error(shell_global, "Unsupported address family=%d", family);
		return -EINVAL;
	}

	/* Validate type parameter */
	if (type != SOCK_STREAM && type != SOCK_DGRAM && type != SOCK_RAW) {
		shell_error(shell_global, "Unsupported address type=%d", type);
		return -EINVAL;
	}

	/* Do security related validation */
	if (secure) {
		if (type == SOCK_RAW) {
			shell_error(shell_global, "Security not supported with address type=%d",
				    type);
			return -EINVAL;
		}

		if (sec_tag < 0) {
			shell_error(shell_global,
				    "Security tag must be given when security is enabled");
			return -EINVAL;
		}
	}

	/* Validate port */
	if (type != SOCK_RAW && (port < 1 || port > 65535)) {
		shell_error(
			shell_global,
			"Port (%d) must be bigger than 0 and smaller than 65536",
			port);
		return -EINVAL;
	}

	/* Validate bind port. Zero means that binding is not done. */
	if (bind_port > 65535) {
		shell_error(
			shell_global,
			"Bind port (%d) must be smaller than 65536",
			bind_port);
		return -EINVAL;
	}
	return 0;
}

static int sock_protocol_resolve(
	int family,
	int type,
	bool secure)
{
	int proto = 0;

	/* Map type to protocol.
	 * If proto is set to zero for stream or dgram socket to let lower stack select it,
	 * socket creation fails with errno=43 (PROTONOSUPPORT). Hence we need to set protocol.
	 */
	if (type == SOCK_STREAM) {
		proto = IPPROTO_TCP;
	} else if (type == SOCK_DGRAM) {
		proto = IPPROTO_UDP;
	}
	/* Proto is left to zero for raw sockets */

	if (secure) {
		if (type == SOCK_STREAM) {
			proto = IPPROTO_TLS_1_2;
		} else if (type == SOCK_DGRAM) {
			proto = IPPROTO_DTLS_1_2;
		}
	}

	return proto;
}

static int sock_getaddrinfo(
	struct sock_info *socket_info,
	int family,
	int type,
	char *address,
	int port)
{
	int err;

	if ((address == NULL) || (strlen(address) == 0)) {
		if (type != SOCK_RAW) {
			shell_error(shell_global, "Address not given");
			return -EINVAL;
		}
	} else {
		struct addrinfo hints = {
			.ai_family = family,
			.ai_socktype = type,
		};
		err = getaddrinfo(address, NULL, &hints, &socket_info->addrinfo);
		if (err) {
			shell_error(
				shell_global,
				"getaddrinfo() failed, err %d errno %d",
				err,
				errno);
			err = errno;
			return err;
		}

		/* Set port to address info */
		if (family == AF_INET) {
			((struct sockaddr_in *)socket_info->addrinfo->ai_addr)->sin_port =
				htons(port);
		} else if (family == AF_INET6) {
			((struct sockaddr_in6 *)socket_info->addrinfo->ai_addr)->sin6_port =
				htons(port);
		} else {
			assert(0);
		}
	}
	return 0;
}

static int sock_set_tls_options(
	int fd,
	int sec_tag,
	bool session_cache,
	int peer_verify,
	char *peer_hostname)
{
	int err;
	sec_tag_t sec_tag_list[] = { sec_tag };
	uint8_t cache;
	uint32_t verify;

	/* Security tag */
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
			 sizeof(sec_tag_t) * ARRAY_SIZE(sec_tag_list));
	if (err) {
		shell_error(shell_global, "Unable to set security tag, errno %d", errno);
		err = errno;
		return -EINVAL;
	}

	/* Session cache */
	if (session_cache) {
		cache = TLS_SESSION_CACHE_ENABLED;
	} else {
		cache = TLS_SESSION_CACHE_DISABLED;
	}
	err = setsockopt(fd, SOL_TLS, TLS_SESSION_CACHE, &cache, sizeof(cache));
	if (err) {
		shell_error(shell_global, "Unable to set session cache, errno %d", errno);
		return errno;
	}

	/* Peer verify */
	switch (peer_verify) {
	case 0:
		verify = TLS_PEER_VERIFY_NONE;
		break;
	case 1:
		verify = TLS_PEER_VERIFY_OPTIONAL;
		break;
	case 2:
	default:
		verify = TLS_PEER_VERIFY_REQUIRED;
		break;
	}
	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		shell_error(shell_global, "Unable to set peer verify, errno %d", errno);
		return errno;
	}

	/* Peer hostname */
	if (strlen(peer_hostname) > 0) {
		err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, &peer_hostname, strlen(peer_hostname));
		if (err) {
			shell_error(shell_global, "Unable to set peer hostname, errno %d", errno);
			return errno;
		}
	}
	return 0;
}

static int sock_bind(
	int fd,
	int family,
	int bind_port)
{
	int err;
	struct sockaddr_in sa_local;
	struct sockaddr_in6 sa_local6;
	struct sockaddr *sa_local_ptr = NULL;
	int sa_local_len = 0;

	memset(&sa_local, 0, sizeof(struct sockaddr_in));
	memset(&sa_local6, 0, sizeof(struct sockaddr_in6));

	sa_local.sin_family = family;
	sa_local.sin_port = htons(bind_port);
	sa_local.sin_addr.s_addr = INADDR_ANY;

	sa_local6.sin6_family = family;
	sa_local6.sin6_port = htons(bind_port);
	sa_local6.sin6_addr = in6addr_any;

	if (family == AF_INET) {
		sa_local_ptr = (struct sockaddr *)&sa_local;
		sa_local_len = sizeof(struct sockaddr_in);
	} else if (family == AF_INET6) {
		sa_local_ptr = (struct sockaddr *)&sa_local6;
		sa_local_len = sizeof(struct sockaddr_in6);
	}

	err = bind(fd, sa_local_ptr, sa_local_len);
	if (err) {
		shell_error(shell_global, "Unable to bind, errno %d", errno);
		return errno;
	}
	return 0;
}

static int sock_bind_to_pdp_context(struct sock_info *socket_info, int pdn_cid)
{
	struct pdp_context_info_array pdp_context_info_tbl;
	int pdn_id = -1;
	bool found = false;
	int err;

	err = link_api_pdp_contexts_read(&pdp_context_info_tbl);
	if (err) {
		shell_error(shell_global, "cannot read current connection info: %d", err);
		return -EINVAL;
	}

	/* Find PDP context info for requested CID */
	for (int i = 0; i < pdp_context_info_tbl.size; i++) {
		if (pdp_context_info_tbl.array[i].cid == pdn_cid) {
			pdn_id = pdp_context_info_tbl.array[i].pdn_id;
			found = true;
			err = 0;
		}
	}

	if (found) {
		err = net_utils_socket_pdn_id_set(socket_info->fd, pdn_id);
		if (err) {
			shell_error(
				shell_global,
				"Cannot bind socket id=%d to PDN ID %d",
				socket_info->id,
				pdn_id);
		}
	} else {
		shell_error(shell_global, "PDN context with CID=%d doesn't exist", pdn_cid);
		err = -EINVAL;
	}

	if (pdp_context_info_tbl.array != NULL) {
		free(pdp_context_info_tbl.array);
	}

	return err;
}

int sock_open_and_connect(
	int family,
	int type,
	char *address,
	int port,
	int bind_port,
	int pdn_cid,
	bool secure,
	int sec_tag,
	bool session_cache,
	int peer_verify,
	char *peer_hostname)
{
	int err = -EINVAL;
	int proto = 0;
	int fd = 0;

	shell_print(shell_global,
		    "Socket open and connect family=%d, type=%d, port=%d, bind_port=%d, "
		    "pdn_cid=%d, address=%s",
		    family, type, port, bind_port, pdn_cid, address);
	if (secure) {
		shell_print(shell_global,
			    "                        secure=%d, sec_tag=%d, session_cache=%d, "
			    "peer_verify=%d, peer_hostname=%s",
			    secure, sec_tag, session_cache, peer_verify, peer_hostname);
	}

	/* Reserve socket ID and structure for a new connection */
	struct sock_info *socket_info = reserve_socket_id();

	if (socket_info == NULL) {
		shell_error(
			shell_global,
			"Socket creation failed. MAX_SOCKETS=%d exceeded",
			MAX_SOCKETS);
		goto connect_error;
	}

	/* Validate parameters */
	proto = sock_validate_parameters(family, type, address, port, bind_port, pdn_cid,
					 secure, sec_tag);
	if (proto < 0) {
		goto connect_error;
	}

	/* Get address */
	err = sock_getaddrinfo(socket_info, family, type, address, port);
	if (err) {
		goto connect_error;
	}

	/* Create socket */
	proto = sock_protocol_resolve(family, type, secure);
	fd = socket(family, type, proto);
	if (fd < 0) {
		if (errno == ENFILE || errno == EMFILE) {
			shell_error(
				shell_global,
				"Socket creation failed due to maximum number of sockets in the "
				"system exceeded (%d). Notice that all file descriptors in the "
				"system are taken into account and not just sockets created "
				"through this application.",
				CONFIG_POSIX_MAX_FDS);
		} else {
			shell_error(
				shell_global,
				"Socket create failed, err %d",
				errno);
		}
		err = errno;
		goto connect_error;
	}

	/* Socket has been created so populate its structure with information */
	socket_info->in_use = true;
	socket_info->fd = fd;
	socket_info->family = family;
	socket_info->type = type;
	socket_info->port = port;
	socket_info->bind_port = bind_port;
	socket_info->pdn_cid = pdn_cid;

	/* Binding socket with PDN ID to particular PDP context */
	if (pdn_cid > 0) {
		err = sock_bind_to_pdp_context(socket_info, pdn_cid);
		if (err) {
			goto connect_error;
		}
	}

	/* Bind socket */
	if (bind_port > 0) {
		err = sock_bind(fd, family, bind_port);
		if (err) {
			goto connect_error;
		}
	}

	/* Set (D)TLS options */
	if (secure) {
		err = sock_set_tls_options(fd, sec_tag, session_cache, peer_verify, peer_hostname);
		if (err) {
			goto connect_error;
		}
	}

	if (type == SOCK_STREAM) {
		/* Connect TCP socket */
		err = connect(
			fd,
			socket_info->addrinfo->ai_addr,
			socket_info->addrinfo->ai_addrlen);
		if (err) {
			shell_error(shell_global, "Unable to connect, errno %d", errno);
			err = errno;
			goto connect_error;
		}
	}

	/* Set socket to non-blocking mode to make sure receiving
	 * is not blocking polling of all sockets
	 */
	set_sock_blocking_mode(socket_info->fd, false);

	/* Trigger socket receive handler if it's waiting for socket creation */
	k_sem_give(&sock_sem);

	shell_print(
		shell_global,
		"Socket created socket_id=%d, fd=%d",
		socket_info->id,
		fd);

	return 0;

connect_error:
	sock_info_clear(socket_info);
	return err;
}

static double calculate_throughput(uint32_t data_len, int64_t time_ms)
{
	/* 8 for bits in one byte, and 1000 for ms->s conversion.
	 * Parenthesis used to change order of multiplying so that
	 * intermediate values do not overflow from 32bit integer.
	 */
	double throughput = 8 * 1000 * ((double)data_len / time_ms);

	return throughput;
}

static void print_throughput_summary(uint32_t data_len, int64_t time_ms)
{
	double throughput = calculate_throughput(data_len, time_ms);

	shell_print(
		shell_global,
		"Summary:\n"
		"Data length: %7u bytes\n"
		"Time:        %7.2f s\n"
		"Throughput:  %7.0f bit/s",
		data_len,
		(float)time_ms / 1000,
		throughput);
}

static void sock_print_data_hex(uint8_t *buffer, uint32_t buffer_size)
{
	/* Print received data in hexadecimal format having 8 bytes per line.
	 * This is not made with single shell_print because we would need to
	 * reserve a lot bigger buffer for converting all data into hexadecimal string.
	 */
	char hex_data[81];
	int data_printed = 0;
	int data_left;
	int print_chars;

	while (data_printed < buffer_size) {
		data_left = buffer_size - data_printed;
		print_chars = data_left <= 8 ? data_left : 8;

		for (int i = 0; i < print_chars; i++) {
			sprintf(hex_data + i * 5, "0x%02X ", buffer[data_printed + i]);
		}
		shell_print(shell_global, "\t%s", hex_data);
		data_printed += print_chars;
	}
}

static int sock_send(
	struct sock_info *socket_info,
	char *data,
	int length,
	bool log_data,
	bool data_hex_format)
{
	int bytes;
	int dest_addr_len = 0;

	if (log_data) {
		if (data_hex_format) {
			shell_print(shell_global, "Socket data send:");
			sock_print_data_hex(data, length);
		} else {
			shell_print(shell_global, "Socket data send:\n\t%s", data);
		}
	}

	if (socket_info->type == SOCK_DGRAM) {
		/* UDP */
		if (socket_info->family == AF_INET) {
			dest_addr_len = sizeof(struct sockaddr_in);
		} else if (socket_info->family == AF_INET6) {
			dest_addr_len = sizeof(struct sockaddr_in6);
		}
		bytes = sendto(socket_info->fd, data, length, 0,
			       socket_info->addrinfo->ai_addr, dest_addr_len);
	} else {
		/* TCP and raw socket */
		bytes = send(socket_info->fd, data, length, 0);
	}
	if (bytes < 0) {
		/* Ideally we'd like to log the failure here but non-blocking
		 * socket causes huge number of failures due to incorrectly
		 * set POLLOUT flag:
		 * https://devzone.nordicsemi.com/f/nordic-q-a/65392/bug-nrf9160-tcp-send-flow-control-seems-entirely-broken
		 * Hence, we'll log only if we have blocking socket
		 */
		if (sock_get_blocking_mode(socket_info->fd)) {
			shell_print(
				shell_global,
				"socket send failed, err %d",
				errno);
		}
		return -1;
	}
	return bytes;
}

static void data_send_work_handler(struct k_work *item)
{
	struct data_transfer_info *data_send_info_ptr =
		CONTAINER_OF(item, struct data_transfer_info, work);
	struct sock_info *socket_info = data_send_info_ptr->parent;

	if (!socket_info->in_use) {
		shell_print(
			shell_global,
			"Socket id=%d not in use. Fatal error and sending won't work.",
			socket_info->id);
		k_work_cancel_delayable(&socket_info->send_info.work);
		return;
	}

	sock_send(
		socket_info,
		socket_info->send_buffer,
		socket_info->send_buffer_size,
		true,
		data_send_info_ptr->data_format_hex);

	k_work_schedule(
		&socket_info->send_info.work,
		K_SECONDS(socket_info->send_info.interval));
}

static void sock_send_random_data_length(struct sock_info *socket_info)
{
	int bytes;
	int64_t time_intermediate;
	int64_t ul_time_intermediate_ms;
	double throughput;
	int64_t ul_time_ms;

	while (socket_info->send_bytes_left > 0) {
		if (socket_info->send_bytes_left < socket_info->send_buffer_size) {
			memset(socket_info->send_buffer, 0, socket_info->send_buffer_size);
			memset(socket_info->send_buffer, 'l', socket_info->send_bytes_left);
		}
		bytes = sock_send(socket_info, socket_info->send_buffer,
				  strlen(socket_info->send_buffer), false, false);

		if (bytes < 0) {
			/* Wait for socket to allow sending again */
			socket_info->send_poll = true;
			return;
		}
		socket_info->send_bytes_sent += bytes;
		socket_info->send_bytes_left -= bytes;

		/* Print throughput stats every 10 seconds */
		time_intermediate = k_uptime_get();
		ul_time_intermediate_ms = time_intermediate - socket_info->start_time_ms;

		if ((ul_time_intermediate_ms / (double)1000) > socket_info->send_print_interval) {
			throughput = calculate_throughput(
				socket_info->send_bytes_sent,
				ul_time_intermediate_ms);

			shell_print(
				shell_global,
				"%7u bytes, %6.2fs, %6.0f bit/s",
				socket_info->send_bytes_sent,
				(float)ul_time_intermediate_ms / 1000,
				throughput);

			socket_info->send_print_interval += 10;
		}
	}
	socket_info->send_poll = false;
	ul_time_ms = k_uptime_delta(&socket_info->start_time_ms);
	sock_send_buffer_free(socket_info);
	print_throughput_summary(socket_info->send_bytes_sent, ul_time_ms);
}

int sock_send_data(
	int socket_id,
	char *data,
	int random_data_length,
	int interval,
	bool blocking,
	int buffer_size,
	bool data_format_hex)
{
	struct sock_info *socket_info = get_socket_info_by_id(socket_id);
	/* Process data to be sent based on input parameters */
	int data_out_length = strlen(data);
	uint8_t *data_out = data;
	uint8_t data_out_hex[SOCK_MAX_SEND_DATA_LEN / 2 + 1] = { 0 };

	if (socket_info == NULL) {
		return -EINVAL;
	}

	sock_all_set_nonblocking();

	/* Process data to be sent if it's in hex format */
	if (data_format_hex) {
		uint16_t data_out_hex_length = SOCK_MAX_SEND_DATA_LEN / 2;

		data_out_hex_length = str_hex_to_bytes(
			data, data_out_length, data_out_hex, data_out_hex_length);
		data_out = data_out_hex;
		data_out_length = data_out_hex_length;
	}

	/* Enable receive data logging as previous commands might have left it disabled */
	socket_info->log_receive_data = true;
	if (random_data_length > 0) {
		/* Send given amount of data */
		uint32_t send_buffer_size = SOCK_SEND_BUFFER_SIZE_TCP;

		/* Interval is not supported with data length */
		if (interval != SOCK_SEND_DATA_INTERVAL_NONE) {
			shell_error(
				shell_global,
				"Data length and interval cannot be specified at the same time");
			return -EINVAL;
		}

		if (buffer_size != SOCK_BUFFER_SIZE_NONE) {
			send_buffer_size = buffer_size;
		} else if (socket_info->type == SOCK_DGRAM) {
			send_buffer_size = SOCK_SEND_BUFFER_SIZE_UDP;
		}
		if (!sock_send_buffer_calloc(socket_info, send_buffer_size)) {
			return -ENOMEM;
		}

		shell_print(
			shell_global,
			"Sending %d bytes of data with buffer_size=%d, blocking=%d",
			random_data_length,
			send_buffer_size,
			blocking);

		/* Warn about big buffer sizes as lower levels gets stuck when buffer size
		 * increases. Not necessarily right above these values you cannot use much bigger
		 * send buffer.
		 */
		if ((socket_info->type == SOCK_STREAM && send_buffer_size > 4096) ||
		    (socket_info->type == SOCK_DGRAM && send_buffer_size > 1200)) {
			shell_warn(
				shell_global,
				"Sending %d bytes of data with buffer_size=%d, blocking=%d",
				random_data_length, send_buffer_size, blocking);
		}

		socket_info->send_bytes_sent = 0;
		socket_info->send_bytes_left = random_data_length;
		socket_info->log_receive_data = false;
		socket_info->send_print_interval = 10;
		/* Set requested blocking mode for duration of data sending */
		set_sock_blocking_mode(socket_info->fd, blocking);

		memset(socket_info->send_buffer, 'd', socket_info->send_buffer_size);

		socket_info->start_time_ms = k_uptime_get();

		sock_send_random_data_length(socket_info);

		/* Keep default mode of the socket in non-blocking mode */
		set_sock_blocking_mode(socket_info->fd, false);

	} else if (interval != SOCK_SEND_DATA_INTERVAL_NONE) {

		if (interval == 0) {
			/* Stop periodic data sending */
			if (k_work_delayable_busy_get(&socket_info->send_info.work) > 0) {
				k_work_cancel_delayable(&socket_info->send_info.work);
				shell_print(shell_global, "Socket data send periodic stop");
			} else {
				shell_error(shell_global,
					    "Socket data send stop: periodic data not started");
				return -EINVAL;
			}
		} else if (interval > 0) {
			/* Send data with given interval */

			/* Data to be sent must also be specified */
			if (data_out_length < 1) {
				shell_error(shell_global,
					    "Data sending interval is specified without "
					    "data to be sent");
				return -EINVAL;
			}

			if (!sock_send_buffer_calloc(socket_info, data_out_length)) {
				return -ENOMEM;
			}
			memcpy(socket_info->send_buffer, data_out, data_out_length);
			socket_info->send_buffer_size = data_out_length;

			socket_info->send_info.data_format_hex = data_format_hex;
			socket_info->send_info.interval = interval;
			shell_print(
				shell_global,
				"Socket data send periodic with interval=%d",
				interval);
			k_work_init_delayable(
				&socket_info->send_info.work,
				data_send_work_handler);
			/* Send immediately for the first time */
			k_work_schedule(&socket_info->send_info.work, K_SECONDS(0));
		}

	} else if (data_out != NULL && data_out_length > 0) {
		/* Send data if it's given and is not zero length */
		sock_send(socket_info, data_out, data_out_length, true, data_format_hex);
	} else {
		shell_print(shell_global, "No send parameters given");
		return -EINVAL;
	}
	return 0;
}

/**
 * @brief Socket receive handler running an infinite loop polling for events on the sockets.
 */
static void sock_receive_handler(void)
{
	struct pollfd fds[MAX_SOCKETS];
	char *receive_buffer = NULL;
	struct sock_info *socket_info;
	int socket_id;
	int count;
	int buffer_size;
	int ret;

	while (true) {
		count = 0;

		/* Create list of sockets with their poll event flags to be polled */
		for (int i = 0; i < MAX_SOCKETS; i++) {
			if (sockets[i].in_use) {
				fds[count].fd = sockets[i].fd;
				fds[count].events = POLLIN;
				if (sockets[i].send_poll) {
					fds[count].events |= POLLOUT;
				}
				fds[count].revents = 0;
				count++;
			}
		}

		if (count == 0) {
			k_sem_reset(&sock_sem);

			/* No sockets, release the receive buffer */
			if (receive_buffer != NULL) {
				k_free(receive_buffer);
				receive_buffer = NULL;
			}

			/* Wait for a socket to be created */
			k_sem_take(&sock_sem, K_FOREVER);
			continue;
		}

		/* Poll sockets */
		ret = poll(fds, count, SOCK_POLL_TIMEOUT_MS);
		if (ret <= 0) {
			continue;
		}

		/* Process all requested sockets for poll events */
		for (int i = 0; i < count; i++) {
			socket_id = get_socket_id_by_fd(fds[i].fd);
			if (socket_id == SOCK_ID_NONE) {
				/* Socket has been already deleted from internal structures.
				 * This occurs at least when we close socket after which
				 * there will be notification for it.
				 */
				continue;
			}
			socket_info = &(sockets[socket_id]);

			if (fds[i].revents & POLLIN) {
				/* Data received. If receive buffer doesn't exist, allocate it. */
				if (receive_buffer == NULL) {
					receive_buffer = k_calloc(SOCK_RECEIVE_BUFFER_SIZE + 1, 1);
					if (receive_buffer == NULL) {
						shell_error(shell_global,
							    "Out of memory while reserving "
							    "receive buffer of size %d bytes",
							    SOCK_RECEIVE_BUFFER_SIZE);
						break;
					}
				}

				/* Store timestamp for receiving data */
				if (socket_info->recv_start_throughput) {
					socket_info->start_time_ms = k_uptime_get();
					socket_info->recv_start_throughput = false;
				}

				buffer_size = recv(fds[i].fd, receive_buffer,
						   SOCK_RECEIVE_BUFFER_SIZE, 0);
				if (buffer_size <= 0) {
					continue;
				}

				socket_info->recv_end_time_ms = k_uptime_get();
				socket_info->recv_data_len += buffer_size;

				if (socket_info->recv_data_len >=
					socket_info->recv_data_len_expected) {

					print_throughput_summary(
						socket_info->recv_data_len,
						socket_info->recv_end_time_ms -
						socket_info->start_time_ms);
					/* Do not print when more data received. */
					socket_info->recv_data_len_expected = SOCK_DATA_LEN_NONE;
				}

				if (socket_info->log_receive_data) {
					shell_print(shell_global,
						    "Received data for socket "
						    "socket_id=%d, buffer_size=%d:",
						    socket_id,
						    buffer_size);

					if (socket_info->recv_print_format ==
						SOCK_RECV_PRINT_FORMAT_HEX) {

						sock_print_data_hex(receive_buffer, buffer_size);
					} else { /* SOCK_RECV_PRINT_FORMAT_STR */
						shell_print(shell_global, "\t%s", receive_buffer);
					}
				}
				memset(receive_buffer, '\0', SOCK_RECEIVE_BUFFER_SIZE);
			}
			if (fds[i].revents & POLLOUT) {
				sock_send_random_data_length(socket_info);
			}
			if (fds[i].revents & POLLERR) {
				shell_print(shell_global,
					    "Error from socket id=%d (fd=%d), closing",
					    socket_id, fds[i].fd);
				sock_info_clear(socket_info);
			}
			if (fds[i].revents & POLLHUP) {
				shell_print(shell_global,
					    "Socket id=%d (fd=%d) disconnected so closing.",
					    socket_id, fds[i].fd);
				sock_info_clear(socket_info);
			}
			if (fds[i].revents & POLLNVAL) {
				shell_print(shell_global, "Socket id=%d invalid", socket_id);
				sock_info_clear(socket_info);
			}
		}
	}
	shell_print(shell_global, "%s exit", __func__);
}

K_THREAD_DEFINE(sock_receive_thread, SOCK_RECEIVE_STACK_SIZE,
		sock_receive_handler, NULL, NULL, NULL,
		SOCK_RECEIVE_PRIORITY, 0, 0);

int sock_recv(int socket_id, bool receive_start, int data_length, bool blocking,
	      enum sock_recv_print_format print_format)
{
	struct sock_info *socket_info = get_socket_info_by_id(socket_id);

	if (socket_info == NULL) {
		return -EINVAL;
	}

	if (print_format != SOCK_RECV_PRINT_FORMAT_NONE) {
		switch (print_format) {
		case SOCK_RECV_PRINT_FORMAT_STR:
		case SOCK_RECV_PRINT_FORMAT_HEX:
			shell_print(
				shell_global,
				"Receive print format changed for socket id=%d",
				socket_info->id);
			socket_info->recv_print_format = print_format;
			break;
		default:
			shell_error(
				shell_global,
				"Receive data print format (%d) must be %d or %d",
				print_format,
				SOCK_RECV_PRINT_FORMAT_STR,
				SOCK_RECV_PRINT_FORMAT_HEX);
			return -EINVAL;
		}
	} else if (receive_start) {
		shell_print(shell_global,
			    "Receive data calculation start socket id=%d", socket_info->id);
		/* Set any leftover blocking sockets to non-blocking */
		sock_all_set_nonblocking();
		socket_info->recv_start_throughput = true;
		socket_info->recv_data_len = 0;
		socket_info->log_receive_data = false;
		socket_info->start_time_ms = 0;
		socket_info->recv_end_time_ms = 0;
		set_sock_blocking_mode(socket_info->fd, blocking);

		/* Set expected data length when summary is printed */
		if (data_length > 0) {
			socket_info->recv_data_len_expected = data_length;
		} else {
			socket_info->recv_data_len_expected = SOCK_DATA_LEN_NONE;
		}
	} else {
		print_throughput_summary(
			socket_info->recv_data_len,
			socket_info->recv_end_time_ms - socket_info->start_time_ms);
	}
	return 0;
}

int sock_close(int socket_id)
{
	struct sock_info *socket_info = get_socket_info_by_id(socket_id);

	if (socket_info == NULL) {
		return -EINVAL;
	}
	shell_print(shell_global, "Close socket id=%d, fd=%d", socket_info->id, socket_info->fd);
	sock_info_clear(socket_info);
	return 0;
}

int sock_rai_enable(int rai_enable)
{
	enum at_cmd_state state = AT_CMD_OK;
	char command[] = "AT%%RAI=0";
	int err;

	if (rai_enable == SOCK_RAI_NONE) {
		shell_error(shell_global, "No valid RAI options given");
		return -EINVAL;
	}
	sprintf(command, "AT%%RAI=%d", rai_enable);
	err = at_cmd_write(command, NULL, 0, &state);
	if (state == AT_CMD_OK) {
		shell_print(
			shell_global,
			"Release Assistance Indication functionality set to enabled=%d",
			rai_enable);
	} else {
		shell_error(shell_global, "Error state=%d, error=%d",
			    state, err);
		return -EINVAL;
	}
	return 0;
}

static int sock_rai_option_set(int fd, int option, char *option_string)
{
	int err = setsockopt(fd, SOL_SOCKET, option, NULL, 0);

	if (err) {
		shell_error(shell_global,
			    "setsockopt() for %s failed with error %d",
			    option_string, errno);
		return err;
	}

	shell_print(shell_global, "Socket option %s set", option_string);
	return 0;
}

int sock_rai(int socket_id, bool rai_last, bool rai_no_data,
	     bool rai_one_resp, bool rai_ongoing, bool rai_wait_more)
{
	struct sock_info *socket_info = get_socket_info_by_id(socket_id);
	int err;

	if (socket_info == NULL) {
		return -EINVAL;
	}

	if (!rai_last && !rai_no_data && !rai_one_resp && !rai_ongoing && !rai_wait_more) {
		shell_error(shell_global, "No socket specific RAI options given with -i");
	}

	/* SO_RAI_LAST */
	if (rai_last) {
		err = sock_rai_option_set(socket_info->fd, SO_RAI_LAST, "SO_RAI_LAST");
		if (err) {
			return err;
		}
	}

	/* SO_RAI_NO_DATA */
	if (rai_no_data) {
		err = sock_rai_option_set(socket_info->fd, SO_RAI_NO_DATA, "SO_RAI_NO_DATA");
		if (err) {
			return err;
		}
	}

	/* SO_RAI_ONE_RESP */
	if (rai_one_resp) {
		err = sock_rai_option_set(socket_info->fd, SO_RAI_ONE_RESP, "SO_RAI_ONE_RESP");
		if (err) {
			return err;
		}
	}

	/* SO_RAI_ONGOING */
	if (rai_ongoing) {
		err = sock_rai_option_set(socket_info->fd, SO_RAI_ONGOING, "SO_RAI_ONGOING");
		if (err) {
			return err;
		}
	}

	/* SO_RAI_WAIT_MORE */
	if (rai_wait_more) {
		err = sock_rai_option_set(socket_info->fd, SO_RAI_WAIT_MORE, "SO_RAI_WAIT_MORE");
		if (err) {
			return err;
		}
	}

	return 0;
}

int sock_list(void)
{
	bool opened_sockets = false;

	for (int i = 0; i < MAX_SOCKETS; i++) {
		struct sock_info *socket_info = &(sockets[i]);

		if (socket_info->in_use) {
			opened_sockets = true;
			shell_print(shell_global,
				    "Socket id=%d, fd=%d, family=%d, type=%d, port=%d, "
				    "bind_port=%d, pdn=%d",
				    i,
				    socket_info->fd,
				    socket_info->family,
				    socket_info->type,
				    socket_info->port,
				    socket_info->bind_port,
				    socket_info->pdn_cid);
		}
	}

	if (!opened_sockets) {
		shell_print(shell_global, "There are no open sockets");
	}
	return 0;
}
