/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <zephyr/bluetooth/mesh.h>

#define LOG_LEVEL CONFIG_BT_MESH_RPL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_rpl);

#include <mesh/net.h>
#include <mesh/rpl.h>
#include <mesh/settings.h>
#include <bluetooth/mesh/rpl.h>

static struct bt_mesh_rpl replay_list[CONFIG_BT_MESH_CRPL];

#if IS_ENABLED(CONFIG_BT_MESH_RPL_STORAGE_MODE_EMDS)
#include <emds/emds.h>

EMDS_STATIC_ENTRY_DEFINE(rpl_store, CONFIG_BT_MESH_RPL_INDEX, replay_list, sizeof(replay_list));
#endif /* CONFIG_BT_MESH_RPL_STORAGE_MODE_EMDS */

#if IS_ENABLED(CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS)
#include <zephyr/kvss/zms.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#if !PARTITION_EXISTS(rpl_partition)
#error "CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS requires a dedicated rpl_partition"
#endif

#define PAIR_SIZE (2)
#define PAIR_COUNT DIV_ROUND_UP(CONFIG_BT_MESH_CRPL, PAIR_SIZE)

static struct zms_fs rpl_zms_fs = {
	.flash_device = PARTITION_DEVICE(rpl_partition),
	.offset = PARTITION_OFFSET(rpl_partition),
};

struct __packed rpl_val {
	uint32_t seq:24,
	      old_iv:1;
};

BUILD_ASSERT(sizeof(struct rpl_val[PAIR_SIZE]) <= ZMS_DATA_IN_ATE_SIZE,
	     "A stored RPL pair must fit inline in a ZMS ATE");

enum {
	FLAGS_CLEAR_PENDING,
	FLAGS_COUNT,
};
static ATOMIC_DEFINE(flags, FLAGS_COUNT);

static ATOMIC_DEFINE(store_pairs, PAIR_COUNT);
static int most_recent_half_pair_idx = -1;

/* k_sched_lock() guards replay_list and its store state during storing; never held across
 * ZMS calls or anything else that may block.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP), "RPL ZMS storage relies on k_sched_lock() for atomicity");

/* Serializes all ZMS access and most_recent_half_pair_idx across init and store callers. */
static K_MUTEX_DEFINE(rpl_store_lock);

static uint32_t rpl_zms_id(uint16_t src1, uint16_t src2)
{
	return ((uint32_t)src1 << 16) | src2;
}

static uint16_t rpl_zms_id_src_first(uint32_t id)
{
	return id >> 16;
}

static uint16_t rpl_zms_id_src_second(uint32_t id)
{
	return id & 0xFFFF;
}

/* RPL entries are divided into pairs of sequential (even, odd) RPL entries. */
static int pair_idx_of(const struct bt_mesh_rpl *rpl)
{
	return ARRAY_INDEX(replay_list, rpl) / PAIR_SIZE;
}

static const struct bt_mesh_rpl *pair_first(int pair_idx)
{
	return &replay_list[pair_idx * PAIR_SIZE];
}

/* Returns the second entry of a pair, or NULL when CONFIG_BT_MESH_CRPL is odd
 * and this is the trailing pair, which has no second slot.
 */
static const struct bt_mesh_rpl *pair_second(int pair_idx)
{
	int idx = pair_idx * PAIR_SIZE + 1;

	if (idx >= ARRAY_SIZE(replay_list)) {
		return NULL;
	}

	return &replay_list[idx];
}

struct pair_snapshot {
	uint16_t src[PAIR_SIZE];
	struct rpl_val pair[PAIR_SIZE];
};

static bool pair_claim_atomic(int pair_idx, struct pair_snapshot *snapshot,
			      bool force, uint16_t addr_filter)
{
	const struct bt_mesh_rpl *first = pair_first(pair_idx);
	const struct bt_mesh_rpl *second = pair_second(pair_idx);
	bool should_store;
	uint16_t second_src;

	k_sched_lock();

	second_src = second == NULL ? 0 : second->src;
	should_store = (addr_filter == BT_MESH_ADDR_ALL_NODES ||
			first->src == addr_filter ||
			second_src == addr_filter) &&
		       !atomic_test_bit(flags, FLAGS_CLEAR_PENDING) &&
		       (atomic_test_and_clear_bit(store_pairs, pair_idx) || force);
	if (should_store) {
		snapshot->src[0] = first->src;
		snapshot->src[1] = second_src;
		snapshot->pair[0].seq = first->seq;
		snapshot->pair[0].old_iv = first->old_iv;
		snapshot->pair[1].seq = second == NULL ? 0 : second->seq;
		snapshot->pair[1].old_iv = second == NULL ? 0 : second->old_iv;
	}

	k_sched_unlock();

	return should_store;
}

