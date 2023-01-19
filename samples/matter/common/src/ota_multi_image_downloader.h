/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <zephyr/storage/stream_flash.h>

#include <platform/CHIPDeviceLayer.h>

#include <array>

using namespace chip;

/* A helper class to facilitate the multi variant application OTA DFU
   (Thread/WiFi switchable)  with the stream_flash backend */
class OTAMultiImageDownloader {
public:
	OTAMultiImageDownloader(int imageId, size_t offset, size_t size, size_t targetOffset, size_t targetSize)
		: mImgId(imageId), mOffset(offset), mImgSize(size), mTargetOffset(targetOffset),
		  mTargetImgSize(targetSize)
	{
	}

	int Init(const device *flash, ssize_t size);
	int Write(const uint8_t *chunk, size_t chunkSize);
	int Finalize();
	int Apply(const device *flash);
	int GetImageId() { return mImgId; }
	void Cleanup() { Platform::MemoryFree(mBuffer); }

private:
	int Init(const device *flash, size_t offset, size_t size);

	int mImgId;
	const size_t mOffset;
	const size_t mImgSize;
	const size_t mTargetOffset;
	const size_t mTargetImgSize;
	uint8_t *mBuffer;
	stream_flash_ctx mStream;

	static constexpr size_t kBufferSize = CONFIG_CHIP_OTA_REQUESTOR_BUFFER_SIZE;
};

/* Wrapper for a collection of OTAMultiImageDownloader static objects */
class OTAMultiImageDownloaders {
public:
	static OTAMultiImageDownloader *GetCurrentImage()
	{
		if (sCurrentId > sImageHandlers.size()) {
			return nullptr;
		}
		return &sImageHandlers[sCurrentId];
	}

	static int Open(int id, size_t size);
	static int Write(const uint8_t *chunk, size_t chunk_size);
	static int Close(bool success);
	static int Apply();
	static bool DfuTargetActionNeeded(int id);
	static void SetCurrentImageId(size_t id) { sCurrentId = id; }
	static size_t CurrentImageId() { return sCurrentId; }

private:
	enum CurrentImageID : int { NONE = -1, APP_1_CORE_APP, APP_1_CORE_NET, APP_2_CORE_APP, APP_2_CORE_NET, LIMIT };

	enum RunningVariant : int { THREAD = 1, WIFI };

	static size_t sCurrentId;
	static std::array<OTAMultiImageDownloader, CurrentImageID::LIMIT> sImageHandlers;
};
