/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>

#include <net/downloader.h>
#include <net/downloader_transport_coap.h>
#include <net/downloader_transport_http.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/coap.h>

#include <zephyr/fff.h>
#include <sys/types.h>
#include <errno.h>

#define HOSTNAME "server.com"
#define HOSTNAME2 "server2.com"

#define NO_PROTO_URL "server.com/path/to/file.end"
#define HTTP_URL "http://server.com:80/path/to/file.end"
#define HTTP_URL_FILE2 "http://server.com/path/to/file2.end"
#define HTTP_URL_HOST2 "http://server2.com/path/to/file.end"
#define HTTPS_URL "https://server.com/path/to/file.end"
#define COAP_URL "coap://server.com/path/to/file.end"
#define COAPS_URL "coaps://server.com/path/to/file.end"
#define BAD_URL "bad://server.com/path/to/file.end"
#define BAD_URL2 "bad:/server.com:80/path/to/file.end"
#define BAD_URL_NO_FILE "http://server.com"

#define HTTP_HOST "http://server.com"
#define HTTPS_HOST "https://server.com"
#define COAP_HOST "coap://server.com"
#define COAPS_HOST "coaps://server.com"
#define FILE_PATH "path/to/file.end"

#define FILE_PATH_LONG  "this/path/is/too/long/to/fit/in/the/buffer/provided/to/the/downloader/" \
			"for/download_get_with_file_and_hostname"

#define TEST_COAP_RETRIES 5
#define TEST_EXPECTED_RECVFROM_CALLS (TEST_COAP_RETRIES - 2)

#define HTTP_HDR_OK "HTTP/1.1 200 OK\r\n" \
"Accept-Ranges: bytes\r\n" \
"Age: 497805\r\n" \
"Cache-Control: max-age=604800\r\n" \
"Content-Encoding: gzip\r\n" \
"Content-Length: 128\r\n" \
"Content-Type: text/html; charset=UTF-8\r\n" \
"Date: Wed, 06 Nov 2024 13:00:48 GMT\r\n" \
"Etag: \"3147526947\"\r\n" \
"Expires: Wed, 23 Nov 2124 23:12:95 GMT\r\n" \
"Last-Modified: Thu, 06 Nov 2024 14:17:23 GMT\r\n" \
"Server: ECAcc (nyd/D184)\r\n" \
"Vary: Accept-Encoding\r\n" \
"X-Cache: HIT\r\n\r\n"

#define HTTP_HDR_OK_WITH_PAYLOAD "HTTP/1.1 200 OK\r\n" \
"Accept-Ranges: bytes\r\n" \
"Age: 497805\r\n" \
"Cache-Control: max-age=604800\r\n" \
"Content-Encoding: gzip\r\n" \
"Content-Length: 20\r\n" \
"Content-Type: text/html; charset=UTF-8\r\n" \
"Date: Wed, 06 Nov 2024 13:00:48 GMT\r\n" \
"Etag: \"3147526947\"\r\n" \
"Expires: Wed, 23 Nov 2124 23:12:95 GMT\r\n" \
"Last-Modified: Thu, 06 Nov 2024 14:17:23 GMT\r\n" \
"Server: ECAcc (nyd/D184)\r\n" \
"Vary: Accept-Encoding\r\n" \
"X-Cache: HIT\r\n\r\n"\
"This is the payload!"

#define HTTP_HDR_OK_PROGRESS "HTTP/1.1 206 OK\r\n" \
"Accept-Ranges: bytes\r\n" \
"Age: 497805\r\n" \
"Cache-Control: max-age=604800\r\n" \
"Content-Encoding: gzip\r\n" \
"Content-Length: 128\r\n" \
"Content-Type: text/html; charset=UTF-8\r\n" \
"Date: Wed, 06 Nov 2024 13:00:48 GMT\r\n" \
"Etag: \"3147526947\"\r\n" \
"Expires: Wed, 23 Nov 2124 23:12:95 GMT\r\n" \
"Last-Modified: Thu, 06 Nov 2024 14:17:23 GMT\r\n" \
"Server: ECAcc (nyd/D184)\r\n" \
"Vary: Accept-Encoding\r\n" \
"X-Cache: HIT\r\n\r\n"

#define HTTP_HDR_OK_PARTIAL_1 "HTTP/1.1 200 OK\r\n" \
"Accept-Ranges: bytes\r\n" \
"Age: 497805\r\n" \
"Cache-Control: max-age=604800\r\n" \
"Content-Encoding: gzip\r\n" \
"Content-Length: 128\r\n" \
"Content-Type: text/html; charset=UTF-8\r\n" \
"Date: W"

#define HTTP_HDR_OK_PARTIAL_2 \
"ed, 06 Nov 2024 13:00:48 GMT\r\n" \
"Etag: \"3147526947\"\r\n" \
"Expires: Wed, 23 Nov 2124 23:12:95 GMT\r\n" \
"Last-Modified: Thu, 06 Nov 2024 14:17:23 GMT\r\n" \
"Server: ECAcc (nyd/D184)\r\n" \
"Vary: Accept-Encoding\r\n" \
"X-Cache: HIT\r\n\r\n"

#define HTTPS_HDR_OK_PARTIAL_CONTENT_1 \
"HTTP/1.1 206 Partial Content\r\n" \
"Date: Tue, 21 Jan 2025 12:08:23 GMT\r\n" \
"Content-Type: text/html; charset=UTF-8\r\n" \
"Content-Length: 32\r\n" \
"Connection: keep-alive\r\n" \
"Accept-Ranges: bytes\r\n" \
"Content-Range: bytes 0-31/64\r\n\r\n"

#define HTTPS_HDR_OK_PARTIAL_CONTENT_2 \
"HTTP/1.1 206 Partial Content\r\n" \
"Date: Tue, 21 Jan 2025 12:08:23 GMT\r\n" \
"Content-Type: text/html; charset=UTF-8\r\n" \
"Content-Length: 32\r\n" \
"Connection: keep-alive\r\n" \
"Accept-Ranges: bytes\r\n" \
"Content-Range: bytes 32-63/64" \
"Accept-Ranges: bytes\r\n" \
"Age: 497805\r\n" \
"Cache-Control: max-age=604800\r\n" \
"Content-Encoding: gzip\r\n" \
"Etag: \"3147526947\"\r\n" \
"Expires: Wed, 23 Nov 2124 23:12:95 GMT\r\n" \
"Last-Modified: Thu, 06 Nov 2024 14:17:23 GMT\r\n" \
"Server: ECAcc (nyd/D184)\r\n" \
"Vary: Accept-Encoding\r\n" \
"X-Cache: HIT\r\n\r\n"

#define HTTPS_HDR_OK_PARTIAL_CONTENT_HDR_2_1 \
"HTTP/1.1 206 Partial Content\r\n" \
"Date: Tue, 21 Jan 2025 12:08:23 GMT\r\n" \
"Content-Type: text/html; charset=UTF-8\r\n" \
"Content-Length: 32\r\n" \
"Connection: keep-alive\r\n" \
"Accept-Ranges: bytes\r\n" \
"Content-Range: bytes 32-63/64\r\n" \
"Last-Modif"

#define HTTPS_HDR_OK_PARTIAL_CONTENT_HDR_2_2 \
"ied: Thu, 06 Nov 2024 14:17:23 GMT\r\n" \
"Server: ECAcc (nyd/D184)\r\n" \
"Vary: Accept-Encoding\r\n" \
"X-Cache: HIT\r\n\r\n"

#define HTTP_HDR_REDIRECT "HTTP/1.1 308 Permanent Redirect\r\n" \
"Date: Wed, 29 Jan 2025 11:16:09 GMT\r\n" \
"Content-Type: text/html\r\n" \
"Content-Length: 164\r\n" \
"Connection: keep-alive\r\n" \
"Location: https://server.com/path/to/file.end\r\n" \
"\r\n\r\n"

#define PAYLOAD "This is the payload!"

#define FD 0

static int dl_callback(const struct downloader_evt *event);
static int dl_callback_abort(const struct downloader_evt *event);

static struct downloader dl;

char dl_buf[2048];
struct downloader_cfg dl_cfg = {
	.callback = dl_callback,
	.buf = dl_buf,
	.buf_size = sizeof(dl_buf),
};

struct downloader_cfg dl_cfg_small_buffer = {
	.callback = dl_callback,
	.buf = dl_buf,
	.buf_size = 32,
};

struct downloader_cfg dl_cfg_cb_abort = {
	.callback = dl_callback_abort,
	.buf = dl_buf,
	.buf_size = sizeof(dl_buf),
};

