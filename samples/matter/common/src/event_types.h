/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

struct AppEvent; /* Needs to be implemented in the application code. */
using EventHandler = void (*)(const AppEvent &);
