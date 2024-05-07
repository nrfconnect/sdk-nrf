.. _migration_2.6:

Migration guide for |NCS| v2.6.0
################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v2.5.0 to |NCS| v2.6.0.

.. HOWTO

.. Add changes in the following format:

.. Component (for example, application, sample or libraries)
.. *********************************************************
..
.. * Change1 and description
.. * Change2 and description

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Samples and applications
========================

This section describes the changes related to samples and applications.

Serial LTE Modem (SLM)
----------------------

.. toggle::

  * The Zephyr settings backend has been changed from :ref:`FCB <fcb_api>` to :ref:`NVS <nvs_api>`.
    Because of this, one setting is restored to its default value after the switch if you are using the :ref:`liblwm2m_carrier_readme` library.
    The setting controls whether the SLM connects automatically to the network on startup.
    You can read and write it using the ``AT#XCARRIER="auto_connect"`` command.

  * The ``AT#XCMNG`` AT command, which is activated with the :file:`overlay-native_tls.conf` overlay file, has been changed from using modem certificate storage to Zephyr settings storage.
    You need to use the ``AT#XCMNG`` command to store previously stored credentials again.
  * The ``CONFIG_SLM_WAKEUP_PIN`` Kconfig option was renamed to :ref:`CONFIG_SLM_POWER_PIN <CONFIG_SLM_POWER_PIN>`.
    If you have defined it separately from the default configurations, you need to update its name accordingly.

Matter
------

.. toggle::

   * For the Matter samples and applications using Intermittently Connected Devices configuration (formerly called Sleepy End Devices):

     * The naming convention for the energy-optimized devices has been changed from Sleepy End Devices (SED) to Intermittently Connected Devices (ICD).
       Because of this, the Kconfig options used to manage this configuration have been aligned as well.
       If your application uses the following Kconfig options, they require name changes:

         * The ``CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT`` Kconfig option was renamed to :kconfig:option:`CONFIG_CHIP_ENABLE_ICD_SUPPORT`.
         * The ``CONFIG_CHIP_SED_IDLE_INTERVAL`` Kconfig option was renamed to :kconfig:option:`CONFIG_CHIP_ICD_SLOW_POLL_INTERVAL`.
         * The ``CONFIG_CHIP_SED_ACTIVE_INTERVAL`` Kconfig option was renamed to :kconfig:option:`CONFIG_CHIP_ICD_FAST_POLLING_INTERVAL`.
         * The ``CONFIG_CHIP_SED_ACTIVE_THRESHOLD`` Kconfig option was renamed to :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_THRESHOLD`.

   * For Matter over Thread samples, starting from this release, the cryptography backend enabled by default is PSA Crypto API instead of Mbed TLS.
     Be aware of the change and consider the following when migrating to |NCS| v2.6.0:

     * You can keep using Mbed TLS API as the cryptography backend by disabling PSA Crypto API.
       You can disable it by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA` Kconfig option to ``n``.
     * Thread libraries will be built with PSA Crypto API enabled without Mbed TLS support.
       This means that if you set the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA` Kconfig option to ``n``, you must also build the Thread libraries from sources.

       To :ref:`inherit Thread certification <ug_matter_device_certification_reqs_dependent>` from Nordic Semiconductor, you must use the PSA Crypto API backend.
     * The device can automatically migrate all operational keys from the Matter's generic persistent storage to the PSA ITS secure storage.
       This means that all keys needed to establish the secure connection between Matter nodes will be moved to the PSA ITS secure storage.
       To enable operational keys migration, set the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_OPERATIONAL_KEYS_MIGRATION_TO_ITS` Kconfig option to ``y``.

       The default reaction to migration failure in |NCS| Matter samples is a factory reset of the device.
       To change the default reaction, set the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_FACTORY_RESET_ON_KEY_MIGRATION_FAILURE` Kconfig option to ``n`` and implement the reaction in your Matter event handler.
     * When the Device Attestation Certificate (DAC) private key exists in the factory data set, it can migrate to the PSA ITS secure storage.

       You can also have the DAC private key replaced by zeros in the factory data partition by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_MIGRATE_DAC_PRIV_KEY` Kconfig option to ``y``.
       This functionality is experimental.

   * For the Matter samples and applications using the :file:`samples/matter/common` directory:

     * The structure of the files located in the :file:`common` directory has been changed.
       Align the appropriate paths in your application's :file:`CMakeLists.txt` file and source files, including header files located in the :file:`common` directory.
     * The :file:`event_types.h` header file was removed.
       If your application uses it, add the :file:`event_types.h` file in your application's :file:`src` directory with the following code in the file:

       .. code-block:: C++

          #pragma once

          struct AppEvent; /* needs to be implemented in the application code */
          using EventHandler = void (*)(const AppEvent &);

     * The :file:`board_util.h` header file was renamed to :file:`board_config.h` and moved to the :file:`samples/matter/common/src/board` directory.
       Align any source files that include it to use the new name.
     * The new ``Nrf`` and ``Matter`` namespaces have been added to the files located in the :file:`common` directory.
       Align the source files using these files to use the appropriate namespaces.

