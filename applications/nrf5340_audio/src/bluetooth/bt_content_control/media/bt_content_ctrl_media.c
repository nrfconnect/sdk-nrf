/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_content_ctrl_media_internal.h"

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/bluetooth/audio/mcs.h>
#include <zephyr/bluetooth/audio/mcc.h>

#include "macros_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_content_ctrl_media, CONFIG_BT_CONTENT_CTRL_MEDIA_LOG_LEVEL);

static uint8_t media_player_state = BT_MCS_MEDIA_STATE_PLAYING;

static struct media_player *local_player;
static bt_content_ctrl_media_play_pause_cb play_pause_cb;

enum mcs_disc_status {
	IDLE,
	IN_PROGRESS,
	FINISHED,
};

struct media_ctlr {
	enum mcs_disc_status mcp_mcs_disc_status;
	struct bt_conn *conn;
};

static struct media_ctlr mcc_peer[CONFIG_BT_MAX_CONN];

/**
 * @brief	Get the index of the first available mcc_peer
 *
 * @return	Index if success, -ENOMEM if no available indexes
 */
static int mcc_peer_index_free_get(void)
{
	for (int i = 0; i < ARRAY_SIZE(mcc_peer); i++) {
		if (mcc_peer[i].conn == NULL) {
			return i;
		}
	}

	LOG_WRN("No more indexes for MCC peer");

	return -ENOMEM;
}

/**
 * @brief	Get index of a given conn pointer
 *
 * @param	conn	Pointer to check against
 *
 * @return	index if found, -ESRCH if not found, -EINVAL if invalid conn pointer
 */
static int mcc_peer_index_get(struct bt_conn *conn)
{
	if (conn == NULL) {
		LOG_WRN("Invalid conn pointer");
		return -EINVAL;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(mcc_peer); i++) {
		if (mcc_peer[i].conn == conn) {
			return i;
		}
	}

	LOG_DBG("No matching conn pointer for this mcc_peer");
	return -ESRCH;
}

/**
 * @brief	Callback handler for MCS discover finished.
 *
 * @note	This callback handler will be triggered when MCS
 *		discovery is finished. Used by the client.
 */
static void mcc_discover_mcs_cb(struct bt_conn *conn, int err)
{
	int ret;
	int idx = mcc_peer_index_get(conn);

	if (idx < 0) {
		LOG_WRN("Unable to look up conn pointer: %d", idx);
		return;
	}

	if (err) {
		if (err == BT_ATT_ERR_UNLIKELY) {
			/* BT_ATT_ERR_UNLIKELY may occur in normal operating conditions if there is
			 * a disconnect while discovering, hence it will be treated as a warning.
			 */
			LOG_WRN("Discovery of MCS failed (%d)", err);
		} else {
			LOG_ERR("Discovery of MCS failed (%d)", err);
		}

		mcc_peer[idx].mcp_mcs_disc_status = IDLE;
		return;
	}

	if (mcc_peer[idx].mcp_mcs_disc_status != IN_PROGRESS) {
		/* Due to the design of MCC, there will be several
		 * invocations of this callback. We are only interested
		 * in what we have explicitly requested.
		 */
		LOG_DBG("Filtered out callback");
		return;
	}

	mcc_peer[idx].mcp_mcs_disc_status = FINISHED;
	LOG_INF("Discovery of MCS finished");

	ret = bt_content_ctrl_media_state_update(conn);
	if (ret < 0 && ret != -EBUSY) {
		LOG_WRN("Failed to update media state: %d", ret);
	}
}

/**
 * @brief	Callback handler for sent MCS commands.
 *
 * @note	This callback will be triggered when MCS commands have been sent.
 *		Used by the client.
 */
static void mcc_send_command_cb(struct bt_conn *conn, int err, const struct mpl_cmd *cmd)
{
	LOG_DBG("mcc_send_command_cb");

	if (err) {
		LOG_ERR("MCC: cmd send failed (%d) - opcode: %u, param: %d", err, cmd->opcode,
			cmd->param);
	}
}

/**
 * @brief  Callback handler for received notifications.
 *
 * @note	This callback will be triggered when a notification has been received.
 *		Used by the client.
 */
static void mcc_cmd_notification_cb(struct bt_conn *conn, int err, const struct mpl_cmd_ntf *ntf)
{
	LOG_DBG("mcc_cmd_ntf_cb");

	if (err) {
		LOG_ERR("MCC: cmd ntf error (%d) - opcode: %u, result: %u", err,
			ntf->requested_opcode, ntf->result_code);
	}
}

/**
 * @brief  Callback handler for reading media state.
 *
 * @note	This callback will be triggered when the client has asked to read
 *		the current state of the media player.
 */
