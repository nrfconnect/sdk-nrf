/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <logging/log_ctrl.h>
#include <logging/log.h>
#include <secure_services.h>
#include <init.h>

LOG_MODULE_REGISTER(spm_ns_logs_dump);

static void log_dump(void)
{
	LOG_PANIC();
}

static int spm_ns_logs_dump_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int err = spm_set_ns_fatal_error_handler(log_dump);

	if (err < 0) {
		LOG_ERR("Non-secure fault handler not enabled in SPM");
	}

	return err;
}

SYS_INIT(spm_ns_logs_dump_init, PRE_KERNEL_1, 0);
