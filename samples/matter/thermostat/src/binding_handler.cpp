/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "binding_handler.h"
#include "temp_sensor_manager.h"
#include "temperature_measurement/sensor.h"

#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/ids/Commands.h>
#include <app/CommandSender.h>
#include <app/OperationalSessionSetup.h>
#include <app/clusters/bindings/BindingManager.h>
#include <app/clusters/bindings/bindings.h>
#include <app/data-model/Decode.h>
#include <app/util/binding-table.h>
#include <controller/InvokeInteraction.h>
#include <controller/ReadInteraction.h>
#include <lib/support/CodeUtils.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::app;

void BindingHandler::Init()
{
	DeviceLayer::PlatformMgr().ScheduleWork(InitInternal);
}

void BindingHandler::TemperatureMeasurementReadHandler(const EmberBindingTableEntry &binding,
						       OperationalDeviceProxy *peer_device, void *context)
{
	VerifyOrReturn(context != nullptr, LOG_ERR("Invalid context for Thermostat handler"););
	BindingData *data = static_cast<BindingData *>(context);

	TemperatureMeasurementUnicastBindingRead(data, binding, peer_device);
}

void BindingHandler::TemperatureMeasurementUnicastBindingRead(BindingData *data, const EmberBindingTableEntry &binding,
							      OperationalDeviceProxy *peer_device)
{
	auto onSuccess = [](const ConcreteDataAttributePath &attributePath, const auto &dataResponse) {
		ChipLogProgress(NotSpecified, "Read Thermostat attribute succeeded");
		int16_t responseValue = dataResponse.Value();
		if (!(dataResponse.IsNull())) {
			TempSensorManager::Instance().SetLocalTemperature(responseValue);
		}
	};

	auto onFailure = [](const ConcreteDataAttributePath *attributePath, CHIP_ERROR error) {
		ChipLogError(NotSpecified, "Read Thermostat attribute failed: %" CHIP_ERROR_FORMAT, error.Format());
	};

	CHIP_ERROR ret = CHIP_NO_ERROR;

	VerifyOrDie(peer_device != nullptr && peer_device->ConnectionReady());

	Controller::ReadAttribute<Clusters::TemperatureMeasurement::Attributes::MeasuredValue::TypeInfo>(
		peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote, onSuccess,
		onFailure);
}

void BindingHandler::TemperatureMeasurementContextReleaseHandler(void *context)
{
	VerifyOrReturn(context != nullptr, LOG_ERR("Invalid context for Thermostat context release handler"));

	Platform::Delete(static_cast<BindingData *>(context));
}

void BindingHandler::InitInternal(intptr_t aArg)
{
	auto &server = Server::GetInstance();
	if (CHIP_NO_ERROR !=
	    BindingManager::GetInstance().Init(
		    { &server.GetFabricTable(), server.GetCASESessionManager(), &server.GetPersistentStorage() })) {
		LOG_ERR("BindingHandler::InitInternal failed");
	}

	BindingManager::GetInstance().RegisterBoundDeviceChangedHandler(TemperatureMeasurementReadHandler);
	BindingManager::GetInstance().RegisterBoundDeviceContextReleaseHandler(
		TemperatureMeasurementContextReleaseHandler);
	BindingHandler::GetInstance().PrintBindingTable();
}

bool BindingHandler::IsGroupBound()
{
	BindingTable &bindingTable = BindingTable::GetInstance();

	for (auto &entry : bindingTable) {
		if (EMBER_MULTICAST_BINDING == entry.type) {
			return true;
		}
	}
	return false;
}

void BindingHandler::PrintBindingTable()
{
	BindingTable &bindingTable = BindingTable::GetInstance();

	LOG_INF("Binding Table size: [%d]:", bindingTable.Size());
	uint8_t i = 0;
	for (auto &entry : bindingTable) {
		switch (entry.type) {
		case EMBER_UNICAST_BINDING:
			LOG_INF("[%d] UNICAST:", i++);
			LOG_INF("\t\t+ Fabric: %d\n \
            \t+ LocalEndpoint %d \n \
            \t+ ClusterId %d \n \
            \t+ RemoteEndpointId %d \n \
            \t+ NodeId %d",
				(int)entry.fabricIndex, (int)entry.local, (int)entry.clusterId.Value(),
				(int)entry.remote, (int)entry.nodeId);
			break;
		case EMBER_MULTICAST_BINDING:
			LOG_INF("[%d] GROUP:", i++);
			LOG_INF("\t\t+ Fabric: %d\n \
            \t+ LocalEndpoint %d \n \
            \t+ RemoteEndpointId %d \n \
            \t+ GroupId %d",
				(int)entry.fabricIndex, (int)entry.local, (int)entry.remote, (int)entry.groupId);
			break;
		case EMBER_UNUSED_BINDING:
			LOG_INF("[%d] UNUSED", i++);
			break;
		default:
			break;
		}
	}
}

void BindingHandler::ThermostatWorkerHandler(intptr_t aContext)
{
	VerifyOrReturn(aContext != 0, LOG_ERR("Invalid Swich data"));
	BindingData *data = reinterpret_cast<BindingData *>(aContext);
	if (BindingTable::GetInstance().Size() != 0) {
		LOG_INF("Notify Bounded Cluster | endpoint: %d cluster: %d", data->localEndpointId, data->clusterId);
		BindingManager::GetInstance().NotifyBoundClusterChanged(data->localEndpointId, data->clusterId,
									static_cast<void *>(data));
	} else {
		LOG_INF("NO SENSOR CONNECTED TO THERMOSTAT DEVICE ");
	}
}
