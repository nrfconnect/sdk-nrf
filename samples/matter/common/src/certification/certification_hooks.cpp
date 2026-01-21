/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "certification_hooks.h"
#include "app/task_executor.h"
#include "clusters/identify.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/DefaultTimerDelegate.h>
#include <app/server/Server.h>
#include <platform/CHIPDeviceLayer.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

#define APPLICATION_BUTTON_MASK DK_BTN2_MSK
#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
#define UAT_BUTTON_MASK DK_BTN3_MSK
#endif

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
class IdentifyDelegateImplCertification : public chip::app::Clusters::IdentifyDelegate {
public:
	void OnIdentifyStart(chip::app::Clusters::IdentifyCluster &cluster) override
	{
		LOG_INF("Identify started event received");
	}

	void OnIdentifyStop(chip::app::Clusters::IdentifyCluster &cluster) override
	{
		LOG_INF("Identify stopped event received");
	}

	void OnTriggerEffect(chip::app::Clusters::IdentifyCluster &cluster) override
	{
		LOG_INF("Trigger identify effect event received");
	}

	bool IsTriggerEffectEnabled() const override { return true; }
};

constexpr EndpointId kLightEndpointId = 1;
IdentifyDelegateImplCertification sIdentifyDelegateImplCertification;
DefaultTimerDelegate sTimerDelegate;

Nrf::Matter::IdentifyCluster sIdentifyCluster(kLightEndpointId, sIdentifyDelegateImplCertification, sTimerDelegate);

} // namespace

namespace Nrf::Matter
{
namespace Certification
{

	void Init()
	{
		sIdentifyCluster.Init();
	}

	void ButtonHandler(ButtonState state, ButtonMask hasChanged)
	{
#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
		if ((UAT_BUTTON_MASK & state & hasChanged)) {
			LOG_INF("ICD UserActiveMode has been triggered.");
			Server::GetInstance().GetICDManager().OnNetworkActivity();
		}
#endif

		if ((APPLICATION_BUTTON_MASK & hasChanged) & state) {
			SystemLayer().ScheduleLambda([] {
				bool onOff;
				Protocols::InteractionModel::Status status =
					app::Clusters::OnOff::Attributes::OnOff::Get(kLightEndpointId, &onOff);

				if (status != Protocols::InteractionModel::Status::Success) {
					LOG_ERR("Getting on/off cluster value failed: %x", to_underlying(status));
					return;
				}

				/* Set the OnOff value to the opposite compared to the current one. */
				status = Clusters::OnOff::Attributes::OnOff::Set(kLightEndpointId, !onOff);

				if (status != Protocols::InteractionModel::Status::Success) {
					LOG_ERR("Updating on/off cluster failed: %x", to_underlying(status));
					return;
				}
			});
		}
	}
} /* namespace Certification */
} /* namespace Nrf::Matter */
