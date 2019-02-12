/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef LV_TUTORIALS_H__
#define LV_TUTORIALS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a simple 'Hello world!' label
 */
void lv_tutorial_hello_world(void);

/**
 * Create some objects
 */
void lv_tutorial_objects(void);

/**
 * Create objects and styles
 */
void lv_tutorial_styles(void);

/**
 * Initialize a theme and create the same objects like "lv_tutorial_objects'
 */
void lv_tutorial_themes(void);

/**
 * Crate some objects an animate them
 */
void lv_tutorial_responsive(void);

#ifdef __cplusplus
}
#endif

#endif /* LV_TUTORIALS_H__ */
