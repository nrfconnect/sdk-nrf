.. _ug_bt_fast_pair_adv_manager:

Google Fast Pair Advertising Manager integration
################################################

.. contents::
   :local:
   :depth: 2

The :ref:`bt_fast_pair_adv_manager_readme` is a helper module for the :ref:`bt_fast_pair_readme` that manages the Fast Pair advertising set.
This page provides guidelines on how to integrate this module in your application.

.. note::
   This page complements the main Fast Pair integration guide and covers only integration steps for the :ref:`bt_fast_pair_adv_manager_readme` module.
   For the complete Fast Pair integration guide, see :ref:`ug_bt_fast_pair`.

Prerequisites
*************

Before using the :ref:`bt_fast_pair_adv_manager_readme`, ensure to fulfill the following prerequisites:

* Read through the main :ref:`ug_bt_fast_pair` guide to understand the overall Fast Pair integration requirements and ensure you meet all the prerequisites listed in this document.
* Accept the usage of the Bluetooth® LE Extended Advertising feature (:kconfig:option:`CONFIG_BT_EXT_ADV`), which is a dependency for the :ref:`bt_fast_pair_adv_manager_readme` module.

  .. note::
     The Bluetooth LE Extended Advertising feature (:kconfig:option:`CONFIG_BT_EXT_ADV` Kconfig option) enforces the SoftDevice Controller library in the multirole variant (:file:`libsoftdevice_controller_multirole.a`) for most supported platforms.
     The multirole SoftDevice Controller library has higher memory footprint than the peripheral and central variants.

