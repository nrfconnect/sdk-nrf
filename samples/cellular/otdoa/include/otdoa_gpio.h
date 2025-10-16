/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#ifndef OTDOA_GPIO_H__
#define OTDOA_GPIO_H__
/* blink speeds */
typedef enum {
	NONE,
	SLOW,
	MED,
	FAST,
	FLASH,
} BLINK_SPEED;

int init_led(void);
int init_button(void);
int set_led(int state);
int toggle_led(void);
void set_blink_mode(BLINK_SPEED speed);
void set_blink_sleep(void);
void set_blink_prs(void);
void set_blink_download(void);
void set_blink_error(void);

#endif /* OTDOA_GPIO_H__ */
