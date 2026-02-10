/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#define SUB_REGION_START_OFFSET 0x80U
#define SUB_REGION_END_OFFSET   0x100U

#define PUB_REGION_START_OFFSET 0x180U
#define PUB_REGION_END_OFFSET   0x200U

#define REG_ADDR_COMMA(node_id) (uint32_t *)DT_REG_ADDR(node_id),
#define LIST_ENABLED_INSTANCES(compat) DT_FOREACH_STATUS_OKAY(compat, REG_ADDR_COMMA)

static bool is_region_zeros(uint32_t *start, uint32_t *end)
{
	for (uint32_t *addr = start; addr < end; addr++) {
		if (*addr != 0U) {
			return false;
		}
	}
	return true;
}

static bool is_publish_subscribe_region_cleared(uint32_t *peripheral_address)
{
	uint32_t *sub_start_addr = peripheral_address + SUB_REGION_START_OFFSET / sizeof(uint32_t);
	uint32_t *sub_end_addr   = peripheral_address + SUB_REGION_END_OFFSET   / sizeof(uint32_t);

	uint32_t *pub_start_addr = peripheral_address + PUB_REGION_START_OFFSET / sizeof(uint32_t);
	uint32_t *pub_end_addr   = peripheral_address + PUB_REGION_END_OFFSET   / sizeof(uint32_t);

	return is_region_zeros(sub_start_addr, sub_end_addr) &&
	       is_region_zeros(pub_start_addr, pub_end_addr);
}

static void verify_pub_sub_regions(uint32_t **instances, size_t length)
{
	for (int i = 0; i < length; i++) {
		zassert_true(
			is_publish_subscribe_region_cleared(instances[i]),
			"Publish/subscribe register is not 0, peripheral base address: 0x%08X",
			(uint32_t)instances[i]
		);
	}
}

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_comp)
ZTEST(publish_subscribe_leftovers, test_comp_cleared)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_comp) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_dppic)
ZTEST(publish_subscribe_leftovers, test_dppic_cleared)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_dppic) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_egu)
ZTEST(publish_subscribe_leftovers, test_egu_cleared)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_egu) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_gpiote)
ZTEST(publish_subscribe_leftovers, test_gpiote_cleread)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_gpiote) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_ipc)
ZTEST(publish_subscribe_leftovers, test_ipc_cleared)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_ipc) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_kmu)
ZTEST(publish_subscribe_leftovers, test_kmu_cleared)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_kmu) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pwm)
ZTEST(publish_subscribe_leftovers, test_pwm_cleared)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_pwm) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_qdec)
ZTEST(publish_subscribe_leftovers, test_qdec_cleared)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_qdec) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_rtc)
ZTEST(publish_subscribe_leftovers, test_rtc_cleared)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_rtc) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_spim)
ZTEST(publish_subscribe_leftovers, test_spim_cleared)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_spim) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_timer)
ZTEST(publish_subscribe_leftovers, test_timer_cleared)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_timer) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_usbd)
ZTEST(publish_subscribe_leftovers, test_usbd_cleared)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_usbd) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_wdt)
ZTEST(publish_subscribe_leftovers, test_wdt_cleared)
{
	static uint32_t *addrs[] = { LIST_ENABLED_INSTANCES(nordic_nrf_wdt) };

	verify_pub_sub_regions(addrs, ARRAY_SIZE(addrs));
}
#endif

ZTEST_SUITE(publish_subscribe_leftovers, NULL, NULL, NULL, NULL, NULL);
