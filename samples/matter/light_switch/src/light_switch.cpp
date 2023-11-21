/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "light_switch.h"
#include "binding_handler.h"

#ifdef CONFIG_CHIP_LIB_SHELL
#include "shell_commands.h"
#endif

#include <app/server/Server.h>
#include <app/util/binding-table.h>
#include <controller/InvokeInteraction.h>
#include <platform/CHIPDeviceLayer.h>
#include <zephyr/logging/log.h>

using namespace chip;
using namespace chip::app;

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

void LightSwitch::Init(chip::EndpointId lightSwitchEndpoint)
{
	BindingHandler::Init();
#ifdef CONFIG_CHIP_LIB_SHELL
	SwitchCommands::RegisterSwitchCommands();
#endif
	mLightSwitchEndpoint = lightSwitchEndpoint;
}

void LightSwitch::InitiateActionSwitch(Action action)
{
	BindingHandler::BindingData *data = Platform::New<BindingHandler::BindingData>();
	if (data) {
		data->EndpointId = mLightSwitchEndpoint;
		data->ClusterId = Clusters::OnOff::Id;
		data->InvokeCommandFunc = SwitchChangedHandler;
		switch (action) {
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
		data->IsGroup = BindingHandler::IsGroupBound();
		BindingHandler::RunBoundClusterAction(data);
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
		data->InvokeCommandFunc = SwitchChangedHandler;
		/* add to brightness 3 to approximate 1% step of brightness after each call dimmer change. */
		sBrightness += kOnePercentBrightnessApproximation;
		if (sBrightness > kMaximumBrightness) {
			sBrightness = 0;
		}
		data->Value = (uint8_t)sBrightness;
		data->IsGroup = BindingHandler::IsGroupBound();
		BindingHandler::RunBoundClusterAction(data);
	}
}

void LightSwitch::SwitchChangedHandler(const EmberBindingTableEntry &binding, OperationalDeviceProxy *deviceProxy,
				       BindingHandler::BindingData &bindingData)
{
	if (binding.type == EMBER_MULTICAST_BINDING && bindingData.IsGroup) {
		switch (bindingData.ClusterId) {
		case Clusters::OnOff::Id:
			OnOffProcessCommand(bindingData.CommandId, binding, nullptr, bindingData);
			break;
		case Clusters::LevelControl::Id:
			LevelControlProcessCommand(bindingData.CommandId, binding, nullptr, bindingData);
			break;
		default:
			ChipLogError(NotSpecified, "Invalid binding group command data");
			break;
		}
	} else if (binding.type == EMBER_UNICAST_BINDING && !bindingData.IsGroup) {
		switch (bindingData.ClusterId) {
		case Clusters::OnOff::Id:
			OnOffProcessCommand(bindingData.CommandId, binding, deviceProxy, bindingData);
			break;
		case Clusters::LevelControl::Id:
			LevelControlProcessCommand(bindingData.CommandId, binding, deviceProxy, bindingData);
			break;
		default:
			ChipLogError(NotSpecified, "Invalid binding unicast command data");
			break;
		}
	}
}

void LightSwitch::OnOffProcessCommand(CommandId commandId, const EmberBindingTableEntry &binding,
				      OperationalDeviceProxy *device, BindingHandler::BindingData &bindingData)
{
	CHIP_ERROR ret = CHIP_NO_ERROR;

	auto onSuccess = [dataPointer = Platform::New<BindingHandler::BindingData>(bindingData)](
				 const ConcreteCommandPath &commandPath, const StatusIB &status,
				 const auto &dataResponse) { BindingHandler::OnInvokeCommandSucces(dataPointer); };

	auto onFailure = [dataPointer =
				  Platform::New<BindingHandler::BindingData>(bindingData)](CHIP_ERROR aError) mutable {
		BindingHandler::OnInvokeCommandFailure(dataPointer, aError);
	};

	if (device) {
		/* We are validating connection is ready once here instead of multiple times in each case
		 * statement below. */
		VerifyOrDie(device->ConnectionReady());
	}

	switch (commandId) {
	case Clusters::OnOff::Commands::Toggle::Id:
		Clusters::OnOff::Commands::Toggle::Type toggleCommand;
		if (device) {
			ret = Controller::InvokeCommandRequest(device->GetExchangeManager(),
							       device->GetSecureSession().Value(), binding.remote,
							       toggleCommand, onSuccess, onFailure);
		} else {
			Messaging::ExchangeManager &exchangeMgr = Server::GetInstance().GetExchangeManager();
			ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId,
								    toggleCommand);
		}
		break;

	case Clusters::OnOff::Commands::On::Id:
		Clusters::OnOff::Commands::On::Type onCommand;
		if (device) {
			ret = Controller::InvokeCommandRequest(device->GetExchangeManager(),
							       device->GetSecureSession().Value(), binding.remote,
							       onCommand, onSuccess, onFailure);
		} else {
			Messaging::ExchangeManager &exchangeMgr = Server::GetInstance().GetExchangeManager();
			ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId,
								    onCommand);
		}
		break;

	case Clusters::OnOff::Commands::Off::Id:
		Clusters::OnOff::Commands::Off::Type offCommand;
		if (device) {
			ret = Controller::InvokeCommandRequest(device->GetExchangeManager(),
							       device->GetSecureSession().Value(), binding.remote,
							       offCommand, onSuccess, onFailure);
		} else {
			Messaging::ExchangeManager &exchangeMgr = Server::GetInstance().GetExchangeManager();
			ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId,
								    offCommand);
		}
		break;
	default:
		LOG_DBG("Invalid binding command data - commandId is not supported");
		break;
	}
	if (CHIP_NO_ERROR != ret) {
		LOG_ERR("Invoke OnOff Command Request ERROR: %s", ErrorStr(ret));
	}
}

void LightSwitch::LevelControlProcessCommand(CommandId commandId, const EmberBindingTableEntry &binding,
					     OperationalDeviceProxy *device, BindingHandler::BindingData &bindingData)
{
	auto onSuccess = [dataPointer = Platform::New<BindingHandler::BindingData>(bindingData)](
				 const ConcreteCommandPath &commandPath, const StatusIB &status,
				 const auto &dataResponse) { BindingHandler::OnInvokeCommandSucces(dataPointer); };

	auto onFailure = [dataPointer =
				  Platform::New<BindingHandler::BindingData>(bindingData)](CHIP_ERROR aError) mutable {
		BindingHandler::OnInvokeCommandFailure(dataPointer, aError);
	};

	CHIP_ERROR ret = CHIP_NO_ERROR;

	if (device) {
		/* We are validating connection is ready once here instead of multiple times in each case
		 * statement below. */
		VerifyOrDie(device->ConnectionReady());
	}

	switch (commandId) {
	case Clusters::LevelControl::Commands::MoveToLevel::Id: {
		Clusters::LevelControl::Commands::MoveToLevel::Type moveToLevelCommand;
		moveToLevelCommand.level = bindingData.Value;
		if (device) {
			ret = Controller::InvokeCommandRequest(device->GetExchangeManager(),
							       device->GetSecureSession().Value(), binding.remote,
							       moveToLevelCommand, onSuccess, onFailure);
		} else {
			Messaging::ExchangeManager &exchangeMgr = Server::GetInstance().GetExchangeManager();
			ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId,
								    moveToLevelCommand);
		}
	} break;
	default:
		LOG_DBG("Invalid binding command data - commandId is not supported");
		break;
	}
	if (CHIP_NO_ERROR != ret) {
		LOG_ERR("Invoke Group Command Request ERROR: %s", ErrorStr(ret));
	}
}
