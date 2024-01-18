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
#include <app/matter_event_handler.h>
#include <controller/InvokeInteraction.h>
#include <lib/core/DataModelTypes.h>
#include <platform/CHIPDeviceEvent.h>
#include <platform/CHIPDeviceLayer.h>

namespace Nrf::Matter
{
class BindingHandler {
public:
	struct BindingData;
	using InvokeCommand = void (*)(const EmberBindingTableEntry &, chip::OperationalDeviceProxy *, BindingData &);
	using ReleaseCallback = void (*)();
	struct BindingData {
		chip::EndpointId EndpointId;
		chip::CommandId CommandId;
		chip::ClusterId ClusterId;
		InvokeCommand InvokeCommandFunc;
		uint8_t Value;
		bool IsGroup{ false };
		bool CaseSessionRecovered{ false };
	};

	static BindingHandler &Instance()
	{
		static BindingHandler sBindingHandler;
		return sBindingHandler;
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
	 * @brief Gives information about group binding
	 *
	 * @return True when group is bound
	 */
	static bool IsGroupBound();
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
	/**
	 * @brief Register Binding Matter Event Handler with release callback, which allow to implement release binding
	 * function
	 *
	 * @param bindingReleaseCallback Callback called after binding release
	 * @return CHIP_ERROR returned by the failed register handler action
	 */
	CHIP_ERROR RegisterBindingEventHandler(ReleaseCallback bindingReleaseCallback);
	static void BindingMatterEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t /* unused */);

	ReleaseCallback mReleaseCallbackPointer;

private:
	static void DeviceWorkerHandler(intptr_t context);
	static void DeviceChangedCallback(const EmberBindingTableEntry &binding,
					  chip::OperationalDeviceProxy *deviceProxy, void *context);
	static void DeviceContextReleaseHandler(void *context);
	static void InitInternal();
	static void UnbindDevice(chip::FabricIndex fabricIndex);
};

} /* namespace Nrf::Matter */
