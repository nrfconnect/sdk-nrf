/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/iterable_sections.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_ble.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_link.h>
#include <zephyr/logging/log_output.h>

#include "cpuapp_verbose.h"
#include "fs_dump_ble.h"

LOG_MODULE_REGISTER(cpuapp_main, LOG_LEVEL_INF);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, LOGGER_BACKEND_BLE_ADV_UUID_DATA),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const char * const app_prefixes[] = {
	"cpuapp_main",
	"cpuflpr_main",
};

static const char * const verbose_prefixes[] = {
	"cpuapp_verbose",
	"cpuflpr_verbose",
};

static const struct log_backend *uart_backend;
static const struct log_backend *ble_backend;
static const struct log_backend *fs_backend;

static bool ble_verbose_enabled;
static uint8_t routed_domains;

static uint8_t active_domain_count(void)
{
	uint8_t cnt = 1U;

#if defined(CONFIG_LOG_MULTIDOMAIN)
	STRUCT_SECTION_FOREACH(log_link, link) {
		if (log_link_is_active(link) == 0) {
			cnt += log_link_domains_count(link);
		}
	}
#endif

	return cnt;
}

static bool name_has_prefix(const char *name, const char *prefix)
{
	return name && prefix && (strncmp(name, prefix, strlen(prefix)) == 0);
}

static bool source_matches(const char *name, const char * const prefixes[], size_t prefix_cnt)
{
	for (size_t i = 0; i < prefix_cnt; i++) {
		if (name_has_prefix(name, prefixes[i])) {
			return true;
		}
	}

	return false;
}

static const struct log_backend *backend_find(const char *prefix)
{
	size_t len = strlen(prefix);

	STRUCT_SECTION_FOREACH(log_backend, backend) {
		if (strncmp(prefix, backend->name, len) == 0) {
			return backend;
		}
	}

	return NULL;
}

static void set_all_sources_level(const struct log_backend *backend, uint32_t level)
{
	uint8_t domains = active_domain_count();

	for (uint8_t d = 0; d < domains; d++) {
		uint16_t src_cnt = log_src_cnt_get(d);

		for (uint16_t s = 0; s < src_cnt; s++) {
			(void)log_filter_set(backend, d, s, level);
		}
	}
}

static void set_matching_sources_level(const struct log_backend *backend,
				       const char * const prefixes[],
				       size_t prefix_cnt,
				       uint32_t level)
{
	uint8_t domains = active_domain_count();

	for (uint8_t d = 0; d < domains; d++) {
		uint16_t src_cnt = log_src_cnt_get(d);

		for (uint16_t s = 0; s < src_cnt; s++) {
			const char *name = log_source_name_get(d, s);

			if (source_matches(name, prefixes, prefix_cnt)) {
				(void)log_filter_set(backend, d, s, level);
			}
		}
	}
}

static void apply_base_routing_filters(void)
{
	if (uart_backend) {
		set_all_sources_level(uart_backend, LOG_LEVEL_ERR);
		set_matching_sources_level(uart_backend, app_prefixes,
					   ARRAY_SIZE(app_prefixes), LOG_LEVEL_INF);
	}

	if (fs_backend) {
		set_all_sources_level(fs_backend, LOG_LEVEL_NONE);
		set_matching_sources_level(fs_backend, app_prefixes,
					   ARRAY_SIZE(app_prefixes), LOG_LEVEL_ERR);
	}

	if (ble_backend) {
		set_all_sources_level(ble_backend, LOG_LEVEL_NONE);
		set_matching_sources_level(ble_backend, app_prefixes,
					   ARRAY_SIZE(app_prefixes), LOG_LEVEL_INF);
		if (ble_verbose_enabled) {
			set_matching_sources_level(ble_backend, verbose_prefixes,
						   ARRAY_SIZE(verbose_prefixes), LOG_LEVEL_INF);
		}
	}

	routed_domains = active_domain_count();
}

static void refresh_filters_if_domains_changed(void)
{
	uint8_t active_domains = active_domain_count();

	if (active_domains == routed_domains) {
		return;
	}

	apply_base_routing_filters();
	LOG_INF("Re-applied routing filters for %u active domain(s)", active_domains);
}

#if defined(CONFIG_MULTIDOMAIN_BACKENDS_DICTIONARY_LOGGING)
static void configure_dictionary_mode(void)
{
	int err;

	if (uart_backend) {
		err = log_backend_format_set(uart_backend, LOG_OUTPUT_DICT);
		LOG_INF("UART dictionary mode set: %d", err);
	}

	if (ble_backend) {
		err = log_backend_format_set(ble_backend, LOG_OUTPUT_DICT);
		LOG_INF("BLE dictionary mode set: %d", err);
	}

	if (fs_backend) {
		err = log_backend_format_set(fs_backend, LOG_OUTPUT_DICT);
		LOG_INF("FS dictionary mode set: %d", err);
	}
}
#else
static inline void configure_dictionary_mode(void)
{
}
#endif

