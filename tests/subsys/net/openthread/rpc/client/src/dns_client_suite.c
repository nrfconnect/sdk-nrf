/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "mocks.h"

#include <mock_nrf_rpc_transport.h>
#include <nrf_rpc/nrf_rpc_cbkproxy.h>
#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/dns_client.h>

static struct net_in6_addr test_addr = {{{ DNS_IPV6_ADDR }}};

FAKE_VOID_FUNC(ot_dns_resolve_cb, otError, const otDnsAddressResponse *, void *);
FAKE_VOID_FUNC(ot_dns_browse_cb, otError, const otDnsBrowseResponse *, void *);
FAKE_VOID_FUNC(ot_dns_service_cb, otError, const otDnsServiceResponse *, void *);

static int resolve_cb_slot;
static int browse_cb_slot;
static int service_cb_slot;

static char instance_buffer[64];
static char name_buffer[128];
static uint8_t txt_data[128];

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void *tc_setup(void)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();

	RESET_FAKE(ot_dns_resolve_cb);
	RESET_FAKE(ot_dns_browse_cb);
	RESET_FAKE(ot_dns_service_cb);

	resolve_cb_slot = nrf_rpc_cbkproxy_in_set(ot_dns_resolve_cb);
	browse_cb_slot = nrf_rpc_cbkproxy_in_set(ot_dns_browse_cb);
	service_cb_slot = nrf_rpc_cbkproxy_in_set(ot_dns_service_cb);

	return NULL;
}

static void tc_cleanup(void *f)
{
	mock_nrf_rpc_tr_expect_done();

	memset(instance_buffer, 0, sizeof(instance_buffer));
	memset(name_buffer, 0, sizeof(name_buffer));
	memset(txt_data, 0, sizeof(txt_data));

	RESET_FAKE(ot_dns_resolve_cb);
	RESET_FAKE(ot_dns_browse_cb);
	RESET_FAKE(ot_dns_service_cb);
}

static void set_default_config(otDnsQueryConfig *config)
{
	memcpy((struct net_in6_addr *)&config->mServerSockAddr.mAddress, &test_addr,
	       OT_IP6_ADDRESS_SIZE);

	config->mServerSockAddr.mPort = DNS_PORT;
	config->mResponseTimeout = DNS_TIMEOUT;
	config->mMaxTxAttempts = DNS_ATTEMPTS;
	config->mRecursionFlag = OT_DNS_FLAG_RECURSION_DESIRED;
	config->mNat64Mode = OT_DNS_NAT64_DISALLOW;
	config->mServiceMode = OT_DNS_SERVICE_MODE_SRV_TXT_OPTIMIZE;
	config->mTransportProto = OT_DNS_TRANSPORT_UDP;
}

