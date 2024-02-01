/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "simulated_onoff_light_switch_data_provider.h"
#include "binding/binding_handler.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace Nrf;

void SimulatedOnOffLightSwitchDataProvider::Init() {}

void SimulatedOnOffLightSwitchDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
							      void *data, size_t dataSize)
{
}

void ProcessCommand(const EmberBindingTableEntry &aBinding, OperationalDeviceProxy *aDevice, Nrf::Matter::BindingHandler::BindingData &aData)
{
	CHIP_ERROR ret = CHIP_NO_ERROR;

	auto onSuccess = [dataRef = Platform::New<Nrf::Matter::BindingHandler::BindingData>(aData)](
				 const ConcreteCommandPath &commandPath, const StatusIB &status,
				 const auto &dataResponse) { Matter::BindingHandler::OnInvokeCommandSucces(dataRef); };

	auto onFailure =
		[dataRef = Platform::New<Nrf::Matter::BindingHandler::BindingData>(aData)](CHIP_ERROR aError) mutable {
			Matter::BindingHandler::OnInvokeCommandFailure(dataRef, aError);
		};

	if (aDevice) {
		/* We are validating connection is ready once here instead of multiple times in each case statement
		 * below. */
		VerifyOrDie(aDevice->ConnectionReady());
	}

	Clusters::OnOff::Commands::Toggle::Type toggleCommand;
	if (aDevice) {
		ret = Controller::InvokeCommandRequest(aDevice->GetExchangeManager(),
						       aDevice->GetSecureSession().Value(), aBinding.remote,
						       toggleCommand, onSuccess, onFailure);
	} else {
		Messaging::ExchangeManager &exchangeMgr = Server::GetInstance().GetExchangeManager();
		ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, aBinding.fabricIndex, aBinding.groupId,
							    toggleCommand);
	}

	if (CHIP_NO_ERROR != ret) {
		LOG_ERR("Invoke command request ERROR: %s", ErrorStr(ret));
	}
}

CHIP_ERROR SimulatedOnOffLightSwitchDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
							      uint8_t *buffer)
{
	if (clusterId != Clusters::OnOff::Id || attributeId != Clusters::OnOff::Attributes::OnOff::Id) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	bool mOnOff;
	memcpy(&mOnOff, buffer, sizeof(mOnOff));

	chip::CommandId commandId = mOnOff ? Clusters::OnOff::Commands::On::Id : Clusters::OnOff::Commands::Off::Id;

	if (mInvokeCommandCallback) {
		mInvokeCommandCallback(*this, Clusters::OnOff::Id, commandId, ProcessCommand);
	}

	return CHIP_NO_ERROR;
}
