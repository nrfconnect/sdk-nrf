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
	struct ImageLocation {
		const uint32_t app_address;
		const uint32_t app_size;
		const uint32_t net_address;
		const uint32_t net_size;
	};

	static SoftwareImagesSwapper &Instance()
	{
		static SoftwareImagesSwapper sSoftwareImagesSwapper;
		return sSoftwareImagesSwapper;
	};

	int Swap(const ImageLocation &source, SoftwareImagesSwapDoneCallback swapDoneCallback);

private:
	const struct device *mFlashDevice = DEVICE_DT_GET(DT_CHOSEN(nordic_pm_ext_flash));
	static constexpr uint16_t kBufferSize = 4096;

	bool mSwapInProgress = false;
	SoftwareImagesSwapDoneCallback swapDoneCallback;

	int SwapImage(uint32_t address, uint32_t size, uint8_t id);
};