static bool store_claimed(int pair_idx, const struct pair_snapshot *snapshot)
{
	uint32_t zms_id = rpl_zms_id(snapshot->src[0], snapshot->src[1]);
	ssize_t rc;
	int err;

	/* Store the new pair and delete the half pair if any */
	rc = zms_write(&rpl_zms_fs, zms_id, snapshot->pair, sizeof(snapshot->pair));
	if (rc < 0) {
		LOG_ERR("Failed to write ZMS entry for id 0x%08x, rc %zd", zms_id, rc);
		return false;
	}

	if (snapshot->src[1] == 0) {
		most_recent_half_pair_idx = pair_idx;
	} else if (most_recent_half_pair_idx == pair_idx) {
		zms_id = rpl_zms_id(snapshot->src[0], 0);
		err = zms_delete(&rpl_zms_fs, zms_id);
		if (err) {
			LOG_ERR("Failed to delete half pair, err %d", err);
			return false;
		}
		most_recent_half_pair_idx = -1;
	}

	return true;
}

static bool store_pair(int pair_idx, uint16_t addr_filter)
{
	struct pair_snapshot snapshot = { 0 };
	struct pair_snapshot most_recent_half_pair = { 0 };

	if (!pair_claim_atomic(pair_idx, &snapshot, false, addr_filter)) {
		return false;
	}

	if (snapshot.src[1] == 0 && most_recent_half_pair_idx != pair_idx &&
	    most_recent_half_pair_idx != -1) {
		/*
		 * We are about to store a half pair, but the last stored half-pair
		 * is still half in ZMS, so promote that first.
		 */
		if (!pair_claim_atomic(most_recent_half_pair_idx, &most_recent_half_pair, true,
				       BT_MESH_ADDR_ALL_NODES) ||
		    !store_claimed(most_recent_half_pair_idx, &most_recent_half_pair)) {
			return false;
		}
	}

	return store_claimed(pair_idx, &snapshot);
}

static void clear_storage(void)
{
	int err;

	err = zms_clear(&rpl_zms_fs);
	if (err) {
		LOG_ERR("Failed to clear ZMS, err %d", err);
		return;
	}

	/* zms_clear unmounts the filesystem, so we need to remount it */
	err = zms_mount(&rpl_zms_fs);
	if (err) {
		LOG_ERR("Failed to remount ZMS, err %d", err);
	}

	most_recent_half_pair_idx = -1;
}

static int rpl_zms_mount(void)
{
	struct flash_pages_info info;
	int err;

	if (!device_is_ready(rpl_zms_fs.flash_device)) {
		LOG_ERR("RPL flash device not ready");
		return -ENODEV;
	}

	err = flash_get_page_info_by_offs(rpl_zms_fs.flash_device, rpl_zms_fs.offset, &info);
	if (err != 0) {
		LOG_ERR("Failed to get RPL partition page info, err %d", err);
		return err;
	}

	rpl_zms_fs.sector_size = info.size;
	rpl_zms_fs.sector_count = PARTITION_SIZE(rpl_partition) / info.size;

	return zms_mount(&rpl_zms_fs);
}

enum rpl_zms_id_check_result {
	RPL_ZMS_ID_CHECK_VALID,
	RPL_ZMS_ID_CHECK_INVALID,
	RPL_ZMS_ID_CHECK_HALF_PAIR_DELETED,
};

static enum rpl_zms_id_check_result rpl_zms_id_check(uint32_t id, size_t len)
{
	uint16_t src1 = rpl_zms_id_src_first(id);
	uint16_t src2 = rpl_zms_id_src_second(id);

	if (!BT_MESH_ADDR_IS_UNICAST(src1) ||
	    !(src2 == 0 || BT_MESH_ADDR_IS_UNICAST(src2)) ||
	    (src1 == src2)) {
		LOG_WRN("Unexpected ZMS entry with invalid id 0x%08x", id);
		return RPL_ZMS_ID_CHECK_INVALID;
	}

	if (src2 == 0 && len == 0) {
		/* This is a half pair delete marker, which is expected. */
		return RPL_ZMS_ID_CHECK_HALF_PAIR_DELETED;
	}

