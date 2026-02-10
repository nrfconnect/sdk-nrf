/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common_fakes.h"

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_lock.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/net/openthread.h>
#include <zephyr/ztest.h>

#include <openthread/instance.h>
#include <openthread/dns_client.h>

FAKE_VALUE_FUNC(const otDnsQueryConfig *, otDnsClientGetDefaultConfig, otInstance *);
FAKE_VOID_FUNC(otDnsClientSetDefaultConfig, otInstance *, const otDnsQueryConfig *);
FAKE_VALUE_FUNC(otError, otDnsClientResolveAddress, otInstance *, const char *,
		otDnsAddressCallback, void *, const otDnsQueryConfig *);
FAKE_VALUE_FUNC(otError, otDnsClientResolveIp4Address, otInstance *, const char *,
		otDnsAddressCallback, void *, const otDnsQueryConfig *);
FAKE_VALUE_FUNC(otError, otDnsClientResolveService, otInstance *, const char *, const char *,
		otDnsServiceCallback, void *, const otDnsQueryConfig *);
FAKE_VALUE_FUNC(otError, otDnsClientResolveServiceAndHostAddress, otInstance *, const char *,
		const char *, otDnsServiceCallback, void *, const otDnsQueryConfig *);
FAKE_VALUE_FUNC(otError, otDnsAddressResponseGetHostName, const otDnsAddressResponse *, char *,
		uint16_t);
FAKE_VALUE_FUNC(otError, otDnsServiceResponseGetServiceName, const otDnsServiceResponse *, char *,
		uint8_t, char *, uint16_t);
FAKE_VALUE_FUNC(otError, otDnsClientBrowse, otInstance *, const char *, otDnsBrowseCallback, void *,
		const otDnsQueryConfig *);
FAKE_VALUE_FUNC(otError, otDnsBrowseResponseGetServiceName, const otDnsBrowseResponse *, char *,
		uint16_t);
FAKE_VALUE_FUNC(otError, otDnsAddressResponseGetAddress, const otDnsAddressResponse *, uint16_t,
		otIp6Address *, uint32_t *);
FAKE_VALUE_FUNC(otError, otDnsServiceResponseGetHostAddress, const otDnsServiceResponse *,
		const char *, uint16_t, otIp6Address *, uint32_t *);
FAKE_VALUE_FUNC(otError, otDnsBrowseResponseGetServiceInstance, const otDnsBrowseResponse *,
		uint16_t, char *, uint8_t);
FAKE_VALUE_FUNC(otError, otDnsBrowseResponseGetHostAddress, const otDnsBrowseResponse *,
		const char *, uint16_t, otIp6Address *, uint32_t *);
FAKE_VALUE_FUNC(otError, otDnsBrowseResponseGetServiceInfo, const otDnsBrowseResponse *,
		const char *, otDnsServiceInfo *);
FAKE_VALUE_FUNC(otError, otDnsServiceResponseGetServiceInfo, const otDnsServiceResponse *,
		otDnsServiceInfo *);

#define FOREACH_FAKE(f)                                                                            \
	f(otDnsClientGetDefaultConfig);                                                            \
	f(otDnsClientSetDefaultConfig);                                                            \
	f(otDnsClientResolveAddress);                                                              \
	f(otDnsClientResolveIp4Address);                                                           \
	f(otDnsClientResolveService);                                                              \
	f(otDnsClientResolveServiceAndHostAddress);                                                \
	f(otDnsAddressResponseGetHostName);                                                        \
	f(otDnsServiceResponseGetServiceName);                                                     \
	f(otDnsClientBrowse);                                                                      \
	f(otDnsBrowseResponseGetServiceName);                                                      \
	f(otDnsAddressResponseGetAddress);                                                         \
	f(otDnsServiceResponseGetHostAddress);                                                     \
	f(otDnsBrowseResponseGetServiceInstance);                                                  \
	f(otDnsBrowseResponseGetHostAddress);                                                      \
	f(otDnsBrowseResponseGetServiceInfo);                                                      \
	f(otDnsServiceResponseGetServiceInfo);                                                     \
	f(nrf_rpc_cbkproxy_out_get);

extern uint64_t browse_response_callback_encoder(uint32_t callback_slot, uint32_t _rsv0,
						 uint32_t _rsv1, uint32_t _ret, otError error,
						 const otDnsBrowseResponse *response,
						 void *context);
