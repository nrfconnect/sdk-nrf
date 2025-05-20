/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sdfw/sdfw_services/ssf_client.h>
#include <sdfw/sdfw_services/ssf_client_notif.h>
#include <sdfw/sdfw_services/ssf_errno.h>

#include <zcbor_common.h>
#include <zephyr/toolchain.h>

#include <unity.h>
#include "cmock_ssf_client_transport.h"

#define CBOR_INF_ARRAY_START 0x9F
#define CBOR_INF_ARRAY_END 0xFF

static ssf_client_transport_notif_handler notification_handler;
static struct ssf_notification notif1_expected;
static int notif1_return_val;
static int notif1_context = 0x15;
static int notif2_context = 0x25;
static int notif3_context = 0x35;

static int stub_client_transport_init(ssf_client_transport_notif_handler notif_handler,
				      int cmock_num_calls)
{
	ARG_UNUSED(cmock_num_calls);

	notification_handler = notif_handler;
	return 0;
}

static int notif1_decode(const uint8_t *payload, size_t payload_len, void *result,
			 size_t *payload_len_out)
{
	ARG_UNUSED(payload_len_out);

	if (payload_len != 4) {
		return ZCBOR_ERR_NO_PAYLOAD;
	}

	memcpy(result, payload, payload_len);
	return 0;
}

static int notif1_handler(struct ssf_notification *notif, void *context)
{
	TEST_ASSERT_EQUAL_PTR(notif1_expected.data, notif->data);
	TEST_ASSERT_EQUAL(notif1_expected.data_len, notif->data_len);
	TEST_ASSERT_EQUAL_PTR(notif1_expected.pkt, notif->pkt);
	TEST_ASSERT_EQUAL_PTR(notif1_expected.notif_decode, notif->notif_decode);
	TEST_ASSERT_EQUAL(notif1_context, *(int *)context);
	return notif1_return_val;
}

static int notif_common_decode(const uint8_t *payload, size_t payload_len, void *result,
			       size_t *payload_len_out)
{
	ARG_UNUSED(payload);
	ARG_UNUSED(payload_len);
	ARG_UNUSED(result);
	ARG_UNUSED(payload_len_out);

	return ZCBOR_ERR_NO_PAYLOAD;
}

static int notif_common_handler(struct ssf_notification *notif, void *context)
{
	ARG_UNUSED(context);

	ssf_client_transport_decoding_done(notif->pkt);
	return -SSF_EFAULT;
}

#define CONFIG_SSF_SRVC1_SERVICE_ID 0x01
#define CONFIG_SSF_SRVC1_SERVICE_VERSION 1
SSF_CLIENT_NOTIF_LISTENER_DEFINE(notif1, SRVC1, notif1_decode, notif1_handler);

#define CONFIG_SSF_SRVC2_SERVICE_ID 0x04
#define CONFIG_SSF_SRVC2_SERVICE_VERSION 2
SSF_CLIENT_NOTIF_LISTENER_DEFINE(notif2, SRVC2, notif_common_decode, notif_common_handler);

#define CONFIG_SSF_SRVC3_SERVICE_ID 0x07
#define CONFIG_SSF_SRVC3_SERVICE_VERSION 3
SSF_CLIENT_NOTIF_LISTENER_DEFINE(notif3, SRVC3, notif_common_decode, notif_common_handler);

#define CONFIG_SSF_SRVC4_SERVICE_ID 0x0A
#define CONFIG_SSF_SRVC4_SERVICE_VERSION 4
SSF_CLIENT_NOTIF_LISTENER_DEFINE(notif4, SRVC4, notif_common_decode, notif_common_handler);

#define CONFIG_SSF_SRVC5_SERVICE_ID 0x0D
#define CONFIG_SSF_SRVC5_SERVICE_VERSION 5
SSF_CLIENT_NOTIF_LISTENER_DEFINE(notif5, SRVC5, NULL, notif_common_handler);

void setUp(void)
{
	int ssf_client_notif_init(void);

	int err = ssf_client_notif_init();

	TEST_ASSERT_EQUAL(err, 0);
}