	if (len != sizeof(struct rpl_val) * PAIR_SIZE) {
		LOG_WRN("Unexpected ZMS entry length %zu for id 0x%08x", len, id);
		return RPL_ZMS_ID_CHECK_INVALID;
	}

	return RPL_ZMS_ID_CHECK_VALID;
}

struct rpl_restore_ctx {
	struct {
		uint16_t src;
		struct rpl_val val;
		bool stale;
		bool found;
	} pending_half_pair;
	int next_free;
};

static struct bt_mesh_rpl *rpl_append(struct rpl_restore_ctx *ctx, uint16_t src,
				      struct rpl_val pair)
{
	if (ctx->next_free >= ARRAY_SIZE(replay_list)) {
		LOG_ERR("RPL restore failed: Not enough space for new entry");
		return NULL;
	}
	replay_list[ctx->next_free].src = src;
	replay_list[ctx->next_free].seq = pair.seq;
	replay_list[ctx->next_free].old_iv = pair.old_iv;
	return &replay_list[ctx->next_free++];
}

static int rpl_restore_half_pair(struct rpl_restore_ctx *ctx, uint32_t id, struct rpl_val *data)
{
	uint16_t src1 = rpl_zms_id_src_first(id);
	bool stale = false;

	if (data == NULL) {
		/* If the first half pair ATE is a delete marker, no half pair is live. */
		ctx->pending_half_pair.found = true;
		return 0;
	}

	if (data[1].seq != 0 || data[1].old_iv != 0) {
		LOG_ERR("Half pair ATE for id 0x%08x has non-zero second half", id);
		return -EINVAL;
	}

	/*
	 * At most one half pair is live, and it is always the first half pair ATE seen. Any
	 * later half pair ATE is history, or a leftover from a failed delete.
	 */
	if (ctx->pending_half_pair.found) {
		return 0;
	}

	for (int i = 0; i < ctx->next_free; i++) {
		if (replay_list[i].src == src1) {
			if (i % 2 != 0) {
				LOG_ERR("RPL inconsistency: Half pair 0x%08x already at odd index",
					id);
				return -EINVAL;
			}

			stale = true;
			break;
		}
	}

	ctx->pending_half_pair.found = true;
	ctx->pending_half_pair.src = src1;
	ctx->pending_half_pair.val = data[0];
	ctx->pending_half_pair.stale = stale;
	return 0;
}

/** Check if a pair identified by `id` is already restored at pair index `pair_idx`
 *  in the replay list.
 */
static bool is_pair_at(int pair_idx, uint32_t id)
{
	const struct bt_mesh_rpl *first = pair_first(pair_idx);
	const struct bt_mesh_rpl *second = pair_second(pair_idx);

	return first->src == rpl_zms_id_src_first(id) &&
	       second != NULL && second->src == rpl_zms_id_src_second(id);
}

static int rpl_restore_full_pair(struct rpl_restore_ctx *ctx, uint32_t id, struct rpl_val *data)
{
	uint16_t src1 = rpl_zms_id_src_first(id);
	uint16_t src2 = rpl_zms_id_src_second(id);
	struct bt_mesh_rpl *rpl;

	for (int i = 0; i < ctx->next_free; i++) {
		if (i % 2 == 0 && is_pair_at(i / PAIR_SIZE, id)) {
			return 0; /* Skip this - we have a newer version already. */
		} else if (replay_list[i].src == src1 ||
			   replay_list[i].src == src2) {
			LOG_ERR("RPL inconsistency: partial match for id 0x%08x", id);
			return -EINVAL;
		}
	}

	if (ctx->pending_half_pair.src == src1 ||
	    ctx->pending_half_pair.src == src2) {
		LOG_ERR("RPL inconsistency: pending half pair conflict for id 0x%08x", id);
		return -EINVAL;
	}

	rpl = rpl_append(ctx, src1, data[0]);
	if (rpl == NULL) {
		return -ENOMEM;
	}
	rpl = rpl_append(ctx, src2, data[1]);
	if (rpl == NULL) {
		return -ENOMEM;
	}
	return 0;
}

static int rpl_handle_pending_half_pair(struct rpl_restore_ctx *ctx)
{
	struct bt_mesh_rpl *rpl;

	if (ctx->pending_half_pair.src != 0) {
		if (ctx->pending_half_pair.stale) {
			uint32_t zms_id = rpl_zms_id(ctx->pending_half_pair.src, 0);

			int err = zms_delete(&rpl_zms_fs, zms_id);

			if (err) {
				LOG_ERR("Failed to delete stale half pair, err %d", err);
			}
			return 0;
		}

		rpl = rpl_append(ctx, ctx->pending_half_pair.src, ctx->pending_half_pair.val);
		if (rpl == NULL) {
			return -ENOMEM;
		}
		most_recent_half_pair_idx = pair_idx_of(rpl);
	}
	return 0;
}

