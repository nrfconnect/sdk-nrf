/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup audio_app_bt_stream
 * @{
 * @defgroup le_audio_tx Functions for LE Audio TX functionality.
 * @{
 * @brief Helper functions to manage LE Audio TX functionality.
 */

#ifndef _LE_AUDIO_TX_H_
#define _LE_AUDIO_TX_H_

#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>

#include "le_audio.h"

struct le_audio_tx_info {
	struct stream_index idx;
	struct bt_cap_stream *cap_stream;
	uint8_t audio_location;
};

struct bt_le_audio_tx_ctx;

#define HCI_ISO_BUF_PER_CHAN 2

#if defined(CONFIG_BT_ISO_MAX_CIG) && defined(CONFIG_BT_ISO_MAX_BIG)
#define GROUP_MAX (CONFIG_BT_ISO_MAX_BIG + CONFIG_BT_ISO_MAX_CIG)
#elif defined(CONFIG_BT_ISO_MAX_CIG)
#define GROUP_MAX CONFIG_BT_ISO_MAX_CIG
#elif defined(CONFIG_BT_ISO_MAX_BIG)
#define GROUP_MAX CONFIG_BT_ISO_MAX_BIG
#else
#error Neither CIG nor BIG defined
#endif

#if (defined(CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT) && defined(CONFIG_BT_BAP_UNICAST))
#define SUBGROUP_MAX (1 + CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT)
#elif (defined(CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT) && !defined(CONFIG_BT_BAP_UNICAST))
#define SUBGROUP_MAX CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT
#else
#define SUBGROUP_MAX 1
#endif

#define TX_STREAMS_MAX CONFIG_BT_ISO_MAX_CHAN

#define TX_BUF_NUM (GROUP_MAX * SUBGROUP_MAX * TX_STREAMS_MAX * HCI_ISO_BUF_PER_CHAN)

struct bt_le_audio_tx_ctx_layout {
	struct net_buf_pool *iso_tx_pool;
	struct {
		uint16_t iso_conn_handle;
		struct bt_iso_tx_info iso_tx;
		struct bt_iso_tx_info iso_tx_readback;
		atomic_t iso_tx_pool_alloc;
		bool hci_wrn_printed;
	} tx_info_arr[GROUP_MAX][SUBGROUP_MAX][TX_STREAMS_MAX];
	bool flush_next;
	uint32_t timestamp_ctlr_esti_us;
	bool timestamp_ctlr_esti_us_valid;
	uint32_t timestamp_ctlr_real_us_last;
	uint32_t timestamp_last_correction;
	uint32_t subequent_rapid_corrections;
	int64_t corr_diff_us;
	uint32_t timestamp_last_us;
	bool timestamp_last_us_valid;
};

#define BT_LE_AUDIO_TX_DEFINE(name)                                                                \
	NET_BUF_POOL_FIXED_DEFINE(name##_cfg_buf, TX_BUF_NUM,                                      \
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),                       \
				  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);                         \
	static struct bt_le_audio_tx_ctx_layout name##_ctx = {                                     \
		.iso_tx_pool = &name##_cfg_buf,                                                    \
	};                                                                                         \
	static struct bt_le_audio_tx_ctx *name = (struct bt_le_audio_tx_ctx *)&name##_ctx

/**
 * @brief	Allocates buffers and sends data to the controller.
 *
 * @note	Send all available channels in a single call.
 *		Do not call this for each channel.
 *
 * @param[in]	ctx		Pointer to TX context.
 * @param[in]	audio_frame	Pointer to the encoded audio data.
 * @param[in]	tx		Pointer to an array of le_audio_tx_info elements.
 * @param[in]	num_tx		Number of elements in @p tx.
 *
 * @retval	0		Success.
 * @retval	-EACCES		@p ctx is NULL.
 * @retval	-EINVAL		Invalid arguments or invalid audio/frame/stream configuration.
 * @retval	-ECANCELED	Current call is intentionally flushed due to previous late timing.
 * @retval	-ETIMEDOUT	Controller timestamp indicates late delivery (empty packet(s) on
 * air).
 * @retval	-EIO		No stream was sent successfully.
 *
 * @return	Other negative error codes may be propagated from timestamp/HCI operations.
 */
int bt_le_audio_tx_send(struct bt_le_audio_tx_ctx *ctx, struct net_buf const *const audio_frame,
			struct le_audio_tx_info *tx, uint8_t num_tx);

/**
 * @brief	Initializes a stream. Must be called when a TX stream is started.
 *
 * @param[in]	ctx		Pointer to TX context.
 * @param[in]	stream_idx	Stream index.
 *
 * @retval	-EACCES		The module has not been initialized.
 * @retval	0		Success.
 */
int bt_le_audio_tx_stream_started(struct bt_le_audio_tx_ctx *ctx, struct stream_index stream_idx);

/**
 * @brief	Frees a TX buffer. Must be called when a TX stream has been sent.
 *
 * @param[in]	ctx		Pointer to TX context.
 * @param[in]	stream_idx	Stream index.
 *
 * @retval	-EACCES		The module has not been initialized.
 * @retval	0		Success.
 */
int bt_le_audio_tx_stream_sent(struct bt_le_audio_tx_ctx *ctx, struct stream_index stream_idx);

/**
 * @brief	Initializes the TX path for ISO transmission.
 *
 * @param[in]	ctx		Pointer to TX context.
 *
 * @retval	-EINVAL		@p ctx is NULL or required configuration is missing.
 * @retval	0		Success.
 */
int bt_le_audio_tx_init(struct bt_le_audio_tx_ctx *ctx);

/**
 * @}
 * @}
 */


#endif /* _LE_AUDIO_TX_H_ */
