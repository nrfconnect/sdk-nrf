.. _bt_le_adv_prov_readme:

Bluetooth LE advertising providers
##################################

.. contents::
   :local:
   :depth: 2

The Bluetooth LE advertising providers library manages advertising data and scan response data.
It does not control Bluetooth LE advertising by itself.

Overview
********

The Bluetooth LE advertising providers library acts as a middleware between data providers and a module that controls Bluetooth advertising.
The module that controls the Bluetooth advertising can use the library's API to get advertising data and scan response data.
On the API call, the Bluetooth LE advertising providers library gets data from providers and passes the data to the module that controls Bluetooth advertising.

Providers
=========

The library gets both advertising data and scan response data from providers.
A provider manages a single element that is part of either advertising data or scan response data.

The |NCS| provides a set of predefined data providers.
An application can also define application-specific data providers.

Application-specific providers
------------------------------

An application-specific provider must implement :c:func:`bt_le_adv_prov_data_get` callback and register it using one of the following macros:

* :c:macro:`BT_LE_ADV_PROV_AD_PROVIDER_REGISTER` - The macro registers provider that appends data to advertising packets.
* :c:macro:`BT_LE_ADV_PROV_SD_PROVIDER_REGISTER` - The macro registers provider that appends data to scan response packets.

When the callback is called, the provider must fill in the Bluetooth data structure passed as a callback argument.
Apart from filling the Bluetooth data structure, the provider can also perform any of the following actions:

* Fill out the feedback structure (:c:struct:`bt_le_adv_prov_feedback`) to provide additional information to the module that controls Bluetooth LE advertising.
* Return an error code.
  The error value of ``-ENOENT`` is used to inform that provider desists from providing data.
  In that case, the provider does not fill in the Bluetooth data structure.
  This error code is not forwarded to the module that controls Bluetooth LE advertising.
  Other negative values denote provider-specific errors that are forwarded directly to the module that controls Bluetooth LE advertising.

The provided Bluetooth data can depend on any of the following options:

* Bluetooth advertising state passed as function argument (:c:struct:`bt_le_adv_prov_adv_state`).
* Calls of the provider's dedicated API, if the provider exposes such an API.

For example, a provider may fill in the Bluetooth data only if the used Bluetooth local identity has no bond.
The provider returns ``-ENOENT`` to desist from providing data if bonded.
Examples of provider implementations can be found in the :file:`subsys/bluetooth/adv_prov/providers/` folder.

Advertising control
===================

The Bluetooth LE advertising providers library does not control Bluetooth LE advertising by itself.
A separate module that controls Bluetooth advertising is mandatory.
For example, :ref:`caf_ble_adv`, that is available in the |NCS|.
An application can use this CAF module or implement an application-specific module that controls Bluetooth advertising.
The application then uses the Bluetooth LE advertising providers library to manage advertising data and scan response data.

Custom module
-------------

An application-specific module can use the following functions to get providers' advertising data:

* :c:func:`bt_le_adv_prov_get_ad_prov_cnt` to get number of advertising data providers.
* :c:func:`bt_le_adv_prov_get_ad` to fill the advertising packet with providers' data.

Similar functions are defined for scan response data (:c:func:`bt_le_adv_prov_get_sd_prov_cnt` and :c:func:`bt_le_adv_prov_get_sd`).

The module must provide :c:struct:`bt_le_adv_prov_adv_state` to inform providers about Bluetooth advertising state.
The module must also take into account providers' feedback received in :c:struct:`bt_le_adv_prov_feedback`.
See mentioned structures' documentation for detailed description of individual members.

Configuration
*************

Set the :kconfig:option:`CONFIG_BT_ADV_PROV` Kconfig option to enable the Bluetooth LE advertising providers library.

Predefined providers
====================

The |NCS| provides a set of predefined providers.
Each provider is enabled using a dedicated Kconfig option.
These options share a common Kconfig option prefix of ``CONFIG_BT_ADV_PROV_``.

Among others, the following providers are available:

* Advertising Flags (:kconfig:option:`CONFIG_BT_ADV_PROV_FLAGS`)
* GAP Appearance (:kconfig:option:`CONFIG_BT_ADV_PROV_GAP_APPEARANCE`)
* Microsoft Swift Pair (:kconfig:option:`CONFIG_BT_ADV_PROV_SWIFT_PAIR`)
* Google Fast Pair (:kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR`)
* TX Power (:kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER`)
* Bluetooth device name (:kconfig:option:`CONFIG_BT_ADV_PROV_DEVICE_NAME`)

For details about each advertising provider, see the Kconfig option description.

API documentation
*****************

| Header file: :file:`include/bluetooth/adv_prov.h`
| Source files: :file:`subsys/bluetooth/adv_prov/`

.. doxygengroup:: bt_le_adv_prov
   :project: nrf
   :members:

Fast Pair provider API
======================

| Header file: :file:`include/bluetooth/adv_prov/fast_pair.h`
| Source files: :file:`subsys/bluetooth/adv_prov/providers/fast_pair.c`

.. doxygengroup:: bt_le_adv_prov_fast_pair
   :project: nrf
   :members:

Swift Pair provider API
=======================

| Header file: :file:`include/bluetooth/adv_prov/swift_pair.h`
| Source files: :file:`subsys/bluetooth/adv_prov/providers/swift_pair.c`

.. doxygengroup:: bt_le_adv_prov_swift_pair
   :project: nrf
   :members:
