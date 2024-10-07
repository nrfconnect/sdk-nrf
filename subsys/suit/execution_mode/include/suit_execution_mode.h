/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_EXECUTION_MODE_H__
#define SUIT_EXECUTION_MODE_H__

#include <stdbool.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	EXECUTION_MODE_STARTUP = 0,
	EXECUTION_MODE_INVOKE,
	EXECUTION_MODE_INVOKE_RECOVERY,
	EXECUTION_MODE_INSTALL,
	EXECUTION_MODE_INSTALL_RECOVERY,
	EXECUTION_MODE_POST_INVOKE,
	EXECUTION_MODE_POST_INVOKE_RECOVERY,
	EXECUTION_MODE_FAIL_NO_MPI,
	EXECUTION_MODE_FAIL_MPI_INVALID,
	EXECUTION_MODE_FAIL_MPI_INVALID_MISSING,
	EXECUTION_MODE_FAIL_MPI_UNSUPPORTED,
	EXECUTION_MODE_FAIL_INVOKE_RECOVERY,
	EXECUTION_MODE_FAIL_INSTALL_NORDIC_TOP,
	EXECUTION_MODE_FAIL_STARTUP,
} suit_execution_mode_t;

/**
 * @brief Return current execution mode set in module
 *
 * @return suit_execution_mode_t Current value
 */
suit_execution_mode_t suit_execution_mode_get(void);

/**
 * @brief Set new execution mode
 *
 * @param mode Valid execution mode
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid mode
 */
suit_plat_err_t suit_execution_mode_set(suit_execution_mode_t mode);

/**
 * @brief Update execution mode as a result of interrupted startup procedure.
 *
 * @details The main purpose of this API is to get out of the non-final state
 *          (i.e. update candidate is detected and the orchestrator entry will
 *          start processing it) if the startup procedure is interrupted
 *          between suit_orchestrator_init(), which schedules processing of
 *          manifests and suit_orchestrator_entry(), which executes scheduled
 *          operations. This situation may occur as a result of BICR or UICR
 *          misconfiguration.
 *          Since execution mode value is used to check if the it is allowed to
 *          modify memory regions, the mode should be changed to one of the
 *          FAILED states in such case to unblock device recovery procedures.
 *
 * @note This API will set the execution mode to EXECUTION_MODE_FAIL_STARTUP
 *          only if the current mode indicates a transient state.
 */
void suit_execution_mode_startup_failed(void);

/**
 * @brief Check if SUIT is processing installed manifests.
 *
 * @details Certain operations are not available if the SUIT orchestrator is
 *          actively using the SUIT processor (and SUIT decoder) states.
 *          Moreover, modifying manifests while they are processed might result
 *          in security bridges.
 *          To allow external modules (i.e. memory lease mechanisms) to check
 *          if the SUIT orchestrator is booting manifests without exposing all
 *          of the details about execution mode, this API may be used.
 */
bool suit_execution_mode_booting(void);

/**
 * @brief Check if SUIT is processing an update candidate.
 *
 * @details Certain operations are not available if the SUIT orchestrator is
 *          actively using the SUIT processor (and SUIT decoder) states.
 *          Moreover, modifying DFU partition while update canidates they are
 *          processed might result in security bridges.
 *          To allow external modules (i.e. memory lease mechanisms) to check
 *          if the SUIT orchestrator is updating manifests without exposing all
 *          of the details about execution mode, this API may be used.
 */
bool suit_execution_mode_updating(void);

/**
 * @brief Check if SUIT failed to process OEM manifests.
 *
 * @details Certain operations must be enabled or disabled if SUIT orchestrator
 *          logic fails to initialize.
 *          The most common reason for such failure is the misconfiguration of
 *          the MPI area.
 *          Although this failure modifies the execution mode, it is not
 *          reflected by the suit_orchestrator_init() return code, so the rest
 *          of the system (UICR allocation, ADAC server) can take place.
 *          Due to that, some modules must rely on the execution mode to set
 *          the permissions for their operation. To allow them to check
 *          if the SUIT orchestrator is in one of the FAILED states without
 *          exposing all of the details about execution mode, this API may be
 *          used.
 */
bool suit_execution_mode_failed(void);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_EXECUTION_H__ */
