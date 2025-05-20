/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief SUIT Manifest-accessible variables.
 *
 * @details Variables are readable by any SUIT manifest, the SDFW and the IPC
 * clients, modifiability of variables depends on access mask.
 */

#ifndef SUIT_MANIFEST_VARIABLES_H__
#define SUIT_MANIFEST_VARIABLES_H__

#include <stdint.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Selected IPC clients are allowed to modify a variable, i.e:
 * MFST_VAR_ACCESS_IPC | MFST_VAR_ACCESS_APP means that variable can be modified
 * by App Domain SUIT manifests and IPC clients executed on App Domain cores
 *
 */
#define MFST_VAR_ACCESS_IPC 0x01

/* Nordic Top, Secure Domain, System Controller manifests are allowed
 * to modify a variable, modifiability by System Controller IPC client
 * depends on MFST_VAR_ACCESS_IPC
 */
#define MFST_VAR_ACCESS_SEC 0x02

/* Root, App Domain manifests are allowed to modify a variable, modifiability by
 * App Domain IPC client depends on MFST_VAR_ACCESS_IPC
 */
#define MFST_VAR_ACCESS_APP 0x04

/* Radio Domain manifests are allowed to modify a variable, modifiability by
 * Radio Domain IPC client depends on MFST_VAR_ACCESS_IPC
 */
#define MFST_VAR_ACCESS_RAD 0x08

/**
 * @brief Modify a variable value
 *
 * @param[in]  id			Variable Identifier
 * @param[in]  val			Value to assigned to variable
 *
 * @retval SUIT_PLAT_SUCCESS		On success.
 * @retval SUIT_PLAT_ERR_NOT_FOUND	Variable with given id is not supported.
 * @retval SUIT_PLAT_ERR_SIZE		Applies to NVM-based variables. Given val
 *					exceeds 8 bits.
 * @retval SUIT_PLAT_ERR_IO		Storage backend was unable to modify NVM content.
 */
suit_plat_err_t suit_mfst_var_set(uint32_t id, uint32_t val);

/**
 * @brief Read out a variable value
 *
 * @param[in]   id			Variable Identifier
 * @param[out]  val			Value to assigned to variable
 *
 * @retval SUIT_PLAT_SUCCESS		On success.
 * @retval SUIT_PLAT_ERR_NOT_FOUND	Variable with given id is not supported.
 * @retval SUIT_PLAT_ERR_INVAL          Invalid parameter, i.e. null pointer
 * @retval SUIT_PLAT_ERR_IO		Storage backend was unable to read out NVM content.
 */
suit_plat_err_t suit_mfst_var_get(uint32_t id, uint32_t *val);

/**
 * @brief Read out an access mask
 *
 * @note Module does not enforce variable access privileges, but provides
 *	 information about access mask, so privileges can be enforced
 *       on client of this module.
 *
 * @param[in]   id			Variable Identifier
 * @param[out]  access_mask		Access mask
 *
 * @retval SUIT_PLAT_SUCCESS		On success.
 * @retval SUIT_PLAT_ERR_NOT_FOUND	Variable with given id is not supported.
 * @retval SUIT_PLAT_ERR_INVAL          Invalid parameter, i.e. null pointer
 */
suit_plat_err_t suit_mfst_var_get_access_mask(uint32_t id, uint32_t *access_mask);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_MANIFEST_VARIABLES_H__ */
