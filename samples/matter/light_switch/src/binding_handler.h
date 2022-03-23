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

class BindingHandler {
public:
	static void Init();
	static void SwitchWorkerHandler(intptr_t);
	static void PrintBindingTable();
	static bool IsGroupBound();

	struct BindingData {
		chip::EndpointId EndpointId;
		chip::CommandId CommandId;
		chip::ClusterId ClusterId;
		uint8_t Value;
		bool IsGroup{ false };
	};

private:
	static void OnOffProcessCommand(chip::CommandId, const EmberBindingTableEntry &, chip::DeviceProxy *, void *);
	static void LevelControlProcessCommand(chip::CommandId, const EmberBindingTableEntry &, chip::DeviceProxy *,
					       void *);
	static void LightSwitchChangedHandler(const EmberBindingTableEntry &, chip::DeviceProxy *, void *);
	static void InitInternal(intptr_t);
};
