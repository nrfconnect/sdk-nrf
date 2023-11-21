/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "light_switch.h"
#include "app_event.h"
#include "binding_handler.h"

#ifdef CONFIG_CHIP_LIB_SHELL
#include "shell_commands.h"
#endif

#include <app/server/Server.h>
#include <app/util/binding-table.h>
#include <controller/InvokeInteraction.h>

using namespace chip;
using namespace chip::app;

void LightSwitch::Init(chip::EndpointId aLightSwitchEndpoint)
{
	BindingHandler::GetInstance().Init();
#ifdef CONFIG_CHIP_LIB_SHELL
	SwitchCommands::RegisterSwitchCommands();
#endif
	mLightSwitchEndpoint = aLightSwitchEndpoint;
}

void LightSwitch::InitiateActionSwitch(Action mAction)
{
	BindingHandler::BindingData *data = Platform::New<BindingHandler::BindingData>();
	if (data) {
		data->EndpointId = mLightSwitchEndpoint;
		data->ClusterId = Clusters::OnOff::Id;
		switch (mAction) {
		case Action::Toggle:
			data->CommandId = Clusters::OnOff::Commands::Toggle::Id;
			break;
		case Action::On:
			data->CommandId = Clusters::OnOff::Commands::On::Id;
			break;
		case Action::Off:
			data->CommandId = Clusters::OnOff::Commands::Off::Id;
			break;
		default:
			Platform::Delete(data);
			return;
		}
		data->IsGroup = BindingHandler::GetInstance().IsGroupBound();
		DeviceLayer::PlatformMgr().ScheduleWork(BindingHandler::DeviceWorkerHandler,
							reinterpret_cast<intptr_t>(data));
	}
}

void LightSwitch::DimmerChangeBrightness()
{
	static uint16_t sBrightness;
	BindingHandler::BindingData *data = Platform::New<BindingHandler::BindingData>();
	if (data) {
		data->EndpointId = mLightSwitchEndpoint;
		data->CommandId = Clusters::LevelControl::Commands::MoveToLevel::Id;
		data->ClusterId = Clusters::LevelControl::Id;
		/* add to brightness 3 to approximate 1% step of brightness after each call dimmer change. */
		sBrightness += kOnePercentBrightnessApproximation;
		if (sBrightness > kMaximumBrightness) {
			sBrightness = 0;
		}
		data->Value = (uint8_t)sBrightness;
		data->IsGroup = BindingHandler::GetInstance().IsGroupBound();
		DeviceLayer::PlatformMgr().ScheduleWork(BindingHandler::DeviceWorkerHandler,
							reinterpret_cast<intptr_t>(data));
	}
}
