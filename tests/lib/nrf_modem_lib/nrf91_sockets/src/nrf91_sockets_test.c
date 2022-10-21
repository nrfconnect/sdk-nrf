/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>

#include <zephyr/kernel.h>
#include <fcntl.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/net/socket.h>

#include <nrf_gai_errors.h>

#include "mock_nrf_socket.h"
#include "mock_nrf_modem_os.h"

#define HTTPS_HOSTNAME "example.com"
#define PORT 8080
#define WRONG_VALUE 4242

void setUp(void)
{
}

void tearDown(void)
{
}

static struct test_state_nrf_getaddrinfo {
	struct nrf_addrinfo res;
	int ret;
} test_state_nrf_getaddrinfo;

static struct test_state_nrf_accept {
	struct nrf_sockaddr nrf_addr;
	nrf_socklen_t nrf_addrlen;
	int ret;
} test_state_nrf_accept;

static struct test_state_nrf_recvfrom {
	int ret;
	struct nrf_sockaddr cliaddr;
	nrf_socklen_t address_len;
} test_state_nrf_recvfrom;

static struct test_state_nrf_poll {
	struct nrf_pollfd expected_in[1];
	struct nrf_pollfd expected_out[1];
	int ret;
} test_state_nrf_poll;

static int nrf_getaddrinfo_stub(const char *p_node, const char *p_service,
				const struct nrf_addrinfo *p_hints,
				struct nrf_addrinfo **pp_res,
				int cmock_num_calls)
{
	*pp_res = k_malloc(sizeof(struct nrf_addrinfo));
	memcpy(*pp_res, &test_state_nrf_getaddrinfo.res, sizeof(struct nrf_addrinfo));

	return test_state_nrf_getaddrinfo.ret;
}

static int nrf_accept_stub(int socket, struct nrf_sockaddr *address,
			   nrf_socklen_t *address_len, int cmock_num_calls)
{
	*address = test_state_nrf_accept.nrf_addr;
	*address_len = test_state_nrf_accept.nrf_addrlen;

	return test_state_nrf_accept.ret;
}

static void nrf_freeaddrinfo_stub(struct nrf_addrinfo *p_res, int cmock_num_calls)
{
	if (p_res) {
		k_free(p_res);
	}
}

static ssize_t nrf_recvfrom_stub(int socket, void *buffer, size_t length,
				 int flags, struct nrf_sockaddr *address,
				 nrf_socklen_t *address_len,
				 int cmock_num_calls)
{
	struct nrf_sockaddr *cliaddr = (struct nrf_sockaddr *)address;

	if (cliaddr && address_len) {
		cliaddr->sa_family = test_state_nrf_recvfrom.cliaddr.sa_family;
		*address_len = test_state_nrf_recvfrom.address_len;
	}

	return test_state_nrf_recvfrom.ret;
}


static int nrf_poll_stub(struct nrf_pollfd *p_fds, uint32_t nfds,
			 int timeout, int cmock_num_calls)
{
	TEST_ASSERT_EQUAL(p_fds[0].events,
			  test_state_nrf_poll.expected_in->events);

	p_fds[0].revents = test_state_nrf_poll.expected_out->revents;

	return test_state_nrf_poll.ret;
}

void test_nrf91_socket_offload_getaddrinfo_errors(void)
{
	int ret;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};

#define ERROR_SIZE 9
	int nrf_errors[ERROR_SIZE] = {
		NRF_EAI_AGAIN,
		NRF_EAI_BADFLAGS,
		NRF_EAI_FAIL,
		NRF_EAI_FAMILY,
		NRF_EAI_MEMORY,
		NRF_EAI_NONAME,
		NRF_EAI_SERVICE,
		NRF_EAI_SOCKTYPE,
		NRF_EAI_SYSTEM
	};
	int dns_errors[ERROR_SIZE] = {
		DNS_EAI_AGAIN,
		DNS_EAI_BADFLAGS,
		DNS_EAI_FAIL,
		DNS_EAI_FAMILY,
		DNS_EAI_MEMORY,
		DNS_EAI_NONAME,
		DNS_EAI_SERVICE,
		DNS_EAI_SOCKTYPE,
		DNS_EAI_SYSTEM
	};

	for (int i = 0; i < ERROR_SIZE; i++) {
		__wrap_nrf_getaddrinfo_ExpectAndReturn(HTTPS_HOSTNAME, NULL,
						       NULL, NULL, nrf_errors[i]);
		__wrap_nrf_getaddrinfo_IgnoreArg_hints();
		__wrap_nrf_getaddrinfo_IgnoreArg_res();

		ret = getaddrinfo(HTTPS_HOSTNAME, NULL, &hints, &res);

		TEST_ASSERT_EQUAL(ret, dns_errors[i]);
	}
}

