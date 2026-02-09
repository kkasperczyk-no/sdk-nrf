// DO NOT EDIT MANUALLY - Generated file
//
// Identifier constant values for cluster RandomNumberGenerator (cluster code: 4294048769/0xFFF1FC01)
#pragma once

#include <lib/core/DataModelTypes.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{
		namespace RandomNumberGenerator
		{
			namespace Commands
			{

				// Total number of client to server commands supported by the cluster
				inline constexpr uint32_t kAcceptedCommandsCount = 1;

				// Total number of server to client commands supported by the cluster (response
				// commands)
				inline constexpr uint32_t kGeneratedCommandsCount = 0;

				namespace Generate
				{
					inline constexpr CommandId Id = 0xFFF10000;
				} // namespace Generate

			} // namespace Commands
		} // namespace RandomNumberGenerator
	} // namespace Clusters
} // namespace app
} // namespace chip
