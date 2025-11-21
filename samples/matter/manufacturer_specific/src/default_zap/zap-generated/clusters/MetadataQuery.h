// DO NOT EDIT MANUALLY - Generated file
//
// This file provides a function to query into the MetadataProviders without
// instantiating any unnecessary metadata.
// It will instatiate all metadata for the selected cluster commands, even unused commands.
//
// If used without parameters it will instatiate metadata
// for all clusters and might incur a big overhead.
//
// based on nrf/samples/matter/manufacturer_specific/src/default_zap/manufacturer_specific.matter
#pragma once

#include <clusters/AccessControl/Ids.h>
#include <clusters/AccessControl/MetadataProvider.h>
#include <clusters/AdministratorCommissioning/Ids.h>
#include <clusters/AdministratorCommissioning/MetadataProvider.h>
#include <clusters/BasicInformation/Ids.h>
#include <clusters/BasicInformation/MetadataProvider.h>
#include <clusters/Descriptor/Ids.h>
#include <clusters/Descriptor/MetadataProvider.h>
#include <clusters/GeneralCommissioning/Ids.h>
#include <clusters/GeneralCommissioning/MetadataProvider.h>
#include <clusters/GeneralDiagnostics/Ids.h>
#include <clusters/GeneralDiagnostics/MetadataProvider.h>
#include <clusters/GroupKeyManagement/Ids.h>
#include <clusters/GroupKeyManagement/MetadataProvider.h>
#include <clusters/NetworkCommissioning/Ids.h>
#include <clusters/NetworkCommissioning/MetadataProvider.h>
#include <clusters/NordicDevKit/Ids.h>
#include <clusters/NordicDevKit/MetadataProvider.h>
#include <clusters/OperationalCredentials/Ids.h>
#include <clusters/OperationalCredentials/MetadataProvider.h>
#include <clusters/OtaSoftwareUpdateProvider/Ids.h>
#include <clusters/OtaSoftwareUpdateProvider/MetadataProvider.h>
#include <clusters/OtaSoftwareUpdateRequestor/Ids.h>
#include <clusters/OtaSoftwareUpdateRequestor/MetadataProvider.h>
#include <lib/core/DataModelTypes.h>
#include <optional>

namespace chip
{
namespace app
{
	namespace DataModel
	{
		namespace detail
		{

			// Implements a search for the AcceptedCommandEntry in multiple clusters
			// If no Clusters are provided it will search all clusters.
			// We provide this function for convenience, however it is not expected to be used long-term.
			template <ClusterId... TClusterIds>
			std::optional<DataModel::AcceptedCommandEntry> AcceptedCommandEntryFor(ClusterId id,
											       CommandId command)
			{
				using namespace chip::app::Clusters;
				if constexpr (sizeof...(TClusterIds) == 0 ||
					      ((TClusterIds == AccessControl::Id) || ...)) {
					if (id == AccessControl::Id)
						return ClusterMetadataProvider<DataModel::AcceptedCommandEntry,
									       AccessControl::Id>::EntryFor(command);
				}
				if constexpr (sizeof...(TClusterIds) == 0 ||
					      ((TClusterIds == AdministratorCommissioning::Id) || ...)) {
					if (id == AdministratorCommissioning::Id)
						return ClusterMetadataProvider<
							DataModel::AcceptedCommandEntry,
							AdministratorCommissioning::Id>::EntryFor(command);
				}
				if constexpr (sizeof...(TClusterIds) == 0 ||
					      ((TClusterIds == BasicInformation::Id) || ...)) {
					if (id == BasicInformation::Id)
						return ClusterMetadataProvider<DataModel::AcceptedCommandEntry,
									       BasicInformation::Id>::EntryFor(command);
				}
				if constexpr (sizeof...(TClusterIds) == 0 || ((TClusterIds == Descriptor::Id) || ...)) {
					if (id == Descriptor::Id)
						return ClusterMetadataProvider<DataModel::AcceptedCommandEntry,
									       Descriptor::Id>::EntryFor(command);
				}
				if constexpr (sizeof...(TClusterIds) == 0 ||
					      ((TClusterIds == GeneralCommissioning::Id) || ...)) {
					if (id == GeneralCommissioning::Id)
						return ClusterMetadataProvider<
							DataModel::AcceptedCommandEntry,
							GeneralCommissioning::Id>::EntryFor(command);
				}
				if constexpr (sizeof...(TClusterIds) == 0 ||
					      ((TClusterIds == GeneralDiagnostics::Id) || ...)) {
					if (id == GeneralDiagnostics::Id)
						return ClusterMetadataProvider<DataModel::AcceptedCommandEntry,
									       GeneralDiagnostics::Id>::EntryFor(command);
				}
				if constexpr (sizeof...(TClusterIds) == 0 ||
					      ((TClusterIds == GroupKeyManagement::Id) || ...)) {
					if (id == GroupKeyManagement::Id)
						return ClusterMetadataProvider<DataModel::AcceptedCommandEntry,
									       GroupKeyManagement::Id>::EntryFor(command);
				}
				if constexpr (sizeof...(TClusterIds) == 0 ||
					      ((TClusterIds == NetworkCommissioning::Id) || ...)) {
					if (id == NetworkCommissioning::Id)
						return ClusterMetadataProvider<
							DataModel::AcceptedCommandEntry,
							NetworkCommissioning::Id>::EntryFor(command);
				}
				if constexpr (sizeof...(TClusterIds) == 0 ||
					      ((TClusterIds == NordicDevKit::Id) || ...)) {
					if (id == NordicDevKit::Id)
						return ClusterMetadataProvider<DataModel::AcceptedCommandEntry,
									       NordicDevKit::Id>::EntryFor(command);
				}
				if constexpr (sizeof...(TClusterIds) == 0 ||
					      ((TClusterIds == OperationalCredentials::Id) || ...)) {
					if (id == OperationalCredentials::Id)
						return ClusterMetadataProvider<
							DataModel::AcceptedCommandEntry,
							OperationalCredentials::Id>::EntryFor(command);
				}
				if constexpr (sizeof...(TClusterIds) == 0 ||
					      ((TClusterIds == OtaSoftwareUpdateProvider::Id) || ...)) {
					if (id == OtaSoftwareUpdateProvider::Id)
						return ClusterMetadataProvider<
							DataModel::AcceptedCommandEntry,
							OtaSoftwareUpdateProvider::Id>::EntryFor(command);
				}
				if constexpr (sizeof...(TClusterIds) == 0 ||
					      ((TClusterIds == OtaSoftwareUpdateRequestor::Id) || ...)) {
					if (id == OtaSoftwareUpdateRequestor::Id)
						return ClusterMetadataProvider<
							DataModel::AcceptedCommandEntry,
							OtaSoftwareUpdateRequestor::Id>::EntryFor(command);
				}

				return std::nullopt;
			}

		} // namespace detail
	} // namespace DataModel
} // namespace app
} // namespace chip