void test_nrf91_socket_offload_getaddrinfo_eai_family(void)
{
	int ret;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};
	struct nrf_sockaddr_in info_in = { 0 };

	test_state_nrf_getaddrinfo.res.ai_family = WRONG_VALUE;
	test_state_nrf_getaddrinfo.res.ai_socktype = NRF_SOCK_STREAM;
	test_state_nrf_getaddrinfo.res.ai_protocol = NRF_IPPROTO_TCP;
	test_state_nrf_getaddrinfo.res.ai_addr = (struct nrf_sockaddr *)&info_in;
	test_state_nrf_getaddrinfo.ret = 0;

	__wrap_nrf_getaddrinfo_Stub(nrf_getaddrinfo_stub);
	__wrap_nrf_freeaddrinfo_Stub(nrf_freeaddrinfo_stub);

	ret = getaddrinfo(HTTPS_HOSTNAME, NULL, &hints, &res);

	TEST_ASSERT_EQUAL(ret, DNS_EAI_FAMILY);
}

void test_nrf91_socket_offload_getaddrinfo_ipv4_success(void)
{
	int ret;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_canonname = "name",
	};
	struct nrf_sockaddr_in info_in = { 0 };

	test_state_nrf_getaddrinfo.res.ai_family = NRF_AF_INET;
	test_state_nrf_getaddrinfo.res.ai_socktype = NRF_SOCK_STREAM;
	test_state_nrf_getaddrinfo.res.ai_protocol = NRF_IPPROTO_TCP;
	test_state_nrf_getaddrinfo.res.ai_addr = (struct nrf_sockaddr *)&info_in;
	test_state_nrf_getaddrinfo.ret = 0;

	__wrap_nrf_getaddrinfo_Stub(nrf_getaddrinfo_stub);
	__wrap_nrf_freeaddrinfo_Stub(nrf_freeaddrinfo_stub);

	ret = getaddrinfo(HTTPS_HOSTNAME, NULL, &hints, &res);
	TEST_ASSERT_EQUAL(ret, 0);

	TEST_ASSERT_EQUAL(res->ai_family, AF_INET);
	TEST_ASSERT_EQUAL(res->ai_socktype, SOCK_STREAM);
	TEST_ASSERT_EQUAL(res->ai_protocol, IPPROTO_TCP);
}

void test_nrf91_socket_offload_getaddrinfo_ipv6_success(void)
{
	int ret;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET6,
		.ai_socktype = SOCK_STREAM,
	};
	struct nrf_sockaddr_in6 info_in = { 0 };

	test_state_nrf_getaddrinfo.res.ai_family = NRF_AF_INET6;
	test_state_nrf_getaddrinfo.res.ai_socktype = NRF_SOCK_STREAM;
	test_state_nrf_getaddrinfo.res.ai_protocol = NRF_IPPROTO_TCP;
	test_state_nrf_getaddrinfo.res.ai_addr = (struct nrf_sockaddr *)&info_in;
	test_state_nrf_getaddrinfo.ret = 0;

	__wrap_nrf_getaddrinfo_Stub(nrf_getaddrinfo_stub);
	__wrap_nrf_freeaddrinfo_Stub(nrf_freeaddrinfo_stub);

	ret = getaddrinfo(HTTPS_HOSTNAME, NULL, &hints, &res);

	TEST_ASSERT_EQUAL(ret, 0);

	TEST_ASSERT_EQUAL(res->ai_family, AF_INET6);
	TEST_ASSERT_EQUAL(res->ai_socktype, SOCK_STREAM);
	TEST_ASSERT_EQUAL(res->ai_protocol, IPPROTO_TCP);
}

void test_nrf91_socket_offload_close_ebadf(void)
{
	int ret;

	ret = close(-1);

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EBADF);
}

void test_nrf91_socket_offload_create_close_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_socket_error(void)
{
	int fd;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, -1);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, -1);
}

void test_nrf91_socket_create_native_socket_eafnosupport(void)
{
	int fd;
	int family = AF_INET;
	int type = SOCK_STREAM | SOCK_NATIVE;
	int proto = IPPROTO_TCP;

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(errno, EAFNOSUPPORT);
	TEST_ASSERT_EQUAL(fd, -1);
}

void test_nrf91_socket_create_native_socket_tls_eafnosupport(void)
{
	int fd;
	int family = AF_INET6;
	int type = SOCK_STREAM | SOCK_NATIVE_TLS;
	int proto = IPPROTO_TLS_1_2;

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(errno, EAFNOSUPPORT);
	TEST_ASSERT_EQUAL(fd, -1);
}

