/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ota_multi_image_processor_impl.h"
#include "ota_multi_image_downloader.h"

#include <app/clusters/ota-requestor/OTADownloader.h>
#include <dfu/dfu_multi_image.h>
#include <dfu/dfu_target_mcuboot.h>
#include <platform/CHIPDeviceLayer.h>
#include <zephyr/dfu/mcuboot.h>

CHIP_ERROR OTAMultiImageProcessorImpl::PrepareDownload()
{
	VerifyOrReturnError(mDownloader != nullptr, CHIP_ERROR_INCORRECT_STATE);

	TriggerFlashAction(ExternalFlashManager::Action::WAKE_UP);

#ifdef CONFIG_WIFI_NRF700X
	ReturnErrorOnFailure(WiFiManager::Instance().SetLowPowerMode(false));
#endif /* CONFIG_WIFI_NRF700X */

	return DeviceLayer::SystemLayer().ScheduleLambda(
		[this] { mDownloader->OnPreparedForDownload(PrepareMultiDownload()); });
}

CHIP_ERROR OTAMultiImageProcessorImpl::PrepareMultiDownload()
{
	mHeaderParser.Init();
	mParams = {};
	ReturnErrorOnFailure(System::MapErrorZephyr(dfu_target_mcuboot_set_buf(mBuffer, sizeof(mBuffer))));
	ReturnErrorOnFailure(System::MapErrorZephyr(dfu_multi_image_init(mBuffer, sizeof(mBuffer))));

	for (int image_id = 0; image_id < CONFIG_DFU_MULTI_IMAGE_MAX_IMAGE_COUNT; ++image_id) {
		dfu_image_writer writer;
		writer.image_id = image_id;
		writer.open = OTAMultiImageDownloaders::Open;
		writer.write = OTAMultiImageDownloaders::Write;
		writer.close = OTAMultiImageDownloaders::Close;

		ReturnErrorOnFailure(System::MapErrorZephyr(dfu_multi_image_register_writer(&writer)));
	};
	return CHIP_NO_ERROR;
}

CHIP_ERROR OTAMultiImageProcessorImpl::ConfirmCurrentImage()
{
	CHIP_ERROR err = mImageConfirmed ? CHIP_NO_ERROR : CHIP_ERROR_INCORRECT_STATE;
	if (err == CHIP_NO_ERROR) {
		err = System::MapErrorZephyr(OTAMultiImageDownloaders::Apply());
	}
	return err;
}
