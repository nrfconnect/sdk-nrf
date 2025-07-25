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
// This file is generated from clusters-Enums-Check.h.zapt

#pragma once

#include <clusters/EnergyPreference/Enums.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		static auto __attribute__((unused)) EnsureKnownEnumValue(EnergyPreference::EnergyPriorityEnum val)
		{
			using EnumType = EnergyPreference::EnergyPriorityEnum;
			switch (val) {
			case EnumType::kComfort:
			case EnumType::kSpeed:
			case EnumType::kEfficiency:
			case EnumType::kWaterConsumption:
				return val;
			default:
				return EnumType::kUnknownEnumValue;
			}
		}
	} // namespace Clusters
} // namespace app
} // namespace chip
