/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef CONFIG_BRIDGE_SMART_PLUG_SUPPORT
#include <lib/support/logging/CHIPLogging.h>

#include "app_task.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>

using namespace ::chip;
using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::OnOff;

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t type,
				       uint16_t size, uint8_t *value)
{
	if (attributePath.mClusterId == OnOff::Id && attributePath.mAttributeId == OnOff::Attributes::OnOff::Id &&
	    attributePath.mEndpointId == AppTask::Instance().kSmartplugEndpointId) {
		ChipLogProgress(Zcl, "Cluster OnOff: attribute OnOff set to %" PRIu8 "", *value);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(*value);
	}
}

void emberAfOnOffClusterInitCallback(EndpointId endpoint)
{
	if (endpoint == AppTask::Instance().kSmartplugEndpointId) {
		Protocols::InteractionModel::Status status;
		bool storedValue;
		/* Read storedValue on/off value */
		status = Attributes::OnOff::Get(endpoint, &storedValue);
		if (status == Protocols::InteractionModel::Status::Success) {
			/* Set actual state to the cluster state that was last persisted */
			Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(storedValue);
		}
	}
}
#endif /* CONFIG_BRIDGE_SMART_PLUG_SUPPORT */