/**
 * Verify that otDnsClientGetDefaultConfig() sends RPC command and properly decodes the response.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_GET_DEFAULT_CONFIG)
 *
 * Responses:
 *   1. RPC_RSP:
 *        ipv6 address(2001:db8::1), port(53), timeout(1000), attempts(5), recursion flag(1),
 *        nat64 flag(2), service mode flag(5), transport flag(1)
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientGetDefaultConfig)
{
	const otDnsQueryConfig *config;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_GET_DEFAULT_CONFIG),
				   RPC_RSP(CBOR_DNS_QUERY_CONFIG));

	config = otDnsClientGetDefaultConfig(NULL);

	zassert_mem_equal(config->mServerSockAddr.mAddress.mFields.m8, &test_addr,
			  OT_IP6_ADDRESS_SIZE);
	zassert_equal(config->mServerSockAddr.mPort, DNS_PORT);
	zassert_equal(config->mResponseTimeout, DNS_TIMEOUT);
	zassert_equal(config->mMaxTxAttempts, DNS_ATTEMPTS);
	zassert_equal(config->mRecursionFlag, OT_DNS_FLAG_RECURSION_DESIRED);
	zassert_equal(config->mNat64Mode, OT_DNS_NAT64_DISALLOW);
	zassert_equal(config->mServiceMode, OT_DNS_SERVICE_MODE_SRV_TXT_OPTIMIZE);
	zassert_equal(config->mTransportProto, OT_DNS_TRANSPORT_UDP);
}

/**
 * Verify that otDnsClientSetDefaultConfig() sends properly encoded RPC command.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_SET_DEFAULT_CONFIG):
 *        ipv6 address(2001:db8::1), port(53), timeout(1000), attempts(5), recursion flag(1),
 *        nat64 flag(2), service mode flag(5), transport flag(1)
 *
 * Responses:
 *   1. RPC_RSP: void
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientSetDefaultConfig)
{
	otDnsQueryConfig config;

	set_default_config(&config);

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_SET_DEFAULT_CONFIG,
					   CBOR_DNS_QUERY_CONFIG),
				   RPC_RSP());

	otDnsClientSetDefaultConfig(NULL, &config);

	mock_nrf_rpc_tr_expect_done();
}

/**
 * Verify that otDnsClientResolveAddress() sends properly encoded RPC command.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_RESOLVE_ADDRESS):
 *        hostname("example.com"), callback slot (assigned at init), callback context(0xFFFFFFFF),
 *        ipv6 address(2001:db8::1), port(53), timeout(1000), attempts(5), recursion flag(1),
 *        nat64 flag(2), service mode flag(5), transport flag(1)
 *
 * Responses:
 *   1. RPC_RSP: OT_ERROR_NONE
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientResolveAddress)
{
	otError error;
	otDnsQueryConfig config;

	set_default_config(&config);

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_DNS_CLIENT_RESOLVE_ADDRESS, CBOR_DNS_HOSTNAME, resolve_cb_slot,
			CBOR_UINT32(UINT32_MAX), CBOR_DNS_QUERY_CONFIG),
			RPC_RSP(OT_ERROR_NONE));

	error = otDnsClientResolveAddress(NULL, DNS_HOSTNAME, ot_dns_resolve_cb, (void *)UINT32_MAX,
					  &config);

	zassert_equal(error, OT_ERROR_NONE);
}

/**
 * Verify that otDnsClientResolveIp4Address() sends properly encoded RPC command.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_RESOLVE_IP4_ADDRESS):
 *        hostname("example.com"), callback slot (assigned at init), callback context(0xFFFFFFFF),
 *        ipv6 address(2001:db8::1), port(53), timeout(1000), attempts(5), recursion flag(1),
 *        nat64 flag(2), service mode flag(5), transport flag(1)
 *
 * Responses:
 *   1. RPC_RSP: OT_ERROR_NONE
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientResolveIp4Address)
{
	otError error;
	otDnsQueryConfig config;

	set_default_config(&config);

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_RESOLVE_IP4_ADDRESS,
				   CBOR_DNS_HOSTNAME, resolve_cb_slot, CBOR_UINT32(UINT32_MAX),
				   CBOR_DNS_QUERY_CONFIG), RPC_RSP(OT_ERROR_NONE));

	error = otDnsClientResolveIp4Address(NULL, DNS_HOSTNAME, ot_dns_resolve_cb,
					     (void *)UINT32_MAX, &config);

	zassert_equal(error, OT_ERROR_NONE);
}

static void verify_resolve_callback_args(otError error, const otDnsAddressResponse *response,
					 void *context)
{
	zassert_true(ot_rpc_is_mutex_locked());
	zassert_equal(error, 0);
	zassert_equal(response, (const otDnsAddressResponse *)0xFACEFACE);
	zassert_equal(context, (void *)UINT32_MAX);
}

/**
 * Verify that address response callback's arguments are properly decoded.
 *
 * Expected packets:
 *   1. RPC_RSP: void
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_ADDRESS_RESPONSE_CB):
 *        OT_ERROR_NONE, response pointer(0xFACEFACE), response context(0xFFFFFFFF),
 *        callback slot (assigned at init)
 */
ZTEST(ot_rpc_dns_client, test_otDnsAddressCallback)
{
	ot_dns_resolve_cb_fake.custom_fake = verify_resolve_callback_args;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_ADDRESS_RESPONSE_CB, OT_ERROR_NONE,
				CBOR_UINT32(0xFACEFACE), CBOR_UINT32(UINT32_MAX), resolve_cb_slot));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_dns_resolve_cb_fake.call_count, 1);
}

static void verify_address_response_hostname(otError error, const otDnsAddressResponse *response,
					     void *context)
{
	otError api_err;

	verify_resolve_callback_args(error, response, context);

	api_err = otDnsAddressResponseGetHostName(response, name_buffer, sizeof(name_buffer));

	zassert_equal(api_err, OT_ERROR_NONE);
	zassert_str_equal(name_buffer, DNS_HOSTNAME);
}

/**
 * Verify that hostname can be properly fetched from address response's callback.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_ADDRESS_RESP_GET_HOST_NAME):
 *        response pointer(0xFACEFACE), maximum length(128)
 *   2. RPC_RSP: void
 *
 * Responses:
 *   1. RPC_RSP: OT_ERROR_NONE, hostname("example.com")
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_ADDRESS_RESPONSE_CB):
 *        OT_ERROR_NONE, response pointer(0xFACEFACE), context(0xFFFFFFFF),
 *        callback slot(assigned at init)
 */
ZTEST(ot_rpc_dns_client, test_otDnsAddressResponseGetHostName)
{
	ot_dns_resolve_cb_fake.custom_fake = verify_address_response_hostname;

	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_ADDRESS_RESP_GET_HOST_NAME,
					      CBOR_UINT32(0xFACEFACE),
					      CBOR_UINT8(sizeof(name_buffer))),
				   RPC_RSP(OT_ERROR_NONE, CBOR_DNS_HOSTNAME));
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_ADDRESS_RESPONSE_CB, OT_ERROR_NONE,
				CBOR_UINT32(0xFACEFACE), CBOR_UINT32(UINT32_MAX), resolve_cb_slot));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_dns_resolve_cb_fake.call_count, 1);
}

static void verify_address_response_address(otError error, const otDnsAddressResponse *response,
					    void *context)
{
	otError api_err;
	uint32_t ttl;
	otIp6Address address;

	verify_resolve_callback_args(error, response, context);

	api_err = otDnsAddressResponseGetAddress(response, 0, &address, &ttl);

	zassert_equal(api_err, OT_ERROR_NONE);
	zassert_mem_equal(&address, &test_addr, sizeof(test_addr));
	zassert_equal(ttl, 120);

	api_err = otDnsAddressResponseGetAddress(response, 1, &address, &ttl);

	zassert_equal(api_err, OT_ERROR_NOT_FOUND);
}

/**
 * Verify that host address can be properly fetched from address response's callback. Also, verify
 * that error is properly returned for non-existing address.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_ADDRESS_RESP_GET_ADDRESS):
 *        response pointer(0xFACEFACE), index(0)
 *   2. RPC_CMD (OT_RPC_CMD_DNS_ADDRESS_RESP_GET_ADDRESS):
 *        response pointer(0xFACEFACE), index(1)
 *
 * Responses:
 *   1. RPC_RSP: OT_ERROR_NONE, ipv6 address(2001:db8::1), ttl(120)
 *   2. RPC_RSP: OT_ERROR_NOT_FOUND
 *   3. RPC_RSP: void
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_ADDRESS_RESPONSE_CB):
 *        OT_ERROR_NONE, response pointer(0xFACEFACE), context(0xFFFFFFFF),
 *        callback slot(assigned at init)
 */
ZTEST(ot_rpc_dns_client, test_otDnsAddressResponseGetAddress)
{
	ot_dns_resolve_cb_fake.custom_fake = verify_address_response_address;

	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_ADDRESS_RESP_GET_ADDRESS,
					      CBOR_UINT32(0xFACEFACE), /* index */ 0),
				   RPC_RSP(OT_ERROR_NONE, CBOR_IPV6_ADDR,
					   /* TTL */ CBOR_UINT8(120)));
	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_ADDRESS_RESP_GET_ADDRESS,
					      CBOR_UINT32(0xFACEFACE), /* index */ 1),
				   RPC_RSP(OT_ERROR_NOT_FOUND));
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_ADDRESS_RESPONSE_CB, OT_ERROR_NONE,
				CBOR_UINT32(0xFACEFACE), CBOR_UINT32(UINT32_MAX), resolve_cb_slot));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_dns_resolve_cb_fake.call_count, 1);
}

/**
 * Verify that calling otDnsClientBrowse() sends properly encoded RPC command.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_BROWSE):
 *        service name("_test._udp.example.com"), callback slot(assigned at init),
 *        callback context(0xFFFFFFFF), ipv6 address(2001:db8::1), port(53), timeout(1000),
 *        attempts(5), recursion flag(1), nat64 flag(2), service mode flag(5), transport flag(1)
 *
 * Responses:
 *   1. RPC_RSP: OT_ERROR_NONE
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientBrowse)
{
	otError error;
	otDnsQueryConfig config;

	set_default_config(&config);

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_BROWSE, CBOR_DNS_SERVICE_NAME,
					   browse_cb_slot, CBOR_UINT32(UINT32_MAX),
					   CBOR_DNS_QUERY_CONFIG),
				   RPC_RSP(OT_ERROR_NONE));

	error = otDnsClientBrowse(NULL, DNS_SERVICE_NAME, ot_dns_browse_cb, (void *)UINT32_MAX,
				  &config);

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
}

static void verify_browse_callback_args(otError error, const otDnsBrowseResponse *response,
					void *context)
{
	zassert_true(ot_rpc_is_mutex_locked());
	zassert_equal(error, 0);
	zassert_equal(response, (const otDnsBrowseResponse *)0xFACEFACE);
	zassert_equal(context, (void *)UINT32_MAX);
}

/**
 * Verify that browse response callback's arguments are properly decoded.
 *
 * Expected packets:
 *   1. RPC_RSP: void
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB):
 *        OT_ERROR_NONE, response pointer(0xFACEFACE), response context(0xFFFFFFFF),
 *        callback slot (assigned at init)
 */
ZTEST(ot_rpc_dns_client, test_otDnsBrowseCallback)
{
	ot_dns_browse_cb_fake.custom_fake = verify_browse_callback_args;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB, OT_ERROR_NONE,
				CBOR_UINT32(0xFACEFACE), CBOR_UINT32(UINT32_MAX), browse_cb_slot));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_dns_browse_cb_fake.call_count, 1);
}

static void verify_browse_response_service_name(otError error, const otDnsBrowseResponse *response,
						void *context)
{
	otError api_err;

	verify_browse_callback_args(error, response, context);

	api_err = otDnsBrowseResponseGetServiceName(response, name_buffer, sizeof(name_buffer));

	zassert_equal(api_err, OT_ERROR_NONE);
	zassert_str_equal(name_buffer, DNS_SERVICE_NAME);
}

/**
 * Verify that service name can be fetched from browse response's callback.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_NAME):
 *        response pointer(0xFACEFACE), maximum length(128)
 *
 * Responses:
 *   1. RPC_RSP: OT_ERROR_NONE, service name("_test._udp.example.com")
 *   2. RPC_RSP: void
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB):
 *        OT_ERROR_NONE, response pointer(0xFACEFACE), response context(0xFFFFFFFF),
 *        callback slot(assigned at init)
 */
ZTEST(ot_rpc_dns_client, test_otDnsBrowseResponseGetServiceName)
{
	ot_dns_browse_cb_fake.custom_fake = verify_browse_response_service_name;

	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_NAME,
					      CBOR_UINT32(0xFACEFACE),
					      CBOR_UINT8(sizeof(name_buffer))),
				   RPC_RSP(OT_ERROR_NONE, CBOR_DNS_SERVICE_NAME));
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB, OT_ERROR_NONE,
				CBOR_UINT32(0xFACEFACE), CBOR_UINT32(UINT32_MAX), browse_cb_slot));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_dns_browse_cb_fake.call_count, 1);
}

static void verify_browse_response_service_instance(otError error,
						    const otDnsBrowseResponse *response,
						    void *context)
{
	otError api_err;

	verify_browse_callback_args(error, response, context);

	api_err = otDnsBrowseResponseGetServiceInstance(response, 0, name_buffer,
							sizeof(name_buffer));

	zassert_equal(api_err, OT_ERROR_NONE);
	zassert_str_equal(name_buffer, DNS_SERVICE_INSTANCE);

	api_err = otDnsBrowseResponseGetServiceInstance(response, 1, name_buffer,
							sizeof(name_buffer));

	zassert_equal(api_err, OT_ERROR_NOT_FOUND);
}