* Accept the usage of the :ref:`bt_le_adv_prov_readme` library (:kconfig:option:`CONFIG_BT_ADV_PROV`) as a way of encoding the advertising payload for the Fast Pair advertising set.
  Additionally, the advertising provider for the Fast Pair payload (:kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR` Kconfig option) must be enabled when using this module.

Extension compatibility
***********************

The Fast Pair Advertising Manager module is compatible with all Fast Pair extensions that are supported by the :ref:`bt_fast_pair_readme` module.

Integration steps
*****************

The process of Fast Pair Advertising Manager integration has the following steps:

1. :ref:`ug_bt_fast_pair_adv_manager_setup`
#. :ref:`ug_bt_fast_pair_adv_manager_triggers`
#. :ref:`ug_bt_fast_pair_adv_manager_payload`
#. :ref:`ug_bt_fast_pair_adv_manager_use_case`

For an integration example, see the :ref:`fast_pair_locator_tag` sample.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_adv_manager_setup:

Setting up the module
*********************

To start integrating the Fast Pair Advertising Manager in your project, complete the following steps:

* :ref:`ug_bt_fast_pair_adv_manager_setup_kconfig`
* :ref:`ug_bt_fast_pair_adv_manager_setup_lifecycle`

.. _ug_bt_fast_pair_adv_manager_setup_kconfig:

Adding module Kconfig configuration
===================================

To support the Fast Pair Advertising Manager in your project, enable the :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER` Kconfig option in the configuration file of your application image.
You must also set the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option that enables the general support for the Fast Pair solution.
Additionally, enable all dependencies of the :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER` Kconfig option.
They are listed in the :ref:`bt_fast_pair_adv_manager_config` section.

.. _ug_bt_fast_pair_adv_manager_setup_lifecycle:

Managing module lifecycle
=========================

Before you can start using the Fast Pair Advertising Manager, you must first enable the Fast Pair subsystem using the :c:func:`bt_fast_pair_enable` function and then enable the Advertising Manager using the :c:func:`bt_fast_pair_adv_manager_enable` function.

During the enable operation, the module allocates the Fast Pair advertising set from the Bluetooth stack memory pool and may start advertising if any client module requests the advertising to be active before the :c:func:`bt_fast_pair_adv_manager_enable` function call.

.. note::
   If you use more than one advertising set simultaneously in your application, make sure that the :kconfig:option:`CONFIG_BT_EXT_ADV_MAX_ADV_SET` Kconfig option is set to a correct value.
   The additional advertising sets must also rotate their Resolvable Private Addresses (RPA) in synchronization with the Fast Pair advertising set to maintain the privacy properties of the overall solution.

You can disable the module using the :c:func:`bt_fast_pair_adv_manager_disable` function.
During the disable operation, all module's activities are stopped:

* The Fast Pair advertising is stopped and the associated advertising set is deallocated from the Bluetooth stack memory pool.
* The Fast Pair connection derived from the Fast Pair advertising set is terminated.

.. note::
   The :c:func:`bt_fast_pair_adv_manager_enable` and :c:func:`bt_fast_pair_adv_manager_disable` functions are not thread-safe and must be called in the cooperative thread context.

You can check the module readiness using the :c:func:`bt_fast_pair_adv_manager_is_ready` function.
The extension is marked as *ready* when it is in the enabled state and *unready* when it is in the disabled state.

Some optional API functions must only be used in the *unready* state.
The following subsections describe them in more details.

Bluetooth identity configuration
--------------------------------

You can configure the Bluetooth identity, but by default, the module uses the :c:macro:`BT_ID_DEFAULT` identity.

To set the Bluetooth identity for Fast Pair advertising and connections, use the :c:func:`bt_fast_pair_adv_manager_id_set` function.
Make sure that the Bluetooth identity has been created in the Bluetooth stack before you enable the module with the :c:func:`bt_fast_pair_adv_manager_enable` function.
The Bluetooth identity cannot be updated if the Fast Pair module is in the *ready* state (see the :c:func:`bt_fast_pair_adv_manager_is_ready` function).

You can also use the :c:func:`bt_fast_pair_adv_manager_id_get` function to get the Bluetooth identity that is currently set in the module.

Information callback registration
---------------------------------

The Fast Pair Advertising Manager uses the information callbacks to notify the application about events related to the module.

The callback registration is optional.
However, if you want to subscribe to information callbacks, you must register them using the :c:func:`bt_fast_pair_adv_manager_info_cb_register` function before the module is enabled with the :c:func:`bt_fast_pair_adv_manager_enable` function.
Make sure that the registered :c:struct:`bt_fast_pair_adv_manager_info_cb` structure persists in the application memory (static declaration), as during the registration, the module stores only the memory pointer to it.
You can register multiple callback sets using the :c:func:`bt_fast_pair_adv_manager_info_cb_register` function.

The :c:struct:`bt_fast_pair_adv_manager_info_cb` structure supports only one callback type.

The :c:member:`bt_fast_pair_adv_manager_info_cb.adv_state_changed` callback provides notifications about changes in the advertising state.
See the :ref:`ug_bt_fast_pair_adv_manager_setup_callbacks_adv_state_changed` section for more details.

.. _ug_bt_fast_pair_adv_manager_setup_callbacks_adv_state_changed:

Advertising state change callback
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :c:member:`bt_fast_pair_adv_manager_info_cb.adv_state_changed` callback is triggered when the advertising state changes.
When the advertising starts, the callback's *active* parameter is set to ``true``.
When the advertising stops, the *active* parameter is set to ``false``.

This callback can also be triggered in the context of the :c:func:`bt_fast_pair_adv_manager_enable` function provided that the module starts advertising during the enable operation.
Similarly, the callback can be triggered in the context of the :c:func:`bt_fast_pair_adv_manager_disable` function provided that the module stops advertising during the disable operation.

You can also use the :c:func:`bt_fast_pair_adv_manager_is_adv_active` function that is associated with this asynchronous callback to synchronously check the current state of the Fast Pair advertising.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_adv_manager_triggers:

Defining and using triggers
***************************

Triggers are the core mechanism for controlling Fast Pair advertising.
Each trigger represents a reason for advertising and can be configured with specific parameters.

Trigger definition
==================

To start using the advertising trigger, you must first define it with the :c:macro:`BT_FAST_PAIR_ADV_MANAGER_TRIGGER_REGISTER` macro.
This macro creates a :c:struct:`bt_fast_pair_adv_manager_trigger` structure in the dedicated memory section that is managed by the Fast Pair Advertising Manager module.
This structure is used to represent the advertising trigger.

The :c:macro:`BT_FAST_PAIR_ADV_MANAGER_TRIGGER_REGISTER` macro requires the following two mandatory parameters:

* The variable name of the :c:struct:`bt_fast_pair_adv_manager_trigger` structure.
* The trigger identifier in string format.

The :c:macro:`BT_FAST_PAIR_ADV_MANAGER_TRIGGER_REGISTER` macro also accepts a third optional parameter, namely a pointer to the :c:struct:`bt_fast_pair_adv_manager_trigger_config` structure that describes the trigger configuration.
The trigger configuration is a constant field of the :c:struct:`bt_fast_pair_adv_manager_trigger` structure and it cannot be changed at runtime.
If you do not need to configure the trigger, you can pass ``NULL`` as the third parameter.

An example of how to define a trigger for the user-initiated pairing mode that uses a dedicated configuration:

.. code-block:: c

   /* Trigger for user-initiated pairing mode */
   BT_FAST_PAIR_ADV_MANAGER_TRIGGER_REGISTER(
       fp_adv_trigger_pairing_mode,
       "pairing_mode",
       (&(const struct bt_fast_pair_adv_manager_trigger_config) {
           .pairing_mode = true,
           .suspend_rpa = true,
       }));

Trigger configuration options
=============================

You can attach a configuration to the trigger using the :c:macro:`BT_FAST_PAIR_ADV_MANAGER_TRIGGER_REGISTER` macro.
The trigger configuration is represented by the :c:struct:`bt_fast_pair_adv_manager_trigger_config` structure that is a part of the :c:struct:`bt_fast_pair_adv_manager_trigger` structure and cannot be changed at runtime once it is defined.

You can customize the advertising properties of each trigger by populating the fields of the :c:struct:`bt_fast_pair_adv_manager_trigger_config` structure.
The available configuration options include:

* :ref:`ug_bt_fast_pair_adv_manager_triggers_pairing_mode`
* :ref:`ug_bt_fast_pair_adv_manager_triggers_suspend_rpa`

.. _ug_bt_fast_pair_adv_manager_triggers_pairing_mode:

Pairing mode
------------

The :c:member:`bt_fast_pair_adv_manager_trigger_config.pairing_mode` option is used to create a trigger that enables advertising in the pairing mode.

When the pairing mode is active, the Fast Pair advertising set uses the discoverable advertising payload.
When inactive, the non-discoverable payload is used.

The pairing mode is active if at least one Fast Pair Advertising Manager trigger is active and is configured for the pairing mode.
The change of the pairing mode state always triggers an advertising payload update.

To check if the Fast Pair Advertising Manager is in the pairing mode, use the :c:func:`bt_fast_pair_adv_manager_is_pairing_mode` function.

.. _ug_bt_fast_pair_adv_manager_triggers_suspend_rpa:

Suspension of Resolvable Private Address (RPA) rotations
--------------------------------------------------------

The :c:member:`bt_fast_pair_adv_manager_trigger_config.suspend_rpa` option creates a trigger that suspends RPA rotations for the Fast Pair advertising set.

The RPA rotation is suspended if at least one Fast Pair Advertising Manager trigger is active and configured to suspend RPA rotations.

Use this option with caution, as it decreases the privacy of the advertiser by freezing its RPA rotations.
It is typically used for triggers that are only active for a short period of time.

Trigger activation
==================

To activate and deactivate a specific trigger, use the :c:func:`bt_fast_pair_adv_manager_request` function.
The function requires a pointer to the :c:struct:`bt_fast_pair_adv_manager_trigger` structure to identify the trigger instance and a boolean parameter that indicates whether the trigger should be activated or deactivated.
Initially, the trigger is in inactive state.

Once the new trigger is activated, the Fast Pair Advertising Manager module parses its configuration and adjusts the advertising process accordingly.
After its deactivation, this particular trigger configuration is no longer taken into account by the module during the advertising.

This Fast Pair Advertising Manager module starts advertising when at least one trigger is active.
Similarly, it stops advertising when all triggers are inactive.

If Fast Pair advertising is active as a result of other active triggers, the activation of a new trigger does not cause the advertising payload update unless the new trigger changes the pairing mode.
If necessary, you can force the advertising payload update using the :c:func:`bt_fast_pair_adv_manager_payload_refresh` function.

You can also call the :c:func:`bt_fast_pair_adv_manager_request` function to set the trigger state when the Fast Pair Advertising Manager module is in the *unready* state and has not been enabled yet with the :c:func:`bt_fast_pair_adv_manager_enable` function.
However, in this case, changes in the trigger state do not have an effect on the Fast Pair advertising.
When the module is in the *unready* state, advertising is disabled regardless of the trigger state.
The advertising is started in the context of the :c:func:`bt_fast_pair_adv_manager_enable` function if at least one trigger is activated before the enable operation.

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_adv_manager_payload:

Managing advertising payload
****************************

The Fast Pair Advertising Manager uses the :ref:`bt_le_adv_prov_readme` library (:kconfig:option:`CONFIG_BT_ADV_PROV`) to construct the advertising payload.
This module acts as middleware between the :ref:`bt_le_adv_prov_readme` library and the application part responsible for controlling Fast Pair advertising with triggers.
This design allows for flexible composition of advertising data.

Understanding advertising providers
===================================

The advertising payload is constructed from multiple providers, each responsible for a specific advertising data element.
To use the Fast Pair Advertising Manager, you must enable the Fast Pair advertising data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR`) in the project's configuration file.
This particular provider is used to encode the Fast Pair advertising data either in the discoverable mode if the pairing mode is active or in the not discoverable mode otherwise.

