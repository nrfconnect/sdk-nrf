// DO NOT EDIT MANUALLY - Generated file
//
// Identifier constant values for cluster AccessControl (cluster code: 31/0x1F)
// based on /home/arbl/ncs/nrf/samples/matter/manufacturer_specific/src/default_zap/manufacturer_specific.matter
#pragma once

#include <lib/core/DataModelTypes.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace AccessControl
		{
			namespace Events
			{
				namespace AccessControlEntryChanged
				{
					inline constexpr EventId Id = 0x00000000;
				} // namespace AccessControlEntryChanged

				namespace AccessControlExtensionChanged
				{
					inline constexpr EventId Id = 0x00000001;
				} // namespace AccessControlExtensionChanged

				namespace FabricRestrictionReviewUpdate
				{
					inline constexpr EventId Id = 0x00000002;
				} // namespace FabricRestrictionReviewUpdate

				namespace AuxiliaryAccessUpdated
				{
					inline constexpr EventId Id = 0x00000003;
				} // namespace AuxiliaryAccessUpdated

			} // namespace Events
		} // namespace AccessControl
	} // namespace Clusters
} // namespace app
} // namespace chip