static int rpl_restore_entry(struct rpl_restore_ctx *ctx, uint32_t id, size_t len,
			     struct rpl_val *data)
{
	switch (rpl_zms_id_check(id, len)) {
	case RPL_ZMS_ID_CHECK_HALF_PAIR_DELETED:
		return rpl_restore_half_pair(ctx, id, NULL);
	case RPL_ZMS_ID_CHECK_VALID:
		return rpl_zms_id_src_second(id) == 0 ? rpl_restore_half_pair(ctx, id, data)
						      : rpl_restore_full_pair(ctx, id, data);
	case RPL_ZMS_ID_CHECK_INVALID:
	default:
		return -EINVAL;
	}
}

static int rpl_restore(void)
{
	int rc;
	int err;
	int err_out = 0;
	struct zms_iter iter;
	zms_id_t id;
	size_t len;
	struct rpl_restore_ctx ctx = { 0 };
	struct rpl_val data[PAIR_SIZE] = { 0 };

	err = zms_iter_init(&rpl_zms_fs, &iter);
	if (err != 0) {
		LOG_ERR("Failed to initialize ZMS iterator, err %d", err);
		err_out = -EIO;
		goto out;
	}

	while ((rc = zms_iter_next_all(&rpl_zms_fs, &iter, &id, &len, data, sizeof(data))) == 1) {
		err = rpl_restore_entry(&ctx, id, len, data);
		if (err != 0) {
			err_out = err;
			if (err_out == -ENOMEM) {
				goto out; /* RPL already full; stop processing further entries. */
			}
		}
	}

	err = rpl_handle_pending_half_pair(&ctx);
	if (err != 0) {
		err_out = err;
		goto out;
	}

	if (rc < 0) {
		LOG_ERR("Failed to iterate over ZMS, err %d", rc);
		err_out = -EIO;
	}

out:
	return err_out;
}

static int rpl_zms_init(void)
{
	int err;

	if (rpl_zms_fs.ready) {
		LOG_ERR("RPL storage is already initialized");
		return -EALREADY;
	}

	if (bt_mesh_is_provisioned()) {
		LOG_ERR("RPL must be initialized before the node is provisioned");
		return -EBUSY;
	}

	err = rpl_zms_mount();
	if (err) {
		LOG_ERR("Failed to mount ZMS for RPL storage, err %d", err);
		return -EACCES;
	}

	err = rpl_restore();
	if (err) {
		LOG_ERR("Failed to restore RPL from ZMS, err %d", err);
		return err;
	}

	return 0;
}

#endif /* CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS */

void bt_mesh_rpl_update(struct bt_mesh_rpl *rpl,
		struct bt_mesh_net_rx *rx)
{
#if IS_ENABLED(CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS)
	k_sched_lock();
#endif /* CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS */

	/* If this is the first message on the new IV index, we should reset it
	 * to zero to avoid invalid combinations of IV index and seg.
	 */
	if (rpl->old_iv && !rx->old_iv) {
		rpl->seg = 0;
	}

	rpl->src = rx->ctx.addr;
	rpl->seq = rx->seq;
	rpl->old_iv = rx->old_iv;

#if IS_ENABLED(CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS)
	atomic_set_bit(store_pairs, pair_idx_of(rpl));
	k_sched_unlock();

	if (CONFIG_BT_MESH_RPL_STORE_TIMEOUT >= 0) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_PENDING);
	}
#endif /* CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS */
}

/* Check the Replay Protection List for a replay attempt. If non-NULL match
 * parameter is given the RPL slot is returned but it is not immediately
 * updated (needed for segmented messages), whereas if a NULL match is given
 * the RPL is immediately updated (used for unsegmented messages).
 */
bool bt_mesh_rpl_check(struct bt_mesh_net_rx *rx,
		struct bt_mesh_rpl **match, bool bridge)
{
	int i;

	/* Don't bother checking messages from ourselves */
	if (rx->net_if == BT_MESH_NET_IF_LOCAL) {
		return false;
	}

	/* The RPL is used only for the local node or Subnet Bridge. */
	if (!rx->local_match && !bridge) {
		return false;
	}

	for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
		struct bt_mesh_rpl *rpl = &replay_list[i];

