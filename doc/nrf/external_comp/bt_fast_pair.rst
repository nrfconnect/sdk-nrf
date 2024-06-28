.. _ug_bt_fast_pair:

Google Fast Pair integration
############################

.. contents::
   :local:
   :depth: 2

Google Fast Pair is a standard for pairing *Bluetooth®* and Bluetooth Low Energy (LE) devices with as little user interaction required as possible.
Google also provides additional features built upon the Fast Pair standard.
For detailed information about supported functionalities, see the official `Fast Pair`_ documentation.

.. note::
   The software maturity level for the Fast Pair support in the |NCS| is indicated at the use case level in the :ref:`software_maturity_fast_pair_use_case` table and at the feature level in the :ref:`software_maturity_fast_pair_feature` table.

Integration prerequisites
*************************

Before you start the |NCS| integration with Fast Pair, make sure that the following prerequisites are fulfilled:

* :ref:`Install the nRF Connect SDK <installation>`.
* Set up a supported :term:`Development Kit (DK)`.
  See :ref:`device_guides` for more information on setting up the DK you are using.
* Install the requirements mentioned in the :ref:`bt_fast_pair_provision_script`.
  The script is automatically invoked by the build system during application build to generate Fast Pair provisioning data hex file.
* During the early stages of development, you can use the debug Model ID and Anti-Spoofing Public/Private Key pair obtained by Nordic Semiconductor for local tests.
  Later on, you need to register your own Fast Pair Provider device with Google.
  The :ref:`ug_bt_fast_pair_provisioning_register` section in this document explains how to register the device and obtain the Model ID and Anti-Spoofing Public/Private Key pair.

Solution architecture
*********************

The |NCS| integrates the Fast Pair Provider role, facilitating communication between the Fast Pair Seeker (typically a smartphone) and the Provider (your device).
The integration involves following the instructions outlined in the :ref:`ug_integrating_fast_pair` section.
The SDK supports extensions such as Battery Notification and Personalized Name, which can be included based on the specific use case requirements.

.. _ug_fast_pair_extensions:

Google Fast Pair extensions
***************************

The Fast Pair standard implementation in the |NCS| supports the following extensions:

* Battery Notification extension
* Personalized Name extension
* Find My Device Network (FMDN) extension

Each supported extension is described in the following sections.

.. tip::
   Extension-specific instructions are located under the extension section in each integration step of this guide.
   You can safely skip sections for extensions that you do not want to support in your application.

Battery Notification extension
==============================

The extension provides a mechanism to broadcast battery level information that is encoded in the Fast Pair not discoverable advertising payload.
You can set up the battery information for up to three different components (required for the earbuds use case: left bud, right bud and case).

For more details on this extension, see the `Fast Pair Battery Notification extension`_ documentation.

Personalized Name extension
===========================

The extension allows the user to attach a personalized name to their Fast Pair accessories.

For more details on this extension, see the `Fast Pair Personalized Name extension`_ documentation.

FMDN extension
==============

The FMDN extension leverages the Find My Device network, which is a crowdsourced network consisting of millions of Android devices that use Bluetooth LE to detect missing devices and report their approximate locations back to their owners.
The entire process is end-to-end encrypted and anonymous, so no one else (including Google) can view device's location or information.
The Find My Device network also includes features protecting the user against unwanted tracking.

You can add your accessory to the Find My Device network through provisioning that happens during the Bluetooth LE connection.
Once provisioned, the accessory starts to advertise FMDN frames that contain its unique identifier.
This advertising payload is used by nearby Android devices to report the accessory location to its owner.
The accessory location is an approximation of the reporting device's location, meaning it is not precise.
The FMDN frames are independently broadcasted alongside the standard application payload.
You can remove your accessory from the Find My Device network in a symmetrical operation, called unprovisioning.
Once unprovisioned, the accessory stops advertising FMDN frames.

The support for the FMDN extension is available on Android platforms.

`Google Play Services`_ and Android system level support are responsible for the provisioning of the FMDN extension.
They also perform background tasks, such as periodic clock synchronization of the provisioned devices.

`Find My Device app`_ is an end-user application for managing the tracking accessories.
It allows you to:

* Locate your accessories using the map view.
* Play sound on the nearby tagged item to make it easier to find.
* Check the battery level of your accessory.
* Remove (unprovision) your item.

For more details on this extension, see the `Fast Pair Find My Device Network extension`_ documentation.
This documentation also contains the FMDN Accessory specification, which is frequently used as a reference in the FMDN sections of this guide.

The FMDN Accessory specification integrates the Detecting Unwanted Location Trackers (DULT) specification, which is a joint standardization effort from Apple, Google and other companies to prevent unwanted tracking.
Relevant FMDN sections of this guide describe the DULT integration with the FMDN extension.
For more details on the DULT integration guidelines, see the `Fast Pair Unwanted Tracking Prevention Guidelines`_ documentation.

.. _ug_integrating_fast_pair:

Integration steps
*****************

The Fast Pair standard integration in the |NCS| consists of the following steps:

1. :ref:`Provisioning the device <ug_bt_fast_pair_provisioning>`
#. :ref:`Performing prerequisite operations <ug_bt_fast_pair_prerequisite_ops>`
#. :ref:`Setting up Bluetooth LE advertising <ug_bt_fast_pair_advertising>`
#. :ref:`Interacting with GATT service <ug_bt_fast_pair_gatt_service>`
#. :ref:`Managing factory resets <ug_bt_fast_pair_factory_reset>`
#. :ref:`Tailoring protocol for specific use case <ug_bt_fast_pair_use_case>`

These steps are described in the following sections.

The Fast Pair standard implementation in the |NCS| integrates Fast Pair Provider, one of the available `Fast Pair roles`_.
For an integration example, see the :ref:`fast_pair_input_device` sample.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_provisioning:

Provisioning the device
***********************

A device model must be registered with Google to work as a Fast Pair Provider.
The data is used for procedures defined by the Fast Pair standard.

.. _ug_bt_fast_pair_provisioning_register:

Registering Fast Pair Provider
==============================

See the official `Fast Pair Model Registration`_ documentation for information on how to register the device and obtain the Model ID and Anti-Spoofing Public/Private Key pair.
Alternatively, you can use the debug Model ID and Anti-Spoofing Public/Private Key pair obtained by Nordic Semiconductor for the development purposes.
See the following samples and applications for details about the debug Fast Pair Providers registered by Nordic:

* The :ref:`fast_pair_input_device` sample
* The :ref:`fast_pair_locator_tag` sample
* The :ref:`nrf_desktop` application

.. _ug_bt_fast_pair_provisioning_register_device_type:

Device type
-----------

When registering the device in the Google Nearby Devices console, go to the **Fast Pair** protocol configuration panel, and in the **Device Type** list select an option that matches your application's use case.
The chosen device type also determines the optional feature set that you can support in your use case.
You declare support for each feature by selecting the **true** option.

