// DO NOT EDIT MANUALLY - Generated file
//
// Cluster metadata information for cluster NordicDevKit (cluster code: 4294048769/0xFFF1FC01)
// based on nrf/samples/matter/manufacturer_specific/src/default_zap/manufacturer_specific.matter
#pragma once

#include <app/data-model-provider/MetadataTypes.h>
#include <array>
#include <lib/core/DataModelTypes.h>

#include <cstdint>

#include <clusters/NordicDevKit/Ids.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace NordicDevKit
		{

			inline constexpr uint32_t kRevision = 1;

			namespace Attributes
			{

				namespace DevKitName
				{
					inline constexpr DataModel::AttributeEntry
						kMetadataEntry(DevKitName::Id,
							       BitFlags<DataModel::AttributeQualityFlags>(),
							       Access::Privilege::kView, Access::Privilege::kOperate);
				} // namespace DevKitName
				namespace UserLED
				{
					inline constexpr DataModel::AttributeEntry
						kMetadataEntry(UserLED::Id,
							       BitFlags<DataModel::AttributeQualityFlags>(),
							       Access::Privilege::kView, std::nullopt);
				} // namespace UserLED
				namespace UserButton
				{
					inline constexpr DataModel::AttributeEntry
						kMetadataEntry(UserButton::Id,
							       BitFlags<DataModel::AttributeQualityFlags>(),
							       Access::Privilege::kView, std::nullopt);
				} // namespace UserButton
				constexpr std::array<DataModel::AttributeEntry, 3> kMandatoryMetadata = {
					DevKitName::kMetadataEntry,
					UserLED::kMetadataEntry,
					UserButton::kMetadataEntry,

				};

			} // namespace Attributes

			namespace Commands
			{

				namespace SetLED
				{
					inline constexpr DataModel::AcceptedCommandEntry
						kMetadataEntry(SetLED::Id, BitFlags<DataModel::CommandQualityFlags>(),
							       Access::Privilege::kOperate);
				} // namespace SetLED

			} // namespace Commands

			namespace Events
			{
				namespace UserButtonChanged
				{
					inline constexpr DataModel::EventEntry kMetadataEntry{
						Access::Privilege::kView
					};
				} // namespace UserButtonChanged

			} // namespace Events
		} // namespace NordicDevKit
	} // namespace Clusters
} // namespace app
} // namespace chip
