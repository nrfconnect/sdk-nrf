/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef SHELL_BT_NUS_H_
#define SHELL_BT_NUS_H_

#include <shell/shell.h>
#include <bluetooth/conn.h>
#include <bluetooth/services/nus.h>
#include <sys/ring_buffer.h>

/**
 * @file
 * @defgroup shell_bt_nus Nordic UART Service (NUS) shell transport
 * @{
 * @brief Nordic UART Service (NUS) shell transport API.
 */

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_bt_nus_transport_api;

/** @brief Instance control block (RW data). */
struct shell_bt_nus_ctrl_blk {
	struct bt_conn *conn;
	atomic_t tx_busy;
	shell_transport_handler_t handler;
	void *context;
};

/** @brief Instance structure. */
struct shell_bt_nus {
	struct shell_bt_nus_ctrl_blk *ctrl_blk;
	struct ring_buf *tx_ringbuf;
	struct ring_buf *rx_ringbuf;
};

/** @brief Macro for creating an instance of the module. */
#define SHELL_BT_NUS_DEFINE(_name, _tx_ringbuf_size, _rx_ringbuf_size)	\
	static struct shell_bt_nus_ctrl_blk _name##_ctrl_blk;		\
	RING_BUF_DECLARE(_name##_tx_ringbuf, _tx_ringbuf_size);		\
	RING_BUF_DECLARE(_name##_rx_ringbuf, _rx_ringbuf_size);		\
	static const struct shell_bt_nus _name##_shell_bt_nus = {	\
		.ctrl_blk = &_name##_ctrl_blk,				\
		.tx_ringbuf = &_name##_tx_ringbuf,			\
		.rx_ringbuf = &_name##_rx_ringbuf,			\
	};								\
	struct shell_transport _name = {				\
		.api = &shell_bt_nus_transport_api,			\
		.ctx = (struct shell_bt_nus *)&_name##_shell_bt_nus	\
	}

/**
 * @brief Initialize the NUS shell transport instance.
 *
 * This function initializes a shell thread and the Nordic UART Service.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int shell_bt_nus_init(void);

/**
 * @brief Enable the NUS shell transport instance.
 *
 * This function should be called when the connection is established.
 *
 * @param conn Connection handle.
 */
void shell_bt_nus_enable(struct bt_conn *conn);

/**
 * @brief Disable the NUS shell transport instance.
 */
void shell_bt_nus_disable(void);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* SHELL_BT_NUS_H_ */
