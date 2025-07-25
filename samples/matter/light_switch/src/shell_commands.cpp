/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "shell_commands.h"
#include "binding/binding_handler.h"
#include "light_switch.h"

#include <platform/CHIPDeviceLayer.h>

#ifdef CONFIG_CHIP_LIB_SHELL

using namespace chip;
using namespace chip::app;

namespace SwitchCommands
{
using Shell::Engine;
using Shell::shell_command_t;
using Shell::streamer_get;
using Shell::streamer_printf;

Engine sShellSwitchSubCommands;
Engine sShellSwitchOnOffSubCommands;
Engine sShellSwitchGroupsSubCommands;
Engine sShellSwitchGroupsOnOffSubCommands;

static CHIP_ERROR SwitchHelpHandler(int argc, char **argv)
{
	sShellSwitchSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
	return CHIP_NO_ERROR;
}

static CHIP_ERROR SwitchCommandHandler(int argc, char **argv)
{
	if (argc == 0) {
		return SwitchHelpHandler(argc, argv);
	}
	return sShellSwitchSubCommands.ExecCommand(argc, argv);
}

static CHIP_ERROR TableCommandHelper(int argc, char **argv)
{
	Nrf::Matter::BindingHandler::PrintBindingTable();
	return CHIP_NO_ERROR;
}

namespace Unicast
{
	static CHIP_ERROR OnOffHelpHandler(int argc, char **argv)
	{
		sShellSwitchOnOffSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
		return CHIP_NO_ERROR;
	}

	static CHIP_ERROR OnOffCommandHandler(int argc, char **argv)
	{
		if (argc == 0) {
			return OnOffHelpHandler(argc, argv);
		}
		return sShellSwitchOnOffSubCommands.ExecCommand(argc, argv);
	}

	static CHIP_ERROR OnCommandHandler(int argc, char **argv)
	{
		Nrf::Matter::BindingHandler::BindingData *data =
			Platform::New<Nrf::Matter::BindingHandler::BindingData>();
		if (data) {
			data->EndpointId = LightSwitch::GetInstance().GetLightSwitchEndpointId();
			data->CommandId = Clusters::OnOff::Commands::On::Id;
			data->ClusterId = Clusters::OnOff::Id;
			data->InvokeCommandFunc = LightSwitch::SwitchChangedHandler;
			data->IsGroup.SetValue(false);

			Nrf::Matter::BindingHandler::RunBoundClusterAction(data);
			return CHIP_NO_ERROR;
		}
		return CHIP_ERROR_NO_MEMORY;
	}

	static CHIP_ERROR OffCommandHandler(int argc, char **argv)
	{
		Nrf::Matter::BindingHandler::BindingData *data =
			Platform::New<Nrf::Matter::BindingHandler::BindingData>();
		if (data) {
			data->EndpointId = LightSwitch::GetInstance().GetLightSwitchEndpointId();
			data->CommandId = Clusters::OnOff::Commands::Off::Id;
			data->ClusterId = Clusters::OnOff::Id;
			data->InvokeCommandFunc = LightSwitch::SwitchChangedHandler;
			data->IsGroup.SetValue(false);

			Nrf::Matter::BindingHandler::RunBoundClusterAction(data);
			return CHIP_NO_ERROR;
		}
		return CHIP_ERROR_NO_MEMORY;
	}

	static CHIP_ERROR ToggleCommandHandler(int argc, char **argv)
	{
		Nrf::Matter::BindingHandler::BindingData *data =
			Platform::New<Nrf::Matter::BindingHandler::BindingData>();
		if (data) {
			data->EndpointId = LightSwitch::GetInstance().GetLightSwitchEndpointId();
			data->CommandId = Clusters::OnOff::Commands::Toggle::Id;
			data->ClusterId = Clusters::OnOff::Id;
			data->InvokeCommandFunc = LightSwitch::SwitchChangedHandler;
			data->IsGroup.SetValue(false);

			Nrf::Matter::BindingHandler::RunBoundClusterAction(data);
			return CHIP_NO_ERROR;
		}
		return CHIP_ERROR_NO_MEMORY;
	}
} /* namespace Unicast */

namespace Group
{
	CHIP_ERROR SwitchHelpHandler(int argc, char **argv)
	{
		sShellSwitchGroupsSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
		return CHIP_NO_ERROR;
	}

	CHIP_ERROR SwitchCommandHandler(int argc, char **argv)
	{
		if (argc == 0) {
			return SwitchHelpHandler(argc, argv);
		}

		return sShellSwitchGroupsSubCommands.ExecCommand(argc, argv);
	}

	static CHIP_ERROR OnOffHelpHandler(int argc, char **argv)
	{
		sShellSwitchGroupsOnOffSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
		return CHIP_NO_ERROR;
	}

	static CHIP_ERROR OnOffCommandHandler(int argc, char **argv)
	{
		if (argc == 0) {
			return OnOffHelpHandler(argc, argv);
		}

		return sShellSwitchGroupsOnOffSubCommands.ExecCommand(argc, argv);
	}

