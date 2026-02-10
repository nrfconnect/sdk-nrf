/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <openthread/srp_client.h>

#define TEST_PORT               55555
#define TEST_TTL                120
#define TEST_LEASE_INTERVAL     UINT32_MAX
#define TEST_KEY_LEASE_INTERVAL 0xcafecafe
#define TEST_PRIORITY           10
#define TEST_WEIGHT             20

#define TEST_HOSTNAME "hostname.local"
#define CBOR_HOSTNAME 0x6E, 'h', 'o', 's', 't', 'n', 'a', 'm', 'e', '.', 'l', \
		      'o', 'c', 'a', 'l'

#define _CBOR_IPV6_START 0x50, 0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
			 0x00, 0x00, 0x00, 0x00

#define CBOR_IPV6_ADDR_1 _CBOR_IPV6_START, 0x01
#define CBOR_IPV6_ADDR_2 _CBOR_IPV6_START, 0x02
#define CBOR_IPV6_ADDR_3 _CBOR_IPV6_START, 0x03

#define CBOR_EMPTY_LIST 0x80
#define CBOR_EMPTY_MAP	0xa0
#define CBOR_MAP(...) 0xBF, __VA_ARGS__, 0xFF

#define TEST_SERVICE_NAME "_test._udp.local"
#define CBOR_SERVICE_NAME 0x70, '_', 't', 'e', 's', 't', '.', '_', 'u', 'd', 'p', '.', 'l', 'o', \
			  'c', 'a', 'l'

#define TEST_INSTANCE_NAME "TestInstance"
#define CBOR_INSTANCE_NAME 0x6C, 'T', 'e', 's', 't', 'I', 'n', 's', 't', 'a', 'n', 'c', 'e'

#define CBOR_NAMES_SIZE CBOR_UINT8(sizeof(TEST_SERVICE_NAME) + sizeof(TEST_INSTANCE_NAME))

#define CBOR_SERVICE_FIELDS CBOR_UINT16(TEST_PORT), CBOR_UINT8(TEST_PRIORITY),         \
			    CBOR_UINT8(TEST_WEIGHT), CBOR_UINT32(TEST_LEASE_INTERVAL), \
			    CBOR_UINT32(TEST_KEY_LEASE_INTERVAL)

/* _subX */
#define CBOR_SUBTYPE_1 0x63, 0x5F, 0x73, 0x31
#define CBOR_SUBTYPE_2 0x63, 0x5F, 0x73, 0x32
#define CBOR_SUBTYPE_3 0x63, 0x5F, 0x73, 0x33
#define CBOR_SUBTYPE_4 0x63, 0x5F, 0x73, 0x34

/* "keyX" */
#define CBOR_TXT_KEY_1 0x64, 0x6B, 0x65, 0x79, 0x31
#define CBOR_TXT_KEY_2 0x64, 0x6B, 0x65, 0x79, 0x32
#define CBOR_TXT_KEY_3 0x64, 0x6B, 0x65, 0x79, 0x33
#define CBOR_TXT_KEY_4 0x64, 0x6B, 0x65, 0x79, 0x34

#define TEST_TXT_VALUE_1 0xAB, 0xCD
#define TEST_TXT_VALUE_2 0xDE, 0xAD, 0xBE, 0xEF
#define TEST_TXT_VALUE_3 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
#define TEST_TXT_VALUE_4 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77

#define CBOR_TXT_VALUE_1 0x42, 0xAB, 0xCD
#define CBOR_TXT_VALUE_2 0x44, 0xDE, 0xAD, 0xBE, 0xEF
#define CBOR_TXT_VALUE_3 0x46, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
#define CBOR_TXT_VALUE_4 0x48, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77

#define CALC_PTRS_SIZES(subtypes, txt_entries) (txt_entries + subtypes  + 1) * sizeof(uintptr_t)

