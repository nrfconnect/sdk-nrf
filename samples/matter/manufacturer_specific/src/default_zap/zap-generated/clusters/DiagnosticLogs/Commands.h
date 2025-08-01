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

#include <clusters/DiagnosticLogs/ClusterId.h>
#include <clusters/DiagnosticLogs/CommandIds.h>
#include <clusters/DiagnosticLogs/Enums.h>
#include <clusters/DiagnosticLogs/Structs.h>

#include <cstdint>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace DiagnosticLogs
		{
			namespace Commands
			{
				// Forward-declarations so we can reference these later.

				namespace RetrieveLogsRequest
				{
					struct Type;
					struct DecodableType;
				} // namespace RetrieveLogsRequest

				namespace RetrieveLogsResponse
				{
					struct Type;
					struct DecodableType;
				} // namespace RetrieveLogsResponse

			} // namespace Commands

			namespace Commands
			{
				namespace RetrieveLogsRequest
				{
					enum class Fields : uint8_t {
						kIntent = 0,
						kRequestedProtocol = 1,
						kTransferFileDesignator = 2,
					};

					struct Type {
					public:
						// Use GetCommandId instead of commandId directly to avoid naming
						// conflict with CommandIdentification in ExecutionOfACommand
						static constexpr CommandId GetCommandId()
						{
							return Commands::RetrieveLogsRequest::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::DiagnosticLogs::Id;
						}

						IntentEnum intent = static_cast<IntentEnum>(0);
						TransferProtocolEnum requestedProtocol =
							static_cast<TransferProtocolEnum>(0);
						Optional<chip::CharSpan> transferFileDesignator;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;

						using ResponseType = Clusters::DiagnosticLogs::Commands::
							RetrieveLogsResponse::DecodableType;

						static constexpr bool MustUseTimedInvoke() { return false; }
					};

					struct DecodableType {
					public:
						static constexpr CommandId GetCommandId()
						{
							return Commands::RetrieveLogsRequest::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::DiagnosticLogs::Id;
						}
						static constexpr bool kIsFabricScoped = false;

						IntentEnum intent = static_cast<IntentEnum>(0);
						TransferProtocolEnum requestedProtocol =
							static_cast<TransferProtocolEnum>(0);
						Optional<chip::CharSpan> transferFileDesignator;

						CHIP_ERROR Decode(TLV::TLVReader &reader);
					};
				}; // namespace RetrieveLogsRequest
				namespace RetrieveLogsResponse
				{
					enum class Fields : uint8_t {
						kStatus = 0,
						kLogContent = 1,
						kUTCTimeStamp = 2,
						kTimeSinceBoot = 3,
					};

					struct Type {
					public:
						// Use GetCommandId instead of commandId directly to avoid naming
						// conflict with CommandIdentification in ExecutionOfACommand
						static constexpr CommandId GetCommandId()
						{
							return Commands::RetrieveLogsResponse::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::DiagnosticLogs::Id;
						}

						StatusEnum status = static_cast<StatusEnum>(0);
						chip::ByteSpan logContent;
						Optional<uint64_t> UTCTimeStamp;
						Optional<uint64_t> timeSinceBoot;

						CHIP_ERROR Encode(DataModel::FabricAwareTLVWriter &aWriter,
								  TLV::Tag aTag) const;

						using ResponseType = DataModel::NullObjectType;

						static constexpr bool MustUseTimedInvoke() { return false; }
					};

					struct DecodableType {
					public:
						static constexpr CommandId GetCommandId()
						{
							return Commands::RetrieveLogsResponse::Id;
						}
						static constexpr ClusterId GetClusterId()
						{
							return Clusters::DiagnosticLogs::Id;
						}

						StatusEnum status = static_cast<StatusEnum>(0);
						chip::ByteSpan logContent;
						Optional<uint64_t> UTCTimeStamp;
						Optional<uint64_t> timeSinceBoot;

						CHIP_ERROR Decode(TLV::TLVReader &reader);
					};
				}; // namespace RetrieveLogsResponse
			} // namespace Commands
		} // namespace DiagnosticLogs
	} // namespace Clusters
} // namespace app
} // namespace chip