static void wait_for_remote_domain(void)
{
	uint32_t waited_ms = 0U;
	const uint8_t expected_domains = 2U;

	while ((active_domain_count() < expected_domains) && (waited_ms < 5000U)) {
		k_sleep(K_MSEC(100));
		waited_ms += 100U;
	}

	if (active_domain_count() >= expected_domains) {
		LOG_INF("Domain count at filter setup: active=%u expected=%u",
			active_domain_count(), expected_domains);
	} else {
		LOG_WRN("Remote domain not active yet (active=%u expected=%u)",
			active_domain_count(), expected_domains);
	}
}

static void start_uart_backend(void)
{
	if (!uart_backend) {
		return;
	}

	if (!uart_backend->cb->initialized) {
		log_backend_init(uart_backend);
	}

	if (!log_backend_is_active(uart_backend)) {
		log_backend_enable(uart_backend, uart_backend->cb->ctx, LOG_LEVEL_NONE);
	}
}

static void purge_fs_log_files(void)
{
	struct fs_dir_t dir;
	struct fs_dirent ent;
	int err;
	char path[96];

	fs_dir_t_init(&dir);
	err = fs_opendir(&dir, CONFIG_LOG_BACKEND_FS_DIR);
	if (err != 0) {
		LOG_WRN("FS cleanup skipped (opendir err %d)", err);
		return;
	}

	while (true) {
		err = fs_readdir(&dir, &ent);
		if ((err != 0) || (ent.name[0] == '\0')) {
			break;
		}

		if (ent.type != FS_DIR_ENTRY_FILE) {
			continue;
		}

		int len = snprintk(path, sizeof(path), "%s/%s",
				  CONFIG_LOG_BACKEND_FS_DIR, ent.name);
		if ((len > 0) && (len < (int)sizeof(path))) {
			(void)fs_unlink(path);
		}
	}

	(void)fs_closedir(&dir);
}

static void start_fs_backend(void)
{
	if (!fs_backend) {
		return;
	}

	purge_fs_log_files();
	if (!fs_backend->cb->initialized) {
		log_backend_init(fs_backend);
	}
	if (!log_backend_is_active(fs_backend)) {
		log_backend_enable(fs_backend, fs_backend->cb->ctx, LOG_LEVEL_NONE);
	}
	LOG_INF("FS backend enabled");
}

static void enable_ble_verbose_modules(void)
{
	if (!ble_backend || ble_verbose_enabled) {
		return;
	}

	ble_verbose_enabled = true;
	set_matching_sources_level(ble_backend, verbose_prefixes,
				   ARRAY_SIZE(verbose_prefixes), LOG_LEVEL_INF);
	LOG_INF("BLE runtime filtering: enabled INF for verbose modules");
}

static void ble_notify_hook(bool status, void *ctx)
{
	ARG_UNUSED(ctx);
	LOG_INF("BLE backend notifications %s", status ? "enabled" : "disabled");
}

static void start_adv(void)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1,
				  ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		LOG_ERR("BLE advertising start failed (%d)", err);
		return;
	}

	LOG_INF("BLE advertising started");
}

static int setup_ble(void)
{
	int err;

	logger_backend_ble_set_hook(ble_notify_hook, NULL);

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (%d)", err);
		return err;
	}

	start_adv();

	return 0;
}

static void report_fs_status(void)
{
	struct fs_dirent ent;
	int err = fs_stat(CONFIG_LOG_BACKEND_FS_DIR, &ent);

	if ((err == 0) && (ent.type == FS_DIR_ENTRY_DIR)) {
		LOG_INF("Filesystem ready at %s", CONFIG_LOG_BACKEND_FS_DIR);
	} else {
		LOG_WRN("Filesystem not ready at %s (err %d)",
			CONFIG_LOG_BACKEND_FS_DIR, err);
	}
}

int main(void)
{
	uint32_t seq = 0U;

	uart_backend = backend_find("log_backend_uart");
	ble_backend = backend_find("log_backend_ble");
	fs_backend = backend_find("log_backend_fs");

	LOG_INF("multi-domain backend sample start (%s)", CONFIG_BOARD);
	LOG_INF("backend presence: uart=%d ble=%d fs=%d",
		uart_backend != NULL, ble_backend != NULL, fs_backend != NULL);
	LOG_INF("Demo cadence: app logs every 10 seconds");

	configure_dictionary_mode();
	fs_dump_ble_init();

	(void)setup_ble();
	report_fs_status();
	wait_for_remote_domain();
	start_uart_backend();
	start_fs_backend();
	apply_base_routing_filters();
	LOG_INF("Routing: UART(ERR all + app INF/WRN), BLE(app INF+), FS(app ERR)");
	LOG_ERR("demo startup marker: this error is intentionally stored in FS");

	while (1) {
		refresh_filters_if_domains_changed();

		if (seq == 20U) {
			enable_ble_verbose_modules();
		}

		LOG_INF("cpuapp info heartbeat seq=%u (10s cadence)", seq);
		if ((seq % 3U) == 0U) {
			LOG_WRN("cpuapp warning checkpoint seq=%u", seq);
		}
		if ((seq % 6U) == 0U) {
			LOG_ERR("cpuapp error checkpoint seq=%u", seq);
		}

		cpuapp_verbose_log(seq);

		seq++;
		k_sleep(K_SECONDS(10));
	}

	return 0;
}
