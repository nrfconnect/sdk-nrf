/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "binding_handler.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

namespace Nrf::Matter
{
	void BindingHandler::Init()
	{
		InitInternal();
	}

	void BindingHandler::RunBoundClusterAction(BindingData *bindingData)
	{
		VerifyOrReturn(bindingData != nullptr, LOG_ERR("Invalid binding data"));
		VerifyOrReturn(bindingData->InvokeCommandFunc != nullptr,
			       LOG_ERR("No valid InvokeCommandFunc assigned"););

		DeviceLayer::PlatformMgr().ScheduleWork(DeviceWorkerHandler, reinterpret_cast<intptr_t>(bindingData));
	}

	void BindingHandler::OnInvokeCommandSucces(BindingData *bindingData)
	{
		VerifyOrReturn(bindingData != nullptr, LOG_ERR("Invalid binding data"));
		LOG_DBG("Binding command applied successfully!");

		/* If session was recovered and communication works, reset flag to the initial state. */
		if (bindingData->CaseSessionRecovered)
			bindingData->CaseSessionRecovered = false;
		Platform::Delete<BindingData>(bindingData);
	}

	void BindingHandler::OnInvokeCommandFailure(BindingData *bindingData, CHIP_ERROR Error)
	{
		CHIP_ERROR error;
		VerifyOrDie(bindingData != nullptr);

		if (Error == CHIP_ERROR_TIMEOUT && !bindingData->CaseSessionRecovered) {
			LOG_INF("Response timeout for invoked command, trying to recover CASE session.");

			/* Set flag to not try recover session multiple times. */
			bindingData->CaseSessionRecovered = true;

			/* Establish new CASE session and retrasmit command that was not applied. */
			error = Binding::Manager::GetInstance().NotifyBoundClusterChanged(
				bindingData->EndpointId, bindingData->ClusterId, static_cast<void *>(bindingData));

			if (CHIP_NO_ERROR != error) {
				LOG_ERR("NotifyBoundClusterChanged failed due to: %" CHIP_ERROR_FORMAT, error.Format());
			}
		} else {
			Platform::Delete<BindingData>(bindingData);
			LOG_ERR("Binding command was not applied! Reason: %" CHIP_ERROR_FORMAT, Error.Format());
		}
	}

	void BindingHandler::DeviceChangedCallback(const Binding::TableEntry &binding,
						   OperationalDeviceProxy *deviceProxy, void *context)
	{
		VerifyOrReturn(context != nullptr, LOG_ERR("Invalid context for device handler"));
		BindingData *data = static_cast<BindingData *>(context);

		if (binding.type == Binding::MATTER_MULTICAST_BINDING) {

			if (data->IsGroup.HasValue() && !data->IsGroup.Value()) {
				return;
			}

			data->InvokeCommandFunc(binding, nullptr, *data);
		} else if (binding.type == Binding::MATTER_UNICAST_BINDING) {

			if (data->IsGroup.HasValue() && data->IsGroup.Value()) {
				return;
			}

			data->InvokeCommandFunc(binding, deviceProxy, *data);
		}
	}

	void BindingHandler::DeviceContextReleaseHandler(void *context)
	{
		VerifyOrDie(context != 0);

		Platform::Delete(static_cast<BindingData *>(context));
	}

	void BindingHandler::InitInternal()
	{
		LOG_INF("Initialize binding Handler");
		auto &server = Server::GetInstance();
		if (CHIP_NO_ERROR !=
		    Binding::Manager::GetInstance().Init({ &server.GetFabricTable(), server.GetCASESessionManager(),
							 &server.GetPersistentStorage() })) {
			LOG_ERR("BindingHandler::InitInternal failed");
		}

		Binding::Manager::GetInstance().RegisterBoundDeviceChangedHandler(DeviceChangedCallback);
		Binding::Manager::GetInstance().RegisterBoundDeviceContextReleaseHandler(DeviceContextReleaseHandler);
		BindingHandler::PrintBindingTable();
	}

	void BindingHandler::PrintBindingTable()
	{
		Binding::Table &bindingTable = Binding::Table::GetInstance();

		LOG_INF("Binding Table size: [%d]:", bindingTable.Size());
		uint8_t i = 0;
		for (auto &entry : bindingTable) {
			switch (entry.type) {
			case Binding::MATTER_UNICAST_BINDING:
				LOG_INF("[%d] UNICAST:", i++);
				LOG_INF("\t\t+ Fabric: %d\n \
            \t+ LocalEndpoint %d \n \
            \t+ ClusterId %d \n \
            \t+ RemoteEndpointId %d \n \
            \t+ NodeId %d",
					(int)entry.fabricIndex, (int)entry.local, (int)entry.clusterId.value(),
					(int)entry.remote, (int)entry.nodeId);
				break;
			case Binding::MATTER_MULTICAST_BINDING:
				LOG_INF("[%d] GROUP:", i++);
				LOG_INF("\t\t+ Fabric: %d\n \
            \t+ LocalEndpoint %d \n \
            \t+ RemoteEndpointId %d \n \
            \t+ GroupId %d",
					(int)entry.fabricIndex, (int)entry.local, (int)entry.remote,
					(int)entry.groupId);
				break;
			case Binding::MATTER_UNUSED_BINDING:
				LOG_INF("[%d] UNUSED", i++);
				break;
			default:
				break;
			}
		}
	}

	void BindingHandler::DeviceWorkerHandler(intptr_t context)
	{
		VerifyOrDie(context != 0);
		BindingData *data = reinterpret_cast<BindingData *>(context);

		if (Binding::Table::GetInstance().Size() != 0) {
			LOG_INF("Notify Bounded Cluster | endpoint: %d cluster: %d", data->EndpointId, data->ClusterId);
			Binding::Manager::GetInstance().NotifyBoundClusterChanged(data->EndpointId, data->ClusterId,
										static_cast<void *>(data));
		} else {
			LOG_INF("NO DEVICE BOUND");
		}
	}

} /* namespace Nrf::Matter */
