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
	switch (commandData.action) {
		case LEDActionEnum::kOff:
			Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);
			break;
		case LEDActionEnum::kOn:
			Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(true);
			break;
		case LEDActionEnum::kToggle:
			Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Invert();
			break;
		default:
			commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Failure);
			return true;
	}

	AppTask::Instance().UpdateClusterState();
	commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Success);

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

	/* Read storedValue UserLED value */
	status = NordicDevKitCluster::Attributes::UserLED::Get(endpoint, &storedValue);
	if (status == Protocols::InteractionModel::Status::Success) {
		/* Set actual state to the cluster state that was last persisted */
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(storedValue);
	}
}