.. note::
   Ensure you make an informed decision when selecting the device type, as it has a significant impact on the Fast Pair Seeker behavior in relation to your Provider's device.

The Fast Pair standard implementation in the |NCS| actively supports the following device types and use cases:

* Input device (see the :ref:`fast_pair_input_device` sample)
* Locator tag (see the :ref:`fast_pair_locator_tag` sample)

FMDN extension
--------------

To support the FMDN extension, set the **Find My Device** feature to **true** for the device that you want to register in the Google Nearby Devices console.

For an example that uses the **Find My Device** feature, see the :ref:`fast_pair_locator_tag` sample.

.. note::
   To test the FMDN extension with the debug (uncertified) device models, you must set up your Android test device.
   Make sure your phone uses the primary email account that is registered on Google's email allow list for the FMDN feature.
   To register your development email account, complete Google's device proposal form.
   You can find the link to the device proposal form in the `Fast Pair Find My Device Network extension`_ specification.

Provisioning registration data onto device
==========================================

The Fast Pair standard requires provisioning the device with Model ID and Anti-Spoofing Private Key obtained during device model registration.
In the |NCS|, the provisioning data is generated as a hexadecimal file using the :ref:`bt_fast_pair_provision_script`.

When building the Fast Pair in the |NCS|, the build system automatically calls the Fast Pair provision script and includes the resulting hexadecimal file in the firmware (the :file:`merged.hex` file).
To build an application with the Fast Pair support, include the following additional CMake options:

* ``FP_MODEL_ID`` - Fast Pair Model ID in format ``0xXXXXXX``,
* ``FP_ANTI_SPOOFING_KEY`` - base64-encoded Fast Pair Anti-Spoofing Private Key.

The ``bt_fast_pair`` partition address is provided automatically by the build system.

For example, when building an application with the |nRFVSC|, you need to add the following parameters in the **Extra CMake arguments** field on the **Add Build Configuration view**: ``-DFP_MODEL_ID=0xFFFFFF -DFP_ANTI_SPOOFING_KEY=AbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbA=``.
Make sure to replace ``0xFFFFFF`` and ``AbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbA=`` with values obtained for your device.
See :ref:`cmake_options` for more information about defining CMake options.
See the following sections for information on how to add the Google Fast Pair subsystem to your project.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_prerequisite_ops:

Performing prerequisite operations
**********************************

To start integrating the Google Fast Pair subsystem in your project, complete the following prerequisite steps:

* :ref:`ug_bt_fast_pair_prerequisite_ops_kconfig`
* :ref:`ug_bt_fast_pair_prerequisite_ops_api`

The subsequent subsections describe required steps for enabling Fast Pair extensions supported in the |NCS|.

.. _ug_bt_fast_pair_prerequisite_ops_kconfig:

Enabling Fast Pair in Kconfig
=============================

If you are using the default |NCS| build system configuration with sysbuild and wish to add the Google Fast Pair subsystem to your project, enable the ``SB_CONFIG_BT_FAST_PAIR`` Kconfig option.
If you do not use sysbuild, you must enable :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option at the main application image level.

.. note::
   Sysbuild sets the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option in the main application image based on the value of the ``SB_CONFIG_BT_FAST_PAIR`` Kconfig option.
   Your configuration of the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option at the main application image will be ineffective, as sysbuild overrides it.

.. _ug_bt_fast_pair_prerequisite_ops_api:

Enabling Fast Pair with API
===========================

An application can communicate with the Fast Pair subsystem using API calls and registered callbacks.
The Fast Pair subsystem uses the registered callbacks to inform the application about the Fast Pair related events.

The application must register the callbacks before it enables the Fast Pair subsystem and starts to operate as the Fast Pair Provider and advertise Bluetooth LE packets.
To identify the callback registration functions in the Fast Pair API, look for the ``_register`` suffix.
Set your application-specific callback functions in the callback structure that is the input parameter for the ``..._register`` API function.
The callback structure must persist in the application memory (static declaration), as during the registration, the Fast Pair module stores only the memory pointer to it.

The standard Fast Pair API (without extensions) currently supports the :c:func:`bt_fast_pair_info_cb_register` function (optional) for registering application callbacks.

The standard Fast Pair (without extensions) does not require registration of any callback type, meaning all callbacks are optional.

After the callback registration, the Fast Pair subsystem must be enabled with the :c:func:`bt_fast_pair_enable` function.
Before performing the :c:func:`bt_fast_pair_enable` operation, you must enable Bluetooth with the :c:func:`bt_enable` function and load Zephyr's :ref:`zephyr:settings_api` with the :c:func:`settings_load` function.
The Fast Pair subsystem readiness can be checked with the :c:func:`bt_fast_pair_is_ready` function.
The Fast Pair subsystem can be disabled with the :c:func:`bt_fast_pair_disable` function.
In the Fast Pair subsystem disabled state, most of the Fast Pair APIs are not available.

Apart from the callback registration and enabling the Fast Pair subsystem, no additional operations are needed to integrate the standard Fast Pair implementation.

Personalized Name extension
===========================

To support the Personalized Name extension, ensure that the :kconfig:option:`CONFIG_BT_FAST_PAIR_PN` Kconfig option is enabled in your project.
This extension is enabled by default.

FMDN extension
==============

Enable the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN` Kconfig option to support the FMDN extension in your project.

Managing the activation state
-----------------------------

The FMDN extension is enabled together with the general Fast Pair module once the :c:func:`bt_fast_pair_enable` function executes successfully.
The Provider can respond to the extension-specific requests coming from the Seeker over the Bluetooth GATT layer only in the enabled state.
Depending on its state, the extension starts other activities, such as:

* Beacon clock service that is used to measure time in seconds.
* FMDN advertising with periodical updates to the FMDN payload (the FMDN advertising is in use only if provisioned).

The FMDN extension is disabled together with the general Fast Pair module once the :c:func:`bt_fast_pair_disable` function executes successfully.
During the disable operation, the Provider terminates all extension-related activities that are mentioned in the enable operation description.
Additionally, it drops all FMDN connections that were established using the FMDN advertising payload.

.. note::
   A failure in the enable or disable operation can have certain side effects related to the module state.
   An error during the :c:func:`bt_fast_pair_enable` or the :c:func:`bt_fast_pair_disable` function call results in the *unready* state of the extension.
   In that case, you should retry the operation or reboot the system, as certain module operations may be active.

To check the FMDN extension readiness, use the :c:func:`bt_fast_pair_is_ready` function of the general Fast Pair module.
The extension is marked as *ready* when it is in the enabled state and *unready* when it is in the disabled state.
The *unready* state is also reported by the :c:func:`bt_fast_pair_is_ready` function if the :c:func:`bt_fast_pair_enable` or :c:func:`bt_fast_pair_disable` operations fail.

You can use the following API functions only in the *unready* state of the FMDN extension:

* API functions used to register callbacks:

  * The :c:func:`bt_fast_pair_fmdn_info_cb_register` function (optional)
  * The :c:func:`bt_fast_pair_fmdn_ring_cb_register` function (mandatory with the Kconfig configuration for at least one ringing component)
  * The :c:func:`bt_fast_pair_fmdn_read_mode_cb_register` function (optional)

* The :c:func:`bt_fast_pair_fmdn_id_set` API function used for assigning Bluetooth identity to FMDN activities (like advertising and connections)
* The :c:func:`bt_fast_pair_factory_reset` API function used for performing factory reset of all Fast Pair data

.. _ug_bt_fast_pair_prerequisite_ops_fmdn_clock_svc:

Beacon clock service
--------------------

Once you have successfully activated the Fast Pair module using the :c:func:`bt_fast_pair_enable` function, the FMDN extension starts the beacon clock service.
The beacon clock service runs in the background and uses the system workqueue to periodically store the clock information in the non-volatile memory (NVM).
To adjust the clock store interval, use the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_CLOCK_NVM_UPDATE_TIME` Kconfig option.
The service is used to measure time in seconds as a sum of two components: the system uptime (see :c:func:`k_uptime_get`) and beacon clock value as read from the non-volatile memory during the system boot.

