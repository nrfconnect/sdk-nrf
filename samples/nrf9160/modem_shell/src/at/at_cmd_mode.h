/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_AT_CMD_MODE_H
#define MOSH_AT_CMD_MODE_H

/** @brief Line Termination Modes. */
enum line_termination {
	NULL_TERM, /**< Null Termination */
	CR_TERM, /**< CR Termination */
	LF_TERM, /**< LF Termination */
	CR_LF_TERM /**< CR+LF Termination */
};

int at_cmd_mode_start(const struct shell *sh);

void at_cmd_mode_line_termination_set(enum line_termination line_term);

#endif /* MOSH_AT_CMD_MODE_H */
