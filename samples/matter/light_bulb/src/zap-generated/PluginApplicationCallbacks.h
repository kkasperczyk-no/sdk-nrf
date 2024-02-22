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

#pragma once

void MatterIdentifyPluginServerInitCallback();
void MatterGroupsPluginServerInitCallback();
void MatterOnOffPluginServerInitCallback();
void MatterLevelControlPluginServerInitCallback();
void MatterDescriptorPluginServerInitCallback();
void MatterAccessControlPluginServerInitCallback();
void MatterBasicInformationPluginServerInitCallback();
void MatterOtaSoftwareUpdateRequestorPluginServerInitCallback();
void MatterGeneralCommissioningPluginServerInitCallback();
void MatterNetworkCommissioningPluginServerInitCallback();
void MatterGeneralDiagnosticsPluginServerInitCallback();
void MatterSoftwareDiagnosticsPluginServerInitCallback();
void MatterThreadNetworkDiagnosticsPluginServerInitCallback();
void MatterWiFiNetworkDiagnosticsPluginServerInitCallback();
void MatterSwitchPluginServerInitCallback();
void MatterAdministratorCommissioningPluginServerInitCallback();
void MatterOperationalCredentialsPluginServerInitCallback();
void MatterGroupKeyManagementPluginServerInitCallback();

#define MATTER_PLUGINS_INIT                                                                                            \
	MatterIdentifyPluginServerInitCallback();                                                                      \
	MatterGroupsPluginServerInitCallback();                                                                        \
	MatterOnOffPluginServerInitCallback();                                                                         \
	MatterLevelControlPluginServerInitCallback();                                                                  \
	MatterDescriptorPluginServerInitCallback();                                                                    \
	MatterAccessControlPluginServerInitCallback();                                                                 \
	MatterBasicInformationPluginServerInitCallback();                                                              \
	MatterOtaSoftwareUpdateRequestorPluginServerInitCallback();                                                    \
	MatterGeneralCommissioningPluginServerInitCallback();                                                          \
	MatterNetworkCommissioningPluginServerInitCallback();                                                          \
	MatterGeneralDiagnosticsPluginServerInitCallback();                                                            \
	MatterSoftwareDiagnosticsPluginServerInitCallback();                                                           \
	MatterThreadNetworkDiagnosticsPluginServerInitCallback();                                                      \
	MatterWiFiNetworkDiagnosticsPluginServerInitCallback();                                                        \
	MatterSwitchPluginServerInitCallback();                                                                        \
	MatterAdministratorCommissioningPluginServerInitCallback();                                                    \
	MatterOperationalCredentialsPluginServerInitCallback();                                                        \
	MatterGroupKeyManagementPluginServerInitCallback();
