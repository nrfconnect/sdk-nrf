/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __CALL_H
#define __CALL_H

void call_syscall(void);
int call_static_inline_fn(void);
int call_inline_static_fn(void);
int call_always_inline_fn(void);
int call_after_macro(void);
int call_extra_whitespace(void);
int call_multiline_def(int arg, int len);
void call_exclude_word_fn(void);

#endif /* __CALL_H */
