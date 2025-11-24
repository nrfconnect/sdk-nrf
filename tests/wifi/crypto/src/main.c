/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/devicetree.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <psa/crypto.h>
#include "wifi_keys.h"

#include "ipc_if.h"

LOG_MODULE_REGISTER(wifi_crypto_test, LOG_LEVEL_DBG);

/* Define keyslot IDs used by wpa_supplicant for WPA2 and WPA3
 * Note: PMK/GMK remain with wpa_supplicant, only PTK/GTK are
 * installed to CRACEN KMU and sent to nRF71
 */
#define WPA2_GTK_KEYSLOT_ID		200
#define WPA3_GTK_KEYSLOT_ID		201
#define WPA2_PTK_KEYSLOT_ID		202
#define WPA3_PTK_KEYSLOT_ID		203

/* Key size definitions */
#define WPA2_GTK_SIZE		32
#define WPA3_GTK_SIZE		32
#define WPA2_PTK_SIZE		48
#define WPA3_PTK_SIZE		48

/* Placeholder command structure for IPC */
struct wifi_crypto_cmd {
	uint32_t cmd_type;
	uint32_t keyslot_id;
	uint32_t key_size;
	uint8_t key_data[64];
} __packed;

/* Dummy key data for testing
 * Note: PMK/GMK are not used here as they remain with wpa_supplicant
 */
static uint8_t wpa2_gtk[WPA2_GTK_SIZE];
static uint8_t wpa3_gtk[WPA3_GTK_SIZE];
static uint8_t wpa2_ptk[WPA2_PTK_SIZE];
static uint8_t wpa3_ptk[WPA3_PTK_SIZE];

/* Minimal IPC implementation for test */
static struct ipc_ept ipc_ep;
static const struct device *ipc_dev;
static int (*ipc_rx_handler)(void *priv);
static void *ipc_rx_priv;
static bool ipc_initialized;

static void ipc_bound(void *priv)
{
	ARG_UNUSED(priv);
	LOG_DBG("IPC endpoint bound");
}

static void ipc_received(const void *data, size_t len, void *priv)
{
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	ARG_UNUSED(priv);
	LOG_DBG("IPC data received");
	if (ipc_rx_handler) {
		ipc_rx_handler(ipc_rx_priv);
	}
}

static int ipc_rx_callback(void *priv)
{
	ARG_UNUSED(priv);
	LOG_DBG("IPC RX callback received");
	return 0;
}

/**
 * @brief Generate dummy key data for testing
 *
 * @param key Buffer to store generated key
 * @param key_size Size of the key buffer
 */
static void generate_dummy_key(uint8_t *key, size_t key_size)
{
	/* Generate dummy key data */
	for (size_t i = 0; i < key_size; i++) {
		key[i] = (uint8_t)(i & 0xFF);
	}
}

/**
 * @brief Install key to CRACEN KMU
 *
 * @param keyslot_id KMU keyslot ID
 * @param key Key data to install
 * @param key_size Size of the key
 *
 * @return 0 on success, negative error code on failure
 */
static int install_key_to_kmu(uint32_t keyslot_id, const uint8_t *key, size_t key_size)
{
	/* TODO: Implement PSA API to install key to CRACEN KMU
	 *       PSA API is not available yet, so this is a placeholder
	 */
	ARG_UNUSED(keyslot_id);
	ARG_UNUSED(key);
	ARG_UNUSED(key_size);

	LOG_INF("TODO: Install key to KMU keyslot %u (size: %zu)", keyslot_id, key_size);
	return 0;
}

/**
 * @brief Minimal IPC init implementation
 *
 * @return 0 on success, negative error code on failure
 */
int ipc_init(void)
{
	int ret;
	struct ipc_ept_cfg ep_cfg = {
		.name = "wifi_crypto",
		.cb = {
			.bound = ipc_bound,
			.received = ipc_received,
		},
	};

	if (ipc_initialized) {
		return 0;
	}

	ipc_dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	if (!device_is_ready(ipc_dev)) {
		LOG_ERR("IPC device not ready");
		return -ENODEV;
	}

	ret = ipc_service_open_instance(ipc_dev);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to open IPC instance: %d", ret);
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc_dev, &ipc_ep, &ep_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to register IPC endpoint: %d", ret);
		return ret;
	}

	ipc_initialized = true;
	LOG_INF("IPC initialized");
	return 0;
}

/**
 * @brief Minimal IPC deinit implementation
 *
 * @return 0 on success, negative error code on failure
 */