extern uint64_t address_response_callback_encoder(uint32_t callback_slot, uint32_t _rsv0,
						  uint32_t _rsv1, uint32_t _ret, otError error,
						  const otDnsAddressResponse *response,
						  void *context);
extern uint64_t service_response_callback_encoder(uint32_t callback_slot, uint32_t _rsv0,
						  uint32_t _rsv1, uint32_t _ret, otError error,
						  const otDnsServiceResponse *response,
						  void *context);

static otDnsQueryConfig config;
static otDnsQueryConfig *expected_config;
static struct net_in6_addr dns_addr = {{{ DNS_IPV6_ADDR }}};
static size_t service_info_txt_size;


static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void *ts_setup(void)
{
	memcpy(config.mServerSockAddr.mAddress.mFields.m8, &dns_addr, sizeof(dns_addr));

	config.mServerSockAddr.mPort = DNS_PORT;
	config.mResponseTimeout = DNS_TIMEOUT;
	config.mMaxTxAttempts = DNS_ATTEMPTS;
	config.mRecursionFlag = OT_DNS_FLAG_RECURSION_DESIRED;
	config.mNat64Mode = OT_DNS_NAT64_DISALLOW;
	config.mServiceMode = OT_DNS_SERVICE_MODE_SRV_TXT_OPTIMIZE;
	config.mTransportProto = OT_DNS_TRANSPORT_UDP;

	return NULL;
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();

	FOREACH_FAKE(RESET_FAKE);
	FFF_RESET_HISTORY();
}

/**
 * Verify that response callbacks are properly encoded by calling functions generated by
 * NRF_RPC_CBKPROXY_HANDLER.
 *
 * Expected packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_ADDRESS_RESPONSE_CB):
 *        error(0), response pointer(0xaaaaaaaa), context(0), callback slot(0)
 *   2. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB):
 *        error(0), response pointer(0xbbbbbbbb), context(0), callback slot(0)
 *   2. RPC_CMD (OT_RPC_CMD_DNS_SERVICE_RESPONSE_CB):
 *        error(0), response pointer(0xcccccccc), context(0), callback slot(0)
 */
ZTEST(ot_rpc_dns_client, test_response_callbacks)
{
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DNS_ADDRESS_RESPONSE_CB, 0,
				   CBOR_UINT32(0xaaaaaaaa), 0, 0), RPC_RSP());
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB, 0,
				   CBOR_UINT32(0xbbbbbbbb), 0, 0), RPC_RSP());
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DNS_SERVICE_RESPONSE_CB, 0,
				   CBOR_UINT32(0xcccccccc), 0, 0), RPC_RSP());

	ot_rpc_mutex_lock();
	(void)address_response_callback_encoder(0, 0, 0, 0, OT_ERROR_NONE, (void *)0xaaaaaaaa,
						NULL);
	(void)browse_response_callback_encoder(0, 0, 0, 0, OT_ERROR_NONE, (void *)0xbbbbbbbb, NULL);
	(void)service_response_callback_encoder(0, 0, 0, 0, OT_ERROR_NONE, (void *)0xcccccccc,
						NULL);
	ot_rpc_mutex_unlock();

	mock_nrf_rpc_tr_expect_done();
}

