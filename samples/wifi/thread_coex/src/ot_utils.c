/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ot_utils.h"
#include "zperf_utils.h"

#include "openthread/ping_sender.h"
#include <zephyr/logging/log.h>
#include <openthread/thread.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

LOG_MODULE_REGISTER(ot_utils, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
#include <zephyr/types.h>

#define WAIT_TIME_FOR_OT_CON K_SECONDS(10)

typedef struct peer_address_info {
	char address_string[OT_IP6_ADDRESS_STRING_SIZE];
	bool address_found;
} peer_address_info_t;

peer_address_info_t peer_address_info  = {.address_found = false};

extern uint8_t ot_wait4_ping_reply_from_peer;
extern bool is_ot_device_role_client;

static K_SEM_DEFINE(connected_sem, 0, 1);

static void ot_device_dettached(void *ptr)
{
	(void) ptr;

	LOG_INF("\nOT device dettached gracefully\n");
}

static void ot_commissioner_state_changed(otCommissionerState aState, void *aContext)
{
	LOG_INF("OT commissioner state changed");
	if (aState == OT_COMMISSIONER_STATE_ACTIVE) {
		LOG_INF("ot commissioner joiner add * FEDCBA9876543210 2000");
		otCommissionerAddJoiner(openthread_get_default_instance(), NULL,
		"FEDCBA9876543210", 2000);
		LOG_INF("\n\nRun thread application on client\n\n");
	}
}

static void ot_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
				    void *user_data)
{
	LOG_INF("OT device state changed");
	if (flags & OT_CHANGED_THREAD_ROLE) {
		otDeviceRole ot_role = otThreadGetDeviceRole(ot_context->instance);

		if (ot_role != OT_DEVICE_ROLE_DETACHED && ot_role != OT_DEVICE_ROLE_DISABLED) {
			/* ot commissioner start */
			LOG_INF("ot commissioner start");
			otCommissionerStart(ot_context->instance, &ot_commissioner_state_changed,
			NULL, NULL);
		}
	}
}

static struct openthread_state_changed_cb ot_state_chaged_cb = {
	.state_changed_cb = ot_thread_state_changed
};

/* call back Thread device joiner */
static void ot_joiner_start_handler(otError error, void *context)
{
	switch (error) {

	case OT_ERROR_NONE:
		LOG_INF("Thread Join success");
		k_sem_give(&connected_sem);
	break;

	default:
		LOG_ERR("Thread join failed [%s]", otThreadErrorToString(error));
	break;
	}
}

int ot_throughput_client_init(void)
{
	otError err = 0;

	ot_start_joiner("FEDCBA9876543210");
	/* k_sleep(K_SECONDS(2)); */
	/* Note: current value of WAIT_TIME_FOR_OT_CON = 4sec.
	 * Sometimes, starting openthread happening before
	 * the thread join failed.So, increase it to 10sec.
	 */
	err = k_sem_take(&connected_sem, WAIT_TIME_FOR_OT_CON);

	LOG_INF("Starting openthread.");
	openthread_api_mutex_lock(openthread_get_default_context());
	/*  ot thread start */
	err = otThreadSetEnabled(openthread_get_default_instance(), true);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Starting openthread: %d (%s)", err, otThreadErrorToString(err));
	}

	otDeviceRole current_role =
		otThreadGetDeviceRole(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	while (current_role != OT_DEVICE_ROLE_CHILD) {
		LOG_INF("Current role of Thread device: %s",
			otThreadDeviceRoleToString(current_role));
		k_sleep(K_MSEC(1000));
		openthread_api_mutex_lock(openthread_get_default_context());
		current_role = otThreadGetDeviceRole(openthread_get_default_instance());
		openthread_api_mutex_unlock(openthread_get_default_context());
	}

	ot_get_peer_address(5000);
	if (!peer_address_info.address_found) {
		LOG_WRN("Peer address not found. Not continuing with zperf test.");
		return -1;
	}
	return 0;
}