int ipc_deinit(void)
{
	ipc_initialized = false;
	ipc_rx_handler = NULL;
	ipc_rx_priv = NULL;
	LOG_INF("IPC deinitialized");
	return 0;
}

/**
 * @brief Minimal IPC send implementation
 *
 * @param ctx IPC context
 * @param data Data to send
 * @param len Length of data
 *
 * @return 0 on success, negative error code on failure
 */
int ipc_send(ipc_ctx_t ctx, const void *data, int len)
{
	int ret;

	ARG_UNUSED(ctx.ept);

	if (!ipc_initialized) {
		LOG_ERR("IPC not initialized");
		return -ENODEV;
	}

	if (ctx.inst != IPC_INSTANCE_CMD_CTRL) {
		LOG_ERR("Unsupported IPC instance: %d", ctx.inst);
		return -EINVAL;
	}

	ret = ipc_service_send(&ipc_ep, data, len);
	if (ret < 0) {
		LOG_ERR("Failed to send IPC data: %d", ret);
		return ret;
	}

	LOG_DBG("IPC send completed: %d bytes", len);
	return 0;
}

/**
 * @brief Minimal IPC receive implementation
 *
 * @param ctx IPC context
 * @param data Buffer to receive data
 * @param len Length of buffer
 *
 * @return 0 on success, negative error code on failure
 */
int ipc_recv(ipc_ctx_t ctx, void *data, int len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	/* TODO: Implement IPC receive */
	LOG_ERR("IPC receive not implemented");
	return 0;
}

/**
 * @brief Minimal IPC register RX callback implementation
 *
 * @param rx_handler RX callback function
 * @param data Private data for callback
 *
 * @return 0 on success, negative error code on failure
 */
int ipc_register_rx_cb(int (*rx_handler)(void *priv), void *data)
{
	ipc_rx_handler = rx_handler;
	ipc_rx_priv = data;
	LOG_DBG("IPC RX callback registered");
	return 0;
}

/**
 * @brief Send IPC command to nRF71
 *
 * @param keyslot_id Keyslot ID
 * @param key Key data
 * @param key_size Size of the key
 *
 * @return 0 on success, negative error code on failure
 */
static int send_ipc_crypto_cmd(uint32_t keyslot_id, const uint8_t *key, size_t key_size)
{
	struct wifi_crypto_cmd cmd;
	ipc_ctx_t ctx;
	int ret;

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd_type = 0; /* TODO: Define actual command type */
	cmd.keyslot_id = keyslot_id;
	cmd.key_size = key_size;

	if (key_size > sizeof(cmd.key_data)) {
		LOG_ERR("Key size too large: %zu", key_size);
		return -EINVAL;
	}

	memcpy(cmd.key_data, key, key_size);

	ctx.inst = IPC_INSTANCE_CMD_CTRL;
	ctx.ept = IPC_EPT_UMAC;

	ret = ipc_send(ctx, &cmd, sizeof(cmd));
	if (ret < 0) {
		LOG_ERR("Failed to send IPC crypto command: %d", ret);
		return ret;
	}

	LOG_INF("IPC crypto command sent successfully (keyslot: %u)", keyslot_id);
	return 0;
}
#if 0
/**
 * @brief Test WPA2 GTK key installation and IPC command
 */
ZTEST(wifi_crypto, test_wpa2_gtk)
{
	int ret;

	LOG_INF("Testing WPA2 GTK key");

	/* Generate dummy WPA2 GTK */
	generate_dummy_key(wpa2_gtk, sizeof(wpa2_gtk));

	/* Install key to KMU */
	ret = install_key_to_kmu(WPA2_GTK_KEYSLOT_ID, wpa2_gtk, sizeof(wpa2_gtk));
	zassert_equal(ret, 0, "Failed to install WPA2 GTK to KMU");

	/* Send IPC command */
	ret = send_ipc_crypto_cmd(WPA2_GTK_KEYSLOT_ID, wpa2_gtk, sizeof(wpa2_gtk));
	zassert_equal(ret, 0, "Failed to send IPC command for WPA2 GTK");
}

/**
 * @brief Test WPA3 GTK key installation and IPC command
 */