static struct downloader_host_cfg dl_host_cfg = {
	.pdn_id = 1,
	.keep_connection = true,
};

int sec_tags[] = {1, 2, 3};
static struct downloader_host_cfg dl_host_conf_w_sec_tags = {
	.pdn_id = 1,
	.sec_tag_list = sec_tags,
	.sec_tag_count = ARRAY_SIZE(sec_tags),
};

static struct downloader_host_cfg dl_host_conf_w_sec_tags_range_override_32 = {
	.pdn_id = 1,
	.sec_tag_list = sec_tags,
	.sec_tag_count = ARRAY_SIZE(sec_tags),
	.range_override = 32,
};

static struct downloader_host_cfg dl_host_conf_w_sec_tags_and_cid = {
	.pdn_id = 1,
	.sec_tag_list = sec_tags,
	.sec_tag_count = ARRAY_SIZE(sec_tags),
	.cid = true,
};

struct downloader_transport_http_cfg dl_http_cfg = {
	.sock_recv_timeo_ms = 60000,
};

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, z_impl_zsock_setsockopt, int, int, int, const void *, net_socklen_t);
FAKE_VALUE_FUNC(int, z_impl_zsock_socket, int, int, int);
FAKE_VALUE_FUNC(int, z_impl_zsock_connect, int, const struct net_sockaddr *, net_socklen_t);
FAKE_VALUE_FUNC(int, z_impl_zsock_close, int)
FAKE_VALUE_FUNC(ssize_t, z_impl_zsock_send, int, const void *, size_t, int)
FAKE_VALUE_FUNC(ssize_t, z_impl_zsock_recv, int, void *, size_t, int)
FAKE_VALUE_FUNC(int, zsock_getaddrinfo, const char *, const char *, const struct zsock_addrinfo *,
		struct zsock_addrinfo **)
FAKE_VOID_FUNC(zsock_freeaddrinfo, struct zsock_addrinfo *);

FAKE_VALUE_FUNC(int, z_impl_zsock_inet_pton, net_sa_family_t, const char *, void *)
FAKE_VALUE_FUNC(char *, z_impl_net_addr_ntop, net_sa_family_t, const void *, char *, size_t)
FAKE_VALUE_FUNC(ssize_t, z_impl_zsock_sendto, int, const void *, size_t, int,
		const struct net_sockaddr *, net_socklen_t);
FAKE_VALUE_FUNC(ssize_t, z_impl_zsock_recvfrom, int, void *, size_t, int, struct net_sockaddr *,
		net_socklen_t *);

FAKE_VALUE_FUNC(int, coap_get_option_int, const struct coap_packet *, uint16_t);
FAKE_VALUE_FUNC(int, coap_block_transfer_init, struct coap_block_context *, enum coap_block_size,
		size_t);
FAKE_VOID_FUNC(coap_pending_clear, struct coap_pending *);
FAKE_VALUE_FUNC(bool, coap_pending_cycle, struct coap_pending *);
FAKE_VALUE_FUNC(int, coap_update_from_block, const struct coap_packet *,
		struct coap_block_context *);
FAKE_VALUE_FUNC(size_t, coap_next_block, const struct coap_packet *, struct coap_block_context *);
FAKE_VALUE_FUNC(int, coap_packet_parse, struct coap_packet *, uint8_t *, uint16_t,
		struct coap_option *, uint8_t);
FAKE_VALUE_FUNC(uint16_t, coap_header_get_id, const struct coap_packet *);
FAKE_VALUE_FUNC(uint8_t, coap_header_get_type, const struct coap_packet *);
FAKE_VALUE_FUNC(uint8_t, coap_header_get_code, const struct coap_packet *);
FAKE_VALUE_FUNC(const uint8_t *, coap_packet_get_payload, const struct coap_packet *, uint16_t *);
FAKE_VALUE_FUNC(int, coap_packet_init, struct coap_packet *, uint8_t *, uint16_t, uint8_t, uint8_t,
		uint8_t, const uint8_t *, uint8_t, uint16_t);
FAKE_VALUE_FUNC(uint8_t *, coap_next_token);
FAKE_VALUE_FUNC(int, coap_packet_append_option, struct coap_packet *, uint16_t, const uint8_t *,
		uint16_t);
FAKE_VALUE_FUNC(int, coap_append_block2_option, struct coap_packet *, struct coap_block_context *);
FAKE_VALUE_FUNC(int, coap_append_size2_option, struct coap_packet *, struct coap_block_context *);
FAKE_VALUE_FUNC(struct coap_transmission_parameters, coap_get_transmission_parameters);
FAKE_VALUE_FUNC(int, coap_pending_init, struct coap_pending *, const struct coap_packet *,
		const struct net_sockaddr *, const struct coap_transmission_parameters *);

uint16_t message_id;
uint16_t coap_next_id(void)
{
	return message_id++;
}

static struct net_sockaddr server_sockaddr = {
	.sa_family = NET_AF_INET,
};

static struct zsock_addrinfo server_addrinfo = {
	.ai_addr = &server_sockaddr,
	.ai_addrlen = sizeof(struct net_sockaddr),
};

static struct net_sockaddr server_sockaddr6 = {
	.sa_family = NET_AF_INET6,
};

static struct zsock_addrinfo server_addrinfo6 = {
	.ai_addr = &server_sockaddr6,
	.ai_addrlen = sizeof(struct net_sockaddr),
};

int zsock_getaddrinfo_server_ok(const char *host, const char *service,
				     const struct zsock_addrinfo *hints,
				     struct zsock_addrinfo **res)
{
	TEST_ASSERT_EQUAL_STRING(HOSTNAME, host);

	if (hints->ai_family == NET_AF_INET) {
		*res = &server_addrinfo;
		return 0;
	} else if (hints->ai_family == NET_AF_INET6) {
		*res = &server_addrinfo6;
		return 0;
	}

	errno = ENOPROTOOPT;
	return DNS_EAI_SYSTEM;
}

int zsock_getaddrinfo_server2_ok(const char *host, const char *service,
				     const struct zsock_addrinfo *hints,
				     struct zsock_addrinfo **res)
{
	TEST_ASSERT_EQUAL_STRING(HOSTNAME2, host);

	if (hints->ai_family == NET_AF_INET) {
		*res = &server_addrinfo;
		return 0;
	} else if (hints->ai_family == NET_AF_INET6) {
		*res = &server_addrinfo6;
		return 0;
	}

	errno = ENOPROTOOPT;
	return DNS_EAI_SYSTEM;
}


int zsock_getaddrinfo_server_ipv6_fail_ipv4_ok(const char *host, const char *service,
				     const struct zsock_addrinfo *hints,
				     struct zsock_addrinfo **res)
{
	if (hints->ai_family == NET_AF_INET6) {
		/* Fail on IPv6 to retry IPv4 */
		errno = ENOPROTOOPT;
		return DNS_EAI_SYSTEM;
	}

	TEST_ASSERT_EQUAL_STRING(HOSTNAME, host);
	TEST_ASSERT_EQUAL(NET_AF_INET, hints->ai_family);
	*res = &server_addrinfo;

	return 0;
}

int zsock_getaddrinfo_server_enetunreach(const char *host, const char *service,
				const struct zsock_addrinfo *hints,
				struct zsock_addrinfo **res)
{
	errno = ENETUNREACH;
	return DNS_EAI_SYSTEM;
}

void zsock_freeaddrinfo_server_ipv4(struct zsock_addrinfo *addr)
{
	TEST_ASSERT_EQUAL_PTR(&server_addrinfo, addr);
}

void zsock_freeaddrinfo_server_ipv6(struct zsock_addrinfo *addr)
{
	TEST_ASSERT_EQUAL_PTR(&server_addrinfo6, addr);
}

void zsock_freeaddrinfo_server_ipv6_then_ipv4(struct zsock_addrinfo *addr)
{
	switch (zsock_freeaddrinfo_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL_PTR(&server_addrinfo6, addr);
		break;
	case 2:
		TEST_ASSERT_EQUAL_PTR(&server_addrinfo, addr);
	default:

	}

}

int z_impl_zsock_socket_http_ipv4_ok(int family, int type, int proto)
{
	TEST_ASSERT_EQUAL(NET_AF_INET, family);
	TEST_ASSERT_EQUAL(NET_SOCK_STREAM, type);
	TEST_ASSERT_EQUAL(NET_IPPROTO_TCP, proto);

	return FD;
}

