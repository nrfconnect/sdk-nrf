/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <controller/CHIPDeviceController.h>
#include <transport/raw/UDP.h>

#include <atomic>

/** @class LightSwitch
 *  @brief Class for controlling a CHIP light bulb over a Thread network
 *
 *  Features:
 *  - discovering a CHIP light bulb which advertises itself by sending Thread multicast packets
 *  - toggling and dimming the connected CHIP light bulb by sending appropriate CHIP messages
 */
class LightSwitch {
public:
	CHIP_ERROR Init();

	void StartDiscovery();
	void StopDiscovery();
	bool IsDiscoveryEnabled() const;

	CHIP_ERROR Pair(const chip::Inet::IPAddress &lightBulbAddress);
	CHIP_ERROR ToggleLight();
	CHIP_ERROR SetLightLevel(uint8_t level);

private:
	class DiscoveryHandler : public chip::Transport::RawTransportDelegate {
	public:
		void SetEnabled(bool enabled) { mEnabled.store(enabled, std::memory_order_relaxed); }

		void HandleMessageReceived(const chip::PacketHeader &header, const chip::Transport::PeerAddress &source,
					   chip::System::PacketBufferHandle msgBuf) override;

	private:
		std::atomic_bool mEnabled;
	};

	chip::Transport::UDP mDiscoveryServiceEndpoint;
	chip::Controller::DeviceCommissioner mCommissioner;
	DiscoveryHandler mDiscoveryHandler;
};