FAKE_VALUE_FUNC(otError, otSrpClientStart, otInstance *, const otSockAddr *);
FAKE_VOID_FUNC(otSrpClientStop, otInstance *);
FAKE_VALUE_FUNC(bool, otSrpClientIsRunning, otInstance *);
FAKE_VALUE_FUNC(const otSockAddr *, otSrpClientGetServerAddress, otInstance *);
FAKE_VOID_FUNC(otSrpClientSetCallback, otInstance *, otSrpClientCallback, void *);
FAKE_VOID_FUNC(otSrpClientEnableAutoStartMode, otInstance *, otSrpClientAutoStartCallback, void *);
FAKE_VOID_FUNC(otSrpClientDisableAutoStartMode, otInstance *);
FAKE_VALUE_FUNC(bool, otSrpClientIsAutoStartModeEnabled, otInstance *);
FAKE_VALUE_FUNC(uint32_t, otSrpClientGetTtl, otInstance *);
FAKE_VOID_FUNC(otSrpClientSetTtl, otInstance *, uint32_t);
FAKE_VALUE_FUNC(uint32_t, otSrpClientGetLeaseInterval, otInstance *);
FAKE_VOID_FUNC(otSrpClientSetLeaseInterval, otInstance *, uint32_t);
FAKE_VALUE_FUNC(uint32_t, otSrpClientGetKeyLeaseInterval, otInstance *);
FAKE_VOID_FUNC(otSrpClientSetKeyLeaseInterval, otInstance *, uint32_t);
FAKE_VALUE_FUNC(otError, otSrpClientSetHostName, otInstance *, const char *);
FAKE_VALUE_FUNC(otError, otSrpClientEnableAutoHostAddress, otInstance *);
FAKE_VALUE_FUNC(otError, otSrpClientSetHostAddresses, otInstance *, const otIp6Address *, uint8_t);
FAKE_VALUE_FUNC(otError, otSrpClientAddService, otInstance *, otSrpClientService *);
FAKE_VALUE_FUNC(otError, otSrpClientRemoveService, otInstance *, otSrpClientService *);
FAKE_VALUE_FUNC(otError, otSrpClientClearService, otInstance *, otSrpClientService *);
FAKE_VALUE_FUNC(otError, otSrpClientRemoveHostAndServices, otInstance *, bool, bool);
FAKE_VOID_FUNC(otSrpClientClearHostAndServices, otInstance *);
FAKE_VALUE_FUNC(const char *, otSrpClientGetDomainName, otInstance *);
FAKE_VALUE_FUNC(otError, otSrpClientSetDomainName, otInstance *, const char *);
FAKE_VOID_FUNC(otSrpClientSetServiceKeyRecordEnabled, otInstance *, bool);
FAKE_VALUE_FUNC(bool, otSrpClientIsServiceKeyRecordEnabled, otInstance *);

#define FOREACH_FAKE(f)                                                                            \
	f(otSrpClientStart)                                                                        \
	f(otSrpClientStop)                                                                         \
	f(otSrpClientIsRunning)                                                                    \
	f(otSrpClientGetServerAddress)                                                             \
	f(otSrpClientSetCallback)                                                                  \
	f(otSrpClientEnableAutoStartMode)                                                          \
	f(otSrpClientDisableAutoStartMode)                                                         \
	f(otSrpClientIsAutoStartModeEnabled)                                                       \
	f(otSrpClientGetTtl)                                                                       \
	f(otSrpClientSetTtl)                                                                       \
	f(otSrpClientGetLeaseInterval)                                                             \
	f(otSrpClientSetLeaseInterval)                                                             \
	f(otSrpClientGetKeyLeaseInterval)                                                          \
	f(otSrpClientSetKeyLeaseInterval)                                                          \
	f(otSrpClientSetHostName)                                                                  \
	f(otSrpClientEnableAutoHostAddress)                                                        \
	f(otSrpClientSetHostAddresses)                                                             \
	f(otSrpClientAddService)                                                                   \
	f(otSrpClientRemoveService)                                                                \
	f(otSrpClientClearService)                                                                 \
	f(otSrpClientRemoveHostAndServices)                                                        \
	f(otSrpClientClearHostAndServices)                                                         \
	f(otSrpClientGetDomainName)                                                                \
	f(otSrpClientSetDomainName)                                                                \
	f(otSrpClientSetServiceKeyRecordEnabled)                                                   \
	f(otSrpClientIsServiceKeyRecordEnabled)

extern size_t get_client_ids(uintptr_t *output, size_t max);

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();

	FOREACH_FAKE(RESET_FAKE);
	FFF_RESET_HISTORY();
}