Wi-Fi®
------

.. toggle::

   * For samples using Wi-Fi features:

     * A few Kconfig options related to scan operations have been removed in the current release.

        If your application uses scan operations, they need to be updated to remove the dependency on the following options:

         * ``CONFIG_WIFI_MGMT_SCAN_BANDS``
         * ``CONFIG_WIFI_MGMT_SCAN_SSID_FILT``
         * ``CONFIG_WIFI_MGMT_SCAN_CHAN``

     * Instead of the ``CONFIG_WIFI_MGMT_SCAN_MAX_BSS_CNT`` Kconfig option, a new :kconfig:option:`CONFIG_NRF_WIFI_SCAN_MAX_BSS_CNT` Kconfig option is added.

     * The Wi-Fi interface is now renamed from ``wlan0`` to ``nordic_wlan0``, and for easier fetching of the handler, an entry in the DTS file is added ``zephyr_wifi``.

       If your application was using ``device_get_binding("wlan0")``, replace with ``DEVICE_DT_GET(DT_CHOSEN(zephyr_wifi))``.

       Optionally, you can override the label `zephyr_wifi` in the DTS file with a different Wi-Fi interface name.


Libraries
=========

Modem library integration layer
-------------------------------

.. toggle::

   * For applications using :ref:`nrf_modem_lib_readme`:

     * The ``lte_connectivity`` module is renamed to ``lte_net_if``.
       Make sure that all references are updated accordingly, including function names and Kconfig options.

     * If your application is using the ``lte_net_if`` (formerly ``lte_connectivity``) without disabling :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_START`, :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_CONNECT`, and :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_DOWN` Kconfig options, changes are required as the default values are changed from enabled to disabled.

       * Consider using the :c:func:`conn_mgr_all_if_up`, :c:func:`conn_mgr_if_connect` and :c:func:`conn_mgr_if_disconnect` functions instead of enabling the Kconfig options to have better control of the initialization and connection establishment.

     * The Release Assistance Indication (RAI) socket options have been deprecated and replaced with a new consolidated socket option.
       If your application uses ``SO_RAI_*`` socket options, you need to update your socket configuration as follows:

       #. Replace the deprecated socket options :c:macro:`SO_RAI_NO_DATA`, :c:macro:`SO_RAI_LAST`, :c:macro:`SO_RAI_ONE_RESP`, :c:macro:`SO_RAI_ONGOING`, and :c:macro:`SO_RAI_WAIT_MORE` with the new :c:macro:`SO_RAI` option.
       #. Set the optval parameter of the :c:macro:`SO_RAI` socket option to one of the new values ``RAI_NO_DATA``, ``RAI_LAST``, ``RAI_ONE_RESP``, ``RAI_ONGOING``, or ``RAI_WAIT_MORE`` to specify the desired indication.

       Example of migration:

       .. code-block:: c

         /* Before migration. */
         setsockopt(socket_fd, SOL_SOCKET, SO_RAI_LAST, NULL, 0);

         /* After migration. */
         int rai_option = RAI_LAST;
         setsockopt(socket_fd, SOL_SOCKET, SO_RAI, &rai_option, sizeof(rai_option));

nRF Cloud
=========

.. toggle::

   * The :c:func:`nrf_cloud_obj_location_request_create` function has changed.
     The parameter ``const bool request_loc`` has been changed to ``const struct nrf_cloud_location_config *const config``.
   * To migrate to the new API, you need to declare a :c:struct:`nrf_cloud_location_config` structure and set the structure's ``do_reply`` variable to the value used for ``request_loc``.
     Set the two remaining structure variables, ``hi_conf`` and ``fallback``, according to your application's needs.
     You also must provide a pointer to the structure to the :c:func:`nrf_cloud_obj_location_request_create` function instead of the boolean value.

Security
========

.. toggle::

   * For samples using ``CONFIG_NRF_SECURITY``:

     * RSA keys are no longer enabled by default.
       This reduces the code size by 30 kB if not using RSA keys.
       This also breaks the configuration if using the RSA keys without explicitly enabling an RSA key size.
       Enable the required key size to fix the configuration, for example by setting the Kconfig option :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048` if 2048-bit RSA keys are required.

     * The PSA config is now validated by the :file:`ncs/nrf/ext/oberon/psa/core/library/check_crypto_config.h` file.
       Users with invalid configurations must update their PSA configuration according to the error messages that the :file:`check_crypto_config.h` file provides.

   * For the :ref:`crypto_persistent_key` sample:

     * The Kconfig option ``CONFIG_PSA_NATIVE_ITS`` is replaced by the Kconfig option :kconfig:option:`CONFIG_TRUSTED_STORAGE`, which enables the new :ref:`trusted_storage_readme` library.
       The :ref:`trusted_storage_readme` library provides the PSA Internal Trusted Storage (ITS) API for board targets without TF-M.
       It is not backward compatible with the previous PSA ITS implementation.
       Migrating from the PSA ITS implementation, enabled by the ``CONFIG_PSA_NATIVE_ITS`` option, to the new :ref:`trusted_storage_readme` library requires manual data migration.

   * For :ref:`lib_wifi_credentials` library and Wi-Fi samples:

     * ``CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET`` has been removed because it was specific to the previous solution that used PSA Protected Storage instead of PSA Internal Trusted Storage (ITS).
       Use :kconfig:option:`CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_OFFSET` to specify the key offset for PSA ITS.
       Be aware that Wi-Fi credentials stored in Protected Storage will not appear in ITS when switching.
       To avoid re-provisioning Wi-Fi credentials, manually read out the old credentials from Protected Storage in the previously used UID and store to ITS.

