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

#include <app/util/endpoint-config-defines.h>
#include <lib/core/CHIPConfig.h>

// Default values for the attributes longer than a pointer,
// in a form of a binary blob
// Separate block is generated for big-endian and little-endian cases.
#if CHIP_CONFIG_BIG_ENDIAN_TARGET
#define GENERATED_DEFAULTS                                                                                             \
	{                                                                                                              \
		/* Endpoint: 0, Cluster: General Commissioning (server), big-endian */                                 \
                                                                                                                       \
		/* 0 - Breadcrumb, */                                                                                  \
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                        \
	}

#else // !CHIP_CONFIG_BIG_ENDIAN_TARGET
#define GENERATED_DEFAULTS                                                                                             \
	{                                                                                                              \
		/* Endpoint: 0, Cluster: General Commissioning (server), little-endian */                              \
                                                                                                                       \
		/* 0 - Breadcrumb, */                                                                                  \
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                        \
	}

#endif // CHIP_CONFIG_BIG_ENDIAN_TARGET

#define GENERATED_DEFAULTS_COUNT (1)

// This is an array of EmberAfAttributeMinMaxValue structures.
#define GENERATED_MIN_MAX_DEFAULT_COUNT 0
#define GENERATED_MIN_MAX_DEFAULTS                                                                                     \
	{                                                                                                              \
	}

