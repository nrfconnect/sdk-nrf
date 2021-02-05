/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nfc_widget.h"

#include <nfc/ndef/uri_msg.h>
#include <nfc/ndef/uri_rec.h>
#include <nfc_t2t_lib.h>
#include <zephyr.h>

int NFCWidget::Init(chip::DeviceLayer::ConnectivityManager &mgr)
{
	mIsTagStarted = false;
	return nfc_t2t_setup(FieldDetectionHandler, &mgr);
}

int NFCWidget::StartTagEmulation(const char *tagPayload, uint8_t tagPayloadLength)
{
	uint32_t len = sizeof(mNdefBuffer);
	int result = 0;

	result = nfc_ndef_uri_msg_encode(NFC_URI_NONE, reinterpret_cast<const uint8_t *>(tagPayload), tagPayloadLength,
					 mNdefBuffer, &len);
	VerifyOrExit(result >= 0, ChipLogProgress(AppServer, "nfc_ndef_uri_msg_encode failed: %d", result));

	result = nfc_t2t_payload_set(mNdefBuffer, len);
	VerifyOrExit(result >= 0, ChipLogProgress(AppServer, "nfc_t2t_payload_set failed: %d", result));

	result = nfc_t2t_emulation_start();
	VerifyOrExit(result >= 0, ChipLogProgress(AppServer, "nfc_t2t_emulation_start failed: %d", result));

	mIsTagStarted = true;

exit:
	return result;
}

int NFCWidget::StopTagEmulation()
{
	int result = 0;

	VerifyOrExit(IsTagEmulationStarted(), );

	result = nfc_t2t_emulation_stop();
	VerifyOrExit(result >= 0, ChipLogProgress(AppServer, "nfc_t2t_emulation_stop failed: %d", result));

	memset(mNdefBuffer, 0, sizeof(mNdefBuffer));
	mIsTagStarted = false;

exit:
	return result;
}

bool NFCWidget::IsTagEmulationStarted() const
{
	return mIsTagStarted;
}

void NFCWidget::FieldDetectionHandler(void *context, nfc_t2t_event event, const uint8_t *data, size_t data_length)
{
	ARG_UNUSED(context);
	ARG_UNUSED(event);
	ARG_UNUSED(data);
	ARG_UNUSED(data_length);
}
