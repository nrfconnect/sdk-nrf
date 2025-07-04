.. _bt_fast_pair_adv_manager_readme:

Fast Pair Advertising Manager
#############################

.. contents::
   :local:
   :depth: 2

The Fast Pair Advertising Manager is a helper module for the :ref:`bt_fast_pair_readme` that provides management of the Fast Pair advertising set.
The module implements a trigger-based system for controlling Fast Pair advertising state that allows client modules to request advertising with their preferred configuration.
It also defines the use case layer that provides an implementation of specific advertising requirements for supported use cases.

Overview
********

The Fast Pair Advertising Manager module provides the following key features:

* Trigger-based advertising control - Allows multiple components to request the start of advertising using registered triggers.
* Trigger-based advertising configuration - Provides flexible configuration of the advertising parameters, such as pairing mode and RPA rotation behavior based on trigger requirements.
* Payload synchronization - Ensures advertising payload updates are synchronized with RPA rotations.
* Extension compatibility - Is compatible with the Fast Pair extensions that are supported by the :ref:`bt_fast_pair_readme` module (for example, the Find My Device Network extension).

  .. include:: /includes/fast_pair_fmdn_rename.txt

* Use case support - Provides tailored behavior for specific Fast Pair device types (for example, locator tags).

.. note::
   The module is designed to work in a cooperative thread context.

.. _bt_fast_pair_adv_manager_config:

Configuration
*************

To enable the Fast Pair Advertising Manager, use the :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER` Kconfig option.
As this is a helper module, the Kconfig option is only configurable if the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option is enabled (see the :ref:`bt_fast_pair_readme` documentation for more details).

With the :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER` Kconfig option enabled, you can use the following Kconfig options with this module:

* :kconfig:option:`BT_FAST_PAIR_ADV_MANAGER_USE_CASE` - This Kconfig option allows to select the Fast Pair use case (device type) and to enable additional advertising behaviors that are specific to your use case.

  * :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_UNSPECIFIED` - The option provides a neutral configuration without behaviors specific to any use case.
    By default, the :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_UNSPECIFIED` option is selected.
  * :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_LOCATOR_TAG` - The option provides a tailored configuration for Fast Pair locator tags that use the FMDN extension.
    It depends on the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN` Kconfig option.

    * :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_LOCATOR_TAG_CLOCK_SYNC_TRIGGER` - The option enables the clock synchronization trigger for locator tag devices.
      The option is enabled by default.

Dependencies
============

The :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER` Kconfig option depends on the following Bluetooth LE features:

