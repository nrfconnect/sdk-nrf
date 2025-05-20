/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include <app/icd/server/ICDConfigurationData.h>

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>

#ifdef CONFIG_CHIP_ICD_DSLS_SUPPORT
#include <app/icd/server/ICDNotifier.h>
#endif

#include <lib/support/logging/CHIPLogging.h>

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::app::Clusters;

#ifdef CONFIG_CHIP_ICD_DSLS_SUPPORT
void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t type,
				       uint16_t size, uint8_t *value)
{
	ClusterId clusterId = attributePath.mClusterId;
	AttributeId attributeId = attributePath.mAttributeId;
	EndpointId endpointId = attributePath.mEndpointId;

	ChipLogProgress(Zcl, "Cluster callback: " ChipLogFormatMEI, ChipLogValueMEI(clusterId));

	if (clusterId == PowerSource::Id && attributeId == PowerSource::Attributes::Status::Id) {
		PowerSource::PowerSourceStatusEnum state =
			*(reinterpret_cast<PowerSource::PowerSourceStatusEnum *>(value));

		/* We do not care about the battery source changes, as it's wired source that dictates the ordering. */
		if (endpointId != AppTask::Instance().kWiredPowerSourceEndpointId) {
			return;
		}

		if (state == PowerSource::PowerSourceStatusEnum::kActive) {
			/* Wired source is active, we can switch into the SIT mode. */
			chip::DeviceLayer::PlatformMgr().ScheduleWork(
				[](intptr_t arg) {
					chip::app::ICDNotifier::GetInstance().NotifySITModeRequestNotification();
				},
				0);
			ChipLogProgress(Zcl, "Wired power source was activated, moving into the ICD SIT mode.");
		} else {
			/* Wired source is inactive, so we need to change mode into LIT to save power. */
			chip::DeviceLayer::PlatformMgr().ScheduleWork(
				[](intptr_t arg) {
					chip::app::ICDNotifier::GetInstance().NotifySITModeRequestWithdrawal();
				},
				0);
			ChipLogProgress(Zcl, "Wired power source was deactivated, moving into the ICD LIT mode.");
		}
	}
}

void emberAfPowerSourceClusterInitCallback(EndpointId endpoint)
{
	Clusters::PowerSource::PowerSourceStatusEnum wiredPowerSourceState;
	Clusters::PowerSource::Attributes::Status::Get(AppTask::Instance().kWiredPowerSourceEndpointId, &wiredPowerSourceState);

	if (wiredPowerSourceState == Clusters::PowerSource::PowerSourceStatusEnum::kActive) {
		/* Request switching to SIT, as soon as it's possible. */
		chip::DeviceLayer::PlatformMgr().ScheduleWork(
			[](intptr_t arg) { chip::app::ICDNotifier::GetInstance().NotifySITModeRequestNotification(); },
			0);
	}
}
#endif /* CONFIG_CHIP_ICD_DSLS_SUPPORT */

void emberAfPluginSmokeCoAlarmSelfTestRequestCommand(EndpointId endpointId)
{
	if (endpointId == AppTask::Instance().kSmokeCoAlarmEndpointId) {
		AppTask::Instance().SelfTestHandler();
	}
}
