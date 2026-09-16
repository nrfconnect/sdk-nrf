/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/init.h>
#include <net/mqtt_helper.h>

#include "zephyr/net/cmock_socket.h"
#include "zephyr/net/cmock_net_if.h"
#include "cmock_mqtt.h"

#define TEST_HOSTNAME		"test-some-host-name.net"
#define TEST_HOSTNAME_LEN	(sizeof(TEST_HOSTNAME) - 1)

#define TEST_DEVICE_ID		"test-device"
#define TEST_DEVICE_ID_LEN	(sizeof(TEST_DEVICE_ID) - 1)

#define TEST_USER_NAME		"test-username"
#define TEST_USER_NAME_LEN	(sizeof(TEST_USER_NAME) - 1)
#define TEST_IF_NAME		"testif0"

#define TEST_MESSAGE_ID		12346

#define TEST_TOPIC_1		"test/topic_1"
#define TEST_TOPIC_1_LEN	(sizeof(TEST_TOPIC_1) - 1)

#define TEST_TOPIC_2		"test/topic_2"
#define TEST_TOPIC_2_LEN	(sizeof(TEST_TOPIC_2) - 1)

#define TEST_PAYLOAD		"This is a test payload"
#define TEST_PAYLOAD_LEN	(sizeof(TEST_PAYLOAD) - 1)

/* Pull in variables and functions from the MQTT helper library. */
extern struct mqtt_client mqtt_client;
extern enum mqtt_state mqtt_state;
extern struct k_sem connection_poll_sem;
extern k_tid_t mqtt_helper_thread;
/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);
extern enum mqtt_state mqtt_state_get(void);
extern void mqtt_state_set(enum mqtt_state state);
extern void mqtt_evt_handler(struct mqtt_client *const mqtt_client,
			     const struct mqtt_evt *mqtt_evt);
extern void mqtt_helper_poll_loop(void);
extern void on_publish(const struct mqtt_evt *mqtt_evt);
extern char payload_buf[];

/* Create one local addrinfo node per macro use. Chain order is set in each test via ai_next. */
#define MAKE_ADDRINFO_NODE_V4(name)							\
	struct zsock_addrinfo name = {						\
		.ai_family = NET_AF_INET,					\
		.ai_addr = (struct net_sockaddr *)&(struct net_sockaddr_in){	\
			.sin_family = NET_AF_INET,				\
		},								\
		.ai_addrlen = sizeof(struct net_sockaddr_in),			\
		.ai_next = NULL,						\
	}

#define MAKE_ADDRINFO_NODE_V6(name)							\
	struct zsock_addrinfo name = {						\
		.ai_family = NET_AF_INET6,					\
		.ai_addr = (struct net_sockaddr *)&(struct net_sockaddr_in6){	\
			.sin6_family = NET_AF_INET6,				\
		},								\
		.ai_addrlen = sizeof(struct net_sockaddr_in6),			\
		.ai_next = NULL,						\
	}

#if defined(CONFIG_NET_IPV4)
static struct net_if test_iface;
static struct net_in_addr test_ipv4_addr;
#endif

#if defined(CONFIG_NET_IPV6)
static struct net_in6_addr test_ipv6_addr;
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
static struct net_if test_iface_named;
#endif

/* Semaphores used by tests to wait for a certain callbacks */
static K_SEM_DEFINE(connack_success_sem, 0, 1);
static K_SEM_DEFINE(connack_failed_sem, 0, 1);
static K_SEM_DEFINE(disconnect_sem, 0, 1);
static K_SEM_DEFINE(puback_sem, 0, 1);
static K_SEM_DEFINE(suback_sem, 0, 1);
static K_SEM_DEFINE(publish_sem, 0, 1);
static K_SEM_DEFINE(error_msg_size_sem, 0, 1);

void setUp(void)
{
	__cmock_mqtt_keepalive_time_left_IgnoreAndReturn(0);

	/* Suspend the polling thread to have full control over polling. */
	k_thread_suspend(mqtt_helper_thread);

	/* Force all tests to start in uninitialized state. */
	mqtt_state = MQTT_STATE_UNINIT;

	/* Reset mqtt_helper_msg_id_get() internal counter */
	while (UINT16_MAX != mqtt_helper_msg_id_get()) {
		/* Do nothing */
	};
}

/* Stubs */
static int mqtt_readall_publish_payload_stub(struct mqtt_client *client, uint8_t *buffer,
					     size_t length, int num_calls)
{
	memcpy(payload_buf, TEST_PAYLOAD, TEST_PAYLOAD_LEN);

	return 0;
}

static int poll_stub_pollin(struct zsock_pollfd *fds, int nfds, int timeout, int num_calls)
{
	fds[0].revents = fds[0].events & ZSOCK_POLLIN;

	/* The stub should only return 1 the first time, and otherwise return
	 * and error to let the calling function return.
	 */

	return num_calls == 0 ? 1 : -1;
}

static int poll_stub_pollnval(struct zsock_pollfd *fds, int nfds, int timeout, int num_calls)
{
	fds[0].revents = ZSOCK_POLLNVAL;

	return num_calls == 0 ? 1 : -1;
}

static int poll_stub_pollhup(struct zsock_pollfd *fds, int nfds, int timeout, int num_calls)
{
	fds[0].revents = ZSOCK_POLLHUP;

	return num_calls == 0 ? 1 : -1;
}