Once the Provider is provisioned, it is important to keep the beacon clock synchronized with its counterpart value on the Seeker side.
The clock drift is the difference between the beacon clock value as measured by the Seeker and the Provider.
The beacon clock is used to calculate the Ephemeral Identifier (EID), which is a part of the FMDN advertising payload.
Seekers identify and track the provisioned Provider by analyzing the broadcasted EIDs in the advertising frames.
Performing frequent system reboots or staying in the turned off state (for example, System OFF) may cause the clock drift to accumulate overtime.
If the clock drift is too high, the Provider EID encoded in the FMDN advertising payload becomes unidentifable to Seeker devices.

When you disable the FMDN extension using the :c:func:`bt_fast_pair_disable` function, the beacon clock service also gets terminated.
As a result, the clock information is no longer updated in the non-volatile memory.

.. caution::
   It is not recommended to persist in the disabled state for too long with the provisioned Provider, as your device may accumulate a significant clock drift on a power loss or reboot event.

.. _ug_bt_fast_pair_prerequisite_ops_fmdn_dult_integration:

DULT integration
----------------

The FMDN extension uses the :ref:`dult_readme` module to satisfy the requirements from the DULT specification.
This guide describes the steps necessary to integrate the FMDN extension with the DULT module.
For more details on the DULT integration, see the :ref:`ug_dult` documentation.

The DULT support for the FMDN extension is controlled by the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` Kconfig option.
This option is enabled by default.
The DULT support is required for small and not easily discoverable accessories, and is recommended for large accessories.

The FMDN extension registers itself as a DULT user during the :c:func:`bt_fast_pair_enable` function call and unregisters itself during :c:func:`bt_fast_pair_disable` function call.
If you have multiple DULT users in your application, you must ensure that there is only one DULT user registered at a time.

The FMDN extension passes accessory information parameters to the DULT module during the registration process.
These parameters are used for the FMDN extension in the DULT module and are configured by the following Kconfig options:

* :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_MANUFACTURER_NAME` - The manufacturer name parameter
* :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_MODEL_NAME` - The model name parameter
* :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_ACCESSORY_CATEGORY` - The accessory category parameter
* :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MAJOR`, :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MINOR` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_REVISION` - The firmware version parameter

For more details on how to set these Kconfig options, refer to the `Fast Pair Unwanted Tracking Prevention Guidelines`_ documentation.

Subsequent sections for the FMDN extension describe further steps for integrating the DULT module once the DULT user is registered and the DULT module is successfully enabled during the :c:func:`bt_fast_pair_enable` function call.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_advertising:

Setting up Bluetooth LE advertising
***********************************

The Fast Pair Provider must include Fast Pair service advertising data in the advertising payload.
The Fast Pair Seeker must also know the Provider's transmit power to determine proximity.

Advertising data and parameters
===============================

The Fast Pair service implementation provides API to generate the advertising data for both discoverable and not discoverable advertising:

:c:func:`bt_fast_pair_adv_data_size`, :c:func:`bt_fast_pair_adv_data_fill`
  These functions are used to check the buffer size required for the advertising data and fill the buffer with data.
  Managing memory used for the advertising packets is a responsibility of the application.
  Make sure that these functions are called by the application from the cooperative context to ensure that not discoverable advertising data generation is not preempted by an Account Key write operation from a connected Fast Pair Seeker.
  Account Keys are used to generate not discoverable advertising data.

:c:func:`bt_fast_pair_set_pairing_mode`
  This function is to be used to set pairing mode before the advertising is started.

Since you control the advertising, make sure to use advertising parameters consistent with the specification.
The Bluetooth privacy is selected by the Fast Pair service, but you must make sure that the following requirements are met:

* The Resolvable Private Address (RPA) rotation is synchronized with the advertising payload update during the not discoverable advertising.
* The Resolvable Private Address (RPA) address is not rotated during discoverable advertising session.

See the official `Fast Pair Advertising`_ documentation for detailed information about the requirements related to discoverable and not discoverable advertising.

Fast Pair advertising data provider
===================================

The Fast Pair :ref:`advertising data provider <bt_le_adv_prov_readme>` (:kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR`) can be used to manage the Fast Pair advertising data.
See :ref:`fast_pair_input_device` for an example of using the provider in a sample.
See :file:`subsys/bluetooth/adv_prov/providers/fast_pair.c` for provider implementation.

Advertising TX power
====================

The Fast Pair Seeker must know the TX power of the Provider to determine proximity.
The TX power can be provided in one of the following ways:

* Defined during model registration
* Included in the advertising payload

See the `Fast Pair TX power`_ documentation for more information.

.. _ug_bt_fast_pair_advertising_tx_power_provider:

Advertising data provider
-------------------------

If your application uses :ref:`bt_le_adv_prov_readme`, you can use the TX power advertising data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER`) to read the advertising TX power from Bluetooth controller and add it to the generated advertising data.
The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` option can be used to define a TX power correction value that is added to the TX power readout included in the advertising data.
The option can be used to take into account hardware configuration, for example, used antenna and device casing.
See :ref:`fast_pair_input_device` sample for an example of how to use the TX power advertising provider.

Multiprotocol Service Layer front-end module (MPSL FEM)
-------------------------------------------------------

If your application uses MPSL :ref:`nrfxlib:mpsl_fem`, you can use a front-end module power model.
The power model allows you to control the TX power more accurately and compensate, for example, for external conditions.
See the TX power split using models section of the :ref:`nrfxlib:mpsl_fem` documentation for more details.
See the MPSL FEM power model section in :ref:`nrfxlib:mpsl_api` for API documentation.

Battery Notification extension
==============================

You can include special battery data in a not discoverable advertising packet using the Fast Pair Battery Notification extension.
To use this extension, ensure the following:

