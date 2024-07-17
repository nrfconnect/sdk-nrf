/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "test_shell.h"

#include <app/server/Server.h>
#include <platform/CHIPDeviceLayer.h>

using namespace chip;
using namespace chip::app;

namespace Nrf
{
using Shell::Engine;
using Shell::shell_command_t;
using Shell::streamer_get;
using Shell::streamer_printf;

Engine sShellTestSubCommands;
Engine sShellResumptionStorageSubCommands;

static CHIP_ERROR TestHelpHandler(int argc, char **argv)
{
	sShellTestSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
	return CHIP_NO_ERROR;
}

static CHIP_ERROR TestCommandHandler(int argc, char **argv)
{
	if (argc == 0) {
		return TestHelpHandler(argc, argv);
	}
	return sShellTestSubCommands.ExecCommand(argc, argv);
}


static CHIP_ERROR ResumptionStorageHelpHandler(int argc, char **argv)
{
	sShellResumptionStorageSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
	return CHIP_NO_ERROR;
}

static CHIP_ERROR ResumptionStorageHandler(int argc, char **argv)
{
	if (argc == 0) {
		return ResumptionStorageHelpHandler(argc, argv);
	}
	return sShellResumptionStorageSubCommands.ExecCommand(argc, argv);
}

static CHIP_ERROR ClearResumptionStorageHandler(int argc, char ** argv)
{
	CHIP_ERROR error = CHIP_NO_ERROR;
	FabricId fabricId = kUndefinedFabricId;
	SessionResumptionStorage * storage = Server::GetInstance().GetSessionResumptionStorage();

	VerifyOrExit(storage != nullptr, error = CHIP_ERROR_INCORRECT_STATE);
	VerifyOrExit(argc < 2, error = CHIP_ERROR_INVALID_ARGUMENT);

	if (argc == 1)
	{
		char *endptr;

		fabricId = static_cast<FabricId>(strtoull(argv[0], &endptr, 0));
		VerifyOrExit(*endptr == '\0', error = CHIP_ERROR_INVALID_ARGUMENT);
	}

	streamer_printf(streamer_get(), "Clearing resumption storage.\r\n");

	for (auto & fabricInfo : Server::GetInstance().GetFabricTable()) {
		if (argc == 0 || fabricInfo.GetFabricId() == fabricId) {
			SuccessOrExit(error = storage->DeleteAll(fabricInfo.GetFabricIndex()));

			if (fabricInfo.GetFabricId() == fabricId) {
				ExitNow();
			}
		}
	}

exit:
	return error;
}

void RegisterTestCommands()
{
	static const shell_command_t sTestSubCommands[] = {
		{ &TestHelpHandler, "help", "Test - specific commands" },
		{ &ResumptionStorageHandler, "restorage", "Resumption storage commands." },
	};

	static const shell_command_t sResumptionStorageSubCommands[] = {
		{ &ResumptionStorageHelpHandler, "help", "Resumption storage commands. "
							 "Usage : test restorage <subcommand>" },
		{ &ClearResumptionStorageHandler, "clear", "Clears resumption storage for fabric-id. If fabric-id is "
							   "not specified, clears resumption storage for all fabrics. "
							   "Usage : test restorage clear [fabric-id]" },
	};

	static const shell_command_t sTestCommand = { &TestCommandHandler, "test", "Test - specific commands" };

	sShellTestSubCommands.RegisterCommands(sTestSubCommands, ArraySize(sTestSubCommands));
	sShellResumptionStorageSubCommands.RegisterCommands(sResumptionStorageSubCommands,
							    ArraySize(sResumptionStorageSubCommands));

	Engine::Root().RegisterCommands(&sTestCommand, 1);
}

} /* namespace Nrf */