static int poll_stub_pollerr(struct zsock_pollfd *fds, int nfds, int timeout, int num_calls)
{
	fds[0].revents = ZSOCK_POLLERR;

	return num_calls == 0 ? 1 : -1;
}

static int net_if_get_by_name_stub(const char *name, int num_calls)
{
	ARG_UNUSED(num_calls);

	return strcmp(name, "invalid0") == 0 ? 0 : 10;
}


/* Helper functions */
static struct mqtt_helper_conn_params default_conn_params(void)
{
	return (struct mqtt_helper_conn_params){
		.hostname = {
			.ptr = TEST_HOSTNAME,
			.size = TEST_HOSTNAME_LEN,
		},
		.user_name = {
			.ptr = TEST_USER_NAME,
			.size = TEST_USER_NAME_LEN,
		},
		.device_id = {
			.ptr = TEST_DEVICE_ID,
			.size = TEST_DEVICE_ID_LEN,
		},
	};
}

static struct mqtt_helper_conn_params conn_params_with_if_name(const char *if_name)
{
	struct mqtt_helper_conn_params conn_params = default_conn_params();

	conn_params.if_name = if_name;

	return conn_params;
}

static void send_publish_event(int message_id)
{
	struct mqtt_evt evt = {
		.type = MQTT_EVT_PUBLISH,
		.result = 0,
		.param.publish.message = {
			.topic = {
				.topic = {
					.utf8 = TEST_TOPIC_1,
					.size = TEST_TOPIC_1_LEN,
				},
				.qos = MQTT_QOS_1_AT_LEAST_ONCE,
			},
			.payload = {
				.data = TEST_PAYLOAD,
				.len = TEST_PAYLOAD_LEN,
			},
		}
	};

	mqtt_evt_handler(&mqtt_client, &evt);
}

static void send_mqtt_event(enum mqtt_evt_type type, int optional_data)
{
	struct mqtt_evt evt = {
		.type = type,
		.result = 0,
	};

	switch (type) {
	case MQTT_EVT_CONNACK:
		evt.param.connack.return_code = optional_data;
		break;
	case MQTT_EVT_DISCONNECT:
		break;
	case MQTT_EVT_PUBACK:
		evt.param.puback.message_id = optional_data;
		break;
	case MQTT_EVT_SUBACK:
		evt.param.suback.message_id = optional_data;
		break;
	case MQTT_EVT_PUBLISH:
		send_publish_event(optional_data);
		return;
	default:
		/* Unahndled event type, should not happen and considered bug
		 * in the test.
		 */
		TEST_ASSERT_TRUE(false);
	}

	mqtt_evt_handler(&mqtt_client, &evt);
}

/* Callbacks used in tests. */
static void cb_on_publish(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload)
{
	TEST_ASSERT_EQUAL(TEST_TOPIC_1_LEN, topic.size);
	TEST_ASSERT_EQUAL_MEMORY(TEST_TOPIC_1, topic.ptr, TEST_TOPIC_1_LEN);
	TEST_ASSERT_EQUAL(TEST_PAYLOAD_LEN, payload.size);
	TEST_ASSERT_EQUAL_MEMORY(TEST_PAYLOAD, payload.ptr, TEST_PAYLOAD_LEN);

	k_sem_give(&publish_sem);
}

static void cb_on_connack(enum mqtt_conn_return_code return_code, bool session_present)
{
	switch (return_code) {
	case MQTT_CONNECTION_ACCEPTED:
		k_sem_give(&connack_success_sem);
		break;

	case MQTT_NOT_AUTHORIZED:
		k_sem_give(&connack_failed_sem);
		break;
	default:
		/* Unexpected return code, should not happen and considered bug in the test. */
		TEST_ASSERT_TRUE(false);
	}
}

static void cb_on_disconnect(int result)
{
	k_sem_give(&disconnect_sem);
}

static void cb_on_puback(uint16_t message_id, int result)
{
	if (message_id == TEST_MESSAGE_ID) {
		k_sem_give(&puback_sem);
	}
}

static void cb_on_suback(uint16_t message_id, int result)
{
	if (message_id == TEST_MESSAGE_ID) {
		k_sem_give(&suback_sem);
	}
}

static void cb_on_error(enum mqtt_helper_error error)
{
	if (error == MQTT_HELPER_ERROR_MSG_SIZE) {
		k_sem_give(&error_msg_size_sem);
	}
}

/* Tests */

void test_mqtt_helper_init_when_unitialized(void)
{
	struct mqtt_helper_cfg cfg = {
		.cb = {
			.on_connack = cb_on_connack,
			.on_disconnect = cb_on_disconnect,
			.on_publish = cb_on_publish,
			.on_puback = cb_on_puback,
			.on_suback = cb_on_suback,
			.on_error = cb_on_error,
		},
	};

	__cmock_mqtt_client_init_Expect(&mqtt_client);

	TEST_ASSERT_EQUAL(0, mqtt_helper_init(&cfg));
	TEST_ASSERT_EQUAL(mqtt_state_get(), MQTT_STATE_DISCONNECTED);
}

void test_mqtt_helper_init_when_connected(void)
{
	struct mqtt_helper_cfg cfg;

	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, mqtt_helper_init(&cfg));
}

