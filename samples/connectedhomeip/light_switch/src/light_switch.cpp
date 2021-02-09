/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "light_switch.h"

#include "app_task.h"

#include <platform/CHIPDeviceLayer.h>
#include <app/chip-zcl-zpro-codec.h>
#include <support/ErrorStr.h>

#include <logging/log.h>
#include <net/net_ip.h>

LOG_MODULE_DECLARE(app);

namespace
{
static constexpr chip::EndpointId kLightBulbEndpointId = 1;
static constexpr uint16_t kDiscoveryPort = 12345;
static constexpr const char kDiscoveryMagicString[] = "chip-light-bulb";
} /* namespace */

void LightSwitch::DiscoveryHandler::HandleMessageReceived(const chip::PacketHeader &header,
							  const chip::Transport::PeerAddress &source,
							  chip::System::PacketBufferHandle msgBuf)
{
	if (!mEnabled.load(std::memory_order_relaxed)) {
		return;
	}

	if (msgBuf->DataLength() != sizeof(kDiscoveryMagicString) ||
	    memcmp(msgBuf->Start(), kDiscoveryMagicString, sizeof(kDiscoveryMagicString)) != 0) {
		LOG_ERR("Discovery service received invalid message");
		return;
	}

	GetAppTask().PostEvent(AppEvent(AppEvent::LightFound, source.GetIPAddress()));
}

CHIP_ERROR LightSwitch::Init()
{
	CHIP_ERROR err = mCommissioner.Init(chip::kTestControllerNodeId, nullptr, nullptr);

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("DeviceCommissioner::Init() failed: %s", chip::ErrorStr(err));
		return err;
	}

	err = mCommissioner.ServiceEvents();

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("DeviceCommissioner::ServiceEvents() failed: %s", chip::ErrorStr(err));
		return err;
	}

	auto params = chip::Transport::UdpListenParameters(&chip::DeviceLayer::InetLayer).SetListenPort(kDiscoveryPort);
	err = mDiscoveryServiceEndpoint.Init(params);

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Transport::UDP::Init() failed: %s", chip::ErrorStr(err));
		return err;
	}

	mDiscoveryHandler.SetEnabled(true);
	mDiscoveryServiceEndpoint.SetDelegate(&mDiscoveryHandler);
	return CHIP_NO_ERROR;
}

void LightSwitch::StartDiscovery()
{
	LOG_INF("Starting light bulb discovery");
	mDiscoveryHandler.SetEnabled(true);
}

void LightSwitch::StopDiscovery()
{
	LOG_INF("Stopping light bulb discovery");
	mDiscoveryHandler.SetEnabled(false);
}

CHIP_ERROR LightSwitch::Pair(const chip::Inet::IPAddress &lightBulbAddress)
{
	{
		char addrStr[NET_IPV6_ADDR_LEN];
		LOG_INF("Pairing with light bulb %s", lightBulbAddress.ToString(addrStr, NET_IPV6_ADDR_LEN));
	}

	CHIP_ERROR err = CHIP_NO_ERROR;

	{
		chip::Controller::SerializedDevice serializedDevice;
		err = mCommissioner.PairTestDeviceWithoutSecurity(chip::kTestDeviceNodeId, lightBulbAddress,
								  serializedDevice, CHIP_PORT);
	}

	if (err != CHIP_NO_ERROR)
		LOG_ERR("Pairing failed: %s", chip::ErrorStr(err));

	return err;
}

CHIP_ERROR LightSwitch::ToggleLight()
{
	LOG_INF("Toggling the light");

	chip::Controller::Device *device;
	CHIP_ERROR err = mCommissioner.GetDevice(chip::kTestDeviceNodeId, &device);

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("No light bulb device is paired");
		return err;
	}

	auto buffer = encodeOnOffClusterToggleCommand(0, kLightBulbEndpointId);
	if (buffer.IsNull()) {
		return CHIP_ERROR_NO_MEMORY;
	}

	return device->SendMessage(std::move(buffer));
}

CHIP_ERROR LightSwitch::SetLightLevel(uint8_t level)
{
	LOG_INF("Setting brightness level to %u", level);

	chip::Controller::Device *device;
	CHIP_ERROR err = mCommissioner.GetDevice(chip::kTestDeviceNodeId, &device);

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("No light bulb device is paired");
		return err;
	}

	auto buffer = encodeLevelControlClusterMoveToLevelCommand(0, kLightBulbEndpointId, level, 0, 0, 0);
	if (buffer.IsNull()) {
		return CHIP_ERROR_NO_MEMORY;
	}

	return device->SendMessage(std::move(buffer));
}