int z_impl_zsock_socket_http_ipv6_ok(int family, int type, int proto)
{
	TEST_ASSERT_EQUAL(NET_AF_INET6, family);
	TEST_ASSERT_EQUAL(NET_SOCK_STREAM, type);
	TEST_ASSERT_EQUAL(NET_IPPROTO_TCP, proto);

	return FD;
}

int z_impl_zsock_socket_http_ipv6_then_ipv4(int family, int type, int proto)
{
	switch (z_impl_zsock_socket_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL(NET_AF_INET6, family);
		break;
	case 2:
	default:
		TEST_ASSERT_EQUAL(NET_AF_INET, family);
	}

	TEST_ASSERT_EQUAL(NET_SOCK_STREAM, type);
	TEST_ASSERT_EQUAL(NET_IPPROTO_TCP, proto);

	return FD;
}

int z_impl_zsock_socket_https_ipv6_ok(int family, int type, int proto)
{
	TEST_ASSERT_EQUAL(NET_AF_INET6, family);
	TEST_ASSERT_EQUAL(NET_SOCK_STREAM, type);
	TEST_ASSERT_EQUAL(NET_IPPROTO_TLS_1_2, proto);

	return FD;
}

int z_impl_zsock_socket_coap_ipv4_ok(int family, int type, int proto)
{
	TEST_ASSERT_EQUAL(NET_AF_INET, family);
	TEST_ASSERT_EQUAL(NET_SOCK_DGRAM, type);
	TEST_ASSERT_EQUAL(NET_IPPROTO_UDP, proto);

	return FD;
}

int z_impl_zsock_socket_coap_ipv6_ok(int family, int type, int proto)
{
	TEST_ASSERT_EQUAL(NET_AF_INET6, family);
	TEST_ASSERT_EQUAL(NET_SOCK_DGRAM, type);
	TEST_ASSERT_EQUAL(NET_IPPROTO_UDP, proto);

	return FD;
}

int z_impl_zsock_socket_coaps_ipv4_ok(int family, int type, int proto)
{
	TEST_ASSERT_EQUAL(NET_AF_INET, family);
	TEST_ASSERT_EQUAL(NET_SOCK_DGRAM, type);
	TEST_ASSERT_EQUAL(NET_IPPROTO_DTLS_1_2, proto);

	return FD;
}

int z_impl_zsock_socket_coaps_ipv6_ok(int family, int type, int proto)
{
	TEST_ASSERT_EQUAL(NET_AF_INET6, family);
	TEST_ASSERT_EQUAL(NET_SOCK_DGRAM, type);
	TEST_ASSERT_EQUAL(NET_IPPROTO_DTLS_1_2, proto);

	return FD;
}

int z_impl_zsock_socket_coaps_ipv6_then_ipv4_ok(int family, int type, int proto)
{
	switch (z_impl_zsock_socket_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL(NET_AF_INET6, family);
		TEST_ASSERT_EQUAL(NET_SOCK_DGRAM, type);
		TEST_ASSERT_EQUAL(NET_IPPROTO_DTLS_1_2, proto);
		break;
	case 2:
		TEST_ASSERT_EQUAL(NET_AF_INET, family);
		TEST_ASSERT_EQUAL(NET_SOCK_DGRAM, type);
		TEST_ASSERT_EQUAL(NET_IPPROTO_DTLS_1_2, proto);
		break;
	}

	return FD;
}

int z_impl_zsock_connect_ipv4_ok(int sock, const struct net_sockaddr *addr,
			net_socklen_t addrlen)
{
	TEST_ASSERT_EQUAL(FD, sock);
	TEST_ASSERT_EQUAL(NET_AF_INET, addr->sa_family);
	return 0;
}

int z_impl_zsock_connect_ipv4_ok_then_enetunreach(int sock, const struct net_sockaddr *addr,
			net_socklen_t addrlen)
{
	TEST_ASSERT_EQUAL(FD, sock);
	TEST_ASSERT_EQUAL(NET_AF_INET, addr->sa_family);

	switch (z_impl_zsock_connect_fake.call_count) {
	case 1:
		return 0;
	}

	errno = ENETUNREACH;
	return -1;
}

int z_impl_zsock_connect_ipv6_ok(int sock, const struct net_sockaddr *addr,
			net_socklen_t addrlen)
{
	TEST_ASSERT_EQUAL(FD, sock);
	TEST_ASSERT_EQUAL(NET_AF_INET6, addr->sa_family);
	return 0;
}

int z_impl_zsock_connect_ipv6_then_ipv4_ok(int sock, const struct net_sockaddr *addr,
			net_socklen_t addrlen)
{
	switch (z_impl_zsock_socket_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL(FD, sock);
		TEST_ASSERT_EQUAL(NET_AF_INET6, addr->sa_family);
		break;
	case 2:
		TEST_ASSERT_EQUAL(FD, sock);
		TEST_ASSERT_EQUAL(NET_AF_INET, addr->sa_family);
		break;
	}

	return 0;
}

int z_impl_zsock_connect_ipv6_fails_ipv4_ok(int sock, const struct net_sockaddr *addr,
			net_socklen_t addrlen)
{
	TEST_ASSERT_EQUAL(FD, sock);

	if (addr->sa_family == NET_AF_INET6) {
		return -EHOSTUNREACH;
	}

	return 0;
}


int z_impl_zsock_connect_enetunreach(int sock, const struct net_sockaddr *addr,
			net_socklen_t addrlen)
{
	errno = ENETUNREACH;
	return -1;
}


int z_impl_zsock_setsockopt_http_ok(int sock, int level, int optname, const void *optval,
			       net_socklen_t optlen)
{
	switch (z_impl_zsock_setsockopt_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(SO_BINDTOPDN, optname);
		TEST_ASSERT_EQUAL(sizeof(int), optlen);
		break;
	case 2:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(ZSOCK_SO_RCVTIMEO, optname);
		TEST_ASSERT_EQUAL(sizeof(struct timeval), optlen);
		/* Ignore value */
		break;
	}

	return 0;
}

int z_impl_zsock_setsockopt_coap_ok(int sock, int level, int optname, const void *optval,
			       net_socklen_t optlen)
{
	switch (z_impl_zsock_setsockopt_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(SO_BINDTOPDN, optname);
		TEST_ASSERT_EQUAL(sizeof(int), optlen);
		break;
	case 2:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(ZSOCK_SO_SNDTIMEO, optname);
		TEST_ASSERT_EQUAL(sizeof(struct timeval), optlen);
		/* Ignore value */
		break;
	case 3:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(ZSOCK_SO_RCVTIMEO, optname);
		TEST_ASSERT_EQUAL(sizeof(struct timeval), optlen);
		/* Ignore value */
		break;
	}

	return 0;
}

int z_impl_zsock_setsockopt_https_ok(int sock, int level, int optname, const void *optval,
			       net_socklen_t optlen)
{
	TEST_ASSERT_EQUAL(FD, sock);

	switch (z_impl_zsock_setsockopt_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(SO_BINDTOPDN, optname);
		TEST_ASSERT_EQUAL(sizeof(int), optlen);
		break;
	case 2:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_TLS, level);
		TEST_ASSERT_EQUAL(ZSOCK_TLS_PEER_VERIFY, optname);
		TEST_ASSERT_EQUAL(4, optlen);
		TEST_ASSERT_EQUAL(2, *(int *)optval);
		break;
	case 3:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_TLS, level);
		TEST_ASSERT_EQUAL(ZSOCK_TLS_SEC_TAG_LIST, optname);
		TEST_ASSERT_EQUAL(sizeof(sec_tags), optlen);
		TEST_ASSERT_EQUAL_MEMORY(sec_tags, optval, sizeof(sec_tags));
		break;
	case 4:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_TLS, level);
		TEST_ASSERT_EQUAL(ZSOCK_TLS_HOSTNAME, optname);
		TEST_ASSERT_EQUAL(strlen(HOSTNAME), optlen);
		TEST_ASSERT_EQUAL_MEMORY(HOSTNAME, optval, strlen(HOSTNAME));
		break;
	case 5:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(ZSOCK_SO_RCVTIMEO, optname);
		TEST_ASSERT_EQUAL(sizeof(struct timeval), optlen);
		/* Ignore value */
		break;
	}

	return 0;
}

