/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <net/cloud.h>

extern struct cloud_backend __cloud_backends_start[0];
extern struct cloud_backend __cloud_backends_end[0];

struct cloud_backend *cloud_get_binding(const char *name)
{
	struct cloud_backend *info;

	for (info = __cloud_backends_start; info != __cloud_backends_end;
	     info++) {
		if (info->api == NULL) {
			continue;
		}

		if (strcmp(name, info->config->name) == 0) {
			return info;
		}
	}

	return NULL;
}

void cloud_backend_list_get(
	struct cloud_backend **backend_list,
	int *backend_count)
{

	*backend_list = __cloud_backends_start;
	*backend_count = __cloud_backends_end - __cloud_backends_start;
}
