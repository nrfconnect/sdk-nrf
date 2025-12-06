/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stddef.h>
#include <stdint.h>


#include <bt_rpc_gatt_common.h>

/* Test helper: Get service count by checking until NULL */
static size_t get_service_count(void)
{
	size_t count = 0;

	while (bt_rpc_gatt_get_service_by_index(count) != NULL) {
		count++;
	}

	return count;
}

/* Test helper: Check if buffer is empty by verifying no services exist */
static bool is_buffer_empty(void)
{
	return get_service_count() == 0;
}

/* Test UUIDs - minimal structure for testing */
static struct bt_uuid test_uuid_1 = {.type = 0x10};
static struct bt_uuid test_uuid_2 = {.type = 0x11};
static struct bt_uuid test_uuid_3 = {.type = 0x12};

/* Test attributes - minimal structure for testing */
static struct bt_gatt_attr test_attrs_1[] = {
	{
		.uuid = &test_uuid_1,
		.handle = 1,
		.perm = 0,
		.read = NULL,
		.write = NULL,
		.user_data = NULL,
	},
	{
		.uuid = &test_uuid_1,
		.handle = 2,
		.perm = 0,
		.read = NULL,
		.write = NULL,
		.user_data = (void *)0x1234,
	},
};

static struct bt_gatt_attr test_attrs_2[] = {
	{
		.uuid = &test_uuid_2,
		.handle = 3,
		.perm = 0,
		.read = NULL,
		.write = NULL,
		.user_data = NULL,
	},
	{
		.uuid = &test_uuid_2,
		.handle = 4,
		.perm = 0,
		.read = NULL,
		.write = NULL,
		.user_data = (void *)0x5678,
	},
};

static struct bt_gatt_attr test_attrs_3[] = {
	{
		.uuid = &test_uuid_3,
		.handle = 5,
		.perm = 0,
		.read = NULL,
		.write = NULL,
		.user_data = NULL,
	},
	{
		.uuid = &test_uuid_3,
		.handle = 6,
		.perm = 0,
		.read = NULL,
		.write = NULL,
		.user_data = (void *)0x9ABC,
	},
};

static struct bt_gatt_service test_svc_1 = {
	.attrs = test_attrs_1,
	.attr_count = ARRAY_SIZE(test_attrs_1),
};

static struct bt_gatt_service test_svc_2 = {
	.attrs = test_attrs_2,
	.attr_count = ARRAY_SIZE(test_attrs_2),
};

static struct bt_gatt_service test_svc_3 = {
	.attrs = test_attrs_3,
	.attr_count = ARRAY_SIZE(test_attrs_3),
};

/**
 * @brief Test setup - verify initial state is empty
 */
static void test_setup(void *fixture)
{
	ARG_UNUSED(fixture);
	/* Verify initial state is empty */
	zassert_true(is_buffer_empty(), "Buffer should be empty initially");
}

/**
 * @brief Test teardown - cleanup all services
 */
static void test_teardown(void *fixture)
{
	ARG_UNUSED(fixture);
	size_t count = get_service_count();

	/* Remove all services */
	for (size_t i = 0; i < count; i++) {
		const struct bt_gatt_service *svc = bt_rpc_gatt_get_service_by_index(0);

		if (svc) {
			bt_rpc_gatt_remove_service(svc);
		}
	}

	/* Verify cleanup */
	zassert_true(is_buffer_empty(), "Buffer should be empty after teardown");
}

/**
 * @brief Test: Register a single service and verify it's added
 */
ZTEST(bt_rpc_gatt_service, test_register_single_service)
{
	uint32_t svc_index;
	int err;

	/* Register service */
	err = bt_rpc_gatt_add_service(&test_svc_1, &svc_index);
	zassert_equal(err, 0, "Service registration should succeed");
	zassert_equal(svc_index, 0, "First service should have index 0");

	/* Verify service count */
	zassert_equal(get_service_count(), 1, "Service count should be 1");

	/* Verify service can be retrieved */
	const struct bt_gatt_service *retrieved = bt_rpc_gatt_get_service_by_index(0);

	zassert_not_null(retrieved, "Service should be retrievable");
	zassert_equal(retrieved, &test_svc_1, "Retrieved service should match");
}

/**
 * @brief Test: Register multiple services and verify all are added
 */
