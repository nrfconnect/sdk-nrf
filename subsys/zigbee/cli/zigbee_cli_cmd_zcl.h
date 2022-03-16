/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZIGBEE_CLI_CMD_ZCL_H__
#define ZIGBEE_CLI_CMD_ZCL_H__

int cmd_zb_ping(const struct shell *shell, size_t argc, char **argv);
int cmd_zb_readattr(const struct shell *shell, size_t argc, char **argv);
int cmd_zb_writeattr(const struct shell *shell, size_t argc, char **argv);
int cmd_zb_subscribe_on(const struct shell *shell, size_t argc, char **argv);
int cmd_zb_subscribe_off(const struct shell *shell, size_t argc, char **argv);
int cmd_zb_generic_cmd(const struct shell *shell, size_t argc, char **argv);
int cmd_zb_zcl_raw(const struct shell *shell, size_t argc, char **argv);
int cmd_zb_get_group_mem(const struct shell *shell, size_t argc, char **argv);
int cmd_zb_add_remove_group(const struct shell *shell, size_t argc, char **argv);
int cmd_zb_remove_all_groups(const struct shell *shell, size_t argc, char **argv);
int cmd_zb_add_group_if_identifying(const struct shell *shell, size_t argc, char **argv);

#endif /* ZIGBEE_CLI_CMD_ZCL_H__ */
