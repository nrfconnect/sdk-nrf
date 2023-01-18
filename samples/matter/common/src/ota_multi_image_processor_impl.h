/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <platform/nrfconnect/ExternalFlashManager.h>
#include <platform/nrfconnect/OTAImageProcessorImpl.h>

using namespace chip;
using namespace chip::DeviceLayer;

class OTAMultiImageProcessorImpl : public OTAImageProcessorImpl {
public:
	explicit OTAMultiImageProcessorImpl(ExternalFlashManager *flashHandler = nullptr)
		: OTAImageProcessorImpl(flashHandler)
	{
	}

	CHIP_ERROR PrepareDownload() override;
	CHIP_ERROR ConfirmCurrentImage() override;

protected:
	CHIP_ERROR PrepareMultiDownload();
};
