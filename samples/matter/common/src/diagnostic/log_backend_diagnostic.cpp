/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "diagnostic_logs_provider.h"

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>

static uint32_t sLogFormat = LOG_OUTPUT_TEXT;
/* The buffer size was selected to be able to store single log line (usually in Matter we have logs < 200 B).
 * In case the log will not fit it will be printed anyway, but usng fragmentation, so setting a small buffer
 * increases the time of printing.
 */
static uint8_t sLogBuffer[256];

static int RedirectNetworkLog(uint8_t *data, size_t length, void *context)
{
	Nrf::Matter::DiagnosticLogProvider::GetInstance().PushLog(
		chip::app::Clusters::DiagnosticLogs::IntentEnum::kNetworkDiag, data, length);

	return length;
}

static int RedirectEndUserLog(uint8_t *data, size_t length, void *context)
{
	Nrf::Matter::DiagnosticLogProvider::GetInstance().PushLog(
		chip::app::Clusters::DiagnosticLogs::IntentEnum::kEndUserSupport, data, length);

	return length;
}

LOG_OUTPUT_DEFINE(log_output_network, RedirectNetworkLog, sLogBuffer, sizeof(sLogBuffer));
LOG_OUTPUT_DEFINE(log_output_end_user, RedirectEndUserLog, sLogBuffer, sizeof(sLogBuffer));

void ProcessLog(const struct log_backend *const backend, union log_msg_generic *msg)
{
	void *source = (void *)log_msg_get_source(&msg->log);

	if (!source) {
		return;
	}

	const uint32_t flags = LOG_OUTPUT_FLAG_TIMESTAMP | LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP | LOG_OUTPUT_FLAG_LEVEL;
	/* Source id has to be obtained from different places, depending on runtime filtering. */
	uint32_t sourceId = IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?
				    log_dynamic_source_id(reinterpret_cast<log_source_dynamic_data *>(source)) :
				    log_const_source_id(reinterpret_cast<log_source_const_data *>(source));

	const char *sourceName = TYPE_SECTION_START(log_const)[sourceId].name;
	if (!sourceName) {
		return;
	}

	const log_format_func_t logFormatter = log_format_func_t_get(sLogFormat);

	/* The assumption following redirection is done:
	 * - chip -> diagnostic network logs
	 * - app -> diagnostic end user logs
	 * - other -> dropped
	 */
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS
	if (strncmp(sourceName, "chip", strlen("chip")) == 0) {
		logFormatter(&log_output_network, &msg->log, flags);
	}
#endif
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS
	if (strncmp(sourceName, "app", strlen("app")) == 0) {
		logFormatter(&log_output_end_user, &msg->log, flags);
	}
#endif
}

int SetFormat(const struct log_backend *const backend, uint32_t log_type)
{
	ARG_UNUSED(backend);

	sLogFormat = log_type;

	return 0;
}

const log_backend_api matter_diagnostic_backend_api = {
	.process = ProcessLog,
	.format_set = SetFormat,
};

LOG_BACKEND_DEFINE(matter_diagnostic_backend, matter_diagnostic_backend_api, /* autostart */ true);
