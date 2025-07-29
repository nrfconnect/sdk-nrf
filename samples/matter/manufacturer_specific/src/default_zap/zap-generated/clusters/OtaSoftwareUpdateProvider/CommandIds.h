// DO NOT EDIT MANUALLY - Generated file
//
// Identifier constant values for cluster OtaSoftwareUpdateProvider (cluster code: 41/0x29)
// based on /home/arbl/ncs/nrf/samples/matter/manufacturer_specific/src/default_zap/manufacturer_specific.matter
#pragma once

#include <lib/core/DataModelTypes.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace OtaSoftwareUpdateProvider
		{
			namespace Commands
			{
				namespace QueryImage
				{
					inline constexpr CommandId Id = 0x00000000;
				} // namespace QueryImage

				namespace ApplyUpdateRequest
				{
					inline constexpr CommandId Id = 0x00000002;
				} // namespace ApplyUpdateRequest

				namespace NotifyUpdateApplied
				{
					inline constexpr CommandId Id = 0x00000004;
				} // namespace NotifyUpdateApplied

				namespace QueryImageResponse
				{
					inline constexpr CommandId Id = 0x00000001;
				} // namespace QueryImageResponse

				namespace ApplyUpdateResponse
				{
					inline constexpr CommandId Id = 0x00000003;
				} // namespace ApplyUpdateResponse

			} // namespace Commands
		} // namespace OtaSoftwareUpdateProvider
	} // namespace Clusters
} // namespace app
} // namespace chip