/* Steps:
 * 1) Resolve one IPv4 broker address.
 * 2) Report local IPv4 as ready.
 * 3) Verify first connect attempt succeeds.
 */
void test_mqtt_helper_connect_when_disconnected(void)
{
#if defined(CONFIG_NET_IPV4)
	struct mqtt_helper_conn_params conn_params = default_conn_params();

	MAKE_ADDRINFO_NODE_V4(ai_v4);
	struct zsock_addrinfo *test_res = &ai_v4;

	__cmock_zsock_getaddrinfo_ExpectAndReturn(NULL, NULL, NULL, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_host();
	__cmock_zsock_getaddrinfo_IgnoreArg_hints();
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&test_res);

	__cmock_net_if_get_default_ExpectAndReturn(&test_iface);
	__cmock_net_if_ipv4_get_global_addr_ExpectAndReturn(&test_iface, NET_ADDR_PREFERRED,
							     &test_ipv4_addr);

	__cmock_zsock_inet_ntop_IgnoreAndReturn(NULL);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, 0);
	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_connect(&conn_params));
	TEST_ASSERT_EQUAL(MQTT_STATE_CONNECTING, mqtt_state_get());
#else
	TEST_IGNORE_MESSAGE("Requires CONFIG_NET_IPV4");
#endif
}

/* Steps:
 * 1) Resolve one IPv4 broker address.
 * 2) Report local IPv4 as ready.
 * 3) Verify connect error is propagated.
 */
void test_mqtt_helper_connect_when_disconnected_mqtt_api_error(void)
{
#if defined(CONFIG_NET_IPV4)
	struct mqtt_helper_conn_params conn_params_dummy = default_conn_params();

	MAKE_ADDRINFO_NODE_V4(ai_v4);
	struct zsock_addrinfo *test_res = &ai_v4;

	__cmock_zsock_getaddrinfo_ExpectAndReturn(NULL, NULL, NULL, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_host();
	__cmock_zsock_getaddrinfo_IgnoreArg_hints();
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&test_res);

	__cmock_net_if_get_default_ExpectAndReturn(&test_iface);
	__cmock_net_if_ipv4_get_global_addr_ExpectAndReturn(&test_iface, NET_ADDR_PREFERRED,
							     &test_ipv4_addr);

	__cmock_zsock_inet_ntop_IgnoreAndReturn(NULL);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, -ENOENT);
	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(-ENOENT, mqtt_helper_connect(&conn_params_dummy));
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
#else
	TEST_IGNORE_MESSAGE("Requires CONFIG_NET_IPV4");
#endif
}

/* Steps:
 * 1) Resolve IPv6 first, then IPv4.
 * 2) Report IPv6 not ready and IPv4 ready.
 * 3) Verify IPv6 is skipped and IPv4 connects.
 */
void test_mqtt_helper_connect_skips_unready_family(void)
{
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
	struct mqtt_helper_conn_params conn_params = default_conn_params();

	MAKE_ADDRINFO_NODE_V6(ai_v6);
	MAKE_ADDRINFO_NODE_V4(ai_v4);
	struct zsock_addrinfo *test_res = &ai_v6;

	ai_v6.ai_next = &ai_v4;

	__cmock_zsock_getaddrinfo_ExpectAndReturn(NULL, NULL, NULL, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_host();
	__cmock_zsock_getaddrinfo_IgnoreArg_hints();
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&test_res);

	__cmock_net_if_ipv6_get_global_addr_ExpectAndReturn(NET_ADDR_PREFERRED, NULL, NULL);
	__cmock_net_if_ipv6_get_global_addr_IgnoreArg_iface();
	__cmock_net_if_get_default_ExpectAndReturn(&test_iface);
	__cmock_net_if_ipv4_get_global_addr_ExpectAndReturn(&test_iface, NET_ADDR_PREFERRED,
							     &test_ipv4_addr);

	__cmock_zsock_inet_ntop_IgnoreAndReturn(NULL);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, 0);
	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_connect(&conn_params));
	TEST_ASSERT_EQUAL(MQTT_STATE_CONNECTING, mqtt_state_get());
#else
	TEST_IGNORE_MESSAGE("Requires CONFIG_NET_IPV4 and CONFIG_NET_IPV6");
#endif
}

/* Steps:
 * 1) Resolve only IPv6 broker address.
 * 2) Report no local IPv6 address.
 * 3) Verify no address is usable and ENETUNREACH is returned.
 */
void test_mqtt_helper_connect_fails_on_family_mismatch(void)
{
#if defined(CONFIG_NET_IPV6)
	struct mqtt_helper_conn_params conn_params = default_conn_params();

	MAKE_ADDRINFO_NODE_V6(ai_v6);
	struct zsock_addrinfo *test_res = &ai_v6;

	__cmock_zsock_getaddrinfo_ExpectAndReturn(NULL, NULL, NULL, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_host();
	__cmock_zsock_getaddrinfo_IgnoreArg_hints();
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&test_res);

	__cmock_net_if_ipv6_get_global_addr_ExpectAndReturn(NET_ADDR_PREFERRED, NULL, NULL);
	__cmock_net_if_ipv6_get_global_addr_IgnoreArg_iface();
	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(-ENETUNREACH, mqtt_helper_connect(&conn_params));
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
#else
	TEST_IGNORE_MESSAGE("Requires CONFIG_NET_IPV6");
#endif
}

