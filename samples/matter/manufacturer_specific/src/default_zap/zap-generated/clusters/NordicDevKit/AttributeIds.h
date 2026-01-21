// DO NOT EDIT MANUALLY - Generated file
//
// Identifier constant values for cluster NordicDevKit (cluster code: 4294048769/0xFFF1FC01)
// based on nrf/samples/matter/manufacturer_specific/src/default_zap/manufacturer_specific.matter
#pragma once

#include <clusters/shared/GlobalIds.h>
#include <lib/core/DataModelTypes.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace NordicDevKit
		{
			namespace Attributes
			{

				// Total number of attributes supported by the cluster, including global attributes
				inline constexpr uint32_t kAttributesCount = 8;

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

				namespace DevKitName
				{
					inline constexpr AttributeId Id = 0xFFF10000;
				} // namespace DevKitName

				namespace UserLED
				{
					inline constexpr AttributeId Id = 0xFFF10001;
				} // namespace UserLED

				namespace UserButton
				{
					inline constexpr AttributeId Id = 0xFFF10002;
				} // namespace UserButton

			} // namespace Attributes
		} // namespace NordicDevKit
	} // namespace Clusters
} // namespace app
} // namespace chip
