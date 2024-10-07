/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_ORCHESTRATOR_H__
#define SUIT_ORCHESTRATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the SUIT orchestrator before it can be used.
 *
 * @retval 0       if successful.
 * @retval -EFAULT if internal module initialization failed.
 * @retval -EROFS  if unable to access SUIT storage area.
 * @retval -EIO    if unable to modify internal state.
 */
int suit_orchestrator_init(void);

/**
 * @brief Main function of the orchestrator, starting boot or update.
 *
 *
 * @retval 0          if successful.
 * @retval -EPERM     if application MPI is missing (invalid digest).
 * @retval -EOVERFLOW if MPI has invalid format (i.e. version, values).
 * @retval -EBADF     if essential (Nordic, root) roles are unprovisioned.
 * @retval -ENOTSUP   if provided MPI configuration is not supported.
 * @retval -EINVAL    if called in invalid SUIT execution mode.
 * @retval -EFAULT    if internal error encountered.
 * @retval -EIO       if unable to modify SUIT storage area.
 * @retval -ENOENT    if attemmpted to boot non-existing manifest.
 * @retval -ENOEXEC   if unable to find or parse installed or candidate manifest.
 * @retval -EILSEQ    if manifest sequence execution failed.
 * @retval -EMSGSIZE  if update candidate size is invalid.
 * @retval -EACCES    if update candiadate manifest is invalid.
 * @retval -ESRCH     if update candiadate manifest class is unsupported.
 */
int suit_orchestrator_entry(void);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_ORCHESTRATOR_H__ */