		/* Empty slot */
		if (!rpl->src) {
			if (match) {
				*match = rpl;
			} else {
				bt_mesh_rpl_update(rpl, rx);
			}

			return false;
		}

		/* Existing slot for given address */
		if (rpl->src == rx->ctx.addr) {
			if (rx->old_iv && !rpl->old_iv) {
				return true;
			}

			if ((!rx->old_iv && rpl->old_iv) ||
			    rpl->seq < rx->seq) {
				if (match) {
					*match = rpl;
				} else {
					bt_mesh_rpl_update(rpl, rx);
				}

				return false;
			} else {
				return true;
			}
		}
	}

	LOG_ERR("RPL is full!");
	return true;
}

void bt_mesh_rpl_clear(void)
{
#if IS_ENABLED(CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS)
	k_sched_lock();
	(void)memset(replay_list, 0, sizeof(replay_list));
	for (int i = 0; i < PAIR_COUNT; i++) {
		atomic_clear_bit(store_pairs, i);
	}
	atomic_set_bit(flags, FLAGS_CLEAR_PENDING);
	k_sched_unlock();

	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_RESET_PENDING);
#else
	(void)memset(replay_list, 0, sizeof(replay_list));
#endif /* CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS */
}

void bt_mesh_rpl_reset(void)
{
	int shift = 0;
	int last = 0;

#if IS_ENABLED(CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS)
	k_sched_lock();
#endif /* CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS */

	/* Discard "old" IV Index entries from RPL and flag
	 * any other ones (which are valid) as old.
	 */
	for (int i = 0; i < ARRAY_SIZE(replay_list); i++) {
		struct bt_mesh_rpl *rpl = &replay_list[i];

		if (rpl->src) {
			if (rpl->old_iv) {
				(void)memset(rpl, 0, sizeof(*rpl));

				shift++;
			} else {
				rpl->old_iv = true;

				if (shift > 0) {
					replay_list[i - shift] = *rpl;
				}
			}

			last = i;
		}
	}

	(void) memset(&replay_list[last - shift + 1], 0, sizeof(struct bt_mesh_rpl) * shift);

#if IS_ENABLED(CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS)
	for (int i = 0; i < PAIR_COUNT; i++) {
		atomic_clear_bit(store_pairs, i);
	}

	/* Schedule rewrite of every pair after clearing. */
	for (int i = 0; i < PAIR_COUNT; i++) {
		if (!pair_first(i)->src) {
			break;
		}
		atomic_set_bit(store_pairs, i);
	}

	atomic_set_bit(flags, FLAGS_CLEAR_PENDING);
	k_sched_unlock();

	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_RESET_PENDING);
#endif /* CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS */
}

void bt_mesh_rpl_pending_store(uint16_t addr)
{
#if IS_ENABLED(CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS)
	bool store_all_pending = (addr == BT_MESH_ADDR_ALL_NODES);
	bool clear_pending;

	if (!BT_MESH_ADDR_IS_UNICAST(addr) && !store_all_pending) {
		return;
	}

	(void)k_mutex_lock(&rpl_store_lock, K_FOREVER);

	clear_pending = atomic_test_and_clear_bit(flags, FLAGS_CLEAR_PENDING);
	/* A clear requires storing all pairs, even if a specific address was requested. */
	store_all_pending = store_all_pending || clear_pending;

	if (store_all_pending) {
		bt_mesh_settings_store_cancel(BT_MESH_SETTINGS_RPL_PENDING);
		bt_mesh_settings_store_cancel(BT_MESH_SETTINGS_RPL_RESET_PENDING);
	}

	if (clear_pending) {
		clear_storage();
	}

	for (int i = 0; i < PAIR_COUNT; i++) {
		(void)store_pair(i, store_all_pending ? BT_MESH_ADDR_ALL_NODES : addr);
	}

	/* A reset or clear during this store may have had its scheduled store cancelled above. */
	if (atomic_test_bit(flags, FLAGS_CLEAR_PENDING)) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_RESET_PENDING);
	}

	(void)k_mutex_unlock(&rpl_store_lock);
#endif /* CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS */
}

void bt_mesh_rpl_pending_store_all_nodes(void)
{
	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);
}

int bt_mesh_rpl_init(void)
{
#if IS_ENABLED(CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS)
	int err;

	(void)k_mutex_lock(&rpl_store_lock, K_FOREVER);
	err = rpl_zms_init();
	(void)k_mutex_unlock(&rpl_store_lock);

	return err;
#else
	return 0;
#endif /* CONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS */
}
