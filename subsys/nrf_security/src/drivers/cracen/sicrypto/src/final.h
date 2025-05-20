/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FINAL_HEADER_FILE
#define FINAL_HEADER_FILE

static inline int si_task_mark_final(struct sitask *t, int code)
{
	return (t->statuscode = code);
}

#endif