/**
 * Verify that service instance can be fetched from browse response's callback.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INSTANCE):
 *        response pointer(0xFACEFACE), index(0), maximum length(128)
 *   2. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INSTANCE):
 *        response pointer(0xFACEFACE), index(1), maximum length(128)
 *
 * Responses:
 *   1. RPC_RSP: OT_ERROR_NONE, service instance("Test")
 *   2. RPC_RSP: OT_ERROR_NOT_FOUND
 *   3. RPC_RSP: void
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB):
 *        OT_ERROR_NONE, response pointer(0xFACEFACE), response context(0xFFFFFFFF),
 *        callback slot(assigned at init)
 */
ZTEST(ot_rpc_dns_client, test_otDnsBrowseResponseGetServiceInstance)
{
	ot_dns_browse_cb_fake.custom_fake = verify_browse_response_service_instance;

	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INSTANCE,
					      CBOR_UINT32(0xFACEFACE), /* index */ 0,
					      CBOR_UINT8(sizeof(name_buffer))),
				   RPC_RSP(OT_ERROR_NONE, CBOR_SERVICE_INSTANCE));
	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INSTANCE,
					      CBOR_UINT32(0xFACEFACE), /* index */ 1,
					      CBOR_UINT8(sizeof(name_buffer))),
				   RPC_RSP(OT_ERROR_NOT_FOUND));
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB, OT_ERROR_NONE,
				CBOR_UINT32(0xFACEFACE), CBOR_UINT32(UINT32_MAX), browse_cb_slot));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_dns_browse_cb_fake.call_count, 1);
}

static void validate_service_info(const otDnsServiceInfo *info)
{
	zassert_equal(info->mTtl, 120);
	zassert_equal(info->mPort, 12345);
	zassert_equal(info->mPriority, 10);
	zassert_equal(info->mWeight, 12);

	zassert_str_equal(info->mHostNameBuffer, DNS_HOSTNAME);
	zassert_str_equal(info->mTxtData, "Service TXT DATA");

	zassert_mem_equal(&info->mHostAddress, &test_addr, sizeof(test_addr));

	zassert_equal(info->mHostAddressTtl, 120);
	zassert_equal(info->mTxtDataSize, strlen(info->mTxtData));

	zassert_equal(info->mTxtDataTruncated, false);
	zassert_equal(info->mTxtDataTtl, 120);
}

static void verify_browse_response_service_info(otError error, const otDnsBrowseResponse *response,
						void *context)
{
	otError api_err;
	otDnsServiceInfo info;

	verify_browse_callback_args(error, response, context);

	info.mHostNameBuffer = name_buffer;
	info.mHostNameBufferSize = sizeof(name_buffer);
	info.mTxtData = txt_data;
	info.mTxtDataSize = sizeof(txt_data);

	api_err = otDnsBrowseResponseGetServiceInfo(response, DNS_SERVICE_INSTANCE, &info);

	zassert_equal(api_err, OT_ERROR_NONE);

	validate_service_info(&info);
}

/**
 * Verify that service info can be fetched from browse response's callback.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO):
 *        response pointer(0xFACEFACE), max name length(128), max txt size(128), instance("Test")
 *
 * Responses:
 *   1. RPC_RSP:
 *        OT_ERROR_NONE, ttl(120), port(12345), priority(10), weight(20), hostname("example.com"),
 *        ipv6 address(2001:db8::1), address TTL(120), TXT data (16 bytes) truncated flag(false),
 *        TXT data TTL(60)
 *   2. RPC_RSP: void
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB):
 *        OT_ERROR_NONE, response pointer(0xFACEFACE), response context(0xFFFFFFFF),
 *        callback slot(assigned at init)
 */
ZTEST(ot_rpc_dns_client, test_otDnsBrowseResponseGetServiceInfo)
{
	ot_dns_browse_cb_fake.custom_fake = verify_browse_response_service_info;

	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO,
					      CBOR_UINT32(0xFACEFACE),
					      CBOR_UINT8(sizeof(name_buffer)),
					      CBOR_UINT8(sizeof(txt_data)),
					      CBOR_SERVICE_INSTANCE),
				   RPC_RSP(OT_ERROR_NONE, CBOR_SERVICE_INFO));
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB, OT_ERROR_NONE,
				CBOR_UINT32(0xFACEFACE), CBOR_UINT32(UINT32_MAX), browse_cb_slot));

	zassert_equal(ot_dns_browse_cb_fake.call_count, 1);
}

