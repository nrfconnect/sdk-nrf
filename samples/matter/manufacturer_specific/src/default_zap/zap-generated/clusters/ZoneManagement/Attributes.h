/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

// THIS FILE IS GENERATED BY ZAP
// This file is generated from clusters-Attributes.h.zapt

#pragma once

#include <app/ConcreteAttributePath.h>
#include <app/data-model/DecodableList.h>
#include <app/data-model/List.h>
#include <app/data-model/Nullable.h>
#include <app/util/basic-types.h>
#include <lib/core/TLV.h>
#include <lib/support/BitMask.h>

#include <clusters/shared/Attributes.h>
#include <clusters/shared/Enums.h>
#include <clusters/shared/Structs.h>

#include <clusters/ZoneManagement/AttributeIds.h>
#include <clusters/ZoneManagement/ClusterId.h>
#include <clusters/ZoneManagement/Enums.h>
#include <clusters/ZoneManagement/Structs.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace ZoneManagement
		{
			namespace Attributes
			{

				namespace MaxUserDefinedZones
				{
					struct TypeInfo {
						using Type = uint8_t;
						using DecodableType = uint8_t;
						using DecodableArgType = uint8_t;

						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}
						static constexpr AttributeId GetAttributeId()
						{
							return Attributes::MaxUserDefinedZones::Id;
						}
						static constexpr bool MustUseTimedWrite() { return false; }
					};
				} // namespace MaxUserDefinedZones
				namespace MaxZones
				{
					struct TypeInfo {
						using Type = uint8_t;
						using DecodableType = uint8_t;
						using DecodableArgType = uint8_t;

						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}
						static constexpr AttributeId GetAttributeId()
						{
							return Attributes::MaxZones::Id;
						}
						static constexpr bool MustUseTimedWrite() { return false; }
					};
				} // namespace MaxZones
				namespace Zones
				{
					struct TypeInfo {
						using Type = chip::app::DataModel::List<
							const chip::app::Clusters::ZoneManagement::Structs::
								ZoneInformationStruct::Type>;
						using DecodableType = chip::app::DataModel::DecodableList<
							chip::app::Clusters::ZoneManagement::Structs::
								ZoneInformationStruct::DecodableType>;
						using DecodableArgType = const chip::app::DataModel::DecodableList<
							chip::app::Clusters::ZoneManagement::Structs::
								ZoneInformationStruct::DecodableType> &;

						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}
						static constexpr AttributeId GetAttributeId()
						{
							return Attributes::Zones::Id;
						}
						static constexpr bool MustUseTimedWrite() { return false; }
					};
				} // namespace Zones
				namespace Triggers
				{
					struct TypeInfo {
						using Type = chip::app::DataModel::List<
							const chip::app::Clusters::ZoneManagement::Structs::
								ZoneTriggerControlStruct::Type>;
						using DecodableType = chip::app::DataModel::DecodableList<
							chip::app::Clusters::ZoneManagement::Structs::
								ZoneTriggerControlStruct::DecodableType>;
						using DecodableArgType = const chip::app::DataModel::DecodableList<
							chip::app::Clusters::ZoneManagement::Structs::
								ZoneTriggerControlStruct::DecodableType> &;

						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}
						static constexpr AttributeId GetAttributeId()
						{
							return Attributes::Triggers::Id;
						}
						static constexpr bool MustUseTimedWrite() { return false; }
					};
				} // namespace Triggers
				namespace SensitivityMax
				{
					struct TypeInfo {
						using Type = uint8_t;
						using DecodableType = uint8_t;
						using DecodableArgType = uint8_t;

						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}
						static constexpr AttributeId GetAttributeId()
						{
							return Attributes::SensitivityMax::Id;
						}
						static constexpr bool MustUseTimedWrite() { return false; }
					};
				} // namespace SensitivityMax
				namespace Sensitivity
				{
					struct TypeInfo {
						using Type = uint8_t;
						using DecodableType = uint8_t;
						using DecodableArgType = uint8_t;

						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}
						static constexpr AttributeId GetAttributeId()
						{
							return Attributes::Sensitivity::Id;
						}
						static constexpr bool MustUseTimedWrite() { return false; }
					};
				} // namespace Sensitivity
				namespace TwoDCartesianMax
				{
					struct TypeInfo {
						using Type = chip::app::Clusters::ZoneManagement::Structs::
							TwoDCartesianVertexStruct::Type;
						using DecodableType = chip::app::Clusters::ZoneManagement::Structs::
							TwoDCartesianVertexStruct::DecodableType;
						using DecodableArgType = const chip::app::Clusters::ZoneManagement::
							Structs::TwoDCartesianVertexStruct::DecodableType &;

						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}
						static constexpr AttributeId GetAttributeId()
						{
							return Attributes::TwoDCartesianMax::Id;
						}
						static constexpr bool MustUseTimedWrite() { return false; }
					};
				} // namespace TwoDCartesianMax
				namespace GeneratedCommandList
				{
					struct TypeInfo
						: public Clusters::Globals::Attributes::GeneratedCommandList::TypeInfo {
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}
					};
				} // namespace GeneratedCommandList
				namespace AcceptedCommandList
				{
					struct TypeInfo
						: public Clusters::Globals::Attributes::AcceptedCommandList::TypeInfo {
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}
					};
				} // namespace AcceptedCommandList
				namespace AttributeList
				{
					struct TypeInfo
						: public Clusters::Globals::Attributes::AttributeList::TypeInfo {
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}
					};
				} // namespace AttributeList
				namespace FeatureMap
				{
					struct TypeInfo : public Clusters::Globals::Attributes::FeatureMap::TypeInfo {
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}
					};
				} // namespace FeatureMap
				namespace ClusterRevision
				{
					struct TypeInfo
						: public Clusters::Globals::Attributes::ClusterRevision::TypeInfo {
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}
					};
				} // namespace ClusterRevision

				struct TypeInfo {
					struct DecodableType {
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ZoneManagement::Id;
						}

						CHIP_ERROR Decode(TLV::TLVReader &reader,
								  const ConcreteAttributePath &path);

						Attributes::MaxUserDefinedZones::TypeInfo::DecodableType
							maxUserDefinedZones = static_cast<uint8_t>(0);
						Attributes::MaxZones::TypeInfo::DecodableType maxZones =
							static_cast<uint8_t>(0);
						Attributes::Zones::TypeInfo::DecodableType zones;
						Attributes::Triggers::TypeInfo::DecodableType triggers;
						Attributes::SensitivityMax::TypeInfo::DecodableType sensitivityMax =
							static_cast<uint8_t>(0);
						Attributes::Sensitivity::TypeInfo::DecodableType sensitivity =
							static_cast<uint8_t>(0);
						Attributes::TwoDCartesianMax::TypeInfo::DecodableType twoDCartesianMax;
						Attributes::GeneratedCommandList::TypeInfo::DecodableType
							generatedCommandList;
						Attributes::AcceptedCommandList::TypeInfo::DecodableType
							acceptedCommandList;
						Attributes::AttributeList::TypeInfo::DecodableType attributeList;
						Attributes::FeatureMap::TypeInfo::DecodableType featureMap =
							static_cast<uint32_t>(0);
						Attributes::ClusterRevision::TypeInfo::DecodableType clusterRevision =
							static_cast<uint16_t>(0);
					};
				};
			} // namespace Attributes
		} // namespace ZoneManagement
	} // namespace Clusters
} // namespace app
} // namespace chip
