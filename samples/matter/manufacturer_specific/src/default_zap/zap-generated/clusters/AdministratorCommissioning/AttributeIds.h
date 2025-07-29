// DO NOT EDIT MANUALLY - Generated file
//
// Identifier constant values for cluster AdministratorCommissioning (cluster code: 60/0x3C)
// based on /home/arbl/ncs/nrf/samples/matter/manufacturer_specific/src/default_zap/manufacturer_specific.matter
#pragma once

#include <clusters/shared/GlobalIds.h>
#include <lib/core/DataModelTypes.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace AdministratorCommissioning
		{
			namespace Attributes
			{
				namespace WindowStatus
				{
					inline constexpr AttributeId Id = 0x00000000;
				} // namespace WindowStatus

				namespace AdminFabricIndex
				{
					inline constexpr AttributeId Id = 0x00000001;
				} // namespace AdminFabricIndex

				namespace AdminVendorId
				{
					inline constexpr AttributeId Id = 0x00000002;
				} // namespace AdminVendorId

				namespace GeneratedCommandList
				{
					inline constexpr AttributeId Id = Globals::Attributes::GeneratedCommandList::Id;
				} // namespace GeneratedCommandList

				namespace AcceptedCommandList
				{
					inline constexpr AttributeId Id = Globals::Attributes::AcceptedCommandList::Id;
				} // namespace AcceptedCommandList

				namespace AttributeList
				{
					inline constexpr AttributeId Id = Globals::Attributes::AttributeList::Id;
				} // namespace AttributeList

				namespace FeatureMap
				{
					inline constexpr AttributeId Id = Globals::Attributes::FeatureMap::Id;
				} // namespace FeatureMap

				namespace ClusterRevision
				{
					inline constexpr AttributeId Id = Globals::Attributes::ClusterRevision::Id;
				} // namespace ClusterRevision

			} // namespace Attributes
		} // namespace AdministratorCommissioning
	} // namespace Clusters
} // namespace app
} // namespace chip