void test_nrf91_socket_offload_create_close_proto_zero_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  0, nrf_fd);

	fd = socket(family, type, 0);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_connect_ebadf(void)
{
	int ret;
	struct sockaddr address = { 0 };

	ret = connect(-1, &address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EBADF);
}

void test_nrf91_socket_offload_connect_ipv4_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	struct sockaddr address = { 0 };

	/* IPv4 */
	address.sa_family = AF_INET;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_connect_ExpectAndReturn(nrf_fd, NULL,
					   sizeof(struct nrf_sockaddr_in), 0);
	__wrap_nrf_connect_IgnoreArg_address();

	ret = connect(fd, &address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_connect_ipv6_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET6;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	struct sockaddr address = { 0 };

	/* IPv6 */
	address.sa_family = AF_INET6;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET6, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_connect_ExpectAndReturn(nrf_fd, NULL,
					   sizeof(struct nrf_sockaddr_in6), 0);
	__wrap_nrf_connect_IgnoreArg_address();

	ret = connect(fd, &address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_connect_non_ip_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_PACKET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	struct sockaddr address = { 0 };

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_PACKET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_connect_ExpectAndReturn(nrf_fd, NULL, sizeof(address), 0);
	__wrap_nrf_connect_IgnoreArg_address();

	ret = connect(fd, &address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

/* The check for wrong socket is done in Zephyr before calling the offloading layer.
 * Still keeping this test since it proves that this case is being handled no matter the
 * Zephyr implementation.
 */
void test_nrf91_socket_offload_bind_ebadf(void)
{
	int ret;
	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	ret = bind(-1, (struct sockaddr *)&address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EBADF);
}

void test_nrf91_socket_offload_bind_eafnosupport(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	struct sockaddr_in address;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM, 0, nrf_fd);
	fd = socket(AF_INET, SOCK_STREAM, 0);

	address.sin_family = WRONG_VALUE;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	ret = bind(fd, (struct sockaddr *)&address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EAFNOSUPPORT);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_bind_ipv4_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	struct sockaddr_in address;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM, 0, nrf_fd);
	fd = socket(AF_INET, SOCK_STREAM, 0);

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	__wrap_nrf_bind_ExpectAndReturn(nrf_fd, NULL, sizeof(struct nrf_sockaddr_in), 0);
	__wrap_nrf_bind_IgnoreArg_address();

	ret = bind(fd, (struct sockaddr *)&address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_bind_ipv6_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	struct sockaddr_in address;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET6, NRF_SOCK_STREAM, 0, nrf_fd);
	fd = socket(AF_INET6, SOCK_STREAM, 0);

	address.sin_family = AF_INET6;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	__wrap_nrf_bind_ExpectAndReturn(nrf_fd, NULL, sizeof(struct nrf_sockaddr_in6), 0);
	__wrap_nrf_bind_IgnoreArg_address();

	ret = bind(fd, (struct sockaddr *)&address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_listen_ebadf(void)
{
	int ret;

	ret = listen(-1, 1);

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EBADF);
}

void test_nrf91_socket_offload_listen_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	struct sockaddr_in address;
	int backlog = 1;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET6, NRF_SOCK_STREAM, 0, nrf_fd);
	fd = socket(AF_INET6, SOCK_STREAM, 0);

	address.sin_family = AF_INET6;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	__wrap_nrf_bind_ExpectAndReturn(nrf_fd, NULL, sizeof(struct nrf_sockaddr_in6), 0);
	__wrap_nrf_bind_IgnoreArg_address();

	ret = bind(fd, (struct sockaddr *)&address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_listen_ExpectAndReturn(nrf_fd, backlog, 0);

	ret = listen(fd, backlog);

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_accept_ebadf(void)
{
	int ret;
	struct sockaddr_in address;
	int addrlen = sizeof(address);

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	ret = accept(-1, (struct sockaddr *)&address,
		     (socklen_t *)&addrlen);

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EBADF);
}

void test_nrf91_socket_offload_accept_addr_null_addrlen_null_error(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	struct sockaddr_in address;
	int backlog = 1;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM, 0, nrf_fd);
	fd = socket(AF_INET, SOCK_STREAM, 0);

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	__wrap_nrf_bind_ExpectAndReturn(nrf_fd, NULL, sizeof(struct nrf_sockaddr_in), 0);
	__wrap_nrf_bind_IgnoreArg_address();

	ret = bind(fd, (struct sockaddr *)&address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_listen_ExpectAndReturn(nrf_fd, backlog, 0);

	ret = listen(fd, backlog);

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_accept_ExpectAndReturn(nrf_fd, NULL, NULL, -2);

	ret = accept(fd, NULL, NULL);

	TEST_ASSERT_EQUAL(ret, -1);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_accept_addr_not_null_addrlen_not_null_enotsup(void)
{
	int ret;
	int fd;
	int accept_fd = 2;
	int nrf_fd = 1;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	int addrlen_unchanged = addrlen;
	int backlog = 1;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM, 0, nrf_fd);
	fd = socket(AF_INET, SOCK_STREAM, 0);

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	__wrap_nrf_bind_ExpectAndReturn(nrf_fd, NULL, sizeof(struct nrf_sockaddr_in), 0);
	__wrap_nrf_bind_IgnoreArg_address();

	ret = bind(fd, (struct sockaddr *)&address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_listen_ExpectAndReturn(nrf_fd, backlog, 0);

	ret = listen(fd, backlog);

	TEST_ASSERT_EQUAL(ret, 0);

	/* Modifying value to a wrong one to trigger the error case */
	test_state_nrf_accept.nrf_addr.sa_family = WRONG_VALUE;
	test_state_nrf_accept.ret = accept_fd;

	__wrap_nrf_accept_Stub(nrf_accept_stub);
	__wrap_nrf_close_ExpectAndReturn(accept_fd, 0);

	ret = accept(fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, ENOTSUP);
	TEST_ASSERT_EQUAL(addrlen, addrlen_unchanged);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_accept_ipv4_success(void)
{
	int ret;
	int fd;
	int nrf_accept_fd = 2;
	int nrf_fd = 1;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	int addrlen_unchanged = addrlen;
	int backlog = 1;
	/* `z_reserve_fd` reserves fd = 1 first */
	int z_fd = 1;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM, 0, nrf_fd);
	fd = socket(AF_INET, SOCK_STREAM, 0);

	address.sin_family = AF_INET;
	address.sin_port = htons(PORT);

	__wrap_nrf_bind_ExpectAndReturn(nrf_fd, NULL, sizeof(struct nrf_sockaddr_in), 0);
	__wrap_nrf_bind_IgnoreArg_address();

	ret = bind(fd, (struct sockaddr *)&address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_listen_ExpectAndReturn(nrf_fd, backlog, 0);

	ret = listen(fd, backlog);

	TEST_ASSERT_EQUAL(ret, 0);

	test_state_nrf_accept.nrf_addr.sa_family = NRF_AF_INET;
	test_state_nrf_accept.ret = nrf_accept_fd;

	__wrap_nrf_accept_Stub(nrf_accept_stub);

	ret = accept(fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

	TEST_ASSERT_EQUAL(ret, z_fd);
	TEST_ASSERT_EQUAL(addrlen, addrlen_unchanged);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);

	z_free_fd(z_fd);
}

void test_nrf91_socket_offload_accept_ipv6_success(void)
{
	int ret;
	int fd;
	int nrf_accept_fd = 2;
	int nrf_fd = 1;
	struct sockaddr_in6 address;
	int addrlen = sizeof(address);
	int addrlen_unchanged = addrlen;
	int backlog = 1;
	/* `z_reserve_fd` reserves fd = 1 first */
	int z_fd = 1;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET6, NRF_SOCK_STREAM, 0, nrf_fd);
	fd = socket(AF_INET6, SOCK_STREAM, 0);

	address.sin6_family = AF_INET6;
	address.sin6_port = htons(PORT);

	__wrap_nrf_bind_ExpectAndReturn(nrf_fd, NULL, sizeof(struct nrf_sockaddr_in6), 0);
	__wrap_nrf_bind_IgnoreArg_address();

	ret = bind(fd, (struct sockaddr *)&address, sizeof(address));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_listen_ExpectAndReturn(nrf_fd, backlog, 0);

	ret = listen(fd, backlog);

	TEST_ASSERT_EQUAL(ret, 0);

	test_state_nrf_accept.nrf_addr.sa_family = NRF_AF_INET6;
	test_state_nrf_accept.ret = nrf_accept_fd;

	__wrap_nrf_accept_Stub(nrf_accept_stub);

	ret = accept(fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

	TEST_ASSERT_EQUAL(ret, z_fd);
	TEST_ASSERT_EQUAL(addrlen, addrlen_unchanged);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);

	z_free_fd(z_fd);
}

void test_nrf91_socket_offload_setsockopt_ebadf(void)
{
	int ret;

	ret = setsockopt(-1, 0, 0, NULL, 0);

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EBADF);
}

void test_nrf91_socket_offload_setsockopt_rcvtimeo_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	struct timeval data = { 0 };

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_setsockopt_ExpectAndReturn(nrf_fd, NRF_SOL_SOCKET, NRF_SO_RCVTIMEO,
					      NULL, sizeof(struct nrf_timeval), 0);
	__wrap_nrf_setsockopt_IgnoreArg_option_value();

	ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &data, sizeof(data));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);
	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_setsockopt_sndtimeo_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	struct timeval data = { 0 };

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_setsockopt_ExpectAndReturn(nrf_fd, NRF_SOL_SOCKET, NRF_SO_SNDTIMEO,
					      NULL, sizeof(struct nrf_timeval), 0);
	__wrap_nrf_setsockopt_IgnoreArg_option_value();

	ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &data, sizeof(data));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_setsockopt_tls_session_cache_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	uint8_t data = 0;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_setsockopt_ExpectAndReturn(nrf_fd, NRF_SOL_SECURE, NRF_SO_SEC_SESSION_CACHE,
					      NULL, sizeof(nrf_sec_session_cache_t), 0);
	__wrap_nrf_setsockopt_IgnoreArg_option_value();

	ret = setsockopt(fd, SOL_TLS, TLS_SESSION_CACHE, &data, sizeof(data));

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_getsockopt_ebadf(void)
{
	int ret;

	ret = getsockopt(-1, 0, 0, NULL, NULL);

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EBADF);
}

