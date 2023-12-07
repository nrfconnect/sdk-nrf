/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "simulated_onoff_light_switch_data_provider.h"
#include "binding_handler.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

void SimulatedOnOffLightSwitchDataProvider::Init() {}

void SimulatedOnOffLightSwitchDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
							      void *data, size_t dataSize)
{
}

void ProcessCommand(CommandId aCommandId, const EmberBindingTableEntry &aBinding, OperationalDeviceProxy *aDevice,
		    void *aContext)
{
	CHIP_ERROR ret = CHIP_NO_ERROR;
	BindingHandler::BindingData *data = reinterpret_cast<BindingHandler::BindingData *>(aContext);

	auto onSuccess = [dataRef = data](const ConcreteCommandPath &commandPath, const StatusIB &status,
					  const auto &dataResponse) {
		LOG_DBG("Binding command applied successfully!");

		/* If session was recovered and communication works, reset flag to the initial state. */
		if (dataRef->CaseSessionRecovered) {
			dataRef->CaseSessionRecovered = false;
		}
	};

	auto onFailure = [dataRef = *data](CHIP_ERROR aError) mutable {
		BindingHandler::OnInvokeCommandFailure(dataRef, aError);
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
