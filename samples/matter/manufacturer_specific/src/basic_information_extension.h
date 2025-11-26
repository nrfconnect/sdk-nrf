/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <app/clusters/basic-information/BasicInformationCluster.h>
#include <app/server-cluster/ServerClusterContext.h>
#include <app/server-cluster/ServerClusterInterface.h>

#include <zephyr/random/random.h>

class BasicInformationExtension : public chip::app::Clusters::BasicInformationCluster {
public:
	BasicInformationExtension() : mRandomNumber(sys_rand16_get()) {}

	chip::app::DataModel::ActionReturnStatus SetRandomNumber(int16_t newRandomNumber);

	/* Overrides the default BasicInformationCluster implementation. */
	chip::app::DataModel::ActionReturnStatus
	ReadAttribute(const chip::app::DataModel::ReadAttributeRequest &request,
		      chip::app::AttributeValueEncoder &encoder) override;

	chip::app::DataModel::ActionReturnStatus
	WriteAttribute(const chip::app::DataModel::WriteAttributeRequest &request,
		       chip::app::AttributeValueDecoder &decoder) override;

	CHIP_ERROR Attributes(const chip::app::ConcreteClusterPath &path,
			      chip::ReadOnlyBufferBuilder<chip::app::DataModel::AttributeEntry> &builder) override;

	CHIP_ERROR
	AcceptedCommands(const chip::app::ConcreteClusterPath &path,
			 chip::ReadOnlyBufferBuilder<chip::app::DataModel::AcceptedCommandEntry> &builder) override;

	std::optional<chip::app::DataModel::ActionReturnStatus>
	InvokeCommand(const chip::app::DataModel::InvokeRequest &request, chip::TLV::TLVReader &input_arguments,
		      chip::app::CommandHandler *handler) override;

private:
	int16_t mRandomNumber;
};
