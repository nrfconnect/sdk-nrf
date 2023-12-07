/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#pragma once

#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/ids/Commands.h>
#include <app/CommandSender.h>
#include <app/clusters/bindings/BindingManager.h>
#include <controller/InvokeInteraction.h>
#include <platform/CHIPDeviceLayer.h>

#include <functional>

class BindingHandler {
public:
	using InvokeCommand = void (*)(chip::CommandId aCommandId, const EmberBindingTableEntry &aBinding,
						 chip::OperationalDeviceProxy *aDevice, void *aContext);

	struct BindingData {
		chip::EndpointId EndpointId;
		chip::CommandId CommandId;
		chip::ClusterId ClusterId;
		InvokeCommand InvokeCommandFunc;
		uint8_t Value;
		bool IsGroup{ false };
		bool CaseSessionRecovered{ false };
	};

	void Init();
	void PrintBindingTable();
	bool IsGroupBound();

	static void DeviceWorkerHandler(intptr_t);
	static void OnInvokeCommandFailure(BindingData &aBindingData, CHIP_ERROR aError);

	static BindingHandler &GetInstance()
	{
		static BindingHandler sBindingHandler;
		return sBindingHandler;
	}

private:
	static void DeviceChangedHandler(const EmberBindingTableEntry &, chip::OperationalDeviceProxy *, void *);
	static void DeviceContextReleaseHandler(void *context);
	static void InitInternal(intptr_t);
};
