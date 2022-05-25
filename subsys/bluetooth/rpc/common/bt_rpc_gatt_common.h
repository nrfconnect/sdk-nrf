/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_RPC_GATT_COMMON_H_
#define BT_RPC_GATT_COMMON_H_

#include <zephyr/bluetooth/gatt.h>
#include <sys/types.h>

#include <nrf_rpc_cbor.h>

#define BT_RPC_GATT_ATTR_READ_PRESENT_FLAG 0x100
#define BT_RPC_GATT_ATTR_WRITE_PRESENT_FLAG 0x200

#define BT_RPC_GATT_CCC_CFG_CHANGE_PRESENT_FLAG 0x100
#define BT_RPC_GATT_CCC_CFG_WRITE_PRESENT_FLAG 0x200
#define BT_RPC_GATT_CCC_CFG_MATCH_PRESET_FLAG 0x400

#define BT_RPC_GATT_ATTR_USER_DEFINED 0
#define BT_RPC_GATT_ATTR_SPECIAL_SERVICE 1
#define BT_RPC_GATT_ATTR_SPECIAL_SECONDARY 2
#define BT_RPC_GATT_ATTR_SPECIAL_INCLUDED 3
#define BT_RPC_GATT_ATTR_SPECIAL_CHRC 4
#define BT_RPC_GATT_ATTR_SPECIAL_CCC 5
#define BT_RPC_GATT_ATTR_SPECIAL_CEP 6
#define BT_RPC_GATT_ATTR_SPECIAL_CUD 7
#define BT_RPC_GATT_ATTR_SPECIAL_CPF 8
#define BT_RPC_GATT_ATTR_SPECIAL_USER 9

/**@brief Helper structure for indication parameter serialization. */
struct bt_rpc_gatt_indication_params {
	/** Pointer to the structure holding indication parameters on the client side. */
	uintptr_t param_addr;

	/** Indication parameters. */
	struct bt_gatt_indicate_params params;
};

/**@brief Add GATT service to the service pool.
 *
 * This function adds GATT service to the pool and assigns an index to it.
 * This index is used for transferring the attribute between the client and host.
 *
 * @param[in] svc GATT service structure
 * @param[in, out] svc_index assigned GATT service index in the service pool.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_rpc_gatt_add_service(const struct bt_gatt_service *svc, uint32_t *svc_index);

/**@brief Remove GATT service from service pool.
 *
 * This function removes GATT service from the pool.
 * This means that service cannot be synchronized between the client and host.
 *
 * @param[in] svc GATT service structure.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_rpc_gatt_remove_service(const struct bt_gatt_service *svc);

/**@brief Get attribute index.
 *
 * @param[in] attr GATT Attribute structure.
 * @param[in,out] index Attribute index inside service pool.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_rpc_gatt_attr_to_index(const struct bt_gatt_attr *attr, uint32_t *index);

/**@brief Get attribute by index.
 *
 * @param[in] index Attribute index in the service pool.
 *
 * @return Pointer to the attribute with given index.
 */
const struct bt_gatt_attr *bt_rpc_gatt_index_to_attr(uint32_t index);

/**@brief Get service by index.
 *
 * @param[in] svc_index Service index in the service pool.
 *
 * @return Pointer to the GATT service with given index.
 */
const struct bt_gatt_service *bt_rpc_gatt_get_service_by_index(uint16_t svc_index);

/**@brief Get service index.
 *
 * @param[in] svc GATT service structure.
 * @param[in, out] svc_index Service index in the service pool.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_rpc_gatt_service_to_index(const struct bt_gatt_service *svc, uint16_t *svc_index);

/** @brief Attribute iterator by type.
 *
 * Iterate attributes in the given range matching given UUID and/or data.
 * This function is using RPC to get attributes handles from host.
 *
 *  @param start_handle Start handle.
 *  @param end_handle End handle.
 *  @param uuid UUID to match, passing NULL skips UUID matching.
 *  @param attr_data Attribute data to match, passing NULL skips data matching.
 *  @param num_matches Number matches, passing 0 makes it unlimited.
 *  @param func Callback function.
 *  @param user_data Data to pass to the callback.
 */
void bt_rpc_gatt_foreach_attr_type(uint16_t start_handle, uint16_t end_handle,
				   const struct bt_uuid *uuid,
				   const void *attr_data, uint16_t num_matches,
				   bt_gatt_attr_func_t func,
				   void *user_data);

/**@brief Encode GATT attribute.
 *
 * @param[in, out] ctx CBOR encoder context.
 * @param[in] attr GATT attribute.
 *
 */
void bt_rpc_encode_gatt_attr(struct nrf_rpc_cbor_ctx *ctx, const struct bt_gatt_attr *attr);

/**@brief Decode GATT attribute.
 *
 * @param[in, out] ctx CBOR decoder context.
 *
 * @return GATT attribute.
 */
const struct bt_gatt_attr *bt_rpc_decode_gatt_attr(struct nrf_rpc_cbor_ctx *ctx);

#endif /* BT_RPC_GATT_COMMON_H_ */