#. Call the :c:func:`bt_fast_pair_battery_set` function to provide battery information.
#. Set :c:member:`bt_fast_pair_not_disc_adv_info.battery_mode` in :c:struct:`bt_fast_pair_adv_config` to either :c:enum:`BT_FAST_PAIR_ADV_BATTERY_MODE_SHOW_UI_IND` or :c:enum:`BT_FAST_PAIR_ADV_BATTERY_MODE_HIDE_UI_IND` to include the battery notification in the generated advertising payload.

See the `Fast Pair Battery Notification extension`_ documentation for more details about this extension.

.. _ug_bt_fast_pair_advertising_fmdn:

FMDN extension
==============

The FMDN extension requires an independent advertising set for location tracking operations.
This advertising set hosts the FMDN payload as defined in the FMDN Accessory specification.
The tracking protocol uses the Bluetooth LE Extended Advertising Zephyr API (:kconfig:option:`CONFIG_BT_EXT_ADV`) to support simultaneous broadcast of advertising sets, which are managed by the application and the FMDN advertising set.
The extension manages the FMDN advertising set without the user's assistance in the following ways:

* It creates (:c:func:`bt_le_ext_adv_create`) and deletes (:c:func:`bt_le_ext_adv_delete`) the FMDN advertising set.
  When the extension is enabled, you must reserve one Bluetooth advertising set from the Bluetooth advertising set pool (:kconfig:option:`CONFIG_BT_EXT_ADV_MAX_ADV_SET`).
  If all advertising sets are reserved for other purposes, the :c:func:`bt_le_ext_adv_create` function fails to create the FMDN advertising set.
* It starts (:c:func:`bt_le_ext_adv_start`) and stops (:c:func:`bt_le_ext_adv_stop`) the FMDN advertising.
  The extension starts the FMDN advertising after a successful FMDN provisioning process and stops it after a successful unprovisioning process.
  Once provisioned, the Provider keeps advertising until Seekers use all connection slots (:kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_MAX_CONN`) by connecting to the FMDN advertising set.
  You must reserve :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_MAX_CONN` connection slots from the Bluetooth connection pool (:kconfig:option:`CONFIG_BT_MAX_CONN`).
* It sets the advertising TX power for the FMDN advertising set using the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_TX_POWER` Kconfig option.
  The configuration is independent from the configuration option that is available for the standard Fast Pair advertising payload.
  FMDN connections also use the same TX power as the FMDN advertising set.
  The connection TX power is inherited from the advertising set that was used to establish it.
  Ensure that the chosen TX power configuration is supported by your hardware setup.
* After successful FMDN provisioning, it controls the Resolvable Private Address (RPA) rotation process for the whole Zephyr Bluetooth subsystem.
  The extension sets the RPA timeout using the :c:func:`bt_le_set_rpa_timeout` function to match the timeout for the FMDN advertising payload update.
  The timeout is slightly different in each interval because it consists of a random component.
  The random part is used to improve the privacy properties of the protocol.
  In some cases, the extension triggers the RPA rotation asynchronously using the :c:func:`bt_le_oob_get_local` function.
  Such asynchronous RPA rotations currently happen right after successful FMDN provisioning or when advertising is started after a period of inactivity (for example, due to unavailable connection slots).

  If you have any other advertising set in your application that contains unique device data in its advertising payload (for example, random nonce or identifiers), you must synchronize updates to their payload and RPA address with the FMDN advertising set.
  Otherwise, the Bluetooth LE advertising process could potentially leak the privacy of your device.
  The Fast Pair not discoverable advertising payload is an example of a payload that needs to be updated in synchronization with the FMDN payload.

Even though the FMDN advertising is controlled by the extension, you must still manage the Fast Pair advertising process in your application.
To comply with the requirements of the FMDN extension, you must manage the Fast Pair advertising payload as part of application's advertising set using the Bluetooth LE Extended Advertising Zephyr API.
When creating the Fast Pair advertising set with the :c:func:`bt_le_ext_adv_create` function, register the :c:struct:`bt_le_ext_adv_cb` structure with the following callbacks:

* The :c:member:`bt_le_ext_adv_cb.connected` callback to track connections that are part of the application's connection pool (and were not created from the FMDN advertising set).
* The ``bt_le_ext_adv_cb.rpa_expired`` callback to synchronize the update of the application's advertising sets' payloads together with their respective Resolvable Private Addresses (RPA).

.. Important::
   You must manage application advertising sets according to the FMDN provisioning state:

   * For the provisioned device, only update the Fast Pair advertising payload during the ``bt_le_ext_adv_cb.rpa_expired`` callback execution.
     The FMDN extension controls the RPA rotation time in this state, and no other module in your application is allowed to change the rotation time.
   * For the unprovisioned device, control the Fast Pair advertising rotation time using the :c:func:`bt_le_set_rpa_timeout` and :c:func:`bt_le_oob_get_local` functions.
     You must still comply with the requirements of the Fast Pair protocol.

   The provisioning state is indicated by the :c:member:`bt_fast_pair_fmdn_info_cb.provisioning_state_changed` callback.
   See :ref:`ug_bt_fast_pair_gatt_service_fmdn_info_callbacks` for more details.

See the :ref:`fast_pair_locator_tag` sample that demonstrates how to comply with the rules described in this section.

Bluetooth identity
------------------

To set the Bluetooth identity for FMDN advertising and connections, use the :c:func:`bt_fast_pair_fmdn_id_set` function.
The Bluetooth identity cannot be updated if the Fast Pair module is in the *ready* state (see the :c:func:`bt_fast_pair_is_ready` function).
The extension uses the :c:macro:`BT_ID_DEFAULT` identity by default.

Advertising interval
--------------------

To configure the advertising interval for the FMDN advertising set, use the :c:func:`bt_fast_pair_fmdn_adv_param_set` function.
You can change the advertising interval even when the FMDN advertising is active.
By default, the FMDN advertising interval is set to two seconds, which is the maximum possible value.

.. note::
   The advertising interval configuration has a significant impact on the battery life of your product.
   It also affects the time necessary to establish a new connection from the FMDN advertising set.

The FMDN Accessory specification determines the recommended ratio between the Fast Pair and FMDN frames in the `Fast Pair FMDN advertising`_ documentation section.
To follow this recommendation, the application is responsible for adjusting the advertising interval of both the FMDN and Fast Pair advertising sets.

.. _ug_bt_fast_pair_advertising_fmdn_battery:

Battery level indication
------------------------

To specify the battery level broadcasted in the FMDN advertising payload, use the :c:func:`bt_fast_pair_fmdn_battery_level_set` function.
You can update the battery level asynchronously without having to wait on the ``bt_le_ext_adv_cb.rpa_expired`` callback.

The current API accepts the battery level as a percentage value, and ranges from 0% to 100%.
This percentage value is first translated according to the quantified battery states defined in the FMDN Accessory specification and then encoded in the FMDN advertising set according to the following rules:

* Normal battery level - The battery level is higher than the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_LOW_THR` Kconfig option threshold and less than or equal to 100%.
* Low battery level - The battery level is higher than the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_CRITICAL_THR` Kconfig option threshold and less than or equal to the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_LOW_THR` Kconfig option threshold.
* Critically low battery level (battery replacement needed soon) - The battery level is higher than or equal to 0% and less than or equal to the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_CRITICAL_THR` Kconfig option threshold.
* Battery level indication unsupported (default setting on bootup) - Occurs when the special :c:macro:`BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE` value is passed to the :c:func:`bt_fast_pair_fmdn_battery_level_set` function.
  This battery level is unavailable when the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` Kconfig option is enabled.

You can change the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_LOW_THR` and the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_CRITICAL_THR` Kconfig options to control the mapping of the battery percentage values to the battery levels as defined by the FMDN Accessory specification.
The mapping is implementation-specific and is up to application developer to select threshold values that fit their application requirements.

If an application does not specify the battery level using the API, the default level, battery level indication unsupported, is encoded in the FMDN advertising payload.

In case the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` Kconfig is enabled, you must initialize battery level with this API before you enable Fast Pair with the :c:func:`bt_fast_pair_enable` function.
This requirement is necessary as the DULT battery mechanism does not support unknown battery levels.
As a result, you must not call this API with the :c:macro:`BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE` value in this configuration variant.

If you want to support the battery information also in the DULT module, follow the instructions in the :ref:`ug_bt_fast_pair_gatt_service_fmdn_battery_dult` section.

Elliptic curve configuration
----------------------------

The key field in the FMDN advertising payload is the Ephemeral Identifier (EID).
The extension calculates the EID using elliptic curve cryptography.
You can choose one of the supported elliptic curves for the EID calculation:

* The secp160r1 elliptic curve configuration (:kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_ECC_SECP160R1`):

  * The FMDN advertising frames use the legacy PDU type (ADV_IND).
    The legacy advertising is understood by a wider range of devices than the extended advertising (higher adoption).
  * The 160-bit curve is less secure than the 256-bit curve.
  * The EID is 20 bytes long.

