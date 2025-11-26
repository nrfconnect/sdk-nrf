/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "basic_information_extension.h"

#include <app/EventLogging.h>
#include <app/util/attribute-storage.h>
#include <clusters/BasicInformation/Events.h>
#include <clusters/BasicInformation/Metadata.h>

using namespace chip;
using namespace chip::app;

constexpr AttributeId kRandomNumberAttributeId = 0x17;

constexpr DataModel::AttributeEntry kExtraAttributeMetadata[] = {
	{ kRandomNumberAttributeId,
	  {} /* qualities */,
	  Access::Privilege::kView /* readPriv */,
	  std::nullopt /* writePriv */ },
};

DataModel::ActionReturnStatus BasicInformationExtension::SetRandomNumber(int16_t newRandomNumber)
{
	mRandomNumber = newRandomNumber;

	Clusters::BasicInformation::Events::RandomNumberChanged::Type event;
	EventNumber eventNumber;

	for (auto endpoint : EnabledEndpointsWithServerCluster(Clusters::BasicInformation::Id)) {
		/* If Basic cluster is implemented on this endpoint */
		if (CHIP_NO_ERROR != LogEvent(event, endpoint, eventNumber)) {
			ChipLogError(Zcl, "Failed to emit RandomNumberChanged event");
		}
	}

	return CHIP_NO_ERROR;
}

DataModel::ActionReturnStatus BasicInformationExtension::ReadAttribute(const DataModel::ReadAttributeRequest &request,
								       AttributeValueEncoder &encoder)
{
	switch (request.path.mAttributeId) {
	case kRandomNumberAttributeId:
		return encoder.Encode(mRandomNumber);
	default:
		return chip::app::Clusters::BasicInformationCluster::ReadAttribute(request, encoder);
	}
}

DataModel::ActionReturnStatus BasicInformationExtension::WriteAttribute(const DataModel::WriteAttributeRequest &request,
									AttributeValueDecoder &decoder)
{
	switch (request.path.mAttributeId) {
	case kRandomNumberAttributeId:
		int16_t newRandomNumber;
		ReturnErrorOnFailure(decoder.Decode(newRandomNumber));
		return NotifyAttributeChangedIfSuccess(request.path.mAttributeId, SetRandomNumber(newRandomNumber));
	default:
		return chip::app::Clusters::BasicInformationCluster::WriteAttribute(request, decoder);
	}
}

CHIP_ERROR BasicInformationExtension::Attributes(const ConcreteClusterPath &path,
						 ReadOnlyBufferBuilder<DataModel::AttributeEntry> &builder)
{
	ReturnErrorOnFailure(builder.ReferenceExisting(kExtraAttributeMetadata));

	return chip::app::Clusters::BasicInformationCluster::Attributes(path, builder);
}

CHIP_ERROR BasicInformationExtension::AcceptedCommands(const ConcreteClusterPath &path,
						       ReadOnlyBufferBuilder<DataModel::AcceptedCommandEntry> &builder)
{
	/* The BasicInformationCluster does not have any commands, so it is not necessary to call the implementation of the base class. */
	static constexpr DataModel::AcceptedCommandEntry kAcceptedCommands[] = {
		Clusters::BasicInformation::Commands::GenerateRandom::kMetadataEntry
	};
	return builder.ReferenceExisting(kAcceptedCommands);
}

std::optional<DataModel::ActionReturnStatus>
BasicInformationExtension::InvokeCommand(const DataModel::InvokeRequest &request, chip::TLV::TLVReader &input_arguments,
					 CommandHandler *handler)
{
	switch (request.path.mCommandId) {
	case Clusters::BasicInformation::Commands::GenerateRandom::Id: {
		return SetRandomNumber(sys_rand16_get());
	}
	default:
		/* The BasicInformationCluster does not have any commands, so it is not necessary to call the implementation of the base class. */
		return Protocols::InteractionModel::Status::UnsupportedCommand;
	}
}
