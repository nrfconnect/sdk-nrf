// DO NOT EDIT MANUALLY - Generated file
//
// Identifier constant values for cluster NordicDevKit (cluster code: 4294048769/0xFFF1FC01)
// based on nrf/samples/matter/manufacturer_specific/src/default_zap/manufacturer_specific.matter
#pragma once

#include <lib/core/DataModelTypes.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace NordicDevKit
		{
			namespace Commands
			{

				// Total number of client to server commands supported by the cluster
				inline constexpr uint32_t kAcceptedCommandsCount = 1;

				// Total number of server to client commands supported by the cluster (response
				// commands)
				inline constexpr uint32_t kGeneratedCommandsCount = 0;

				namespace SetLED
				{
					inline constexpr CommandId Id = 0xFFF10000;
				} // namespace SetLED

			} // namespace Commands
		} // namespace NordicDevKit
	} // namespace Clusters
} // namespace app
} // namespace chip
