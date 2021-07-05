/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

struct AppEvent {
	enum class Type { kButtonPush, kButtonRelease, kTimer };

	using Handler = void (*)(AppEvent *);

	Type mType;
	Handler mHandler;
};