/**
 * Verify that RPC server responds with CBOR_NULL when otDnsClientGetDefaultConfig returns NULL.
 *
 * Expected packets:
 *   1. RPC_RSP: NULL
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_GET_DEFAULT_CONFIG)
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientGetDefaultConfig_null)
{
	otDnsClientGetDefaultConfig_fake.return_val = NULL;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_NULL), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_GET_DEFAULT_CONFIG));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsClientGetDefaultConfig_fake.call_count, 1);
}

/**
 * Verify that RPC server responds with encoded config.
 *
 * Expected packets:
 *   1. RPC_RSP:
 *        ipv6 address(2001:db8::1), port(53), timeout(1000), attempts(5), recursion flag(1),
 *        nat64 flag(2), service mode flag(5), transport flag(1)
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_GET_DEFAULT_CONFIG)
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientGetDefaultConfig_valid)
{
	otDnsClientGetDefaultConfig_fake.return_val = &config;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_DNS_QUERY_CONFIG), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_GET_DEFAULT_CONFIG));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsClientGetDefaultConfig_fake.call_count, 1);
}

void verify_config(const otDnsQueryConfig *config)
{
	if (expected_config == NULL) {
		zassert_is_null(config);
	} else {
		zassert_not_null(config);

		zassert_mem_equal(&config->mServerSockAddr.mAddress,
				  &expected_config->mServerSockAddr.mAddress, OT_IP6_ADDRESS_SIZE);
		zassert_equal(config->mServerSockAddr.mPort,
			      expected_config->mServerSockAddr.mPort);

		zassert_equal(config->mResponseTimeout, expected_config->mResponseTimeout);
		zassert_equal(config->mMaxTxAttempts, expected_config->mMaxTxAttempts);
		zassert_equal(config->mRecursionFlag, expected_config->mRecursionFlag);
		zassert_equal(config->mNat64Mode, expected_config->mNat64Mode);
		zassert_equal(config->mServiceMode, expected_config->mServiceMode);
		zassert_equal(config->mTransportProto, expected_config->mTransportProto);
	}
}

void custom_default_config_set(otInstance *instance, const otDnsQueryConfig *config)
{
	verify_config(config);
}

/**
 * Verify that RPC server decodes and stores default config.
 *
 * Expected packets:
 *   1. RPC_RSP: void
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_SET_DEFAULT_CONFIG):
 *        ipv6 address(2001:db8::1), port(53), timeout(1000), attempts(5), recursion flag(1),
 *        nat64 flag(2), service mode flag(5), transport flag(1)
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientSetDefaultConfig)
{
	otDnsClientSetDefaultConfig_fake.custom_fake = custom_default_config_set;

	expected_config = &config;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_SET_DEFAULT_CONFIG,
				CBOR_DNS_QUERY_CONFIG));

	zassert_equal(otDnsClientSetDefaultConfig_fake.call_count, 1);
}

otError verify_resolve_address_args(otInstance *instance, const char *hostname,
				    otDnsAddressCallback cb, void *ctx,
				    const otDnsQueryConfig *config)
{
	zassert_str_equal(hostname, DNS_HOSTNAME);

	verify_config(config);

	return OT_ERROR_NONE;
}

/**
 * Verify that address resolve command arguments are properly decoded.
 *
 * Expected packets:
 *   1. RPC_RSP: OT_ERROR_NONE
 *   2. RPC_RSP: OT_ERROR_NONE
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_RESOLVE_ADDRESS):
 *        hostname("example.com"), callback slot(0), callback context(0xdeadbeef),
 *        ipv6 address(2001:db8::1), port(53), timeout(1000), attempts(5), recursion flag(1),
 *        nat64 flag(2), service mode flag(5), transport flag(1)
 *   2. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_RESOLVE_ADDRESS):
 *        hostname("example.com"), callback slot(0), callback context(0xdeadbeef)
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientResolveAddress)
{
	nrf_rpc_cbkproxy_out_get_fake.return_val = (void *)0xfacecafe;
	otDnsClientResolveAddress_fake.custom_fake = verify_resolve_address_args;

	expected_config = &config;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_RESOLVE_ADDRESS, CBOR_DNS_HOSTNAME, 0,
				CBOR_UINT32(0xdeadbeef), CBOR_DNS_QUERY_CONFIG));

	zassert_equal(otDnsClientResolveAddress_fake.call_count, 1);
	zassert_equal(otDnsClientResolveAddress_fake.arg2_val, (void *)0xfacecafe);
	zassert_equal(otDnsClientResolveAddress_fake.arg3_val, (void *)0xdeadbeef);

	expected_config = NULL;

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_RESOLVE_ADDRESS, CBOR_DNS_HOSTNAME, 0,
				CBOR_UINT32(0xdeadbeef), CBOR_NULL));

	zassert_equal(otDnsClientResolveAddress_fake.call_count, 2);
	zassert_equal(otDnsClientResolveAddress_fake.arg2_val, (void *)0xfacecafe);
	zassert_equal(otDnsClientResolveAddress_fake.arg3_val, (void *)0xdeadbeef);

	mock_nrf_rpc_tr_expect_done();
}

/**
 * Verify that IPv4 address resolve command arguments are properly decoded.
 *
 * Expected packets:
 *   1. RPC_RSP: OT_ERROR_NONE
 *   2. RPC_RSP: OT_ERROR_NONE
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_RESOLVE_IP4_ADDRESS):
 *        hostname("example.com"), callback slot(0), callback context(0xdeadbeef),
 *        ipv6 address(2001:db8::1), port(53), timeout(1000), attempts(5), recursion flag(1),
 *        nat64 flag(2), service mode flag(5), transport flag(1)
 *   2. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_RESOLVE_IP4_ADDRESS):
 *        hostname("example.com"), callback slot(0), callback context(0xdeadbeef)
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientResolveIp4Address)
{
	nrf_rpc_cbkproxy_out_get_fake.return_val = (void *)0xfacecafe;
	otDnsClientResolveIp4Address_fake.custom_fake = verify_resolve_address_args;

	expected_config = &config;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_RESOLVE_IP4_ADDRESS,
					CBOR_DNS_HOSTNAME, 0, CBOR_UINT32(0xdeadbeef),
					CBOR_DNS_QUERY_CONFIG));

	zassert_equal(otDnsClientResolveIp4Address_fake.call_count, 1);
	zassert_equal(otDnsClientResolveIp4Address_fake.arg2_val, (void *)0xfacecafe);
	zassert_equal(otDnsClientResolveIp4Address_fake.arg3_val, (void *)0xdeadbeef);

	expected_config = NULL;

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_RESOLVE_IP4_ADDRESS,
					CBOR_DNS_HOSTNAME, 0, CBOR_UINT32(0xdeadbeef), CBOR_NULL));

	zassert_equal(otDnsClientResolveIp4Address_fake.call_count, 2);
	zassert_equal(otDnsClientResolveIp4Address_fake.arg2_val, (void *)0xfacecafe);
	zassert_equal(otDnsClientResolveIp4Address_fake.arg3_val, (void *)0xdeadbeef);

	mock_nrf_rpc_tr_expect_done();
}

otError custom_get_host_name(const otDnsAddressResponse *response, char *name, uint16_t size)
{
	size_t strl = strlen(DNS_HOSTNAME);

	zassert_equal(size, 64);
	zassert_equal(response, (const otDnsAddressResponse *)0xabcdabcd);

	memcpy(name, DNS_HOSTNAME, strl + 1);
	name[strl] = 0;

	return OT_ERROR_NONE;
}

/**
 * Verify that hostname is properly encoded.
 *
 * Expected packets:
 *   1. RPC_RSP: OT_ERROR_NONE, hostname("example.com")
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_ADDRESS_RESP_GET_HOST_NAME):
 *        response pointer(0xabcdabcd), max hostname length(64)
 */
