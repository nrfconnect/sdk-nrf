/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/ot_dns_offload.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket_offload.h>

#include <openthread/dns_client.h>

#include <openthread.h>

struct ot_dns_gai_ctx {
	/* Input */
	uint16_t port;
	int socktype;
	/* Output */
	struct k_sem sem;
	int gai_err;
	struct zsock_addrinfo *head;
	struct zsock_addrinfo *tail;
};

enum ot_dns_phase {
	OT_DNS_PHASE_IP4 = 0,
	OT_DNS_PHASE_IP6 = 1,
	OT_DNS_PHASE_COUNT,
};

static void ot_dns_free_result_list(struct zsock_addrinfo *ai);

static int append_addrinfo(struct ot_dns_gai_ctx *ctx, const otIp6Address *addr)
{
	struct zsock_addrinfo *ai;
	struct net_sockaddr_in6 *sa6;

	ai = k_calloc(1, sizeof(*ai));
	if (ai == NULL) {
		return -ENOMEM;
	}

	sa6 = net_sin6(net_sad(&ai->_ai_addr));
	sa6->sin6_family = NET_AF_INET6;
	sa6->sin6_port = htons(ctx->port);
	memcpy(&sa6->sin6_addr, addr->mFields.m8, sizeof(sa6->sin6_addr.s6_addr));

	ai->ai_family = NET_AF_INET6;
	ai->ai_socktype = ctx->socktype;
	ai->ai_protocol = (ctx->socktype == NET_SOCK_DGRAM) ? NET_IPPROTO_UDP : NET_IPPROTO_TCP;
	ai->ai_addr = net_sad(&ai->_ai_addr);
	ai->ai_addrlen = sizeof(*sa6);

	if (ctx->head == NULL) {
		ctx->head = ai;
	} else {
		ctx->tail->ai_next = ai;
	}
	ctx->tail = ai;

	return 0;
}

static int map_ot_error_to_gai(otError error)
{
	switch (error) {
	case OT_ERROR_NONE:
		return 0;
	case OT_ERROR_NOT_FOUND:
		return DNS_EAI_NODATA;
	case OT_ERROR_NO_BUFS:
		return DNS_EAI_MEMORY;
	case OT_ERROR_INVALID_STATE:
	case OT_ERROR_RESPONSE_TIMEOUT:
		return DNS_EAI_AGAIN;
	default:
		return DNS_EAI_FAIL;
	}
}

static void ot_dns_address_cb(otError error, const otDnsAddressResponse *response, void *aContext)
{
	struct ot_dns_gai_ctx *ctx = aContext;
	otIp6Address addr;

	if (error != OT_ERROR_NONE) {
		ctx->gai_err = map_ot_error_to_gai(error);
		goto out;
	}

	for (uint16_t i = 0;
	     otDnsAddressResponseGetAddress(response, i, &addr, NULL) == OT_ERROR_NONE; i++) {
		if (append_addrinfo(ctx, &addr) < 0) {
			ctx->gai_err = DNS_EAI_MEMORY;
			ot_dns_free_result_list(ctx->head);
			ctx->head = ctx->tail = NULL;
			goto out;
		}
	}

	if (ctx->head == NULL) {
		ctx->gai_err = DNS_EAI_NODATA;
	}

out:
	k_sem_give(&ctx->sem);
}

static void ot_dns_free_result_list(struct zsock_addrinfo *ai)
{
	while (ai != NULL) {
		struct zsock_addrinfo *next = ai->ai_next;

		k_free(ai);
		ai = next;
	}
}

static int ot_dns_offload_get_addrinfo_phase(enum ot_dns_phase phase, const char *hostname,
					     uint16_t port, int socktype,
					     struct zsock_addrinfo **res)
{
	struct ot_dns_gai_ctx ctx;
	otInstance *instance;
	otError error;

	*res = NULL;

	instance = openthread_get_default_instance();
	if (instance == NULL) {
		return DNS_EAI_FAIL;
	}

	memset(&ctx, 0, sizeof(ctx));
	k_sem_init(&ctx.sem, 0, 1);
	ctx.port = port;
	ctx.socktype = socktype;

	openthread_mutex_lock();

	if (phase == OT_DNS_PHASE_IP4) {
		error = otDnsClientResolveIp4Address(instance, hostname, ot_dns_address_cb, &ctx,
						     NULL);
	} else {
		error = otDnsClientResolveAddress(instance, hostname, ot_dns_address_cb, &ctx,
						  NULL);
	}

	openthread_mutex_unlock();

	if (error != OT_ERROR_NONE) {
		return map_ot_error_to_gai(error);
	}

	k_sem_take(&ctx.sem, K_FOREVER);

	if (ctx.gai_err == 0) {
		*res = ctx.head;
	}

	return ctx.gai_err;
}

static int ot_dns_offload_getaddrinfo(const char *node, const char *service,
				      const struct zsock_addrinfo *hints,
				      struct zsock_addrinfo **res)
{
	long port = 0;
	int socktype = 0;
	int gai_err;

	*res = NULL;

	if (node == NULL) {
		if (service == NULL) {
			return DNS_EAI_NONAME;
		}
		return DNS_EAI_FAIL;
	}

	if (hints != NULL) {
		if (hints->ai_family != NET_AF_UNSPEC && hints->ai_family != NET_AF_INET6) {
			return DNS_EAI_ADDRFAMILY;
		}
		if ((hints->ai_flags & ZSOCK_AI_NUMERICHOST) != 0) {
			return DNS_EAI_NONAME;
		}

		socktype = hints->ai_socktype;
	}

	if (service != NULL) {
		port = strtol(service, NULL, 10);
		if (port < 1 || port > 65535) {
			return DNS_EAI_SERVICE;
		}
	}

	gai_err = ot_dns_offload_get_addrinfo_phase(OT_DNS_PHASE_IP4, node, (uint16_t)port,
						    socktype, res);

	if (gai_err == 0) {
		return gai_err;
	}

	return ot_dns_offload_get_addrinfo_phase(OT_DNS_PHASE_IP6, node, (uint16_t)port, socktype,
						 res);
}

static void ot_dns_offload_freeaddrinfo(struct zsock_addrinfo *res)
{
	ot_dns_free_result_list(res);
}

static const struct socket_dns_offload ot_dns_socket_ops = {
	.getaddrinfo = ot_dns_offload_getaddrinfo,
	.freeaddrinfo = ot_dns_offload_freeaddrinfo,
};

int ot_dns_offload_register(void)
{
	if (socket_offload_dns_is_enabled()) {
		return -EALREADY;
	}

	socket_offload_dns_register(&ot_dns_socket_ops);
	return 0;
}

#if defined(CONFIG_OPENTHREAD_ZEPHYR_DNS_OFFLOAD_SYS_INIT)
SYS_INIT(ot_dns_offload_register, APPLICATION,
	 CONFIG_OPENTHREAD_ZEPHYR_DNS_OFFLOAD_SYS_INIT_PRIORITY);
#endif
