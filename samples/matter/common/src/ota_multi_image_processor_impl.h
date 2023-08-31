/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "ota_image_processor_base_impl.h"
#include <platform/nrfconnect/ExternalFlashManager.h>
#include <platform/nrfconnect/OTAImageProcessorImpl.h>

using namespace chip;
using namespace chip::DeviceLayer;

class OTAMultiImageProcessorImpl : public OTAImageProcessorBaseImpl {
public:
	explicit OTAMultiImageProcessorImpl(ExternalFlashManager *flashHandler = nullptr)
		: OTAImageProcessorBaseImpl(flashHandler)
	{
	}

	CHIP_ERROR PrepareDownload() override;
	CHIP_ERROR ConfirmCurrentImage() override;
	void SetImageConfirmed() { mImageConfirmed = true; }

protected:
	CHIP_ERROR PrepareMultiDownload();

private:
    bool mImageConfirmed = false;
};
