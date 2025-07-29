// DO NOT EDIT MANUALLY - Generated file
//
// Identifier constant values for cluster OtaSoftwareUpdateRequestor (cluster code: 42/0x2A)
// based on /home/arbl/ncs/nrf/samples/matter/manufacturer_specific/src/default_zap/manufacturer_specific.matter
#pragma once

#include <lib/core/DataModelTypes.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace OtaSoftwareUpdateRequestor
		{
			namespace Events
			{
				namespace StateTransition
				{
					inline constexpr EventId Id = 0x00000000;
				} // namespace StateTransition

				namespace VersionApplied
				{
					inline constexpr EventId Id = 0x00000001;
				} // namespace VersionApplied

				namespace DownloadError
				{
					inline constexpr EventId Id = 0x00000002;
				} // namespace DownloadError

			} // namespace Events
		} // namespace OtaSoftwareUpdateRequestor
	} // namespace Clusters
} // namespace app
} // namespace chip
