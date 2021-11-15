/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "light_switch.h"

#include "app_task.h"

#include <controller/CHIPDeviceControllerFactory.h>
#include <controller/CommissioneeDeviceProxy.h>
#include <crypto/CHIPCryptoPAL.h>
#include <lib/support/ErrorStr.h>
#include <lib/support/ScopedBuffer.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/KeyValueStoreManager.h>
#include <zap-generated/CHIPClusters.h>

#include <logging/log.h>
#include <net/net_ip.h>

LOG_MODULE_DECLARE(app);

namespace
{
static constexpr chip::EndpointId kLightBulbEndpointId = 1;
static constexpr uint16_t kDiscoveryPort = 12345;
static constexpr const char kDiscoveryMagicString[] = "chip-light-bulb";
} /* namespace */

void LightSwitch::DiscoveryHandler::HandleMessageReceived(const chip::Transport::PeerAddress &source,
							  chip::System::PacketBufferHandle &&msgBuf)
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

CHIP_ERROR
LightSwitch::PlaformPersistentStorageDelegate::SyncGetKeyValue(const char *key, void *buffer, uint16_t &size)
{
	return chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(key, buffer, size);
}

CHIP_ERROR
LightSwitch::PlaformPersistentStorageDelegate::SyncSetKeyValue(const char *key, const void *value, uint16_t size)
{
	return chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Put(key, value, size);
}

CHIP_ERROR LightSwitch::PlaformPersistentStorageDelegate::SyncDeleteKeyValue(const char *key)
{
	return chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Delete(key);
}

CHIP_ERROR LightSwitch::Init()
{
	CHIP_ERROR err = mOpCredDelegate.Initialize(mStorageDelegate);

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ExampleOperationalCredentialsIssuer::Initialize() failed: %s", chip::ErrorStr(err));
		return err;
	}

	chip::Crypto::P256Keypair ephemeralKey;
	err = ephemeralKey.Initialize();

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("P256Keypair::Initialize() failed: %s", chip::ErrorStr(err));
		return err;
	}

	chip::Platform::ScopedMemoryBuffer<uint8_t> rcac;
	chip::Platform::ScopedMemoryBuffer<uint8_t> icac;
	chip::Platform::ScopedMemoryBuffer<uint8_t> noc;

	if (!rcac.Alloc(chip::Controller::kMaxCHIPDERCertLength) ||
	    !icac.Alloc(chip::Controller::kMaxCHIPDERCertLength) ||
	    !noc.Alloc(chip::Controller::kMaxCHIPDERCertLength)) {
		LOG_ERR("Failed to allocated memory for certificate chain");
		return CHIP_ERROR_NO_MEMORY;
	}

	chip::MutableByteSpan rcacSpan(rcac.Get(), chip::Controller::kMaxCHIPDERCertLength);
	chip::MutableByteSpan icacSpan(icac.Get(), chip::Controller::kMaxCHIPDERCertLength);
	chip::MutableByteSpan nocSpan(noc.Get(), chip::Controller::kMaxCHIPDERCertLength);

	err = mOpCredDelegate.GenerateNOCChainAfterValidation(chip::kTestControllerNodeId, 0, ephemeralKey.Pubkey(),
							      rcacSpan, icacSpan, nocSpan);

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Failed to issue certificate chain: %s", chip::ErrorStr(err));
		return err;
	}

	chip::Controller::SetupParams commissionerSetupParams = {};
	commissionerSetupParams.storageDelegate = &mStorageDelegate;
	commissionerSetupParams.operationalCredentialsDelegate = &mOpCredDelegate;
	commissionerSetupParams.ephemeralKeypair = &ephemeralKey;
	commissionerSetupParams.controllerRCAC = chip::ByteSpan(rcac.Get(), chip::Controller::kMaxCHIPDERCertLength);
	commissionerSetupParams.controllerICAC = chip::ByteSpan(icac.Get(), chip::Controller::kMaxCHIPDERCertLength);
	commissionerSetupParams.controllerNOC = chip::ByteSpan(noc.Get(), chip::Controller::kMaxCHIPDERCertLength);

	err = mFabricStorage.Initialize(&mStorageDelegate);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("mFabricStorage.Initialize() failed: %s", chip::ErrorStr(err));
		return err;
	}

	chip::Controller::FactoryInitParams factoryParams;
	factoryParams.fabricStorage = &mFabricStorage;
	factoryParams.listenPort = CHIP_PORT + 2;

	err = chip::Controller::DeviceControllerFactory::GetInstance().Init(factoryParams);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("DeviceControllerFactory::Init() failed: %s", chip::ErrorStr(err));
		return err;
	}

	err = chip::Controller::DeviceControllerFactory::GetInstance().SetupCommissioner(commissionerSetupParams,
											 mCommissioner);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("DeviceControllerFactory::SetupCommissioner() failed: %s", chip::ErrorStr(err));
		return err;
	}

	err = chip::Controller::DeviceControllerFactory::GetInstance().ServiceEvents();

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("DeviceCommissioner::ServiceEvents() failed: %s", chip::ErrorStr(err));
		return err;
	}

	auto params =
		chip::Transport::UdpListenParameters(&chip::DeviceLayer::InetLayer()).SetListenPort(kDiscoveryPort);
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
		err = mCommissioner.PairTestDeviceWithoutSecurity(
			chip::kTestDeviceNodeId, chip::Transport::PeerAddress::UDP(lightBulbAddress, CHIP_PORT));
	}

	if (err != CHIP_NO_ERROR)
		LOG_ERR("Pairing failed: %s", chip::ErrorStr(err));

	return err;
}

CHIP_ERROR LightSwitch::ToggleLight()
{
	LOG_INF("Toggling the light");

	chip::CommissioneeDeviceProxy *device;
	CHIP_ERROR err = mCommissioner.GetDeviceBeingCommissioned(chip::kTestDeviceNodeId, &device);

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("No light bulb device is paired");
		return err;
	}

	chip::Controller::OnOffCluster cluster;
	cluster.Associate(device, kLightBulbEndpointId);

	return cluster.Toggle(nullptr, nullptr);
}

CHIP_ERROR LightSwitch::SetLightLevel(uint8_t level)
{
	LOG_INF("Setting brightness level to %u", level);

	chip::CommissioneeDeviceProxy *device;
	CHIP_ERROR err = mCommissioner.GetDeviceBeingCommissioned(chip::kTestDeviceNodeId, &device);

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("No light bulb device is paired");
		return err;
	}

	chip::Controller::LevelControlCluster cluster;
	cluster.Associate(device, kLightBulbEndpointId);

	return cluster.MoveToLevel(nullptr, nullptr, level, 0, 0, 0);
}