* :kconfig:option:`CONFIG_BT_PERIPHERAL` - Peripheral role support
* :kconfig:option:`CONFIG_BT_EXT_ADV` - Extended advertising support
* :kconfig:option:`CONFIG_BT_PRIVACY` - Bluetooth privacy support
* :kconfig:option:`CONFIG_BT_SMP` - Security Manager Protocol support
* :kconfig:option:`CONFIG_BT_ADV_PROV` - Advertising data provider support
* :kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR` - Fast Pair advertising data provider support

Implementation details
**********************

This module uses Zephyr Bluetooth subsystem APIs to manage the Fast Pair advertising set and the connections that are derived from it.

Module lifecycle
================

The module follows a specific lifecycle:

1. Initialization - The module is initialized during system startup by the ``SYS_INIT`` macro.
#. Configuration - The majority of the configuration (such as information callbacks or Bluetooth identity) is performed before enabling the module.
#. Enabling - The module is enabled with the :c:func:`bt_fast_pair_adv_manager_enable` function, which requires that the Fast Pair subsystem is ready (:c:func:`bt_fast_pair_enable`).
#. Operation - Triggers can request advertising activation or deactivation.
#. Disabling - The module is disabled with the :c:func:`bt_fast_pair_adv_manager_disable` function before the Fast Pair subsystem is disabled.

Advertising triggers
====================

The module uses a trigger-based system where different components can request advertising activation.

All advertising triggers are registered in the module section with the :c:macro:`BT_FAST_PAIR_ADV_MANAGER_TRIGGER_REGISTER` macro that uses the :c:macro:`STRUCT_SECTION_ITERABLE` macro to place the :c:struct:`bt_fast_pair_adv_manager_trigger` descriptor in the module section.
See the :ref:`zephyr:iterable_sections_api` section for more details.
The number of registered triggers is limited to ``32``.

Each trigger can have a dedicated configuration that is described by the :c:struct:`bt_fast_pair_adv_manager_trigger_config` structure.
The trigger configuration is a constant field of the :c:struct:`bt_fast_pair_adv_manager_trigger` structure and cannot be changed at runtime.

Advertising process
===================

The module uses the Bluetooth Extended Advertising API (:file:`include/zephyr/bluetooth/bluetooth.h`) to manage the Fast Pair advertising set.

.. note::
   Even though the Bluetooth Extended Advertising API is used to manage the Fast Pair advertising set, the module still broadcasts advertising frames in the legacy format over-the-air.

Advertising set initialization
------------------------------

The Fast Pair advertising set is allocated with the :c:func:`bt_le_ext_adv_create` function in the :c:func:`bt_fast_pair_adv_manager_enable` context.
During the advertising set creation, the module registers the following callbacks:

* :c:member:`bt_le_ext_adv_cb.connected` - The callback is used to track the connection state.
* :c:member:`bt_le_ext_adv_cb.rpa_expired` - The callback is used to manage the RPA rotations.

Additionally, during the advertising set creation, the advertising parameters are set as follows:

* Advertising interval ``100 ms``

  .. note::
     Currently, the advertising interval is fixed and cannot be changed with a Kconfig option or API functions.

* Bluetooth identity according to the module configuration (:c:func:`bt_fast_pair_adv_manager_id_set`).

Since this module is typically responsible for setting the RPA timeout that affects all advertising sets, it triggers the initial RPA rotation with the :c:func:`bt_le_oob_get_local` function to configure the timeout according to the Fast Pair advertising requirements.
This operation takes place right after the advertising set creation and before the advertising process is started.

The module starts advertising in the :c:func:`bt_fast_pair_adv_manager_enable` context if the appropriate conditions are met.

RPA rotations
-------------

The module is typically responsible for setting the RPA timeout.
The module sets the RPA timeout with the :c:func:`bt_le_set_rpa_timeout` function in the context of the :c:member:`bt_le_ext_adv_cb.rpa_expired` callback that is registered during the advertising set creation.

.. note::
   If the FMDN extension is used, the FMDN advertising configures the RPA timeout in the FMDN provisioned state.
   This module is tightly integrated with the FMDN extension and respects this requirement.

The timeout value is recalculated for every RPA rotation and randomized to increase privacy.
The typical timeout ranges between ``11`` and ``15`` minutes.
The :c:func:`sys_csrand_get` function from the :ref:`zephyr:random_api` subsystem is used to generate random value for the timeout.

The RPA rotation is suspended if any active advertising trigger has the :c:member:`bt_fast_pair_adv_manager_trigger_config.suspend_rpa` flag set to ``true``.
This is achieved by returning ``false`` from the :c:member:`bt_le_ext_adv_cb.rpa_expired` callback.

Advertising payload
-------------------

The module uses the :ref:`bt_le_adv_prov_readme` library (:kconfig:option:`CONFIG_BT_ADV_PROV`) to construct the advertising payload.

During the payload update procedure, the module populates the :c:struct:`bt_le_adv_prov_adv_state` structure to indicate the current advertising state.
Information about the pairing mode, RPA rotation, and other parameters is passed to the :ref:`bt_le_adv_prov_readme` library with the help of this structure and the API functions that are mentioned in subsequent sentences.
Then, it calls the :c:func:`bt_le_adv_prov_get_ad` function to encode the advertising payload in the array of :c:struct:`bt_data` elements.
The array size is properly allocated with the :c:func:`bt_le_adv_prov_get_ad_prov_cnt` function before the encode operation.
Similarly, the :c:func:`bt_le_adv_prov_get_sd` function is used to encode the scanning data.
As the final step, the collected payload is set in the Fast Pair advertising set with the :c:func:`bt_le_ext_adv_set_data` function.

The advertising payload is updated in the following cases:

* Advertising process is started.
* The :c:func:`bt_fast_pair_adv_manager_payload_refresh` function is called and the advertising is ongoing.
* The :c:member:`bt_le_ext_adv_cb.rpa_expired` callback is triggered due to the RPA rotation and the advertising is ongoing.
* The pairing mode state of the module changes as a result of the :c:func:`bt_fast_pair_adv_manager_request` function call and the advertising is ongoing.

Advertising conditions
----------------------

The advertising is started with the :c:func:`bt_le_ext_adv_start` function and the process is maintained if the following conditions are met:

* The module is enabled (:c:func:`bt_fast_pair_adv_manager_enable`).
* At least one advertising trigger gets activated with the :c:func:`bt_fast_pair_adv_manager_request` function.
* The module does not maintain any connection.

  .. note::
     The module currently supports only one connection at a time.
     A connection stops the Fast Pair advertising even if the advertising triggers are active.
     The advertising is restarted when the connection is terminated.

Otherwise, the advertising is inactive.

The module tracks the advertising activity and can report each change with the :c:member:`bt_fast_pair_adv_manager_info_cb.adv_state_changed` callback to the application.

Trigger state changes
---------------------

The trigger configuration (:c:struct:`bt_fast_pair_adv_manager_trigger_config`) may update the module's advertising state once the given trigger is activated with the :c:func:`bt_fast_pair_adv_manager_request` function.
The advertising state is also updated when the previously activated trigger gets deactivated as its configuration is no longer applied.

Advertising set deinitialization
--------------------------------

During the disable operation, in the :c:func:`bt_fast_pair_adv_manager_disable` function context, the module stops the ongoing Bluetooth activities:

* Advertising process with the :c:func:`bt_le_ext_adv_stop` function.
* Active connections with the :c:func:`bt_conn_disconnect` function.
  This disconnect operation is only limited to the connections that are derived from the Fast Pair advertising set.

The advertising set is also deleted with the :c:func:`bt_le_ext_adv_delete` function.

Coexistence with other advertising sets
=======================================

The Fast Pair advertising set, that is managed by this module, can operate in parallel with other advertising sets without interfering with them.

The module only manages the advertising process for the Fast Pair advertising set and the connections that are derived from it.
It uses one connection slot from the connection pool of the Bluetooth stack (:kconfig:option:`CONFIG_BT_MAX_CONN`).

Other advertising sets that also use RPA as its MAC address are affected by the RPA timeout setting of the Fast Pair advertising set.
The RPA timeout is set globally with the :c:func:`bt_le_set_rpa_timeout` function in the Bluetooth stack and affects all advertising sets.

FMDN integration
================

The module is compatible with the Find My Device Network (FMDN) extension and handles the complex advertising state transitions automatically.
The advertising policy of the Fast Pair advertising set respects the requirements of the FMDN advertising set.
The module rotates the RPA and the payload of the Fast Pair advertising set synchronously with the FMDN advertising set.

The module yields control over the RPA timeout configuration to the FMDN advertising set once the FMDN provisioning is completed.
Conversely, right after the unprovisioning, the module takes back control over the RPA timeout configuration.
To achieve this, the module uses the :c:func:`bt_le_oob_get_local` function to trigger the premature RPA rotation and set the RPA according to the Fast Pair advertising requirements.

Use case extensions
===================

The module supports use case layer that provides an implementation of specific advertising requirements for supported use cases.
Currently, the use case extension is only available for the locator tags.

By default, the :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_UNSPECIFIED` Kconfig option is enabled, which means that this layer is disabled.