static void verify_browse_response_host_address(otError error, const otDnsBrowseResponse *response,
						void *context)
{
	otError api_err;
	otIp6Address address;
	uint32_t ttl;

	verify_browse_callback_args(error, response, context);

	api_err = otDnsBrowseResponseGetHostAddress(response, DNS_HOSTNAME, 0, &address,
						    &ttl);

	zassert_equal(api_err, OT_ERROR_NONE);
	zassert_mem_equal(&address, &test_addr, sizeof(test_addr));
	zassert_equal(ttl, 120);

	api_err = otDnsBrowseResponseGetHostAddress(response, DNS_HOSTNAME, 1, &address,
						    &ttl);

	zassert_equal(api_err, OT_ERROR_NOT_FOUND);
}

/**
 * Verify that host address can be properly fetched from address response's callback. Also, verify
 * that error is properly returned for non-existing address.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_HOST_ADDRESS):
 *        response pointer(0xFACEFACE), index(0), hostname("example.com")
 *   2. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_HOST_ADDRESS):
 *        response pointer(0xFACEFACE), index(1), hostname("example.com")
 *
 * Responses:
 *   1. RPC_RSP: OT_ERROR_NONE, ipv6 address(2001:db8::1), ttl(120)
 *   2. RPC_RSP: OT_ERROR_NOT_FOUND
 *   3. RPC_RSP: void
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB):
 *        OT_ERROR_NONE, response pointer(0xFACEFACE), context(0xFFFFFFFF),
 *        callback slot(assigned at init)
 */
ZTEST(ot_rpc_dns_client, test_otDnsBrowseResponseGetHostAddress)
{
	ot_dns_browse_cb_fake.custom_fake = verify_browse_response_host_address;

	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_HOST_ADDRESS,
					      CBOR_UINT32(0xFACEFACE), /* index */ 0,
					      CBOR_DNS_HOSTNAME),
				   RPC_RSP(OT_ERROR_NONE, CBOR_IPV6_ADDR,
					   /* TTL */ CBOR_UINT8(120)));
	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_HOST_ADDRESS,
					      CBOR_UINT32(0xFACEFACE), /* index */ 1,
					      CBOR_DNS_HOSTNAME),
				   RPC_RSP(OT_ERROR_NOT_FOUND));
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB, OT_ERROR_NONE,
				CBOR_UINT32(0xFACEFACE), CBOR_UINT32(UINT32_MAX), browse_cb_slot));

	mock_nrf_rpc_tr_expect_done();
}

/**
 * Verify that calling otDnsClientResolveService() sends properly encoded RPC command.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE):
 *        service instance("Test"), service name("_test._udp.example.com"),
 *        callback slot(assigned at init), callback context(0xFFFFFFFF), ipv6 address(2001:db8::1),
 *        port(53), timeout(1000), attempts(5), recursion flag(1), nat64 flag(2),
 *        service mode flag(5), transport flag(1)
 *
 * Responses:
 *   1. RPC_RSP: OT_ERROR_NONE
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientResolveService)
{
	otError error;
	otDnsQueryConfig config;

	set_default_config(&config);

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE,
					   CBOR_SERVICE_INSTANCE, CBOR_DNS_SERVICE_NAME,
					   service_cb_slot, CBOR_UINT32(UINT32_MAX),
					   CBOR_DNS_QUERY_CONFIG),
				   RPC_RSP(OT_ERROR_NONE));

	error = otDnsClientResolveService(NULL, DNS_SERVICE_INSTANCE, DNS_SERVICE_NAME,
					  ot_dns_service_cb, (void *)UINT32_MAX, &config);

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
}

/**
 * Verify that calling test_otDnsClientResolveServiceAndHostAddress() sends properly encoded RPC
 * command.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE_AND_HOST_ADDRESS):
 *        service instance("Test"), service name("_test._udp.example.com"),
 *        callback slot(assigned at init), callback context(0xFFFFFFFF), ipv6 address(2001:db8::1),
 *        port(53), timeout(1000), attempts(5), recursion flag(1), nat64 flag(2),
 *        service mode flag(5), transport flag(1)
 *
 * Responses:
 *   1. RPC_RSP: OT_ERROR_NONE
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientResolveServiceAndHostAddress)
{
	otError error;
	otDnsQueryConfig config;

	set_default_config(&config);

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE_AND_HOST_ADDRESS,
					   CBOR_SERVICE_INSTANCE, CBOR_DNS_SERVICE_NAME,
					   service_cb_slot, CBOR_UINT32(UINT32_MAX),
					   CBOR_DNS_QUERY_CONFIG),
				   RPC_RSP(OT_ERROR_NONE));

	error = otDnsClientResolveServiceAndHostAddress(NULL, DNS_SERVICE_INSTANCE,
							DNS_SERVICE_NAME, ot_dns_service_cb,
							(void *)UINT32_MAX, &config);

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
}

static void verify_service_callback_args(otError error, const otDnsServiceResponse *response,
					 void *context)
{
	zassert_true(ot_rpc_is_mutex_locked());
	zassert_equal(error, 0);
	zassert_equal(response, (const otDnsServiceResponse *)0xFACEFACE);
	zassert_equal(context, (void *)UINT32_MAX);
}

/**
 * Verify that service response callback's arguments are properly decoded.
 *
 * Expected packets:
 *   1. RPC_RSP: void
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_SERVICE_RESPONSE_CB):
 *        OT_ERROR_NONE, response pointer(0xFACEFACE), response context(0xFFFFFFFF),
 *        callback slot (assigned at init)
 */