	CHIP_ERROR OnCommandHandler(int argc, char **argv)
	{
		Nrf::Matter::BindingHandler::BindingData *data =
			Platform::New<Nrf::Matter::BindingHandler::BindingData>();
		if (data) {
			data->EndpointId = LightSwitch::GetInstance().GetLightSwitchEndpointId();
			data->CommandId = Clusters::OnOff::Commands::On::Id;
			data->ClusterId = Clusters::OnOff::Id;
			data->InvokeCommandFunc = LightSwitch::SwitchChangedHandler;
			data->IsGroup.SetValue(true);

			Nrf::Matter::BindingHandler::RunBoundClusterAction(data);
			return CHIP_NO_ERROR;
		}
		return CHIP_ERROR_NO_MEMORY;
	}

	CHIP_ERROR OffCommandHandler(int argc, char **argv)
	{
		Nrf::Matter::BindingHandler::BindingData *data =
			Platform::New<Nrf::Matter::BindingHandler::BindingData>();
		if (data) {
			data->EndpointId = LightSwitch::GetInstance().GetLightSwitchEndpointId();
			data->CommandId = Clusters::OnOff::Commands::Off::Id;
			data->ClusterId = Clusters::OnOff::Id;
			data->InvokeCommandFunc = LightSwitch::SwitchChangedHandler;
			data->IsGroup.SetValue(true);

			Nrf::Matter::BindingHandler::RunBoundClusterAction(data);
			return CHIP_NO_ERROR;
		}
		return CHIP_ERROR_NO_MEMORY;
	}

	CHIP_ERROR ToggleCommandHandler(int argc, char **argv)
	{
		Nrf::Matter::BindingHandler::BindingData *data =
			Platform::New<Nrf::Matter::BindingHandler::BindingData>();
		if (data) {
			data->EndpointId = LightSwitch::GetInstance().GetLightSwitchEndpointId();
			data->CommandId = Clusters::OnOff::Commands::Toggle::Id;
			data->ClusterId = Clusters::OnOff::Id;
			data->InvokeCommandFunc = LightSwitch::SwitchChangedHandler;
			data->IsGroup.SetValue(true);

			Nrf::Matter::BindingHandler::RunBoundClusterAction(data);
			return CHIP_NO_ERROR;
		}
		return CHIP_ERROR_NO_MEMORY;
	}

} /* namespace Group */

void RegisterSwitchCommands()
{
	static const shell_command_t sSwitchSubCommands[] = {
		{ &SwitchHelpHandler, "help", "Switch commands" },
		{ &Unicast::OnOffCommandHandler, "onoff", "Usage: switch onoff [on|off|toggle]" },
		{ &Group::SwitchCommandHandler, "groups", "Usage: switch groups onoff [on|off|toggle]" },
		{ &TableCommandHelper, "table", "Print a binding table" }
	};

	static const shell_command_t sSwitchOnOffSubCommands[] = {
		{ &Unicast::OnOffHelpHandler, "help", "Usage : switch ononff [on|off|toggle]" },
		{ &Unicast::OnCommandHandler, "on", "Sends on command to bound lighting app" },
		{ &Unicast::OffCommandHandler, "off", "Sends off command to bound lighting app" },
		{ &Unicast::ToggleCommandHandler, "toggle", "Sends toggle command to bound lighting app" }
	};

	static const shell_command_t sSwitchGroupsSubCommands[] = {
		{ &Group::SwitchHelpHandler, "help", "switch a group of bounded lightning apps" },
		{ &Group::OnOffCommandHandler, "onoff", "Usage: switch groups onoff [on|off|toggle]" }
	};

	static const shell_command_t sSwichGroupsOnOffSubCommands[] = {
		{ &Group::OnOffHelpHandler, "help", "Usage: switch groups onoff [on|off|toggle]" },
		{ &Group::OnCommandHandler, "on", "Sends on command to bound Group" },
		{ &Group::OffCommandHandler, "off", "Sends off command to bound Group" },
		{ &Group::ToggleCommandHandler, "toggle", "Sends toggle command to bound Group" }
	};

	static const shell_command_t sSwitchCommand = { &SwitchCommandHandler, "switch",
							"Light-switch commands. Usage: switch [onoff|groups]" };

	sShellSwitchGroupsOnOffSubCommands.RegisterCommands(sSwichGroupsOnOffSubCommands,
							    MATTER_ARRAY_SIZE(sSwichGroupsOnOffSubCommands));
	sShellSwitchOnOffSubCommands.RegisterCommands(sSwitchOnOffSubCommands,
						      MATTER_ARRAY_SIZE(sSwitchOnOffSubCommands));
	sShellSwitchGroupsSubCommands.RegisterCommands(sSwitchGroupsSubCommands,
						       MATTER_ARRAY_SIZE(sSwitchGroupsSubCommands));
	sShellSwitchSubCommands.RegisterCommands(sSwitchSubCommands, MATTER_ARRAY_SIZE(sSwitchSubCommands));

	Engine::Root().RegisterCommands(&sSwitchCommand, 1);
}

} /* namespace SwitchCommands */
#endif
