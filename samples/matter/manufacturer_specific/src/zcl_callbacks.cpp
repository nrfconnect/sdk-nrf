/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <lib/support/logging/CHIPLogging.h>

#include "app_task.h"
#include "board/board.h"

#include <zap-generated/PluginApplicationCallbacks.h>
#include <app-common/zap-generated/callback.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/ConcreteAttributePath.h>

using namespace ::chip;
using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::NordicDevKitCluster;



bool emberAfNordicDevKitClusterClusterSetLEDCallback(chip::app::CommandHandler *commandObj,
						     const chip::app::ConcreteCommandPath &commandPath,
						     const Commands::SetLED::DecodableType &commandData)
{
	bool result = true;

	/* Shift ledID by one, as numeration of Nrf::DeviceLeds starts from 0 */
	Nrf::DeviceLeds ledID = static_cast<Nrf::DeviceLeds>(commandData.ledid - 1);

	VerifyOrExit(ledID == Nrf::DeviceLeds::LED2 || ledID == Nrf::DeviceLeds::LED3, result = false);

	switch (commandData.action) {
		case LEDActionEnum::kOff:
			Nrf::GetBoard().GetLED(ledID).Set(false);
			break;
		case LEDActionEnum::kOn:
			Nrf::GetBoard().GetLED(ledID).Set(true);
			break;
		case LEDActionEnum::kToggle:
			Nrf::GetBoard().GetLED(ledID).Invert();
			break;
		default:
			ExitNow(result = false);
	}

exit:
	if (result) {
		AppTask::Instance().UpdateClusterState();
		commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Success);
	} else {
		commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Failure);
	}
	return true;
}

void MatterNordicDevKitClusterPluginServerInitCallback() {}

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t type,
				       uint16_t size, uint8_t *value) {}

/** @brief NordicDevkit Cluster Init
 *
 * This function is called when a specific cluster is initialized. It gives the
 * application an opportunity to take care of cluster initialization procedures.
 * It is called exactly once for each endpoint where cluster is present.
 *
 * @param endpoint   Ver.: always
 *
 */
void emberAfNordicDevKitClusterClusterInitCallback(EndpointId endpoint)
{
	Protocols::InteractionModel::Status status;
	bool storedValue;

	/* Read storedValue on/off value */
	status = NordicDevKitCluster::Attributes::Led2::Get(endpoint, &storedValue);
	if (status == Protocols::InteractionModel::Status::Success) {
		/* Set actual state to the cluster state that was last persisted */
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(storedValue);
	}
	/* Read storedValue on/off value */
	status = NordicDevKitCluster::Attributes::Led3::Get(endpoint, &storedValue);
	if (status == Protocols::InteractionModel::Status::Success) {
		/* Set actual state to the cluster state that was last persisted */
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(storedValue);
	}
}