zcbor
=====

.. toggle::

   * If you have zcbor-generated code that relies on the zcbor libraries through Zephyr, you must regenerate the files using zcbor 0.8.1.
     Note that the names of generated types and members has been overhauled, so the code using the generated code must likely be changed.

     For example:

      * Leading single underscores and all double underscores are largely gone.
      * Names sometimes gain suffixes like ``_m`` or ``_l`` for disambiguation.
      * All enum (choice) names have now gained a ``_c`` suffix, so the enum name no longer matches the corresponding member name exactly (because this broke C++ namespace rules).

    * The function :c:func:`zcbor_new_state`, :c:func:`zcbor_new_decode_state` and the macro :c:macro:`ZCBOR_STATE_D` have gained new parameters related to decoding of unordered maps.
      Unless you are using that new functionality, these can all be set to NULL or 0.
    * The functions :c:func:`zcbor_bstr_put_term` and :c:func:`zcbor_tstr_put_term` have gained a new parameter ``maxlen``, referring to the maximum length of the parameter ``str``.
      This parameter is passed directly to :c:func:`strnlen` under the hood.
    * The function :c:func:`zcbor_tag_encode` has been renamed to :c:func:`zcbor_tag_put`.
    * Printing has been changed significantly, for example, :c:func:`zcbor_print` is now called :c:func:`zcbor_log`, and :c:func:`zcbor_trace` with no parameters is gone, and in its place are :c:func:`zcbor_trace_file` and :c:func:`zcbor_trace`, both of which take a ``state`` parameter.

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Samples and applications
========================

This section describes the changes related to samples and applications.

Matter
------