static void clear_host_and_services(void)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_CLEAR_HOST_AND_SERVICES));

	mock_nrf_rpc_tr_expect_done();
}

static void tc_cleanup(void *f)
{
	clear_host_and_services();

	FOREACH_FAKE(RESET_FAKE);
	FFF_RESET_HISTORY();
}

otError verify_sockaddr(otInstance *instance, const otSockAddr *sockaddr)
{
	struct net_in6_addr addr = {{{ ADDR_1 }}};

	zassert_mem_equal(&addr, &sockaddr->mAddress, OT_IP6_ADDRESS_SIZE);
	zassert_equal(sockaddr->mPort, TEST_PORT);

	return OT_ERROR_NONE;
}

ZTEST(ot_rpc_srp_client, test_otSrpClientStart)
{
	otSrpClientStart_fake.custom_fake = verify_sockaddr;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_START, 0x50, ADDR_1,
					CBOR_UINT16(TEST_PORT)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientStart_fake.call_count, 1);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientStop)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_STOP));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientStop_fake.call_count, 1);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientIsRunning)
{
	otSrpClientIsRunning_fake.return_val = false;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_FALSE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_IS_RUNNING));

	otSrpClientIsRunning_fake.return_val = true;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_TRUE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_IS_RUNNING));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientIsRunning_fake.call_count, 2);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientGetServerAddress)
{
	const otSockAddr sockaddr = { .mAddress = {{{ ADDR_1 }}}, .mPort = TEST_PORT };

	otSrpClientGetServerAddress_fake.return_val = &sockaddr;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x50, ADDR_1, CBOR_UINT16(TEST_PORT)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_GET_SERVER_ADDRESS));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientGetServerAddress_fake.call_count, 1);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientSetCallback)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_SET_CALLBACK, CBOR_TRUE));

	zassert_not_null(otSrpClientSetCallback_fake.arg1_val);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_SET_CALLBACK, CBOR_FALSE));

	zassert_is_null(otSrpClientSetCallback_fake.arg1_val);

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientSetCallback_fake.call_count, 2);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientDisableAutoStartMode)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_DISABLE_AUTO_START_MODE));

	zassert_equal(otSrpClientDisableAutoStartMode_fake.call_count, 1);

	mock_nrf_rpc_tr_expect_done();
}

ZTEST(ot_rpc_srp_client, test_otSrpClientIsAutoStartModeEnabled)
{
	otSrpClientIsAutoStartModeEnabled_fake.return_val = false;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_FALSE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_IS_AUTO_START_MODE_ENABLED));

	otSrpClientIsAutoStartModeEnabled_fake.return_val = true;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_TRUE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_IS_AUTO_START_MODE_ENABLED));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientIsAutoStartModeEnabled_fake.call_count, 2);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientGetTtl)
{
	otSrpClientGetTtl_fake.return_val = TEST_TTL;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT8(TEST_TTL)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_GET_TTL));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientGetTtl_fake.call_count, 1);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientSetTtl)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_SET_TTL, CBOR_UINT8(TEST_TTL)));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientSetTtl_fake.call_count, 1);
	zassert_equal(otSrpClientSetTtl_fake.arg1_val, TEST_TTL);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientGetLeaseInterval)
{
	otSrpClientGetLeaseInterval_fake.return_val = TEST_LEASE_INTERVAL;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT32(TEST_LEASE_INTERVAL)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_GET_LEASE_INTERVAL));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientGetLeaseInterval_fake.call_count, 1);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientSetLeaseInterval)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_SET_LEASE_INTERVAL,
					CBOR_UINT32(TEST_LEASE_INTERVAL)));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientSetLeaseInterval_fake.call_count, 1);
	zassert_equal(otSrpClientSetLeaseInterval_fake.arg1_val, TEST_LEASE_INTERVAL);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientGetKeyLeaseInterval)
{
	otSrpClientGetKeyLeaseInterval_fake.return_val = TEST_KEY_LEASE_INTERVAL;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT32(TEST_KEY_LEASE_INTERVAL)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_GET_KEY_LEASE_INTERVAL));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientGetKeyLeaseInterval_fake.call_count, 1);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientSetKeyLeaseInterval)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_SET_KEY_LEASE_INTERVAL,
					CBOR_UINT32(TEST_KEY_LEASE_INTERVAL)));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientSetKeyLeaseInterval_fake.call_count, 1);
	zassert_equal(otSrpClientSetKeyLeaseInterval_fake.arg1_val, TEST_KEY_LEASE_INTERVAL);
}

