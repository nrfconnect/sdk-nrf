/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <app/ReadHandler.h>

class ICDUtil : public chip::app::ReadHandler::ApplicationCallback {
	CHIP_ERROR OnSubscriptionRequested(chip::app::ReadHandler &aReadHandler,
					   chip::Transport::SecureSession &aSecureSession) override;

	friend ICDUtil &GetICDUtil();
	static ICDUtil sICDUtil;
};

inline ICDUtil &GetICDUtil()
{
	return ICDUtil::sICDUtil;
}