/* Steps:
 * 1) Resolve 4 addresses (IPv6, IPv4, IPv6, IPv4).
 * 2) Report both families ready.
 * 3) Verify all 4 connect attempts are made and last error is returned.
 */
void test_mqtt_helper_connect_iterates_all_addresses_both_families_fail(void)
{
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
	struct mqtt_helper_conn_params conn_params = default_conn_params();

	MAKE_ADDRINFO_NODE_V6(ai_v6_1);
	MAKE_ADDRINFO_NODE_V4(ai_v4_1);
	MAKE_ADDRINFO_NODE_V6(ai_v6_2);
	MAKE_ADDRINFO_NODE_V4(ai_v4_2);
	struct zsock_addrinfo *test_res = &ai_v6_1;

	ai_v6_1.ai_next = &ai_v4_1;
	ai_v4_1.ai_next = &ai_v6_2;
	ai_v6_2.ai_next = &ai_v4_2;

	__cmock_zsock_getaddrinfo_ExpectAndReturn(NULL, NULL, NULL, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_host();
	__cmock_zsock_getaddrinfo_IgnoreArg_hints();
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&test_res);

	__cmock_net_if_ipv6_get_global_addr_ExpectAndReturn(NET_ADDR_PREFERRED, NULL,
							    &test_ipv6_addr);
	__cmock_net_if_ipv6_get_global_addr_IgnoreArg_iface();
	__cmock_net_if_get_default_ExpectAndReturn(&test_iface);
	__cmock_net_if_ipv4_get_global_addr_ExpectAndReturn(&test_iface, NET_ADDR_PREFERRED,
							     &test_ipv4_addr);
	__cmock_net_if_ipv6_get_global_addr_ExpectAndReturn(NET_ADDR_PREFERRED, NULL,
							    &test_ipv6_addr);
	__cmock_net_if_ipv6_get_global_addr_IgnoreArg_iface();
	__cmock_net_if_get_default_ExpectAndReturn(&test_iface);
	__cmock_net_if_ipv4_get_global_addr_ExpectAndReturn(&test_iface, NET_ADDR_PREFERRED,
							     &test_ipv4_addr);

	__cmock_zsock_inet_ntop_IgnoreAndReturn(NULL);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, -ECONNREFUSED);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, -ETIMEDOUT);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, -EHOSTUNREACH);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, -EHOSTUNREACH);
	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(-EHOSTUNREACH, mqtt_helper_connect(&conn_params));
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
#else
	TEST_IGNORE_MESSAGE("Requires CONFIG_NET_IPV4 and CONFIG_NET_IPV6");
#endif
}

/* Steps:
 * 1) Resolve 4 addresses (IPv6, IPv6, IPv4, IPv4).
 * 2) Report both families ready.
 * 3) Verify retries continue until one address connects.
 */
void test_mqtt_helper_connect_iterates_all_addresses_both_families_connect(void)
{
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
	struct mqtt_helper_conn_params conn_params = default_conn_params();

	MAKE_ADDRINFO_NODE_V6(ai_v6_1);
	MAKE_ADDRINFO_NODE_V6(ai_v6_2);
	MAKE_ADDRINFO_NODE_V4(ai_v4_1);
	MAKE_ADDRINFO_NODE_V4(ai_v4_2);
	struct zsock_addrinfo *test_res = &ai_v6_1;

	ai_v6_1.ai_next = &ai_v6_2;
	ai_v6_2.ai_next = &ai_v4_1;
	ai_v4_1.ai_next = &ai_v4_2;

	__cmock_zsock_getaddrinfo_ExpectAndReturn(NULL, NULL, NULL, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_host();
	__cmock_zsock_getaddrinfo_IgnoreArg_hints();
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&test_res);

	__cmock_net_if_ipv6_get_global_addr_ExpectAndReturn(NET_ADDR_PREFERRED, NULL,
							    &test_ipv6_addr);
	__cmock_net_if_ipv6_get_global_addr_IgnoreArg_iface();
	__cmock_net_if_ipv6_get_global_addr_ExpectAndReturn(NET_ADDR_PREFERRED, NULL,
							    &test_ipv6_addr);
	__cmock_net_if_ipv6_get_global_addr_IgnoreArg_iface();
	__cmock_net_if_get_default_ExpectAndReturn(&test_iface);
	__cmock_net_if_ipv4_get_global_addr_ExpectAndReturn(&test_iface, NET_ADDR_PREFERRED,
							     &test_ipv4_addr);

	__cmock_zsock_inet_ntop_IgnoreAndReturn(NULL);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, -ECONNREFUSED);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, -ETIMEDOUT);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, 0);
	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_connect(&conn_params));
	TEST_ASSERT_EQUAL(MQTT_STATE_CONNECTING, mqtt_state_get());
#else
	TEST_IGNORE_MESSAGE("Requires CONFIG_NET_IPV4 and CONFIG_NET_IPV6");
#endif
}

/* Steps:
 * 1) Resolve mixed addresses (IPv4 then IPv6).
 * 2) Report local IPv4 not ready and IPv6 ready.
 * 3) Verify IPv4 is skipped and IPv6 is tried.
 */
