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

// Prevent multiple inclusion
#pragma once

#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/ids/Commands.h>

#include <controller/CHIPCluster.h>
#include <lib/core/CHIPCallback.h>
#include <lib/support/Span.h>

namespace chip
{
namespace Controller
{

	class DLL_EXPORT OnOffCluster : public ClusterBase {
	public:
		OnOffCluster(Messaging::ExchangeManager &exchangeManager, const SessionHandle &session,
			     EndpointId endpoint)
			: ClusterBase(exchangeManager, session, endpoint)
		{
		}
		~OnOffCluster() {}
	};

	class DLL_EXPORT OtaSoftwareUpdateProviderCluster : public ClusterBase {
	public:
		OtaSoftwareUpdateProviderCluster(Messaging::ExchangeManager &exchangeManager,
						 const SessionHandle &session, EndpointId endpoint)
			: ClusterBase(exchangeManager, session, endpoint)
		{
		}
		~OtaSoftwareUpdateProviderCluster() {}
	};

} // namespace Controller
} // namespace chip
