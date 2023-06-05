/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Internal HCI interface
 */

#include <stdint.h>
#include <stdbool.h>
#include <sdc_hci_cmd_le.h>
#include <sdc_hci_cmd_info_params.h>

#ifndef HCI_INTERNAL_H__
#define HCI_INTERNAL_H__


/** @brief Send an HCI command packet to the SoftDevice Controller.
 *
 * If the application has provided a user handler, this handler get precedence
 * above the default HCI command handlers. See @ref hci_internal_user_cmd_handler_register.
 *
 * @param[in] cmd_in  HCI Command packet. The first byte in the buffer should correspond to
 *                    OpCode, as specified by the Bluetooth Core Specification.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int hci_internal_cmd_put(uint8_t *cmd_in);

/** A user implementable HCI command handler
 *
 * What is done in the command handler is up to the user.
 * Parameters can be returned to the host through the raw_event_out output parameter.
 *
 * When the command handler returns, a Command Complete or Command Status event is generated.
 *
 * @param[in]  cmd               The HCI command itself. The first byte in the buffer corresponds
 *                               to OpCode, as specified by the Bluetooth Core Specification.
 * @param[out] raw_event_out     Parameters to be returned from the event as return parameters in
 *                               the Command Complete event.
 *                               Parameters can only be returned if the generated event is
 *                               a Command Complete event.
 * @param[out] param_length_out  Length of parameters to be returned.
 * @param[out] gives_cmd_status  Set to true if the command is returning a Command Status event.
 *
 * @return Bluetooth status code. BT_HCI_ERR_UNKNOWN_CMD if unknown.
 */
typedef uint8_t (*hci_internal_user_cmd_handler_t)(uint8_t const *cmd,
						   uint8_t *raw_event_out,
						   uint8_t *param_length_out,
						   bool *gives_cmd_status);

/** @brief Register a user handler for HCI commands.
 *
 * The user handler can be used to handle custom HCI commands.
 *
 * The user handler will have precedence over all other command handling.
 * Therefore, the application needs to ensure it is not using opcodes that
 * are used for other Bluetooth or vendor specific HCI commands.
 * See sdc_hci_vs.h for the opcodes that are reserved.
 *
 * @note Only one handler can be registered.
 *
 * @param[in] handler
 * @return Zero on success or (negative) error code otherwise
 */
int hci_internal_user_cmd_handler_register(const hci_internal_user_cmd_handler_t handler);

/** @brief Retrieve an HCI packet from the SoftDevice Controller.
 *
 * This API is non-blocking.
 *
 * @note The application should ensure that the size of the provided buffer is at least
 *       @ref HCI_EVENT_PACKET_MAX_SIZE bytes.
 *
 * @param[in,out] msg_out Buffer where the HCI packet will be stored.
 *                        If an event is retrieved, the first byte corresponds to Event Code,
 *                        as specified by the Bluetooth Core Specification.
 * @param[out] msg_type_out Indicates the type of hci message received.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int hci_internal_msg_get(uint8_t *msg_out, sdc_hci_msg_type_t *msg_type_out);

/** @brief Retrieve the list of supported commands configured for this build
 *
 * @param[out] cmds The list of supported commands
 */
void hci_internal_supported_commands(
	sdc_hci_ip_supported_commands_t *cmds);

/** @brief Retrieve the list of supported LE features configured for this build
 *
 * @param[out] features The list of supported features
 */
void hci_internal_le_supported_features(
	sdc_hci_cmd_le_read_local_supported_features_return_t *features);

#endif
