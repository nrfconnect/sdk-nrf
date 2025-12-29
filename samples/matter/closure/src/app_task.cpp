/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "closure_control_endpoint.h"
#include "closure_manager.h"
#include "clusters/identify.h"

#include "lib/core/CHIPError.h"
#include "lib/support/CodeUtils.h"
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/data-model/Nullable.h>
#include <app/util/attribute-storage.h>
#include <setup_payload/OnboardingCodesUtil.h>

#include <persistent_storage/persistent_storage.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;
using namespace chip::app::Clusters::Descriptor;

namespace
{
const struct pwm_dt_spec sPhysicalIndicatorPwmDevice = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));

constexpr chip::EndpointId kClosureEndpoint = 1;

/* TODO: Add a custom identify delegate to handle the custom identify stop behavior according to the closure state */
Nrf::Matter::IdentifyCluster sIdentifyCluster(kClosureEndpoint);
} /* namespace */

AppTask::AppTask() : mPhysicalDevice(&sPhysicalIndicatorPwmDevice), mClosureManager(mPhysicalDevice, kClosureEndpoint)
{
}

CHIP_ERROR AppTask::Init()
{
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

	if (!Nrf::GetBoard().Init()) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

	ReturnErrorOnFailure(Nrf::Matter::StartServer());

	ReturnErrorOnFailure(sIdentifyCluster.Init());

	if (Nrf::GetPersistentStorage().NonSecureInit(&mRootNode) != Nrf::PSErrorCode::Success) {
		return CHIP_ERROR_PERSISTED_STORAGE_FAILED;
	}

	ReturnErrorOnFailure(mClosureManager.Init());
	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