ZTEST(bt_rpc_gatt_service, test_register_multiple_services)
{
	uint32_t svc_index_1, svc_index_2, svc_index_3;
	int err;

	/* Register first service */
	err = bt_rpc_gatt_add_service(&test_svc_1, &svc_index_1);
	zassert_equal(err, 0, "First service registration should succeed");
	zassert_equal(svc_index_1, 0, "First service should have index 0");

	/* Register second service */
	err = bt_rpc_gatt_add_service(&test_svc_2, &svc_index_2);
	zassert_equal(err, 0, "Second service registration should succeed");
	zassert_equal(svc_index_2, 1, "Second service should have index 1");

	/* Register third service */
	err = bt_rpc_gatt_add_service(&test_svc_3, &svc_index_3);
	zassert_equal(err, 0, "Third service registration should succeed");
	zassert_equal(svc_index_3, 2, "Third service should have index 2");

	/* Verify service count */
	zassert_equal(get_service_count(), 3, "Service count should be 3");

	/* Verify all services can be retrieved */
	zassert_not_null(bt_rpc_gatt_get_service_by_index(0), "Service 0 should exist");
	zassert_not_null(bt_rpc_gatt_get_service_by_index(1), "Service 1 should exist");
	zassert_not_null(bt_rpc_gatt_get_service_by_index(2), "Service 2 should exist");
}

/**
 * @brief Test: Deregister all services and verify buffer is reset
 *
 */
ZTEST(bt_rpc_gatt_service, test_deregister_all_services_resets_buffer)
{
	uint32_t svc_index_1, svc_index_2, svc_index_3;
	int err;
	size_t initial_count, after_remove_count, final_count;

	/* Get initial state */
	initial_count = get_service_count();
	zassert_true(initial_count == 0, "Initial state should be empty");

	/* Register 3 services */
	err = bt_rpc_gatt_add_service(&test_svc_1, &svc_index_1);
	zassert_equal(err, 0, "Service 1 registration should succeed");

	err = bt_rpc_gatt_add_service(&test_svc_2, &svc_index_2);
	zassert_equal(err, 0, "Service 2 registration should succeed");

	err = bt_rpc_gatt_add_service(&test_svc_3, &svc_index_3);
	zassert_equal(err, 0, "Service 3 registration should succeed");

	/* Verify all services are registered */
	zassert_equal(get_service_count(), 3, "All 3 services should be registered");

	/* Remove first service */
	err = bt_rpc_gatt_remove_service(&test_svc_1);
	zassert_equal(err, 0, "Service 1 removal should succeed");
	after_remove_count = get_service_count();
	zassert_equal(after_remove_count, 2, "Service count should be 2 after removing first");

	/* Remove second service */
	err = bt_rpc_gatt_remove_service(&test_svc_2);
	zassert_equal(err, 0, "Service 2 removal should succeed");
	zassert_equal(get_service_count(), 1, "Service count should be 1 after removing second");

	/* Remove last service - this should trigger buffer reset */
	err = bt_rpc_gatt_remove_service(&test_svc_3);
	zassert_equal(err, 0, "Service 3 removal should succeed");

	/* Verify buffer is empty after removing all services */
	final_count = get_service_count();
	zassert_equal(final_count, 0, "Service count should be 0 after removing all services");
	zassert_true(is_buffer_empty(), "Buffer should be empty after removing all services");

	/* Verify no services can be retrieved */
	zassert_is_null(bt_rpc_gatt_get_service_by_index(0),
			"Service at index 0 should not exist after cleanup");
}

/**
 * @brief Test: Multiple init/disable cycles
 *
 * This test simulates multiple bt_init/bt_disable cycles to ensure
 * the buffer doesn't leak memory across cycles.
 */
ZTEST(bt_rpc_gatt_service, test_multiple_init_disable_cycles)
{
	const int num_cycles = 10;
	uint32_t svc_index;
	int err;

	for (int cycle = 0; cycle < num_cycles; cycle++) {
		svc_index = (uint32_t)-1;
		/* Verify initial state is empty */
		zassert_true(is_buffer_empty(), "Buffer should be empty at start of cycle %d",
			     cycle);

		/* Register service (simulating bt_init) */
		err = bt_rpc_gatt_add_service(&test_svc_1, &svc_index);
		zassert_equal(err, 0, "Service registration should succeed in cycle %d", cycle);
		zassert_equal(get_service_count(), 1, "Service count should be 1 in cycle %d",
			      cycle);
		/* svc_index should be valid and when we retrieve the service by
		 * index it should be the same test_svc_1 that was added
		 */
		zassert_not_equal(svc_index, (uint32_t)-1, "svc_index should be valid in cycle %d",
				  cycle);
		const struct bt_gatt_service *svc = bt_rpc_gatt_get_service_by_index(svc_index);

		zassert_not_null(svc, "Service should be retrievable by index in cycle %d", cycle);
		zassert_equal_ptr(svc, &test_svc_1,
				  "Service at index should match test_svc_1 in cycle %d", cycle);

		/* Deregister service (simulating bt_disable) */
		err = bt_rpc_gatt_remove_service(&test_svc_1);
		zassert_equal(err, 0, "Service removal should succeed in cycle %d", cycle);

		/* Verify buffer is empty after each cycle */
		zassert_true(is_buffer_empty(), "Buffer should be empty after cycle %d", cycle);
		zassert_equal(get_service_count(), 0, "Service count should be 0 after cycle %d",
			      cycle);
	}
}