int ot_throughput_test_run(bool is_ot_zperf_udp)
{
	if (is_ot_device_role_client) {
		ot_zperf_test(is_ot_zperf_udp);
		/* Only for client case. Server case is handled as part of init */
	}
	return 0;
}

void ot_start_joiner(const char *pskd)
{
	LOG_INF("Starting joiner");

	otInstance *instance = openthread_get_default_instance();
	struct openthread_context *context = openthread_get_default_context();

	openthread_api_mutex_lock(context);

	/** Step1: Set null network key i.e,
	 * ot networkkey 00000000000000000000000000000000
	 */
	/* ot_setNullNetworkKey(instance); */

	/** Step2: Bring up the interface and start joining to the network
	 * on DK2 with pre-shared key.
	 * i.e. ot ifconfig up
	 * ot joiner start FEDCBA9876543210
	 */
	otIp6SetEnabled(instance, true); /* ot ifconfig up */
	otJoinerStart(instance, pskd, NULL,
				"Zephyr", "Zephyr",
				KERNEL_VERSION_STRING, NULL,
				&ot_joiner_start_handler, NULL);
	openthread_api_mutex_unlock(context);
	/* LOG_INF("Thread start joiner Done."); */
}

int ot_throughput_test_init(bool is_ot_client, bool is_ot_zperf_udp)
{
	int ret = 0;

	if (is_ot_client) {
		ret = ot_throughput_client_init();
		if (ret != 0) {
			LOG_ERR("Thread throughput client init failed: %d", ret);
			return ret;
		}
	}
	if (!is_ot_client) { /* for server */
		ot_initialization();
		openthread_state_changed_cb_register(openthread_get_default_context(),
		&ot_state_chaged_cb);

		LOG_INF("Starting zperf server");
		ot_start_zperf_test_recv(is_ot_zperf_udp);
	}
	return 0;
}

int ot_tput_test_exit(void)
{
	otInstance *instance = openthread_get_default_instance();
	struct openthread_context *context = openthread_get_default_context();

	otThreadDetachGracefully(instance, ot_device_dettached, context);
	k_sleep(K_MSEC(1000));

	return 0;
}

void ot_setNullNetworkKey(otInstance *aInstance)
{
	otOperationalDataset aDataset;
	uint8_t key[OT_NETWORK_KEY_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	/* memset(&aDataset, 0, sizeof(otOperationalDataset)); */ /* client */
	otDatasetCreateNewNetwork(aInstance, &aDataset); /* server */


	/* Set network key to null */
	memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
	aDataset.mComponents.mIsNetworkKeyPresent = true;
	otDatasetSetActive(aInstance, &aDataset);
}

void ot_setNetworkConfiguration(otInstance *aInstance)
{
	static const char aNetworkName[] = "TestNetwork";
	otOperationalDataset aDataset;
	size_t length;
	uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x11, 0x11, 0x11, 0x11,	0x11, 0x11, 0x11, 0x11};
	uint8_t key[OT_NETWORK_KEY_SIZE] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
		0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

	/* memset(&aDataset, 0, sizeof(otOperationalDataset)); */
	otDatasetCreateNewNetwork(aInstance, &aDataset);

	/**
	 * Fields that can be configured in otOperationDataset to override defaults:
	 *     Network Name, Mesh Local Prefix, Extended PAN ID, PAN ID, Delay Timer,
	 *     Channel, Channel Mask Page 0, Network Key, PSKc, Security Policy
	 */

	aDataset.mActiveTimestamp.mSeconds = 1;
	aDataset.mActiveTimestamp.mTicks = 0;
	aDataset.mActiveTimestamp.mAuthoritative = false;
	aDataset.mComponents.mIsActiveTimestampPresent = true;

	/* Set Channel */
	aDataset.mChannel = CONFIG_OT_CHANNEL;
	aDataset.mComponents.mIsChannelPresent = true;

	/* Set Pan ID */
	aDataset.mPanId = (otPanId)CONFIG_OT_PAN_ID;
	aDataset.mComponents.mIsPanIdPresent = true;

	/* Set Extended Pan ID */
	memcpy(aDataset.mExtendedPanId.m8, extPanId, sizeof(aDataset.mExtendedPanId));
	aDataset.mComponents.mIsExtendedPanIdPresent = true;

	/* Set network key */
	memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
	aDataset.mComponents.mIsNetworkKeyPresent = true;

	/* Set Network Name */
	length = strlen(aNetworkName);
	assert(length <= OT_NETWORK_NAME_MAX_SIZE);
	memcpy(aDataset.mNetworkName.m8, aNetworkName, length);
	aDataset.mComponents.mIsNetworkNamePresent = true;
	otDatasetSetActive(aInstance, &aDataset);
}