void test_nrf91_socket_offload_getsockopt_rcvtimeo_error(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	struct timeval data = { 0 };
	int data_len = sizeof(data);

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_getsockopt_ExpectAndReturn(nrf_fd, NRF_SOL_SOCKET, NRF_SO_RCVTIMEO,
					      NULL, NULL, -1);
	__wrap_nrf_getsockopt_IgnoreArg_option_value();
	__wrap_nrf_getsockopt_IgnoreArg_option_len();

	ret = getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &data, &data_len);

	TEST_ASSERT_EQUAL(ret, -1);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_getsockopt_sndtimeo_error(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	struct timeval data = { 0 };
	int data_len = sizeof(data);

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_getsockopt_ExpectAndReturn(nrf_fd, NRF_SOL_SOCKET, NRF_SO_SNDTIMEO,
					      NULL, NULL, -1);
	__wrap_nrf_getsockopt_IgnoreArg_option_value();
	__wrap_nrf_getsockopt_IgnoreArg_option_len();

	ret = getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &data, &data_len);

	TEST_ASSERT_EQUAL(ret, -1);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_getsockopt_rcvtimeo_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	struct timeval data = { 0 };
	int data_len = sizeof(data);

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_getsockopt_ExpectAndReturn(nrf_fd, NRF_SOL_SOCKET, NRF_SO_RCVTIMEO,
					      NULL, NULL, 0);
	__wrap_nrf_getsockopt_IgnoreArg_option_value();
	__wrap_nrf_getsockopt_IgnoreArg_option_len();

	/* Change `data_len` before calling so that we can see if the
	 * function changes it on return
	 */
	data_len = data_len + 1;

	ret = getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &data, &data_len);

	TEST_ASSERT_EQUAL(ret, 0);
	TEST_ASSERT_EQUAL(data_len, sizeof(struct timeval));

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_getsockopt_sndtimeo_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	struct timeval data = { 0 };
	int data_len = sizeof(data);

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_getsockopt_ExpectAndReturn(nrf_fd, NRF_SOL_SOCKET, NRF_SO_SNDTIMEO,
					      NULL, NULL, 0);
	__wrap_nrf_getsockopt_IgnoreArg_option_value();
	__wrap_nrf_getsockopt_IgnoreArg_option_len();

	/* Change `data_len` before calling so that we can see if the
	 * function changes it on return
	 */
	data_len = data_len + 1;

	ret = getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &data, &data_len);

	TEST_ASSERT_EQUAL(ret, 0);
	TEST_ASSERT_EQUAL(data_len, sizeof(struct timeval));

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_getsockopt_so_error_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	int data = 1;
	int data_len = 42;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	__wrap_nrf_getsockopt_ExpectAndReturn(nrf_fd, NRF_SOL_SOCKET, NRF_SO_ERROR,
					      NULL, NULL, 0);
	__wrap_nrf_getsockopt_IgnoreArg_option_value();
	__wrap_nrf_getsockopt_IgnoreArg_option_len();
	__wrap_nrf_modem_os_errno_set_Expect(data);

	ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &data, &data_len);

	TEST_ASSERT_EQUAL(ret, 0);
	TEST_ASSERT_EQUAL(data, errno);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_recvfrom_ebadf(void)
{
	int ret;

	ret = recvfrom(-1, NULL, 0, 0, NULL, NULL);

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EBADF);
}

