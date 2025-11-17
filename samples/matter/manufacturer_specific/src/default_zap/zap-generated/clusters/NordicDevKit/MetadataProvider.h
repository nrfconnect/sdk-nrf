// DO NOT EDIT MANUALLY - Generated file
//
// Cluster metadata information for cluster NordicDevKit (cluster code: 4294048769/0xFFF1FC01)
// based on /home/arbl/ncs/nrf/samples/matter/manufacturer_specific/src/default_zap/manufacturer_specific.matter
#pragma once

#include <optional>

#include <app/data-model-provider/ClusterMetadataProvider.h>
#include <app/data-model-provider/MetadataTypes.h>
#include <clusters/NordicDevKit/Ids.h>
#include <clusters/NordicDevKit/Metadata.h>

namespace chip
{
namespace app
{
	namespace DataModel
	{

		template <> struct ClusterMetadataProvider<DataModel::AttributeEntry, Clusters::NordicDevKit::Id> {
			static constexpr std::optional<DataModel::AttributeEntry> EntryFor(AttributeId attributeId)
			{
				using namespace Clusters::NordicDevKit::Attributes;
				switch (attributeId) {
				case DevKitName::Id:
					return DevKitName::kMetadataEntry;
				case UserLED::Id:
					return UserLED::kMetadataEntry;
				case UserButton::Id:
					return UserButton::kMetadataEntry;
				default:
					return std::nullopt;
				}
			}
		};

		template <>
		struct ClusterMetadataProvider<DataModel::AcceptedCommandEntry, Clusters::NordicDevKit::Id> {
			static constexpr std::optional<DataModel::AcceptedCommandEntry> EntryFor(CommandId commandId)
			{
				using namespace Clusters::NordicDevKit::Commands;
				switch (commandId) {
				case SetLED::Id:
					return SetLED::kMetadataEntry;

				default:
					return std::nullopt;
				}
			}
		};

	} // namespace DataModel
} // namespace app
} // namespace chip