ZTEST(ot_rpc_dns_client, test_otDnsServiceCallback)
{
	ot_dns_service_cb_fake.custom_fake = verify_service_callback_args;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_SERVICE_RESPONSE_CB, OT_ERROR_NONE,
				CBOR_UINT32(0xFACEFACE), CBOR_UINT32(UINT32_MAX), service_cb_slot));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_dns_service_cb_fake.call_count, 1);
}

static void verify_service_callback_name(otError error, const otDnsServiceResponse *response,
					 void *context)
{
	otError api_err;

	verify_service_callback_args(error, response, context);

	api_err = otDnsServiceResponseGetServiceName(response, instance_buffer,
						     sizeof(instance_buffer), name_buffer,
						     sizeof(name_buffer));
	zassert_equal(api_err, OT_ERROR_NONE);
	zassert_str_equal(instance_buffer, DNS_SERVICE_INSTANCE);
	zassert_str_equal(name_buffer, DNS_SERVICE_NAME);
}

/**
 * Verify that service name can be fetched from service resolve response's callback.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_NAME):
 *        response pointer(0xFACEFACE), max service name length(128), max instance name length(128)
 *
 * Responses:
 *   1. RPC_RSP: OT_ERROR_NONE, service name("_test._udp.example.com"), service instance("Test")
 *   2. RPC_RSP: void
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_SERVICE_RESPONSE_CB):
 *        OT_ERROR_NONE, response pointer(0xFACEFACE), response context(0xFFFFFFFF),
 *        callback slot(assigned at init)
 */
ZTEST(ot_rpc_dns_client, test_otDnsServiceResponseGetServiceName)
{
	ot_dns_service_cb_fake.custom_fake = verify_service_callback_name;

	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_NAME,
					      CBOR_UINT32(0xFACEFACE),
					      CBOR_UINT8(sizeof(name_buffer)),
					      CBOR_UINT8(sizeof(instance_buffer))),
				   RPC_RSP(OT_ERROR_NONE, CBOR_DNS_SERVICE_NAME,
					CBOR_SERVICE_INSTANCE));
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_SERVICE_RESPONSE_CB, OT_ERROR_NONE,
				CBOR_UINT32(0xFACEFACE), CBOR_UINT32(UINT32_MAX), service_cb_slot));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_dns_service_cb_fake.call_count, 1);
}

static void verify_service_response_service_info(otError error,
						 const otDnsServiceResponse *response,
						 void *context)
{
	otError api_err;
	otDnsServiceInfo info;

	verify_service_callback_args(error, response, context);

	info.mHostNameBuffer = name_buffer;
	info.mHostNameBufferSize = sizeof(name_buffer);
	info.mTxtData = txt_data;
	info.mTxtDataSize = sizeof(txt_data);

	api_err = otDnsServiceResponseGetServiceInfo(response, &info);

	zassert_equal(api_err, OT_ERROR_NONE);

	validate_service_info(&info);
}

