/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cstdint>

#include <zephyr/drivers/flash.h>

typedef void (*SoftwareImagesSwapDoneCallback)(void);

class SoftwareImagesSwapper {
public:
	enum class ApplicationImageId : uint8_t {
		Image_1 = 0,
		Image_2,
	};

	static SoftwareImagesSwapper &Instance()
	{
		static SoftwareImagesSwapper sSoftwareImagesSwapper;
		return sSoftwareImagesSwapper;
	};

	int Swap(ApplicationImageId applicationId, SoftwareImagesSwapDoneCallback swapDoneCallback);

private:
	const struct device *mFlashDevice = DEVICE_DT_GET(DT_CHOSEN(nordic_pm_ext_flash));
	static constexpr uint16_t kBufferSize = 256;

	bool mSwapInProgress = false;
	SoftwareImagesSwapDoneCallback swapDoneCallback;

	int SwapImage(uint32_t address, uint32_t size, uint8_t id);
};
