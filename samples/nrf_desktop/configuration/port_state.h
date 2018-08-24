/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef _PORT_STATE_H_
#define _PORT_STATE_H_

#include <stddef.h>

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif


struct pin_state {
	u32_t pin;
	u32_t val;
};

struct port_state {
	const char             *name;
	const struct pin_state *ps;
	size_t                  ps_count;
};


extern const struct port_state	port_state_on[];
extern const size_t		port_state_on_size;

extern const struct port_state	port_state_off[];
extern const size_t		port_state_off_size;


#ifdef __cplusplus
}
#endif

#endif /* _PORT_STATE_H_ */