void test_nrf91_socket_offload_recvfrom_from_null_error(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	uint8_t data[8] = { 1 };
	size_t data_len = sizeof(data);
	int flags = MSG_WAITALL;
	socklen_t fromlen = 0;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	/* Expect that with `from` NULL the modem library call receives a forced
	 * `fromlen` NULL as well despite passing a pointer to the Zephyr call
	 */
	__wrap_nrf_recvfrom_ExpectAndReturn(nrf_fd, data, data_len, NRF_MSG_WAITALL,
					    NULL, NULL, -1);

	ret = recvfrom(fd, data, data_len, flags, NULL, &fromlen);

	TEST_ASSERT_EQUAL(ret, -1);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_recvfrom_from_null_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	uint8_t data[8] = { 1 };
	size_t data_len = sizeof(data);
	int flags = MSG_WAITALL;
	socklen_t fromlen = 0;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	/* Expect that with `from` NULL the modem library call receives a forced
	 * `fromlen` NULL as well despite passing a pointer to the Zephyr call
	 */
	__wrap_nrf_recvfrom_ExpectAndReturn(nrf_fd, data, data_len, NRF_MSG_WAITALL,
					    NULL, NULL, 8);

	ret = recvfrom(fd, data, data_len, flags, NULL, &fromlen);

	TEST_ASSERT_EQUAL(ret, 8);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_recvfrom_ipv4_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	uint8_t data[8] = { 1 };
	size_t data_len = sizeof(data);
	int flags = MSG_WAITALL;
	struct sockaddr from = { 0 };
	socklen_t fromlen = 0;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	test_state_nrf_recvfrom.cliaddr.sa_family = NRF_AF_INET;
	test_state_nrf_recvfrom.address_len = sizeof(struct nrf_sockaddr_in);
	test_state_nrf_recvfrom.ret = 8;

	__wrap_nrf_recvfrom_Stub(nrf_recvfrom_stub);

	ret = recvfrom(fd, data, data_len, flags, &from, &fromlen);

	TEST_ASSERT_EQUAL(ret, 8);
	TEST_ASSERT_EQUAL(from.sa_family, AF_INET);
	TEST_ASSERT_EQUAL(fromlen, sizeof(struct sockaddr_in));

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_recvfrom_ipv6_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	uint8_t data[8] = { 1 };
	size_t data_len = sizeof(data);
	int flags = MSG_WAITALL;
	struct sockaddr from = { 0 };
	socklen_t fromlen = 0;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	test_state_nrf_recvfrom.cliaddr.sa_family = NRF_AF_INET6;
	test_state_nrf_recvfrom.address_len = sizeof(struct nrf_sockaddr_in6);
	test_state_nrf_recvfrom.ret = 8;

	__wrap_nrf_recvfrom_Stub(nrf_recvfrom_stub);

	ret = recvfrom(fd, data, data_len, flags, &from, &fromlen);

	TEST_ASSERT_EQUAL(ret, 8);
	TEST_ASSERT_EQUAL(from.sa_family, AF_INET6);
	TEST_ASSERT_EQUAL(fromlen, sizeof(struct sockaddr_in6));

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_sendto_ebadf(void)
{
	int ret;

	ret = sendto(-1, NULL, 0, 0, NULL, 0);

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EBADF);
}

