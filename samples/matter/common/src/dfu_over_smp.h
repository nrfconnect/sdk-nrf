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

private:
	friend DFUOverSMP &GetDFUOverSMP(void);

	static int UploadConfirmHandler(const struct img_mgmt_upload_req req,
					const struct img_mgmt_upload_action action);
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
