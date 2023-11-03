/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_scene_srv Scene Server model
 * @{
 * @brief API for the Scene Server model.
 */

#ifndef BT_MESH_SCENE_SRV_H__
#define BT_MESH_SCENE_SRV_H__

#include <bluetooth/mesh/scene.h>
#include <zephyr/settings/settings.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_BT_MESH_SCENES_MAX
#define CONFIG_BT_MESH_SCENES_MAX 0
#endif

/** @def BT_MESH_SCENE_ENTRY_SIG
 *
 *  @brief Scene entry type definition for SIG models
 *
 *  @param[in] _name Name of the scene entry type
 */
#define BT_MESH_SCENE_ENTRY_SIG(_name)                                         \
	static const STRUCT_SECTION_ITERABLE(                                \
		bt_mesh_scene_entry, bt_mesh_scene_entry_sig_##_name)

/** @def BT_MESH_SCENE_ENTRY_VND
 *
 *  @brief Scene entry type definition for vendor models
 *
 *  @param[in] _name Name of the scene entry type
 */
#define BT_MESH_SCENE_ENTRY_VND(_name)                                         \
	static const STRUCT_SECTION_ITERABLE(                                \
		bt_mesh_scene_entry, bt_mesh_scene_entry_vnd_##_name)

struct bt_mesh_scene_srv;

/** @def BT_MESH_MODEL_SCENE_SRV
 *
 *  @brief Scene Server model composition data entry.
 *
 *  @param[in] _srv Pointer to a @ref bt_mesh_scene_srv instance.
 */
#define BT_MESH_MODEL_SCENE_SRV(_srv)                                          \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SCENE_SRV, _bt_mesh_scene_srv_op,    \
			 &(_srv)->pub,                                         \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_scene_srv,     \
						 _srv),                        \
			 &_bt_mesh_scene_srv_cb),                              \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SCENE_SETUP_SRV,                     \
			 _bt_mesh_scene_setup_srv_op, NULL,                    \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_scene_srv,     \
						 _srv),                        \
			 &_bt_mesh_scene_setup_srv_cb)

/** Scene Server model instance */
struct bt_mesh_scene_srv {
	/** All known scenes. */
	uint16_t all[CONFIG_BT_MESH_SCENES_MAX];
	/** Number of known scenes. */
	uint16_t count;

	/** @cond INTERNAL_HIDDEN */

	/** Next scene. */
	uint16_t next;
	/** Previous scene. */
	uint16_t prev;

	/** Largest number of pages used to store vendor model scene data. */
	uint8_t vndpages;
	/** Largest number of pages used to store SIG model scene data. */
	uint8_t sigpages;

	/** Linked list node for Scene Server list */
	sys_snode_t n;

	/** Transition timer */
	struct k_work_delayable work;
	/** Transition parameters. */
	struct bt_mesh_model_transition transition;

	/** TID context. */
	struct bt_mesh_tid_ctx tid;
	/** Composition data model pointer. */
	struct bt_mesh_model *model;
	/** Composition data setup model pointer. */
	struct bt_mesh_model *setup_mod;
	/** Publication state. */
	struct bt_mesh_model_pub pub;
	/** Publication message. */
	struct net_buf_simple pub_msg;
	/** Publication message buffer. */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(BT_MESH_SCENE_OP_STATUS,
					  BT_MESH_SCENE_MSG_MAXLEN_STATUS)];
	/** @endcond */
};

/** Scene entry type. */
struct bt_mesh_scene_entry {
	/** Model ID */
	union {
		/** SIG model ID */
		uint16_t sig;
		/** Vendor model ID */
		struct bt_mesh_mod_id_vnd vnd;
	} id;
	/** Longest scene data */
	size_t maxlen;

	/** @brief Store the current state as a scene.
	 *
	 *  The callback should fill the @c data with the current scene state
	 *  and return the number of bytes written. @c data is guaranteed to
	 *  fit @c maxlen number of bytes.
	 *
	 *  @param[in]  model Model to get the scene data of.
	 *  @param[out] data  Scene data buffer to fill. Fits @c maxlen bytes.
	 *
	 *  @return The number of bytes written to @c data or a negative value
	 *          on failure.
	 */
	ssize_t (*store)(struct bt_mesh_model *model, uint8_t data[]);