void test_nrf91_socket_offload_sendto_to_null_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	uint8_t data[8] = { 1 };
	size_t data_len = sizeof(data);
	int flags = MSG_DONTWAIT;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	/* Observe that the `tolen` parameter will be changed to zero when
	 * the `to` parameter is NULL
	 */
	__wrap_nrf_sendto_ExpectAndReturn(nrf_fd, data, data_len,
					  NRF_MSG_DONTWAIT, NULL, 0, 8);

	ret = sendto(fd, data, data_len, flags, NULL, 42);

	TEST_ASSERT_EQUAL(ret, 8);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_sendto_ipv4_error(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	uint8_t data[8] = { 1 };
	size_t data_len = sizeof(data);
	int flags = MSG_DONTWAIT;
	struct sockaddr to = { .sa_family = family };

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	/* Observe that the `tolen` parameter will be changed to size of struct
	 * `nrf_sockaddr_in` when the `to` parameter has `sa_family` with value
	 * `AF_INET`
	 */
	__wrap_nrf_sendto_ExpectAndReturn(nrf_fd, data, data_len,
					  NRF_MSG_DONTWAIT, NULL,
					  sizeof(struct nrf_sockaddr_in), -1);
	__wrap_nrf_sendto_IgnoreArg_dest_addr();

	ret = sendto(fd, data, data_len, flags, &to, 42);

	TEST_ASSERT_EQUAL(ret, -1);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_sendto_ipv4_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	uint8_t data[8] = { 1 };
	size_t data_len = sizeof(data);
	int flags = MSG_DONTWAIT;
	struct sockaddr to = { .sa_family = family };

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	/* Observe that the `tolen` parameter will be changed to size of struct
	 * `nrf_sockaddr_in` when the `to` parameter has `sa_family` with value
	 * `AF_INET`
	 */
	__wrap_nrf_sendto_ExpectAndReturn(nrf_fd, data, data_len,
					  NRF_MSG_DONTWAIT, NULL,
					  sizeof(struct nrf_sockaddr_in), 8);
	__wrap_nrf_sendto_IgnoreArg_dest_addr();

	ret = sendto(fd, data, data_len, flags, &to, 42);

	TEST_ASSERT_EQUAL(ret, 8);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_sendto_ipv6_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET6;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	uint8_t data[8] = { 1 };
	size_t data_len = sizeof(data);
	int flags = MSG_DONTWAIT;
	struct sockaddr to = { .sa_family = family };

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET6, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	/* Observe that the `tolen` parameter will be changed to size of struct
	 * `nrf_sockaddr_in` when the `to` parameter has `sa_family` with value
	 * `AF_INET`
	 */
	__wrap_nrf_sendto_ExpectAndReturn(nrf_fd, data, data_len,
					  NRF_MSG_DONTWAIT, NULL,
					  sizeof(struct nrf_sockaddr_in6), 8);
	__wrap_nrf_sendto_IgnoreArg_dest_addr();

	ret = sendto(fd, data, data_len, flags, &to, 42);

	TEST_ASSERT_EQUAL(ret, 8);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_sendto_not_ipv4_not_ipv6_eafnosupport(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	uint8_t data[8] = { 1 };
	size_t data_len = sizeof(data);
	int flags = MSG_DONTWAIT;
	struct sockaddr to = { 0 };

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	ret = sendto(fd, data, data_len, flags, &to, 42);

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EAFNOSUPPORT);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_sendmsg_ebadf(void)
{
	int ret;

	ret = sendmsg(-1, NULL, 0);

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EBADF);
}

