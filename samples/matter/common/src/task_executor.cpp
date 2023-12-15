/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "task_executor.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

constexpr size_t kTaskQueueSize = CONFIG_NCS_SAMPLE_MATTER_APP_TASK_QUEUE_SIZE;

K_MSGQ_DEFINE(sTaskQueue, sizeof(TaskExecutor::Task), kTaskQueueSize, alignof(TaskExecutor::Task));

void TaskExecutor::PostTask(const Task& task)
{
	if (k_msgq_put(&sTaskQueue, &task, K_NO_WAIT) != 0) {
		LOG_ERR("Failed to post event to app task event queue");
	}
}

void TaskExecutor::DispatchNextTask()
{
	Task task;
	k_msgq_get(&sTaskQueue, &task, K_FOREVER);
	task();
}