int z_impl_zsock_setsockopt_coaps_ok(int sock, int level, int optname, const void *optval,
			       net_socklen_t optlen)
{
	TEST_ASSERT_EQUAL(FD, sock);

	switch (z_impl_zsock_setsockopt_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(SO_BINDTOPDN, optname);
		TEST_ASSERT_EQUAL(sizeof(int), optlen);
		break;
	case 2:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_TLS, level);
		TEST_ASSERT_EQUAL(ZSOCK_TLS_PEER_VERIFY, optname);
		TEST_ASSERT_EQUAL(4, optlen);
		TEST_ASSERT_EQUAL(2, *(int *)optval);
		break;
	case 3:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_TLS, level);
		TEST_ASSERT_EQUAL(ZSOCK_TLS_SEC_TAG_LIST, optname);
		TEST_ASSERT_EQUAL(sizeof(sec_tags), optlen);
		TEST_ASSERT_EQUAL_MEMORY(sec_tags, optval, sizeof(sec_tags));
		break;
	case 4:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(ZSOCK_SO_SNDTIMEO, optname);
		TEST_ASSERT_EQUAL(sizeof(struct timeval), optlen);
		/* Ignore value */
		break;
	case 5:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(ZSOCK_SO_RCVTIMEO, optname);
		TEST_ASSERT_EQUAL(sizeof(struct timeval), optlen);
		/* Ignore value */
		break;
	}

	return 0;
}


int z_impl_zsock_setsockopt_coaps_fail_on_pdn(int sock, int level, int optname, const void *optval,
			       net_socklen_t optlen)
{
	TEST_ASSERT_EQUAL(FD, sock);

	if (optname == SO_BINDTOPDN) {
		errno = ENOMEM;
		return -1;
	}

	return 0;
}

int z_impl_zsock_setsockopt_coaps_fail_on_ZSOCK_TLS_PEER_VERIFY(int sock, int level, int optname,
	const void *optval, net_socklen_t optlen)
{
	TEST_ASSERT_EQUAL(FD, sock);

	if (optname == ZSOCK_TLS_PEER_VERIFY) {
		errno = ENOMEM;
		return -1;
	}

	return 0;
}

int z_impl_zsock_setsockopt_coaps_fail_on_sec_tag_list(int sock, int level, int optname,
	const void *optval, net_socklen_t optlen)
{
	TEST_ASSERT_EQUAL(FD, sock);

	if (optname == ZSOCK_TLS_SEC_TAG_LIST) {
		errno = ENOMEM;
		return -1;
	}

	return 0;
}

int z_impl_zsock_setsockopt_coaps_fail_on_sndtimeo(int sock, int level, int optname,
	const void *optval, net_socklen_t optlen)
{
	TEST_ASSERT_EQUAL(FD, sock);

	if (optname == ZSOCK_SO_SNDTIMEO) {
		errno = ENOMEM;
		return -1;
	}

	return 0;
}

int z_impl_zsock_setsockopt_coaps_fail_on_rcvtimeo(int sock, int level, int optname,
	const void *optval, net_socklen_t optlen)
{
	TEST_ASSERT_EQUAL(FD, sock);


	if (optname == ZSOCK_SO_RCVTIMEO) {
		errno = ENOMEM;
		return -1;
	}

	return 0;
}

int z_impl_zsock_setsockopt_coaps_fail_on_cid(int sock, int level, int optname,
	const void *optval, net_socklen_t optlen)
{
	TEST_ASSERT_EQUAL(FD, sock);


	if (optname == ZSOCK_TLS_DTLS_CID) {
		errno = ENOMEM;
		return -1;
	}

	return 0;
}

int z_impl_zsock_setsockopt_coaps_cid_ok(int sock, int level, int optname, const void *optval,
			       net_socklen_t optlen)
{
	TEST_ASSERT_EQUAL(FD, sock);

	switch (z_impl_zsock_setsockopt_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(SO_BINDTOPDN, optname);
		TEST_ASSERT_EQUAL(sizeof(int), optlen);
		break;
	case 2:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_TLS, level);
		TEST_ASSERT_EQUAL(ZSOCK_TLS_PEER_VERIFY, optname);
		TEST_ASSERT_EQUAL(4, optlen);
		TEST_ASSERT_EQUAL(2, *(int *)optval);
		break;
	case 3:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_TLS, level);
		TEST_ASSERT_EQUAL(ZSOCK_TLS_SEC_TAG_LIST, optname);
		TEST_ASSERT_EQUAL(sizeof(sec_tags), optlen);
		TEST_ASSERT_EQUAL_MEMORY(sec_tags, optval, sizeof(sec_tags));
		break;
	case 4:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_TLS, level);
		TEST_ASSERT_EQUAL(ZSOCK_TLS_DTLS_CID, optname);
		TEST_ASSERT_EQUAL(sizeof(uint32_t), optlen);
		break;
	case 5:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(ZSOCK_SO_SNDTIMEO, optname);
		TEST_ASSERT_EQUAL(sizeof(struct timeval), optlen);
		/* Ignore value */
		break;
	case 6:
		TEST_ASSERT_EQUAL(ZSOCK_SOL_SOCKET, level);
		TEST_ASSERT_EQUAL(ZSOCK_SO_RCVTIMEO, optname);
		TEST_ASSERT_EQUAL(sizeof(struct timeval), optlen);
		/* Ignore value */
		break;
	}

	return 0;
}

ssize_t z_impl_zsock_sendto_ok(int sock, const void *buf, size_t len, int flags,
			   const struct net_sockaddr *dest_addr, net_socklen_t addrlen)
{
	TEST_ASSERT_EQUAL(FD, sock);
	return len;
}

static ssize_t z_impl_zsock_recvfrom_http_header_then_data(
	int sock, void *buf, size_t max_len, int flags, struct net_sockaddr *src_addr,
	net_socklen_t *addrlen)
{
	TEST_ASSERT_EQUAL(FD, sock);
	TEST_ASSERT(sizeof(dl_buf) >= max_len);

	switch (z_impl_zsock_recvfrom_fake.call_count) {
	case 1:
		memcpy(buf, HTTP_HDR_OK, strlen(HTTP_HDR_OK));
		return strlen(HTTP_HDR_OK);
	case 2:
		memset(buf, 23, 128);
		return 128;
	}

	return 0;
}

static ssize_t z_impl_zsock_recvfrom_http_partial_header_then_header_with_data(
	int sock, void *buf, size_t max_len, int flags, struct net_sockaddr *src_addr,
	net_socklen_t *addrlen)
{
	TEST_ASSERT_EQUAL(FD, sock);
	TEST_ASSERT(sizeof(dl_buf) >= max_len);

	switch (z_impl_zsock_recvfrom_fake.call_count) {
	case 1:
		memcpy(buf, HTTP_HDR_OK_PARTIAL_1, strlen(HTTP_HDR_OK_PARTIAL_1));
		return strlen(HTTP_HDR_OK_PARTIAL_1);
	case 2:
		memcpy(buf, HTTP_HDR_OK_PARTIAL_2, strlen(HTTP_HDR_OK_PARTIAL_2));
		memset((char *)buf + strlen(HTTP_HDR_OK_PARTIAL_2), 23, 128);
		return strlen(HTTP_HDR_OK_PARTIAL_2) + 128;
	}

	return 0;
}

static ssize_t z_impl_zsock_recvfrom_https_partial_content(
	int sock, void *buf, size_t max_len, int flags, struct net_sockaddr *src_addr,
	net_socklen_t *addrlen)
{
	TEST_ASSERT_EQUAL(FD, sock);
	TEST_ASSERT(sizeof(dl_buf) >= max_len);

	switch (z_impl_zsock_recvfrom_fake.call_count) {
	case 1:
		memcpy(buf, HTTPS_HDR_OK_PARTIAL_CONTENT_1, strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_1));
		memset((char *)buf + strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_1), 23, 32);
		return strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_1) + 32;
	case 2:
		memcpy(buf, HTTPS_HDR_OK_PARTIAL_CONTENT_2, strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_2));
		memset((char *)buf + strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_2), 23, 32);
		return strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_2) + 16;
	case 3:
	case 4:
		memset(buf, ' ', 8);
		return 8;
	}

	return 0;
}

static ssize_t z_impl_zsock_recvfrom_https_partial_content_partial_2nd_header(
	int sock, void *buf, size_t max_len, int flags, struct net_sockaddr *src_addr,
	net_socklen_t *addrlen)
{
	TEST_ASSERT_EQUAL(FD, sock);
	TEST_ASSERT(sizeof(dl_buf) >= max_len);

	switch (z_impl_zsock_recvfrom_fake.call_count) {
	case 1:
		memcpy(buf, HTTPS_HDR_OK_PARTIAL_CONTENT_1, strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_1));
		memset((char *)buf + strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_1), 23, 32);
		return strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_1) + 32;
	case 2:
		memcpy(buf, HTTPS_HDR_OK_PARTIAL_CONTENT_HDR_2_1,
		       strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_HDR_2_1));
		return strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_HDR_2_1);
	case 3:
		memcpy(buf, HTTPS_HDR_OK_PARTIAL_CONTENT_HDR_2_2,
		       strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_HDR_2_2));
		memset((char *)buf + strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_HDR_2_2), 23, 32);
		return strlen(HTTPS_HDR_OK_PARTIAL_CONTENT_HDR_2_2) + 16;
	case 4:
	case 5:
		memset(buf, ' ', 8);
		return 8;
	}

	return 0;
}