* The secp256r1 elliptic curve configuration (:kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_ECC_SECP256R1`):

  * The FMDN advertising frames use the extended advertising PDU type (ADV_EXT_IND).
    The extended advertising is understood by a smaller range of devices than the legacy advertising (lower adoption).
  * The 256-bit curve is more secure than the 160-bit curve.
  * The EID is 32 bytes long.

By default, the FMDN extension uses the secp160r1 elliptic curve configuration.

TX power
--------

The Fast Pair Seeker receives the calibrated TX power from the Provider during the FMDN provisioning process and uses it to measure distance based on the RSSI value.
The Provider includes the calibrated TX power value in the Read Beacon Parameters response.
Typically, the Seeker displays different status messages based on the measured distance when its user is trying to find the Provider device.
For example, the "It's here" status message is displayed in the "Hot & Cold" experience of the `Find My Device app`_ when the missing device is in very close proximity of the smartphone.

You can set the TX power for the FMDN advertising and connections using the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_TX_POWER` Kconfig option.
The configured value is directly used to set the TX power in the Bluetooth LE controller using an HCI command.
By default, 0 dBm is used for the FMDN TX power configuration.

You can use the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_TX_POWER_CORRECTION_VAL` Kconfig option to define a correction value that is added to TX power readout from the Bluetooth LE controller (usually equal to the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_TX_POWER` Kconfig option), when calculating the calibrated TX power reported in the Read Beacon Parameters response.
The hardware configuration, for example used antenna and device casing, may affect the actual TX power of packets broadcasted by the Fast Pair Provider.
The correction value allows to improve the accuracy of the Fast Pair Seeker's distance measurement.
The calculated calibrated TX power should range between -100 dBm and 20 dBm.

You need to adjust the correction value for both the FMDN extension and the TX power AD type in the Fast Pair advertising set.
If your application uses the :ref:`bt_le_adv_prov_readme` library, see the :ref:`ug_bt_fast_pair_advertising_tx_power_provider` section for details on how to configure the TX power AD type in the Fast Pair advertising set.
Otherwise, make sure to encode the calibrated TX power in the TX power AD type of the Fast Pair advertising set.

To select proper correction values, use the ``Calibration`` test from the ``FAST PAIR`` test category that is available in the `Fast Pair Validator app`_.

.. tip::
   If you plan to use a different TX power configuration for the FMDN extension than for the Fast Pair advertising set, you need to perform individual calibration for the extension to select a proper correction value.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_gatt_service:

Interacting with GATT service
*****************************

The Fast Pair GATT service is implemented by the :ref:`bt_fast_pair_readme`.
The service implements functionalities required by the `Fast Pair Procedure`_.
The procedure is initiated by the Fast Pair Seeker after Bluetooth LE connection is established.
No application interaction is required.

The Fast Pair GATT service is statically defined, so it is still present in the GATT database after the Fast Pair subsystem is disabled.
In the Fast Pair subsystem disabled state, GATT operations on the Fast Pair service are rejected.

The Fast Pair GATT service modifies default values of related Kconfig options to follow Fast Pair requirements.
The service also enables the needed functionalities using Kconfig select statement.
For details, see the :ref:`bt_fast_pair_readme` Bluetooth service documentation in the |NCS|.

.. _ug_bt_fast_pair_gatt_service_no_ble_pairing:

Procedure without Bluetooth pairing
===================================

The Fast Pair specification allows the `Fast Pair Procedure`_ to operate in a special mode.
In this mode, the Provider and Seeker skip the steps that involve Bluetooth pairing and bonding.
In this case, the `Fast Pair Procedure`_ is only used to pass the Account Key from the Seeker to the Provider device.

You can disable the :kconfig:option:`CONFIG_BT_FAST_PAIR_REQ_PAIRING` configuration option to support the `Fast Pair Procedure`_  without Bluetooth pairing and bonding.
By default, the :kconfig:option:`CONFIG_BT_FAST_PAIR_REQ_PAIRING` configuration option is enabled, and the standard mode of the procedure is required by the Provider.

Using the information callbacks
===============================

To register the information callbacks, use the :c:func:`bt_fast_pair_info_cb_register` function.

All Account Key writes are indicated by the :c:member:`bt_fast_pair_info_cb.account_key_written` callback.
This callback is optional to register and is triggered on a successful Account Key write operation over the Account Key characteristic.

The typical use case of this callback is to have a notification mechanism that informs you about any updates to the Account Key storage.
You may decide to use the Fast Pair not discoverable advertising mode on the first Account Key write or update this type of advertising payload on subsequent Account Key writes.
In the Fast Pair not discoverable advertising mode, the Provider informs the listening Seeker devices about all Account Keys that it has stored so far.
You can also use the :c:func:`bt_fast_pair_has_account_key` function to check whether your Provider has any Account Keys.
This API is especially useful after a system reboot when some Account Keys may already be stored in non-volatile memory.

FMDN extension
==============