otError verify_hostname(otInstance *instance, const char *hostname)
{
	ARG_UNUSED(instance);

	zassert_str_equal(TEST_HOSTNAME, hostname);

	return OT_ERROR_NONE;
}

ZTEST(ot_rpc_srp_client, test_otSrpClientSetHostName)
{
	otSrpClientSetHostName_fake.custom_fake = verify_hostname;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_SET_HOSTNAME, CBOR_HOSTNAME));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientSetHostName_fake.call_count, 1);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientEnableAutoHostAddress)
{
	otSrpClientEnableAutoHostAddress_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_ENABLE_AUTO_HOST_ADDR));

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otSrpClientEnableAutoHostAddress_fake.call_count, 1);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientSetHostAddresses)
{
	struct net_in6_addr addr1;
	struct net_in6_addr addr2;
	struct net_in6_addr addr3;

	const otIp6Address *addrs;

	net_ipv6_addr_create(&addr1, 0x2001, 0x0db8, 0, 0, 0, 0, 0, 0x1);
	net_ipv6_addr_create(&addr2, 0x2001, 0x0db8, 0, 0, 0, 0, 0, 0x2);
	net_ipv6_addr_create(&addr3, 0x2001, 0x0db8, 0, 0, 0, 0, 0, 0x3);

	otSrpClientSetHostAddresses_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_SET_HOST_ADDRESSES, CBOR_UINT8(1),
					CBOR_LIST(CBOR_IPV6_ADDR_1)));

	zassert_equal(otSrpClientSetHostAddresses_fake.arg2_val, 1);

	addrs = otSrpClientSetHostAddresses_fake.arg1_val;

	zassert_mem_equal(&addr1, &addrs[0], OT_IP6_ADDRESS_SIZE);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_SET_HOST_ADDRESSES, CBOR_UINT8(2),
					CBOR_LIST(CBOR_IPV6_ADDR_1, CBOR_IPV6_ADDR_2)));

	zassert_equal(otSrpClientSetHostAddresses_fake.arg2_val, 2);

	addrs = otSrpClientSetHostAddresses_fake.arg1_val;

	zassert_mem_equal(&addr1, &addrs[0], OT_IP6_ADDRESS_SIZE);
	zassert_mem_equal(&addr2, &addrs[1], OT_IP6_ADDRESS_SIZE);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_SET_HOST_ADDRESSES, CBOR_UINT8(3),
					CBOR_LIST(CBOR_IPV6_ADDR_1, CBOR_IPV6_ADDR_2,
						  CBOR_IPV6_ADDR_2)));

	zassert_equal(otSrpClientSetHostAddresses_fake.arg2_val, 3);

	addrs = otSrpClientSetHostAddresses_fake.arg1_val;

	zassert_mem_equal(&addr1, &addrs[0], OT_IP6_ADDRESS_SIZE);
	zassert_mem_equal(&addr2, &addrs[1], OT_IP6_ADDRESS_SIZE);
	zassert_mem_equal(&addr2, &addrs[2], OT_IP6_ADDRESS_SIZE);

	zassert_equal(otSrpClientSetHostAddresses_fake.call_count, 3);

	mock_nrf_rpc_tr_expect_done();
}

void verify_fields(otSrpClientService *service)
{
	zassert_equal(service->mPort, TEST_PORT);
	zassert_equal(service->mWeight, TEST_WEIGHT);
	zassert_equal(service->mPriority, TEST_PRIORITY);
	zassert_equal(service->mLease, TEST_LEASE_INTERVAL);
	zassert_equal(service->mKeyLease, TEST_KEY_LEASE_INTERVAL);
}

void verify_names(otSrpClientService *service, const char *service_name, const char *instance)
{
	zassert_str_equal(service->mName, service_name);
	zassert_str_equal(service->mInstanceName, instance);
}

void verify_subtypes(otSrpClientService *service, size_t num, const char * const exp_subtypes[])
{
	size_t n;

	for (n = 0; service->mSubTypeLabels[n]; ++n) {
		zassert_str_equal(service->mSubTypeLabels[n], exp_subtypes[n]);
	}

	zassert_equal(n, num);
}

