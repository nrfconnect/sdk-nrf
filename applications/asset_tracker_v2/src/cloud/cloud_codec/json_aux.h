/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cJSON.h"

int json_add_obj(cJSON *parent, const char *str, cJSON *item);

int json_add_obj_array(cJSON *parent, cJSON *item);

int json_add_number(cJSON *parent, const char *str, double item);

int json_add_bool(cJSON *parent, const char *str, int item);

cJSON *json_object_decode(cJSON *obj, const char *str);

int json_add_str(cJSON *parent, const char *str, const char *item);

void json_print_obj(const char *prefix, const cJSON *obj);