.. toggle::

   * For the Matter samples and applications:

     * The new API and helper modules have been added to the :file:`samples/matter/common` directory.
       All Matter samples and applications have been changed to use the common software modules.

       The inclusion of common software module source code in the CMake application target has been moved to the :file:`samples/matter/common/cmake/source_common.cmake` file.
       Source code for specific software modules is added automatically based on the selected Kconfig options.
       To include all required source code files, add the following line to the :file:`CMakeLists.txt` file in your project directory:

       .. code-block:: console

         include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/source_common.cmake)

       You can follow the new approach and migrate your application to use the common software modules.
       This will significantly reduce the size of the code required to be implemented in the application.
       You can also choose to keep using the previous approach, but due to the structural differences, it may be harder to use Matter samples and applications as a reference for an application using the older approach.

       The following steps use the :ref:`matter_template_sample` sample as an example.
       To migrate the application from |NCS| v2.5.0 and start using the common software modules used in |NCS| v2.6.0:

       * Replace the code used for initialization and handling of the board's components, like LEDs or buttons, with the common ``board`` module.
         The ``board`` module handles buttons and LEDs in a way consistent with Matter samples UI.
         It uses the ``task_executor`` common module for posting a board-related event.
         You can also use the ``task_executor`` module for posting and dispatching events in your application.

         To replace the |NCS| v2.5.0 compliant implementation with the ``board`` module, complete the following steps:

         1. Remove the following code from the :file:`app_task.h` file:

            .. code-block:: C++

             #include "app_event.h"
             #include "led_widget.h"

             static void PostEvent(const AppEvent &event);
             void CancelTimer();
             void StartTimer(uint32_t timeoutInMs);

             static void DispatchEvent(const AppEvent &event);
             static void UpdateLedStateEventHandler(const AppEvent &event);
             static void FunctionHandler(const AppEvent &event);
             static void FunctionTimerEventHandler(const AppEvent &event);
             static void ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged);
             static void LEDStateUpdateHandler(LEDWidget &ledWidget);
             static void FunctionTimerTimeoutCallback(k_timer *timer);
             static void UpdateStatusLED();

             FunctionEvent mFunction = FunctionEvent::NoneSelected;
             bool mFunctionTimerActive = false;

         #. Remove the following code from the :file:`app_task.cpp` file:

            .. code-block:: C++

             #include "app_config.h"
             #include "led_util.h"
             #include "board_util.h"
             #include <dk_buttons_and_leds.h>

             namespace
             {
             constexpr size_t kAppEventQueueSize = 10;
             constexpr uint32_t kFactoryResetTriggerTimeout = 6000;

             K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));
             k_timer sFunctionTimer;

             LEDWidget sStatusLED;
             #if NUMBER_OF_LEDS == 2
             FactoryResetLEDsWrapper<1> sFactoryResetLEDs{ { FACTORY_RESET_SIGNAL_LED } };
             #else
             FactoryResetLEDsWrapper<3> sFactoryResetLEDs{ { FACTORY_RESET_SIGNAL_LED, FACTORY_RESET_SIGNAL_LED1,
                         FACTORY_RESET_SIGNAL_LED2 } };
             #endif

             bool sIsNetworkProvisioned = false;
             bool sIsNetworkEnabled = false;
             bool sHaveBLEConnections = false;
             } /* namespace */

             namespace LedConsts
             {
             namespace StatusLed
             {
               namespace Unprovisioned
               {
                 constexpr uint32_t kOn_ms{ 100 };
                 constexpr uint32_t kOff_ms{ kOn_ms };
               } /* namespace Unprovisioned */
               namespace Provisioned
               {
                 constexpr uint32_t kOn_ms{ 50 };
                 constexpr uint32_t kOff_ms{ 950 };
               } /* namespace Provisioned */

             } /* namespace StatusLed */
             } /* namespace LedConsts */

             void AppTask::ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged)
             {
               AppEvent button_event;
               button_event.Type = AppEventType::Button;

               if (FUNCTION_BUTTON_MASK & hasChanged) {
                 button_event.ButtonEvent.PinNo = FUNCTION_BUTTON;
                 button_event.ButtonEvent.Action =
                   static_cast<uint8_t>((FUNCTION_BUTTON_MASK & buttonState) ? AppEventType::ButtonPushed :
                                     AppEventType::ButtonReleased);
                 button_event.Handler = FunctionHandler;
                 PostEvent(button_event);
               }
             }

             void AppTask::FunctionTimerTimeoutCallback(k_timer *timer)
             {
               if (!timer) {
                 return;
               }

               AppEvent event;
               event.Type = AppEventType::Timer;
               event.TimerEvent.Context = k_timer_user_data_get(timer);
               event.Handler = FunctionTimerEventHandler;
               PostEvent(event);
             }

             void AppTask::FunctionTimerEventHandler(const AppEvent &)
             {
               if (Instance().mFunction == FunctionEvent::FactoryReset) {
                 Instance().mFunction = FunctionEvent::NoneSelected;
                 LOG_INF("Factory Reset triggered");

                 sStatusLED.Set(true);
                 sFactoryResetLEDs.Set(true);

                 chip::Server::GetInstance().ScheduleFactoryReset();
               }
             }

             void AppTask::FunctionHandler(const AppEvent &event)
             {
               if (event.ButtonEvent.PinNo != FUNCTION_BUTTON)
                 return;

               if (event.ButtonEvent.Action == static_cast<uint8_t>(AppEventType::ButtonPushed)) {
                 Instance().StartTimer(kFactoryResetTriggerTimeout);
                 Instance().mFunction = FunctionEvent::FactoryReset;
               } else if (event.ButtonEvent.Action == static_cast<uint8_t>(AppEventType::ButtonReleased)) {
                 if (Instance().mFunction == FunctionEvent::FactoryReset) {
                   sFactoryResetLEDs.Set(false);
                   UpdateStatusLED();
                   Instance().CancelTimer();
                   Instance().mFunction = FunctionEvent::NoneSelected;
                   LOG_INF("Factory Reset has been Canceled");
                 }
               }
             }

             void AppTask::LEDStateUpdateHandler(LEDWidget &ledWidget)
             {
               AppEvent event;
               event.Type = AppEventType::UpdateLedState;
               event.Handler = UpdateLedStateEventHandler;
               event.UpdateLedStateEvent.LedWidget = &ledWidget;
               PostEvent(event);
             }

             void AppTask::UpdateLedStateEventHandler(const AppEvent &event)
             {
               if (event.Type == AppEventType::UpdateLedState) {
                 event.UpdateLedStateEvent.LedWidget->UpdateState();
               }
             }

             void AppTask::UpdateStatusLED()
             {
               /* Update the status LED.
               *
               * If IPv6 networking and service provisioned, keep the LED On constantly.
               *
               * If the system has BLE connection(s) uptill the stage above, THEN blink the LED at an even
               * rate of 100ms.
               *
               * Otherwise, blink the LED for a very short time. */
               if (sIsNetworkProvisioned && sIsNetworkEnabled) {
                 sStatusLED.Set(true);
               } else if (sHaveBLEConnections) {
                 sStatusLED.Blink(LedConsts::StatusLed::Unprovisioned::kOn_ms,
                     LedConsts::StatusLed::Unprovisioned::kOff_ms);
               } else {
                 sStatusLED.Blink(LedConsts::StatusLed::Provisioned::kOn_ms, LedConsts::StatusLed::Provisioned::kOff_ms);
               }
             }

             void AppTask::CancelTimer()
             {
               k_timer_stop(&sFunctionTimer);
             }

             void AppTask::StartTimer(uint32_t timeoutInMs)
             {
               k_timer_start(&sFunctionTimer, K_MSEC(timeoutInMs), K_NO_WAIT);
             }

             void AppTask::PostEvent(const AppEvent &event)
             {
               if (k_msgq_put(&sAppEventQueue, &event, K_NO_WAIT) != 0) {
                 LOG_INF("Failed to post event to app task event queue");
               }
             }

             void AppTask::DispatchEvent(const AppEvent &event)
             {
               if (event.Handler) {
                 event.Handler(event);
               } else {
                 LOG_INF("Event received with no handler. Dropping event.");
               }
             }

         #. Include the ``board`` and ``task_executor`` modules to the :file:`app_task.cpp` file.

            .. code-block:: C++

             #include "app/task_executor.h"
             #include "board/board.h"

         #. Replace the code in the :c:func:`Init` method, in the :file:`app_task.cpp` file.
            The :c:func:`Init` method from the ``board`` module has two optional arguments, that you can use to pass your own handler implementations for handling buttons or LEDs.

            * Remove:

              .. code-block:: C++

               /* Initialize LEDs */
               LEDWidget::InitGpio();
               LEDWidget::SetStateUpdateCallback(LEDStateUpdateHandler);

               sStatusLED.Init(SYSTEM_STATE_LED);

               UpdateStatusLED();

               /* Initialize buttons */
               int ret = dk_buttons_init(ButtonEventHandler);
               if (ret) {
                 LOG_ERR("dk_buttons_init() failed");
                 return chip::System::MapErrorZephyr(ret);
               }

               /* Initialize function timer */
               k_timer_init(&sFunctionTimer, &AppTask::FunctionTimerTimeoutCallback, nullptr);
               k_timer_user_data_set(&sFunctionTimer, this);

            * Add:

              .. code-block:: C++

               if (!Nrf::GetBoard().Init()) {
                   LOG_ERR("User interface initialization failed.");
                   return CHIP_ERROR_INCORRECT_STATE;
               }

         #. Replace the code in the :c:func:`StartApp` method, in the :file:`app_task.cpp` file:

            * Remove:

              .. code-block:: C++

               AppEvent event = {};

               k_msgq_get(&sAppEventQueue, &event, K_FOREVER);
               DispatchEvent(event);

            * Add in the while loop:

              .. code-block:: C++

               Nrf::DispatchNextTask();

         #. Replace the code in the :c:func:`ChipEventHandler` method, in the :file:`app_task.cpp` file:

            * Add at the top of the method:

              .. code-block:: C++

               bool sIsNetworkProvisioned = false;
               bool sIsNetworkEnabled = false;

            * Remove for the :c:enum:`kCHIPoBLEAdvertisingChange` enum:

              .. code-block:: C++

               sHaveBLEConnections = ConnectivityMgr().NumBLEConnections() != 0;
               UpdateStatusLED();

            * Add for the :c:enum:`kCHIPoBLEAdvertisingChange` enum:

              .. code-block:: C++

               if (ConnectivityMgr().NumBLEConnections() != 0) {
                 Nrf::GetBoard().UpdateDeviceState(Nrf::DeviceState::DeviceConnectedBLE);
               }

            * Remove for the :c:enum:`kThreadStateChange` and the :c:enum:`kWiFiConnectivityChange` enums:

              .. code-block:: C++

               UpdateStatusLED();

            * Add for the :c:enum:`kThreadStateChange` and the :c:enum:`kWiFiConnectivityChange` enums:

              .. code-block:: C++

               if (sIsNetworkProvisioned && sIsNetworkEnabled) {
                 Nrf::GetBoard().UpdateDeviceState(Nrf::DeviceState::DeviceProvisioned);
               } else {
                 Nrf::GetBoard().UpdateDeviceState(Nrf::DeviceState::DeviceDisconnected);
               }

         #. Add the ``board`` and ``task_executor`` modules to the compilation.
            Edit the :file:`CMakeLists.txt` file as follows:

            .. code-block:: cmake

             target_sources(app PRIVATE
                 ${COMMON_ROOT}/src/app/task_executor.cpp
                 ${COMMON_ROOT}/src/board/board.cpp
             )

         #. Add the common :file:`Kconfig` file to the list of sourced Kconfig files.
            To do so, edit your application :file:`Kconfig` file and add the following code one line before sourcing the :file:`Kconfig.zephyr` file:

            .. code-block:: kconfig

             source "${ZEPHYR_BASE}/../nrf/samples/matter/common/src/Kconfig"

       * Replace the code used for Matter stack initialization with the common ``matter_init`` module.
         The ``matter_init`` module initializes the Matter stack in a safe way, which means it takes care of the proper order of initialization for software modules.
         It uses the ``matter_event_handler`` common module for defining a default Matter event handler.
         You can customize the module behavior by injecting your own initialization parameters and callbacks.

         To replace the |NCS| v2.5.0 compliant implementation with the ``matter_init`` module, complete the following steps:

         1. Remove the following code from the :file:`app_task.h` file:

            .. code-block:: C++

             #if CONFIG_CHIP_FACTORY_DATA
             #include <platform/nrfconnect/FactoryDataProvider.h>
             #else
             #include <platform/nrfconnect/DeviceInstanceInfoProviderImpl.h>
             #endif

         #. Replace the code in the :c:struct:`AppTask` class, in the :file:`app_task.h` file:

            * Remove:

              .. code-block:: C++

               static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);

               #if CONFIG_CHIP_FACTORY_DATA
                 chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
               #endif

            * Add:

              .. code-block:: C++

               static void MatterEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);

         #. Remove the following code from the :file:`app_task.cpp` file:

            .. code-block:: C++

             #include "fabric_table_delegate.h"
             #include <platform/CHIPDeviceLayer.h>
             #include <app/server/Server.h>
             #include <credentials/DeviceAttestationCredsProvider.h>
             #include <credentials/examples/DeviceAttestationCredsExample.h>
             #include <lib/support/CHIPMem.h>
             #include <lib/support/CodeUtils.h>
             #include <system/SystemError.h>

             #ifdef CONFIG_CHIP_WIFI
             #include <app/clusters/network-commissioning/network-commissioning.h>
             #include <platform/nrfconnect/wifi/NrfWiFiDriver.h>
             #endif

             #include <zephyr/kernel.h>

             using namespace ::chip::Credentials;

             #ifdef CONFIG_CHIP_WIFI
             app::Clusters::NetworkCommissioning::Instance
               sWiFiCommissioningInstance(0, &(NetworkCommissioning::NrfWiFiDriver::Instance()));
             #endif

         #. Include the ``matter_init`` module to the :file:`app_task.cpp` file.

            .. code-block:: C++

             #include "app/matter_init.h"

         #. Rename the :c:func:`ChipEventHandler` method, in the :file:`app_task.cpp` file, to the :c:func:`MatterEventHandler` method.
         #. Replace the code in the :c:func:`Init` method, in the :file:`app_task.cpp` file:

            * Remove:

              .. code-block:: C++

               /* Initialize CHIP stack */
               LOG_INF("Init CHIP stack");

               CHIP_ERROR err = chip::Platform::MemoryInit();
               if (err != CHIP_NO_ERROR) {
                 LOG_ERR("Platform::MemoryInit() failed");
                 return err;
               }

               err = PlatformMgr().InitChipStack();
               if (err != CHIP_NO_ERROR) {
                 LOG_ERR("PlatformMgr().InitChipStack() failed");
                 return err;
               }

               #if defined(CONFIG_NET_L2_OPENTHREAD)
                 err = ThreadStackMgr().InitThreadStack();
                 if (err != CHIP_NO_ERROR) {
                   LOG_ERR("ThreadStackMgr().InitThreadStack() failed: %s", ErrorStr(err));
                   return err;
                 }

               #ifdef CONFIG_OPENTHREAD_MTD_SED
                 err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
               #elif CONFIG_OPENTHREAD_MTD
                 err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_MinimalEndDevice);
               #else
                 err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_Router);
               #endif /* CONFIG_OPENTHREAD_MTD_SED */
                 if (err != CHIP_NO_ERROR) {
                   LOG_ERR("ConnectivityMgr().SetThreadDeviceType() failed: %s", ErrorStr(err));
                   return err;
                 }

               #elif defined(CONFIG_CHIP_WIFI)
                 sWiFiCommissioningInstance.Init();
               #else
                 return CHIP_ERROR_INTERNAL;
               #endif /* CONFIG_NET_L2_OPENTHREAD */

               #ifdef CONFIG_CHIP_OTA_REQUESTOR
                 /* OTA image confirmation must be done before the factory data init. */
                 OtaConfirmNewImage();
               #endif

                 /* Initialize CHIP server */
               #if CONFIG_CHIP_FACTORY_DATA
                 ReturnErrorOnFailure(mFactoryDataProvider.Init());
                 SetDeviceInstanceInfoProvider(&mFactoryDataProvider);
                 SetDeviceAttestationCredentialsProvider(&mFactoryDataProvider);
                 SetCommissionableDataProvider(&mFactoryDataProvider);
               #else
                 SetDeviceInstanceInfoProvider(&DeviceInstanceInfoProviderMgrImpl());
                 SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
               #endif

                 static chip::CommonCaseDeviceServerInitParams initParams;
                 (void)initParams.InitializeStaticResourcesBeforeServerInit();

                 ReturnErrorOnFailure(chip::Server::GetInstance().Init(initParams));
                 ConfigurationMgr().LogDeviceConfig();
                 PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
                 AppFabricTableDelegate::Init();

                 /*
                 * Add CHIP event handler and start CHIP thread.
                 * Note that all the initialization code should happen prior to this point to avoid data races
                 * between the main and the CHIP threads.
                 */
                 PlatformMgr().AddEventHandler(ChipEventHandler, 0);

                 err = PlatformMgr().StartEventLoopTask();
                 if (err != CHIP_NO_ERROR) {
                   LOG_ERR("PlatformMgr().StartEventLoopTask() failed");
                   return err;
                 }

                 return CHIP_NO_ERROR;

            * Add the following code before the board components initialization.
              The :c:func:`PrepareServer` method has two optional arguments that you can use to pass your own Matter event handler and  initialization data, including custom callbacks to invoke before and after the initialization.

              .. code-block:: C++

                 /* Initialize Matter stack */
                 ReturnErrorOnFailure(Nrf::Matter::PrepareServer(MatterEventHandler));

            * Add the following code at the end of :c:func:`Init` method:

              .. code-block:: C++

               return Nrf::Matter::StartServer();

         #. Add the ``main_init`` and ``matter_event_handler`` modules to the compilation.
            Edit the :file:`CMakeLists.txt` file as follows:

           .. code-block:: cmake

             target_sources(app PRIVATE
                 ${COMMON_ROOT}/src/app/matter_init.cpp
                 ${COMMON_ROOT}/src/app/matter_event_handler.cpp
             )

       * Replace the code used for Matter event handling with the common ``matter_event_handler`` module.
         The ``matter_event_handler`` module handles events generated by the Matter stack in a Nordic platform-specific way.
         You can customize the module behavior by registering your own Matter event handler that extends the default implementation.

         To replace the |NCS| v2.5.0 compliant implementation with the ``matter_event_handler`` module, complete the following steps:

         1. Remove the :c:func:`MatterEventHandler` method declaration from the :file:`app_task.h` file.
         #. Remove the :c:func:`MatterEventHandler` method implementation from the :file:`app_task.cpp` file.
         #. Replace the code in the :c:func:`Init` method, in the :file:`app_task.cpp` file:

            * Remove:

              .. code-block:: C++

               ReturnErrorOnFailure(Nrf::Matter::PrepareServer(MatterEventHandler));

            * Add:

              .. code-block:: C++

               ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

               /* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
                * state. */
                ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

         #. Add the ``matter_event_handler`` module to the compilation.
            Edit :file:`CMakeLists.txt` file as follows:

            .. code-block:: cmake

             target_sources(app PRIVATE
                 ${COMMON_ROOT}/src/app/matter_event_handler.cpp
             )