static ssize_t z_impl_zsock_recvfrom_http_header_and_payload(
	int sock, void *buf, size_t max_len, int flags, struct net_sockaddr *src_addr,
	net_socklen_t *addrlen)
{
	switch (z_impl_zsock_recvfrom_fake.call_count) {
	case 1:
		memcpy(buf, HTTP_HDR_OK_WITH_PAYLOAD, strlen(HTTP_HDR_OK_WITH_PAYLOAD));
		return strlen(HTTP_HDR_OK_WITH_PAYLOAD);
	}

	return 0;
}

static ssize_t z_impl_zsock_recvfrom_http_header_and_frag_data_w_err(
	int sock, void *buf, size_t max_len, int flags, struct net_sockaddr *src_addr,
	net_socklen_t *addrlen)
{
	switch (z_impl_zsock_recvfrom_fake.call_count) {
	case 1:
		memcpy(buf, HTTP_HDR_OK, strlen(HTTP_HDR_OK));
		return strlen(HTTP_HDR_OK);
	case 2:
		memset(buf, 23, 32);
		return 32;
	case 3:
		/* connection reset */
		errno = ECONNRESET;
		return -1;
	case 4:
		memcpy(buf, HTTP_HDR_OK_PROGRESS, strlen(HTTP_HDR_OK_PROGRESS));
		return strlen(HTTP_HDR_OK_PROGRESS);
	}

	return 32;
}

static ssize_t z_impl_zsock_recvfrom_http_header_and_frag_data_peer_close(
	int sock, void *buf, size_t max_len, int flags, struct net_sockaddr *src_addr,
	net_socklen_t *addrlen)
{
	switch (z_impl_zsock_recvfrom_fake.call_count) {
	case 1:
		memcpy(buf, HTTP_HDR_OK, strlen(HTTP_HDR_OK));
		return strlen(HTTP_HDR_OK);
	case 2:
		memset(buf, 23, 32);
		return 32;
	case 3:
		/* connection closed */
		return 0;
	case 4:
		memcpy(buf, HTTP_HDR_OK_PROGRESS, strlen(HTTP_HDR_OK_PROGRESS));
		return strlen(HTTP_HDR_OK_PROGRESS);
	}

	return 32;
}

static ssize_t z_impl_zsock_recvfrom_partial_then_econnreset(
	int sock, void *buf, size_t max_len, int flags, struct net_sockaddr *src_addr,
	net_socklen_t *addrlen)
{
	TEST_ASSERT_EQUAL(FD, sock);
	TEST_ASSERT(sizeof(dl_buf) >= max_len);

	switch (z_impl_zsock_recvfrom_fake.call_count) {
	case 1:
		memcpy(buf, HTTP_HDR_OK, strlen(HTTP_HDR_OK));
		return strlen(HTTP_HDR_OK);
	case 2:
		memset(buf, 23, 32);
		return 32;
	}

	return -ECONNRESET;
}

static ssize_t z_impl_zsock_recvfrom_http_redirect_and_close(
	int sock, void *buf, size_t max_len, int flags, struct net_sockaddr *src_addr,
	net_socklen_t *addrlen)
{
	switch (z_impl_zsock_recvfrom_fake.call_count) {
	case 1:
		memcpy(buf, HTTP_HDR_REDIRECT, strlen(HTTP_HDR_REDIRECT));
		return strlen(HTTP_HDR_REDIRECT);
	case 2:
		/* connection closed */
		return 0;
	}

	memcpy(buf, HTTP_HDR_REDIRECT, strlen(HTTP_HDR_REDIRECT));
	return strlen(HTTP_HDR_REDIRECT);
}

int z_impl_zsock_socket_http_then_https(int family, int type, int proto)
{
	switch (z_impl_zsock_socket_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL(NET_SOCK_STREAM, type);
		TEST_ASSERT_EQUAL(NET_IPPROTO_TCP, proto);
		break;
	case 2:
	default:
		TEST_ASSERT_EQUAL(NET_SOCK_STREAM, type);
		TEST_ASSERT_EQUAL(NET_IPPROTO_TLS_1_2, proto);
		RESET_FAKE(z_impl_zsock_setsockopt);
		RESET_FAKE(z_impl_zsock_recvfrom);
		z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_https_ok;
		z_impl_zsock_recvfrom_fake.custom_fake =
			z_impl_zsock_recvfrom_http_header_then_data;
	}

	return FD;
}

static ssize_t z_impl_zsock_recvfrom_coap(
	int sock, void *buf, size_t max_len, int flags, struct net_sockaddr *src_addr,
	net_socklen_t *addrlen)
{
	memset(buf, 23, 32);
	return 32;
}

static ssize_t z_impl_zsock_recvfrom_coap_timeout(int sock, void *buf, size_t max_len, int flags,
						  struct net_sockaddr *src_addr,
						  net_socklen_t *addrlen)
{
	errno = EAGAIN;
	return -1;
}

static int recovery_call_count;

static ssize_t z_impl_zsock_recvfrom_coap_timeout_then_data(int sock, void *buf, size_t max_len,
							    int flags,
							    struct net_sockaddr *src_addr,
							    net_socklen_t *addrlen)
{
	if (recovery_call_count == 0) {
		recovery_call_count++;
		errno = EAGAIN;
		return -1;
	}

	recovery_call_count++;
	return z_impl_zsock_recvfrom_coap(sock, buf, max_len, flags, src_addr, addrlen);
}

int coap_get_option_int_ok(const struct coap_packet *cpkt, uint16_t code)
{
	return 0;
}

int coap_block_transfer_init_ok(struct coap_block_context *ctx,
			      enum coap_block_size block_size,
			      size_t total_size)
{
	return 0;
}

void coap_pending_clear_ok(struct coap_pending *pending)
{
	/* empty */
}

bool coap_pending_cycle_ok(struct coap_pending *pending)
{
	pending->timeout = 10000;
	return true;
}

bool coap_pending_cycles(struct coap_pending *pending)
{
	pending->timeout = 1000 * (TEST_COAP_RETRIES - coap_pending_cycle_fake.call_count);

	if (pending->timeout) {
		return true;
	}

	return false;
}

int coap_update_from_block_ok(const struct coap_packet *cpkt,
			   struct coap_block_context *ctx)
{
	return 0;
}

size_t coap_next_block_empty(const struct coap_packet *cpkt,
		       struct coap_block_context *ctx)
{
	return 0;
}

int coap_packet_parse_ok(struct coap_packet *cpkt, uint8_t *data, uint16_t len,
		      struct coap_option *options, uint8_t opt_num)
{
	return 0;
}

uint16_t coap_header_get_id_ok(const struct coap_packet *cpkt)
{
	return 0;
}

uint8_t coap_header_get_type_ack(const struct coap_packet *cpkt)
{
	return COAP_TYPE_ACK;
}

uint8_t coap_header_get_code_ok(const struct coap_packet *cpkt)
{
	return COAP_RESPONSE_CODE_OK;
}

uint8_t coap_header_get_code_bad_then_ok(const struct coap_packet *cpkt)
{
	switch (coap_header_get_code_fake.call_count) {
	case 1:
	return 0xba;
	default: return COAP_RESPONSE_CODE_OK;
	}


	return COAP_RESPONSE_CODE_OK;
}

uint8_t coap_header_get_code_bad(const struct coap_packet *cpkt)
{
	return 0xba;
}

#define COAP_PAYLOAD "This is the payload"
const uint8_t *coap_packet_get_payload_ok(const struct coap_packet *cpkt, uint16_t *len)
{
	*len = sizeof(COAP_PAYLOAD);

	return COAP_PAYLOAD;
}
int coap_packet_init_ok(struct coap_packet *cpkt, uint8_t *data, uint16_t max_len,
		     uint8_t ver, uint8_t type, uint8_t token_len,
		     const uint8_t *token, uint8_t code, uint16_t id)
{
	return 0;
}
uint8_t *coap_next_token_ok(void)
{
	static uint8_t token[COAP_TOKEN_MAX_LEN];

	return token;
}
int coap_packet_append_option_ok(struct coap_packet *cpkt, uint16_t code,
			      const uint8_t *value, uint16_t len)
{
	return 0;
}
int coap_append_block2_option_ok(struct coap_packet *cpkt,
			      struct coap_block_context *ctx)
{
	return 0;
}
int coap_append_size2_option_ok(struct coap_packet *cpkt,
			     struct coap_block_context *ctx)
{
	return 0;
}