/**
 * @brief Test: Remove services in different orders
 */
ZTEST(bt_rpc_gatt_service, test_remove_services_different_orders)
{
	uint32_t svc_index_1, svc_index_2, svc_index_3;
	int err;

	/* Register 3 services */
	bt_rpc_gatt_add_service(&test_svc_1, &svc_index_1);
	bt_rpc_gatt_add_service(&test_svc_2, &svc_index_2);
	bt_rpc_gatt_add_service(&test_svc_3, &svc_index_3);

	zassert_equal(get_service_count(), 3, "All services should be registered");

	/* Remove middle service first */
	err = bt_rpc_gatt_remove_service(&test_svc_2);
	zassert_equal(err, 0, "Middle service removal should succeed");
	zassert_equal(get_service_count(), 2, "Service count should be 2");

	/* Remove last service */
	err = bt_rpc_gatt_remove_service(&test_svc_3);
	zassert_equal(err, 0, "Last service removal should succeed");
	zassert_equal(get_service_count(), 1, "Service count should be 1");

	/* Remove first service (last remaining) */
	err = bt_rpc_gatt_remove_service(&test_svc_1);
	zassert_equal(err, 0, "First service removal should succeed");

	/* Verify buffer is empty */
	zassert_true(is_buffer_empty(), "Buffer should be empty after removing all services");
}

/**
 * @brief Test: Service count should not exceed maximum even with many services
 *
 * This test adds services up to the maximum allowed and verifies that:
 * 1. We can add up to CONFIG_BT_RPC_GATT_SRV_MAX services
 * 2. Adding more than the maximum fails
 * 3. Removing services allows adding new ones again
 * 4. The count never exceeds the maximum
 */
ZTEST(bt_rpc_gatt_service, test_service_count_never_exceeds_maximum)
{
	const size_t max_services = CONFIG_BT_RPC_GATT_SRV_MAX;
	struct bt_gatt_service test_services[10];
	struct bt_gatt_attr test_attrs_array[10][2];
	uint32_t svc_indices[10];
	int err;
	size_t count;

	/* Create test services */
	for (size_t i = 0; i < 10; i++) {
		test_attrs_array[i][0] = (struct bt_gatt_attr){
			.uuid = &test_uuid_1,
			.handle = (uint16_t)(i * 10 + 1),
			.perm = 0,
			.read = NULL,
			.write = NULL,
			.user_data = NULL,
		};
		test_attrs_array[i][1] = (struct bt_gatt_attr){
			.uuid = &test_uuid_1,
			.handle = (uint16_t)(i * 10 + 2),
			.perm = 0,
			.read = NULL,
			.write = NULL,
			.user_data = (void *)(uintptr_t)(i + 100),
		};
		test_services[i] = (struct bt_gatt_service){
			.attrs = test_attrs_array[i],
			.attr_count = 2,
		};
	}

	/* Add services up to maximum */
	for (size_t i = 0; i < max_services; i++) {
		err = bt_rpc_gatt_add_service(&test_services[i], &svc_indices[i]);
		zassert_equal(err, 0, "Adding service %zu should succeed", i);
		count = get_service_count();
		zassert_equal(count, i + 1, "Service count should be %zu after adding service %zu",
			      i + 1, i);
		zassert_true(count <= max_services,
			     "Service count (%zu) should not exceed maximum (%zu)", count,
			     max_services);
	}

	/* Verify we're at maximum */
	count = get_service_count();
	zassert_equal(count, max_services, "Service count should equal maximum (%zu)",
		      max_services);

	/* Try to add one more - should fail */
	err = bt_rpc_gatt_add_service(&test_services[max_services], &svc_indices[max_services]);
	zassert_not_equal(err, 0, "Adding service beyond maximum should fail");
	count = get_service_count();
	zassert_equal(count, max_services, "Service count should still be %zu after failed add",
		      max_services);

	/* Remove all services */
	for (size_t i = 0; i < max_services; i++) {
		err = bt_rpc_gatt_remove_service(&test_services[i]);
		zassert_equal(err, 0, "Removing service %zu should succeed", i);
		count = get_service_count();
		zassert_equal(count, max_services - i - 1,
			      "Service count should be %zu after removing service %zu",
			      max_services - i - 1, i);
	}

	/* Verify empty state */
	count = get_service_count();
	zassert_equal(count, 0, "Service count should be 0 after removing all");
	zassert_true(is_buffer_empty(), "Buffer should be empty after removing all services");

	/* Now we should be able to add services again */
	err = bt_rpc_gatt_add_service(&test_svc_1, &svc_indices[0]);
	zassert_equal(err, 0, "Should be able to add service after cleanup");
	count = get_service_count();
	zassert_equal(count, 1, "Service count should be 1 after re-adding");
}

ZTEST_SUITE(bt_rpc_gatt_service, NULL, NULL, test_setup, test_teardown, NULL);
