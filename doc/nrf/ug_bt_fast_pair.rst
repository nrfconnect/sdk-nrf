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
   The implementation is not yet ready for production and extensions are not supported.

   The implementation does not pass end-to-end integration tests in the Fast Pair Validator.
   The procedure triggered in Android settings is successful (tested with Android 10).

.. _ug_integrating_fast_pair:

Integrating Fast Pair
*********************

The Fast Pair standard integration in the |NCS| consists of the following steps:

1. :ref:`Provisioning the device <ug_bt_fast_pair_provisioning>`
#. :ref:`Setting up Bluetooth LE advertising <ug_bt_fast_pair_advertising>`
#. :ref:`Setting up GATT service <ug_bt_fast_pair_gatt_service>`

These steps are described in the following sections.

The Fast Pair standard implementation in the |NCS| integrates Fast Pair Provider, one of the available `Fast Pair roles`_.
For an integration example, see the :ref:`peripheral_fast_pair` sample.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_provisioning:

Provisioning the device
***********************

A device model must be registered with Google to work as a Fast Pair Provider.
The data is used for procedures defined by the Fast Pair standard.

Registering Fast Pair Provider
==============================

See the official `Fast Pair Model Registration`_ documentation for information how to register the device and obtain the Model ID and Anti-Spoofing Public/Private Key pair.

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
  Special battery data can be included in not discoverable advertising packet using the Fast Pair Battery Notification extension.
  To use this extension, you have to:

  #. Call :c:func:`bt_fast_pair_battery_set` to provide battery information.
  #. Set :c:member:`bt_fast_pair_adv_config.adv_battery_mode` to either :c:enum:`BT_FAST_PAIR_ADV_BATTERY_MODE_SHOW_UI_IND` or :c:enum:`BT_FAST_PAIR_ADV_BATTERY_MODE_HIDE_UI_IND` to include the battery notification in the generated advertising payload.

  See the `Fast Pair Battery Notification extension`_ documentation for more details about this extension.

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
See :ref:`peripheral_fast_pair` for an example of using the provider in a sample.
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
See :ref:`peripheral_fast_pair` sample for an example of how to use the TX power advertising provider.

Multiprotocol Service Layer front-end module (MPSL FEM)
-------------------------------------------------------

If your application uses MPSL :ref:`nrfxlib:mpsl_fem`, you can use a front-end module power model.
The power model allows you to control the TX power more accurately and compensate, for example, for external conditions.
See the TX power split using models section of the :ref:`nrfxlib:mpsl_fem` documentation for more details.
See the MPSL FEM power model section in :ref:`nrfxlib:mpsl_api` for API documentation.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_gatt_service:

Setting up GATT service
***********************

The Fast Pair GATT service is implemented by the :ref:`bt_fast_pair_readme`.
The service implements functionalities required by the `Fast Pair Procedure`_.
The procedure is initiated by the Fast Pair Seeker after Bluetooth LE connection is established.
No application interaction is required.

The Fast Pair GATT service modifies default values of related Kconfig options to follow Fast Pair requirements.
The service also enables the needed functionalities using Kconfig select statement.
For details, see the :ref:`bt_fast_pair_readme` Bluetooth service documentation in the |NCS|.

The Fast Pair GATT service uses a non-volatile memory to store the Fast Pair user data such as Account Keys and the Personalized Name.
This data can be cleared by calling the :c:func:`bt_fast_pair_factory_reset` function.
For details, see the :c:func:`bt_fast_pair_factory_reset` function documentation.
