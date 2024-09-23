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
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_rpl);

#include <mesh/net.h>
#include <mesh/rpl.h>
#include <emds/emds.h>

static struct bt_mesh_rpl replay_list[CONFIG_BT_MESH_CRPL];

EMDS_STATIC_ENTRY_DEFINE(rpl_store, CONFIG_BT_MESH_RPL_INDEX, replay_list, sizeof(replay_list));

void bt_mesh_rpl_update(struct bt_mesh_rpl *rpl,
		struct bt_mesh_net_rx *rx)
{
	/* If this is the first message on the new IV index, we should reset it
	 * to zero to avoid invalid combinations of IV index and seg.
	 */
	if (rpl->old_iv && !rx->old_iv) {
		rpl->seg = 0;
	}

	rpl->src = rx->ctx.addr;
	rpl->seq = rx->seq;
	rpl->old_iv = rx->old_iv;
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
	(void)memset(replay_list, 0, sizeof(replay_list));
}

void bt_mesh_rpl_reset(void)
{
	int shift = 0;
	int last = 0;

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
}

void bt_mesh_rpl_pending_store(uint16_t addr)
{}
