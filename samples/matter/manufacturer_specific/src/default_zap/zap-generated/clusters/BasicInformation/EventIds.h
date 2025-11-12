// DO NOT EDIT MANUALLY - Generated file
//
// Identifier constant values for cluster BasicInformation (cluster code: 40/0x28)
// based on
// /home/arbl/ncs/zephyr/../nrf/samples/matter/manufacturer_specific/src/default_zap/manufacturer_specific.matter
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
			namespace Events
			{
				namespace StartUp
				{
					inline constexpr EventId Id = 0x00000000;
				} // namespace StartUp

				namespace ShutDown
				{
					inline constexpr EventId Id = 0x00000001;
				} // namespace ShutDown

				namespace Leave
				{
					inline constexpr EventId Id = 0x00000002;
				} // namespace Leave

				namespace ReachableChanged
				{
					inline constexpr EventId Id = 0x00000003;
				} // namespace ReachableChanged

				namespace RandomNumberChanged
				{
					inline constexpr EventId Id = 0x00000004;
				} // namespace RandomNumberChanged

			} // namespace Events
		} // namespace BasicInformation
	} // namespace Clusters
} // namespace app
} // namespace chip
