/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <platform/CHIPDeviceLayer.h>

#include <zephyr/kernel.h>

typedef void (*DFUOverSMPRestartAdvertisingHandler)(void);

class DFUOverSMP {
public:
	void Init(DFUOverSMPRestartAdvertisingHandler startAdvertisingCb);
	void ConfirmNewImage();
	void StartServer();
	void StartBLEAdvertising();
	bool IsEnabled() { return mIsEnabled; }
	static int32_t UploadConfirmHandler(uint32_t event, int32_t rc, bool *abort_more,
					    void *data, size_t data_size);
	static int32_t CommandHandler(uint32_t event, int32_t rc, bool *abort_more,
				      void *data, size_t data_size);

private:
	friend DFUOverSMP &GetDFUOverSMP(void);

	static void OnBleDisconnect(bt_conn *conn, uint8_t reason);
	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);

	bool mIsEnabled;
	bool mIsAdvertisingEnabled;
	bt_conn_cb mBleConnCallbacks;
	DFUOverSMPRestartAdvertisingHandler restartAdvertisingCallback;

	static DFUOverSMP sDFUOverSMP;
};

inline DFUOverSMP &GetDFUOverSMP(void)
{
	return DFUOverSMP::sDFUOverSMP;
}
