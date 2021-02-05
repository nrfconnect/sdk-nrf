/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file
 * @defgroup bt_mesh_model_types Common model types
 * @{
 * @brief Collection of types exposed in the model definitions.
 */

#ifndef BT_MESH_MODEL_TYPES_H__
#define BT_MESH_MODEL_TYPES_H__

#include <bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum permissible transition time in milliseconds */
#define BT_MESH_MODEL_TRANSITION_TIME_MAX_MS (10 * 60 * MSEC_PER_SEC * 0x3e)

/** Delay field step factor in milliseconds */
#define BT_MESH_MODEL_DELAY_TIME_STEP_FACTOR_MS (5)
/** Maximum permissible delay time in milliseconds */
#define BT_MESH_MODEL_DELAY_TIME_MAX_MS                                        \
	(UINT8_MAX * BT_MESH_MODEL_DELAY_TIME_STEP_FACTOR_MS)

/** Generic Transition parameters for the model messages.
 *
 * @note Time can not be larger than @ref BT_MESH_MODEL_TRANSITION_TIME_MAX_MS
 *       and delay can not be larger than @ref BT_MESH_MODEL_DELAY_TIME_MAX_MS.
 */
struct bt_mesh_model_transition {
	uint32_t time; /**< Transition time value in milliseconds */
	uint32_t delay; /**< Message execution delay in milliseconds */
};

/**
 * Transaction ID context, storing information about the previous
 * transaction in model spec messages.
 */
struct bt_mesh_tid_ctx {
	uint16_t src; /**< Source address. */
	uint16_t dst; /**< Destination address. */
	uint64_t timestamp; /**< System uptime of the transaction. */
	uint8_t tid; /**< Transaction ID. */
};

/**
 * Acknowledged message context for tracking the status of model messages
 * pending a response.
 */
struct bt_mesh_model_ack_ctx {
	struct k_sem sem; /**< Sync semaphore. */
	uint32_t op; /**< Opcode we're waiting for. */
	uint16_t dst; /**< Address of the node that should respond. */
	void *user_data; /**< User specific parameter. */
};

/** Model status values. */
enum bt_mesh_model_status {
	/** Command successfully processed. */
	BT_MESH_MODEL_SUCCESS,
	/** The provided value for range min cannot be set. */
	BT_MESH_MODEL_ERROR_INVALID_RANGE_MIN,
	/** The provided value for range max cannot be set. */
	BT_MESH_MODEL_ERROR_INVALID_RANGE_MAX,
	/** Invalid status code. */
	BT_MESH_MODEL_STATUS_INVALID,
};

/** RGB color channels */
enum bt_mesh_rgb_ch {
	BT_MESH_RGB_CH_RED,
	BT_MESH_RGB_CH_GREEN,
	BT_MESH_RGB_CH_BLUE,

	BT_MESH_RGB_CHANNELS,
};

/** @cond INTERNAL_HIDDEN
 * @def BT_MESH_MODEL_USER_DATA
 *
 * @brief Internal utility macro for type checking model user data.
 *
 * Produces a "Comparison of distinct pointer types" warning if @p _user_data
 * is of the wrong type.
 *
 * This macro abuses the C language a bit, but when used in the
 * @c BT_MESH_MODEL_ macros, it generates a compiler warning for a bug that is
 * otherwise very hard to detect, and relatively easy to make:
 *
 * As the @ref bt_mesh_model::user_data is a void pointer, it does not have
 * any type checking. The mesh model implementations wrap this macro, often
 * taking a pointer parameter to a context structure, passing it to the model
 * user data. As the @c BT_MESH_MODEL_ macros are used in listing of models,
 * users are likely to copy and paste them, but only change the suffix.
 * If they do this, but fail to change the context pointer, the resulting
 * misbehavior will be confusing, dangerous and potentially hard to detect.
 *
 * @param[in] _type Expected type of the user data.
 * @param[in] _user_data User data pointer.
 */
#define BT_MESH_MODEL_USER_DATA(_type, _user_data)                             \
	(((_user_data) == (_type *)0) ? NULL : (_user_data))
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_MODEL_TYPES_H__ */

/** @} */
