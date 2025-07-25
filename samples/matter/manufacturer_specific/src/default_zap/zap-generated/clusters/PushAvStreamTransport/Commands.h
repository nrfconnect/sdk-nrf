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
// This file is generated from clusters-Commands.h.zapt

#pragma once

#include <app/data-model/DecodableList.h>
#include <app/data-model/Encode.h>
#include <app/data-model/List.h>
#include <app/data-model/NullObject.h>
#include <app/data-model/Nullable.h>
#include <lib/core/DataModelTypes.h>
#include <lib/core/Optional.h>
#include <lib/core/TLV.h>
#include <lib/support/BitMask.h>

#include <clusters/shared/Enums.h>
#include <clusters/shared/Structs.h>

#include <clusters/PushAvStreamTransport/ClusterId.h>
#include <clusters/PushAvStreamTransport/CommandIds.h>
#include <clusters/PushAvStreamTransport/Enums.h>
#include <clusters/PushAvStreamTransport/Structs.h>

#include <cstdint>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace PushAvStreamTransport
		{
			namespace Commands
			{
				// Forward-declarations so we can reference these later.

				namespace AllocatePushTransport
				{
					struct Type;
					struct DecodableType;
				} // namespace AllocatePushTransport

				namespace AllocatePushTransportResponse
				{
					struct Type;
					struct DecodableType;
				} // namespace AllocatePushTransportResponse

				namespace DeallocatePushTransport
				{
					struct Type;
					struct DecodableType;
				} // namespace DeallocatePushTransport

				namespace ModifyPushTransport
				{
					struct Type;
					struct DecodableType;
				} // namespace ModifyPushTransport

				namespace SetTransportStatus
				{
					struct Type;
					struct DecodableType;
				} // namespace SetTransportStatus

				namespace ManuallyTriggerTransport
				{
					struct Type;
					struct DecodableType;
				} // namespace ManuallyTriggerTransport

				namespace FindTransport
				{
					struct Type;
					struct DecodableType;
				} // namespace FindTransport

				namespace FindTransportResponse
				{
					struct Type;
					struct DecodableType;
				} // namespace FindTransportResponse

			} // namespace Commands

			namespace Commands
			{
				namespace AllocatePushTransport
				{
					enum class Fields : uint8_t {
						kTransportOptions = 0,
					};

					struct Type {
					public:
						// Use GetCommandId instead of commandId directly to avoid naming
						// conflict with CommandIdentification in ExecutionOfACommand
						static constexpr CommandId GetCommandId()
						{
							return Commands::AllocatePushTransport::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}

						Structs::TransportOptionsStruct::Type transportOptions;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;

						using ResponseType = Clusters::PushAvStreamTransport::Commands::
							AllocatePushTransportResponse::DecodableType;

						static constexpr bool MustUseTimedInvoke() { return false; }
					};

					struct DecodableType {
					public:
						static constexpr CommandId GetCommandId()
						{
							return Commands::AllocatePushTransport::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}
						static constexpr bool kIsFabricScoped = true;

						Structs::TransportOptionsStruct::DecodableType transportOptions;

						CHIP_ERROR Decode(TLV::TLVReader &reader,
								  FabricIndex aAccessingFabricIndex);
					};
				}; // namespace AllocatePushTransport
				namespace AllocatePushTransportResponse
				{
					enum class Fields : uint8_t {
						kTransportConfiguration = 0,
					};

					struct Type {
					public:
						// Use GetCommandId instead of commandId directly to avoid naming
						// conflict with CommandIdentification in ExecutionOfACommand
						static constexpr CommandId GetCommandId()
						{
							return Commands::AllocatePushTransportResponse::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}

						Structs::TransportConfigurationStruct::Type transportConfiguration;

						CHIP_ERROR Encode(DataModel::FabricAwareTLVWriter &aWriter,
								  TLV::Tag aTag) const;

						using ResponseType = DataModel::NullObjectType;

						static constexpr bool MustUseTimedInvoke() { return false; }
					};

					struct DecodableType {
					public:
						static constexpr CommandId GetCommandId()
						{
							return Commands::AllocatePushTransportResponse::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}

						Structs::TransportConfigurationStruct::DecodableType
							transportConfiguration;

						CHIP_ERROR Decode(TLV::TLVReader &reader);
					};
				}; // namespace AllocatePushTransportResponse
				namespace DeallocatePushTransport
				{
					enum class Fields : uint8_t {
						kConnectionID = 0,
					};

					struct Type {
					public:
						// Use GetCommandId instead of commandId directly to avoid naming
						// conflict with CommandIdentification in ExecutionOfACommand
						static constexpr CommandId GetCommandId()
						{
							return Commands::DeallocatePushTransport::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}

						uint16_t connectionID = static_cast<uint16_t>(0);

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;

						using ResponseType = DataModel::NullObjectType;

						static constexpr bool MustUseTimedInvoke() { return false; }
					};

					struct DecodableType {
					public:
						static constexpr CommandId GetCommandId()
						{
							return Commands::DeallocatePushTransport::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}
						static constexpr bool kIsFabricScoped = true;

						uint16_t connectionID = static_cast<uint16_t>(0);

						CHIP_ERROR Decode(TLV::TLVReader &reader,
								  FabricIndex aAccessingFabricIndex);
					};
				}; // namespace DeallocatePushTransport
				namespace ModifyPushTransport
				{
					enum class Fields : uint8_t {
						kConnectionID = 0,
						kTransportOptions = 1,
					};

					struct Type {
					public:
						// Use GetCommandId instead of commandId directly to avoid naming
						// conflict with CommandIdentification in ExecutionOfACommand
						static constexpr CommandId GetCommandId()
						{
							return Commands::ModifyPushTransport::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}

						uint16_t connectionID = static_cast<uint16_t>(0);
						Structs::TransportOptionsStruct::Type transportOptions;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;

						using ResponseType = DataModel::NullObjectType;

						static constexpr bool MustUseTimedInvoke() { return false; }
					};

					struct DecodableType {
					public:
						static constexpr CommandId GetCommandId()
						{
							return Commands::ModifyPushTransport::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}
						static constexpr bool kIsFabricScoped = true;

						uint16_t connectionID = static_cast<uint16_t>(0);
						Structs::TransportOptionsStruct::DecodableType transportOptions;

						CHIP_ERROR Decode(TLV::TLVReader &reader,
								  FabricIndex aAccessingFabricIndex);
					};
				}; // namespace ModifyPushTransport
				namespace SetTransportStatus
				{
					enum class Fields : uint8_t {
						kConnectionID = 0,
						kTransportStatus = 1,
					};

					struct Type {
					public:
						// Use GetCommandId instead of commandId directly to avoid naming
						// conflict with CommandIdentification in ExecutionOfACommand
						static constexpr CommandId GetCommandId()
						{
							return Commands::SetTransportStatus::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}

						DataModel::Nullable<uint16_t> connectionID;
						TransportStatusEnum transportStatus =
							static_cast<TransportStatusEnum>(0);

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;

						using ResponseType = DataModel::NullObjectType;

						static constexpr bool MustUseTimedInvoke() { return false; }
					};

					struct DecodableType {
					public:
						static constexpr CommandId GetCommandId()
						{
							return Commands::SetTransportStatus::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}
						static constexpr bool kIsFabricScoped = true;

						DataModel::Nullable<uint16_t> connectionID;
						TransportStatusEnum transportStatus =
							static_cast<TransportStatusEnum>(0);

						CHIP_ERROR Decode(TLV::TLVReader &reader,
								  FabricIndex aAccessingFabricIndex);
					};
				}; // namespace SetTransportStatus
				namespace ManuallyTriggerTransport
				{
					enum class Fields : uint8_t {
						kConnectionID = 0,
						kActivationReason = 1,
						kTimeControl = 2,
					};

					struct Type {
					public:
						// Use GetCommandId instead of commandId directly to avoid naming
						// conflict with CommandIdentification in ExecutionOfACommand
						static constexpr CommandId GetCommandId()
						{
							return Commands::ManuallyTriggerTransport::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}

						uint16_t connectionID = static_cast<uint16_t>(0);
						TriggerActivationReasonEnum activationReason =
							static_cast<TriggerActivationReasonEnum>(0);
						Optional<Structs::TransportMotionTriggerTimeControlStruct::Type>
							timeControl;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;

						using ResponseType = DataModel::NullObjectType;

						static constexpr bool MustUseTimedInvoke() { return false; }
					};

					struct DecodableType {
					public:
						static constexpr CommandId GetCommandId()
						{
							return Commands::ManuallyTriggerTransport::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}
						static constexpr bool kIsFabricScoped = true;

						uint16_t connectionID = static_cast<uint16_t>(0);
						TriggerActivationReasonEnum activationReason =
							static_cast<TriggerActivationReasonEnum>(0);
						Optional<Structs::TransportMotionTriggerTimeControlStruct::DecodableType>
							timeControl;

						CHIP_ERROR Decode(TLV::TLVReader &reader,
								  FabricIndex aAccessingFabricIndex);
					};
				}; // namespace ManuallyTriggerTransport
				namespace FindTransport
				{
					enum class Fields : uint8_t {
						kConnectionID = 0,
					};

					struct Type {
					public:
						// Use GetCommandId instead of commandId directly to avoid naming
						// conflict with CommandIdentification in ExecutionOfACommand
						static constexpr CommandId GetCommandId()
						{
							return Commands::FindTransport::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}

						Optional<DataModel::Nullable<uint16_t>> connectionID;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;

						using ResponseType = Clusters::PushAvStreamTransport::Commands::
							FindTransportResponse::DecodableType;

						static constexpr bool MustUseTimedInvoke() { return false; }
					};

					struct DecodableType {
					public:
						static constexpr CommandId GetCommandId()
						{
							return Commands::FindTransport::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}
						static constexpr bool kIsFabricScoped = true;

						Optional<DataModel::Nullable<uint16_t>> connectionID;

						CHIP_ERROR Decode(TLV::TLVReader &reader,
								  FabricIndex aAccessingFabricIndex);
					};
				}; // namespace FindTransport
				namespace FindTransportResponse
				{
					enum class Fields : uint8_t {
						kTransportConfigurations = 0,
					};

					struct Type {
					public:
						// Use GetCommandId instead of commandId directly to avoid naming
						// conflict with CommandIdentification in ExecutionOfACommand
						static constexpr CommandId GetCommandId()
						{
							return Commands::FindTransportResponse::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}

						DataModel::List<const Structs::TransportConfigurationStruct::Type>
							transportConfigurations;

						CHIP_ERROR Encode(DataModel::FabricAwareTLVWriter &aWriter,
								  TLV::Tag aTag) const;

						using ResponseType = DataModel::NullObjectType;

						static constexpr bool MustUseTimedInvoke() { return false; }
					};

					struct DecodableType {
					public:
						static constexpr CommandId GetCommandId()
						{
							return Commands::FindTransportResponse::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::PushAvStreamTransport::Id;
						}

						DataModel::DecodableList<
							Structs::TransportConfigurationStruct::DecodableType>
							transportConfigurations;

						CHIP_ERROR Decode(TLV::TLVReader &reader);
					};
				}; // namespace FindTransportResponse
			} // namespace Commands
		} // namespace PushAvStreamTransport
	} // namespace Clusters
} // namespace app
} // namespace chip
