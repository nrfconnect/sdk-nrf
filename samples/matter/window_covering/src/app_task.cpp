/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "window_covering.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "clusters/identify.h"

#include <setup_payload/OnboardingCodesUtil.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
Nrf::Matter::IdentifyCluster sIdentifyCluster(WindowCovering::Endpoint());

#define OPEN_BUTTON_MASK DK_BTN2_MSK
#define CLOSE_BUTTON_MASK DK_BTN3_MSK
} /* namespace */

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
	if (OPEN_BUTTON_MASK & hasChanged) {
		WindowButtonAction action =
			(OPEN_BUTTON_MASK & state) ? WindowButtonAction::Pressed : WindowButtonAction::Released;
		Nrf::PostTask([action] { OpenHandler(action); });
	}

	if (CLOSE_BUTTON_MASK & hasChanged) {
		WindowButtonAction action =
			(CLOSE_BUTTON_MASK & state) ? WindowButtonAction::Pressed : WindowButtonAction::Released;
		Nrf::PostTask([action] { CloseHandler(action); });
	}
}

void AppTask::OpenHandler(const WindowButtonAction &action)
{
	if (action == WindowButtonAction::Pressed) {
		Instance().mOpenButtonIsPressed = true;
		if (Instance().mCloseButtonIsPressed) {
			Instance().ToggleMoveType();
		}
	} else {
		if (!Instance().mCloseButtonIsPressed) {
			if (!Instance().mMoveTypeRecentlyChanged) {
				WindowCovering::Instance().SetSingleStepTarget(OperationalState::MovingUpOrOpen);
			} else {
				Instance().mMoveTypeRecentlyChanged = false;
			}
		}
		Instance().mOpenButtonIsPressed = false;
	}
}

void AppTask::CloseHandler(const WindowButtonAction &action)
{
	if (action == WindowButtonAction::Pressed) {
		Instance().mCloseButtonIsPressed = true;
		if (Instance().mOpenButtonIsPressed) {
			Instance().ToggleMoveType();
		}
	} else {
		if (!Instance().mOpenButtonIsPressed) {
			if (!Instance().mMoveTypeRecentlyChanged) {
				WindowCovering::Instance().SetSingleStepTarget(OperationalState::MovingDownOrClose);
			} else {
				Instance().mMoveTypeRecentlyChanged = false;
			}
		}
		Instance().mCloseButtonIsPressed = false;
	}
}

void AppTask::ToggleMoveType()
{
	if (WindowCovering::Instance().GetMoveType() == WindowCovering::MoveType::LIFT) {
		WindowCovering::Instance().SetMoveType(WindowCovering::MoveType::TILT);
		LOG_INF("Window covering move: tilt");
	} else {
		WindowCovering::Instance().SetMoveType(WindowCovering::MoveType::LIFT);
		LOG_INF("Window covering move: lift");
	}
	mMoveTypeRecentlyChanged = true;
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer(Nrf::Matter::InitData{ .mPostServerInitClbk = [] {
		WindowCovering::Instance().PositionLEDUpdate(WindowCovering::MoveType::LIFT);
		WindowCovering::Instance().PositionLEDUpdate(WindowCovering::MoveType::TILT);
		return CHIP_NO_ERROR;
	} }));

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

	ReturnErrorOnFailure(sIdentifyCluster.Init());

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