If you change this default configuration and select a specific use case, the core library uses the enable callback to activate the use case extension in the :c:func:`bt_fast_pair_adv_manager_enable` function.

Similarly, the disable callback is used in the context of the :c:func:`bt_fast_pair_adv_manager_disable` function to deactivate the use case extension.

For this process to work, the use case layer must register the mandatory callback structure with the enable and disable callbacks.

Locator tag
-----------

The locator tag extension registers the :c:struct:`bt_fast_pair_info_cb` and the :c:struct:`bt_fast_pair_fmdn_info_cb` callback structures to manage the advertising triggers that are specific to the locator tag use case.

This extension defines the FMDN provisioning trigger that is used to maintain the Fast Pair advertising between the Owner Account Key write (:c:member:`bt_fast_pair_info_cb.account_key_written`) and the successful FMDN provisioning (:c:member:`bt_fast_pair_fmdn_info_cb.provisioning_state_changed`).
This advertising trigger is also configured to suspend the RPA rotation during the FMDN provisioning.
If the FMDN provisioning is not completed within the allowed time, the extension deactivates the FMDN provisioning with the :c:func:`bt_fast_pair_adv_manager_request` function.
The trigger behavior is aligned with the requirements of the `Fast Pair Locator Tag Specific Guidelines`_ section of the FMDN Accessory specification.

.. note::
   If the FMDN provisioning trigger is deactivated prematurely with the :c:func:`bt_fast_pair_adv_manager_disable` function, it will not be restored during the :c:func:`bt_fast_pair_adv_manager_enable` function call.
   The device should typically advertise continuously for the predefined period until it is factory reset or successfully provisioned.

The extension also defines the FMDN clock synchronization trigger that is used to maintain the Fast Pair advertising after a power loss until the clock synchronization (:c:member:`bt_fast_pair_fmdn_info_cb.clock_synced`) is completed.
This trigger is enabled by default and can be controlled with the :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_LOCATOR_TAG_CLOCK_SYNC_TRIGGER` Kconfig option.
This advertising trigger does not use a dedicated configuration structure and passes ``NULL`` as the configuration pointer.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/fast_pair/adv_manager.h`
| Source files: :file:`subsys/bluetooth/services/fast_pair/adv_manager`

.. doxygengroup:: bt_fast_pair_adv_manager
