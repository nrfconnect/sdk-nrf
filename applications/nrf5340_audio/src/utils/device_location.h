/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DEVICE_LOCATION_H_
#define _DEVICE_LOCATION_H_

/** @file
 *  @brief Audio location assignment
 *
 * Audio location can be assigned at runtime or compile-time, depending on configuration.
 *
 */

#include <zephyr/bluetooth/audio/audio.h>
#include <audio_defines.h>

#ifndef DEVICE_LOCATION_DEFAULT
#define DEVICE_LOCATION_DEFAULT BT_AUDIO_LOCATION_FRONT_LEFT
#endif /* DEVICE_LOCATION_DEFAULT */

static const char HS_LOC_L_TAG[] = "HL";
static const char HS_LOC_R_TAG[] = "HR";
static const char HS_LOC_LR_TAG[] = "HS";
static const char HS_LOC_OTHER_TAG[] = "H*";
static const char GW_TAG[] = "GW";

/**
 * @brief Get assigned audio location.
 *
 * @param[out] location Location bitfield
 */
void device_location_get(enum bt_audio_location *location);

#if CONFIG_DEVICE_LOCATION_SET_RUNTIME
/**
 * @brief Assign audio location.
 *
 * @param[out] location Location bitfield
 */
void device_location_set(enum bt_audio_location location);
#endif /* LOCATION_SET_RUNTIME */

#endif /* _DEVICE_LOCATION_H_ */