/**
 * Verify that service info can be fetched from browse response's callback.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_INFO):
 *        response pointer(0xFACEFACE), max name length(128), max txt size(128)
 * Responses:
 *   1. RPC_RSP:
 *        OT_ERROR_NONE, ttl(120), port(12345), priority(10), weight(20), hostname("example.com"),
 *        ipv6 address(2001:db8::1), address TTL(120), TXT data (16 bytes) truncated flag(false),
 *        TXT data TTL(60)
 *   2. RPC_RSP: void
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_SERVICE_RESPONSE_CB):
 *        OT_ERROR_NONE, response pointer(0xFACEFACE), response context(0xFFFFFFFF),
 *        callback slot(assigned at init)
 */
ZTEST(ot_rpc_dns_client, test_otDnsServiceResponseGetServiceInfo)
{
	ot_dns_service_cb_fake.custom_fake = verify_service_response_service_info;

	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_INFO,
					      CBOR_UINT32(0xFACEFACE),
					      CBOR_UINT8(sizeof(name_buffer)),
					      CBOR_UINT8(sizeof(txt_data))),
				   RPC_RSP(OT_ERROR_NONE, CBOR_SERVICE_INFO));
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_SERVICE_RESPONSE_CB, OT_ERROR_NONE,
				CBOR_UINT32(0xFACEFACE), CBOR_UINT32(UINT32_MAX), service_cb_slot));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_dns_service_cb_fake.call_count, 1);
}

static void verify_service_response_host_address(otError error,
						 const otDnsServiceResponse *response,
						 void *context)
{
	otError api_err;
	otIp6Address address;
	uint32_t ttl;

	verify_service_callback_args(error, response, context);

	api_err = otDnsServiceResponseGetHostAddress(response, DNS_HOSTNAME, 0, &address,
						    &ttl);

	zassert_equal(api_err, OT_ERROR_NONE);
	zassert_mem_equal(&address, &test_addr, sizeof(test_addr));
	zassert_equal(ttl, 120);

	api_err = otDnsServiceResponseGetHostAddress(response, DNS_HOSTNAME, 1, &address,
						     &ttl);

	zassert_equal(api_err, OT_ERROR_NOT_FOUND);
}

/**
 * Verify that host address can be properly fetched from service response's callback. Also, verify
 * that error is properly returned for non-existing address.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_SERVICE_RESP_GET_HOST_ADDRESS):
 *        response pointer(0xFACEFACE), index(0), hostname("example.com")
 *   2. RPC_CMD (OT_RPC_CMD_DNS_SERVICE_RESP_GET_HOST_ADDRESS):
 *        response pointer(0xFACEFACE), index(1), hostname("example.com")
 *
 * Responses:
 *   1. RPC_RSP: OT_ERROR_NONE, ipv6 address(2001:db8::1), ttl(120)
 *   2. RPC_RSP: OT_ERROR_NOT_FOUND
 *   3. RPC_RSP: void
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_SERVICE_RESPONSE_CB):
 *        OT_ERROR_NONE, response pointer(0xFACEFACE), context(0xFFFFFFFF),
 *        callback slot(assigned at init)
 */
ZTEST(ot_rpc_dns_client, test_otDnsServiceResponseGetHostAddress)
{
	ot_dns_service_cb_fake.custom_fake = verify_service_response_host_address;

	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_SERVICE_RESP_GET_HOST_ADDRESS,
					      CBOR_UINT32(0xFACEFACE), /* index */ 0,
					      CBOR_DNS_HOSTNAME),
				   RPC_RSP(OT_ERROR_NONE, CBOR_IPV6_ADDR,
					   /* TTL */ CBOR_UINT8(120)));
	mock_nrf_rpc_tr_expect_add(RPC_CB_CMD(OT_RPC_CMD_DNS_SERVICE_RESP_GET_HOST_ADDRESS,
					      CBOR_UINT32(0xFACEFACE), /* index */ 1,
					      CBOR_DNS_HOSTNAME),
				   RPC_RSP(OT_ERROR_NOT_FOUND));
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_SERVICE_RESPONSE_CB, OT_ERROR_NONE,
				CBOR_UINT32(0xFACEFACE), CBOR_UINT32(UINT32_MAX), service_cb_slot));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_dns_service_cb_fake.call_count, 1);
}

ZTEST_SUITE(ot_rpc_dns_client, NULL, tc_setup, NULL, tc_cleanup, NULL);