void test_mqtt_helper_connect_ipv6_only_device_mixed_resolved(void)
{
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
	struct mqtt_helper_conn_params conn_params = default_conn_params();

	MAKE_ADDRINFO_NODE_V4(ai_v4);
	MAKE_ADDRINFO_NODE_V6(ai_v6);
	struct zsock_addrinfo *test_res = &ai_v4;

	ai_v4.ai_next = &ai_v6;

	__cmock_zsock_getaddrinfo_ExpectAndReturn(NULL, NULL, NULL, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_host();
	__cmock_zsock_getaddrinfo_IgnoreArg_hints();
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&test_res);

	__cmock_net_if_get_default_ExpectAndReturn(&test_iface);
	__cmock_net_if_ipv4_get_global_addr_ExpectAndReturn(&test_iface, NET_ADDR_PREFERRED, NULL);
	__cmock_net_if_ipv6_get_global_addr_ExpectAndReturn(NET_ADDR_PREFERRED, NULL,
							    &test_ipv6_addr);
	__cmock_net_if_ipv6_get_global_addr_IgnoreArg_iface();

	__cmock_zsock_inet_ntop_IgnoreAndReturn(NULL);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, 0);
	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_connect(&conn_params));
	TEST_ASSERT_EQUAL(MQTT_STATE_CONNECTING, mqtt_state_get());
#else
	TEST_IGNORE_MESSAGE("Requires CONFIG_NET_IPV4 and CONFIG_NET_IPV6");
#endif
}

/* Steps:
 * 1) Resolve only IPv4 broker address.
 * 2) Report local IPv4 not ready (IPv6-only device state).
 * 3) Verify mismatch returns ENETUNREACH.
 */
void test_mqtt_helper_connect_ipv4_only_resolved_ipv6_only_device_mismatch(void)
{
#if defined(CONFIG_NET_IPV4)
	struct mqtt_helper_conn_params conn_params = default_conn_params();

	MAKE_ADDRINFO_NODE_V4(ai_v4);
	struct zsock_addrinfo *test_res = &ai_v4;

	__cmock_zsock_getaddrinfo_ExpectAndReturn(NULL, NULL, NULL, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_host();
	__cmock_zsock_getaddrinfo_IgnoreArg_hints();
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&test_res);

	__cmock_net_if_get_default_ExpectAndReturn(&test_iface);
	__cmock_net_if_ipv4_get_global_addr_ExpectAndReturn(&test_iface, NET_ADDR_PREFERRED, NULL);
	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(-ENETUNREACH, mqtt_helper_connect(&conn_params));
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
#else
	TEST_IGNORE_MESSAGE("Requires CONFIG_NET_IPV4");
#endif
}

/* Steps:
 * 1) Resolve mixed addresses (IPv4 and IPv6).
 * 2) Report no local IPv4 and no local IPv6.
 * 3) Verify both families are skipped and ENETUNREACH is returned.
 */
void test_mqtt_helper_connect_mixed_resolved_device_none(void)
{
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
	struct mqtt_helper_conn_params conn_params = default_conn_params();

	MAKE_ADDRINFO_NODE_V4(ai_v4);
	MAKE_ADDRINFO_NODE_V6(ai_v6);
	struct zsock_addrinfo *test_res = &ai_v4;

	ai_v4.ai_next = &ai_v6;

	__cmock_zsock_getaddrinfo_ExpectAndReturn(NULL, NULL, NULL, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_host();
	__cmock_zsock_getaddrinfo_IgnoreArg_hints();
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&test_res);

	__cmock_net_if_get_default_ExpectAndReturn(&test_iface);
	__cmock_net_if_ipv4_get_global_addr_ExpectAndReturn(&test_iface, NET_ADDR_PREFERRED, NULL);
	__cmock_net_if_ipv6_get_global_addr_ExpectAndReturn(NET_ADDR_PREFERRED, NULL, NULL);
	__cmock_net_if_ipv6_get_global_addr_IgnoreArg_iface();
	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(-ENETUNREACH, mqtt_helper_connect(&conn_params));
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
#else
	TEST_IGNORE_MESSAGE("Requires CONFIG_NET_IPV4 and CONFIG_NET_IPV6");
#endif
}

/* Steps:
 * 1) Resolve two IPv4 addresses.
 * 2) Report local IPv4 ready for both checks.
 * 3) Verify first connect fails and second succeeds.
 */
void test_mqtt_helper_connect_iterates_two_ipv4_addresses(void)
{
#if defined(CONFIG_NET_IPV4)
	struct mqtt_helper_conn_params conn_params = default_conn_params();

	MAKE_ADDRINFO_NODE_V4(ai_v4_1);
	MAKE_ADDRINFO_NODE_V4(ai_v4_2);
	struct zsock_addrinfo *test_res = &ai_v4_1;

	ai_v4_1.ai_next = &ai_v4_2;

	__cmock_zsock_getaddrinfo_ExpectAndReturn(NULL, NULL, NULL, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_host();
	__cmock_zsock_getaddrinfo_IgnoreArg_hints();
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&test_res);

	__cmock_net_if_get_default_ExpectAndReturn(&test_iface);
	__cmock_net_if_ipv4_get_global_addr_ExpectAndReturn(&test_iface, NET_ADDR_PREFERRED,
							     &test_ipv4_addr);
	__cmock_net_if_get_default_ExpectAndReturn(&test_iface);
	__cmock_net_if_ipv4_get_global_addr_ExpectAndReturn(&test_iface, NET_ADDR_PREFERRED,
							     &test_ipv4_addr);

	__cmock_zsock_inet_ntop_IgnoreAndReturn(NULL);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, -ECONNREFUSED);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, 0);
	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_connect(&conn_params));
	TEST_ASSERT_EQUAL(MQTT_STATE_CONNECTING, mqtt_state_get());
