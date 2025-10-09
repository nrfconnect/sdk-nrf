.. _ot_rpc:

OpenThread Remote Procedure Call
################################

.. contents::
   :local:
   :depth: 2

The OpenThread Remote Procedure Call (RPC) solution is a set of libraries that enable controlling the :ref:`ug_thread` stack running on a remote device or processor.

Overview
********

The solution allows calling the OpenThread API on one device to control the OpenThread stack running on another device.
This is accomplished by serializing the API call and sending it to the remote device over a selected transport.
Use this solution when you cannot or do not want to include the OpenThread stack in your application firmware, for example to offload the application CPU, save memory, or optimize the power consumption.

Implementation
==============

The OpenThread RPC consists of the following libraries:

  * OpenThread RPC Client, which includes the OpenThread RPC common library.
    This library serializes the OpenThread API using the :ref:`nrf_rpc` library.
    OpenThread RPC Client has to be part of the user application.
  * OpenThread RPC Server, which includes the OpenThread RPC common library and the OpenThread stack.
    This library enables communication with OpenThread RPC Client, and has to run on a device that has an 802.15.4 radio hardware peripheral.

Configuration
*************

These Kconfig options must be enabled to use OpenThread RPC:

  * :kconfig:option:`CONFIG_OPENTHREAD_RPC`
  * :kconfig:option:`CONFIG_OPENTHREAD_RPC_CLIENT` (for the user application with OpenThread RPC Client)
  * :kconfig:option:`CONFIG_OPENTHREAD_RPC_SERVER` (for the device that has an 802.15.4 radio hardware peripheral and OpenThread RPC Server)

These Kconfig options must additionally be enabled for the server:

  * :kconfig:option:`CONFIG_NETWORKING`
  * :kconfig:option:`CONFIG_NET_L2_OPENTHREAD`
  * :kconfig:option:`CONFIG_OPENTHREAD_MANUAL_START`

Read :ref:`ug_thread_configuring` to learn more about configuring the OpenThread stack.

The :kconfig:option:`CONFIG_OPENTHREAD_RPC_NET_IF` Kconfig option enables the network interface on the client, which forwards and receives IPv6 packets to and from the server.
This option must be set to the same value on both the client and server, and is enabled by default.

Samples using the library
*************************

The following |NCS| samples use this library:

* :ref:`nrf_rpc_protocols_serialization_client`
* :ref:`nrf_rpc_protocols_serialization_server`

Limitations
***********

OpenThread RPC currently supports the serialization of the following OpenThread functions:

* :c:func:`otCliInit`
* :c:func:`otCliInputLine`
* :c:func:`otCoapAddResource`
* :c:func:`otCoapMessageAppendUriPathOptions`
* :c:func:`otCoapMessageGetCode`
* :c:func:`otCoapMessageGetMessageId`
* :c:func:`otCoapMessageGetToken`
* :c:func:`otCoapMessageGetTokenLength`
* :c:func:`otCoapMessageGetType`
* :c:func:`otCoapMessageInit`
* :c:func:`otCoapMessageInitResponse`
* :c:func:`otCoapMessageSetPayloadMarker`
* :c:func:`otCoapNewMessage`
* :c:func:`otCoapRemoveResource`
* :c:func:`otCoapSendRequest`
* :c:func:`otCoapSendRequestWithParameters`
* :c:func:`otCoapSendResponse`
* :c:func:`otCoapSendResponseWithParameters`
* :c:func:`otCoapSetDefaultHandler`
* :c:func:`otCoapStart`
* :c:func:`otCoapStop`
* :c:func:`otDatasetGetActive`
* :c:func:`otDatasetGetActiveTlvs`
* :c:func:`otDatasetGetPending`
* :c:func:`otDatasetGetPendingTlvs`
* :c:func:`otDatasetIsCommissioned`
* :c:func:`otDatasetSetActive`
* :c:func:`otDatasetSetActiveTlvs`
* :c:func:`otDatasetSetPending`
* :c:func:`otDatasetSetPendingTlvs`
* :c:func:`otDnsAddressResponseGetAddress`
* :c:func:`otDnsAddressResponseGetHostName`
* :c:func:`otDnsBrowseResponseGetHostAddress`
* :c:func:`otDnsBrowseResponseGetServiceInfo`
* :c:func:`otDnsBrowseResponseGetServiceInstance`
* :c:func:`otDnsBrowseResponseGetServiceName`
* :c:func:`otDnsClientBrowse`
* :c:func:`otDnsClientGetDefaultConfig`
* :c:func:`otDnsClientResolveAddress`
* :c:func:`otDnsClientResolveIp4Address`
* :c:func:`otDnsClientResolveService`
* :c:func:`otDnsClientResolveServiceAndHostAddress`
* :c:func:`otDnsClientSetDefaultConfig`
* :c:func:`otDnsServiceResponseGetHostAddress`
* :c:func:`otDnsServiceResponseGetServiceInfo`
* :c:func:`otDnsServiceResponseGetServiceName`
* :c:func:`otGetVersionString`
* :c:func:`otInstanceErasePersistentInfo`
* :c:func:`otInstanceFinalize`
* :c:func:`otInstanceGetId`
* :c:func:`otInstanceInitSingle`
* :c:func:`otInstanceIsInitialized`
* :c:func:`otIp6GetMulticastAddresses`
* :c:func:`otIp6GetUnicastAddresses`
* :c:func:`otIp6AddressToString`
* :c:func:`otIp6IsEnabled`
* :c:func:`otIp6SetEnabled`
* :c:func:`otIp6SubscribeMulticastAddress`
* :c:func:`otIp6UnsubscribeMulticastAddress`
* :c:func:`otLinkGetChannel`
* :c:func:`otLinkGetCounters`
* :c:func:`otLinkGetExtendedAddress`
* :c:func:`otLinkGetFactoryAssignedIeeeEui64`
* :c:func:`otLinkGetPanId`
* :c:func:`otLinkGetPollPeriod`
* :c:func:`otLinkRawGetRadioTime`
* :c:func:`otLinkSetEnabled`
* :c:func:`otLinkSetMaxFrameRetriesDirect`
* :c:func:`otLinkSetMaxFrameRetriesIndirect`
* :c:func:`otLinkSetPollPeriod`
* :c:func:`otMeshDiagCancel`
* :c:func:`otMeshDiagDiscoverTopology`
* :c:func:`otMeshDiagGetNextChildInfo`
* :c:func:`otMeshDiagGetNextIp6Address`
* :c:func:`otMessageAppend`
* :c:func:`otMessageFree`
* :c:func:`otMessageGetLength`
* :c:func:`otMessageGetOffset`
* :c:func:`otMessageGetThreadLinkInfo`
* :c:func:`otMessageRead`
* :c:func:`otNetDataGet`
* :c:func:`otNetDataGetNextOnMeshPrefix`
* :c:func:`otNetDataGetNextService`
* :c:func:`otNetDataGetStableVersion`
* :c:func:`otNetDataGetVersion`
* :c:func:`otRemoveStateChangeCallback`
* :c:func:`otSetStateChangedCallback`
* :c:func:`otSrpClientAddService`
* :c:func:`otSrpClientClearHostAndServices`
* :c:func:`otSrpClientClearService`
* :c:func:`otSrpClientDisableAutoStartMode`
* :c:func:`otSrpClientEnableAutoHostAddress`
* :c:func:`otSrpClientEnableAutoStartMode`
* :c:func:`otSrpClientGetHostInfo`
* :c:func:`otSrpClientGetKeyLeaseInterval`
* :c:func:`otSrpClientGetLeaseInterval`
* :c:func:`otSrpClientGetServerAddress`
* :c:func:`otSrpClientGetServices`
* :c:func:`otSrpClientGetTtl`
* :c:func:`otSrpClientIsAutoStartModeEnabled`
* :c:func:`otSrpClientIsRunning`
* :c:func:`otSrpClientItemStateToString`
* :c:func:`otSrpClientRemoveHostAndServices`
* :c:func:`otSrpClientRemoveService`
* :c:func:`otSrpClientSetCallback`
* :c:func:`otSrpClientSetHostAddresses`
* :c:func:`otSrpClientSetHostName`
* :c:func:`otSrpClientSetKeyLeaseInterval`
* :c:func:`otSrpClientSetLeaseInterval`
* :c:func:`otSrpClientSetTtl`
* :c:func:`otSrpClientStart`
* :c:func:`otSrpClientStop`
* :c:func:`otThreadDiscover`
* :c:func:`otThreadErrorToString`
* :c:func:`otThreadGetDeviceRole`
* :c:func:`otThreadGetExtendedPanId`
* :c:func:`otThreadGetLeaderRouterId`
* :c:func:`otThreadGetLeaderWeight`
* :c:func:`otThreadGetLinkMode`
* :c:func:`otThreadGetMeshLocalPrefix`
* :c:func:`otThreadGetMleCounters`
* :c:func:`otThreadGetNetworkName`
* :c:func:`otThreadGetNextDiagnosticTlv`
* :c:func:`otThreadGetPartitionId`
* :c:func:`otThreadGetRloc16`
* :c:func:`otThreadGetVendorModel`
* :c:func:`otThreadGetVendorName`
* :c:func:`otThreadGetVendorSwVersion`
* :c:func:`otThreadGetVersion`
* :c:func:`otThreadSendDiagnosticGet`
* :c:func:`otThreadSendDiagnosticReset`
* :c:func:`otThreadSetEnabled`
* :c:func:`otThreadSetLinkMode`
* :c:func:`otThreadSetVendorModel`
* :c:func:`otThreadSetVendorName`
* :c:func:`otThreadSetVendorSwVersion`
* :c:func:`otUdpBind`
* :c:func:`otUdpClose`
* :c:func:`otUdpConnect`
* :c:func:`otUdpIsOpen`
* :c:func:`otUdpNewMessage`
* :c:func:`otUdpOpen`
* :c:func:`otUdpSend`

The libraries will be extended to support more functions in the future.

Dependencies
************

The libraries have the following dependencies:

  * :ref:`nrf_rpc`
  * :ref:`ug_thread`

.. _ot_rpc_api:

API documentation
*****************

This library is an implementation of the existing OpenThread C API.
Please refer to `OpenThread Reference`_ for the OpenThread C API documentation.

In addition, the library also implements some API additions.

| Header file: :file:`include/net/ot_rpc.h`
| Source files: :file:`subsys/net/openthread/rpc`

.. doxygengroup:: ot_rpc