ZTEST(ot_rpc_dns_client, test_otDnsAddressResponseGetHostName)
{
	otDnsAddressResponseGetHostName_fake.custom_fake = custom_get_host_name;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_DNS_HOSTNAME), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_ADDRESS_RESP_GET_HOST_NAME,
				CBOR_UINT32(0xabcdabcd), CBOR_UINT8(64)));

	mock_nrf_rpc_tr_expect_done();
}

otError custom_get_address(const otDnsAddressResponse *response, uint16_t index,
			   otIp6Address *address, uint32_t *ttl)
{
	zassert_equal(response, (const otDnsAddressResponse *)0xabcdabcd);
	zassert_equal(index, 0);

	memcpy(address, &dns_addr, OT_IP6_ADDRESS_SIZE);

	*ttl = 120;

	return OT_ERROR_NONE;
}

/**
 * Verify that host address is properly encoded.
 *
 * Expected packets:
 *   1. RPC_RSP: OT_ERROR_NONE, ipv6 address(2001:db8::1)
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_ADDRESS_RESP_GET_HOST_NAME):
 *        response pointer(0xabcdabcd), index(0)
 */
ZTEST(ot_rpc_dns_client, test_otDnsAddressResponseGetAddress)
{
	otDnsAddressResponseGetAddress_fake.custom_fake = custom_get_address;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_IPV6_ADDR, CBOR_UINT8(120)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_ADDRESS_RESP_GET_ADDRESS,
				CBOR_UINT32(0xabcdabcd), 0));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsAddressResponseGetAddress_fake.call_count, 1);
}

otError verify_browse_args(otInstance *instance, const char *name, otDnsBrowseCallback cb,
			   void *ctx, const otDnsQueryConfig *config)
{
	zassert_equal(cb, (void *)0xfacecafe);
	zassert_equal(ctx, (void *)0xdeadbeef);

	verify_config(config);

	return OT_ERROR_NONE;
}

