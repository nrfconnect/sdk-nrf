/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


/**
 * @brief Memfault LTE coredump modem trace initialization. Shall only be called once.
 *	  Ths function sets up a CDR (Custom Data Recording) source for modem traces.
 *
 * @retval -EALREADY if the module has already been initialized.
 * @retval -EIO if the modem trace CDR source registration failed.
 * @retval 0 if the modem trace backend was successfully initialized.
 */
int memfault_lte_coredump_modem_trace_init(void);

/**
 * @brief Prepare modem trace data for upload
 *
 * @retval -ENOTSUP if the current modem trace backend is not supported.
 * @retval -ENODATA if there are no modem traces to send.
 * @retval 0 if the modem trace data is ready for upload.
 *
 * @returns Negative error if an error occurred preparing the modem trace.
 */
int memfault_lte_coredump_modem_trace_prepare_for_upload(void);
