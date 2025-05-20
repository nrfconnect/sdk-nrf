/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#pragma once

#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/ids/Commands.h>
#include <app/CommandSender.h>
#include <app/clusters/bindings/BindingManager.h>
#include <controller/InvokeInteraction.h>
#include <lib/core/Optional.h>
#include <platform/CHIPDeviceLayer.h>

namespace Nrf::Matter
{
class BindingHandler {
public:
	struct BindingData;
	using InvokeCommand = void (*)(const EmberBindingTableEntry &, chip::OperationalDeviceProxy *, BindingData &);
	struct BindingData {
		chip::EndpointId EndpointId;
		chip::CommandId CommandId;
		chip::ClusterId ClusterId;
		InvokeCommand InvokeCommandFunc;
		uint8_t Value;
		/* It can be optionally forced to apply specific command only to the group (groupcast) or only to the specific
		 * device (unicast). The default behavior is to send data to all targets defined in the binding table.
		 */
		chip::Optional<bool> IsGroup;
		bool CaseSessionRecovered{ false };
	};

	/**
	 * @brief Initialaize internal actions
	 */
	static void Init();
	/**
	 * @brief Print out to the log, binding table components and it's size
	 */
	static void PrintBindingTable();

	/**
	 * @brief Runs binding function with proper binding data, depending on the give bindingData param.
	 *
	 * @param bindingData BindingData structure which contain binding infroamtion
	 */
	static void RunBoundClusterAction(BindingData *bindingData);
	/**
	 * @brief Method which should be called after proper binding command execution
	 *
	 * @param bindingData BindingData structure which contain binding infroamtion
	 */
	static void OnInvokeCommandSucces(BindingData *bindingData);
	/**
	 * @brief Method which should be called after failed binding command execution
	 *
	 * @param bindingData BindingData structure which contain binding infroamtion
	 * @param error CHIP_ERROR returned by the failed chip binding action
	 */
	static void OnInvokeCommandFailure(BindingData *bindingData, CHIP_ERROR error);

private:
	static void DeviceWorkerHandler(intptr_t context);
	static void DeviceChangedCallback(const EmberBindingTableEntry &binding,
					  chip::OperationalDeviceProxy *deviceProxy, void *context);
	static void DeviceContextReleaseHandler(void *context);
	static void InitInternal();
};

} /* namespace Nrf::Matter */
