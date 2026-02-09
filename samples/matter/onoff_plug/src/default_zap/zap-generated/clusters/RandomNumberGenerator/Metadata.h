// DO NOT EDIT MANUALLY - Generated file
//
// Cluster metadata information for cluster RandomNumberGenerator (cluster code: 4294048769/0xFFF1FC01)
#pragma once

#include <app/data-model-provider/MetadataTypes.h>
#include <array>
#include <lib/core/DataModelTypes.h>

#include <cstdint>

#include <clusters/RandomNumberGenerator/Ids.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace RandomNumberGenerator
		{

			inline constexpr uint32_t kRevision = 1;

			namespace Attributes
			{

				namespace GeneratedNumber
				{
					inline constexpr DataModel::AttributeEntry
						kMetadataEntry(GeneratedNumber::Id,
							       BitFlags<DataModel::AttributeQualityFlags>(),
							       Access::Privilege::kView, std::nullopt);
				} // namespace GeneratedNumber
				constexpr std::array<DataModel::AttributeEntry, 1> kMandatoryMetadata = {
					GeneratedNumber::kMetadataEntry,

				};

			} // namespace Attributes

			namespace Commands
			{

				namespace Generate
				{
					inline constexpr DataModel::AcceptedCommandEntry
						kMetadataEntry(Generate::Id, BitFlags<DataModel::CommandQualityFlags>(),
							       Access::Privilege::kOperate);
				} // namespace Generate

			} // namespace Commands

			namespace Events
			{
				namespace NumberGenerated
				{
					inline constexpr DataModel::EventEntry kMetadataEntry{
						Access::Privilege::kView
					};
				} // namespace NumberGenerated

			} // namespace Events
		} // namespace RandomNumberGenerator
	} // namespace Clusters
} // namespace app
} // namespace chip
