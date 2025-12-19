/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>

#include <net/dect/dect_utils.h>

/* MAC spec: Table 6.2.1-3a */
static const int8_t m_dbm_power_tbl[] = {
	-40,
	-30,
	-20,
	-16,
	-12,
	-8,
	-4,
	0,
	4,
	7,
	10,
	13,
	16,
	19,
	21,
	23
};

uint32_t dect_utils_lib_dst_long_rd_id_get_from_pkt_dst_addr(struct net_pkt *pkt)
{
	struct net_linkaddr *lladdr = net_pkt_lladdr_dst(pkt);
	uint32_t long_rd_id;

	if (!lladdr || lladdr->len != sizeof(uint32_t)) {
		return 0;
	}
	/* Destination long RD ID is lowest 4 bytes */
	memcpy(&long_rd_id, lladdr->addr, sizeof(uint32_t));

	return ntohl(long_rd_id);
}

uint32_t dect_utils_lib_dst_long_rd_id_get_from_dst_sock_ll_addr(struct sockaddr_ll *dst_addr)
{
	uint32_t long_rd_id;

	if (!dst_addr || dst_addr->sll_halen != sizeof(uint32_t)) {
		return 0;
	}
	/* Destination long RD ID is lowest 4 bytes */
	memcpy(&long_rd_id, dst_addr->sll_addr, sizeof(uint32_t));

	return ntohl(long_rd_id);
}

bool dect_utils_lib_net_linkaddr_set_from_long_rd_id(struct net_linkaddr *lladdr,
						     uint32_t long_rd_id)
{
	if (!lladdr) {
		return false;
	}
	uint32_t tmp_long_rd_id = htonl(long_rd_id);

	memcpy(lladdr->addr, &tmp_long_rd_id, sizeof(uint32_t));
	lladdr->len = sizeof(uint32_t);

	return true;
}

/* NR+ version of zephyr net_ipv6_addr_create_iid(): */
void dect_utils_lib_net_ipv6_addr_create_iid(struct in6_addr *addr, struct net_linkaddr *lladdr)
{
	__ASSERT_NO_MSG(lladdr->len == 8);

	UNALIGNED_PUT(htonl(0xfe800000), &addr->s6_addr32[0]);
	UNALIGNED_PUT(0, &addr->s6_addr32[1]);

	memcpy(&addr->s6_addr[8], lladdr->addr, lladdr->len);
	/* Note: no toggling of universal/local bit */
}

uint32_t dect_utils_lib_long_rd_id_from_ipv6_addr(struct in6_addr *dst_addr)
{
	uint32_t long_rd_id;

	if (!dst_addr) {
		return 0;
	}
	/* Long RD ID is the lowest 4 bytes */
	memcpy(&long_rd_id, &dst_addr->s6_addr[12], sizeof(uint32_t));

	return ntohl(long_rd_id);
}

uint32_t dect_utils_lib_sink_rd_id_from_ipv6_addr(struct in6_addr *dst_addr)
{
	uint32_t sink_rd_id;

	if (!dst_addr) {
		return 0;
	}
	/* Sink RD ID is the 1st 4 bytes of postfix (64bit) */
	memcpy(&sink_rd_id, &dst_addr->s6_addr[8], sizeof(uint32_t));

	return ntohl(sink_rd_id);
}

bool dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(struct in6_addr prefix_64,
								  uint32_t sink_rd_id,
								  uint32_t own_rd_id,
								  struct in6_addr *addr)
{
	if (!addr) {
		return false;
	}
	/* Create IPv6 address from sink and own long RD ID */
	memcpy(addr, &prefix_64, sizeof(struct in6_addr) / 2);
	addr->s6_addr32[2] = htonl(sink_rd_id);
	addr->s6_addr32[3] = htonl(own_rd_id);

	return true;
}

uint8_t dect_utils_lib_dbm_to_phy_tx_power(int8_t pwr_dBm)
{
	const uint16_t table_size = ARRAY_SIZE(m_dbm_power_tbl);

	for (uint16_t i = 0; i < table_size; i++) {
		if (m_dbm_power_tbl[i] >= pwr_dBm) {
			return i;
		}
	}
	return table_size - 1;
}

int8_t dect_utils_lib_phy_tx_power_to_dbm(uint8_t phy_power)
{
	const uint16_t table_size = ARRAY_SIZE(m_dbm_power_tbl);

	if (phy_power >= table_size) {
		phy_power = table_size - 1;
	}
	return m_dbm_power_tbl[phy_power];
}

/* MAC spec ch. 4.2.3.1:
 * The network ID shall be set to a value where neither
 * the 8 LSB bits are 0x00 nor the 24 MSB bits are 0x000000.
 */
bool dect_utils_lib_32bit_network_id_validate(uint32_t network_id)
{
	uint32_t msb_bits = network_id & 0xFFFFFF00; /* 24-bit MSB */
	uint8_t lsb_bits = network_id & 0xFF;	     /* 8-bit LSB */

	return ((0 != lsb_bits) && (0 != msb_bits));
}
