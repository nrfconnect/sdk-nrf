/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_prop_srv Generic Property server models
 * @{
 * @brief API for the Generic Property server models.
 */
#ifndef BT_MESH_GEN_PROP_SRV_H__
#define BT_MESH_GEN_PROP_SRV_H__

#include <bluetooth/mesh/gen_prop.h>
#include <bluetooth/mesh/model_types.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_prop_srv;

/** @def BT_MESH_PROP_SRV_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_prop_srv instance.
 *
 * @param[in] _properties Array of properties supported by the server.
 * @param[in] _property_count Number of properties supported by the server.
 *                            Cannot be larger than
 *                            @kconfig{CONFIG_BT_MESH_PROP_MAXCOUNT}.
 * @param[in] _get Getter handler for property values. @sa
 * bt_mesh_prop_srv::get.
 * @param[in] _set Setter handler for property values. @sa
 * bt_mesh_prop_srv::set.
 */
#define BT_MESH_PROP_SRV_INIT(_properties, _property_count, _get, _set)        \
	{                                                                      \
		.get = _get, .set = _set, .properties = _properties,           \
		.property_count = _property_count,                             \
	}

/** @def BT_MESH_PROP_SRV_USER_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_prop_srv acting as a
 * Generic User Property Server.
 */
#define BT_MESH_PROP_SRV_USER_INIT() {}

/** @def BT_MESH_PROP_SRV_ADMIN_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_prop_srv acting as a
 * Generic Admin Property Server.
 *
 * @param[in] _properties Array of properties exposed by the server.
 * @param[in] _get Getter function for property values.
 * @param[in] _set Setter function for property values.
 */
#define BT_MESH_PROP_SRV_ADMIN_INIT(_properties, _get, _set)                   \
	BT_MESH_PROP_SRV_INIT(_properties, ARRAY_SIZE(_properties), _get, _set)

/** @def BT_MESH_PROP_SRV_MFR_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_prop_srv acting as a
 * Generic Manufacturer Property Server.
 *
 * @param[in] _properties Array of properties exposed by the server.
 * @param[in] _get Getter function for property values.
 */
#define BT_MESH_PROP_SRV_MFR_INIT(_properties, _get)                           \
	BT_MESH_PROP_SRV_INIT(_properties, ARRAY_SIZE(_properties), _get, NULL)

/** @def BT_MESH_PROP_SRV_CLIENT_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_prop_srv acting as a
 * Generic Client Property Server.
 *
 * @param[in] _properties Array of properties exposed by the server. May be
 * constant.
 */
#define BT_MESH_PROP_SRV_CLIENT_INIT(_properties)                              \
	BT_MESH_PROP_SRV_INIT((struct bt_mesh_prop *)_properties,              \
			      ARRAY_SIZE(_properties), NULL, NULL)

/** @def BT_MESH_MODEL_PROP_SRV_USER
 *
 * @brief Generic User Property Server model composition data entry.
 */
#define BT_MESH_MODEL_PROP_SRV_USER(_srv)                                      \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_USER_PROP_SRV,                   \
			 _bt_mesh_prop_user_srv_op, &(_srv)->pub,              \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_prop_srv,      \
						 _srv),                        \
			 &_bt_mesh_prop_srv_cb)

/** @def BT_MESH_MODEL_PROP_SRV_ADMIN
 *
 * @brief Generic Admin Property Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_prop_srv instance acting as an
 * Admin Property Server.
 */
#define BT_MESH_MODEL_PROP_SRV_ADMIN(_srv)                                     \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_ADMIN_PROP_SRV,                  \
			 _bt_mesh_prop_admin_srv_op, &(_srv)->pub,             \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_prop_srv,      \
						 _srv),                        \
			 &_bt_mesh_prop_srv_cb)

/** @def BT_MESH_MODEL_PROP_SRV_MFR
 *
 * @brief Generic Manufacturer Property Server model data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_prop_srv instance acting as a
 * Manufacturer Property Server.
 */
#define BT_MESH_MODEL_PROP_SRV_MFR(_srv)                                       \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_MANUFACTURER_PROP_SRV,           \
			 _bt_mesh_prop_mfr_srv_op, &(_srv)->pub,               \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_prop_srv,      \
						 _srv),                        \
			 &_bt_mesh_prop_srv_cb)

/** @def BT_MESH_MODEL_PROP_SRV_CLIENT
 *
 * @brief Generic Client Property Server model data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_prop_srv instance acting as a
 * Client Property Server.
 */
