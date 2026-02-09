// DO NOT EDIT MANUALLY - Generated file
//
// Cluster metadata information for cluster RandomNumberGenerator (cluster code: 4294048769/0xFFF1FC01)
#pragma once

#include <optional>

#include <app/data-model-provider/ClusterMetadataProvider.h>
#include <app/data-model-provider/MetadataTypes.h>
#include <clusters/RandomNumberGenerator/Ids.h>
#include <clusters/RandomNumberGenerator/Metadata.h>

namespace chip
{
namespace app
{
	namespace DataModel
	{

		template <>
		struct ClusterMetadataProvider<DataModel::AttributeEntry, Clusters::RandomNumberGenerator::Id> {
			static constexpr std::optional<DataModel::AttributeEntry> EntryFor(AttributeId attributeId)
			{
				using namespace Clusters::RandomNumberGenerator::Attributes;
				switch (attributeId) {
				case GeneratedNumber::Id:
					return GeneratedNumber::kMetadataEntry;
				default:
					return std::nullopt;
				}
			}
		};

		template <>
		struct ClusterMetadataProvider<DataModel::AcceptedCommandEntry, Clusters::RandomNumberGenerator::Id> {
			static constexpr std::optional<DataModel::AcceptedCommandEntry> EntryFor(CommandId commandId)
			{
				using namespace Clusters::RandomNumberGenerator::Commands;
				switch (commandId) {
				case Generate::Id:
					return Generate::kMetadataEntry;

				default:
					return std::nullopt;
				}
			}
		};

	} // namespace DataModel
} // namespace app
} // namespace chip