/**
 * Verify that browse command arguments are properly decoded.
 *
 * Expected packets:
 *   1. RPC_RSP: OT_ERROR_NONE
 *   2. RPC_RSP: OT_ERROR_NONE
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_BROWSE):
 *        service name("_test._udp.example.com"), callback slot(0), callback context(0xdeadbeef),
 *        ipv6 address(2001:db8::1), port(53), timeout(1000), attempts(5), recursion flag(1),
 *        nat64 flag(2), service mode flag(5), transport flag(1)
 *   2. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_BROWSE):
 *        service name("_test._udp.example.com"), callback slot(0), callback context(0xdeadbeef)
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientBrowse)
{
	nrf_rpc_cbkproxy_out_get_fake.return_val = (void *)0xfacecafe;

	otDnsClientBrowse_fake.custom_fake = verify_browse_args;

	expected_config = &config;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_BROWSE, CBOR_DNS_SERVICE_NAME, 0,
				CBOR_UINT32(0xdeadbeef), CBOR_DNS_QUERY_CONFIG));

	expected_config = NULL;

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_BROWSE, CBOR_DNS_SERVICE_NAME, 0,
				CBOR_UINT32(0xdeadbeef), CBOR_NULL));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsClientBrowse_fake.call_count, 2);
}

otError custom_browse_get_service_name(const otDnsBrowseResponse *response, char *name,
				       uint16_t size)
{
	size_t strl = strlen(DNS_SERVICE_NAME);

	zassert_equal(size, 64);
	zassert_equal(response, (const otDnsBrowseResponse *)0xabcdabcd);

	memcpy(name, DNS_SERVICE_NAME, strl + 1);
	name[strl] = 0;

	return OT_ERROR_NONE;
}

/**
 * Verify that service name is properly encoded.
 *
 * Expected packets:
 *   1. RPC_RSP: OT_ERROR_NONE, service name("_test._udp.example.com")
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_NAME):
 *        response pointer(0xabcdabcd), max name length(64)
 */
ZTEST(ot_rpc_dns_client, test_otDnsBrowseResponseGetServiceName)
{
	otDnsBrowseResponseGetServiceName_fake.custom_fake = custom_browse_get_service_name;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_DNS_SERVICE_NAME), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_NAME,
				CBOR_UINT32(0xabcdabcd), CBOR_UINT8(64)));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsBrowseResponseGetServiceName_fake.call_count, 1);
}

otError custom_browse_get_instance(const otDnsBrowseResponse *response, uint16_t index,
				   char *label, uint8_t size)
{
	zassert_equal(response, (const otDnsBrowseResponse *)0xabcdabcd);
	zassert_equal(index, 0);
	zassert_equal(size, 64);

	strcpy(label, DNS_SERVICE_INSTANCE);

	return OT_ERROR_NONE;
}

/**
 * Verify that service instance is properly encoded.
 *
 * Expected packets:
 *   1. RPC_RSP: OT_ERROR_NONE, service instance("Test")
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_NAME):
 *        response pointer(0xabcdabcd), index(0), max name length(64)
 */
ZTEST(ot_rpc_dns_client, test_otDnsBrowseResponseGetServiceInstance)
{
	otDnsBrowseResponseGetServiceInstance_fake.custom_fake = custom_browse_get_instance;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_SERVICE_INSTANCE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INSTANCE,
				CBOR_UINT32(0xabcdabcd), 0, CBOR_UINT8(64)));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsBrowseResponseGetServiceInstance_fake.call_count, 1);
}

void fill_service_info(otDnsServiceInfo *info)
{
	info->mTtl = 120;
	info->mPort = 12345;
	info->mPriority = 10;
	info->mWeight = 20;

	if (info->mHostNameBuffer) {
		strcpy(info->mHostNameBuffer, DNS_HOSTNAME);
	}

	memcpy(&info->mHostAddress, &dns_addr, OT_IP6_ADDRESS_SIZE);

	info->mHostAddressTtl = 120;
	info->mTxtDataTruncated = false;
	info->mTxtDataTtl = 60;

	if (service_info_txt_size == 0) {
		info->mTxtData = NULL;
		info->mTxtDataSize = 0;
	} else if (info->mTxtData) {
		for (int i = 0; i <= service_info_txt_size; ++i) {
			info->mTxtData[i] = (i + 1) % UINT8_MAX;
		}
		info->mTxtDataSize = service_info_txt_size;
	}
}

