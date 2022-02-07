/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sized_events.h"


EVENT_TYPE_DEFINE(test_size1_event, false, NULL, NULL);
EVENT_TYPE_DEFINE(test_size2_event, false, NULL, NULL);
EVENT_TYPE_DEFINE(test_size3_event, false, NULL, NULL);
EVENT_TYPE_DEFINE(test_size_big_event, false, NULL, NULL);

EVENT_TYPE_DEFINE(test_dynamic_event, false, NULL, NULL);
EVENT_TYPE_DEFINE(test_dynamic_with_data_event, false, NULL, NULL);