#else
	TEST_IGNORE_MESSAGE("Requires CONFIG_NET_IPV4");
#endif
}

/* Steps:
 * 1) Resolve two IPv6 addresses.
 * 2) Report local IPv6 ready for both checks.
 * 3) Verify first connect fails and second succeeds.
 */
void test_mqtt_helper_connect_iterates_two_ipv6_addresses(void)
{
#if defined(CONFIG_NET_IPV6)
	struct mqtt_helper_conn_params conn_params = default_conn_params();

	MAKE_ADDRINFO_NODE_V6(ai_v6_1);
	MAKE_ADDRINFO_NODE_V6(ai_v6_2);
	struct zsock_addrinfo *test_res = &ai_v6_1;

	ai_v6_1.ai_next = &ai_v6_2;

	__cmock_zsock_getaddrinfo_ExpectAndReturn(NULL, NULL, NULL, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_host();
	__cmock_zsock_getaddrinfo_IgnoreArg_hints();
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&test_res);

	__cmock_net_if_ipv6_get_global_addr_ExpectAndReturn(NET_ADDR_PREFERRED, NULL,
							    &test_ipv6_addr);
	__cmock_net_if_ipv6_get_global_addr_IgnoreArg_iface();
	__cmock_net_if_ipv6_get_global_addr_ExpectAndReturn(NET_ADDR_PREFERRED, NULL,
							    &test_ipv6_addr);
	__cmock_net_if_ipv6_get_global_addr_IgnoreArg_iface();

	__cmock_zsock_inet_ntop_IgnoreAndReturn(NULL);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, -ECONNREFUSED);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, 0);
	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_connect(&conn_params));
	TEST_ASSERT_EQUAL(MQTT_STATE_CONNECTING, mqtt_state_get());
#else
	TEST_IGNORE_MESSAGE("Requires CONFIG_NET_IPV6");
#endif
}

/* Steps:
 * 1) Bind connection to named interface via if_name.
 * 2) Resolve IPv6 then IPv4, with IPv6 not ready on that interface.
 * 3) Verify IPv6 is skipped and IPv4 uses the named interface readiness.
 */
void test_mqtt_helper_connect_if_name_ipv4_only_iface(void)
{
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
	struct mqtt_helper_conn_params conn_params = conn_params_with_if_name(TEST_IF_NAME);

	MAKE_ADDRINFO_NODE_V6(ai_v6);
	MAKE_ADDRINFO_NODE_V4(ai_v4);
	struct zsock_addrinfo *test_res = &ai_v6;

	ai_v6.ai_next = &ai_v4;

	/* native_sim may trigger extra net_if lookups; keep iface selection deterministic in CI. */
	__cmock_net_if_get_by_name_IgnoreAndReturn(10);
	__cmock_net_if_get_by_index_IgnoreAndReturn(&test_iface_named);
	__cmock_net_if_get_default_IgnoreAndReturn(&test_iface_named);
	__cmock_zsock_getaddrinfo_ExpectAndReturn(NULL, NULL, NULL, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_host();
	__cmock_zsock_getaddrinfo_IgnoreArg_hints();
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&test_res);

	__cmock_net_if_ipv6_get_global_addr_ExpectAnyArgsAndReturn(NULL);
	__cmock_net_if_ipv4_get_global_addr_ExpectAndReturn(&test_iface_named, NET_ADDR_PREFERRED,
							     &test_ipv4_addr);

	__cmock_zsock_inet_ntop_IgnoreAndReturn(NULL);
	__cmock_mqtt_connect_ExpectAndReturn(&mqtt_client, 0);
	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_connect(&conn_params));
	TEST_ASSERT_EQUAL(MQTT_STATE_CONNECTING, mqtt_state_get());
#else
	TEST_IGNORE_MESSAGE("Requires CONFIG_NET_IPV4 and CONFIG_NET_IPV6");
#endif
}

/* Steps:
 * 1) Set if_name to a non-existing interface.
 * 2) Trigger connect and verify it returns an error.
 * 3) Verify state remains disconnected.
 */
void test_mqtt_helper_connect_if_name_invalid(void)
{
	struct mqtt_helper_conn_params conn_params = conn_params_with_if_name("invalid0");
	int err;

	mqtt_state = MQTT_STATE_DISCONNECTED;

	/* Native background code may call getaddrinfo during test runtime. */
	__cmock_zsock_getaddrinfo_IgnoreAndReturn(1);
	/* Allow unrelated background lookups while keeping invalid interface deterministic. */
	__cmock_net_if_get_by_name_Stub(net_if_get_by_name_stub);

	err = mqtt_helper_connect(&conn_params);
	TEST_ASSERT_TRUE(err < 0);
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

void test_mqtt_helper_connect_when_uninitialized(void)
{
	struct mqtt_helper_conn_params conn_params;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, mqtt_helper_connect(&conn_params));
}

