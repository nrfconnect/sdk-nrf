/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

struct AppEvent {
	enum class Type {
		kButtonPush,
		kButtonRelease,
		kTimer,
#ifdef CONFIG_MCUMGR_SMP_BT
		kStartSMPAdvertising
#endif
	};

	using Handler = void (*)(AppEvent *);

	Type mType;
	Handler mHandler;
};
