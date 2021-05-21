/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "light_bulb_publish_service.h"

#include "app_task.h"

#include <logging/log.h>
#include <zephyr.h>

#include <inet/InetConfig.h>
#include <platform/CHIPDeviceLayer.h>
#include <transport/TransportMgr.h>
#include <transport/raw/UDP.h>

LOG_MODULE_DECLARE(app);

const char kPublishServicePayload[] = "chip-light-bulb";
const char kPublishDestAddr[] = "ff03::1";

using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::Transport;
using namespace ::chip::DeviceLayer;

void LightBulbPublishService::Init()
{
	mIsServiceStarted = false;

	/* Initialize timer */
	k_timer_init(&mPublishTimer, &PublishTimerHandler, nullptr);
	k_timer_user_data_set(&mPublishTimer, this);

	/* Initialize UDP endpoint */
	UdpListenParameters udpParams(&chip::DeviceLayer::InetLayer);
	udpParams.SetListenPort(kServiceUDPPort);
	if (mUdp.Init(udpParams)) {
		LOG_ERR("LightBulbPublishService initialization failed");
	}

	mMessageId = 0;
	IPAddress::FromString(kPublishDestAddr, mPublishDestAddr);
}

void LightBulbPublishService::Start(uint32_t aTimeoutInMs, uint32_t aIntervalInMs)
{
	if (!mIsServiceStarted) {
		if (aTimeoutInMs >= aIntervalInMs) {
			LOG_INF("Started Publish service");

			mPublishInterval = aIntervalInMs;
			/* Calculate number of publish iterations to do considering given timeout and interval values.
			 */
			mPublishIterations = aTimeoutInMs / aIntervalInMs;
			mIsServiceStarted = true;

			k_timer_start(&mPublishTimer, K_MSEC(aIntervalInMs), K_NO_WAIT);
		} else {
			LOG_ERR("Publish service cannot be started: service timeout is greater than interval");
		}
	} else {
		LOG_INF("Publish service is already running");
	}
}

void LightBulbPublishService::Stop()
{
	if (mIsServiceStarted) {
		LOG_INF("Stopped Publish service");

		k_timer_stop(&mPublishTimer);

		mIsServiceStarted = false;
		mMessageId = 0;
	} else {
		LOG_INF("Publish service is not running");
	}
}

void LightBulbPublishService::Publish()
{
	if (!chip::DeviceLayer::ConnectivityMgrImpl().IsThreadAttached()) {
		return;
	}

	const uint16_t payloadLength = sizeof(kPublishServicePayload);

	auto buffer = chip::System::PacketBufferHandle::NewWithData(kPublishServicePayload, payloadLength);
	if (!buffer.IsNull()) {
		/* Subsequent messages are assumed to have increasing IDs. */
		PacketHeader header;
		header.SetMessageId(mMessageId++);

		if (mUdp.SendMessage(header, Transport::PeerAddress::UDP(mPublishDestAddr, kServiceUDPPort),
				     std::move(buffer)) == CHIP_NO_ERROR) {
			LOG_INF("LightBulbPublishService message sent successfully");
		} else {
			LOG_INF("LightBulbPublishService message sending failed");
		}
	}
}

void LightBulbPublishService::PublishTimerHandler(k_timer *aTimer)
{
	LightBulbPublishService *sLightBulbPublishService =
		reinterpret_cast<LightBulbPublishService *>(aTimer->user_data);
	if (sLightBulbPublishService->mPublishIterations > 0) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::PublishLightBulbService });
		k_timer_start(&(sLightBulbPublishService->mPublishTimer),
			      K_MSEC(sLightBulbPublishService->mPublishInterval), K_NO_WAIT);
		sLightBulbPublishService->mPublishIterations--;
	} else {
		sLightBulbPublishService->Stop();
	}
}