void test_on_connack_successful(void)
{
	mqtt_state = MQTT_STATE_CONNECTING;

	send_mqtt_event(MQTT_EVT_CONNACK, MQTT_CONNECTION_ACCEPTED);

	TEST_ASSERT_EQUAL(0, k_sem_take(&connack_success_sem, K_SECONDS(1)));
	TEST_ASSERT_EQUAL(MQTT_STATE_CONNECTED, mqtt_state_get());
}

void test_on_connack_failed(void)
{
	mqtt_state = MQTT_STATE_CONNECTING;

	send_mqtt_event(MQTT_EVT_CONNACK, MQTT_NOT_AUTHORIZED);

	TEST_ASSERT_EQUAL(0, k_sem_take(&connack_failed_sem, K_SECONDS(1)));
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

void test_on_disconnect(void)
{
	mqtt_state = MQTT_STATE_CONNECTED;

	send_mqtt_event(MQTT_EVT_DISCONNECT, 0);

	TEST_ASSERT_EQUAL(0, k_sem_take(&disconnect_sem, K_SECONDS(1)));
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

void test_on_puback(void)
{
	send_mqtt_event(MQTT_EVT_PUBACK, TEST_MESSAGE_ID);

	TEST_ASSERT_EQUAL(0, k_sem_take(&puback_sem, K_SECONDS(1)));
}

void test_on_suback(void)
{
	send_mqtt_event(MQTT_EVT_SUBACK, TEST_MESSAGE_ID);

	TEST_ASSERT_EQUAL(0, k_sem_take(&suback_sem, K_SECONDS(1)));
}

void test_on_publish(void)
{
	__cmock_mqtt_readall_publish_payload_Stub(mqtt_readall_publish_payload_stub);
	__cmock_mqtt_publish_qos1_ack_ExpectAnyArgsAndReturn(0);

	send_mqtt_event(MQTT_EVT_PUBLISH, TEST_MESSAGE_ID);

	TEST_ASSERT_EQUAL(0, k_sem_take(&publish_sem, K_SECONDS(1)));
}

void test_on_publish_too_large_incoming_msg(void)
{
	struct mqtt_evt evt = {
		.type = MQTT_EVT_PUBLISH,
		.param.publish.message = {
			.payload = {
				.len = CONFIG_MQTT_HELPER_PAYLOAD_BUFFER_LEN + 1,
			},
		}
	};

	mqtt_evt_handler(&mqtt_client, &evt);
	TEST_ASSERT_EQUAL(0, k_sem_take(&error_msg_size_sem, K_SECONDS(1)));
}

void test_mqtt_helper_disconnect_when_connected(void)
{
	__cmock_mqtt_disconnect_ExpectAndReturn(&mqtt_client, NULL, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_disconnect());
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTING, mqtt_state_get());
}

void test_mqtt_helper_disconnect_when_connected_mqtt_api_error(void)
{
	__cmock_mqtt_disconnect_ExpectAndReturn(&mqtt_client, NULL, -1);

	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-1, mqtt_helper_disconnect());
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, mqtt_state_get());
}

void test_mqtt_helper_disconnect_when_disconnected(void)
{
	mqtt_state = MQTT_STATE_UNINIT;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, mqtt_helper_disconnect());
}

void test_mqtt_helper_subscribe_when_connected(void)
{
	struct mqtt_topic sub_topics[] = {
		{
			.topic.utf8 = TEST_TOPIC_1,
			.topic.size = TEST_TOPIC_1_LEN,
		},
		{
			.topic.utf8 = TEST_TOPIC_2,
			.topic.size = TEST_TOPIC_2_LEN,
		},
	};
	struct mqtt_subscription_list sub_list = {
		.list = sub_topics,
		.list_count = ARRAY_SIZE(sub_topics),
		.message_id = TEST_MESSAGE_ID,
	};

	__cmock_mqtt_subscribe_ExpectAndReturn(&mqtt_client, &sub_list, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_subscribe(&sub_list));
}

void test_mqtt_helper_subscribe_when_disconnected(void)
{
	struct mqtt_subscription_list sub_list_dummy = {
		.message_id = mqtt_helper_msg_id_get(),
	};

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, mqtt_helper_subscribe(&sub_list_dummy));
}

void test_mqtt_helper_subscribe_mqtt_api_error(void)
{
	struct mqtt_subscription_list sub_list_dummy = {
		.message_id = mqtt_helper_msg_id_get(),
	};

	__cmock_mqtt_subscribe_ExpectAndReturn(&mqtt_client, &sub_list_dummy, -EINVAL);

	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-EINVAL, mqtt_helper_subscribe(&sub_list_dummy));
}

void test_mqtt_helper_subscribe_invalid_message_id(void)
{
	struct mqtt_subscription_list sub_list_dummy = { 0 };

	TEST_ASSERT_EQUAL(-EINVAL, mqtt_helper_subscribe(&sub_list_dummy));
}

