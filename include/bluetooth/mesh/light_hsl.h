/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file
 * @defgroup bt_mesh_light_hsl Light HSL models
 * @{
 * @brief API for the Light HSL models.
 */

#ifndef BT_MESH_LIGHT_HSL_H__
#define BT_MESH_LIGHT_HSL_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_MESH_LIGHT_HSL_MIN 0
#define BT_MESH_LIGHT_HSL_MAX UINT16_MAX
#define BT_MESH_LIGHT_HSL_OP_RANGE_DEFAULT                                     \
	{                                                                      \
		.min = BT_MESH_LIGHT_HSL_MIN, .max = BT_MESH_LIGHT_HSL_MAX,    \
	}

/** Common Light HSL parameters */
struct bt_mesh_light_hsl {
	/** Lightness level */
	uint16_t lightness;
	/** Hue level */
	uint16_t hue;
	/** Saturation level */
	uint16_t saturation;
};

/** Light Hue and Saturation parameters */
struct bt_mesh_light_hue_sat {
	/** Hue level */
	uint16_t hue;
	/** Saturation level */
	uint16_t saturation;
};

/** Common Light HSL set message parameters. */
struct bt_mesh_light_hsl_params {
	/** HSL set parameters */
	struct bt_mesh_light_hsl params;
	/**
	 * Transition time parameters for the state change, or NULL.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	const struct bt_mesh_model_transition *transition;
};

/** Hue move set parameters */
struct bt_mesh_light_hue_move {
	/** Translation to make for every transition step. */
	int16_t delta;
	/**
	 * Transition parameters. @c delay indicates time until the move
	 * should start, @c time indicates the amount of time each
	 * @c delta step should take.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	const struct bt_mesh_model_transition *transition;
};

/** Common Light HSL status message parameters. */
struct bt_mesh_light_hsl_status {
	/** Status parameters */
	struct bt_mesh_light_hsl params;
	/** Remaining time for the state change (ms). */
	int32_t remaining_time;
};

/** Common Light Hue set message parameters. */
struct bt_mesh_light_hue {
	/** Level to set */
	uint16_t lvl;
	/**
	 * Transition time parameters for the state change, or NULL.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	const struct bt_mesh_model_transition *transition;
	/**
	 * Direction of the Hue state change, if @c transition is present.
	 *
	 * If positive, the current value should be incremented even if the target value is less
	 * than the current value. If negative, the current value should be decremented even if the
	 * target value is greater than the current value. The application is responsible for
	 * wrapping the Hue state around when the value reaches the end of the type range.
	 *
	 * @note Set by the Light Hue Server model.
	 */
	int direction;
};

/** Common Light Saturation set message parameters. */
struct bt_mesh_light_sat {
	/** Level to set */
	uint16_t lvl;
	/**
	 * Transition time parameters for the state change, or NULL.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	const struct bt_mesh_model_transition *transition;
};

/** Common Light Hue status message parameters. */
struct bt_mesh_light_hue_status {
	/** Current level */
	uint16_t current;
	/** Target level */
	uint16_t target;
	/** Remaining time for the state change (ms). */
	int32_t remaining_time;
};

/** Common Light Saturation status message parameters. */
struct bt_mesh_light_sat_status {
	/** Current level */
	uint16_t current;
	/** Target level */
	uint16_t target;
	/** Remaining time for the state change (ms). */
	int32_t remaining_time;
};

/** Light HSL range parameters. */
struct bt_mesh_light_hsl_range {
	/** Minimum range value */
	uint16_t min;
	/** Maximum range value */
	uint16_t max;
};

/** Light HSL range set message parameters. */
struct bt_mesh_light_hue_sat_range {
	/** Minimum hue and saturation */
	struct bt_mesh_light_hue_sat min;
	/** Maximum hue and saturation */
	struct bt_mesh_light_hue_sat max;
};

/** Light HSL range status message parameters. */
struct bt_mesh_light_hsl_range_status {
	/** Range set status code */
	enum bt_mesh_model_status status_code;
	/** Hue and saturation range */
	struct bt_mesh_light_hue_sat_range range;
};

/** @brief Convert HSL to a 16 bit RGB channel value.
 *
 *  Uses the standard conversion formula to convert HSL to RGB, as described in
 *  https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB. Note that the accuracy
 *  of the conversion depends on the characteristics of the light source
 *  hardware, and may need correction for each channel.
 *
 *  @param[in] hsl HSL value.
 *  @param[in] ch RGB color channel.
 *
 *  @return The value of the chosen RGB channel for the given HSL value.
 */
uint16_t bt_mesh_light_hsl_to_rgb(const struct bt_mesh_light_hsl *hsl,
				  enum bt_mesh_rgb_ch ch);

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_LIGHT_HSL_OP_GET BT_MESH_MODEL_OP_2(0x82, 0x6D)
#define BT_MESH_LIGHT_HUE_OP_GET BT_MESH_MODEL_OP_2(0x82, 0x6E)
#define BT_MESH_LIGHT_HUE_OP_SET BT_MESH_MODEL_OP_2(0x82, 0x6F)
#define BT_MESH_LIGHT_HUE_OP_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x70)
#define BT_MESH_LIGHT_HUE_OP_STATUS BT_MESH_MODEL_OP_2(0x82, 0x71)
#define BT_MESH_LIGHT_SAT_OP_GET BT_MESH_MODEL_OP_2(0x82, 0x72)
#define BT_MESH_LIGHT_SAT_OP_SET BT_MESH_MODEL_OP_2(0x82, 0x73)
#define BT_MESH_LIGHT_SAT_OP_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x74)
#define BT_MESH_LIGHT_SAT_OP_STATUS BT_MESH_MODEL_OP_2(0x82, 0x75)
#define BT_MESH_LIGHT_HSL_OP_SET BT_MESH_MODEL_OP_2(0x82, 0x76)
#define BT_MESH_LIGHT_HSL_OP_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x77)
#define BT_MESH_LIGHT_HSL_OP_STATUS BT_MESH_MODEL_OP_2(0x82, 0x78)
#define BT_MESH_LIGHT_HSL_OP_TARGET_GET BT_MESH_MODEL_OP_2(0x82, 0x79)
#define BT_MESH_LIGHT_HSL_OP_TARGET_STATUS BT_MESH_MODEL_OP_2(0x82, 0x7A)
#define BT_MESH_LIGHT_HSL_OP_DEFAULT_GET BT_MESH_MODEL_OP_2(0x82, 0x7B)
#define BT_MESH_LIGHT_HSL_OP_DEFAULT_STATUS BT_MESH_MODEL_OP_2(0x82, 0x7C)
#define BT_MESH_LIGHT_HSL_OP_RANGE_GET BT_MESH_MODEL_OP_2(0x82, 0x7D)
#define BT_MESH_LIGHT_HSL_OP_RANGE_STATUS BT_MESH_MODEL_OP_2(0x82, 0x7E)
#define BT_MESH_LIGHT_HSL_OP_DEFAULT_SET BT_MESH_MODEL_OP_2(0x82, 0x7F)
#define BT_MESH_LIGHT_HSL_OP_DEFAULT_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x80)
#define BT_MESH_LIGHT_HSL_OP_RANGE_SET BT_MESH_MODEL_OP_2(0x82, 0x81)
#define BT_MESH_LIGHT_HSL_OP_RANGE_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x82)

#define BT_MESH_LIGHT_HSL_MSG_LEN_GET 0
#define BT_MESH_LIGHT_HSL_MSG_MINLEN_SET 7
#define BT_MESH_LIGHT_HSL_MSG_MAXLEN_SET 9
#define BT_MESH_LIGHT_HSL_MSG_MINLEN_STATUS 6
#define BT_MESH_LIGHT_HSL_MSG_MAXLEN_STATUS 7
#define BT_MESH_LIGHT_HSL_MSG_MINLEN_HUE 3
#define BT_MESH_LIGHT_HSL_MSG_MAXLEN_HUE 5
#define BT_MESH_LIGHT_HSL_MSG_MINLEN_HUE_STATUS 2
#define BT_MESH_LIGHT_HSL_MSG_MAXLEN_HUE_STATUS 5
#define BT_MESH_LIGHT_HSL_MSG_MINLEN_SAT 3
#define BT_MESH_LIGHT_HSL_MSG_MAXLEN_SAT 5
#define BT_MESH_LIGHT_HSL_MSG_MINLEN_SAT_STATUS 2
#define BT_MESH_LIGHT_HSL_MSG_MAXLEN_SAT_STATUS 5
#define BT_MESH_LIGHT_HSL_MSG_LEN_DEFAULT 6
#define BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_SET 8
#define BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_STATUS 9

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_HSL_H__ */

/** @} */
