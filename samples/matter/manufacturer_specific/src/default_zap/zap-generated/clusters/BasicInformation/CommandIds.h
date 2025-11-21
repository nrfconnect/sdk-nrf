// DO NOT EDIT MANUALLY - Generated file
//
// Identifier constant values for cluster BasicInformation (cluster code: 40/0x28)
// based on nrf/samples/matter/manufacturer_specific/src/default_zap/manufacturer_specific.matter
#pragma once

#include <lib/core/DataModelTypes.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace BasicInformation
		{
			namespace Commands
			{

				// Total number of client to server commands supported by the cluster
				inline constexpr uint32_t kAcceptedCommandsCount = 2;

				// Total number of server to client commands supported by the cluster (response
				// commands)
				inline constexpr uint32_t kGeneratedCommandsCount = 0;

				namespace GenerateRandom
				{
					inline constexpr CommandId Id = 0x00000000;
				} // namespace GenerateRandom

				namespace MfgSpecificPing
				{
					inline constexpr CommandId Id = 0x00000000;
				} // namespace MfgSpecificPing

			} // namespace Commands
		} // namespace BasicInformation
	} // namespace Clusters
} // namespace app
} // namespace chip
