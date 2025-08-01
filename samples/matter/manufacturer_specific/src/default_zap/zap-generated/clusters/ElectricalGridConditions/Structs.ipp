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
// This file is generated from clusters-Structs.ipp.zapt

#include <clusters/ElectricalGridConditions/Structs.h>

#include <app/data-model/StructDecodeIterator.h>
#include <app/data-model/WrappedStructEncoder.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace ElectricalGridConditions
		{
			namespace Structs
			{

				namespace ElectricalGridConditionsStruct
				{
					CHIP_ERROR Type::Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const
					{
						DataModel::WrappedStructEncoder encoder{ aWriter, aTag };
						encoder.Encode(to_underlying(Fields::kPeriodStart), periodStart);
						encoder.Encode(to_underlying(Fields::kPeriodEnd), periodEnd);
						encoder.Encode(to_underlying(Fields::kGridCarbonIntensity),
							       gridCarbonIntensity);
						encoder.Encode(to_underlying(Fields::kGridCarbonLevel),
							       gridCarbonLevel);
						encoder.Encode(to_underlying(Fields::kLocalCarbonIntensity),
							       localCarbonIntensity);
						encoder.Encode(to_underlying(Fields::kLocalCarbonLevel),
							       localCarbonLevel);
						return encoder.Finalize();
					}

					CHIP_ERROR DecodableType::Decode(TLV::TLVReader &reader)
					{
						detail::StructDecodeIterator __iterator(reader);
						while (true) {
							uint8_t __context_tag = 0;
							CHIP_ERROR err = __iterator.Next(__context_tag);
							VerifyOrReturnError(err != CHIP_ERROR_END_OF_TLV,
									    CHIP_NO_ERROR);
							ReturnErrorOnFailure(err);

							if (__context_tag == to_underlying(Fields::kPeriodStart)) {
								err = DataModel::Decode(reader, periodStart);
							} else if (__context_tag == to_underlying(Fields::kPeriodEnd)) {
								err = DataModel::Decode(reader, periodEnd);
							} else if (__context_tag ==
								   to_underlying(Fields::kGridCarbonIntensity)) {
								err = DataModel::Decode(reader, gridCarbonIntensity);
							} else if (__context_tag ==
								   to_underlying(Fields::kGridCarbonLevel)) {
								err = DataModel::Decode(reader, gridCarbonLevel);
							} else if (__context_tag ==
								   to_underlying(Fields::kLocalCarbonIntensity)) {
								err = DataModel::Decode(reader, localCarbonIntensity);
							} else if (__context_tag ==
								   to_underlying(Fields::kLocalCarbonLevel)) {
								err = DataModel::Decode(reader, localCarbonLevel);
							}

							ReturnErrorOnFailure(err);
						}
					}

				} // namespace ElectricalGridConditionsStruct
			} // namespace Structs
		} // namespace ElectricalGridConditions
	} // namespace Clusters
} // namespace app
} // namespace chip
