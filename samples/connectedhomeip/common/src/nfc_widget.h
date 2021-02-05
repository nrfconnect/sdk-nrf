/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <platform/CHIPDeviceLayer.h>

#include <nfc_t2t_lib.h>

class NFCWidget {
public:
	int Init(chip::DeviceLayer::ConnectivityManager &mgr);
	int StartTagEmulation(const char *tagPayload, uint8_t tagPayloadLength);
	int StopTagEmulation();
	bool IsTagEmulationStarted() const;

private:
	static void FieldDetectionHandler(void *context, nfc_t2t_event event, const uint8_t *data, size_t data_length);

	constexpr static uint8_t kNdefBufferSize = 128;

	uint8_t mNdefBuffer[kNdefBufferSize];
	bool mIsTagStarted;
};
