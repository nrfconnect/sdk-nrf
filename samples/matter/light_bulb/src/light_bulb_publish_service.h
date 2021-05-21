/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <inet/InetConfig.h>
#include <transport/raw/UDP.h>

#include <zephyr.h>

#include <string>

using namespace ::chip::Inet;
using namespace ::chip::Transport;

class LightBulbPublishService {
public:
	void Init();
	void Start(uint32_t aTimeoutInMs, uint32_t aIntervalInMs);
	void Stop();
	void Publish();

private:
	friend LightBulbPublishService &LightBulbPublishServ();

	static constexpr uint16_t kServiceUDPPort = 12345;

	bool mIsServiceStarted;
	uint32_t mPublishInterval;
	uint32_t mPublishIterations;
	k_timer mPublishTimer;
	UDP mUdp;
	uint32_t mMessageId;
	IPAddress mPublishDestAddr;

	static void PublishTimerHandler(k_timer *aTimer);
};
