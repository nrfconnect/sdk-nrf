/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DATE_TIME_CORE_H_
#define DATE_TIME_CORE_H_

#define DATE_TIME_TZ_INVALID 99

void date_time_core_init(void);
int date_time_core_now(int64_t *unix_time_ms);
int date_time_core_now_local(int64_t *local_time_ms);
int date_time_core_update_async(date_time_evt_handler_t evt_handler);
void date_time_core_register_handler(date_time_evt_handler_t evt_handler);
bool date_time_core_is_valid(void);
bool date_time_core_is_valid_local(void);
void date_time_core_clear(void);

void date_time_core_store(int64_t curr_time_ms, enum date_time_evt_type time_source);
void date_time_core_store_tz(int64_t curr_time_ms, enum date_time_evt_type time_source, int tz);
int date_time_core_current_check(void);

#endif /* DATE_TIME_CORE_H_ */
