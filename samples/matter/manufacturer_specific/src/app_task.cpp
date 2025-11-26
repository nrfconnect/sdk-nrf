/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "basic_information_extension.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "lib/core/CHIPError.h"
#include "lib/support/CodeUtils.h"

#include <app-common/zap-generated/attributes/Accessors.h>

#include <app/EventLogging.h>
#include <app/util/attribute-storage.h>
#include <setup_payload/OnboardingCodesUtil.h>

#include <data-model-providers/codegen/CodegenDataModelProvider.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
#define BUTTON2_MASK DK_BTN2_MSK
constexpr EndpointId kBasicInformationEndpointId = 0;
constexpr EndpointId kNordicDevKitEndpointId = 1;

RegisteredServerCluster<BasicInformationExtension> sBasicInformationExtension;
} /* namespace */

static void ButtonEventHandler(Nrf::ButtonState /* unused */, Nrf::ButtonMask has_changed)
{
	/* Handle button press */
	if (ConnectivityMgrImpl().IsIPv6NetworkProvisioned() && ConnectivityMgrImpl().IsIPv6NetworkEnabled() &&
	    BUTTON2_MASK & has_changed) {
		AppTask::Instance().UpdateNordicDevkitClusterState();
	}
}

void AppTask::UpdateNordicDevkitClusterState()
{
	SystemLayer().ScheduleLambda([] {
		Protocols::InteractionModel::Status status;
		Nrf::ButtonState button_state;

		dk_read_buttons(&button_state, nullptr);

		status = Clusters::NordicDevKit::Attributes::UserLED::Set(
			kNordicDevKitEndpointId, Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).GetState());

		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating NordicDevkit cluster failed: %x", to_underlying(status));
		}

		status = Clusters::NordicDevKit::Attributes::UserButton::Set(kNordicDevKitEndpointId,
									     BUTTON2_MASK & button_state);

		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating NordicDevkit cluster failed: %x", to_underlying(status));
		}

		for (auto endpoint : EnabledEndpointsWithServerCluster(Clusters::NordicDevKit::Id)) {
			/* If NordicDevKit cluster is implemented on this endpoint */
			Clusters::NordicDevKit::Events::UserButtonChanged::Type event;
			EventNumber eventNumber;

			if (CHIP_NO_ERROR != LogEvent(event, endpoint, eventNumber)) {
				ChipLogError(Zcl, "Failed to emit UserButtonChanged event");
			}
		}
	});
}

void AppTask::UpdateBasicInformationClusterState()
{
	SystemLayer().ScheduleLambda([] {
		DataModel::ActionReturnStatus status =
			sBasicInformationExtension.Cluster().SetRandomNumber(sys_rand16_get());

		if (status != CHIP_NO_ERROR) {
			ChipLogError(Zcl, "Updating Basic information cluster failed");
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

	/* Replaces the registered BasicInformation cluster with a customized one that adds random number handling. */
	auto &registry = chip::app::CodegenDataModelProvider::Instance().Registry();

	ServerClusterInterface *interface =
		registry.Get({ kRootEndpointId, chip::app::Clusters::BasicInformation::Id });

	VerifyOrDie(interface != nullptr);

	registry.Unregister(interface);
	VerifyOrDie(registry.Register(sBasicInformationExtension.Registration()) == CHIP_NO_ERROR);

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