static void mcc_read_media_state_cb(struct bt_conn *conn, int err, uint8_t state)
{
	LOG_DBG("mcc_read_media_cb, state: %d", state);

	if (err) {
		LOG_ERR("MCC: Media State read failed (%d)", err);
		return;
	}

	media_player_state = state;
}

/**
 * @brief  Callback handler for received MCS commands.
 *
 * @note	This callback will be triggered when the server has received a
 *		command from the client or the commander.
 */
static void mcs_command_recv_cb(struct media_player *plr, int err,
				const struct mpl_cmd_ntf *cmd_ntf)
{
	if (err) {
		LOG_ERR("Command failed (%d)", err);
		return;
	}

	LOG_DBG("Received opcode: %d", cmd_ntf->requested_opcode);

	if (cmd_ntf->requested_opcode == BT_MCS_OPC_PLAY) {
		play_pause_cb(true);
	} else if (cmd_ntf->requested_opcode == BT_MCS_OPC_PAUSE) {
		play_pause_cb(false);
	} else {
		LOG_WRN("Unsupported opcode: %d", cmd_ntf->requested_opcode);
	}
}

/**
 * @brief  Callback handler for getting the current state of the media player.
 *
 * @note	This callback will be triggered when the server has asked for the
 *		current state of its local media player.
 */
static void mcs_media_state_cb(struct media_player *plr, int err, uint8_t state)
{
	if (err) {
		LOG_ERR("Media state failed (%d)", err);
		return;
	}

	media_player_state = state;
}

/**
 * @brief  Callback handler for getting a pointer to the local media player.
 *
 * @note	This callback will be triggered during initialization when the
 *		local media player is ready.
 */
static void mcs_local_player_instance_cb(struct media_player *player, int err)
{
	int ret;
	struct mpl_cmd cmd;

	if (err) {
		LOG_ERR("Local player instance failed (%d)", err);
		return;
	}

	LOG_DBG("Received local player");

	local_player = player;

	cmd.opcode = BT_MCS_OPC_PLAY;

	/* Since the media player is default paused when initialized, we
	 * send a play command when the first stream is enabled
	 */
	ret = media_proxy_ctrl_send_command(local_player, &cmd);
	if (ret) {
		LOG_WRN("Failed to set media proxy state to play: %d", ret);
	}
}

/**
 * @brief	Send command to either local media player or peer
 *
 * @param	conn	Pointer to the conn to send the command to
 * @param	cmd	Command to send
 *
 * @return	0 for success, error otherwise.
 */
static int mpl_cmd_send(struct bt_conn *conn, struct mpl_cmd *cmd)
{
	int ret;
	int any_failures = 0;

	if (IS_ENABLED(CONFIG_BT_MCS)) {
		ret = media_proxy_ctrl_send_command(local_player, cmd);
		if (ret) {
			LOG_WRN("Failed to send command: %d", ret);
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_BT_MCC)) {
		if (conn != NULL) {
			int idx = mcc_peer_index_get(conn);

			if (idx < 0) {
				LOG_ERR("Unable to find mcc_peer");
				return idx;
			}

			if (mcc_peer[idx].mcp_mcs_disc_status == FINISHED) {
				ret = bt_mcc_send_cmd(mcc_peer[idx].conn, cmd);
				if (ret) {
					LOG_WRN("Failed to send command: %d", ret);
					return ret;
				}
			} else {
				LOG_WRN("MCS discovery has not finished: %d",
					mcc_peer[idx].mcp_mcs_disc_status);
				return -EBUSY;
			}

			return 0;
		}

		/* Send cmd to all peers connected and has finished discovery */
		for (uint8_t i = 0; i < ARRAY_SIZE(mcc_peer); i++) {
			if (mcc_peer[i].conn != NULL) {
				if (mcc_peer[i].mcp_mcs_disc_status == FINISHED) {
					ret = bt_mcc_send_cmd(mcc_peer[i].conn, cmd);
					if (ret) {
						LOG_WRN("Failed to send command: %d", ret);
						any_failures = ret;
					}
				} else {
					LOG_WRN("MCS discovery has not finished: %d",
						mcc_peer[i].mcp_mcs_disc_status);
					any_failures = -EBUSY;
				}
			}
		}
	}

	if (any_failures) {
		return any_failures;
	}

	return 0;
}