ZTEST(wifi_crypto, test_wpa3_gtk)
{
	int ret;

	LOG_INF("Testing WPA3 GTK key");

	/* Generate dummy WPA3 GTK */
	generate_dummy_key(wpa3_gtk, sizeof(wpa3_gtk));

	/* Install key to KMU */
	ret = install_key_to_kmu(WPA3_GTK_KEYSLOT_ID, wpa3_gtk, sizeof(wpa3_gtk));
	zassert_equal(ret, 0, "Failed to install WPA3 GTK to KMU");

	/* Send IPC command */
	ret = send_ipc_crypto_cmd(WPA3_GTK_KEYSLOT_ID, wpa3_gtk, sizeof(wpa3_gtk));
	zassert_equal(ret, 0, "Failed to send IPC command for WPA3 GTK");
}

/**
 * @brief Test WPA2 PTK key installation and IPC command
 */
ZTEST(wifi_crypto, test_wpa2_ptk)
{
	int ret;

	LOG_INF("Testing WPA2 PTK key");

	/* Generate dummy WPA2 PTK */
	generate_dummy_key(wpa2_ptk, sizeof(wpa2_ptk));

	/* Install key to KMU */
	ret = install_key_to_kmu(WPA2_PTK_KEYSLOT_ID, wpa2_ptk, sizeof(wpa2_ptk));
	zassert_equal(ret, 0, "Failed to install WPA2 PTK to KMU");

	/* Send IPC command */
	ret = send_ipc_crypto_cmd(WPA2_PTK_KEYSLOT_ID, wpa2_ptk, sizeof(wpa2_ptk));
	zassert_equal(ret, 0, "Failed to send IPC command for WPA2 PTK");
}

/**
 * @brief Test WPA3 PTK key installation and IPC command
 */
ZTEST(wifi_crypto, test_wpa3_ptk)
{
	int ret;

	LOG_INF("Testing WPA3 PTK key");

	/* Generate dummy WPA3 PTK */
	generate_dummy_key(wpa3_ptk, sizeof(wpa3_ptk));

	/* Install key to KMU */
	ret = install_key_to_kmu(WPA3_PTK_KEYSLOT_ID, wpa3_ptk, sizeof(wpa3_ptk));
	zassert_equal(ret, 0, "Failed to install WPA3 PTK to KMU");

	/* Send IPC command */
	ret = send_ipc_crypto_cmd(WPA3_PTK_KEYSLOT_ID, wpa3_ptk, sizeof(wpa3_ptk));
	zassert_equal(ret, 0, "Failed to send IPC command for WPA3 PTK");
}
#endif

ZTEST(wifi_crypto, test_main)
{
	uint32_t ccmp256_key[8] = {0x0C0D0E0F, 0x08090A0B, 0x04050607, 0x00010203,
				   0xF2BDD52F, 0x514A8A19, 0xCE371185, 0xC97C1F67};

	wifi_keys_key_type_t type = PEER_UCST_ENC;
	psa_key_attributes_t attr = wifi_keys_key_attributes_init(type, 0, 0);
	uint32_t key_length = wifi_keys_get_key_size_in_bytes(type);
	psa_key_id_t key_id;

	psa_status_t status;

	status = psa_crypto_init();
	zassert_equal(status, PSA_SUCCESS);

	status = psa_import_key(&attr, (const uint8_t *)ccmp256_key, key_length, &key_id);
	zassert_equal(status, PSA_SUCCESS);

	*(volatile uint32_t *)0x48086C04 = 1; /* NRF_WIFICORE_RPUSYS->EDCPERIP.EDCGPIO1OUT */
	while (*(volatile uint32_t *)0x48086C00 != 2) { /* NRF_WIFICORE_RPUSYS->EDCPERIP.EDCGPIO0OUT */
	}

	status = psa_destroy_key(key_id);
	zassert_equal(status, PSA_SUCCESS);
}

/**
 * @brief Test setup function - initialize IPC
 */
static void test_setup(void *fixture)
{
	NRF_WIFICORE_LMAC_VPR->CPURUN = (VPR_CPURUN_EN_Running << VPR_CPURUN_EN_Pos);

	int ret;

	ARG_UNUSED(fixture);

	LOG_INF("Initializing IPC for crypto test");

	ret = ipc_init();
	zassert_equal(ret, 0, "Failed to initialize IPC");

	/* Register RX callback */
	ret = ipc_register_rx_cb(ipc_rx_callback, NULL);
	zassert_equal(ret, 0, "Failed to register IPC RX callback");
}

/**
 * @brief Test teardown function - deinitialize IPC
 */
static void test_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	LOG_INF("Deinitializing IPC for crypto test");

	ipc_deinit();
}

ZTEST_SUITE(wifi_crypto, NULL, NULL, test_setup, test_teardown, NULL);