void test_mqtt_helper_publish_when_connected(void)
{
	struct mqtt_publish_param pub_param = {
		.message = {
			.payload = {
				.data = TEST_PAYLOAD,
				.len = TEST_PAYLOAD_LEN,
			},
			.topic = {
				.topic = {
					.utf8 = TEST_TOPIC_1,
					.size = TEST_TOPIC_1_LEN,
				},
				.qos = MQTT_QOS_1_AT_LEAST_ONCE,
			},
		},
		.message_id = TEST_MESSAGE_ID,
	};

	__cmock_mqtt_publish_ExpectAndReturn(&mqtt_client, &pub_param, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_publish(&pub_param));
}

void test_mqtt_helper_publish_when_disconnected(void)
{
	struct mqtt_publish_param pub_param_dummy = {
		.message_id = mqtt_helper_msg_id_get(),
	};

	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, mqtt_helper_publish(&pub_param_dummy));
}

void test_mqtt_helper_publish_invalid_message_id(void)
{
	struct mqtt_publish_param pub_param_dummy = { 0 };

	TEST_ASSERT_EQUAL(-EINVAL, mqtt_helper_publish(&pub_param_dummy));
}

void test_mqtt_helper_deinit_when_disconnected(void)
{
	mqtt_state = MQTT_STATE_DISCONNECTED;

	TEST_ASSERT_EQUAL(0, mqtt_helper_deinit());
	TEST_ASSERT_EQUAL(MQTT_STATE_UNINIT, mqtt_state_get());
}

void test_mqtt_helper_deinit_when_connected(void)
{
	mqtt_state = MQTT_STATE_CONNECTED;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, mqtt_helper_deinit());
}

/* Test that the polling stops and state is left unchanged when the
 * library has already initiated disconnection.
 * It's expected that no socket or MQTT APIs are called.
 */
void test_mqtt_helper_poll_loop_disconnecting(void)
{
	mqtt_state = MQTT_STATE_DISCONNECTING;

	k_sem_give(&connection_poll_sem);
	mqtt_helper_poll_loop();
	TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTING, mqtt_state_get());
}

/* The test verifies that mqtt_live() is called when poll() returns 0. */
void test_mqtt_helper_poll_loop_timeout(void)
{
	/* Let poll() return 0 first and then -ENOTCONN on subsequent call to end the test. */
	__cmock_zsock_poll_ExpectAnyArgsAndReturn(0);
	__cmock_zsock_poll_ExpectAnyArgsAndReturn(-ENOTCONN);
	__cmock_mqtt_live_ExpectAndReturn(&mqtt_client, 0);

	/* mqtt_abort() should be called when the connection is dropped. */
	__cmock_mqtt_abort_ExpectAndReturn(&mqtt_client, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	k_sem_give(&connection_poll_sem);
	mqtt_helper_poll_loop();
}

/* The test verifies that mqtt_helper_poll_loop sets the fd's events
 * mask to ZSOCK_POLLIN and that mqtt_input is subsequently called when poll() returns
 * ZSOCK_POLLIN in the revents.
 */
void test_mqtt_helper_poll_loop_pollin(void)
{
	__cmock_zsock_poll_Stub(poll_stub_pollin);
	__cmock_mqtt_input_ExpectAndReturn(&mqtt_client, 0);

	/* mqtt_abort() should be called when the poll fails (after the second call). */
	__cmock_mqtt_abort_ExpectAndReturn(&mqtt_client, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	k_sem_give(&connection_poll_sem);
	mqtt_helper_poll_loop();
}

/* The test verifies that mqtt_helper_poll_loop sets the fd's events
 * mask to ZSOCK_POLLNVAL and that no other calls are made to socket or MQTT APIs subsequently.
 */
void test_mqtt_helper_poll_loop_pollnval(void)
{
	__cmock_zsock_poll_Stub(poll_stub_pollnval);
	__cmock_mqtt_abort_ExpectAndReturn(&mqtt_client, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	k_sem_give(&connection_poll_sem);
	mqtt_helper_poll_loop();
}

/* The test verifies that mqtt_helper_poll_loop sets the fd's events
 * mask to ZSOCK_POLLHUP and that no other calls are made to socket or MQTT APIs subsequently.
 */
void test_mqtt_helper_poll_loop_pollhup(void)
{
	__cmock_zsock_poll_Stub(poll_stub_pollhup);
	__cmock_mqtt_abort_ExpectAndReturn(&mqtt_client, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	k_sem_give(&connection_poll_sem);
	mqtt_helper_poll_loop();
}

/* The test verifies that mqtt_helper_poll_loop sets the fd's events
 * mask to ZSOCK_POLLERR and that no other calls are made to socket or MQTT APIs subsequently.
 */
void test_mqtt_helper_poll_loop_pollerr(void)
{
	__cmock_zsock_poll_Stub(poll_stub_pollerr);
	__cmock_mqtt_abort_ExpectAndReturn(&mqtt_client, 0);

	mqtt_state = MQTT_STATE_CONNECTED;

	k_sem_give(&connection_poll_sem);
	mqtt_helper_poll_loop();
}

void test_mqtt_helper_msg_id_get_returns_valid_ids(void)
{
	for (int i = 1; i == UINT16_MAX; i++) {
		TEST_ASSERT_EQUAL((i), mqtt_helper_msg_id_get());
	}

	TEST_ASSERT_EQUAL(1, mqtt_helper_msg_id_get());
}

int main(void)
{
	(void)unity_main();

	return 0;
}
