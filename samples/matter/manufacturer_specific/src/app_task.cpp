/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "lib/core/CHIPError.h"
#include "lib/support/CodeUtils.h"

#include <app-common/zap-generated/attributes/Accessors.h>

#include <setup_payload/OnboardingCodesUtil.h>

#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
#define BUTTON2_MASK DK_BTN2_MSK
#define BUTTON3_MASK DK_BTN3_MSK
constexpr EndpointId kEndpointId = 1;
} /* namespace */

static void ButtonEventHandler(Nrf::ButtonState button_state, Nrf::ButtonMask has_changed)
{
        if(!ConnectivityMgrImpl().IsIPv6NetworkProvisioned() || !ConnectivityMgrImpl().IsIPv6NetworkEnabled()) {
                return;
        }

        /* Handle button press */
	if (BUTTON2_MASK & has_changed && BUTTON2_MASK & button_state) {
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Invert();
	} else if (BUTTON3_MASK & has_changed && BUTTON3_MASK & button_state) {
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Invert();
	} else {
		return;
	}

	AppTask::Instance().UpdateClusterState();
}

void AppTask::UpdateClusterState()
{
	SystemLayer().ScheduleLambda([] {
		Protocols::InteractionModel::Status status;

		status = Clusters::NordicDevKitCluster::Attributes::Led2::Set(kEndpointId,
								Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).GetState());

		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating NordicDevkit cluster failed: %x", to_underlying(status));
		}

		status = Clusters::NordicDevKitCluster::Attributes::Led3::Set(kEndpointId,
								Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).GetState());

		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating NordicDevkit cluster failed: %x", to_underlying(status));
		}
	});
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
