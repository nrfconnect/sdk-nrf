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
// This file is generated from clusters-Events.h.zapt

#pragma once

#include <app/EventLoggingTypes.h>
#include <app/data-model/DecodableList.h>
#include <app/data-model/List.h>
#include <app/data-model/Nullable.h>
#include <lib/core/DataModelTypes.h>
#include <lib/core/TLV.h>
#include <lib/support/BitMask.h>

#include <clusters/shared/Enums.h>
#include <clusters/shared/Structs.h>

#include <clusters/ThreadNetworkDiagnostics/ClusterId.h>
#include <clusters/ThreadNetworkDiagnostics/Enums.h>
#include <clusters/ThreadNetworkDiagnostics/EventIds.h>
#include <clusters/ThreadNetworkDiagnostics/Structs.h>

#include <cstdint>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace ThreadNetworkDiagnostics
		{
			namespace Events
			{
				namespace ConnectionStatus
				{
					static constexpr PriorityLevel kPriorityLevel = PriorityLevel::Info;

					enum class Fields : uint8_t {
						kConnectionStatus = 0,
					};

					struct Type {
					public:
						static constexpr PriorityLevel GetPriorityLevel()
						{
							return kPriorityLevel;
						}
						static constexpr EventId GetEventId()
						{
							return Events::ConnectionStatus::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ThreadNetworkDiagnostics::Id;
						}
						static constexpr bool kIsFabricScoped = false;

						ConnectionStatusEnum connectionStatus =
							static_cast<ConnectionStatusEnum>(0);

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;
					};

					struct DecodableType {
					public:
						static constexpr PriorityLevel GetPriorityLevel()
						{
							return kPriorityLevel;
						}
						static constexpr EventId GetEventId()
						{
							return Events::ConnectionStatus::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ThreadNetworkDiagnostics::Id;
						}

						ConnectionStatusEnum connectionStatus =
							static_cast<ConnectionStatusEnum>(0);

						CHIP_ERROR Decode(TLV::TLVReader &reader);
					};
				} // namespace ConnectionStatus
				namespace NetworkFaultChange
				{
					static constexpr PriorityLevel kPriorityLevel = PriorityLevel::Info;

					enum class Fields : uint8_t {
						kCurrent = 0,
						kPrevious = 1,
					};

					struct Type {
					public:
						static constexpr PriorityLevel GetPriorityLevel()
						{
							return kPriorityLevel;
						}
						static constexpr EventId GetEventId()
						{
							return Events::NetworkFaultChange::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ThreadNetworkDiagnostics::Id;
						}
						static constexpr bool kIsFabricScoped = false;

						DataModel::List<const NetworkFaultEnum> current;
						DataModel::List<const NetworkFaultEnum> previous;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;
					};

					struct DecodableType {
					public:
						static constexpr PriorityLevel GetPriorityLevel()
						{
							return kPriorityLevel;
						}
						static constexpr EventId GetEventId()
						{
							return Events::NetworkFaultChange::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::ThreadNetworkDiagnostics::Id;
						}

						DataModel::DecodableList<NetworkFaultEnum> current;
						DataModel::DecodableList<NetworkFaultEnum> previous;

						CHIP_ERROR Decode(TLV::TLVReader &reader);
					};
				} // namespace NetworkFaultChange
			} // namespace Events
		} // namespace ThreadNetworkDiagnostics
	} // namespace Clusters
} // namespace app
} // namespace chip
