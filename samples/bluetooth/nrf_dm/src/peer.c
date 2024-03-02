/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include <math.h>

#include "peer.h"
#include "pwm_led.h"
#include "service.h"

#define PEER_MAX                8   /* Maximum number of tracked peer devices. */

#define STACKSIZE               1024
#define PEER_THREAD_PRIORITY    K_LOWEST_APPLICATION_THREAD_PRIO

#define PEER_TIMEOUT_STEP_MS    500
#define PEER_TIMEOUT_INIT_MS    10000

#define DISTANCE_MAX_LED        50   /* from 0 to DISTANCE_MAX_LED [decimeter] -
				      * the distance range which is indicated by the PWM LED
				      */
#define DEFAULT_RANGING_MODE    DM_RANGING_MODE_MCPD

static void timeout_handler(struct k_timer *timer_id);
static K_TIMER_DEFINE(timer, timeout_handler, NULL);

struct peer_entry {
	sys_snode_t node;
	bt_addr_le_t bt_addr;
	struct dm_result result;
	uint16_t timeout_ms;
};

static struct peer_entry *closest_peer;
static uint32_t rng_seed;
static enum dm_ranging_mode ranging_mode = DEFAULT_RANGING_MODE;

K_MSGQ_DEFINE(result_msgq, sizeof(struct dm_result), 16, 4);

static K_HEAP_DEFINE(peer_heap, PEER_MAX * sizeof(struct peer_entry));
static sys_slist_t peer_list = SYS_SLIST_STATIC_INIT(&peer_list);

static K_MUTEX_DEFINE(list_mtx);

static void list_lock(void)
{
	k_mutex_lock(&list_mtx, K_FOREVER);
}

static void list_unlock(void)
{
	k_mutex_unlock(&list_mtx);
}

static struct peer_entry *mcpd_min_peer_result(struct peer_entry *a, struct peer_entry *b)
{
	if (!a && !b) {
		return NULL;
	} else if (!a && b) {
		return b->result.ranging_mode == DM_RANGING_MODE_MCPD ? b : NULL;
	} else if (a && !b) {
		return a->result.ranging_mode == DM_RANGING_MODE_MCPD ? a : NULL;
	} else if (a->result.ranging_mode == DM_RANGING_MODE_RTT &&
		   b->result.ranging_mode == DM_RANGING_MODE_RTT) {
		return NULL;
	} else if (a->result.ranging_mode == DM_RANGING_MODE_RTT &&
		   b->result.ranging_mode == DM_RANGING_MODE_MCPD) {
		return b;
	} else if (a->result.ranging_mode == DM_RANGING_MODE_MCPD &&
		   b->result.ranging_mode == DM_RANGING_MODE_RTT){
		return a;
	}

#ifdef CONFIG_DM_HIGH_PRECISION_CALC
	float dist_a = !isnan(a->result.dist_estimates.mcpd.high_precision) ?
		  a->result.dist_estimates.mcpd.high_precision : a->result.dist_estimates.mcpd.best;
	float dist_b = !isnan(b->result.dist_estimates.mcpd.high_precision) ?
		  b->result.dist_estimates.mcpd.high_precision : b->result.dist_estimates.mcpd.best;

	if (dist_a < dist_b) {
		return a;
	}
	return b;
#else
	if (a->result.dist_estimates.mcpd.best < b->result.dist_estimates.mcpd.best) {
		return a;
	}
	return b;
#endif
}

static struct peer_entry *peer_find_closest(void)
{
	struct peer_entry *closest_peer, *item;
	sys_snode_t *node, *tmp;

	closest_peer = SYS_SLIST_PEEK_HEAD_CONTAINER(&peer_list, closest_peer, node);
	if (!closest_peer) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_NODE_SAFE(&peer_list, node, tmp) {
		item = CONTAINER_OF(node, struct peer_entry, node);
		closest_peer = mcpd_min_peer_result(closest_peer, item);
	}

	return closest_peer;
}

struct peer_entry *peer_find(const bt_addr_le_t *peer)
{
	if (!peer) {
		return NULL;
	}

	sys_snode_t *node, *tmp;
	struct peer_entry *item;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&peer_list, node, tmp) {
		item = CONTAINER_OF(node, struct peer_entry, node);
		if (bt_addr_le_cmp(&item->bt_addr, peer) == 0) {
			return item;
		}
	}

	return NULL;
}

static void ble_notification(const struct peer_entry *peer)
{
	service_distance_measurement_update(&peer->bt_addr, &peer->result);
}

static void led_notification(const struct peer_entry *peer)
{
	if (!peer) {
		pwm_led_set(0);
		return;
	}

	float res;

	if (peer->result.ranging_mode == DM_RANGING_MODE_RTT) {
		res = peer->result.dist_estimates.rtt.rtt;
	} else {
#ifdef CONFIG_DM_HIGH_PRECISION_CALC
		res = peer->result.dist_estimates.mcpd.high_precision;
		res = isnan(res) ? peer->result.dist_estimates.mcpd.best : res;
#else
		res = peer->result.dist_estimates.mcpd.best;
#endif
	}

	res = (res < 0 ? 0 : res) * 10;
	if (res > DISTANCE_MAX_LED) {
		pwm_led_set(0);
	} else {
		pwm_led_set((UINT16_MAX/DISTANCE_MAX_LED * res * (-1)) + UINT16_MAX);
	}
}