// This is an array of EmberAfAttributeMetadata structures.
#define GENERATED_ATTRIBUTE_COUNT 85
#define GENERATED_ATTRIBUTES                                                                                                          \
	{                                                                                                                             \
		/* Endpoint: 0, Cluster: Descriptor (server) */                                                                       \
		{ ZAP_EMPTY_DEFAULT(), 0x00000000, 0, ZAP_TYPE(ARRAY), ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* DeviceTypeList      \
														*/                    \
			{ ZAP_EMPTY_DEFAULT(), 0x00000001, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* ServerList */                                                    \
			{ ZAP_EMPTY_DEFAULT(), 0x00000002, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* ClientList */                                                    \
			{ ZAP_EMPTY_DEFAULT(), 0x00000003, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* PartsList */                                                     \
			{ ZAP_EMPTY_DEFAULT(), 0x0000FFFC, 4, ZAP_TYPE(BITMAP32),                                                     \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* FeatureMap */                                                    \
			{ ZAP_EMPTY_DEFAULT(), 0x0000FFFD, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* ClusterRevision */                                               \
                                                                                                                                      \
			/* Endpoint: 0, Cluster: Access Control (server) */                                                           \
			{ ZAP_EMPTY_DEFAULT(), 0x00000000, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(WRITABLE) }, /* ACL */                            \
			{ ZAP_EMPTY_DEFAULT(), 0x00000001, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(WRITABLE) }, /* Extension */                      \
			{ ZAP_EMPTY_DEFAULT(), 0x00000002, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* SubjectsPerAccessControlEntry */                                 \
			{ ZAP_EMPTY_DEFAULT(), 0x00000003, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* TargetsPerAccessControlEntry */                                  \
			{ ZAP_EMPTY_DEFAULT(), 0x00000004, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* AccessControlEntriesPerFabric */                                 \
			{ ZAP_SIMPLE_DEFAULT(1), 0x0000FFFC, 4, ZAP_TYPE(BITMAP32), 0 }, /* FeatureMap */                             \
			{ ZAP_EMPTY_DEFAULT(), 0x0000FFFD, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* ClusterRevision */                                               \
                                                                                                                                      \
			/* Endpoint: 0, Cluster: Basic Information (server) */                                                        \
			{ ZAP_EMPTY_DEFAULT(), 0x00000000, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* DataModelRevision                \
												   */                                 \
			{ ZAP_EMPTY_DEFAULT(), 0x00000001, 33, ZAP_TYPE(CHAR_STRING),                                                 \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* VendorName */                    \
			{ ZAP_EMPTY_DEFAULT(), 0x00000002, 2, ZAP_TYPE(VENDOR_ID),                                                    \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* VendorID */                      \
			{ ZAP_EMPTY_DEFAULT(), 0x00000003, 33, ZAP_TYPE(CHAR_STRING),                                                 \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* ProductName */                   \
			{ ZAP_EMPTY_DEFAULT(), 0x00000004, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* ProductID */                     \
			{ ZAP_EMPTY_DEFAULT(), 0x00000005, 33, ZAP_TYPE(CHAR_STRING),                                                 \
			  ZAP_ATTRIBUTE_MASK(TOKENIZE) | ZAP_ATTRIBUTE_MASK(SINGLETON) |                                              \
				  ZAP_ATTRIBUTE_MASK(WRITABLE) }, /* NodeLabel */                                                     \
			{ ZAP_EMPTY_DEFAULT(), 0x00000006, 3, ZAP_TYPE(CHAR_STRING),                                                  \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) |                                      \
				  ZAP_ATTRIBUTE_MASK(WRITABLE) }, /* Location */                                                      \
			{ ZAP_EMPTY_DEFAULT(), 0x00000007, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* HardwareVersion                  \
												   */                                 \
			{ ZAP_EMPTY_DEFAULT(), 0x00000008, 65, ZAP_TYPE(CHAR_STRING),                                                 \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* HardwareVersionString            \
												   */                                 \
			{ ZAP_EMPTY_DEFAULT(), 0x00000009, 4, ZAP_TYPE(INT32U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* SoftwareVersion                  \
												   */                                 \
			{ ZAP_EMPTY_DEFAULT(), 0x0000000A, 65, ZAP_TYPE(CHAR_STRING),                                                 \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* SoftwareVersionString            \
												   */                                 \
			{ ZAP_EMPTY_DEFAULT(), 0x0000000B, 17, ZAP_TYPE(CHAR_STRING),                                                 \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* ManufacturingDate                \
												   */                                 \
			{ ZAP_EMPTY_DEFAULT(), 0x0000000F, 33, ZAP_TYPE(CHAR_STRING),                                                 \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* SerialNumber */                  \
			{ ZAP_EMPTY_DEFAULT(), 0x00000012, 33, ZAP_TYPE(CHAR_STRING),                                                 \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* UniqueID */                      \
			{ ZAP_EMPTY_DEFAULT(), 0x00000013, 0, ZAP_TYPE(STRUCT),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* CapabilityMinima */                                              \
			{ ZAP_EMPTY_DEFAULT(), 0x00000015, 4, ZAP_TYPE(INT32U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* SpecificationVersion             \
												   */                                 \
			{ ZAP_EMPTY_DEFAULT(), 0x00000016, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* MaxPathsPerInvoke                \
												   */                                 \
			{ ZAP_SIMPLE_DEFAULT(0), 0x0000FFFC, 4, ZAP_TYPE(BITMAP32), 0 }, /* FeatureMap */                             \
			{ ZAP_SIMPLE_DEFAULT(4), 0x0000FFFD, 2, ZAP_TYPE(INT16U),                                                     \
			  ZAP_ATTRIBUTE_MASK(SINGLETON) }, /* ClusterRevision */                                                      \
                                                                                                                                      \
			/* Endpoint: 0, Cluster: OTA Software Update Requestor (server) */                                            \
			{ ZAP_EMPTY_DEFAULT(), 0x00000000, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(WRITABLE) }, /* DefaultOTAProviders               \
												  */                                  \
			{ ZAP_SIMPLE_DEFAULT(1), 0x00000001, 1, ZAP_TYPE(BOOLEAN), 0 }, /* UpdatePossible */                          \
			{ ZAP_SIMPLE_DEFAULT(0), 0x00000002, 1, ZAP_TYPE(ENUM8), 0 }, /* UpdateState */                               \
			{ ZAP_EMPTY_DEFAULT(), 0x00000003, 1, ZAP_TYPE(INT8U), ZAP_ATTRIBUTE_MASK(NULLABLE) }, /* UpdateStateProgress \
														*/                    \
			{ ZAP_SIMPLE_DEFAULT(0), 0x0000FFFC, 4, ZAP_TYPE(BITMAP32), 0 }, /* FeatureMap */                             \
			{ ZAP_SIMPLE_DEFAULT(1), 0x0000FFFD, 2, ZAP_TYPE(INT16U), 0 }, /* ClusterRevision */                          \
                                                                                                                                      \
			/* Endpoint: 0, Cluster: General Commissioning (server) */                                                    \
			{ ZAP_LONG_DEFAULTS_INDEX(0), 0x00000000, 8, ZAP_TYPE(INT64U),                                                \
			  ZAP_ATTRIBUTE_MASK(WRITABLE) }, /* Breadcrumb */                                                            \
			{ ZAP_EMPTY_DEFAULT(), 0x00000001, 0, ZAP_TYPE(STRUCT),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* BasicCommissioningInfo */                                        \
			{ ZAP_EMPTY_DEFAULT(), 0x00000002, 1, ZAP_TYPE(ENUM8),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* RegulatoryConfig */                                              \
			{ ZAP_EMPTY_DEFAULT(), 0x00000003, 1, ZAP_TYPE(ENUM8),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* LocationCapability */                                            \
			{ ZAP_EMPTY_DEFAULT(), 0x00000004, 1, ZAP_TYPE(BOOLEAN),                                                      \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* SupportsConcurrentConnection */                                  \
			{ ZAP_SIMPLE_DEFAULT(0), 0x0000FFFC, 4, ZAP_TYPE(BITMAP32), 0 }, /* FeatureMap */                             \
			{ ZAP_SIMPLE_DEFAULT(2), 0x0000FFFD, 2, ZAP_TYPE(INT16U), 0 }, /* ClusterRevision */                          \
                                                                                                                                      \
			/* Endpoint: 0, Cluster: Network Commissioning (server) */                                                    \
			{ ZAP_EMPTY_DEFAULT(), 0x00000000, 1, ZAP_TYPE(INT8U),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* MaxNetworks */                                                   \
			{ ZAP_EMPTY_DEFAULT(), 0x00000001, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* Networks */                                                      \
			{ ZAP_EMPTY_DEFAULT(), 0x00000002, 1, ZAP_TYPE(INT8U),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* ScanMaxTimeSeconds */                                            \
			{ ZAP_EMPTY_DEFAULT(), 0x00000003, 1, ZAP_TYPE(INT8U),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* ConnectMaxTimeSeconds */                                         \
			{ ZAP_EMPTY_DEFAULT(), 0x00000004, 1, ZAP_TYPE(BOOLEAN),                                                      \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(WRITABLE) }, /* InterfaceEnabled                  \
												  */                                  \
			{ ZAP_EMPTY_DEFAULT(), 0x00000005, 1, ZAP_TYPE(ENUM8),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(NULLABLE) }, /* LastNetworkingStatus              \
												  */                                  \
			{ ZAP_EMPTY_DEFAULT(), 0x00000006, 33, ZAP_TYPE(OCTET_STRING),                                                \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(NULLABLE) }, /* LastNetworkID */                  \
			{ ZAP_EMPTY_DEFAULT(), 0x00000007, 4, ZAP_TYPE(INT32S),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(NULLABLE) }, /* LastConnectErrorValue             \
												  */                                  \
			{ ZAP_EMPTY_DEFAULT(), 0x00000008, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* SupportedWiFiBands */                                            \
			{ ZAP_EMPTY_DEFAULT(), 0x00000009, 2, ZAP_TYPE(BITMAP16), 0 }, /* SupportedThreadFeatures */                  \
			{ ZAP_SIMPLE_DEFAULT(4), 0x0000000A, 2, ZAP_TYPE(INT16U), 0 }, /* ThreadVersion */                            \
			{ ZAP_EMPTY_DEFAULT(), 0x0000FFFC, 4, ZAP_TYPE(BITMAP32),                                                     \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* FeatureMap */                                                    \
			{ ZAP_SIMPLE_DEFAULT(2), 0x0000FFFD, 2, ZAP_TYPE(INT16U), 0 }, /* ClusterRevision */                          \
                                                                                                                                      \
			/* Endpoint: 0, Cluster: General Diagnostics (server) */                                                      \
			{ ZAP_EMPTY_DEFAULT(), 0x00000000, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* NetworkInterfaces */                                             \
			{ ZAP_EMPTY_DEFAULT(), 0x00000001, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* RebootCount */                                                   \
			{ ZAP_EMPTY_DEFAULT(), 0x00000002, 8, ZAP_TYPE(INT64U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* UpTime */                                                        \
			{ ZAP_EMPTY_DEFAULT(), 0x00000003, 4, ZAP_TYPE(INT32U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* TotalOperationalHours */                                         \
			{ ZAP_EMPTY_DEFAULT(), 0x00000004, 1, ZAP_TYPE(ENUM8),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* BootReason */                                                    \
			{ ZAP_EMPTY_DEFAULT(), 0x00000008, 1, ZAP_TYPE(BOOLEAN),                                                      \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* TestEventTriggersEnabled */                                      \
			{ ZAP_EMPTY_DEFAULT(), 0x0000FFFC, 4, ZAP_TYPE(BITMAP32),                                                     \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* FeatureMap */                                                    \
			{ ZAP_EMPTY_DEFAULT(), 0x0000FFFD, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* ClusterRevision */                                               \
                                                                                                                                      \
			/* Endpoint: 0, Cluster: Administrator Commissioning (server) */                                              \
			{ ZAP_EMPTY_DEFAULT(), 0x00000000, 1, ZAP_TYPE(ENUM8),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* WindowStatus */                                                  \
			{ ZAP_EMPTY_DEFAULT(), 0x00000001, 1, ZAP_TYPE(FABRIC_IDX),                                                   \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(NULLABLE) }, /* AdminFabricIndex                  \
												  */                                  \
			{ ZAP_EMPTY_DEFAULT(), 0x00000002, 2, ZAP_TYPE(VENDOR_ID),                                                    \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(NULLABLE) }, /* AdminVendorId */                  \
			{ ZAP_SIMPLE_DEFAULT(1), 0x0000FFFC, 4, ZAP_TYPE(BITMAP32), 0 }, /* FeatureMap */                             \
			{ ZAP_SIMPLE_DEFAULT(1), 0x0000FFFD, 2, ZAP_TYPE(INT16U), 0 }, /* ClusterRevision */                          \
                                                                                                                                      \
			/* Endpoint: 0, Cluster: Operational Credentials (server) */                                                  \
			{ ZAP_EMPTY_DEFAULT(), 0x00000000, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* NOCs */                                                          \
			{ ZAP_EMPTY_DEFAULT(), 0x00000001, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* Fabrics */                                                       \
			{ ZAP_EMPTY_DEFAULT(), 0x00000002, 1, ZAP_TYPE(INT8U),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* SupportedFabrics */                                              \
			{ ZAP_EMPTY_DEFAULT(), 0x00000003, 1, ZAP_TYPE(INT8U),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* CommissionedFabrics */                                           \
			{ ZAP_EMPTY_DEFAULT(), 0x00000004, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* TrustedRootCertificates */                                       \
			{ ZAP_EMPTY_DEFAULT(), 0x00000005, 1, ZAP_TYPE(INT8U),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* CurrentFabricIndex */                                            \
			{ ZAP_SIMPLE_DEFAULT(0), 0x0000FFFC, 4, ZAP_TYPE(BITMAP32), 0 }, /* FeatureMap */                             \
			{ ZAP_SIMPLE_DEFAULT(1), 0x0000FFFD, 2, ZAP_TYPE(INT16U), 0 }, /* ClusterRevision */                          \
                                                                                                                                      \
			/* Endpoint: 0, Cluster: Group Key Management (server) */                                                     \
			{ ZAP_EMPTY_DEFAULT(), 0x00000000, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(WRITABLE) }, /* GroupKeyMap */                    \
			{ ZAP_EMPTY_DEFAULT(), 0x00000001, 0, ZAP_TYPE(ARRAY),                                                        \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* GroupTable */                                                    \
			{ ZAP_EMPTY_DEFAULT(), 0x00000002, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* MaxGroupsPerFabric */                                            \
			{ ZAP_EMPTY_DEFAULT(), 0x00000003, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* MaxGroupKeysPerFabric */                                         \
			{ ZAP_EMPTY_DEFAULT(), 0x0000FFFC, 4, ZAP_TYPE(BITMAP32),                                                     \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* FeatureMap */                                                    \
			{ ZAP_EMPTY_DEFAULT(), 0x0000FFFD, 2, ZAP_TYPE(INT16U),                                                       \
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) }, /* ClusterRevision */                                               \
	}

// clang-format off
#define GENERATED_EVENT_COUNT 6
#define GENERATED_EVENTS { \
  /* Endpoint: 0, Cluster: Basic Information (server) */ \
  /* EventList (index=0) */ \
  0x00000000, /* StartUp */ \
  0x00000001, /* ShutDown */ \
  0x00000002, /* Leave */ \
  /* Endpoint: 0, Cluster: OTA Software Update Requestor (server) */ \
  /* EventList (index=3) */ \
  0x00000000, /* StateTransition */ \
  0x00000001, /* VersionApplied */ \
  0x00000002, /* DownloadError */ \
}

// clang-format on

// Cluster function static arrays
#define GENERATED_FUNCTION_ARRAYS

// clang-format off
#define GENERATED_COMMANDS { \
  /* Endpoint: 0, Cluster: OTA Software Update Requestor (server) */\
  /*   AcceptedCommandList (index=0) */ \
  0x00000000 /* AnnounceOTAProvider */, \
  chip::kInvalidCommandId /* end of list */, \
  /* Endpoint: 0, Cluster: General Commissioning (server) */\
  /*   AcceptedCommandList (index=2) */ \
  0x00000000 /* ArmFailSafe */, \
  0x00000002 /* SetRegulatoryConfig */, \
  0x00000004 /* CommissioningComplete */, \
  chip::kInvalidCommandId /* end of list */, \
  /*   GeneratedCommandList (index=6)*/ \
  0x00000001 /* ArmFailSafeResponse */, \
  0x00000003 /* SetRegulatoryConfigResponse */, \
  0x00000005 /* CommissioningCompleteResponse */, \
  chip::kInvalidCommandId /* end of list */, \
  /* Endpoint: 0, Cluster: Network Commissioning (server) */\
  /*   AcceptedCommandList (index=10) */ \
  0x00000000 /* ScanNetworks */, \
  0x00000002 /* AddOrUpdateWiFiNetwork */, \
  0x00000003 /* AddOrUpdateThreadNetwork */, \
  0x00000004 /* RemoveNetwork */, \
  0x00000006 /* ConnectNetwork */, \
  0x00000008 /* ReorderNetwork */, \
  chip::kInvalidCommandId /* end of list */, \
  /*   GeneratedCommandList (index=17)*/ \
  0x00000001 /* ScanNetworksResponse */, \
  0x00000005 /* NetworkConfigResponse */, \
  0x00000007 /* ConnectNetworkResponse */, \
  chip::kInvalidCommandId /* end of list */, \
  /* Endpoint: 0, Cluster: General Diagnostics (server) */\
  /*   AcceptedCommandList (index=21) */ \
  0x00000000 /* TestEventTrigger */, \
  0x00000001 /* TimeSnapshot */, \
  chip::kInvalidCommandId /* end of list */, \
  /*   GeneratedCommandList (index=24)*/ \
  0x00000002 /* TimeSnapshotResponse */, \
  chip::kInvalidCommandId /* end of list */, \
  /* Endpoint: 0, Cluster: Administrator Commissioning (server) */\
  /*   AcceptedCommandList (index=26) */ \
  0x00000000 /* OpenCommissioningWindow */, \
  0x00000001 /* OpenBasicCommissioningWindow */, \
  0x00000002 /* RevokeCommissioning */, \
  chip::kInvalidCommandId /* end of list */, \
  /* Endpoint: 0, Cluster: Operational Credentials (server) */\
  /*   AcceptedCommandList (index=30) */ \
  0x00000000 /* AttestationRequest */, \
  0x00000002 /* CertificateChainRequest */, \
  0x00000004 /* CSRRequest */, \
  0x00000006 /* AddNOC */, \
  0x00000007 /* UpdateNOC */, \
  0x00000009 /* UpdateFabricLabel */, \
  0x0000000A /* RemoveFabric */, \
  0x0000000B /* AddTrustedRootCertificate */, \
  chip::kInvalidCommandId /* end of list */, \
  /*   GeneratedCommandList (index=39)*/ \
  0x00000001 /* AttestationResponse */, \
  0x00000003 /* CertificateChainResponse */, \
  0x00000005 /* CSRResponse */, \
  0x00000008 /* NOCResponse */, \
  chip::kInvalidCommandId /* end of list */, \
  /* Endpoint: 0, Cluster: Group Key Management (server) */\
  /*   AcceptedCommandList (index=44) */ \
  0x00000000 /* KeySetWrite */, \
  0x00000001 /* KeySetRead */, \
  0x00000003 /* KeySetRemove */, \
  0x00000004 /* KeySetReadAllIndices */, \
  chip::kInvalidCommandId /* end of list */, \
  /*   GeneratedCommandList (index=49)*/ \
  0x00000002 /* KeySetReadResponse */, \
  0x00000005 /* KeySetReadAllIndicesResponse */, \
  chip::kInvalidCommandId /* end of list */, \
}

// clang-format on

// This is an array of EmberAfCluster structures.
#define GENERATED_CLUSTER_COUNT 11
// clang-format off
#define GENERATED_CLUSTERS { \
  { \
      /* Endpoint: 0, Cluster: Descriptor (server) */ \
      .clusterId = 0x0000001D, \
      .attributes = ZAP_ATTRIBUTE_INDEX(0), \
      .attributeCount = 6, \
      .clusterSize = 0, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .acceptedCommandList = nullptr, \
      .generatedCommandList = nullptr, \
      .eventList = nullptr, \
      .eventCount = 0, \
    },\
  { \
      /* Endpoint: 0, Cluster: Access Control (server) */ \
      .clusterId = 0x0000001F, \
      .attributes = ZAP_ATTRIBUTE_INDEX(6), \
      .attributeCount = 7, \
      .clusterSize = 4, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .acceptedCommandList = nullptr, \
      .generatedCommandList = nullptr, \
      .eventList = nullptr, \
      .eventCount = 0, \
    },\
  { \
      /* Endpoint: 0, Cluster: Basic Information (server) */ \
      .clusterId = 0x00000028, \
      .attributes = ZAP_ATTRIBUTE_INDEX(13), \
      .attributeCount = 19, \
      .clusterSize = 39, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .acceptedCommandList = nullptr, \
      .generatedCommandList = nullptr, \
      .eventList = ZAP_GENERATED_EVENTS_INDEX( 0 ), \
      .eventCount = 3, \
    },\
  { \
      /* Endpoint: 0, Cluster: OTA Software Update Provider (client) */ \
      .clusterId = 0x00000029, \
      .attributes = ZAP_ATTRIBUTE_INDEX(32), \
      .attributeCount = 0, \
      .clusterSize = 0, \
      .mask = ZAP_CLUSTER_MASK(CLIENT), \
      .functions = NULL, \
      .acceptedCommandList = nullptr, \
      .generatedCommandList = nullptr, \
      .eventList = nullptr, \
      .eventCount = 0, \
    },\
  { \
      /* Endpoint: 0, Cluster: OTA Software Update Requestor (server) */ \
      .clusterId = 0x0000002A, \
      .attributes = ZAP_ATTRIBUTE_INDEX(32), \
      .attributeCount = 6, \
      .clusterSize = 9, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .acceptedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 0 ), \
      .generatedCommandList = nullptr, \
      .eventList = ZAP_GENERATED_EVENTS_INDEX( 3 ), \
      .eventCount = 3, \
    },\
  { \
      /* Endpoint: 0, Cluster: General Commissioning (server) */ \
      .clusterId = 0x00000030, \
      .attributes = ZAP_ATTRIBUTE_INDEX(38), \
      .attributeCount = 7, \
      .clusterSize = 14, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .acceptedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 2 ), \
      .generatedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 6 ), \
      .eventList = nullptr, \
      .eventCount = 0, \
    },\
  { \
      /* Endpoint: 0, Cluster: Network Commissioning (server) */ \
      .clusterId = 0x00000031, \
      .attributes = ZAP_ATTRIBUTE_INDEX(45), \
      .attributeCount = 13, \
      .clusterSize = 6, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .acceptedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 10 ), \
      .generatedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 17 ), \
      .eventList = nullptr, \
      .eventCount = 0, \
    },\
  { \
      /* Endpoint: 0, Cluster: General Diagnostics (server) */ \
      .clusterId = 0x00000033, \
      .attributes = ZAP_ATTRIBUTE_INDEX(58), \
      .attributeCount = 8, \
      .clusterSize = 0, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .acceptedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 21 ), \
      .generatedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 24 ), \
      .eventList = nullptr, \
      .eventCount = 0, \
    },\
  { \
      /* Endpoint: 0, Cluster: Administrator Commissioning (server) */ \
      .clusterId = 0x0000003C, \
      .attributes = ZAP_ATTRIBUTE_INDEX(66), \
      .attributeCount = 5, \
      .clusterSize = 6, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .acceptedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 26 ), \
      .generatedCommandList = nullptr, \
      .eventList = nullptr, \
      .eventCount = 0, \
    },\
  { \
      /* Endpoint: 0, Cluster: Operational Credentials (server) */ \
      .clusterId = 0x0000003E, \
      .attributes = ZAP_ATTRIBUTE_INDEX(71), \
      .attributeCount = 8, \
      .clusterSize = 6, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .acceptedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 30 ), \
      .generatedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 39 ), \
      .eventList = nullptr, \
      .eventCount = 0, \
    },\
  { \
      /* Endpoint: 0, Cluster: Group Key Management (server) */ \
      .clusterId = 0x0000003F, \
      .attributes = ZAP_ATTRIBUTE_INDEX(79), \
      .attributeCount = 6, \
      .clusterSize = 0, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .acceptedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 44 ), \
      .generatedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 49 ), \
      .eventList = nullptr, \
      .eventCount = 0, \
    },\
}

// clang-format on

#define ZAP_FIXED_ENDPOINT_DATA_VERSION_COUNT 10

// This is an array of EmberAfEndpointType structures.
#define GENERATED_ENDPOINT_TYPES                                                                                       \
	{                                                                                                              \
		{ ZAP_CLUSTER_INDEX(0), 11, 84 },                                                                      \
	}

// Largest attribute size is needed for various buffers
#define ATTRIBUTE_LARGEST (66)

static_assert(ATTRIBUTE_LARGEST <= CHIP_CONFIG_MAX_ATTRIBUTE_STORE_ELEMENT_SIZE,
	      "ATTRIBUTE_LARGEST larger than expected");

// Total size of singleton attributes
#define ATTRIBUTE_SINGLETONS_SIZE (35)

// Total size of attribute storage
#define ATTRIBUTE_MAX_SIZE (84)

// Number of fixed endpoints
#define FIXED_ENDPOINT_COUNT (1)

// Array of endpoints that are supported, the data inside
// the array is the endpoint number.
#define FIXED_ENDPOINT_ARRAY                                                                                           \
	{                                                                                                              \
		0x0000                                                                                                 \
	}

// Array of profile ids
#define FIXED_PROFILE_IDS                                                                                              \
	{                                                                                                              \
		0x0103                                                                                                 \
	}

// Array of device types
#define FIXED_DEVICE_TYPES                                                                                             \
	{                                                                                                              \
		{                                                                                                      \
			0x00000016, 3                                                                                  \
		}                                                                                                      \
	}

// Array of device type offsets
#define FIXED_DEVICE_TYPE_OFFSETS                                                                                      \
	{                                                                                                              \
		0                                                                                                      \
	}

// Array of device type lengths
#define FIXED_DEVICE_TYPE_LENGTHS                                                                                      \
	{                                                                                                              \
		1                                                                                                      \
	}

// Array of endpoint types supported on each endpoint
#define FIXED_ENDPOINT_TYPES                                                                                           \
	{                                                                                                              \
		0                                                                                                      \
	}

// Array of parent endpoints for each endpoint
#define FIXED_PARENT_ENDPOINTS                                                                                         \
	{                                                                                                              \
		kInvalidEndpointId                                                                                     \
	}
