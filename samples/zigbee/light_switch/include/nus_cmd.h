/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file nus_cmd.h
 *
 * @brief APIs for NUS Command Service.
 */

#ifndef __NUS_CMD_H__
#define __NUS_CMD_H__

/** @brief Type indicates function called when Bluetooth LE connection
 *         is established.
 *
 * @param[in] item pointer to work item.
 */
typedef void (*nus_connection_cb_t)(struct k_work *item);

/** @brief Type indicates function called when Bluetooth LE connection is ended.
 *
 * @param[in] item pointer to work item.
 */
typedef void (*nus_disconnection_cb_t)(struct k_work *item);

/** @brief NUS command entry.
 */
struct nus_entry {
	const char *cmd;        /**< Command string. */
	struct k_work work;     /**< Command work item. */
};

/**@brief Macro to initialise (bind) command string to work handler.
 */
#define NUS_COMMAND(_cmd, _handler)		      \
	{					      \
		.work = Z_WORK_INITIALIZER(_handler), \
		.cmd = _cmd,			      \
	}					      \

/**@brief Function to initialise NUS Command service.
 *
 * @param[in] on_connect Callback that will be called when central connects to
 *			 the NUS service.
 * @param[in] on_disconnect Callback that will be called when BLE central
 *			    disconnects from the NUS service.
 * @param[in] command_set   A pointer to an array with NUS commands.
 */
void nus_cmd_init(nus_connection_cb_t on_connect,
		  nus_disconnection_cb_t on_disconnect,
		  struct nus_entry *command_set);

#endif