The FMDN extension defines a new characteristic inside the Fast Pair service.
The new characteristic is called Beacon Actions and is used to exchange extension-related messages between the Seeker and the Provider.

.. _ug_bt_fast_pair_gatt_service_fmdn_info_callbacks:

Using the information callbacks
-------------------------------

Register the information callbacks in the FMDN extension using the :c:func:`bt_fast_pair_fmdn_info_cb_register` function.
This callback registration is optional.
You can register multiple callback sets using the :c:func:`bt_fast_pair_fmdn_info_cb_register` function.

This function supports the following callbacks:

* :c:member:`bt_fast_pair_fmdn_info_cb.provisioning_state_changed` -  Notification about the provisioning state update
* :c:member:`bt_fast_pair_fmdn_info_cb.clock_synced` - Notification about the beacon clock synchronization event

The provisioning state is indicated by the :c:member:`bt_fast_pair_fmdn_info_cb.provisioning_state_changed` callback.
This callback is triggered in the following scenarios:

* Right after the :c:func:`bt_fast_pair_enable` enable operation to indicate the initial provisioning state of the extension.
* On the successful provisioning operation over Beacon Actions characteristic.
* On the successful unprovisioning operation over Beacon Actions characteristic.

The provisioning state callback is used to notify the application about switching to a proper advertising policy.
The advertising policies are extensively described in the :ref:`Setting up Bluetooth LE advertising <ug_bt_fast_pair_advertising>` section of this integration guide.

The clock synchronization is indicated by the :c:member:`bt_fast_pair_fmdn_info_cb.clock_synced` callback.
This callback is triggered on a successful beacon clock read operation over Beacon Actions characteristic.

A typical use case for this callback is to synchronize the beacon clock after the system reboot of the accessory (for example, due to battery replacement).
In this case, the affected device might have stayed in the power-down state for an unknown period of time.
As a result, the beacon clock drift may become so high that the Ephemeral Identifier (EID) from the FMDN advertising payload is no longer recognized by the Seeker devices.
As a fallback mechanism for clock synchronization, the accessory must simultaneously advertise the Fast Pair not discoverable and FMDN payloads right after the system reboot.
The Fast Pair advertising frames make the affected Provider visible to nearby Seekers.
Once one of the Seeker devices connects to the accessory and synchronizes the clock, the :c:member:`bt_fast_pair_fmdn_info_cb.clock_synced` callback is called to indicate that the Provider is no longer required to advertise the Fast Pair payload.

.. _ug_bt_fast_pair_gatt_service_fmdn_read_mode_callbacks:

Using the read mode callbacks and managing the read mode state
--------------------------------------------------------------

The FMDN extension defines special read modes, in which sensitive data can be read from the device by the connected peer.
The read mode persists only for limited time after which it is deactivated.

To enter the chosen read mode, you must call the :c:func:`bt_fast_pair_fmdn_read_mode_enter` function and pass the supported read mode type as a function parameter.
You can only call this function in the ready state of the Fast Pair module (see the :c:func:`bt_fast_pair_is_ready` function) and in the FMDN provisioned state (see the :c:member:`bt_fast_pair_fmdn_info_cb.provisioning_state_changed` callback).
The FMDN extension supports the following read mode types:

* :c:enum:`BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY` - Ephemeral Identity Key (EIK) recovery mode
* :c:enum:`BT_FAST_PAIR_FMDN_READ_MODE_DULT_ID` - Identification mode
  This mode is available only when the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` Kconfig option is enabled.

To register the read mode callbacks, use the :c:func:`bt_fast_pair_fmdn_read_mode_cb_register` function.
Callback registration is optional.
You can register only one callback set with this function, as the subsequent call overrides the previous set.
The :c:func:`bt_fast_pair_fmdn_read_mode_cb_register` function supports currently only one callback type, :c:member:`bt_fast_pair_fmdn_read_mode_cb.exited`, that provides a notification when the specific read mode is over.
Read mode exit occurs when the read mode naturally times out or when it is forcefully canceled (for example, during the :c:func:`bt_fast_pair_disable` function call).

When the device is already in the selected read mode, you can call the :c:func:`bt_fast_pair_fmdn_read_mode_enter` function with the same read mode type to prolong its timeout.

The :c:enum:`BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY` read mode type is called EIK recovery mode.
This mode is mandatory to support, as it enables the EIK recovery operation.
The FMDN extension validates if the mode is active during the EIK read operation over Beacon Actions characteristic.
This read operation is accepted only when the device is in the recovery mode.
It is recommended to enter this mode after a user interaction (for example, a button press).
This physical interaction constitutes user consent to activate the recovery mode.
You can configure the timeout of the recovery mode using the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY_TIMEOUT` Kconfig option.

The :c:enum:`BT_FAST_PAIR_FMDN_READ_MODE_DULT_ID` read mode type is called identification mode.
This mode is only available when the DULT integration with the FMDN extension is enabled with the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` Kconfig option.
In this configuration, your application must implement support for this mode.
The identification mode allows for reading the Identifier Payload defined in the Detecting Unwanted Location Trackers (DULT) specification.
This read operation is accepted only when the device is in the identification mode.
For more details on the Identifier Payload in the DULT module, see the :ref:`ug_dult_identifier` documentation.
It is recommended to enter this mode after a user interaction (for example, a button press).
This physical interaction constitutes user consent to activate the identification mode.
Apart from that, the device should emit visual or audio signal to indicate mode activation.
The timeout of the identification mode is equal to five minutes according to the DULT specification requirements and cannot be configured by the user.
For more details on the identification mode, refer to the `Fast Pair Unwanted Tracking Prevention Guidelines`_ documentation.

.. _ug_bt_fast_pair_gatt_service_fmdn_ring_callbacks:

Using the ringing callbacks and managing the ringing state
----------------------------------------------------------

Select the number of ringing components that you want to support in your application configuration (see the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_COMP` choice configuration).
You can only pick one of the following options:

* :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_NONE`: No component is capable of ringing (the default choice).
* :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_ONE`: One component is capable of ringing.
* :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_TWO`: Two components (left and right buds) are capable of ringing.
* :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_THREE`: Three components (left and right buds and case) are capable of ringing.