int bt_content_ctrl_media_discover(struct bt_conn *conn)
{
	int ret;

	if (!IS_ENABLED(CONFIG_BT_MCC)) {
		LOG_ERR("MCC not enabled");
		return -ECANCELED;
	}

	if (conn == NULL) {
		LOG_ERR("Invalid conn pointer");
		return -EINVAL;
	}

	int idx = mcc_peer_index_get(conn);

	if (idx == -ESRCH) {
		idx = mcc_peer_index_free_get();
		if (idx < 0) {
			LOG_WRN("Error getting free index: %d", idx);
			return idx;
		}

		mcc_peer[idx].conn = conn;
	}

	if (mcc_peer[idx].mcp_mcs_disc_status == FINISHED ||
	    mcc_peer[idx].mcp_mcs_disc_status == IN_PROGRESS) {
		return -EALREADY;
	}

	mcc_peer[idx].mcp_mcs_disc_status = IN_PROGRESS;

	ret = bt_mcc_discover_mcs(conn, true);
	if (ret) {
		mcc_peer[idx].mcp_mcs_disc_status = IDLE;
		return ret;
	}

	return 0;
}

int bt_content_ctrl_media_state_update(struct bt_conn *conn)
{
	if (!IS_ENABLED(CONFIG_BT_MCC)) {
		LOG_ERR("MCC not enabled");
		return -ECANCELED;
	}

	int idx = mcc_peer_index_get(conn);

	if (idx < 0) {
		LOG_WRN("Unable to look up conn pointer: %d", idx);
		return idx;
	}

	if (mcc_peer[idx].mcp_mcs_disc_status != FINISHED) {
		LOG_WRN("MCS discovery has not finished");
		return -EBUSY;
	}

	return bt_mcc_read_media_state(conn);
}

int bt_content_ctrl_media_play(struct bt_conn *conn)
{
	int ret;
	struct mpl_cmd cmd;

	if (media_player_state != BT_MCS_MEDIA_STATE_PLAYING &&
	    media_player_state != BT_MCS_MEDIA_STATE_PAUSED) {
		LOG_ERR("Invalid state: %d", media_player_state);
		return -ECANCELED;
	}

	if (media_player_state == BT_MCS_MEDIA_STATE_PLAYING) {
		LOG_WRN("Already in a playing state");
		return -EAGAIN;
	}

	cmd.opcode = BT_MCS_OPC_PLAY;
	cmd.use_param = false;

	ret = mpl_cmd_send(conn, &cmd);
	if (ret) {
		return ret;
	}

	return 0;
}

int bt_content_ctrl_media_pause(struct bt_conn *conn)
{
	int ret;
	struct mpl_cmd cmd;

	if (media_player_state != BT_MCS_MEDIA_STATE_PLAYING &&
	    media_player_state != BT_MCS_MEDIA_STATE_PAUSED) {
		LOG_ERR("Invalid state: %d", media_player_state);
		return -ECANCELED;
	}

	if (media_player_state == BT_MCS_MEDIA_STATE_PAUSED) {
		LOG_WRN("Already in a paused state");
		return -EAGAIN;
	}

	cmd.opcode = BT_MCS_OPC_PAUSE;
	cmd.use_param = false;

	ret = mpl_cmd_send(conn, &cmd);
	if (ret) {
		return ret;
	}

	return 0;
}

int bt_content_ctrl_media_conn_disconnected(struct bt_conn *conn)
{
	int idx = mcc_peer_index_get(conn);

	if (idx < 0) {
		LOG_WRN("Unable to look up conn pointer: %d", idx);
		return idx;
	}

	LOG_DBG("MCS discover state reset due to disconnection");
	mcc_peer[idx].mcp_mcs_disc_status = IDLE;
	mcc_peer[idx].conn = NULL;
	return 0;
}

int bt_content_ctrl_media_client_init(void)
{
	if (!IS_ENABLED(CONFIG_BT_MCC)) {
		LOG_ERR("MCC not enabled");
		return -ECANCELED;
	}

	static struct bt_mcc_cb mcc_cb;

	mcc_cb.discover_mcs = mcc_discover_mcs_cb;
	mcc_cb.send_cmd = mcc_send_command_cb;
	mcc_cb.cmd_ntf = mcc_cmd_notification_cb;
	mcc_cb.read_media_state = mcc_read_media_state_cb;
	return bt_mcc_init(&mcc_cb);
}

int bt_content_ctrl_media_server_init(bt_content_ctrl_media_play_pause_cb play_pause)
{
	int ret;

	if (!IS_ENABLED(CONFIG_BT_MCS)) {
		LOG_ERR("MCS not enabled");
		return -ECANCELED;
	}

	static struct media_proxy_ctrl_cbs mcs_cb;

	play_pause_cb = play_pause;

	ret = media_proxy_pl_init();
	if (ret) {
		LOG_ERR("Failed to init media proxy: %d", ret);
		return ret;
	}

	mcs_cb.command_recv = mcs_command_recv_cb;
	mcs_cb.media_state_recv = mcs_media_state_cb;
	mcs_cb.local_player_instance = mcs_local_player_instance_cb;

	ret = media_proxy_ctrl_register(&mcs_cb);
	if (ret) {
		LOG_ERR("Could not init mpl: %d", ret);
		return ret;
	}

	return 0;
}
