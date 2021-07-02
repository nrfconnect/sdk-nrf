/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cJSON.h"

/**
 * @brief Associate a child object with a parent.
 *
 * This API is expected to be called after data has been added to the child item. If this API
 * fails to add the child item to the parent it will free the child item. Due to this its not
 * guaranteed that subsequent accesses to the child item will succeed after this API call.
 *
 * @param[inout] parent	Pointer to a parent object.
 * @param[in]	 str	Name of the added child object.
 * @param[in]	 item	Pointer to child object.
 *
 */
void json_add_obj(cJSON *parent, const char *str, cJSON *item);

/**
 * @brief Add item to array object
 *
 * This API is expected to be called after data has been added to the child item. If this API
 * fails to add the child item to the parent it will free the child item. Due to this its not
 * guaranteed that subsequent accesses to the child item will succeed after this API call.
 *
 * @param[inout] parent	Pointer to a parent array object.
 * @param[in]    item	Pointer to item to be added.
 *
 */
void json_add_obj_array(cJSON *parent, cJSON *item);

int json_add_number(cJSON *parent, const char *str, double item);

int json_add_bool(cJSON *parent, const char *str, int item);

cJSON *json_object_decode(cJSON *obj, const char *str);

int json_add_str(cJSON *parent, const char *str, const char *item);

void json_print_obj(const char *prefix, const cJSON *obj);