otError custom_browse_get_service_info(const otDnsBrowseResponse *response, const char *instance,
				       otDnsServiceInfo *info)
{
	zassert_str_equal(instance, DNS_SERVICE_INSTANCE);

	fill_service_info(info);

	return OT_ERROR_NONE;
}

/**
 * Verify that service info is properly encoded when both hostname and txt data buffer are provided.
 *
 * Expected packets:
 *   1. RPC_RSP:
 *        OT_ERROR_NONE, ttl(120), port(12345), priority(10), weight(20), hostname("example.com"),
 *        ipv6 address(2001:db8::1), address TTL(120), TXT data(8 bytes sequence),
 *        truncated flag(false), TXT data TTL(60)
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO):
 *        response pointer(0xabcdabcd), max name length(128), max txt size(128), instance("Test")
 */
ZTEST(ot_rpc_dns_client, test_otDnsBrowseResponseGetServiceInfo_full_data)
{
	otDnsBrowseResponseGetServiceInfo_fake.custom_fake = custom_browse_get_service_info;

	service_info_txt_size = 8;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_UINT8(120), CBOR_UINT16(12345),
				   10, 20, CBOR_DNS_HOSTNAME, CBOR_IPV6_ADDR, CBOR_UINT8(120), 0x48,
				   INT_SEQUENCE(8), CBOR_FALSE, CBOR_UINT8(60)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO,
				CBOR_UINT32(0xabcdabcd), CBOR_UINT8(128), CBOR_UINT8(128),
				CBOR_SERVICE_INSTANCE));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsBrowseResponseGetServiceInfo_fake.call_count, 1);
}

/**
 * Verify that service info is properly encoded when both hostname and txt data buffer are provided
 * but there is no TXT data in the info.
 *
 * Expected packets:
 *   1. RPC_RSP:
 *        OT_ERROR_NONE, ttl(120), port(12345), priority(10), weight(20), hostname("example.com"),
 *        ipv6 address(2001:db8::1), address TTL(120), TXT data(NULL), truncated flag(false),
 *        TXT data TTL(60)
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO):
 *        response pointer(0xabcdabcd), max name length(128), max txt size(128), instance("Test")
 */
ZTEST(ot_rpc_dns_client, test_otDnsBrowseResponseGetServiceInfo_no_txt)
{
	otDnsBrowseResponseGetServiceInfo_fake.custom_fake = custom_browse_get_service_info;

	service_info_txt_size = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_UINT8(120), CBOR_UINT16(12345),
					   10, 20, CBOR_DNS_HOSTNAME, CBOR_IPV6_ADDR,
					   CBOR_UINT8(120), CBOR_NULL, CBOR_FALSE, CBOR_UINT8(60)),
					   NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO,
				CBOR_UINT32(0xabcdabcd), CBOR_UINT8(128), CBOR_UINT8(128),
				CBOR_SERVICE_INSTANCE));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsBrowseResponseGetServiceInfo_fake.call_count, 1);
}

/**
 * Verify that service info is properly encoded when the info contains txt data but the buffer is
 * not provided.
 *
 * Expected packets:
 *   1. RPC_RSP:
 *        OT_ERROR_NONE, ttl(120), port(12345), priority(10), weight(20), hostname("example.com"),
 *        ipv6 address(2001:db8::1), address TTL(120), truncated flag(false), TXT data TTL(60)
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO):
 *        response pointer(0xabcdabcd), max name length(128), max txt size(0), instance("Test")
 */
ZTEST(ot_rpc_dns_client, test_otDnsBrowseResponseGetServiceInfo_txt_without_buffer)
{
	otDnsBrowseResponseGetServiceInfo_fake.custom_fake = custom_browse_get_service_info;

	service_info_txt_size = 64;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_UINT8(120), CBOR_UINT16(12345),
					   10, 20, CBOR_DNS_HOSTNAME, CBOR_IPV6_ADDR,
					   CBOR_UINT8(120), CBOR_FALSE, CBOR_UINT8(60)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO,
				CBOR_UINT32(0xabcdabcd), CBOR_UINT8(128), 0,
				CBOR_SERVICE_INSTANCE));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsBrowseResponseGetServiceInfo_fake.call_count, 1);
}

/**
 * Verify that service info is properly encoded when the buffer for service name is not provided.
 *
 * Expected packets:
 *   1. RPC_RSP:
 *        OT_ERROR_NONE, ttl(120), port(12345), priority(10), weight(20), ipv6 address(2001:db8::1),
 *        address TTL(120), TXT data(NULL), truncated flag(false), TXT data TTL(60)
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO):
 *        response pointer(0xabcdabcd), max name length(0), max txt size(128), instance("Test")
 */
