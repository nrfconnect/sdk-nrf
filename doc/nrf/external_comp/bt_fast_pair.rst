.. _ug_bt_fast_pair:

Using Google Fast Pair with the |NCS|
#####################################

.. contents::
   :local:
   :depth: 2

Google Fast Pair is a standard for pairing Bluetooth® and Bluetooth Low Energy (LE) devices with as little user interaction required as possible.
Google also provides additional features built upon the Fast Pair standard.
For detailed information about supported functionalities, see the official `Fast Pair`_ documentation.

.. note::
   The Fast Pair support in the |NCS| is :ref:`experimental <software_maturity>`.
   The implementation is not yet ready for production and extensions are not fully supported.

   The implementation passes tests of `Fast Pair Validator app`_ (beta version).
   The procedure triggered in Android settings is successful (tested with Android 11).

.. _ug_fast_pair_extensions:

Google Fast Pair extensions
***************************

The Fast Pair standard implementation in the |NCS| supports the following extensions:

* Battery Notification extension
* Personalized Name extension

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

.. _ug_integrating_fast_pair:

Integrating Fast Pair
*********************

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

* the :ref:`fast_pair_input_device` sample
* the :ref:`nrf_desktop` application

.. _ug_bt_fast_pair_provisioning_register_device_type:

Device type
-----------

When registering the device in the Google Nearby Devices console, go to the **Fast Pair** protocol configuration panel, and in the **Device Type** field select an option that matches your application's use case.
The chosen device type also determines the optional feature set that you can support in your use case.
You declare support for each feature by selecting the **true** option.

.. note::
   Ensure you make an informed decision when selecting the device type, as it has a significant impact on the Fast Pair Seeker behavior in relation to your Provider's device.

The Fast Pair standard implementation in the |NCS| actively supports the following device types and use cases:

* Input device (see the :ref:`fast_pair_input_device` sample)

Provisioning registration data onto device
==========================================

The Fast Pair standard requires provisioning the device with Model ID and Anti-Spoofing Private Key obtained during device model registration.
In the |NCS|, the provisioning data is generated as a hexadecimal file using the :ref:`bt_fast_pair_provision_script`.

If Fast Pair is enabled with the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option, the Fast Pair provision script is called automatically by the build system and the resulting hexadecimal file is automatically added to the firmware (that is, to the :file:`merged.hex` file).
You must provide the following CMake options:

* :c:macro:`FP_MODEL_ID` - Fast Pair Model ID in format ``0xXXXXXX``,
* :c:macro:`FP_ANTI_SPOOFING_KEY` - base64-encoded Fast Pair Anti-Spoofing Private Key.

The ``bt_fast_pair`` partition address is provided automatically by the build system.

For example, when building an application with the |nRFVSC|, you need to add the following parameters in the **Extra CMake arguments** field on the **Add Build Configuration view**: ``-DFP_MODEL_ID=0xFFFFFF -DFP_ANTI_SPOOFING_KEY=AbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbA=``.
Make sure to replace ``0xFFFFFF`` and ``AbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbA=`` with values obtained for your device.
See :ref:`cmake_options` for more information about defining CMake options.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_prerequisite_ops:

Performing prerequisite operations
**********************************

You must enable the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option to support the Google Fast Pair standard in your project.

An application can communicate with the Fast Pair subsystem using API calls and registered callbacks.
The Fast Pair subsystem uses the registered callbacks to inform the application about the Fast Pair related events.

The application must register the callbacks before it starts to operate as the Fast Pair Provider and advertise Bluetooth LE packets.
To identify the callback registration functions in the Fast Pair API, look for the ``_register`` suffix.
Set your application-specific callback functions in the callback structure that is the input parameter for the ``..._register`` API function.
The callback structure must persist in the application memory (static declaration), as during the registration, the Fast Pair module stores only the memory pointer to it.

The standard Fast Pair API (without extensions) currently supports the :c:func:`bt_fast_pair_info_cb` function (optional) for registering application callbacks.

The standard Fast Pair (without extensions) does not require registration of any callback type, meaning all callbacks are optional.

Apart from the callback registration, no additional operations are needed to integrate the standard Fast Pair implementation.

Personalized Name extension
===========================

To support the Personalized Name extension, ensure that the :kconfig:option:`CONFIG_BT_FAST_PAIR_EXT_PN` Kconfig option is enabled in your project.
This extension is enabled by default.

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

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_gatt_service:

Interacting with GATT service
*****************************

The Fast Pair GATT service is implemented by the :ref:`bt_fast_pair_readme`.
The service implements functionalities required by the `Fast Pair Procedure`_.
The procedure is initiated by the Fast Pair Seeker after Bluetooth LE connection is established.
No application interaction is required.

The Fast Pair GATT service modifies default values of related Kconfig options to follow Fast Pair requirements.
The service also enables the needed functionalities using Kconfig select statement.
For details, see the :ref:`bt_fast_pair_readme` Bluetooth service documentation in the |NCS|.

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

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_factory_reset:

Managing factory resets
***********************

The Fast Pair GATT service uses a non-volatile memory to store the Fast Pair user data such as Account Keys and the Personalized Name.
This data can be cleared by calling the :c:func:`bt_fast_pair_factory_reset` function.
For details, see the :c:func:`bt_fast_pair_factory_reset` function documentation.

.. _ug_bt_fast_pair_factory_reset_custom_user_reset_action:

Custom user reset action
========================

Use the :kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_USER_RESET_ACTION` Kconfig option to enable a custom user reset action that executes together with the factory reset operation.
To define the custom user reset action, you need to implement the :c:func:`bt_fast_pair_factory_reset_user_action_perform` function in your application code.
Optionally, you can also define the :c:func:`bt_fast_pair_factory_reset_user_action_prepare` function if you want an earlier notification that the reset operation is due to begin.
Both functions are defined as weak no-op functions.
Ensure that your reset action implementation executes correctly in the following execution contexts:

* In the :c:func:`bt_fast_pair_factory_reset` function context: the factory reset action is triggered by calling the :c:func:`bt_fast_pair_factory_reset` function.
* In the :c:func:`settings_load` function context during the commit phase (after data is loaded from the non-volatile memory): the factory reset action using the :c:func:`bt_fast_pair_factory_reset` function was interrupted and the factory reset is retried on the system bootup.

.. caution::
   If the factory reset operation constantly fails due to an error in the custom user reset action, the system may never be able to properly boot-up.

During the custom user reset action, you can safely delete additional non-volatile data that are not owned by the Fast Pair modules.
A typical use case is to delete Bluetooth bonding information using either the :c:func:`bt_unpair` or the :c:func:`bt_id_reset` function.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_use_case:

Tailoring protocol for specific use case
****************************************

The specific use case of the Google Fast Pair application is indicated by the chosen device type in the Google Nearby Devices console (see the :ref:`ug_bt_fast_pair_provisioning_register_device_type` subsection).
Different use cases may require implementation of additional guidelines for your accessory firmware or specific configuration of your device model in the Google Nearby Devices console.
These requirements typically help to improve user experience or security properties for the chosen use case.
To learn more, see the official `Fast Pair`_ documentation.
