/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <random/rand32.h>
#include <bluetooth/services/hrs.h>
#include "pwm_led.h"
#include "peer.h"
#include <bluetooth/bluetooth.h>

#define PEER_MAX                8   /* Maximum number of tracked peer devices. */

#define STACKSIZE               1024
#define PEER_THREAD_PRIORITY    K_LOWEST_APPLICATION_THREAD_PRIO

#define PEER_TIMEOUT_STEP_MS    500
#define PEER_TIMEOUT_INIT_MS    10000

#define DISTANCE_MAX_LED        50   /* from 0 to DISTANCE_MAX_LED [decimeter] -
				      * the distance range which is indicated by the PWM LED
				      */

#define AC_ACCESS_ADDRESS       0x8e89bed6

static void timeout_handler(struct k_timer *timer_id);
static K_TIMER_DEFINE(timer, timeout_handler, NULL);

struct peer_entry {
	sys_snode_t node;
	bt_addr_le_t bt_addr;
	struct dm_result result;
	uint16_t timeout_ms;
};

static struct peer_entry *closest_peer;
static uint32_t access_address;

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

	if (a->result.dist_estimates.mcpd.best > b->result.dist_estimates.mcpd.best) {
		return a;
	}
	return b;
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

static uint16_t meter_to_decimeter(float value)
{
	float val;

	val = (value < 0 ? 0 : value) * 10;
	return val < UINT16_MAX ? (uint16_t)val : UINT16_MAX;
}

static void ble_notification(const struct peer_entry *peer)
{
	if (!peer) {
		return;
	}

	float res;

	if (peer->result.ranging_mode == DM_RANGING_MODE_RTT) {
		res = peer->result.dist_estimates.rtt.rtt;
	} else {
		res = peer->result.dist_estimates.mcpd.best;
	}

	bt_hrs_notify(meter_to_decimeter(res));
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
		res = peer->result.dist_estimates.mcpd.best;
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
		printk("rtt: rtt=%.2f\n", result->dist_estimates.rtt.rtt);
	} else {
		printk("mcpd: ifft=%.2f phase_slope=%.2f rssi_openspace=%.2f best=%.2f\n",
			result->dist_estimates.mcpd.ifft,
			result->dist_estimates.mcpd.phase_slope,
			result->dist_estimates.mcpd.rssi_openspace,
			result->dist_estimates.mcpd.best);
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
			ble_notification(closest_peer);
		}
	}
}

int peer_access_address_prepare(void)
{
	bt_addr_le_t addr = {0};
	size_t count = 1;

	bt_id_get(&addr, &count);

	access_address = addr.a.val[0];
	access_address |= addr.a.val[1] << 8;
	access_address |= addr.a.val[2] << 16;
	access_address |= addr.a.val[3] << 24;

	if (access_address == 0) {
		return -EFAULT;
	}

	return 0;
}

uint32_t peer_access_address_get(void)
{
	return access_address;
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