ZTEST(ot_rpc_dns_client, test_otDnsBrowseResponseGetServiceInfo_no_hostname_buffer)
{
	otDnsBrowseResponseGetServiceInfo_fake.custom_fake = custom_browse_get_service_info;

	service_info_txt_size = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_UINT8(120), CBOR_UINT16(12345),
					   10, 20, CBOR_IPV6_ADDR, CBOR_UINT8(120),
					   CBOR_NULL, CBOR_FALSE, CBOR_UINT8(60)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO,
				CBOR_UINT32(0xabcdabcd), 0, CBOR_UINT8(128),
				CBOR_SERVICE_INSTANCE));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsBrowseResponseGetServiceInfo_fake.call_count, 1);
}

otError custom_browse_get_host_address(const otDnsBrowseResponse *response, const char *name,
				       uint16_t index, otIp6Address *address, uint32_t *ttl)
{
	zassert_str_equal(name, DNS_HOSTNAME);
	zassert_equal(index, 0);

	zassert_not_null(address);
	zassert_not_null(ttl);

	memcpy(address, &dns_addr, OT_IP6_ADDRESS_SIZE);

	*ttl = 120;

	return OT_ERROR_NONE;
}

/**
 * Verify that host address is properly encoded.
 *
 * Expected packets:
 *   1. RPC_RSP: OT_ERROR_NONE, ipv6 address(2001:db8::1), ttl(120)s
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_HOST_ADDRESS):
 *        response pointer(0xabcdabcd), index(0), hostname("example.com")
 */
ZTEST(ot_rpc_dns_client, test_otDnsBrowseResponseGetHostAddress)
{
	otDnsBrowseResponseGetHostAddress_fake.custom_fake = custom_browse_get_host_address;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_IPV6_ADDR, CBOR_UINT8(120)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_BROWSE_RESP_GET_HOST_ADDRESS,
				CBOR_UINT32(0xabcdabcd), 0, CBOR_DNS_HOSTNAME));

	mock_nrf_rpc_tr_expect_done();
}

otError verify_service_args(otInstance *instance, const char *instance_name, const char *service,
			    otDnsServiceCallback cb, void *ctx, const otDnsQueryConfig *config)
{
	zassert_str_equal(instance_name, DNS_SERVICE_INSTANCE);
	zassert_str_equal(service, DNS_SERVICE_NAME);
	zassert_equal(cb, (otDnsServiceCallback)0xfacecafe);
	zassert_equal(ctx, (void *)0xdeadbeef);

	verify_config(config);

	return OT_ERROR_NONE;
}

/**
 * Verify that service resolve command arguments are properly decoded.
 *
 * Expected packets:
 *   1. RPC_RSP: OT_ERROR_NONE
 *   2. RPC_RSP: OT_ERROR_NONE
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE):
 *        service instance("Test"), service name("_test._udp.example.com"), callback slot(0),
 *        callback context(0xdeadbeef)
 *   2. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE):
 *        service instance("Test"), service name("_test._udp.example.com"), callback slot(0),
 *        callback context(0xdeadbeef), ipv6 address(2001:db8::1), port(53), timeout(1000),
 *        attempts(5), recursion flag(1), nat64 flag(2), service mode flag(5), transport flag(1)
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientResolveService)
{
	nrf_rpc_cbkproxy_out_get_fake.return_val = (void *)0xfacecafe;
	otDnsClientResolveService_fake.custom_fake = verify_service_args;

	expected_config = NULL;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE,
				CBOR_SERVICE_INSTANCE, CBOR_DNS_SERVICE_NAME, 0,
				CBOR_UINT32(0xdeadbeef), CBOR_NULL));

	expected_config = &config;

	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE,
				CBOR_SERVICE_INSTANCE, CBOR_DNS_SERVICE_NAME, 0,
				CBOR_UINT32(0xdeadbeef), CBOR_DNS_QUERY_CONFIG));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsClientResolveService_fake.call_count, 2);
}

/**
 * Verify that service resolve command arguments are properly decoded.
 *
 * Expected packets:
 *   1. RPC_RSP: OT_ERROR_NONE
 *   2. RPC_RSP: OT_ERROR_NONE
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE):
 *        service instance("Test"), service name("_test._udp.example.com"), callback slot(0),
 *        callback context(0xdeadbeef)
 *   2. RPC_CMD (OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE):
 *        service instance("Test"), service name("_test._udp.example.com"), callback slot(0),
 *        callback context(0xdeadbeef), ipv6 address(2001:db8::1), port(53), timeout(1000),
 *        attempts(5), recursion flag(1), nat64 flag(2), service mode flag(5), transport flag(1)
 */
