/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef THROUGHPUT_MAIN_H_
#define THROUGHPUT_MAIN_H_

/**
 * @brief Run the test
 *
 * @param shell       Shell instance where output will be printed.
 * @param conn_param  Connection parameters.
 * @param phy         Phy parameters.
 * @param data_len    Maximum transmission payload.
 */
int test_run(const struct shell *shell,
	     const struct bt_le_conn_param *conn_param,
	     const struct bt_conn_le_phy_param *phy,
	     const struct bt_conn_le_data_len_param *data_len);

/**
 * @brief Set the board into a specific role.
 *
 * @param is_central true for central role, false for peripheral role.
 */
void select_role(bool is_central);

#endif /* THROUGHPUT_MAIN_H_ */
