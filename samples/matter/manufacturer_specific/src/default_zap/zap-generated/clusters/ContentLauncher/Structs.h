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
		namespace ContentLauncher
		{
			namespace Structs
			{
				namespace DimensionStruct
				{
					enum class Fields : uint8_t {
						kWidth = 0,
						kHeight = 1,
						kMetric = 2,
					};

					struct Type {
					public:
						double width = static_cast<double>(0);
						double height = static_cast<double>(0);
						MetricTypeEnum metric = static_cast<MetricTypeEnum>(0);

						CHIP_ERROR Decode(TLV::TLVReader &reader);

						static constexpr bool kIsFabricScoped = false;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;
					};

					using DecodableType = Type;

				} // namespace DimensionStruct
				namespace TrackPreferenceStruct
				{
					enum class Fields : uint8_t {
						kLanguageCode = 0,
						kCharacteristics = 1,
						kAudioOutputIndex = 2,
					};

					struct Type {
					public:
						chip::CharSpan languageCode;
						Optional<DataModel::List<const CharacteristicEnum>> characteristics;
						uint8_t audioOutputIndex = static_cast<uint8_t>(0);

						static constexpr bool kIsFabricScoped = false;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;
					};

					struct DecodableType {
					public:
						chip::CharSpan languageCode;
						Optional<DataModel::DecodableList<CharacteristicEnum>> characteristics;
						uint8_t audioOutputIndex = static_cast<uint8_t>(0);

						CHIP_ERROR Decode(TLV::TLVReader &reader);

						static constexpr bool kIsFabricScoped = false;
					};

				} // namespace TrackPreferenceStruct
				namespace PlaybackPreferencesStruct
				{
					enum class Fields : uint8_t {
						kPlaybackPosition = 0,
						kTextTrack = 1,
						kAudioTracks = 2,
					};

					struct Type {
					public:
						uint64_t playbackPosition = static_cast<uint64_t>(0);
						Structs::TrackPreferenceStruct::Type textTrack;
						Optional<DataModel::List<const Structs::TrackPreferenceStruct::Type>>
							audioTracks;

						static constexpr bool kIsFabricScoped = false;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;
					};

					struct DecodableType {
					public:
						uint64_t playbackPosition = static_cast<uint64_t>(0);
						Structs::TrackPreferenceStruct::DecodableType textTrack;
						Optional<DataModel::DecodableList<
							Structs::TrackPreferenceStruct::DecodableType>>
							audioTracks;

						CHIP_ERROR Decode(TLV::TLVReader &reader);

						static constexpr bool kIsFabricScoped = false;
					};

				} // namespace PlaybackPreferencesStruct
				namespace AdditionalInfoStruct
				{
					enum class Fields : uint8_t {
						kName = 0,
						kValue = 1,
					};

					struct Type {
					public:
						chip::CharSpan name;
						chip::CharSpan value;

						CHIP_ERROR Decode(TLV::TLVReader &reader);

						static constexpr bool kIsFabricScoped = false;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;
					};

					using DecodableType = Type;

				} // namespace AdditionalInfoStruct
				namespace ParameterStruct
				{
					enum class Fields : uint8_t {
						kType = 0,
						kValue = 1,
						kExternalIDList = 2,
					};

					struct Type {
					public:
						ParameterEnum type = static_cast<ParameterEnum>(0);
						chip::CharSpan value;
						Optional<DataModel::List<const Structs::AdditionalInfoStruct::Type>>
							externalIDList;

						static constexpr bool kIsFabricScoped = false;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;
					};

					struct DecodableType {
					public:
						ParameterEnum type = static_cast<ParameterEnum>(0);
						chip::CharSpan value;
						Optional<DataModel::DecodableList<
							Structs::AdditionalInfoStruct::DecodableType>>
							externalIDList;

						CHIP_ERROR Decode(TLV::TLVReader &reader);

						static constexpr bool kIsFabricScoped = false;
					};

				} // namespace ParameterStruct
				namespace ContentSearchStruct
				{
					enum class Fields : uint8_t {
						kParameterList = 0,
					};

					struct Type {
					public:
						DataModel::List<const Structs::ParameterStruct::Type> parameterList;

						static constexpr bool kIsFabricScoped = false;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;
					};

					struct DecodableType {
					public:
						DataModel::DecodableList<Structs::ParameterStruct::DecodableType>
							parameterList;

						CHIP_ERROR Decode(TLV::TLVReader &reader);

						static constexpr bool kIsFabricScoped = false;
					};

				} // namespace ContentSearchStruct
				namespace StyleInformationStruct
				{
					enum class Fields : uint8_t {
						kImageURL = 0,
						kColor = 1,
						kSize = 2,
					};

					struct Type {
					public:
						Optional<chip::CharSpan> imageURL;
						Optional<chip::CharSpan> color;
						Optional<Structs::DimensionStruct::Type> size;

						CHIP_ERROR Decode(TLV::TLVReader &reader);

						static constexpr bool kIsFabricScoped = false;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;
					};

					using DecodableType = Type;

				} // namespace StyleInformationStruct
				namespace BrandingInformationStruct
				{
					enum class Fields : uint8_t {
						kProviderName = 0,
						kBackground = 1,
						kLogo = 2,
						kProgressBar = 3,
						kSplash = 4,
						kWaterMark = 5,
					};

					struct Type {
					public:
						chip::CharSpan providerName;
						Optional<Structs::StyleInformationStruct::Type> background;
						Optional<Structs::StyleInformationStruct::Type> logo;
						Optional<Structs::StyleInformationStruct::Type> progressBar;
						Optional<Structs::StyleInformationStruct::Type> splash;
						Optional<Structs::StyleInformationStruct::Type> waterMark;

						CHIP_ERROR Decode(TLV::TLVReader &reader);

						static constexpr bool kIsFabricScoped = false;

						CHIP_ERROR Encode(TLV::TLVWriter &aWriter, TLV::Tag aTag) const;
					};

					using DecodableType = Type;

				} // namespace BrandingInformationStruct
			} // namespace Structs
		} // namespace ContentLauncher
	} // namespace Clusters
} // namespace app
} // namespace chip