void verify_txt(otSrpClientService *service, size_t num, const otDnsTxtEntry *entries)
{
	zassert_equal(service->mNumTxtEntries, num);

	for (int i = 0; i < num; ++i) {
		zassert_equal(!service->mTxtEntries[i].mKey, !entries[i].mKey);

		if (service->mTxtEntries[i].mKey) {
			zassert_str_equal(service->mTxtEntries[i].mKey, entries[i].mKey);
		}

		zassert_equal(service->mTxtEntries[i].mValueLength, entries[i].mValueLength);
		zassert_equal(!service->mTxtEntries[i].mValue, !entries[i].mValue);

		if (service->mTxtEntries[i].mValueLength) {
			zassert_mem_equal(service->mTxtEntries[i].mValue, entries[i].mValue,
					  service->mTxtEntries[i].mValueLength);
		}
	}
}

ZTEST(ot_rpc_srp_client, test_otSrpClientAddService_no_subtypes_no_txt)
{
	otSrpClientService *service;
	static const char * const subtypes[] = { NULL };

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, CBOR_UINT32(0xaaaabbbb),
					CBOR_UINT8(0), CBOR_UINT8(0), CBOR_NAMES_SIZE,
					CBOR_UINT8(0), CBOR_UINT8(0), CBOR_SERVICE_NAME,
					CBOR_INSTANCE_NAME, CBOR_EMPTY_LIST, CBOR_EMPTY_MAP,
					CBOR_SERVICE_FIELDS));
	mock_nrf_rpc_tr_expect_done();

	/* Memory is allocated on the heap so we can safely access the pointer */
	service = otSrpClientAddService_fake.arg1_val;

	zassert_equal(otSrpClientAddService_fake.call_count, 1);

	verify_fields(service);
	verify_names(service, TEST_SERVICE_NAME, TEST_INSTANCE_NAME);
	verify_subtypes(service, 0, subtypes);
	verify_txt(service, 0, NULL);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientAddService_single_subtype_no_txt)
{
	otSrpClientService *service;
	static const char * const subtypes[] = { "_s1", NULL };

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, CBOR_UINT32(0xaaaabbbb),
					CBOR_UINT8(1), CBOR_UINT8(0), CBOR_NAMES_SIZE,
					CBOR_UINT8(sizeof("_s1")), CBOR_UINT8(0), CBOR_SERVICE_NAME,
					CBOR_INSTANCE_NAME, CBOR_LIST(CBOR_SUBTYPE_1),
					CBOR_EMPTY_MAP, CBOR_SERVICE_FIELDS));
	mock_nrf_rpc_tr_expect_done();

	/* Memory is allocated on the heap so we can safely access the pointer */
	service = otSrpClientAddService_fake.arg1_val;

	zassert_equal(otSrpClientAddService_fake.call_count, 1);

	verify_fields(service);
	verify_names(service, TEST_SERVICE_NAME, TEST_INSTANCE_NAME);
	verify_subtypes(service, 1, subtypes);
	verify_txt(service, 0, NULL);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientAddService_multi_subtypes_no_txt)
{
	otSrpClientService *service;
	static const char * const subtypes[] = { "_s1", "_s2", "_s3", "_s4", NULL };

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, CBOR_UINT32(0xaaaabbbb),
					CBOR_UINT8(4), CBOR_UINT8(0), CBOR_NAMES_SIZE,
					CBOR_UINT8(sizeof("_s1") * 4), CBOR_UINT8(0),
					CBOR_SERVICE_NAME, CBOR_INSTANCE_NAME,
					CBOR_LIST(CBOR_SUBTYPE_1, CBOR_SUBTYPE_2, CBOR_SUBTYPE_3,
						  CBOR_SUBTYPE_4), CBOR_EMPTY_MAP,
					CBOR_SERVICE_FIELDS));
	mock_nrf_rpc_tr_expect_done();

	/* Memory is allocated on the heap so we can safely access the pointer */
	service = otSrpClientAddService_fake.arg1_val;

	zassert_equal(otSrpClientAddService_fake.call_count, 1);

	verify_fields(service);
	verify_names(service, TEST_SERVICE_NAME, TEST_INSTANCE_NAME);
	verify_subtypes(service, 4, subtypes);
	verify_txt(service, 0, NULL);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientAddService_no_subtypes_single_txt_key_only)
{
	otSrpClientService *service;
	static const char * const subtypes[] = { NULL };

	const otDnsTxtEntry entries[] = { { .mKey = "key1", .mValue = NULL, .mValueLength = 0 } };

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, CBOR_UINT32(0xaaaabbbb),
					CBOR_UINT8(0), CBOR_UINT8(1), CBOR_NAMES_SIZE,
					CBOR_UINT8(0), CBOR_UINT8(sizeof("key1")),
					CBOR_SERVICE_NAME, CBOR_INSTANCE_NAME, CBOR_EMPTY_LIST,
					CBOR_MAP(CBOR_TXT_KEY_1, CBOR_NULL), CBOR_SERVICE_FIELDS));
	mock_nrf_rpc_tr_expect_done();

	/* Memory is allocated on the heap so we can safely access the pointer */
	service = otSrpClientAddService_fake.arg1_val;

	zassert_equal(otSrpClientAddService_fake.call_count, 1);

	verify_fields(service);
	verify_names(service, TEST_SERVICE_NAME, TEST_INSTANCE_NAME);
	verify_subtypes(service, 0, subtypes);
	verify_txt(service, 1, entries);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientAddService_no_subtypes_single_txt_value_only)
{
	otSrpClientService *service;
	static const char * const subtypes[] = { NULL };
	uint8_t values[] = { 0xAB, 0xCD };

	otDnsTxtEntry entries[] = { { .mKey = NULL, .mValue = values,
				      .mValueLength = sizeof(values) } };

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, CBOR_UINT32(0xaaaabbbb),
					CBOR_UINT8(0), CBOR_UINT8(1), CBOR_NAMES_SIZE,
					CBOR_UINT8(0), CBOR_UINT8(sizeof(values)),
					CBOR_SERVICE_NAME, CBOR_INSTANCE_NAME, CBOR_EMPTY_LIST,
					CBOR_MAP(CBOR_NULL, CBOR_TXT_VALUE_1),
					CBOR_SERVICE_FIELDS));
	mock_nrf_rpc_tr_expect_done();

	/* Memory is allocated on the heap so we can safely access the pointer */
	service = otSrpClientAddService_fake.arg1_val;

	zassert_equal(otSrpClientAddService_fake.call_count, 1);

	verify_fields(service);
	verify_names(service, TEST_SERVICE_NAME, TEST_INSTANCE_NAME);
	verify_subtypes(service, 0, subtypes);
	verify_txt(service, 1, entries);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientAddService_no_subtypes_single_txt_key_value)
{
	otSrpClientService *service;
	static const char * const subtypes[] = { NULL };
	uint8_t values[] = { TEST_TXT_VALUE_1 };

	otDnsTxtEntry entries[] = { { .mKey = "key1", .mValue = values,
				      .mValueLength = sizeof(values) } };

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, CBOR_UINT32(0xaaaabbbb),
					CBOR_UINT8(0), CBOR_UINT8(1), CBOR_NAMES_SIZE,
					CBOR_UINT8(0), CBOR_UINT8(sizeof("key1") + sizeof(values)),
					CBOR_SERVICE_NAME, CBOR_INSTANCE_NAME, CBOR_EMPTY_LIST,
					CBOR_MAP(CBOR_TXT_KEY_1, CBOR_TXT_VALUE_1),
					CBOR_SERVICE_FIELDS));
	mock_nrf_rpc_tr_expect_done();

	/* Memory is allocated on the heap so we can safely access the pointer */
	service = otSrpClientAddService_fake.arg1_val;

	zassert_equal(otSrpClientAddService_fake.call_count, 1);

	verify_fields(service);
	verify_names(service, TEST_SERVICE_NAME, TEST_INSTANCE_NAME);
	verify_subtypes(service, 0, subtypes);
	verify_txt(service, 1, entries);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientAddService_no_subtypes_multi_txt_key_value)
{
	otSrpClientService *service;
	static const char * const subtypes[] = { NULL };
	uint8_t values1[] = { TEST_TXT_VALUE_1 };
	uint8_t values2[] = { TEST_TXT_VALUE_2 };
	uint8_t values3[] = { TEST_TXT_VALUE_3 };
	uint8_t values4[] = { TEST_TXT_VALUE_4 };

	otDnsTxtEntry entries[] = { { .mKey = "key1", .mValue = values1,
				      .mValueLength = sizeof(values1) },
				    { .mKey = "key2", .mValue = values2,
				      .mValueLength = sizeof(values2) },
				    { .mKey = "key3", .mValue = values3,
				      .mValueLength = sizeof(values3) },
				    { .mKey = "key4", .mValue = values4,
				      .mValueLength = sizeof(values4) } };

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, CBOR_UINT32(0xaaaabbbb),
					CBOR_UINT8(0), CBOR_UINT8(4), CBOR_NAMES_SIZE,
					CBOR_UINT8(0), CBOR_UINT8(sizeof("key1") * 4 +
								  sizeof(values1) +
								  sizeof(values2) +
								  sizeof(values3) +
								  sizeof(values4)),
					CBOR_SERVICE_NAME, CBOR_INSTANCE_NAME, CBOR_EMPTY_LIST,
					CBOR_MAP(CBOR_TXT_KEY_1, CBOR_TXT_VALUE_1, CBOR_TXT_KEY_2,
						 CBOR_TXT_VALUE_2, CBOR_TXT_KEY_3, CBOR_TXT_VALUE_3,
						 CBOR_TXT_KEY_4, CBOR_TXT_VALUE_4),
					CBOR_SERVICE_FIELDS));
	mock_nrf_rpc_tr_expect_done();

	/* Memory is allocated on the heap so we can safely access the pointer */
	service = otSrpClientAddService_fake.arg1_val;

	zassert_equal(otSrpClientAddService_fake.call_count, 1);

	verify_fields(service);
	verify_names(service, TEST_SERVICE_NAME, TEST_INSTANCE_NAME);
	verify_subtypes(service, 0, subtypes);
	verify_txt(service, 4, entries);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientAddService_single_subtype_single_txt_key_value)
{
	otSrpClientService *service;
	static const char * const subtypes[] = { "_s1", NULL };
	uint8_t values[] = { TEST_TXT_VALUE_1 };

	otDnsTxtEntry entries[] = { { .mKey = "key1", .mValue = values,
				      .mValueLength = sizeof(values) } };

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, CBOR_UINT32(0xaaaabbbb),
					CBOR_UINT8(1), CBOR_UINT8(1), CBOR_NAMES_SIZE,
					CBOR_UINT8(sizeof("_s1")), CBOR_UINT8(sizeof("key1") +
									      sizeof(values)),
					CBOR_SERVICE_NAME, CBOR_INSTANCE_NAME,
					CBOR_LIST(CBOR_SUBTYPE_1), CBOR_MAP(CBOR_TXT_KEY_1,
									    CBOR_TXT_VALUE_1),
					CBOR_SERVICE_FIELDS));
	mock_nrf_rpc_tr_expect_done();

	/* Memory is allocated on the heap so we can safely access the pointer */
	service = otSrpClientAddService_fake.arg1_val;

	zassert_equal(otSrpClientAddService_fake.call_count, 1);

	verify_fields(service);
	verify_names(service, TEST_SERVICE_NAME, TEST_INSTANCE_NAME);
	verify_subtypes(service, 1, subtypes);
	verify_txt(service, 1, entries);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientAddService_multi_subtype_multi_txt_mixed)
{
	otSrpClientService *service;
	static const char * const subtypes[] = { "_s1", "_s2", "_s3", NULL };
	uint8_t values2[] = { TEST_TXT_VALUE_2 };
	uint8_t values3[] = { TEST_TXT_VALUE_3 };

	otDnsTxtEntry entries[] = { { .mKey = "key1", .mValue = NULL,
				      .mValueLength = 0 },
				    { .mKey = NULL, .mValue = values2,
				      .mValueLength = sizeof(values2) },
				    { .mKey = "key3", .mValue = values3,
				      .mValueLength = sizeof(values3) } };

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, CBOR_UINT32(0xaaaabbbb),
					CBOR_UINT8(3), CBOR_UINT8(3), CBOR_NAMES_SIZE,
					CBOR_UINT8(sizeof("_s1") * 3),
					CBOR_UINT8(sizeof("key1") * 2 + sizeof(values2) +
						   sizeof(values3)),
					CBOR_SERVICE_NAME, CBOR_INSTANCE_NAME,
					CBOR_LIST(CBOR_SUBTYPE_1, CBOR_SUBTYPE_2,
						  CBOR_SUBTYPE_3), CBOR_MAP(CBOR_TXT_KEY_1,
									    CBOR_NULL, CBOR_NULL,
									    CBOR_TXT_VALUE_2,
									    CBOR_TXT_KEY_3,
									    CBOR_TXT_VALUE_3),
					CBOR_SERVICE_FIELDS));
	mock_nrf_rpc_tr_expect_done();

	/* Memory is allocated on the heap so we can safely access the pointer */
	service = otSrpClientAddService_fake.arg1_val;

	zassert_equal(otSrpClientAddService_fake.call_count, 1);

	verify_fields(service);
	verify_names(service, TEST_SERVICE_NAME, TEST_INSTANCE_NAME);
	verify_subtypes(service, 3, subtypes);
	verify_txt(service, 3, entries);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientRemoveService)
{
	uintptr_t ids[4] = { 0 };
	size_t max_ids = sizeof(ids) / sizeof(uintptr_t);

	zassert_equal(get_client_ids(ids, max_ids), 0);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, CBOR_UINT32(0xaaaabbbb),
					CBOR_UINT8(0), CBOR_UINT8(0), CBOR_NAMES_SIZE,
					CBOR_UINT8(0), CBOR_UINT8(0), CBOR_SERVICE_NAME,
					CBOR_INSTANCE_NAME, CBOR_EMPTY_LIST, CBOR_EMPTY_MAP,
					CBOR_SERVICE_FIELDS));

	zassert_equal(get_client_ids(ids, max_ids), 1);
	zassert_equal(ids[0], 0xaaaabbbb);

	/* Try to remove invalid service */
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NOT_FOUND), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_REMOVE_SERVICE,
					CBOR_UINT32(0xbeefbeef)));

	otSrpClientRemoveService_fake.return_val = OT_ERROR_NONE;

	/* Remove valid service */
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_REMOVE_SERVICE,
					CBOR_UINT32(0xaaaabbbb)));

	zassert_equal(otSrpClientRemoveService_fake.call_count, 1);

	mock_nrf_rpc_tr_expect_done();

	/* Verify that successful request does not remove the service until callback is called */
	zassert_equal(get_client_ids(ids, max_ids), 1);
	zassert_equal(ids[0], 0xaaaabbbb);
}