The Fast Pair Advertising Manager module does not include any other providers by default.
You can either implement additional providers manually or use the ones available in the :ref:`bt_le_adv_prov_readme` library (such as the TX Power provider - :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER`).
To understand how to implement custom providers, see the :ref:`ug_bt_fast_pair_adv_manager_payload_custom` section.

Each provider can decide whether to include its data based on the current advertising state.
The pairing mode state of the Fast Pair Advertising Manager is directly passed to the provider in the :c:struct:`bt_le_adv_prov_adv_state` structure.

For more details on the advertising providers, see the :ref:`bt_le_adv_prov_readme` page.

Managing advertising space
==========================

Bluetooth LE advertising packets have limited space (31 bytes for legacy advertising).
When combining multiple advertising providers, ensure that the encoded advertising payload does not exceed the advertising packet size.
The Fast Pair Advertising Manager module does not perform any payload size validation and you must verify if the advertising payload is within the limits.
Note that providers can include their data conditionally based on the current advertising state.
Ensure that data length is within the limits in all cases.

Data privacy considerations
===========================

When designing custom advertising data providers, pay special attention to data privacy to maintain the privacy properties of the Fast Pair solution:

* Avoid constant device identifiers - Do not include static device identifiers, such as serial numbers, MAC addresses, or other persistent device-specific information in the advertising payload.
  These identifiers can be used to track the device across different advertising sessions and compromise user privacy.
  If strictly necessary, you can encode the device identifier in the advertising payload provided that it is broadcasted for a limited time.

* Handle random data properly - If your custom provider includes random numbers or other dynamic data, ensure that this data is regenerated on each Resolvable Private Address (RPA) rotation.
  This prevents correlation of advertising data across different RPA periods, which could be used to track the device.
  The Fast Pair Advertising Manager automatically rotates the RPA if it is not blocked by any advertising trigger (with the suspend RPA rotation option).
  Your custom providers should respond to these rotations by updating any random data they include in the advertising payload.

.. _ug_bt_fast_pair_adv_manager_payload_custom:

Adding custom advertising data
==============================

To extend the advertising payload you can implement custom advertising data providers.
This allows you to include application-specific data alongside the Fast Pair payload.

To create a custom provider, implement the :c:func:`bt_le_adv_prov_data_get` callback and register it using the :c:macro:`BT_LE_ADV_PROV_AD_PROVIDER_REGISTER` macro to include its data in the advertising payload.

Similarly, you can use the :c:macro:`BT_LE_ADV_PROV_SD_PROVIDER_REGISTER` macro to register the scan response data provider.

Here is an example that conditionally includes the Bluetooth Advertising Data (AD) with the 16-bit UUID type and the payload corresponding to the Human Interface Device Service (HIDS) and Battery Service (BAS):

.. code-block:: c

   #include <zephyr/bluetooth/uuid.h>
   #include <bluetooth/adv_prov.h>

   static int get_data(struct bt_data *ad, const struct bt_le_adv_prov_adv_state *state,
                       struct bt_le_adv_prov_feedback *fb)
   {
      ARG_UNUSED(fb);

      /* Only include this advertising data when the pairing mode is active. */
      if (!state->pairing_mode) {
         return -ENOENT;
      }

      /* Define the list of Bluetooth service UUIDs that should be included in the payload. */
      static const uint8_t data[] = {
         BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
         BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
      };

      /* Populate the AD structure with the list of Bluetooth service UUIDs. */
      ad->type = BT_DATA_UUID16_ALL;
      ad->data_len = sizeof(data);
      ad->data = data;

      return 0;
   }

   BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(uuid16_all, get_data);

.. rst-class:: numbered-step

.. _ug_bt_fast_pair_adv_manager_use_case:

Use case integration
********************

The module supports extensions that handle common advertising scenarios automatically.
Each extension is intended for a specific use case, which means that you can only enable one extension at a time.
By default, the use case layer is disabled and the :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE` Kconfig choice is set to the :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_UNSPECIFIED` value.
If you want to support a specific use case, you must enable the corresponding Kconfig choice manually.
Currently, this layer supports only the locator tag use case.

Locator tag
===========

To enable the module extension for the locator tag use case, use the :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_LOCATOR_TAG` Kconfig option.

.. note::
   If the locator tag extension of this module does not fit your application requirements, you can choose not to enable the :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_LOCATOR_TAG` Kconfig option and replace it with your custom implementation.

The Fast Pair Advertising Manager module is compatible with the Find My Device Network (FMDN) extension and handles the complex advertising state transitions automatically.
The advertising policy of the Fast Pair advertising set respects the requirements of the FMDN advertising set.

.. include:: /includes/fast_pair_fmdn_rename.txt

This extension provides automatic handling of FMDN-related advertising requirements that are specified in the `Fast Pair Locator Tag Specific Guidelines`_ section of the FMDN Accessory specification.
This is achieved by using the advertising triggers in the module's use case layer for locator tags.
This extension manages the following triggers:

* The FMDN provisioning trigger used to maintain Fast Pair non-discoverable advertising after the Account Key write operation.
  The FMDN provisioning trigger is active until the FMDN provisioning is completed or when the timeout period expires.
* The FMDN clock synchronization trigger used to maintain Fast Pair non-discoverable advertising after a power loss to synchronize time between the locator tag and the phone.
  The FMDN clock synchronization trigger is active until the clock synchronization operation is completed.
  You can disable this trigger with the :kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_LOCATOR_TAG_CLOCK_SYNC_TRIGGER` Kconfig option and replace it with your custom implementation.

See the :ref:`fast_pair_locator_tag` sample to understand how to use the locator tag extension of the Fast Pair Advertising Manager module in the example application.

For the overall integration details on the locator tag use case, see the :ref:`ug_bt_fast_pair_use_case_locator_tag` section in the general Fast Pair integration guide.

Applications and samples
************************

See the :ref:`fast_pair_locator_tag` sample for a complete implementation example using the Fast Pair Advertising Manager.

Dependencies
************

The following are the required dependencies for the Fast Pair integration:

* :ref:`zephyr:random_api`
* :ref:`zephyr:kernel_api`
* :ref:`zephyr:bluetooth`
* :ref:`bt_fast_pair_readme`
* :ref:`bt_le_adv_prov_readme`