struct coap_transmission_parameters coap_transmission_params = {
	.ack_timeout = 10000,
	.coap_backoff_percent = 50,
	.max_retransmission = 10,
};

struct coap_transmission_parameters coap_get_transmission_parameters_ok(void)
{
	return coap_transmission_params;
}

struct pipe {
	struct downloader_evt data[10];
	uint8_t wr_idx;
	uint8_t rd_idx;
};

static struct pipe event_pipe;
K_SEM_DEFINE(pipe_sem, 0, 10);

static void pipe_reset(struct pipe *pipe)
{
	memset(pipe, 0, sizeof(struct pipe));
}

static int pipe_put(struct pipe *pipe, const struct downloader_evt *evt)
{
	if (k_sem_count_get(&pipe_sem) >= 10) {
		return -ENOSPC;
	}

	memcpy(&pipe->data[pipe->wr_idx], evt, sizeof(struct downloader_evt));

	pipe->wr_idx++;
	pipe->wr_idx = CLAMP(pipe->wr_idx, 0,
			     (sizeof(pipe->data) / sizeof(struct downloader_evt)) - 1);

	k_sem_give(&pipe_sem);

	return 0;
}

static int pipe_get(struct pipe *pipe, struct downloader_evt *evt, k_timeout_t timeo)
{
	int err;

	err = k_sem_take(&pipe_sem, timeo);
	if (err) {
		return err;
	}

	memcpy(evt, &pipe->data[pipe->rd_idx], sizeof(struct downloader_evt));

	pipe->rd_idx++;
	pipe->rd_idx = CLAMP(pipe->rd_idx, 0,
			     (sizeof(pipe->data) / sizeof(struct downloader_evt)) - 1);

	return 0;
}

static const char *dl_event_id_str(int evt_id)
{
	switch (evt_id) {
	case DOWNLOADER_EVT_FRAGMENT: return "DOWNLOADER_EVT_FRAGMENT";
	case DOWNLOADER_EVT_ERROR: return "DOWNLOADER_EVT_ERROR";
	case DOWNLOADER_EVT_DONE: return "DOWNLOADER_EVT_DONE";
	case DOWNLOADER_EVT_STOPPED: return "DOWNLOADER_EVT_STOPPED";
	case DOWNLOADER_EVT_DEINITIALIZED: return "DOWNLOADER_EVT_DEINITIALIZED";
	}

	return "unknown";
}

static int dl_callback(const struct downloader_evt *event)
{
	TEST_ASSERT(event != NULL);

	printk("event: %s ", dl_event_id_str(event->id));
	if (event->id == DOWNLOADER_EVT_ERROR) {
		printk("reason: %d\n", event->error);
		/* avoid spamming error events during development */
		k_sleep(K_MSEC(100));
	} else if (event->id == DOWNLOADER_EVT_FRAGMENT) {
		printk("len %d\n", event->fragment.len);
	} else {
		printk("\n");
	}

	pipe_put(&event_pipe, event);

	return 0;
}

static int dl_callback_abort(const struct downloader_evt *event)
{
	TEST_ASSERT(event != NULL);

	printk("event: %s\n", dl_event_id_str(event->id));
	if (event->id == DOWNLOADER_EVT_ERROR) {
		/* avoid spamming error events during development */
		k_sleep(K_MSEC(100));
	}
	pipe_put(&event_pipe, event);

	return 1; /* stop download*/
}

static struct downloader_evt dl_wait_for_event(enum downloader_evt_id event,
						     k_timeout_t timeout)
{
	struct downloader_evt evt;
	int err;

	while (true) {
		err = pipe_get(&event_pipe, &evt, K_FOREVER);
		TEST_ASSERT_EQUAL(0, err);
		if (evt.id == event) {
			break;
		}
	}
	TEST_ASSERT_EQUAL(evt.id, event);
	return evt;
}

