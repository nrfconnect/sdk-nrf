/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_plat_mem_util.h>

#ifndef TEST_COMMON_H__
#define TEST_COMMON_H__

#define SUIT_STORAGE_NORDIC_ADDRESS suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_NORDIC_OFFSET)
#define SUIT_STORAGE_NORDIC_OFFSET  FIXED_PARTITION_OFFSET(cpusec_suit_storage)
#define SUIT_STORAGE_NORDIC_SIZE    FIXED_PARTITION_SIZE(cpusec_suit_storage)
#define SUIT_STORAGE_RAD_ADDRESS    suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_RAD_OFFSET)
#define SUIT_STORAGE_RAD_OFFSET	    FIXED_PARTITION_OFFSET(cpurad_suit_storage)
#define SUIT_STORAGE_RAD_SIZE	    FIXED_PARTITION_SIZE(cpurad_suit_storage)
#define SUIT_STORAGE_APP_ADDRESS    suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_APP_OFFSET)
#define SUIT_STORAGE_APP_OFFSET	    FIXED_PARTITION_OFFSET(cpuapp_suit_storage)
#define SUIT_STORAGE_APP_SIZE	    FIXED_PARTITION_SIZE(cpuapp_suit_storage)

#define SUIT_STORAGE_APP_NVV_ADDRESS suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_APP_NVV_OFFSET)
#define SUIT_STORAGE_APP_NVV_OFFSET  (SUIT_STORAGE_APP_OFFSET + 0x380)
#define SUIT_STORAGE_APP_NVV_SIZE    0x80
#define SUIT_STORAGE_APP_MPI_SIZE    0xf0
#define SUIT_STORAGE_RAD_MPI_SIZE    0x90
#define SUIT_STORAGE_DIGEST_SIZE     0x20

extern uint8_t nvv_empty[64];
extern uint8_t nvv_sample[64];

/** @brief Erase MPI area, assigned for Nordic-controlled manifests.
 */
void erase_area_nordic(void);

/** @brief Erase MPI area, assigned for radio core manifests.
 */
void erase_area_rad(void);

/** @brief Erase MPI area, assigned for application manifests.
 */
void erase_area_app(void);

/** @brief Erase part of MPI area, holding NVV values.
 */
void erase_area_app_nvv(void);

/** @brief Erase part of MPI area, holding NVV backup.
 */
void erase_area_app_nvv_backup(void);

/** @brief Populate part of MPI area, holding NVV backup with initial values.
 */
void write_area_app_empty_nvv_backup(void);

/** @brief Populate part of MPI area, holding NVV backup with sample values.
 */
void write_area_app_nvv_backup(void);

/** @brief Populate part of MPI area, holding NVV values with sample values.
 */
void write_area_app_nvv(void);

/** @brief Populate application MPI area, with ones and the correct digest.
 */
void write_empty_area_app(void);

/** @brief Populate radio MPI area, with ones and the correct digest.
 */
void write_empty_area_rad(void);

/** @brief Populate part of application MPI area, with sample root MPI.
 */
void write_area_app_root(void);

/** @brief Populate part of radio MPI area, with sample MPI entry.
 */
void write_area_rad(void);

/** @brief Populate part of Nordic MPI area, with sample root MPI backup.
 */
void write_area_nordic_root(void);

/** @brief Populate part of Nordic MPI area, with sample radio MPI entry backup.
 */
void write_area_nordic_rad(void);

/** @brief Populate part of Nordic MPI area, with sample outdated root MPI backup.
 */
void write_area_nordic_old_root(void);

/** @brief Populate part of Nordic MPI area, with sample outdated radio MPI backup.
 */
void write_area_nordic_old_rad(void);

/** @brief Assert that all Nordic-controlled MPIs are provisioned.
 */
void assert_nordic_classes(void);

/** @brief Assert that root MPI is provisioned.
 */
void assert_sample_root_class(void);

/** @brief Assert that root and sample radio MPI are provisioned.
 */
void assert_sample_root_rad_class(void);

/** @brief Assert that the area contains empty application MPI.
 *
 * @param[in] addr  Address of the area.
 * @param[in] size  Size of the area.
 */
void assert_empty_mpi_area_app(uint8_t *addr, size_t size);

/** @brief Assert that the area contains valid root MPI.
 *
 * @param[in] addr  Address of the area.
 * @param[in] size  Size of the area.
 */
void assert_valid_mpi_area_app(uint8_t *addr, size_t size);

/** @brief Assert that the area contains empty radio MPI.
 *
 * @param[in] addr  Address of the area.
 * @param[in] size  Size of the area.
 */
void assert_empty_mpi_area_rad(uint8_t *addr, size_t size);

/** @brief Assert that the area contains valid sample radio MPI entry.
 *
 * @param[in] addr  Address of the area.
 * @param[in] size  Size of the area.
 */
void assert_valid_mpi_area_rad(uint8_t *addr, size_t size);

#endif /* TEST_COMMON_H__ */
