/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "rpc_test_rsp_builder.h"

#include <stdbool.h>
#include <string.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

static void cbor_append_uint(uint8_t *buf, size_t *pos, uint32_t val)
{
	if (val <= 23) {
		buf[(*pos)++] = (uint8_t)val;
	} else if (val <= 0xff) {
		buf[(*pos)++] = 0x18;
		buf[(*pos)++] = (uint8_t)val;
	} else if (val <= 0xffff) {
		buf[(*pos)++] = 0x19;
		buf[(*pos)++] = (val >> 8) & 0xff;
		buf[(*pos)++] = val & 0xff;
	} else {
		buf[(*pos)++] = 0x1a;
		buf[(*pos)++] = (val >> 24) & 0xff;
		buf[(*pos)++] = (val >> 16) & 0xff;
		buf[(*pos)++] = (val >> 8) & 0xff;
		buf[(*pos)++] = val & 0xff;
	}
}

static void cbor_append_bool(uint8_t *buf, size_t *pos, bool val)
{
	buf[(*pos)++] = val ? 0xf5 : 0xf4;
}

static void cbor_append_bstr16(uint8_t *buf, size_t *pos, const uint8_t *data)
{
	buf[(*pos)++] = 0x50;
	memcpy(&buf[*pos], data, 16);
	*pos += 16;
}

mock_dect_rpc_pkt_t rpc_test_expect_get_addrs_rsp(struct net_if *iface)
{
	static uint8_t storage[256];
	static mock_dect_rpc_pkt_t pkt;
	struct net_if_ipv6 *ipv6;
	size_t pos = 0;
	int n_addrs = 0;
	int n_prefixes = 0;
	int i;

	storage[pos++] = 0x01;
	storage[pos++] = 0xff;
	storage[pos++] = 0x00;
	storage[pos++] = 0x00;
	storage[pos++] = 0x00;

	if (iface && net_if_config_ipv6_get(iface, &ipv6) >= 0) {
		for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
			if (ipv6->unicast[i].is_used) {
				n_addrs++;
			}
		}
		for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
			if (ipv6->prefix[i].is_used) {
				n_prefixes++;
			}
		}
	}

	cbor_append_uint(storage, &pos, (uint32_t)n_addrs);
	if (iface && net_if_config_ipv6_get(iface, &ipv6) >= 0) {
		for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
			if (ipv6->unicast[i].is_used) {
				cbor_append_bstr16(storage, &pos,
						   (const uint8_t *)&ipv6->unicast[i]
							    .address.in6_addr);
			}
		}
	}

	cbor_append_uint(storage, &pos, (uint32_t)n_prefixes);
	if (iface && net_if_config_ipv6_get(iface, &ipv6) >= 0) {
		for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
			if (ipv6->prefix[i].is_used) {
				cbor_append_bstr16(storage, &pos,
						   (const uint8_t *)&ipv6->prefix[i].prefix);
				cbor_append_uint(storage, &pos, (uint32_t)ipv6->prefix[i].len);
			}
		}
	}

	cbor_append_uint(storage, &pos, iface ? net_if_get_mtu(iface) : (uint32_t)NET_IPV6_MTU);
	cbor_append_bool(storage, &pos, iface ? net_if_is_carrier_ok(iface) : false);
	cbor_append_bool(storage, &pos, iface ? net_if_is_dormant(iface) : true);
	storage[pos++] = 0xf6;

	pkt.data = storage;
	pkt.len = pos;
	return pkt;
}
