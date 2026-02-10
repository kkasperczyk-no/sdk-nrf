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

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::app;

constexpr AttributeId kCustomBasicInfoAttributeId = 0xfff10000;

constexpr DataModel::AttributeEntry kExtraAttributeMetadata[] = {
	{ kCustomBasicInfoAttributeId,
	  {} /* qualities */,
	  Access::Privilege::kView /* readPriv */,
	  std::nullopt /* writePriv */ },
};

DataModel::ActionReturnStatus BasicInformationExtension::SetCustomBasicInfoAttribute(bool newCustomBasicInfoAttribute)
{
	mCustomBasicInfoAttribute = newCustomBasicInfoAttribute;

	return CHIP_NO_ERROR;
}

DataModel::ActionReturnStatus BasicInformationExtension::ReadAttribute(const DataModel::ReadAttributeRequest &request,
								       AttributeValueEncoder &encoder)
{
	switch (request.path.mAttributeId) {
	case kCustomBasicInfoAttributeId:
		return encoder.Encode(mCustomBasicInfoAttribute);
	default:
		return chip::app::Clusters::BasicInformationCluster::ReadAttribute(request, encoder);
	}
}

DataModel::ActionReturnStatus BasicInformationExtension::WriteAttribute(const DataModel::WriteAttributeRequest &request,
									AttributeValueDecoder &decoder)
{
	switch (request.path.mAttributeId) {
	case kCustomBasicInfoAttributeId:
		bool newCustomBasicInfoAttribute;
		ReturnErrorOnFailure(decoder.Decode(newCustomBasicInfoAttribute));
		return NotifyAttributeChangedIfSuccess(request.path.mAttributeId, SetCustomBasicInfoAttribute(newCustomBasicInfoAttribute));
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
		Clusters::BasicInformation::Commands::CustomBasicInfoCommand::kMetadataEntry
	};
	return builder.ReferenceExisting(kAcceptedCommands);
}

std::optional<DataModel::ActionReturnStatus>
BasicInformationExtension::InvokeCommand(const DataModel::InvokeRequest &request, chip::TLV::TLVReader &input_arguments,
					 CommandHandler *handler)
{
	switch (request.path.mCommandId) {
	case Clusters::BasicInformation::Commands::CustomBasicInfoCommand::Id: {
		LOG_INF("CustomBasicInfoCommand received");
		return Protocols::InteractionModel::Status::Success;
	}
	default:
		/* The BasicInformationCluster does not have any commands, so it is not necessary to call the implementation of the base class. */
		return Protocols::InteractionModel::Status::UnsupportedCommand;
	}
}
