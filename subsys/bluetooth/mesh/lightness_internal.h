/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file
 * @brief Light Lightness model internal defines
 */

#ifndef LIGHTNESS_INTERNAL_H__
#define LIGHTNESS_INTERNAL_H__

#include <bluetooth/mesh/lightness.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BT_MESH_LIGHTNESS_LINEAR)
#define LIGHT_USER_REPR LINEAR
#else
#define LIGHT_USER_REPR ACTUAL
#endif /* CONFIG_BT_MESH_LIGHTNESS_LINEAR */

/** Power-up sequence and scene store/recall behaviors are controlled by Light LC server. */
#define LIGHTNESS_SRV_FLAG_EXTENDED_BY_LIGHT_CTRL 1

enum light_repr {
	ACTUAL,
	LINEAR,
};

enum bt_mesh_lightness_op_type {
	LIGHTNESS_OP_TYPE_GET,
	LIGHTNESS_OP_TYPE_SET,
	LIGHTNESS_OP_TYPE_SET_UNACK,
	LIGHTNESS_OP_TYPE_STATUS,
};

static inline uint32_t op_get(enum bt_mesh_lightness_op_type type,
				     enum light_repr repr)
{
	switch (type) {
	case LIGHTNESS_OP_TYPE_GET:
		return repr == ACTUAL ? BT_MESH_LIGHTNESS_OP_GET :
					BT_MESH_LIGHTNESS_OP_LINEAR_GET;
	case LIGHTNESS_OP_TYPE_SET:
		return repr == ACTUAL ? BT_MESH_LIGHTNESS_OP_SET :
					BT_MESH_LIGHTNESS_OP_LINEAR_SET;
	case LIGHTNESS_OP_TYPE_SET_UNACK:
		return repr == ACTUAL ? BT_MESH_LIGHTNESS_OP_SET_UNACK :
					BT_MESH_LIGHTNESS_OP_LINEAR_SET_UNACK;
	case LIGHTNESS_OP_TYPE_STATUS:
		return repr == ACTUAL ? BT_MESH_LIGHTNESS_OP_STATUS :
					BT_MESH_LIGHTNESS_OP_LINEAR_STATUS;
	default:
		return 0;
	}

	return 0;
}

static inline uint32_t lightness_sqrt32(uint32_t val)
{
	/* Shortcut out of this for the very common case of 0: */
	if (val == 0) {
		return 0;
	}

	/* Square root by binary search from the highest bit: */
	uint32_t factor = 0;

	/* sqrt(UINT32_MAX) < (1 << 16), so the highest bit will be bit 15: */
	for (int i = 15; i >= 0; --i) {
		factor |= BIT(i);
		if (factor * factor > val) {
			factor &= ~BIT(i);
		}
	}

	return factor;
}

static inline uint16_t linear_to_actual(uint16_t linear)
{
	/* Conversion: actual = 65535 * sqrt(linear / 65535) */
	return lightness_sqrt32(65535UL * linear);
}

static inline uint16_t actual_to_linear(uint16_t actual)
{
	/* Conversion:
	 * linear = CEIL(65535 * (actual * actual) / (65535 * 65535)))
	 */
	return DIV_ROUND_UP((uint32_t)actual * (uint32_t)actual, 65535UL);
}

/** @brief Convert light from the specified representation to the configured.
 *
 *  @param val  Value to convert.
 *  @param repr Representation the value is in.
 *
 *  @return The light value in the configured representation.
 */
static inline uint16_t repr_to_light(uint16_t val, enum light_repr repr)
{
	if (IS_ENABLED(CONFIG_BT_MESH_LIGHTNESS_LINEAR) && repr == ACTUAL) {
		return actual_to_linear(val);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LIGHTNESS_ACTUAL) && repr == LINEAR) {
		return linear_to_actual(val);
	}

	return val;
}

/** @brief Convert light from linear representation to the configured.
 *
 *  @param light Light value in linear representation.
 *
 *  @return The light value in the configured representation.
 */
static inline uint16_t from_linear(uint16_t val)
{
	if (IS_ENABLED(CONFIG_BT_MESH_LIGHTNESS_ACTUAL)) {
		return linear_to_actual(val);
	}

	return val;
}

/** @brief Convert light from actual representation to the configured.
 *
 *  @param light Light value in actual representation.
 *
 *  @return The light value in the configured representation.
 */
static inline uint16_t from_actual(uint16_t val)
{
	if (IS_ENABLED(CONFIG_BT_MESH_LIGHTNESS_LINEAR)) {
		return actual_to_linear(val);
	}

	return val;
}

/** @brief Convert light from the configured representation to the specified.
 *
 *  @param light Light value in the configured representation.
 *  @param repr  Representation to convert to
 *
 *  @return The light value in the representation specified in @c repr.
 */
static inline uint16_t light_to_repr(uint16_t light, enum light_repr repr)
{
	if (IS_ENABLED(CONFIG_BT_MESH_LIGHTNESS_LINEAR) && repr == ACTUAL) {
		return linear_to_actual(light);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LIGHTNESS_ACTUAL) && repr == LINEAR) {
		return actual_to_linear(light);
	}

	return light;
}

/** @brief Convert light from the configured representation to linear.
 *
 *  @param light Light value in the configured representation.
 *
 *  @return The light value in linear representation.
 */
static inline uint16_t to_linear(uint16_t val)
{
	if (IS_ENABLED(CONFIG_BT_MESH_LIGHTNESS_ACTUAL)) {
		return actual_to_linear(val);
	}

	return val;
}

/** @brief Convert light from the configured representation to actual.
 *
 *  @param light Light value in the configured representation.
 *
 *  @return The light value in actual representation.
 */
static inline uint16_t to_actual(uint16_t val)
{
	if (IS_ENABLED(CONFIG_BT_MESH_LIGHTNESS_LINEAR)) {
		return linear_to_actual(val);
	}

	return val;
}

struct bt_mesh_lightness_srv;
struct bt_mesh_lightness_cli;

void lightness_srv_disable_control(struct bt_mesh_lightness_srv *srv);

void lightness_srv_change_lvl(struct bt_mesh_lightness_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_lightness_set *set,
			      struct bt_mesh_lightness_status *status,
			      bool publish);


/** @brief Set the lightness without side effects such as storage or publication
 *
 *  @param[in] srv     Lightness server
 *  @param[in] ctx     Message context or NULL
 *  @param[in] set     Value to set
 *  @param[out] status Status response
 */
void bt_mesh_lightness_srv_set(struct bt_mesh_lightness_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_lightness_set *set,
			       struct bt_mesh_lightness_status *status);

/* For testing purposes */
int lightness_cli_light_get(struct bt_mesh_lightness_cli *cli,
			    struct bt_mesh_msg_ctx *ctx, enum light_repr repr,
			    struct bt_mesh_lightness_status *rsp);

int lightness_cli_light_set(struct bt_mesh_lightness_cli *cli,
			    struct bt_mesh_msg_ctx *ctx, enum light_repr repr,
			    const struct bt_mesh_lightness_set *set,
			    struct bt_mesh_lightness_status *rsp);

int lightness_cli_light_set_unack(struct bt_mesh_lightness_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  enum light_repr repr,
				  const struct bt_mesh_lightness_set *set);

void lightness_srv_default_set(struct bt_mesh_lightness_srv *srv,
			       struct bt_mesh_msg_ctx *ctx, uint16_t set);

int lightness_on_power_up(struct bt_mesh_lightness_srv *srv);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTNESS_INTERNAL_H__ */
