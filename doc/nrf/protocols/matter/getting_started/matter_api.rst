.. _ug_matter_gs_matter_api:

Matter APIs
###########

.. contents::
   :local:
   :depth: 2

This section describes the Application Programming Interfaces (APIs) that allow the developer to interact with core components of the Matter stack.
It covers the initialization of Matter modules, user callback registration, and the interaction with Matter Data Model.

.. _ug_matter_gs_matter_api_overview:

Overview
********

The Matter SDK is the software library that constitutes the reference implementation of the Matter protocol specification.
There is no one unified, documented Matter SDK API that can be referenced when developing a custom application.
Instead, to learn how to interact with the Matter library, a Matter firmware developer must peruse the source code of any of the existing Matter sample applications.
To aid developers, Nordic Semiconductor provides a unified API that wraps initialization of Matter-specific components into more user-friendly high level code.

The Matter application code in the |NCS| can be divided into the following steps:

1. Initialization of application-specific components.
   This includes initialization of hardware modules and registration of proprietary Bluetooth® LE services.
#. Initialization of the Matter stack.
#. Starting the application main event loop.
#. Interaction between the application and the Matter Data Model.
   This is based on the Zigbee Cluster Library (ZCL) callbacks.

Steps 1 to 3 can be implemented with the use of the nRF Connect Matter API and the utilities provided as a part of |NCS| Matter samples' ``common`` modules (:file:`ncs/nrf/samples/matter/common`).
Step 4 requires ZCL callback functions that must be provided to interact with the Matter Data Model.
These callbacks are specific to the particular configuration of the Data Model (in other words, the supported clusters) and therefore cannot be generalized or abstracted in a more user-friendly form.

.. note::
   Before the interaction between the application and the Matter Data Model is possible, the source files of the Data Model must be first generated with the `ZCL Advanced Platform`_ (ZAP tool).
   Please refer to the :ref:`ug_matter_gs_adding_cluster` for detailed description on how to configure and generate the Matter Data model for your application.

nRF Connect Matter API
**********************

The nRF Connect Matter API provides:

* Initialization API: A set of functions and structures to initialize and manage a Matter server within a Matter application.
* Event handler API: Mechanisms to register and unregister custom functions that handle public events propagated from the Matter stack to the application layer.

Initialization API
==================

The initialization API allows developers to configure Matter stack core components, event handlers, and networking backends.

The following structure collects all Matter interfaces that need to be initialized before starting the Matter Server:

.. code-block:: C++

   using CustomInit = CHIP_ERROR (*)(void);

   /** @brief Matter initialization data.
    *
    * This structure contains all user specific implementations of Matter interfaces
    * and custom initialization callbacks that must be initialized in the Matter thread.
    */
   struct InitData {
         /** @brief Matter stack events handler. */
         chip::DeviceLayer::PlatformManager::EventHandlerFunct mEventHandler{ DefaultEventHandler };
         /** @brief Pointer to the user provided NetworkCommissioning instance. */
   #ifdef CONFIG_CHIP_WIFI
         chip::app::Clusters::NetworkCommissioning::Instance *mNetworkingInstance{ &sWiFiCommissioningInstance };
   #else
         chip::app::Clusters::NetworkCommissioning::Instance *mNetworkingInstance{ nullptr };
   #endif
         /** @brief Pointer to the user provided custom server initialization parameters. */
         chip::CommonCaseDeviceServerInitParams *mServerInitParams{ &sServerInitParamsDefault };
         /** @brief Pointer to the user provided custom device info provider implementation. */
         chip::DeviceLayer::DeviceInfoProviderImpl *mDeviceInfoProvider{ nullptr };
   #ifdef CONFIG_CHIP_FACTORY_DATA
         /** @brief Pointer to the user provided FactoryDataProvider implementation. */
         chip::DeviceLayer::FactoryDataProviderBase *mFactoryDataProvider{ &sFactoryDataProviderDefault };
   #endif
   #ifdef CONFIG_CHIP_CRYPTO_PSA
         /** @brief Pointer to the user provided OperationalKeystore implementation. */
         chip::Crypto::OperationalKeystore *mOperationalKeyStore{ &sOperationalKeystoreDefault };
   #endif
         /** @brief Custom code to execute in the Matter main event loop before the server initialization. */
         CustomInit mPreServerInitClbk{ nullptr };
         /** @brief Custom code to execute in the Matter main event loop after the server initialization. */
         CustomInit mPostServerInitClbk{ nullptr };

         /** @brief Default implementation static objects that will be stripped by the compiler when above
          * pointers are overwritten by the application. */
   #ifdef CONFIG_CHIP_WIFI
         static chip::app::Clusters::NetworkCommissioning::Instance sWiFiCommissioningInstance;
   #endif
         static chip::CommonCaseDeviceServerInitParams sServerInitParamsDefault;
   #ifdef CONFIG_CHIP_FACTORY_DATA
         static chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> sFactoryDataProviderDefault;
   #endif
   #ifdef CONFIG_CHIP_CRYPTO_PSA
         static chip::Crypto::PSAOperationalKeystore sOperationalKeystoreDefault;
   #endif
   };

