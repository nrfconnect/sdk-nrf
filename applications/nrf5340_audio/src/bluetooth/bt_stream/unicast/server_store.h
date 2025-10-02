/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SERVER_STORE_H_
#define _SERVER_STORE_H_

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/addr.h>

struct unicast_server_snk_vars {
	bool waiting_for_disc;
	/* PACS response. Location should be a superset of all codec locations. Bitfield */
	uint32_t locations;
	/* lc3_preset will propagate to the streams. */
	struct bt_bap_lc3_preset lc3_preset[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	struct bt_audio_codec_cap codec_caps[CONFIG_CODEC_CAP_COUNT_MAX];
	size_t num_codec_caps;
	/* One array for discovering the eps */
	struct bt_bap_ep *eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	size_t num_eps;
	enum bt_audio_context supported_ctx;
	/* Check this before calling unicast audio start */
	enum bt_audio_context available_ctx;
	/* We should have all info here. (Locations, stream status etc.) */
	struct bt_cap_stream cap_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
};

struct unicast_server_src_vars {
	bool waiting_for_disc;
	uint32_t locations;
	struct bt_bap_lc3_preset lc3_preset[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
	struct bt_audio_codec_cap codec_caps[CONFIG_CODEC_CAP_COUNT_MAX];
	size_t num_codec_caps;
	struct bt_bap_ep *eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
	size_t num_eps;
	enum bt_audio_context supported_ctx;
	enum bt_audio_context available_ctx;
	struct bt_cap_stream cap_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
};

/* This struct holds the parameters for a given unicast server/headset device */
struct server_store {
	char *name;
	bt_addr_le_t addr;
	struct bt_conn *conn;
	const struct bt_csip_set_coordinator_set_member *member;
	struct unicast_server_snk_vars snk;
	struct unicast_server_src_vars src;
};

struct client_supp_configs {
	enum bt_audio_codec_cap_freq freq;
	enum bt_audio_codec_cap_frame_dur dur;
	enum bt_audio_codec_cap_chan_count chan_count;
	struct bt_audio_codec_octets_per_codec_frame oct_per_codec_frame;
};

/**
 * @brief Validate the codec configuration preset.
 *
 * @param new  Pointer to the new codec configuration.
 * @param existing  Pointer to the existing codec configuration.
 * @param pref_sample_rate_value  Preferred sample rate value.
 *
 * @return true if the preset should overwrite the existing one, false otherwise.
 */
bool srv_store_preset_validated(struct bt_audio_codec_cfg *new, struct bt_audio_codec_cfg *existing,
				uint8_t pref_sample_rate_value);

/**
 * @brief Search for a common presentation delay across all server Audio Stream Endpoints (ASEs) for
 * the given direction. This function will try to satisfy the preferred presentation delay for all
 * ASEs. If that is not possible, it will try to satisfy the max and min values.
 *
 * @param common_pres_dly_us Pointer to store the new common presentation delay in microseconds.
 * @param dir	The direction of the Audio Stream Endpoints (ASEs) to search for.
 *
 * @note	This function will search across CIGs. This may not make sense, as the same
 * presentation delay is only mandated within a CIG. srv_store_lock() must be called before
 * accessing this function.
 *
 * @return 0 on success, negative error code on failure.
 * @return -ESPIPE if there is no common presentation delay found.
 * @return -EMLINK if the search was conducted across multiple CIGs
 */
int srv_store_pres_dly_find(struct bt_bap_stream *stream, uint32_t *computed_pres_dly_us,
			    uint32_t *existing_pres_dly_us,
			    struct bt_bap_qos_cfg_pref const *qos_cfg_pref_in,
			    bool *group_reconfig_needed);

/**
 * @brief	Set the valid locations of a unicast server.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in] conn  Pointer to the connection.
 * @param[in] dir   Direction to store.
 * @param[in] loc  Location to store.
 *
 * @return	0 on success.
 * @return	-ENOENT Server not found.
 */
int srv_store_location_set(struct bt_conn const *const conn, enum bt_audio_dir dir,
			   enum bt_audio_location loc);

/**
 * @brief	Check which codec capabilities are valid.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in] conn  Pointer to the connection.
 * @param[in] dir   Direction to check.
 * @param[out] valid_codec_caps  Bitfield will be populated with valid codec capabilities.
 * @param[in] client_supp_cfgs  Supported configs of the unicast client (Reserved).
 * @param[in] num_client_supp_cfgs  Number of supported configs in the array (Reserved).
 *
 * @return	0 on success.
 * @return	-ENOENT Server not found.
 */
int srv_store_valid_codec_cap_check(struct bt_conn const *const conn, enum bt_audio_dir dir,
				    uint32_t *valid_codec_caps,
				    struct client_supp_configs const **const client_supp_cfgs,
				    uint8_t num_client_supp_cfgs);

/**
 * @brief	Get a server based on stream pointer.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in] stream  Pointer to the stream.
 * @param[out] server  Pointer to the server structure to fill. NULL if not found.
 *
 * @return	0 on success.
 * @return	-ENOENT Server not found.
 */
int srv_store_from_stream_get(struct bt_bap_stream const *const stream,
			      struct server_store **server);

/**
 * @brief	Get the number of endpoints in a given state across all stored servers.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in] state  State to search for.
 * @param[in] dir   Direction to filter on.
 *
 * @return	Number of endpoints in the given state and direction.
 */
int srv_store_all_ep_state_count(enum bt_bap_ep_state state, enum bt_audio_dir dir);

/**
 * @brief	Store the available audio context for a server based on conn dst address.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in] conn  Pointer to the connection.
 * @param[in] snk_ctx   Sink context.
 * @param[out] src_ctx  Source context.
 *
 * @return	0 on success.
 * @return	-EINVAL Illegal argument(s)
 * @return	-ENOENT Server not found.
 */
int srv_store_avail_context_set(struct bt_conn *conn, enum bt_audio_context snk_ctx,
				enum bt_audio_context src_ctx);

/**
 * @brief	Store the codec capabilities for a given server based on conn dst address.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in] conn  Pointer to the connection.
 * @param[in] dir   Direction to store.
 * @param[out] codec  Codec capabilities to store.
 *
 * @return	0 on success.
 * @return	-EINVAL Illegal argument(s)
 * @return	-ENOMEM Out of memory.
 */
int srv_store_codec_cap_set(struct bt_conn const *const conn, enum bt_audio_dir dir,
			    struct bt_audio_codec_cap const *const codec);

/**
 * @brief	Get a server from the address.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in] addr  Pointer to the address to find the server for.
 * @param[out] server  Pointer to the server structure to fill. NULL if not found.
 *
 * @return	0 on success.
 * @return	-ENOENT Server not found in storage.
 */
int srv_store_from_addr_get(bt_addr_le_t const *const addr, struct server_store **server);

/**
 * @brief	Find if a stored server exists based on address.
 *
 * @param[in] addr  Pointer to the address to find the server for.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @return	true if the server exists, false otherwise.
 */
bool srv_store_server_exists(bt_addr_le_t const *const addr);

/**
 * @brief	Get a server from the dst address in the conn pointer.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in] conn  Pointer to the connection to find the server for.
 * @param[out] server  Pointer to the server structure to fill. NULL if not found.
 *
 * @return	0 on success.
 * @return	-ENOENT Server not found in storage.
 */
int srv_store_from_conn_get(struct bt_conn const *const conn, struct server_store **server);

/**
 * @brief	Get the number of stored servers.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	check_consecutive  Forces all the servers to be stored consecutively.
 * I.e. if the returned value is used as an iterator, all the servers must be stored consecutively.
 * If true, the function will return -EINVAL if the servers are not all stored consecutevly.
 *
 * @return	number of servers
 * @return	-EINVAL Non-consecutive server storage detected.
 */
int srv_store_num_get(bool check_consecutive);

/**
 * @brief	Get a server based on index.
 *
 * @param[out]	server  Pointer to the server at the given index.
 * @param[in]	index    Index of the server to retrieve.
 *
 * @note	When an entry is deleted, the remaining servers are not re-indexed.
 * Hence, there may be indexes which are vacant between other servers.
 * srv_store_lock() must be called before accessing this function.
 *
 * @return	0 on success.
 * @return	-EINVAL Illegal value
 * @return	-ENOENT No server found at index.
 * @retval	Other negative.	Errors from underlying functions.
 */
int srv_store_server_get(struct server_store **server, uint8_t index);

/**
 * @brief	Add a server to the storage based on conn.
 *
 * @param[in] conn  Pointer to the connection associated with the server to add, based on address.
 *
 * @note	This function should not be used if the peer uses a resolvable address
 * which has not yet been resolved, or any other changing address type.
 * srv_store_lock() must be called before accessing this function.
 *
 * @return	0 on success.
 * @return	-EALREADY The server already exists.
 * @return	-ENOMEM  Out of memory.
 * @retval	Other negative.	Errors from underlying functions.
 */
int srv_store_add_by_conn(struct bt_conn *conn);

/**
 * @brief	Add a server to the storage based on address.
 *
 * @param[in] addr  Pointer to the address associated with the server to add.
 *
 * @note	This function can be used when getting addresses from bonding information.
 *		At this point, there will only be an address associated with the server, no conn
 * pointer etc. Hence, it is required to call srv_store_conn_update() when a connection is
 * established. srv_store_lock() must be called before accessing this function.
 *
 * @return	0 on success.
 * @return	-EALREADY The server already exists.
 * @return	-ENOMEM  Out of memory.
 * @retval	Other negative.	Errors from underlying functions.
 */
int srv_store_add_by_addr(const bt_addr_le_t *addr);

/**
 * @brief	Update the conn pointer of an existing server based on address.
 *
 * @param[in] conn  Pointer to the connection to write.
 * @param[in] addr  Pointer to the address associated with the server to update.
 *
 *  * @note	This function can be used when a secure connection is established to an already
 * bonded device added by srv_store_add_by_addr(). srv_store_lock() must be called before accessing
 * this function.
 *
 * @return	0 on success.
 * @return	-ENOENT Server not found.
 * @return	-EPERM Address does not match the connection's peer address.
 * @return	-EACCES Server already has a different conn assigned.
 * @return	-EALREADY Server is already assigned to the same conn.
 * @retval	Other negative.	Errors from underlying functions.
 */
int srv_store_conn_update(struct bt_conn *conn, bt_addr_le_t const *const addr);

/**
 * @brief	Clear the contents of a server based on conn pointer.
 *
 * @note	This function can be used in a disconnect callback to clear all the contents of a
 * server. Must only be called when there are no active connections. srv_store_lock() must be called
 * before accessing this function.
 *
 * @return 0 on success, negative error code on failure.
 */
int srv_store_clear_by_conn(struct bt_conn const *const conn);

/**
 * @brief	Remove a single stored server based on conn pointer.
 *
 * @param[in] conn  Pointer to the connection associated with the server to remove.
 *
 * @note	Depending on the application, it is strongly recommended to call this function
 * when an unbonded/untrusted connection is terminated or the bond for that connection is cleared.
 * If not, the address will still be stored, and other/new connections which maliciously presents
 * the same address will be recognized as a valid previously stored server.
 * srv_store_lock() must be called before accessing this function.
 *
 * @return 0 on success, negative error code on failure.
 * @return	-EACCES Server has active conn.
 * @retval	Other negative.	Errors from underlying functions.
 */
int srv_store_remove_by_conn(struct bt_conn const *const conn);

/**
 * @brief	Remove a single stored server based on address.
 * @param[in] addr  Pointer to the address associated with the server to remove.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @return 0 on success, negative error code on failure.
 */
int srv_store_remove_by_addr(bt_addr_le_t const *const addr);

/**
 * @brief	Remove all stored servers.
 *
 * @note	Must only be called when there are no active connections.
 * srv_store_lock() must be called before accessing this function.
 *
 * @return 0 on success, negative error code on failure.
 */
int srv_store_remove_all(void);

/**
 * @brief Lock/take the server store semaphore.
 *
 * @param timeout	Waiting period to take the semaphore,
 *			or one of the special values K_NO_WAIT and K_FOREVER.
 * @param str		String to log the lock source.
 *
 * @retval 0 lock taken. One can now operate on the server store.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out, or the semaphore was reset during the waiting period.
 */
int _srv_store_lock(k_timeout_t timeout, const char *file, int line);

#ifdef CONFIG_DEBUG
#define srv_store_lock(timeout) _srv_store_lock(timeout, __func__, __LINE__)
#else
#define srv_store_lock(timeout) _srv_store_lock(timeout, NULL, 0)
#endif

/**
 * @brief Unlock/give the server store semaphore.
 *
 * @param str String to log the unlock source.
 */
void srv_store_unlock(void);

/**
 * @brief	Initializes the server store and clears all contents.
 *
 * @return 0 on success, negative error code on failure.
 */
int srv_store_init(void);

#endif /* _SERVER_STORE_H_ */
