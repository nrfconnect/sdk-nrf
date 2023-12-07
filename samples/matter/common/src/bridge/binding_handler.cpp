/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "binding_handler.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::app;

void BindingHandler::Init()
{
	DeviceLayer::PlatformMgr().ScheduleWork(InitInternal);
}

void BindingHandler::OnInvokeCommandFailure(BindingData &aBindingData, CHIP_ERROR aError)
{
	CHIP_ERROR error;

	if (aError == CHIP_ERROR_TIMEOUT && !aBindingData.CaseSessionRecovered) {
		LOG_INF("Response timeout for invoked command, trying to recover CASE session.");

		/* Set flag to not try recover session multiple times. */
		aBindingData.CaseSessionRecovered = true;

		/* Allocate new object to make sure its life time will be appropriate. */
		BindingHandler::BindingData *data = Platform::New<BindingHandler::BindingData>();
		*data = aBindingData;

		/* Establish new CASE session and retrasmit command that was not applied. */
		error = BindingManager::GetInstance().NotifyBoundClusterChanged(
			aBindingData.EndpointId, aBindingData.ClusterId, static_cast<void *>(data));

		if (CHIP_NO_ERROR != error) {
			LOG_ERR("NotifyBoundClusterChanged failed due to: %" CHIP_ERROR_FORMAT, error.Format());
			return;
		}
	} else {
		LOG_ERR("Binding command was not applied! Reason: %" CHIP_ERROR_FORMAT, aError.Format());
	}
}

void BindingHandler::DeviceChangedHandler(const EmberBindingTableEntry &binding, OperationalDeviceProxy *deviceProxy,
					  void *context)
{
	VerifyOrReturn(context != nullptr, LOG_ERR("Invalid context for device handler"););
	BindingData *data = static_cast<BindingData *>(context);

	if (binding.type == EMBER_MULTICAST_BINDING && data->IsGroup) {
		data->InvokeCommandFunc(data->CommandId, binding, nullptr, context);
	} else if (binding.type == EMBER_UNICAST_BINDING && !data->IsGroup) {
		data->InvokeCommandFunc(data->CommandId, binding, deviceProxy, context);
	}
}

void BindingHandler::DeviceContextReleaseHandler(void *context)
{
	VerifyOrReturn(context != nullptr, LOG_ERR("Invalid context for device context release handler"););

	Platform::Delete(static_cast<BindingData *>(context));
}

void BindingHandler::InitInternal(intptr_t aArg)
{
	LOG_INF("Initialize binding Handler");
	auto &server = Server::GetInstance();
	if (CHIP_NO_ERROR !=
	    BindingManager::GetInstance().Init(
		    { &server.GetFabricTable(), server.GetCASESessionManager(), &server.GetPersistentStorage() })) {
		LOG_ERR("BindingHandler::InitInternal failed");
	}

	BindingManager::GetInstance().RegisterBoundDeviceChangedHandler(DeviceChangedHandler);
	BindingManager::GetInstance().RegisterBoundDeviceContextReleaseHandler(DeviceContextReleaseHandler);
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

void BindingHandler::DeviceWorkerHandler(intptr_t aContext)
{
	VerifyOrReturn(aContext != 0, LOG_ERR("Invalid context data"));

	BindingData *data = reinterpret_cast<BindingData *>(aContext);
	LOG_INF("Notify Bounded Cluster | endpoint: %d cluster: %d", data->EndpointId, data->ClusterId);
	BindingManager::GetInstance().NotifyBoundClusterChanged(data->EndpointId, data->ClusterId,
								static_cast<void *>(data));
}