In this structure, each Matter component is defined as a public pointer to its generic interface.
Each pointer is initially assigned with the default concrete implementation of the given interface.
The default implementation object is stripped by the compiler if the user overwrites it with a customized implementation in the application.

The nRF Connect Matter API contains the following functions that can be used to initialize Matter components in proper order:

:c:func:`PrepareServer()`:
  This function schedules the initialization of Matter components, including memory, server configuration and networking backend.
  Depending on the selected Kconfig options, the initialization may also include factory data and operational key storage.
  All initialization procedures are scheduled to the Matter thread to provide a synchronization between all components and the application code.

  This function accepts an :c:struct:`InitData` argument that contains the implementation of all required Matter interfaces.
  If no argument is provided, this function uses the default-constructed :c:struct:`InitData` temporary object.
  After this function is used, the :c:func:`StartServer()` function must be called to start the Matter thread, eventually execute the initialization, and wait to synchronize the caller's thread with the Matter thread.

:c:func:`StartServer()`:
  This is a blocking function that starts the Matter thread and waits until all Matter server components are initialized.

:c:func:`GetFactoryDataProvider()`:
  This function returns the generic pointer to the ``FactoryDataProvider`` object that was set during the initialization.
  It can be used when you need to access factory data at the Matter server initialization stage or as a part of the post initialization callback (``mPostServerInitClbk`` in :c:struct:`InitData`).

  This function is only available if the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA` Kconfig option is selected.

For more details regarding nRF Connect Matter initialization API, refer to the Doxygen commentary in the :file:`ncs/nrf/samples/matter/common/src/app/matter_init.h` header file.

Event handler API
=================

The Matter SDK provides a notification scheme based on the public events that are propagated from the Matter stack to the application layer.
The nRF Connect Matter event handler API provides mechanisms to register and unregister custom functions that handle these events within an application.
This module also includes a default handler that is used in nRF Connect SDK Matter samples and applications.

The specific Matter events that can be handled in the application are listed in the :file:`ncs/modules/lib/matter/src/include/platform/CHIPDeviceEvent.h` header file.
The nRF Connect Matter API contains of the following functions that can be used to handle events:

:c:func:`RegisterEventHandler()`:
  This function is used to register the provided Matter event handler(``EventHandlerFunct``) in a thread-safe manner.
  It is safe to call this function in the application after the Matter server has already been initialized.

:c:func:`UnregisterEventHandler()`:
  This function is used to unregister the provided Matter event handler(``EventHandlerFunct``) in a thread-safe manner.
  It is safe to call this function in the application after the Matter server has already been initialized.

:c:func:`DefaultEventHandler()`:
  This is an nRF Connect Matter event handler function that is registered in the nRF Connect Matter Initialization API by default.
  You can unregister this handler with the :c:func:`UnregisterEventHandler()` function in the application if needed.

For more details regarding nRF Connect Matter event handler API, refer to the Doxygen commentary in the :file:`ncs/nrf/samples/matter/common/src/app/matter_event_handler.h` header file.

nRF Connect Matter API usage example
====================================

Combining both aforementioned nRF Connect Matter APIs, you can develop an application by following the initialization scheme listed below:

.. code-block:: C++

   #include "app/matter_init.h"

   #include <zephyr/logging/log.h>

   LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

   static void CustomMatterEventHandler(const ChipDeviceEvent *event, intptr_t /* unused */)
   {
         switch (event->Type) {
         case DeviceEventType::kCommissioningComplete:
             /* Custom code, e.g. control LED */
             break;
         }
   }

   CHIP_ERROR MatterAppInit()
   {
         /* Initialize Matter stack */
         ReturnErrorOnFailure(Nrf::Matter::PrepareServer(Nrf::Matter::InitData{ .mPostServerInitClbk = [] {
                LOG_INF("Matter server has been initialized.");
                return CHIP_NO_ERROR;
         } }));

         /* Register custom Matter event handler. */
         ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(CustomMatterEventHandler, 0));

         /* Application specific initialization, e.g. hardware initialization.

            ...
         */

         /* Start Matter thread that will run the scheduled initialization procedure. */
         return Nrf::Matter::StartServer();
   }

Note that the ``PrepareServer()`` call may contain more fields of the :c:struct:`InitData` being initialized, or can be called without any explicit argument.
If there is no explicit argument, the default initialization will be provided.
For more references and examples on how to leverage the nRF Connect Matter APIs, examine the source code for the :ref:`matter_samples` in the |NCS|.

Interacting with Matter Data Model
**********************************

The Matter SDK Data Model interacs with the user's code based on callbacks that can be implemented by the application.
The generic callbacks that are common for Matter applications, regardless of the clusters configuration, are defined in the :file:`ncs/modules/lib/matter/src/app/util/generic-callbacks.h` header file.
The weak implementations of these functions, that can be overwritten in the application, are provided in the :file:`ncs/modules/lib/matter/src/app/util/generic-callback-stubs.cpp` source file.

For example, the :c:func:`MatterPostAttributeChangeCallback()` function is called by the Matter Data Model engine directly after an attribute value is changed.
The value passed into this callback is the value to which the attribute was set by the framework.
In addition to the value, this function is called with the attribute path ( of ``chip::app::ConcreteAttributePath`` type) that can be used to filter the cluster and particular attribute.
The :c:func:`MatterPostAttributeChangeCallback()` function is useful if you need to provide the synchronization between the Data Model and the application state.
For instance, a Matter device that implements a light bulb may drive the state of the LED based on the ``On/Off`` attribute value.
Every change of this attribute is reported by the aforementioned callback and thus can be captured in the application layer.

In addition to the :c:func:`MatterPostAttributeChangeCallback()` function, Matter defines other generic callbacks that can be employed in different use cases.
For example, the :c:func:`emberAfExternalAttributeReadCallback()` and :c:func:`emberAfExternalAttributeWriteCallback()` functions can be used to store and handle attributes externally, by bypassing the Matter Data Model framework.
To learn the complete set of Matter generic callbacks, refer to the :file:`ncs/modules/lib/matter/src/app/util/generic-callbacks.h` header file and included Doxygen commentary.
An example implementation of the :c:func:`MatterPostAttributeChangeCallback()` that can be used to control the Door Lock Matter device type is listed below:

.. code-block:: C++

   #include <app-common/zap-generated/ids/Clusters.h>
   #include <app/ConcreteAttributePath.h>
   #include <app/util/generic-callbacks.h>

   using namespace ::chip;
   using namespace ::chip::app::Clusters;

   void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t type,
                   uint16_t size, uint8_t *value)
   {
         if (attributePath.mClusterId == DoorLock::Id &&
             attributePath.mAttributeId == DoorLock::Attributes::LockState::Id) {
           /* Post events only if current lock state is different than given */
           switch (*value) {
           case to_underlying(DlLockState::kLocked):
                 /* Lock the physical door lock. */
                 break;
           case to_underlying(DlLockState::kUnlocked):
               /* Unlock the physical door lock. */
                 break;
           default:
                 break;
           }
         }
   }

In addition to Matter generic callbacks, the Matter Data Model engine provides callbacks that are cluster-specific.
These callbacks are usually defined as weak functions in the :file:`callback-stub.cpp` file that is generated together with other C++ source files when configuring clusters automatically with the `ZCL Advanced Platform`_ (ZAP tool).
In case of some clusters, however, a different approach is used and related callbacks are defined within the source code that constitutes the implementation of the cluster itself.
As an example, the ``DoorLock`` cluster server implementation defines application callbacks in the :file:`ncs/modules/lib/matter/src/app/clusters/door-lock-server/door-lock-server.h` header file and places the related weak implementations in the :file:`ncs/modules/lib/matter/src/app/clusters/door-lock-server/door-lock-server-callback.cpp` file.

.. note::
   Most of the Matter Data Model callback function names are prefixed with ``emberAf``.
   The reason for this is the fact that the Matter Data Model inherits extensively from the Zigbee Ember Application Framework API.

In the |NCS|, all Matter samples follow the same convention and implement the described Matter Data Model callbacks in the :file:`zcl_callbacks.cpp` files which are populated as a part of the application source code.
You can review the :file:`zcl_callbacks.cpp` file of any |NCS| Matter sample to find example implementations of various Data Model callbacks.
For instance, you can find the reference implementation of ``DoorLock``-specific Matter Data Model callbacks in the :file:`ncs/nrf/samples/matter/lock/src/zcl_callbacks.cpp` source file.