void test_client_notif_register_unregister_success(void)
{
	int err;

	/* Check that listener 1 and 2 are unregistered */
	TEST_ASSERT_EQUAL(false, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif1.context);
	TEST_ASSERT_EQUAL(false, notif2.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif2.context);

	/* Register listener 1 and 2 */
	err = ssf_client_notif_register(&notif1, (void *)&notif1_context);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(true, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR((void *)&notif1_context, notif1.context);

	err = ssf_client_notif_register(&notif2, (void *)&notif2_context);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(true, notif2.is_registered);
	TEST_ASSERT_EQUAL_PTR((void *)&notif2_context, notif2.context);

	/* Deregister listener 1 and 2 */
	err = ssf_client_notif_deregister(&notif1);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(false, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif1.context);

	err = ssf_client_notif_deregister(&notif2);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(false, notif2.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif2.context);
}

void test_client_notif_register_max_listeners_registered(void)
{
	int err;

	/* Check that listener 1, 2 and 3 are unregistered */
	TEST_ASSERT_EQUAL(false, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif1.context);
	TEST_ASSERT_EQUAL(false, notif2.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif2.context);
	TEST_ASSERT_EQUAL(false, notif3.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif3.context);

	/* Register listener 1 and 2 */
	err = ssf_client_notif_register(&notif1, (void *)&notif1_context);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(true, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR((void *)&notif1_context, notif1.context);

	err = ssf_client_notif_register(&notif2, (void *)&notif2_context);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(true, notif2.is_registered);
	TEST_ASSERT_EQUAL_PTR((void *)&notif2_context, notif2.context);

	/* Registering listener 3 is expected to fail since SSF_CLIENT_REGISTERED_LISTENERS_MAX=2 */
	err = ssf_client_notif_register(&notif3, (void *)&notif3_context);
	TEST_ASSERT_EQUAL(-SSF_ENOBUFS, err);
	TEST_ASSERT_EQUAL(false, notif3.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif3.context);

	/* Deregister listener 1 and 2 */
	err = ssf_client_notif_deregister(&notif1);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(false, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif1.context);

	err = ssf_client_notif_deregister(&notif2);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(false, notif2.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif2.context);
}

void test_client_notif_register_params_invalid(void)
{
	int err;

	/* Unspecified listener */
	err = ssf_client_notif_register(NULL, NULL);
	TEST_ASSERT_EQUAL(-SSF_EINVAL, err);

	/* Listener with missing handler */
	notif4.handler = NULL;
	err = ssf_client_notif_register(&notif4, NULL);
	TEST_ASSERT_EQUAL(-SSF_EINVAL, err);
	notif4.handler = notif_common_handler;

	/* Listener with missing decoder */
	err = ssf_client_notif_register(&notif5, NULL);
	TEST_ASSERT_EQUAL(-SSF_EINVAL, err);
}

void test_client_notif_register_already_registered(void)
{
	int err;

	/* Check that listener 1 is unregistered */
	TEST_ASSERT_EQUAL(false, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif1.context);

	/* Register listener 1 */
	err = ssf_client_notif_register(&notif1, (void *)&notif1_context);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(true, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR((void *)&notif1_context, notif1.context);

	/* Attempt to register listener 1 again. Expect it to fail. */
	err = ssf_client_notif_register(&notif1, (void *)&notif1_context);
	TEST_ASSERT_EQUAL(-SSF_EALREADY, err);
	TEST_ASSERT_EQUAL(true, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR((void *)&notif1_context, notif1.context);

	/* Deregister listener 1 */
	err = ssf_client_notif_deregister(&notif1);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(false, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif1.context);
}

void test_client_notif_deregister_params_invalid(void)
{
	int err;

	/* Check that listener 1 is unregistered */
	TEST_ASSERT_EQUAL(false, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif1.context);

	/* Register listener 1 */
	err = ssf_client_notif_register(&notif1, (void *)&notif1_context);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(true, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR((void *)&notif1_context, notif1.context);

	/* Expect deregistering with NULL to fail */
	err = ssf_client_notif_deregister(NULL);
	TEST_ASSERT_EQUAL(-SSF_EINVAL, err);
	TEST_ASSERT_EQUAL(true, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR((void *)&notif1_context, notif1.context);

	/* Deregister listener 1 */
	err = ssf_client_notif_deregister(&notif1);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(false, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif1.context);
}

void test_client_notif_deregister_not_already_registered(void)
{
	int err;

	/* Check that listener 1 is unregistered */
	TEST_ASSERT_EQUAL(false, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif1.context);

	/* Register listener 1 */
	err = ssf_client_notif_register(&notif1, (void *)&notif1_context);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(true, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR((void *)&notif1_context, notif1.context);

	/* Attempt to deregister listener 2. Expect it to fail. */
	err = ssf_client_notif_deregister(&notif2);
	TEST_ASSERT_EQUAL(-SSF_ENXIO, err);
	TEST_ASSERT_EQUAL(false, notif2.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif2.context);
	TEST_ASSERT_EQUAL(true, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR((void *)&notif1_context, notif1.context);

	/* Deregister listener 1 */
	err = ssf_client_notif_deregister(&notif1);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(false, notif1.is_registered);
	TEST_ASSERT_EQUAL_PTR(NULL, notif1.context);
}

void test_client_notif_decode_success(void)
{
	int err;

	const uint8_t data[] = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA };
	const struct ssf_notification notif = { .data = &data[2],
						.data_len = sizeof(data) - 2,
						.pkt = data,
						.notif_decode = notif1.notif_decode };
	int decoded_notif;
	const int decoded_notif_expected = 0xAABBCCDD;

	err = ssf_client_notif_decode(&notif, &decoded_notif);
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(decoded_notif_expected, decoded_notif);
}

void test_client_notif_decode_params_invalid(void)
{
	int err;

	const uint8_t data[] = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA };
	const struct ssf_notification notif = { .data = &data[2],
						.data_len = sizeof(data) - 2,
						.pkt = data,
						.notif_decode = notif1.notif_decode };
	int decoded_notif;

	err = ssf_client_notif_decode(NULL, &decoded_notif);
	TEST_ASSERT_EQUAL(-SSF_EINVAL, err);

	err = ssf_client_notif_decode(&notif, NULL);
	TEST_ASSERT_EQUAL(-SSF_EINVAL, err);
}

void test_client_notif_decode_func_error(void)
{
	int err;

	const uint8_t data[] = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA };
	const struct ssf_notification notif = { .data = &data[2],
						.data_len = sizeof(data) - 2,
						.pkt = data,
						.notif_decode = notif2.notif_decode };
	int decoded_notif;

	err = ssf_client_notif_decode(&notif, &decoded_notif);
	TEST_ASSERT_EQUAL(-SSF_EPROTO, err);
}

void test_client_notif_decode_done_success(void)
{
	const uint8_t data[] = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA };
	struct ssf_notification notif = { .data = &data[2],
					  .data_len = sizeof(data) - 2,
					  .pkt = data,
					  .notif_decode = notif1.notif_decode };

	__cmock_ssf_client_transport_decoding_done_Expect(data);

	ssf_client_notif_decode_done(&notif);
}

void test_client_notif_decode_done_param_invalid(void)
{
	struct ssf_notification notif = {
		.data = NULL, .data_len = 0, .pkt = NULL, .notif_decode = notif1.notif_decode
	};

	ssf_client_notif_decode_done(NULL);

	ssf_client_notif_decode_done(&notif);
}

void test_client_notif_receive_success(void)
{
	int err;

	/* Get "notif receive" handler to be able to invoke it later. */
	__cmock_ssf_client_transport_init_Stub(stub_client_transport_init);
	ssf_client_init();
	TEST_ASSERT_NOT_NULL(notification_handler);

	/* Register listener 1 */
	err = ssf_client_notif_register(&notif1, (void *)&notif1_context);
	TEST_ASSERT_EQUAL(0, err);

	/* Define incoming package */
	const uint8_t shmem_rx[] = { CBOR_INF_ARRAY_START,
				     CONFIG_SSF_SRVC1_SERVICE_ID,
				     CONFIG_SSF_SRVC1_SERVICE_VERSION,
				     CBOR_INF_ARRAY_END,
				     0xCE,
				     0xCA };
	size_t hdr_len = 4;

	notif1_expected.data = &shmem_rx[hdr_len];
	notif1_expected.data_len = sizeof(shmem_rx) - hdr_len;
	notif1_expected.pkt = shmem_rx;
	notif1_expected.notif_decode = notif1.notif_decode;
	notif1_return_val = 0;

	/* Function under test */
	notification_handler(shmem_rx, sizeof(shmem_rx));

	/* Deregister listener 1 */
	err = ssf_client_notif_deregister(&notif1);
	TEST_ASSERT_EQUAL(0, err);
}

void test_client_notif_receive_decode_hdr_error(void)
{
	int err;

	/* Get "notif receive" handler to be able to invoke it later. */
	__cmock_ssf_client_transport_init_Stub(stub_client_transport_init);
	ssf_client_init();
	TEST_ASSERT_NOT_NULL(notification_handler);

	/* Register listener 1 */
	err = ssf_client_notif_register(&notif1, (void *)&notif1_context);
	TEST_ASSERT_EQUAL(0, err);

	/* Define incoming package */
	const uint8_t shmem_rx[] = { CBOR_INF_ARRAY_END,
				     CONFIG_SSF_SRVC1_SERVICE_ID,
				     CONFIG_SSF_SRVC1_SERVICE_VERSION,
				     CBOR_INF_ARRAY_END,
				     0xCE,
				     0xCA };
	notif1_expected.data = NULL;
	notif1_expected.data_len = 0;
	notif1_expected.pkt = NULL;
	notif1_expected.notif_decode = NULL;
	notif1_return_val = -SSF_EFAULT;

	__cmock_ssf_client_transport_decoding_done_Expect(shmem_rx);

	/* Function under test */
	notification_handler(shmem_rx, sizeof(shmem_rx));

	/* Deregister listener 1 */
	err = ssf_client_notif_deregister(&notif1);
	TEST_ASSERT_EQUAL(0, err);
}

void test_client_notif_receive_listener_not_registered(void)
{
	int err;

	/* Get "notif receive" handler to be able to invoke it later. */
	__cmock_ssf_client_transport_init_Stub(stub_client_transport_init);
	ssf_client_init();
	TEST_ASSERT_NOT_NULL(notification_handler);

	/* Define incoming package */
	const uint8_t shmem_rx[] = { CBOR_INF_ARRAY_START,
				     CONFIG_SSF_SRVC1_SERVICE_ID,
				     CONFIG_SSF_SRVC1_SERVICE_VERSION,
				     CBOR_INF_ARRAY_END,
				     0xCE,
				     0xCA };
	notif1_expected.data = NULL;
	notif1_expected.data_len = 0;
	notif1_expected.pkt = NULL;
	notif1_expected.notif_decode = NULL;
	notif1_return_val = -SSF_EFAULT;

	__cmock_ssf_client_transport_decoding_done_Expect(shmem_rx);

	/*
	 * Function under test. Expect listener to not be found and handler not be invoked.
	 * Expect call to decoding_done.
	 */
	notification_handler(shmem_rx, sizeof(shmem_rx));

	/* Register listener 2. Expect this listener to not receive the notification. */
	err = ssf_client_notif_register(&notif2, (void *)&notif2_context);
	TEST_ASSERT_EQUAL(0, err);

	__cmock_ssf_client_transport_decoding_done_Expect(shmem_rx);

	/*
	 * Function under test. With another listener registered, expect listener
	 * to not be found and handler not be invoked. Expect call to decoding_done.
	 */
	notification_handler(shmem_rx, sizeof(shmem_rx));

	/* Deregister listener 2 */
	err = ssf_client_notif_deregister(&notif2);
	TEST_ASSERT_EQUAL(0, err);
}

void test_client_notif_receive_handler_missing(void)
{
	int err;

	/* Get "notif receive" handler to be able to invoke it later. */
	__cmock_ssf_client_transport_init_Stub(stub_client_transport_init);
	ssf_client_init();
	TEST_ASSERT_NOT_NULL(notification_handler);

	/* Register listener 4. Remove handler after successfully registering the listener. */
	err = ssf_client_notif_register(&notif4, NULL);
	TEST_ASSERT_EQUAL(0, err);
	notif4.handler = NULL;

	/* Define incoming package */
	const uint8_t shmem_rx[] = { CBOR_INF_ARRAY_START,
				     CONFIG_SSF_SRVC4_SERVICE_ID,
				     CONFIG_SSF_SRVC4_SERVICE_VERSION,
				     CBOR_INF_ARRAY_END,
				     0xCE,
				     0xCA };

	__cmock_ssf_client_transport_decoding_done_Expect(shmem_rx);

	/*
	 * Function under test. Expect it to fail because handler is missing
	 * for listener 4. Expect call to decoding_done.
	 */
	notification_handler(shmem_rx, sizeof(shmem_rx));

	/* Restore handler, then deregister listener 4 */
	notif4.handler = notif_common_handler;
	err = ssf_client_notif_deregister(&notif4);
	TEST_ASSERT_EQUAL(0, err);
}

void test_client_notif_receive_version_mismatch(void)
{
	int err;

	/* Get "notif receive" handler to be able to invoke it later. */
	__cmock_ssf_client_transport_init_Stub(stub_client_transport_init);
	ssf_client_init();
	TEST_ASSERT_NOT_NULL(notification_handler);

	/* Define incoming package */
	const uint8_t shmem_rx[] = { CBOR_INF_ARRAY_START,
				     CONFIG_SSF_SRVC1_SERVICE_ID,
				     CONFIG_SSF_SRVC1_SERVICE_VERSION + 1,
				     CBOR_INF_ARRAY_END,
				     0xCE,
				     0xCA };
	notif1_expected.data = NULL;
	notif1_expected.data_len = 0;
	notif1_expected.pkt = NULL;
	notif1_expected.notif_decode = NULL;
	notif1_return_val = -SSF_EFAULT;

	/* Register listener 1 */
	err = ssf_client_notif_register(&notif1, (void *)&notif1_context);
	TEST_ASSERT_EQUAL(0, err);

	__cmock_ssf_client_transport_decoding_done_Expect(shmem_rx);

	/*
	 * Function under test. Expect version number in incoming packet to
	 * not be equal to the version number for the listener.
	 */
	notification_handler(shmem_rx, sizeof(shmem_rx));

	/* Deregister listener 1 */
	err = ssf_client_notif_deregister(&notif1);
	TEST_ASSERT_EQUAL(0, err);
}

void test_client_notif_receive_listener_handler_error(void)
{
	int err;

	/* Get "notif receive" handler to be able to invoke it later. */
	__cmock_ssf_client_transport_init_Stub(stub_client_transport_init);
	ssf_client_init();
	TEST_ASSERT_NOT_NULL(notification_handler);

	/* Define incoming package */
	const uint8_t shmem_rx[] = { CBOR_INF_ARRAY_START,
				     CONFIG_SSF_SRVC3_SERVICE_ID,
				     CONFIG_SSF_SRVC3_SERVICE_VERSION,
				     CBOR_INF_ARRAY_END,
				     0xCE,
				     0xCA };

	/* Register listener 3 */
	err = ssf_client_notif_register(&notif3, NULL);
	TEST_ASSERT_EQUAL(0, err);

	__cmock_ssf_client_transport_decoding_done_Expect(shmem_rx);

	/*
	 * Function under test. Expect version number in incoming packet to
	 * not be equal to the version number for the listener.
	 */
	notification_handler(shmem_rx, sizeof(shmem_rx));

	/* Deregister listener 3 */
	err = ssf_client_notif_deregister(&notif3);
	TEST_ASSERT_EQUAL(0, err);
}

extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}