ZTEST(ot_rpc_dns_client, test_otDnsClientResolveServiceAndHostAddress)
{
	nrf_rpc_cbkproxy_out_get_fake.return_val = (void *)0xfacecafe;
	otDnsClientResolveServiceAndHostAddress_fake.custom_fake = verify_service_args;

	expected_config = NULL;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE_AND_HOST_ADDRESS,
				CBOR_SERVICE_INSTANCE, CBOR_DNS_SERVICE_NAME, 0,
				CBOR_UINT32(0xdeadbeef), CBOR_NULL));

	expected_config = &config;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE_AND_HOST_ADDRESS,
				CBOR_SERVICE_INSTANCE, CBOR_DNS_SERVICE_NAME, 0,
				CBOR_UINT32(0xdeadbeef), CBOR_DNS_QUERY_CONFIG));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsClientResolveServiceAndHostAddress_fake.call_count, 2);
}

otError custom_get_service_name(const otDnsServiceResponse *response, char *label,
				uint8_t max_label_size, char *name, uint16_t max_name_size)
{
	zassert_equal(response, (const otDnsServiceResponse *)0xabcdabcd);
	zassert_equal(max_label_size, 64);
	zassert_equal(max_name_size, 128);

	strcpy(label, DNS_SERVICE_INSTANCE);
	strcpy(name, DNS_SERVICE_NAME);

	return OT_ERROR_NONE;
}

/**
 * Verify that service name is properly encoded.
 *
 * Expected packets:
 *   1. RPC_RSP: OT_ERROR_NONE, service name("_test._udp.example.com"), service instance("Test")
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_NAME):
 *        response pointer(0xabcdabcd), max service name length(128), max instance name length(64)
 */
ZTEST(ot_rpc_dns_client, test_otDnsServiceResponseGetServiceName)
{
	otDnsServiceResponseGetServiceName_fake.custom_fake = custom_get_service_name;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_DNS_SERVICE_NAME,
					   CBOR_SERVICE_INSTANCE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_NAME,
				CBOR_UINT32(0xabcdabcd), CBOR_UINT8(128), CBOR_UINT8(64)));

	mock_nrf_rpc_tr_expect_done();
}

otError custom_get_service_info(const otDnsServiceResponse *response, otDnsServiceInfo *info)
{
	zassert_equal(response, (const otDnsServiceResponse *)0xabcdabcd);

	fill_service_info(info);

	return OT_ERROR_NONE;
}

/**
 * Verify that service info is properly encoded.
 *
 * Expected packets:
 *   1. RPC_RSP:
 *        OT_ERROR_NONE, ttl(120), port(12345), priority(10), weight(20), hostname("example.com"),
 *        ipv6 address(2001:db8::1), address TTL(120), TXT data(8 bytes sequence),
 *        truncated flag(false), TXT data TTL(60)
 *
 * Received packets:
 *   1. RPC_CMD (OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO):
 *        response pointer(0xabcdabcd), max name length(128), max txt size(128), instance("Test")
 */
ZTEST(ot_rpc_dns_client, test_otDnsServiceResponseGetServiceInfo)
{
	otDnsServiceResponseGetServiceInfo_fake.custom_fake = custom_get_service_info;

	service_info_txt_size = 8;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_UINT8(120), CBOR_UINT16(12345), 10,
				   20, CBOR_DNS_HOSTNAME, CBOR_IPV6_ADDR, CBOR_UINT8(120), 0x48,
				   INT_SEQUENCE(8), CBOR_FALSE, CBOR_UINT8(60)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_INFO,
				CBOR_UINT32(0xabcdabcd), CBOR_UINT8(128), CBOR_UINT8(128),
				CBOR_SERVICE_INSTANCE));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDnsServiceResponseGetServiceInfo_fake.call_count, 1);
}

ZTEST_SUITE(ot_rpc_dns_client, NULL, ts_setup, tc_setup, NULL, NULL);