Apart from the ringing component configuration, you can enable support for the ringing volume feature by setting the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_VOLUME` option.
When this option is enabled, you should be able to individually set the volume for all of your ringing components, choosing from the three provided levels.
For some devices, however, volume adjustment options may not be available.
In such a case, you should keep the ringing volume feature disabled and use the default volume (:c:enum:`BT_FAST_PAIR_FMDN_RING_VOLUME_DEFAULT`) for all declared ringing components.

To adjust volume levels for the devices that support the feature, use the following options:

* Low (:c:enum:`BT_FAST_PAIR_FMDN_RING_VOLUME_LOW`)
* Medium (:c:enum:`BT_FAST_PAIR_FMDN_RING_VOLUME_MEDIUM`)
* High (:c:enum:`BT_FAST_PAIR_FMDN_RING_VOLUME_HIGH`)

If your application configuration supports at least one ringing component, you must register the ringing callbacks using the :c:func:`bt_fast_pair_fmdn_ring_cb_register` function.
In this case, all ringing callbacks defined in the :c:struct:`bt_fast_pair_fmdn_ring_cb` structure are mandatory to register.

Ringing callbacks pass the information about the source that triggered the ringing activity as the first parameter of the callback function.
The following sources of ringing activity are supported:

* :c:enum:`BT_FAST_PAIR_FMDN_RING_SRC_FMDN_BT_GATT` - This ringing source originates from the Bluetooth Fast Pair service and its Beacon Actions characteristic that is defined in the FMDN Accessory specification.
* :c:enum:`BT_FAST_PAIR_FMDN_RING_SRC_DULT_BT_GATT` - This ringing source originates from the Bluetooth Accessory Non-owner service and its characteristic that are defined in the DULT specification.
  This source is available only when the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` Kconfig option is enabled.

The following callbacks are defined in the :c:struct:`bt_fast_pair_fmdn_ring_cb` structure:

* The ringing start request is indicated by the :c:member:`bt_fast_pair_fmdn_ring_cb.start_request` callback.
  The connected peer can trigger the callback by sending the relevant request message over supported data channel.

  The :c:struct:`bt_fast_pair_fmdn_ring_req_param` structure that is passed in the :c:member:`bt_fast_pair_fmdn_ring_cb.start_request` callback determines the following request parameters as specified by the requesting peer:

    * Bitmask with ringing component identifiers that are requested to start ringing.
    * Ringing timeout in deciseconds.
      The timeout value of the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_REQ_TIMEOUT_DULT_BT_GATT` Kconfig option is used for the :c:enum:`BT_FAST_PAIR_FMDN_RING_SRC_DULT_BT_GATT` DULT source.
      The default value of this Kconfig is in line with the `Fast Pair Unwanted Tracking Prevention Guidelines`_ documentation.
    * Ringing volume level.

  The :c:member:`bt_fast_pair_fmdn_ring_cb.start_request` callback can be called again when the ringing action has already started.
  In this case, you must update the ringing activity to match the newest set of parameters.

* The ringing timeout is indicated by the :c:member:`bt_fast_pair_fmdn_ring_cb.timeout_expired` callback.
  The extension triggers the callback when the ringing timeout expires on the device.

* The ringing stop request is indicated by the :c:member:`bt_fast_pair_fmdn_ring_cb.stop_request` callback.
  The connected peer can trigger the callback by sending the relevant request message over supported data channel.

You must treat all callbacks from the :c:struct:`bt_fast_pair_fmdn_ring_cb` structure as requests.
The internal ringing state of the extension is not automatically changed on any callback event.
The state is only changed when you acknowledge such a request in your application using the :c:func:`bt_fast_pair_fmdn_ring_state_update` function.

.. note::
   The ringing timeout countdown starts once you report a start or restart of the ringing action using the :c:func:`bt_fast_pair_fmdn_ring_state_update` function.

You must call the :c:func:`bt_fast_pair_fmdn_ring_state_update` function whenever the bitmask with active ringing components changes due to the extension initiated operations.
Additionally, you must also call this function in response to the :c:member:`bt_fast_pair_fmdn_ring_cb.start_request` and the :c:member:`bt_fast_pair_fmdn_ring_cb.stop_request` callbacks in case of a failure (no bitmask change).
A call to the :c:func:`bt_fast_pair_fmdn_ring_state_update` function sends a message with the ringing state update.
The message is sent over the ringing source that is used by the connected peer.

You must select the ringing source that is passed to the :c:func:`bt_fast_pair_fmdn_ring_state_update` function as a first parameter.
Typically, you pass the ringing source that is used in the last ringing callback that triggerred the ringing state update.
In certain edge cases, you can get two simultaneous requests to start ringing with two different sources before you are able to indicate the start of ringing with the :c:func:`bt_fast_pair_fmdn_ring_state_update` function.
In this situation, you need to select the preferred ringing source.

You must also configure the following fields in the :c:struct:`bt_fast_pair_fmdn_ring_state_param` structure that is passed to the :c:func:`bt_fast_pair_fmdn_ring_state_update` function as a second parameter:

* Trigger for the new ringing state:

  * Started (:c:enum:`BT_FAST_PAIR_FMDN_RING_TRIGGER_STARTED`): set in response to the :c:member:`bt_fast_pair_fmdn_ring_cb.start_request` callback when at least one component from the requested set has started to ring.
  * Failed (:c:enum:`BT_FAST_PAIR_FMDN_RING_TRIGGER_FAILED`):

    * Set in response to the :c:member:`bt_fast_pair_fmdn_ring_cb.start_request` callback when not even one component from the requested set has started to ring.
    * Set in response to the :c:member:`bt_fast_pair_fmdn_ring_cb.stop_request` callbacks when not even one component has stopped ringing.

  * Stopped on timeout (:c:enum:`BT_FAST_PAIR_FMDN_RING_TRIGGER_TIMEOUT_STOPPED`): set in response to the :c:member:`bt_fast_pair_fmdn_ring_cb.timeout_expired` callback when at least one component has stopped ringing.
  * Stopped on a button press (:c:enum:`BT_FAST_PAIR_FMDN_RING_TRIGGER_UI_STOPPED`): set in response to the application-specific button press when at least one component has stopped ringing.
  * Stopped on a GATT request (:c:enum:`BT_FAST_PAIR_FMDN_RING_TRIGGER_GATT_STOPPED`): set in response to the :c:member:`bt_fast_pair_fmdn_ring_cb.stop_request` callback when at least one component has stopped ringing.

* Bitmask with currently ringing components.
* Remaining ringing timeout in deciseconds (can be set to zero to preserve the existing timeout).

If you cannot start the ringing action on all requested components (for example, one of them is out of range), you must set the started trigger and update the connected peer with a ringing state update using the :c:func:`bt_fast_pair_fmdn_ring_state_update` function.
Once an unavailable component becomes reachable, you can start the delayed ringing action on it and send another update to the connected peer with the :c:func:`bt_fast_pair_fmdn_ring_state_update` function.
Alternatively, you can also ignore it and exclude it altogether from the current ringing action.

Handle a partially executed ringing stop request (with at least one of the components still ringing) in the similar way.
This update policy applies to all listed stop trigger types.

To satisfy the requirements from the DULT specification when the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` Kconfig option is enabled, the FMDN extension communicates with the DULT module to receive ringing requests from the DULT peers and to send updates regarding the ringing state.
For more details on the ringing mechanism in the DULT module, see the :ref:`ug_dult_sound` documentation.

.. _ug_bt_fast_pair_gatt_service_fmdn_battery_dult:

Battery information with DULT
-----------------------------