	/** @brief Recall a scene based on the given scene data.
	 *
	 *  When a scene is recalled, the Scene Server calls this callback for
	 *  every scene entry that has data for the recalled scene. The handler
	 *  shall start transitioning to the given scene with the given
	 *  transition parameters. If the transition is non-instantaneous, for example
	 *  when @ref bt_mesh_model_transition_time returns a nonzero value, the
	 *  handler shall publish a status message at the end of the transition.
	 *
	 *  @param[in] model      Model to restore the scene of.
	 *  @param[in] data       Scene data to restore.
	 *  @param[in] len        Scene data length.
	 *  @param[in] transition Transition parameters.
	 */
	void (*recall)(struct bt_mesh_model *model, const uint8_t data[],
		       size_t len, struct bt_mesh_model_transition *transition);

	/** @brief Recall a scene is completed
	 *
	 * This callback is called for each model that has a scene entry when
	 * recalling a scene data is done and @ref bt_mesh_scene_entry::recall
	 * callback has been called for all models. The model can use this
	 * callback when it needs to publish its state, which is a composite
	 * state and requires all corresponding models to recall their scene
	 * data first.
	 *
	 * Can be NULL.
	 *
	 *  @param[in] model      Model to restore the scene of.
	 */
	void (*recall_complete)(struct bt_mesh_model *model);
};

/** @brief Notify the Scene Server that a Scene entry has changed.
 *
 *  Whenever some state in the Scene has changed outside of Scene recall
 *  procedure, this function must be called to notify the Scene Server that
 *  the current Scene is no longer active.
 *
 *  @param[in] mod Model that invalidated the scene.
 */
void bt_mesh_scene_invalidate(struct bt_mesh_model *mod);

/** @brief Set the current Scene.
 *
 *  All Scene entries of the given Scene Server will transition to the given
 *  Scene according to the transition parameters.
 *
 *  @param[in] srv        Scene Server model.
 *  @param[in] scene      Scene to transition to. Cannot be
 *                        @ref BT_MESH_SCENE_NONE.
 *  @param[in] transition Transition parameters, or NULL to use the default
 *                        parameters.
 *
 *  @retval 0 Successfully transitioned to the given scene.
 *  @retval -EINVAL Invalid scene number or transition parameters.
 *  @retval -ENOENT No such scene.
 */
int bt_mesh_scene_srv_set(struct bt_mesh_scene_srv *srv, uint16_t scene,
			  struct bt_mesh_model_transition *transition);

/** @brief Publish the current Scene status.
 *
 *  @param[in] srv Scene Server model.
 *  @param[in] ctx Message context, or NULL to publish with the configured
 *                 parameters.
 *
 * @return 0 Successfully published the current Scene state.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_scene_srv_pub(struct bt_mesh_scene_srv *srv,
			 struct bt_mesh_msg_ctx *ctx);

/** @brief Get the current scene.
 *
 * This will return the scene that the scene servers is currently in. If
 * there is no currently active scene this will return @ref BT_MESH_SCENE_NONE.
 *
 *  @param[in] srv Scene Server model.
 *
 * @return Return the current scene for the scene server.
 */
uint16_t
bt_mesh_scene_srv_current_scene_get(const struct bt_mesh_scene_srv *srv);

/** @brief Get the target scene.
 *
 * This will return the scene that the scene servers is transiting into. If
 * there is no transition running this will return @ref BT_MESH_SCENE_NONE.
 *
 *  @param[in] srv Scene Server model.
 *
 * @return Return the target scene for the scene server.
 */
uint16_t
bt_mesh_scene_srv_target_scene_get(const struct bt_mesh_scene_srv *srv);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_cb _bt_mesh_scene_srv_cb;
extern const struct bt_mesh_model_op _bt_mesh_scene_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_scene_setup_srv_cb;
extern const struct bt_mesh_model_op _bt_mesh_scene_setup_srv_op[];
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SCENE_SRV_H__ */

/** @} */