#define BT_MESH_MODEL_PROP_SRV_CLIENT(_srv)                                    \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_CLIENT_PROP_SRV,                 \
			 _bt_mesh_prop_client_srv_op, &(_srv)->pub,            \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_prop_srv,      \
						 _srv),                        \
			 &_bt_mesh_prop_srv_cb)

/** Enumeration of all property server states. */
enum bt_mesh_prop_srv_state {
	BT_MESH_PROP_SRV_STATE_NONE, /**< No state. */
	BT_MESH_PROP_SRV_STATE_LIST, /**< Property List. */
	BT_MESH_PROP_SRV_STATE_PROP, /**< Property value. */
};

/**
 * Generic Property Server instance. Should be initialized with the
 * @ref BT_MESH_PROP_SRV_INIT, @ref BT_MESH_PROP_SRV_ADMIN_INIT,
 * @ref BT_MESH_PROP_SRV_MFR_INIT or @ref BT_MESH_PROP_SRV_CLIENT_INIT macro.
 */
struct bt_mesh_prop_srv {
	/** Pointer to the mesh model entry. */
	struct bt_mesh_model *model;
	/** Model publication parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_PROP_MSG_MAXLEN(CONFIG_BT_MESH_PROP_MAXCOUNT)];
	/** Property ID currently being published. */
	uint16_t pub_id;
	/** Which state is currently being published. */
	enum bt_mesh_prop_srv_state pub_state;

	/** List of properties supported by the server. */
	struct bt_mesh_prop *const properties;
	/** Number of properties supported by the server. */
	const uint32_t property_count;

	/** @brief Set a property value.
	 *
	 * The handler may reject the value change with two levels of severity:
	 *
	 * To indicate data format errors and permanently illegal values, change
	 * the @c val::meta::id field to @ref BT_MESH_PROP_ID_PROHIBITED before
	 * returning. This will cancel the response message sending, indicating
	 * a protocol violation.
	 *
	 * To indicate that the new value is temporarily unable to change, but
	 * not in violation of the protocol, replace the contents of @c buf and
	 * @c size with the current property value and size. Note that @c buf
	 * can only fit a value of size @c CONFIG_GEN_PROP_MAXSIZE.
	 *
	 * @note This handler is mandatory if the server is acting as an Admin
	 * Property Server, and ignored if the server is acting as a
	 * Manufacturer Property Server.
	 *
	 * @param[in] srv Property Server instance whose property to set.
	 * @param[in] ctx Message context for the message that triggered the
	 * change, or NULL if the change is not coming from a message.
	 * @param[in,out] val Property value to set. Any changes to the value
	 * will be reflected in the response message.
	 */
	void (*const set)(struct bt_mesh_prop_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_prop_val *val);

	/** @brief Get a property value.
	 *
	 * Note that @p buf can only fit a value of size
	 * @c CONFIG_GEN_PROP_MAXSIZE.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Property Server instance whose property to set.
	 * @param[in] ctx Message context for the message that triggered the
	 * change, or NULL if the change is not coming from a message.
	 * @param[in,out] val Property value to get. Any changes to the value
	 * will be reflected in the response message.
	 */
	void (*const get)(struct bt_mesh_prop_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_prop_val *val);
};

/** @brief Publish a list of all properties on the server.
 *
 * @param[in] srv Server that owns the property.
 * @param[in] ctx Message context to publish with, or NULL to publish on the
 * configured publish parameters.
 *
 * @retval 0 Successfully publish a Generic Level Status message.
 * @retval -EMSGSIZE The given property size is not supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_prop_srv_pub_list(struct bt_mesh_prop_srv *srv,
			      struct bt_mesh_msg_ctx *ctx);

/** @brief Publish a property value.
 *
 * @param[in] srv Server that owns the property.
 * @param[in] ctx Message context to publish with, or NULL to publish on the
 * configured publish parameters.
 * @param[in] val Value of the property.
 *
 * @retval 0 Successfully publish a Generic Level Status message.
 * @retval -EINVAL The server is a Client Property server, which does not
 * support publishing of property values.
 * @retval -EMSGSIZE The given property size is not supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_prop_srv_pub(struct bt_mesh_prop_srv *srv,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_prop_val *val);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_cb _bt_mesh_prop_srv_cb;
extern const struct bt_mesh_model_op _bt_mesh_prop_user_srv_op[];
extern const struct bt_mesh_model_op _bt_mesh_prop_admin_srv_op[];
extern const struct bt_mesh_model_op _bt_mesh_prop_mfr_srv_op[];
extern const struct bt_mesh_model_op _bt_mesh_prop_client_srv_op[];
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_PROP_SRV_H__ */

/** @} */
