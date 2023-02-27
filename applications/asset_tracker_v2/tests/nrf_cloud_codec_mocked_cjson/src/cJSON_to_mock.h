/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cJSON.h>

/* This file is needed because cmock's generator cannot parse the original header. */

cJSON *cJSON_CreateArray(void);
cJSON *cJSON_CreateObject(void);
cJSON *cJSON_CreateString(const char *string);
void cJSON_Delete(cJSON *item);
cJSON *cJSON_DetachItemFromObject(cJSON *object, const char *string);
int cJSON_GetArraySize(const cJSON *array);
char *cJSON_PrintUnformatted(const cJSON *item);
void cJSON_Init(void);
void cJSON_FreeString(char *ptr);
cJSON *cJSON_AddStringToObjectCS(cJSON * const object, const char * const name,
	const char * const string);
cJSON *cJSON_AddNumberToObjectCS(cJSON * const object, const char * const name,
	const double number);
void cJSON_DeleteItemFromObject(cJSON *object, const char *string);
cJSON *cJSON_AddStringToObject(cJSON * const object, const char * const name,
	const char * const string);
cJSON *cJSON_AddObjectToObject(cJSON * const object, const char * const name);