void test_nrf91_socket_offload_sendmsg_msg_null_einval(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	int flags = MSG_DONTWAIT;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	ret = sendmsg(fd, NULL, flags);

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EINVAL);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_sendmsg_fits_buf(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	int flags = MSG_DONTWAIT;
	struct msghdr msg = { 0 };
	struct iovec chunks[2] = { 0 };
	int chunk_1 = 42;
	int chunk_2 = 43;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	chunks[0].iov_base = &chunk_1;
	chunks[0].iov_len = sizeof(int);
	chunks[1].iov_base = &chunk_2;
	chunks[1].iov_len = sizeof(int);
	msg.msg_iov = chunks;
	msg.msg_iovlen = 2;

	__wrap_nrf_sendto_ExpectAndReturn(nrf_fd, NULL, 2 * sizeof(int),
					  NRF_MSG_DONTWAIT,
					  NULL, 0, 2 * sizeof(int));
	__wrap_nrf_sendto_IgnoreArg_message();

	ret = sendmsg(fd, &msg, flags);

	TEST_ASSERT_EQUAL(ret, 2 * sizeof(int));

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_sendmsg_not_fits_buf(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	int flags = MSG_DONTWAIT;
	struct msghdr msg = { 0 };
	struct iovec chunks[3] = { 0 };

	/* 3 ints are enough to surpass the defined size of the
	 * intermediate buffer (which is set to 8 bytes in this project)
	 */
	int chunk_1 = 42;
	int chunk_2 = 43;
	int chunk_3 = 44;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	chunks[0].iov_base = &chunk_1;
	chunks[0].iov_len = sizeof(int);
	chunks[1].iov_base = &chunk_2;
	chunks[1].iov_len = sizeof(int);
	chunks[2].iov_base = &chunk_3;
	chunks[2].iov_len = sizeof(int);
	msg.msg_iov = chunks;
	msg.msg_iovlen = 3;

	/* First send doesn't send all data of the first chunk */
	__wrap_nrf_sendto_ExpectAndReturn(nrf_fd, &chunk_1, sizeof(int),
					  NRF_MSG_DONTWAIT,
					  NULL, 0, sizeof(int) - 1);
	/* Second send will send the remaining part of the first chunk */
	__wrap_nrf_sendto_ExpectAndReturn(nrf_fd, ((uint8_t *)&chunk_1) + sizeof(int) - 1, 1,
					  NRF_MSG_DONTWAIT,
					  NULL, 0, 1);
	__wrap_nrf_sendto_ExpectAndReturn(nrf_fd, &chunk_2, sizeof(int),
					  NRF_MSG_DONTWAIT,
					  NULL, 0, sizeof(int));
	__wrap_nrf_sendto_ExpectAndReturn(nrf_fd, &chunk_3, sizeof(int),
					  NRF_MSG_DONTWAIT,
					  NULL, 0, sizeof(int));
	__wrap_nrf_sendto_IgnoreArg_message();

	ret = sendmsg(fd, &msg, flags);

	TEST_ASSERT_EQUAL(ret, 3 * sizeof(int));

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_ioctl_poll_prepare(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	ret = fcntl(fd, ZFD_IOCTL_POLL_PREPARE);

	TEST_ASSERT_EQUAL(ret, -EXDEV);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_ioctl_poll_update(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	/* `fcntl` will call `ioctl` underneath */
	ret = fcntl(fd, ZFD_IOCTL_POLL_UPDATE);

	TEST_ASSERT_EQUAL(ret, -EOPNOTSUPP);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_fcntl_einval(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	int wrong_command = ~(F_SETFL | F_GETFL);

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	ret = fcntl(fd, wrong_command);

	TEST_ASSERT_EQUAL(ret, -1);
	TEST_ASSERT_EQUAL(errno, EINVAL);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_fcntl_f_setfl(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	__wrap_nrf_fcntl_ExpectAndReturn(nrf_fd, NRF_F_SETFL, NRF_O_NONBLOCK, 0);

	ret = fcntl(fd, F_SETFL, O_NONBLOCK);

	TEST_ASSERT_EQUAL(ret, 0);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_fcntl_f_getfl(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	__wrap_nrf_fcntl_ExpectAndReturn(nrf_fd, NRF_F_GETFL, 0, NRF_O_NONBLOCK);

	ret = fcntl(fd, F_GETFL);

	TEST_ASSERT_EQUAL(ret, O_NONBLOCK);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_poll_non_offloaded_socket(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	struct pollfd fds[3] = { 0 };

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	fds[0].fd = fd;
	fds[1].fd = 42;
	fds[2].fd = 43;

	ret = poll(fds, 3, 0);

	/* We had two errors, i.e. two sockets weren't correctly offloaded */
	TEST_ASSERT_EQUAL(ret, 2);
	TEST_ASSERT_EQUAL(fds[1].revents, POLLNVAL);
	TEST_ASSERT_EQUAL(fds[2].revents, POLLNVAL);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

void test_nrf91_socket_offload_poll_all_events_success(void)
{
	int ret;
	int fd;
	int nrf_fd = 2;
	int family = AF_INET;
	int type = SOCK_STREAM;
	int proto = IPPROTO_TCP;
	struct pollfd fds[1] = { 0 };
	int events = POLLIN | POLLOUT;
	int revents = POLLIN | POLLOUT |
		      POLLERR | POLLNVAL |
		      POLLHUP;

	__wrap_nrf_socket_ExpectAndReturn(NRF_AF_INET, NRF_SOCK_STREAM,
					  NRF_IPPROTO_TCP, nrf_fd);

	fd = socket(family, type, proto);

	TEST_ASSERT_EQUAL(fd, 0);

	/* Skip connect, etc. since for testing we just
	 * need a working socket
	 */

	fds[0].fd = fd;
	fds[0].events = events;

	test_state_nrf_poll.expected_in->events = NRF_POLLIN | NRF_POLLOUT;
	test_state_nrf_poll.expected_out->revents = NRF_POLLIN | NRF_POLLOUT |
						    NRF_POLLERR | NRF_POLLNVAL |
						    NRF_POLLHUP;
	test_state_nrf_poll.ret = 0;

	__wrap_nrf_poll_Stub(nrf_poll_stub);

	ret = poll(fds, 1, 0);

	TEST_ASSERT_EQUAL(ret, 0);
	TEST_ASSERT_EQUAL(fds[0].revents, revents);

	__wrap_nrf_close_ExpectAndReturn(nrf_fd, 0);

	ret = close(fd);

	TEST_ASSERT_EQUAL(ret, 0);
}

/* It is required to be added to each test. That is because unity is using
 * different main signature (returns int) and zephyr expects main which does
 * not return value.
 */
extern int unity_main(void);

void main(void)
{
	(void)unity_main();
}