LwM2M
-----

.. toggle::

   * For LwM2M applications, replace the :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_DTLS_CID` Kconfig option with :kconfig:option:`CONFIG_LWM2M_DTLS_CID`.


.. _nrf5340_audio_migration_notes:

nRF5340 Audio applications
--------------------------

.. toggle::

   * The :ref:`nrf53_audio_app` have changed the default controller from the LE Audio controller for nRF5340 library to Nordic Semiconductor's standard :ref:`ug_ble_controller_softdevice` (:ref:`softdevice_controller_iso`).
     :ref:`ug_ble_controller_softdevice` is included and built automatically.
     For |NCS| 2.6.0, tests have been run and issues documented as before for the previously used LE Audio controller for nRF5340 library.
     However, the LE Audio controller for nRF5340 library is marked as deprecated, it will be removed soon, and there will be no new features or fixes to this controller.
     Make sure to remove references to LE Audio controller for nRF5340 from your application and transition to the new controller.
     There should be no negative impact on performance of the nRF5340 Audio applications with the :ref:`ug_ble_controller_softdevice`.
     This change enables the use of standard |NCS| tools and procedures for building, configuring and DFU.

Bluetooth® Mesh
---------------

.. toggle::

   * For the Bluetooth Mesh samples and applications, a new sensor API (see :ref:`bt_mesh_sensors_readme`) is introduced with |NCS| v2.6.0.
     The previous sensor API is deprecated.

     The usage of the new sensor API is demonstrated in samples :ref:`bluetooth_mesh_sensor_client`, :ref:`bluetooth_mesh_sensor_server` and :ref:`bluetooth_mesh_light_lc`.

     The Kconfig option :kconfig:option:`CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE` is enabled by default in the deprecation period.
     This means that the existing samples and applications can continue using the deprecated sensor API as normal during the deprecation period, without the additional configuration.
     The samples and applications will get a deprecation warning when compiled, that the user can choose to disregard.

     When the deprecation period is over, the deprecated sensor API will be removed, and the samples and applications will no longer compile unless updated to the new sensor API.

     To use the new sensor API for new and existing samples and applications, disable the Kconfig option :kconfig:option:`CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE` in the configuration.
     This configuration option will be removed when the deprecation period is over, and then it has to be removed from the sample and application configuration.
     It is also recommended to enable the Kconfig option :kconfig:option:`CONFIG_FPU` to support the accelerated floating point operations, and the Kconfig option :kconfig:option:`CONFIG_CBPRINTF_FP_SUPPORT` to support the floating point printing.

     * Sensor API arguments and callback parameters previously defined with :c:struct:`sensor_value` now use :c:struct:`bt_mesh_sensor_value` instead.
     * The :c:member:`bt_mesh_sensor_value.format` needs to be filled by the application for variables passed to the sensor API.
     * There are several different types of :c:struct:`bt_mesh_sensor_format` described in the file :file:`include/bluetooth/mesh/sensor_types.h`.
     * When filling in sensor values for a channel, the format can be found through ``sensor->type.channels[i].format`` defined for the given :c:struct:`bt_mesh_sensor` sensor.
     * :c:struct:`bt_mesh_sensor_value` with a valid format can be converted to and from integer, float and :c:struct:`sensor_value` through ``bt_mesh_sensor_value_to/from*`` functions.
     * Where the applications previously just added values directly to :c:member:`sensor_value.val1` and :c:member:`sensor_value.val2`, the correct way is to use ``bt_mesh_sensor_value_to/from*`` functions to either set or extract the values.

     Example of changes that need to be done for a sensor using sensor values, from e.g. the file :file:`drivers/sensor.h`:

       ..  code-block:: diff

           static int chip_temp_get(struct bt_mesh_sensor_srv *srv,
                                    struct bt_mesh_sensor *sensor,
                                    struct bt_mesh_msg_ctx *ctx,
           -                         struct sensor_value *rsp)
           +                         struct bt_mesh_sensor_value *rsp)
           {
           +        struct sensor_value channel_val;
                   int err;

                   sensor_sample_fetch(dev);

           -        err = sensor_channel_get(dev, SENSOR_DATA_TYPE, rsp);
           +        err = sensor_channel_get(dev, SENSOR_DATA_TYPE, &channel_val);
                   if (err) {
                           printk("Error getting temperature sensor data (%d)\n", err);
                   }
           +        err = bt_mesh_sensor_value_from_sensor_value(
           +                sensor->type->channels[0].format, &channel_val, rsp);
           +        if (err) {
           +                printk("Error encoding temperature sensor data (%d)\n", err);
           +        }

                   return err;
           }

     Example of changes that need to be done for a sensor using the float values:

       ..  code-block:: diff

           static int amb_light_level_get(struct bt_mesh_sensor_srv *srv,
                                          struct bt_mesh_sensor *sensor,
                                          struct bt_mesh_msg_ctx *ctx,
           -                               struct sensor_value *rsp)
           +                               struct bt_mesh_sensor_value *rsp)
           {
                   int err;

                   /* Report ambient light as dummy value, and changing it by pressing
                    * a button. The logic and hardware for measuring the actual ambient
                    * light usage of the device should be implemented here.
                    */
           -        double reported_value = amb_light_level_gain * dummy_ambient_light_value;
           +        float reported_value = amb_light_level_gain * dummy_ambient_light_value;

           -        err = sensor_value_from_double(rsp, reported_value);
           +        err = bt_mesh_sensor_value_from_float(sensor->type->channels[0].format,
           +                                              reported_value, rsp);
           -        if (err) {
           +        if (err && err != -ERANGE) {
                           printk("Error encoding ambient light level sensor data (%d)\n", err);
                           return err;
                   }
                   return 0;
           }