void test_downloader_init_einval(void)
{
	int err;
	char buf[1];
	struct downloader dl = {};
	struct downloader_cfg dl_cfg = {
		.buf = buf,
		.buf_size = ARRAY_SIZE(buf),
		.callback = dl_callback,
	};
	struct downloader_cfg dl_cfg_no_buf = {
		.buf = NULL,
		.buf_size = ARRAY_SIZE(buf),
		.callback = dl_callback,
	};
	struct downloader_cfg dl_cfg_no_buf_len = {
		.buf = buf,
		.buf_size = 0,
		.callback = dl_callback,
	};
	struct downloader_cfg dl_cfg_no_cb = {
		.buf = buf,
		.buf_size = ARRAY_SIZE(buf),
		.callback = NULL,
	};

	err = downloader_init(NULL, &dl_cfg);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_init(&dl, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_init(&dl, &dl_cfg_no_buf);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_init(&dl, &dl_cfg_no_buf_len);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_init(&dl, &dl_cfg_no_cb);
	TEST_ASSERT_EQUAL(-EINVAL, err);
}

void test_downloader_deinit_einval(void)
{
	int err;

	err = downloader_deinit(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, err);
}

void test_downloader_deinit_eperm(void)
{
	int err;
	struct downloader dl = {};

	err = downloader_deinit(&dl);
	TEST_ASSERT_EQUAL(-EPERM, err);

	dl.state = 0xffffffff;
	/* bad state */
	err = downloader_deinit(&dl);
	TEST_ASSERT_EQUAL(-EPERM, err);
}

void test_downloader_get_eperm(void)
{
	int err;

	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(-EPERM, err);
}

void test_downloader_get_http(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ipv6_fail_ipv4_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv4_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv4_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;

	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_http_partial_header(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ipv6_fail_ipv4_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv4_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv4_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake =
		z_impl_zsock_recvfrom_http_partial_header_then_header_with_data;

	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_https(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	err = downloader_transport_http_set_config(&dl, &dl_http_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_https_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_https_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;

	err = downloader_get(&dl, &dl_host_conf_w_sec_tags, HTTPS_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_https_partial_content(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_https_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_https_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_https_partial_content;

	err = downloader_get(&dl, &dl_host_conf_w_sec_tags_range_override_32, HTTPS_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_https_partial_content_partial_2nd_header(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_https_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_https_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake =
		z_impl_zsock_recvfrom_https_partial_content_partial_2nd_header;

	err = downloader_get(&dl, &dl_host_conf_w_sec_tags_range_override_32, HTTPS_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_https_unlimited_redirect(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_https_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_https_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_redirect_and_close;


	err = downloader_get(&dl, &dl_host_conf_w_sec_tags, HTTPS_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_ERROR, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_http_connect_enetunreach(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg_cb_abort);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ipv6_fail_ipv4_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv4_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_enetunreach;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;


	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_ERROR, K_SECONDS(1));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_http_getaddr_failed(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_enetunreach;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv4_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv4_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;

	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_ERROR, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_default_proto_http(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg_cb_abort);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ipv6_fail_ipv4_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv4_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv4_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;

	err = downloader_get(&dl, &dl_host_cfg, NO_PROTO_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_default_proto_https(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg_cb_abort);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_https_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_https_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;

	err = downloader_get(&dl, &dl_host_conf_w_sec_tags, NO_PROTO_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_reconnect_on_socket_error(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake =
		z_impl_zsock_recvfrom_http_header_and_frag_data_w_err;

	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_reconnect_on_peer_close(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake =
		z_impl_zsock_recvfrom_http_header_and_frag_data_peer_close;

	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_coap(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_coap_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_coap_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_coap;

	coap_get_transmission_parameters_fake.custom_fake = coap_get_transmission_parameters_ok;
	coap_pending_cycle_fake.custom_fake = coap_pending_cycle_ok;
	coap_header_get_type_fake.custom_fake = coap_header_get_type_ack;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_ok;
	coap_packet_get_payload_fake.custom_fake = coap_packet_get_payload_ok;

	err = downloader_get(&dl, &dl_host_cfg, COAP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_coaps(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_coaps_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_coaps_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_coap;

	coap_get_transmission_parameters_fake.custom_fake = coap_get_transmission_parameters_ok;
	coap_pending_cycle_fake.custom_fake = coap_pending_cycle_ok;
	coap_header_get_type_fake.custom_fake = coap_header_get_type_ack;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_ok;
	coap_packet_get_payload_fake.custom_fake = coap_packet_get_payload_ok;

	err = downloader_get(&dl, &dl_host_conf_w_sec_tags, COAPS_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_coaps_setsockopt_fail_pdn(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6_then_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_coaps_ipv6_then_ipv4_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_coaps_fail_on_pdn;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_coap;

	coap_get_transmission_parameters_fake.custom_fake = coap_get_transmission_parameters_ok;
	coap_pending_cycle_fake.custom_fake = coap_pending_cycle_ok;
	coap_header_get_type_fake.custom_fake = coap_header_get_type_ack;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_ok;
	coap_packet_get_payload_fake.custom_fake = coap_packet_get_payload_ok;

	err = downloader_get(&dl, &dl_host_conf_w_sec_tags, COAPS_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_ERROR, K_SECONDS(3));

	err = downloader_cancel(&dl);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_coaps_setsockopt_fail_sec_tag_list(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6_then_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_coaps_ipv6_then_ipv4_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_then_ipv4_ok;
	z_impl_zsock_setsockopt_fake.custom_fake =
		z_impl_zsock_setsockopt_coaps_fail_on_sec_tag_list;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_coap;

	coap_get_transmission_parameters_fake.custom_fake = coap_get_transmission_parameters_ok;
	coap_pending_cycle_fake.custom_fake = coap_pending_cycle_ok;
	coap_header_get_type_fake.custom_fake = coap_header_get_type_ack;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_ok;
	coap_packet_get_payload_fake.custom_fake = coap_packet_get_payload_ok;

	err = downloader_get(&dl, &dl_host_conf_w_sec_tags, COAPS_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_ERROR, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_coaps_setsockopt_fail_sndtimeo(void)
{
	int err;
	struct downloader_evt evt;
	struct downloader_transport_coap_cfg coap_cfg = {
		.block_size = COAP_BLOCK_512,
		.max_retransmission = 10,
	};

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	err = downloader_transport_coap_set_config(&dl, &coap_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_coaps_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake =
		z_impl_zsock_setsockopt_coaps_fail_on_sndtimeo;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_coap;

	coap_get_transmission_parameters_fake.custom_fake = coap_get_transmission_parameters_ok;
	coap_pending_cycle_fake.custom_fake = coap_pending_cycle_ok;
	coap_header_get_type_fake.custom_fake = coap_header_get_type_ack;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_ok;
	coap_packet_get_payload_fake.custom_fake = coap_packet_get_payload_ok;

	err = downloader_get(&dl, &dl_host_conf_w_sec_tags, COAPS_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_ERROR, K_SECONDS(3));

	err = downloader_cancel(&dl);
	evt = dl_wait_for_event(DOWNLOADER_EVT_STOPPED, K_SECONDS(3));


	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_coaps_setsockopt_fail_rcvtimeo(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6_then_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_coaps_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake =
		z_impl_zsock_setsockopt_coaps_fail_on_rcvtimeo;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_coap;

	coap_get_transmission_parameters_fake.custom_fake = coap_get_transmission_parameters_ok;
	coap_pending_cycle_fake.custom_fake = coap_pending_cycle_ok;
	coap_header_get_type_fake.custom_fake = coap_header_get_type_ack;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_ok;
	coap_packet_get_payload_fake.custom_fake = coap_packet_get_payload_ok;

	err = downloader_get(&dl, &dl_host_conf_w_sec_tags, COAPS_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_ERROR, K_SECONDS(3));

	err = downloader_cancel(&dl);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_coaps_setsockopt_fail_cid(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6_then_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_coaps_ipv6_then_ipv4_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake =
		z_impl_zsock_setsockopt_coaps_fail_on_cid;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_coap;

	coap_get_transmission_parameters_fake.custom_fake = coap_get_transmission_parameters_ok;
	coap_pending_cycle_fake.custom_fake = coap_pending_cycle_ok;
	coap_header_get_type_fake.custom_fake = coap_header_get_type_ack;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_ok;
	coap_packet_get_payload_fake.custom_fake = coap_packet_get_payload_ok;

	err = downloader_get(&dl, &dl_host_conf_w_sec_tags_and_cid, COAPS_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_ERROR, K_SECONDS(3));

	err = downloader_cancel(&dl);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_coaps_cid(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_coaps_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_coaps_cid_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_coap;

	coap_get_transmission_parameters_fake.custom_fake = coap_get_transmission_parameters_ok;
	coap_pending_cycle_fake.custom_fake = coap_pending_cycle_ok;
	coap_header_get_type_fake.custom_fake = coap_header_get_type_ack;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_ok;
	coap_packet_get_payload_fake.custom_fake = coap_packet_get_payload_ok;

	err = downloader_get(&dl, &dl_host_conf_w_sec_tags_and_cid, COAPS_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_coap_one_bad_header_code(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg_cb_abort);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_coap_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_coap_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_coap;

	coap_get_transmission_parameters_fake.custom_fake = coap_get_transmission_parameters_ok;
	coap_pending_cycle_fake.custom_fake = coap_pending_cycle_ok;
	coap_header_get_type_fake.custom_fake = coap_header_get_type_ack;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_bad_then_ok;
	coap_packet_get_payload_fake.custom_fake = coap_packet_get_payload_ok;

	err = downloader_get(&dl, &dl_host_cfg, COAP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_coap_bad_header_code_timeout(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg_cb_abort);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_coap_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_coap_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_coap;

	coap_get_transmission_parameters_fake.custom_fake = coap_get_transmission_parameters_ok;
	coap_pending_cycle_fake.custom_fake = coap_pending_cycles;
	coap_header_get_type_fake.custom_fake = coap_header_get_type_ack;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_bad;
	coap_packet_get_payload_fake.custom_fake = coap_packet_get_payload_ok;

	err = downloader_get(&dl, &dl_host_cfg, COAP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_ERROR, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_coap_retransmission_timeout(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_coap_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_coap_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_coap_timeout;

	coap_get_transmission_parameters_fake.custom_fake = coap_get_transmission_parameters_ok;
	coap_pending_cycle_fake.custom_fake = coap_pending_cycles;
	coap_header_get_type_fake.custom_fake = coap_header_get_type_ack;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_ok;
	coap_packet_get_payload_fake.custom_fake = coap_packet_get_payload_ok;

	err = downloader_get(&dl, &dl_host_cfg, COAP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_ERROR, K_SECONDS(3));
	TEST_ASSERT_EQUAL(-ETIMEDOUT, evt.error);
	TEST_ASSERT_EQUAL(TEST_EXPECTED_RECVFROM_CALLS, z_impl_zsock_recvfrom_fake.call_count);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_coap_retransmission_recovery(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_coap_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_coap_ok;

	RESET_FAKE(z_impl_zsock_sendto);
	RESET_FAKE(z_impl_zsock_recvfrom);
	recovery_call_count = 0;

	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_coap_timeout_then_data;

	coap_get_transmission_parameters_fake.custom_fake = coap_get_transmission_parameters_ok;
	RESET_FAKE(coap_pending_cycle);
	coap_pending_cycle_fake.custom_fake = coap_pending_cycles;
	coap_header_get_type_fake.custom_fake = coap_header_get_type_ack;
	coap_header_get_code_fake.custom_fake = coap_header_get_code_ok;
	coap_packet_get_payload_fake.custom_fake = coap_packet_get_payload_ok;

	err = downloader_get(&dl, &dl_host_cfg, COAP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));
	TEST_ASSERT_EQUAL(DOWNLOADER_EVT_DONE, evt.id);

	TEST_ASSERT_EQUAL(2, coap_pending_cycle_fake.call_count);
	TEST_ASSERT_EQUAL(2, z_impl_zsock_recvfrom_fake.call_count);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_einval(void)
{
	int err;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	err = downloader_get(NULL, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_get(&dl, NULL, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_get(&dl, &dl_host_cfg, NULL, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_get(&dl, &dl_host_cfg, BAD_URL_NO_FILE, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	/* https url, no sec tag specified */
	err = downloader_get(&dl, &dl_host_cfg, HTTPS_URL, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	/* coaps url, no sec tag specified */
	err = downloader_get(&dl, &dl_host_cfg, COAPS_URL, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_eprotonotsupport(void)
{
	int err;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	err = downloader_get(&dl, &dl_host_cfg, BAD_URL, 0);
	TEST_ASSERT_EQUAL(-EPROTONOSUPPORT, err);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;


	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_two_files_same_host(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;


	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	z_impl_zsock_recvfrom_fake.call_count = 0;

	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL_FILE2, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));

	/* make sure we only connected once */
	TEST_ASSERT_EQUAL(1, z_impl_zsock_connect_fake.call_count);
}

void test_downloader_get_two_files_different_host(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;


	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	z_impl_zsock_recvfrom_fake.call_count = 0;

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server2_ok;

	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL_HOST2, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));

	/* make sure we reconnected */
	TEST_ASSERT_EQUAL(2, z_impl_zsock_connect_fake.call_count);
}

void test_downloader_get_hdr_and_payload(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_and_payload;


	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_FRAGMENT, K_SECONDS(3));
	TEST_ASSERT_EQUAL(20, evt.fragment.len);
	TEST_ASSERT_EQUAL_MEMORY(PAYLOAD, evt.fragment.buf, evt.fragment.len);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_ipv6_fails_to_connect_ipv4_success(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6_then_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv6_then_ipv4;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_fails_ipv4_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;

	err = downloader_get(&dl, &dl_host_cfg, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_ipv4_specific_ok(void)
{
	int err;
	struct downloader_evt evt;
	static struct downloader_host_cfg dl_host_conf_ipv4 = {
		.pdn_id = 1,
		.family = NET_AF_INET,
	};

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv4_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv4_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;

	err = downloader_get(&dl, &dl_host_conf_ipv4, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_econnreset_reconnect_fails(void)
{
	int err;
	struct downloader_evt evt;
	static struct downloader_host_cfg dl_host_conf_ipv4 = {
		.pdn_id = 1,
		.family = NET_AF_INET,
	};

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv4_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv4_ok_then_enetunreach;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_partial_then_econnreset;

	err = downloader_get(&dl, &dl_host_conf_ipv4, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_FRAGMENT, K_SECONDS(3));

	evt = dl_wait_for_event(DOWNLOADER_EVT_ERROR, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_cancel_einval(void)
{
	int err;

	err = downloader_cancel(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, err);

}

void test_downloader_cancel_eperm(void)
{
	int err;

	err = downloader_cancel(&dl);
	TEST_ASSERT_EQUAL(-EPERM, err);
}

void test_downloader_cancel(void)
{
	int err;
	struct downloader_evt evt;
	static struct downloader_host_cfg dl_host_conf_ipv4 = {
		.pdn_id = 1,
		.family = NET_AF_INET,
	};

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv4;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv4_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv4_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_partial_then_econnreset;

	err = downloader_get(&dl, &dl_host_conf_ipv4, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_FRAGMENT, K_SECONDS(3));

	err = downloader_cancel(&dl);
	TEST_ASSERT_EQUAL(0, err);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_with_host_and_file_einval(void)
{
	int err;

	err = downloader_init(&dl, &dl_cfg_small_buffer);
	TEST_ASSERT_EQUAL(0, err);

	err = downloader_get_with_host_and_file(NULL, &dl_host_cfg, HTTP_HOST, FILE_PATH, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_get_with_host_and_file(&dl, NULL, HTTP_HOST, FILE_PATH, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_get_with_host_and_file(&dl, &dl_host_cfg, NULL, FILE_PATH, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_get_with_host_and_file(&dl, &dl_host_cfg, HTTP_HOST, NULL, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_get_with_host_and_file(&dl, &dl_host_cfg, HTTP_HOST, FILE_PATH_LONG, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_get_with_host_and_file_eperm(void)
{
	int err;

	err = downloader_get_with_host_and_file(&dl, &dl_host_cfg, HTTP_HOST, FILE_PATH, 0);
	TEST_ASSERT_EQUAL(-EPERM, err);
}

void test_downloader_get_with_host_and_file(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;


	err = downloader_get_with_host_and_file(&dl, &dl_host_cfg, HTTP_HOST, FILE_PATH, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_file_size_get_einval(void)
{
	int err;
	size_t fs;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	err = downloader_file_size_get(NULL, &fs);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_file_size_get(&dl, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_file_size_get_eperm(void)
{
	int err;
	size_t fs;

	err = downloader_file_size_get(&dl, &fs);
	TEST_ASSERT_EQUAL(-EPERM, err);
}

void test_downloader_file_size_get(void)
{
	int err;
	struct downloader_evt evt;
	size_t fs;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;


	err = downloader_get_with_host_and_file(&dl, &dl_host_cfg, HTTP_HOST, FILE_PATH, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_FRAGMENT, K_SECONDS(3));

	err = downloader_file_size_get(&dl, &fs);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(128, fs);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_downloaded_size_get_einval(void)
{
	int err;
	size_t fs;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	err = downloader_downloaded_size_get(NULL, &fs);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = downloader_downloaded_size_get(&dl, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_downloaded_size_get_eperm(void)
{
	int err;
	size_t fs;

	err = downloader_downloaded_size_get(&dl, &fs);
	TEST_ASSERT_EQUAL(-EPERM, err);
}

void test_downloader_downloaded_size_get(void)
{
	int err;
	struct downloader_evt evt;
	size_t fs;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_ipv6_ok;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_header_then_data;

	err = downloader_downloaded_size_get(&dl, &fs);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(0, fs);

	err = downloader_get_with_host_and_file(&dl, &dl_host_cfg, HTTP_HOST, FILE_PATH, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_FRAGMENT, K_SECONDS(3));

	err = downloader_downloaded_size_get(&dl, &fs);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(128, fs);

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));
}

void test_downloader_http_redirect_to_https(void)
{
	int err;
	struct downloader_evt evt;

	err = downloader_init(&dl, &dl_cfg);
	TEST_ASSERT_EQUAL(0, err);

	zsock_getaddrinfo_fake.custom_fake = zsock_getaddrinfo_server_ok;
	zsock_freeaddrinfo_fake.custom_fake = zsock_freeaddrinfo_server_ipv6;
	z_impl_zsock_socket_fake.custom_fake = z_impl_zsock_socket_http_then_https;
	z_impl_zsock_connect_fake.custom_fake = z_impl_zsock_connect_ipv6_ok;
	z_impl_zsock_setsockopt_fake.custom_fake = z_impl_zsock_setsockopt_http_ok;
	z_impl_zsock_sendto_fake.custom_fake = z_impl_zsock_sendto_ok;
	z_impl_zsock_recvfrom_fake.custom_fake = z_impl_zsock_recvfrom_http_redirect_and_close;


	err = downloader_get(&dl, &dl_host_conf_w_sec_tags, HTTP_URL, 0);
	TEST_ASSERT_EQUAL(0, err);

	evt = dl_wait_for_event(DOWNLOADER_EVT_DONE, K_SECONDS(3));

	downloader_deinit(&dl);
	dl_wait_for_event(DOWNLOADER_EVT_DEINITIALIZED, K_SECONDS(1));

}

void setUp(void)
{
	RESET_FAKE(z_impl_zsock_setsockopt);
	RESET_FAKE(z_impl_zsock_socket);
	RESET_FAKE(z_impl_zsock_connect);
	RESET_FAKE(z_impl_zsock_close);
	RESET_FAKE(z_impl_zsock_send);
	RESET_FAKE(z_impl_zsock_recv);
	RESET_FAKE(zsock_getaddrinfo);
	RESET_FAKE(zsock_freeaddrinfo);
	RESET_FAKE(z_impl_zsock_inet_pton);
	RESET_FAKE(z_impl_net_addr_ntop);
	RESET_FAKE(z_impl_zsock_sendto);
	RESET_FAKE(z_impl_zsock_recvfrom);

	RESET_FAKE(coap_get_option_int);
	RESET_FAKE(coap_block_transfer_init);
	RESET_FAKE(coap_pending_clear);
	RESET_FAKE(coap_pending_cycle);
	RESET_FAKE(coap_update_from_block);
	RESET_FAKE(coap_next_block);
	RESET_FAKE(coap_packet_parse);
	RESET_FAKE(coap_header_get_id);
	RESET_FAKE(coap_header_get_type);
	RESET_FAKE(coap_header_get_code);
	RESET_FAKE(coap_packet_get_payload);
	RESET_FAKE(coap_packet_init);
	RESET_FAKE(coap_next_token);
	RESET_FAKE(coap_packet_append_option);
	RESET_FAKE(coap_append_block2_option);
	RESET_FAKE(coap_append_size2_option);
	RESET_FAKE(coap_get_transmission_parameters);
	RESET_FAKE(coap_pending_init);

	pipe_reset(&event_pipe);
}

void tearDown(void)
{
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}