Enable the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` Kconfig option to pass the battery level information from the FMDN extension to the DULT module.
You need to have the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` Kconfig option enabled that indicates the general DULT integration in the FMDN extension.

The battery level information is passed using the :c:func:`bt_fast_pair_fmdn_battery_level_set` function.
For more details on how to use this function, see the :ref:`ug_bt_fast_pair_advertising_fmdn_battery` section.
Similarly to the FMDN battery level indication feature, the DULT module uses Kconfig options to map percentage values to battery levels that are defined in the DULT specification.
For more details on how to use these Kconfig options, see the :ref:`ug_dult_battery` documentation.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_factory_reset:

Managing factory resets
***********************

The Fast Pair GATT service uses a non-volatile memory to store the Fast Pair user data such as Account Keys and the Personalized Name.
This data can be cleared by calling the :c:func:`bt_fast_pair_factory_reset` function.
Calling the :c:func:`bt_fast_pair_factory_reset` function does not affect the Fast Pair subsystem's readiness.
If the subsystem is enabled with the :c:func:`bt_fast_pair_enable` function, it stays enabled after calling the :c:func:`bt_fast_pair_factory_reset` function.
The same applies for the Fast Pair subsystem disabled state.
For details, see the :c:func:`bt_fast_pair_factory_reset` function documentation.

.. _ug_bt_fast_pair_factory_reset_custom_user_reset_action:

Custom user reset action
========================

Use the :kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_USER_RESET_ACTION` Kconfig option to enable a custom user reset action that executes together with the factory reset operation.
To define the custom user reset action, you need to implement the ``bt_fast_pair_factory_reset_user_action_perform`` function in your application code.
Optionally, you can also define the ``bt_fast_pair_factory_reset_user_action_prepare`` function if you want an earlier notification that the reset operation is due to begin.
Both functions are defined as weak no-op functions.
Ensure that your reset action implementation executes correctly in the following execution contexts:

* In the :c:func:`bt_fast_pair_factory_reset` function context - The factory reset action is triggered by calling the :c:func:`bt_fast_pair_factory_reset` function.
* In the :c:func:`bt_fast_pair_enable` function context - The factory reset action using the :c:func:`bt_fast_pair_factory_reset` function was interrupted, and the factory reset is retried when enabling the Fast Pair subsystem.

.. caution::
   If the factory reset operation constantly fails due to an error in the custom user reset action, the system may never be able to properly boot-up.

During the custom user reset action, you can safely delete additional non-volatile data that are not owned by the Fast Pair modules.
A typical use case is to delete Bluetooth bonding information using either the :c:func:`bt_unpair` or the :c:func:`bt_id_reset` function.

For an example on how to use the custom reset action, see the :ref:`fast_pair_locator_tag` sample.

FMDN extension
==============

The FMDN extension additionally stores the following data in the non-volatile memory:

* Owner Account Key (a special Account Key with additional permissions).
* Ephemeral Identity Key (EIK).
* Beacon clock.

To perform the factory reset of all Fast Pair non-volatile data, ensure that the Fast Pair module is in the *unready* state (see the :c:func:`bt_fast_pair_is_ready` function).
In the *ready* state of the module, the :c:func:`bt_fast_pair_factory_reset` function does not perform a factory reset and returns with an error.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_use_case:

Tailoring protocol for specific use case
****************************************

The specific use case of the Google Fast Pair application is indicated by the chosen device type in the Google Nearby Devices console (see the :ref:`ug_bt_fast_pair_provisioning_register_device_type` subsection).
In the official `Fast Pair`_ documentation, the `Fast Pair Device Feature Requirements`_ category defines additional requirements for each supported use case, and specifies a list of mandatory, optional, and unsupported Fast Pair features.
If your product is targeting one of the listed use cases, you must align your accessory firmware to meet these requirements.

To learn about the software maturity levels for Google Fast Pair use cases supported by the |NCS|, see the :ref:`software_maturity_fast_pair_use_case` table.

Locator tag
===========

Locator tag is a small electronic device that can be attached to an object or a person, and is designed to help locate them in case they are missing.
The locator tags can use different wireless technologies like GPS, Bluetooth LE or UWB for location tracking.
It is even possible to combine multiple technologies in a single product to improve the user experience.

The `Fast Pair Device Feature Requirements for Locator Tags`_ documentation defines the Fast Pair requirements for the locator tag use case.
If your product is targeting the locator tag use case, you must configure your application according to these requirements.
Enable the mandatory Fast Pair features and extensions using the appropriate Kconfig options in your application's configuration.
For the reference configuration of the `Fast Pair Device Feature Requirements for Locator Tags`_  specification, see the :ref:`fast_pair_locator_tag` sample.

The `Fast Pair Device Feature Requirements for Locator Tags`_ documentation refers to the `Fast Pair Locator Tag Specific Guidelines`_ section from the FMDN Accessory specification.
You must implement the guidelines at application level as they cannot be automatically handled by the Fast Pair subsystem.
Implement these guidelines in your application if your product is targeting the locator tag use case.
To see how to implement `Fast Pair Locator Tag Specific Guidelines`_ , see the :ref:`fast_pair_locator_tag` sample.

You should declare support for the locator tag use case during the device registration process in the Google Nearby Device console.
To activate the support, populate the **Fast Pair** protocol configuration panel in the following order:

#. Select :guilabel:`Locator Tag` option in the **Device Type** list.
#. Set the **Find My Device** feature to **true**.

.. note::
   It is recommended to use the special mode of the ``Fast Pair Procedure`` for the locator tag use case (see :ref:`ug_bt_fast_pair_gatt_service_no_ble_pairing` for more details).
   The Bluetooth bonding information can cause connection establishment issues and delays on some Android devices.

Applications and samples
************************

The following application and sample use the Fast Pair integration in the |NCS|:

* :ref:`nrf_desktop` application
* :ref:`fast_pair_input_device` sample
* :ref:`fast_pair_locator_tag` sample

Library support
***************

The following |NCS| libraries support the Fast Pair integration:

* :ref:`bt_fast_pair_readme` library implements the Fast Pair GATT Service and provides the APIs required for :ref:`ug_bt_fast_pair` with the |NCS|.
* :ref:`bt_le_adv_prov_readme` library - Google Fast Pair advertising data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR`) can be used to integrate Fast Pair advertising payload to this library.
  The Bluetooth LE advertising provider subsystem can be used to manage advertising and scan response data.

Required scripts
****************

The :ref:`bt_fast_pair_provision_script` is required to generate the provisioning data for the device.
The build system calls it automatically when building with Fast Pair in the |NCS|.

Terms and licensing
*******************

The use of Google Fast Pair may be subject to Google's terms and licensing.
Refer to the official `Fast Pair`_ documentation for development-related licensing information.

Dependencies
************

The following are the required dependencies for the Fast Pair integration:

* :ref:`nrfxlib:crypto`
* :ref:`zephyr:bluetooth`
* :ref:`zephyr:settings_api`
* :ref:`partition_manager`
* :ref:`dult_readme`
