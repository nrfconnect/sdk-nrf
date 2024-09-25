/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_TRAP_MACROS_
#define SLM_TRAP_MACROS_

/** These functions are disallowed for the modem AT command
 *  interception (using at_cmd_custom lib) to work properly.
 *  For that, all the AT commands must go through @c nrf_modem_at_cmd() except
 *  when forwarding intercepted AT commands from within the callbacks.
 *  Alternatives to these functions are available in slm_util.h.
 */
#define nrf_modem_at_printf(...) function_disallowed_use_slm_util_printf_alternative(__VA_ARGS__)
#define nrf_modem_at_scanf(...) function_disallowed_use_slm_util_scanf_alternative(__VA_ARGS__)
#define nrf_modem_at_cmd_async(...) function_disallowed(__VA_ARGS__)

#endif