ZTEST(ot_rpc_srp_client, test_otSrpClientClearService)
{
	uintptr_t ids[4] = { 0 };
	size_t max_ids = sizeof(ids) / sizeof(uintptr_t);

	zassert_equal(get_client_ids(ids, max_ids), 0);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, CBOR_UINT32(0xaaaabbbb),
					CBOR_UINT8(0), CBOR_UINT8(0), CBOR_NAMES_SIZE,
					CBOR_UINT8(0), CBOR_UINT8(0), CBOR_SERVICE_NAME,
					CBOR_INSTANCE_NAME, CBOR_EMPTY_LIST, CBOR_EMPTY_MAP,
					CBOR_SERVICE_FIELDS));

	zassert_equal(get_client_ids(ids, max_ids), 1);
	zassert_equal(ids[0], 0xaaaabbbb);

	/* Try to remove invalid service */
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NOT_FOUND), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_CLEAR_SERVICE,
					CBOR_UINT32(0xbeefbeef)));

	otSrpClientClearService_fake.return_val = OT_ERROR_NONE;

	/* Remove valid service */
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_SRP_CLIENT_CLEAR_SERVICE,
					CBOR_UINT32(0xaaaabbbb)));

	zassert_equal(otSrpClientClearService_fake.call_count, 1);

	mock_nrf_rpc_tr_expect_done();

	/* Verify that clearing does remove service immediately */
	memset(ids, 0, sizeof(ids));
	zassert_equal(get_client_ids(ids, max_ids), 0);
	zassert_equal(ids[0], 0);
}

ZTEST_SUITE(ot_rpc_srp_client, NULL, NULL, tc_setup, tc_cleanup, NULL);
