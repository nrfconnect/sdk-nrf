/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TEST_COMMON_H__
#define TEST_COMMON_H__

#include <suit_metadata.h>

/** Nordic vendor ID UUID */
extern const suit_manifest_class_id_t nordic_vid;

/** Nordic top class ID UUID */
extern const suit_manifest_class_id_t nordic_top_cid;

/** SDFW class ID UUID */
extern const suit_manifest_class_id_t nordic_sdfw_cid;

/** SCFW class ID UUID */
extern const suit_manifest_class_id_t nordic_scfw_cid;

/** Application root class ID UUID */
extern const suit_manifest_class_id_t app_root_cid;

/** Application local class ID UUID */
extern const suit_manifest_class_id_t app_local_cid;

/** @brief Erase MPI area, assigned for Nordic-controlled manifests.
 */
void erase_area_nordic(void);

/** @brief Erase MPI area, assigned for radio core manifests.
 */
void erase_area_rad(void);

/** @brief Erase MPI area, assigned for application manifests.
 */
void erase_area_app(void);

/** @brief Populate part of application MPI area, with sample local and root MPI.
 */
void write_area_app(void);

/** @brief Assert that all Nordic-controlled MPIs are provisioned.
 */
void assert_nordic_classes(void);

/** @brief Assert that application MPIs are provisioned.
 */
void assert_sample_app_classes(void);

#endif /* TEST_COMMON_H__ */