int ot_initialization(void)
{
	struct openthread_context *context = openthread_get_default_context();

	otInstance *instance = openthread_get_default_instance();

	/* LOG_INF("Updating thread parameters"); */
	ot_setNetworkConfiguration(instance);
	/* LOG_INF("Enabling thread"); */
	otError err = openthread_start(context); /* 'ifconfig up && thread start' */

	if (err != OT_ERROR_NONE) {
		LOG_ERR("Starting openthread: %d (%s)", err, otThreadErrorToString(err));
	}
	otDeviceRole current_role = otThreadGetDeviceRole(instance);

	LOG_INF("Current role of Thread device: %s", otThreadDeviceRoleToString(current_role));
	return 0;
}

void ot_handle_ping_reply(const otPingSenderReply *reply, void *context)
{
	otIp6Address add = reply->mSenderAddress;
	char string[OT_IP6_ADDRESS_STRING_SIZE];

	otIp6AddressToString(&add, string, OT_IP6_ADDRESS_STRING_SIZE);
	LOG_WRN("Reply received from: %s\n", string);

	ot_wait4_ping_reply_from_peer = 1;

	if (!peer_address_info.address_found) {
		strcpy(peer_address_info.address_string, string);
		peer_address_info.address_found = true;
	}
}

void ot_get_peer_address(uint64_t timeout_ms)
{
	const char *dest = "ff03::1"; /* Mesh-Local anycast for all FTDs and MEDs */
	uint64_t start_time;
	otPingSenderConfig config;

	/* LOG_INF("Finding other devices..."); */
	memset(&config, 0, sizeof(config));
	config.mReplyCallback = ot_handle_ping_reply;

	openthread_api_mutex_lock(openthread_get_default_context());
	otIp6AddressFromString(dest, &config.mDestination);
	otPingSenderPing(openthread_get_default_instance(), &config);
	openthread_api_mutex_unlock(openthread_get_default_context());

	start_time = k_uptime_get();
	while (!peer_address_info.address_found && k_uptime_get() < start_time + timeout_ms) {
		k_sleep(K_MSEC(100));
	}
}

void ot_start_zperf_test_send(const char *peer_addr, uint32_t duration_sec,
	uint32_t packet_size_bytes,	uint32_t rate_bps, bool is_ot_zperf_udp)
{
	zperf_upload(peer_addr, duration_sec, packet_size_bytes, rate_bps, is_ot_zperf_udp);
}


void ot_start_zperf_test_recv(bool is_ot_zperf_udp)
{
	zperf_download(is_ot_zperf_udp);
}

void ot_zperf_test(bool is_ot_zperf_udp)
{
	if (is_ot_device_role_client) {
		uint32_t ot_zperf_duration_sec = CONFIG_COEX_TEST_DURATION/1000;

		ot_start_zperf_test_send(peer_address_info.address_string, ot_zperf_duration_sec,
		CONFIG_OT_PACKET_SIZE, CONFIG_OT_RATE_BPS, is_ot_zperf_udp);
	}
	/* Note: ot_start_zperf_test_recv() done as part of init */
}
