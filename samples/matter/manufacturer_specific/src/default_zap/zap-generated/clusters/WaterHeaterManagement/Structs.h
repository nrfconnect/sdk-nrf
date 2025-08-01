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
// This file is generated from clusters-Structs.h.zapt

#pragma once

#include <app/data-model/DecodableList.h>
#include <app/data-model/List.h>
#include <app/data-model/Nullable.h>
#include <lib/core/DataModelTypes.h>
#include <lib/core/Optional.h>
#include <lib/core/TLV.h>
#include <lib/support/BitMask.h>

#include <clusters/shared/Structs.h>

#include <cstdint>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace WaterHeaterManagement
		{
			namespace Structs
			{
				namespace WaterHeaterBoostInfoStruct
				{
					enum class Fields : uint8_t {
						kDuration = 0,
						kOneShot = 1,
						kEmergencyBoost = 2,
						kTemporarySetpoint = 3,
						kTargetPercentage = 4,
						kTargetReheat = 5,
					};

					struct Type {
					public:
						uint32_t duration = static_cast<uint32_t>(0);
						Optional<bool> oneShot;
						Optional<bool> emergencyBoost;
						Optional<int16_t> temporarySetpoint;
						Optional<chip::Percent> targetPercentage;
						Optional<chip::Percent> targetReheat;

						CHIP_ERROR Decode(TLV::TLVReader &reader);

						static constexpr bool kIsFabricScoped = false;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;
					};

					using DecodableType = Type;

				} // namespace WaterHeaterBoostInfoStruct
			} // namespace Structs
		} // namespace WaterHeaterManagement
	} // namespace Clusters
} // namespace app
} // namespace chip
