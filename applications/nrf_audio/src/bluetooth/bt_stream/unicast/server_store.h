/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup audio_app_bt_stream
 * @{
 * @defgroup server_store Storage on unicast client of remote unicast servers.
 * @{
 * @brief Helper functions to manage how a unicast client (gateway side) stores the
 * unicast server(s) (headset side) it is connected to.
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
	uint32_t locations;
	struct bt_bap_lc3_preset lc3_preset[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	struct bt_audio_codec_cap codec_caps[CONFIG_CODEC_CAP_COUNT_MAX];
	size_t num_codec_caps;
	struct bt_bap_ep *eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	size_t num_eps;
	enum bt_audio_context supported_ctx;
	enum bt_audio_context available_ctx;
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
 * @brief	Function for bt_bap_unicast_group_foreach_stream().
 *
 * @param[in]	server		The audio server.
 * @param[in]	user_data	User data.
 *
 * @retval	false	Continue iterating.
 * @retval	true	Stop iterating.
 */
typedef bool (*srv_store_foreach_func_t)(struct server_store *server, void *user_data);

/**
 * @brief	Iterate through all stored servers and call the given function for each.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	func		Function to call for each server.
 * @param[in]	user_data	User data to pass to the function.
 *
 * @retval	0		Success.
 * @retval	-EINVAL		Function pointer is NULL.
 * @retval	-ECANCELED	The iteration was stopped by the callback function.
 */
int srv_store_foreach_server(srv_store_foreach_func_t func, void *user_data);

/**
 * @brief Validate the codec configuration preset.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	new			Pointer to the new codec configuration.
 * @param[in]	existing		Pointer to the existing codec configuration.
 * @param[in]	pref_sample_rate_value	Preferred sample rate value.
 *
 * @retval	true	The new preset should overwrite the existing one.
 * @retval	false	Otherwise.
 */
bool srv_store_preset_validated(struct bt_audio_codec_cfg const *const new,
				struct bt_audio_codec_cfg const *const existing,
				uint8_t pref_sample_rate_value);

/**
 * @brief Search for a common presentation delay across all server Audio Stream Endpoints (ASEs) in
 * a given @p unicast_group for the given direction.
 *
 * This function will try to satisfy the preferred presentation delay for all
 * ASEs. If that is not possible, it will try to satisfy the max and min values.
 *
 * @note srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	stream			Pointer to a new stream to be started
 * @param[out]	computed_pres_dly_us	Pointer to store the computed presentation delay in
 *					microseconds.
 * @param[out]	existing_pres_dly_us	Pointer to store the existing presentation delay in
 *					microseconds.
 * @param[in]	server_qos_pref		Pointer to the preferred QoS configuration.
 * @param[out]	group_reconfig_needed	True if a group reconfiguration is needed.
 * @param[in]	unicast_group		Pointer to the unicast group to search within.
 *
 * @retval	0	Success, negative error code on failure.
 * @retval	-ESPIPE	There is no common presentation delay found.
 * @retval	-EINVAL	Illegal argument(s), or submitted streams are in different groups.
 */
int srv_store_pres_dly_find(struct bt_bap_stream *stream, uint32_t *computed_pres_dly_us,
			    uint32_t *existing_pres_dly_us,
			    struct bt_bap_qos_cfg_pref const *server_qos_pref,
			    bool *group_reconfig_needed,
			    struct bt_cap_unicast_group *unicast_group);

/**
 * @brief	Set the valid locations of a unicast server.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in] conn	Pointer to the connection.
 * @param[in] dir	Direction to store.
 * @param[in] loc	Location to store.
 *
 * @retval	0		Success.
 * @retval	-ENOENT		Server not found.
 * @retval	-ENOTCONN	Server conn is NULL.
 */
int srv_store_location_set(struct bt_conn const *const conn, enum bt_audio_dir dir,
			   enum bt_audio_location loc);

/**
 * @brief	Check which codec capabilities are valid.
 *
 * @note	Will go through each of the codec_caps for a given direction for a given server.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	conn			Pointer to the connection.
 * @param[in]	dir			Direction to check for (sink or source).
 * @param[out]	valid_codec_caps	Bit field to be populated with valid codec capabilities.
 * @param[in]	client_supp_cfgs	(Reserved) Supported configurations of the unicast client.
 * @param[in]	num_client_supp_cfgs	(Reserved) Number of supported configurations in the array.
 *
 * @retval	0		Success.
 * @retval	-ENOENT		Server not found.
 * @retval	-ENOTCONN	Server conn is NULL.
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
 * @param[in]	stream	Pointer to the stream.
 * @param[out]	server	Pointer to the server. NULL if not found.
 *
 * @retval	0	Success.
 * @retval	-ENOENT	Server not found.
 */
int srv_store_from_stream_get(struct bt_bap_stream const *const stream,
			      struct server_store **server);

/**
 * @brief	Get the number of endpoints in a given state across all stored servers.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	state	State to search for.
 * @param[in]	dir	Direction to filter on.
 *
 * @return	Number of endpoints in the given state and direction.
 */
int srv_store_all_ep_state_count(enum bt_bap_ep_state state, enum bt_audio_dir dir);

/**
 * @brief	Store the available audio context for a server based on conn.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	conn	Pointer to the connection.
 * @param[in]	snk_ctx	Sink context.
 * @param[out]	src_ctx	Source context.
 *
 * @retval	0	Success.
 * @retval	-EINVAL	Illegal argument(s)
 * @retval	-ENOENT	Server not found.
 */
int srv_store_avail_context_set(struct bt_conn *conn, enum bt_audio_context snk_ctx,
				enum bt_audio_context src_ctx);

/**
 * @brief	Store the codec capabilities for a given server based on conn.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	conn	Pointer to the connection.
 * @param[in]	dir	Direction of the capability.
 * @param[out]	codec	Codec capability to store.
 *
 * @retval	0		Success.
 * @retval	-EINVAL		Illegal argument(s)
 * @retval	-ENOMEM		Out of memory.
 * @retval	-ENOTCONN	Server conn is NULL.
 */
int srv_store_codec_cap_set(struct bt_conn const *const conn, enum bt_audio_dir dir,
			    struct bt_audio_codec_cap const *const codec);

/**
 * @brief	Get a server from the address.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	addr	Pointer to the address of the server.
 * @param[out]	server	Pointer to the server. NULL if not found.
 *
 * @retval	0	Success.
 * @retval	-ENOENT	Server not found in storage.
 */
int srv_store_from_addr_get(bt_addr_le_t const *const addr, struct server_store **server);

/**
 * @brief	Check if there is a server with the given address in the server store.
 *
 * @param[in]	addr	Pointer to the address of the server.
 *
 * @retval	true	The server exists.
 * @retval	false	The server does not exist.
 */
bool srv_store_server_exists(bt_addr_le_t const *const addr);

/**
 * @brief	Get a server from the conn pointer.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	conn	Pointer to the connection to find the server for.
 * @param[out]	server	Pointer to the server. NULL if not found.
 *
 * @retval	0		Success.
 * @retval	-ENOENT		Server not found in storage.
 * @retval	-ENOTCONN	Server conn is NULL.
 */
int srv_store_from_conn_get(struct bt_conn const *const conn, struct server_store **server);

/**
 * @brief	Get the number of stored servers.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @note	Do not iterate through the servers, as they may not be stored consecutively in
 *		memory. Use the srv_store_foreach_func_t function.
 *
 * @retval	Number of servers.
 * @retval	-EINVAL	Non-consecutive server storage detected.
 * @retval	-ENOENT	Server not found in storage.
 */
int srv_store_num_get(void);

/**
 * @brief	Add a server to the storage based on conn.
 *
 * @note	This function should not be used if the peer uses a resolvable address
 *		which has not yet been resolved, or any other changing address type.
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	conn	Pointer to the connection associated with the server to add, based on
 *			address.
 *
 * @retval	0		Success.
 * @retval	-EALREADY	The server already exists.
 * @retval	-ENOMEM		Out of memory.
 * @retval	Other negative.	Errors from underlying functions.
 */
int srv_store_add_by_conn(struct bt_conn *conn);

/**
 * @brief	Add a server to the storage based on address.
 *
 * @note	This function can be used when getting addresses from bonding information.
 *		At this point, there will only be an address associated with the server, no conn
 *		pointer, and so on. Hence, you must call srv_store_conn_update() when a connection
 *		is established.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	addr	Pointer to the address associated with the server to add.
 *
 * @retval	0		Success.
 * @retval	-EALREADY	The server already exists.
 * @retval	-ENOMEM		Out of memory.
 * @retval	Other negative.	Errors from underlying functions.
 */
int srv_store_add_by_addr(const bt_addr_le_t *addr);

/**
 * @brief	Update the conn pointer of an existing server, based on address.
 *
 * @note	This function can be used when an earlier pairing has been done to this server.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	conn	Pointer to the connection to write.
 * @param[in]	addr	Address to write.
 *
 * @retval	0		Success.
 * @retval	-ENOENT		Server not found.
 * @retval	-EPERM		Address does not match the connection's peer address.
 * @retval	-EACCES		Server already has a different conn assigned.
 * @retval	-EALREADY	Server is already assigned to the same conn.
 * @retval	Other negative.	Errors from underlying functions.
 */
int srv_store_conn_update(struct bt_conn *conn, bt_addr_le_t const *const addr);

/**
 * @brief	Clear the contents of a server based on conn pointer.
 *
 * @note	This function can be used in a disconnect callback.
 *		Will not clear the address or streams.
 *		Must only be called when there are no active connections.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	conn	Connection to the server to be cleared.
 *
 * @retval	0		Success, negative error code on failure.
 * @retval	Other negative.	Errors from underlying functions.
 */
int srv_store_clear_by_conn(struct bt_conn const *const conn);

/**
 * @brief	Remove a server by conn pointer.
 *
 * @note	Depending on the application, it is strongly recommended to call this function
 *		when an unbonded (untrusted) connection is terminated or the bond for that
 *		connection is cleared. If not, the address will still be stored, and other or new
 *		connections that maliciously present the same address will be recognized as a valid,
 *		previously stored server.
 *
 * @note	This function assumes that this conn will no longer be valid.
 *		Hence, it will remove a server with a (previously) active conn.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	conn	Connection to the server to be removed.
 *
 * @retval	0		Success.
 * @retval	-EACCES		Server has active conn.
 * @retval	Other negative.	Errors from underlying functions.
 */
int srv_store_remove_by_conn(struct bt_conn const *const conn);

/**
 * @brief	Remove a single stored server based on address.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	addr	Pointer to the address associated with the server to remove.
 *
 * @retval	0	Success.
 * @retval	Negative error code on failure.
 */
int srv_store_remove_by_addr(bt_addr_le_t const *const addr);

/**
 * @brief	Remove all servers.
 *
 * @note	Must only be called when there are no active connections.
 *
 * @note	srv_store_lock() must be called before accessing this function.
 *
 * @param[in]	force	Set true to remove servers even if they have active connection.
 *			Use false for normal operation, use true with caution.
 *
 * @retval	0	Success.
 * @retval	Negative error code on failure.
 */
int srv_store_remove_all(bool force);

/**
 * @brief	Take the server store semaphore to lock down server store.
 * This is to prevent race conditions.
 *
 * @note	Do not use directly. Use the supplied defines.
 *
 * @param	timeout[in]	Waiting period to take the semaphore, or one of the special values
 *				K_NO_WAIT and K_FOREVER.
 * @param	file[in]	String to log the lock source.
 * @param	line[in]	String to log the lock line number.
 *
 * @retval	0	Lock taken. One can now operate on the server store.
 * @retval	-EBUSY	Returned without waiting.
 * @retval	-EAGAIN	Waiting period timed out, or the semaphore reset during the waiting period.
 */
int _srv_store_lock(k_timeout_t timeout, const char *file, int line);

#ifdef CONFIG_DEBUG
#define srv_store_lock(timeout) _srv_store_lock(timeout, __func__, __LINE__)
#else
#define srv_store_lock(timeout) _srv_store_lock(timeout, NULL, 0)
#endif

/**
 * @brief	Give the server store semaphore to unlock server store.
 */
void srv_store_unlock(void);

/**
 * @brief	Initialize the server store and clear all contents.
 *
 * @retval	0	Success.
 * @retval	Negative error code on failure.
 */
int srv_store_init(void);

/**
 * @}
 * @}
 */

#endif /* _SERVER_STORE_H_ */