static void print_result(struct dm_result *result)
{
	if (!result) {
		return;
	}

	const char *quality[DM_QUALITY_NONE + 1] = {"ok", "poor", "do not use", "crc fail", "none"};
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&result->bt_addr, addr, sizeof(addr));

	printk("\nMeasurement result:\n");
	printk("\tAddr: %s\n", addr);
	printk("\tQuality: %s\n", quality[result->quality]);

	printk("\tDistance estimates: ");
	if (result->ranging_mode == DM_RANGING_MODE_RTT) {
		printk("rtt: rtt=%.2f\n", (double)result->dist_estimates.rtt.rtt);
	} else {
#ifdef CONFIG_DM_HIGH_PRECISION_CALC
		printk("mcpd: high_precision=%.2f ifft=%.2f phase_slope=%.2f "
			"rssi_openspace=%.2f best=%.2f\n",
			(double)result->dist_estimates.mcpd.high_precision,
			(double)result->dist_estimates.mcpd.ifft,
			(double)result->dist_estimates.mcpd.phase_slope,
			(double)result->dist_estimates.mcpd.rssi_openspace,
			(double)result->dist_estimates.mcpd.best);
#else
		printk("mcpd: ifft=%.2f phase_slope=%.2f rssi_openspace=%.2f best=%.2f\n",
			(double)result->dist_estimates.mcpd.ifft,
			(double)result->dist_estimates.mcpd.phase_slope,
			(double)result->dist_estimates.mcpd.rssi_openspace,
			(double)result->dist_estimates.mcpd.best);
#endif
	}
}

static void timeout_handler(struct k_timer *timer_id)
{
	sys_snode_t *node, *tmp;
	struct peer_entry *item;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&peer_list, node, tmp) {
		item = CONTAINER_OF(node, struct peer_entry, node);
		if (item->timeout_ms > PEER_TIMEOUT_STEP_MS) {
			item->timeout_ms -= PEER_TIMEOUT_STEP_MS;
		} else {
			sys_slist_remove(&peer_list, NULL, node);
			k_heap_free(&peer_heap, item);
			closest_peer = peer_find_closest();

			led_notification(closest_peer);
		}
	}
}

static void peer_thread(void)
{
	struct dm_result result;

	while (1) {
		if (k_msgq_get(&result_msgq, &result, K_FOREVER) == 0) {
			struct peer_entry *peer;

			peer = peer_find(&result.bt_addr);
			if (!peer) {
				continue;
			}

			memcpy(&peer->result, &result, sizeof(peer->result));
			peer->timeout_ms = PEER_TIMEOUT_INIT_MS;
			print_result(&peer->result);

			closest_peer = mcpd_min_peer_result(closest_peer, peer);

			led_notification(closest_peer);
			ble_notification(peer);
		}
	}
}

void peer_ranging_mode_set(enum dm_ranging_mode mode)
{
	ranging_mode = mode;
}

enum dm_ranging_mode peer_ranging_mode_get(void)
{
	return ranging_mode;
}

uint32_t peer_rng_seed_prepare(void)
{
	rng_seed = sys_rand32_get();
	return rng_seed;
}

uint32_t peer_rng_seed_get(void)
{
	return rng_seed;
}

bool peer_supported_test(const bt_addr_le_t *peer)
{
	sys_snode_t *node, *tmp;
	struct peer_entry *item;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&peer_list, node, tmp) {
		item = CONTAINER_OF(node, struct peer_entry, node);
		if (bt_addr_le_cmp(&item->bt_addr, peer) == 0) {
			return true;
		}
	}

	return false;
}

int peer_supported_add(const bt_addr_le_t *peer)
{
	struct peer_entry *item;

	if (peer_supported_test(peer)) {
		return 0;
	}

	item = k_heap_alloc(&peer_heap, sizeof(struct peer_entry), K_NO_WAIT);
	if (!item) {
		return -ENOMEM;
	}

	item->timeout_ms = PEER_TIMEOUT_INIT_MS;
	bt_addr_le_copy(&item->bt_addr, peer);
	list_lock();
	sys_slist_append(&peer_list, &item->node);
	list_unlock();

	return 0;
}

void peer_update(struct dm_result *result)
{
	k_msgq_put(&result_msgq, result, K_NO_WAIT);
}

int peer_init(void)
{
	int err;

	k_timer_start(&timer, K_NO_WAIT, K_MSEC(PEER_TIMEOUT_STEP_MS));

	err = pwm_led_init();
	if (err) {
		printk("PWM LED init failed (err %d)\n", err);
		return err;
	}

	return 0;
}

K_THREAD_DEFINE(peer_consumer_thred_id, STACKSIZE, peer_thread,
		NULL, NULL, NULL, PEER_THREAD_PRIORITY, 0, 0);
