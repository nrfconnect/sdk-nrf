.. _ug_dult:

Detecting Unwanted Location Trackers (DULT) integration
#######################################################

.. contents::
   :local:
   :depth: 2

The Detecting Unwanted Location Trackers (DULT) is a specification that lists a set of best practices and protocols for manufacturers of products with built-in location tracking capabilities.
Following the specification improves the privacy and safety of individuals by preventing the location tracking products from tracking users without their knowledge or consent.
For detailed information about supported functionalities, see the official `DULT`_ documentation.

.. note::
   The `DULT`_ documentation has the Internet-Draft status, which means it is valid for a maximum of six months and subject to change or obsolescence.
   For more details, refer to the `DULT status`_ section from the DULT specification.

   The DULT support in the |NCS| is :ref:`experimental <software_maturity>`.
   Breaking updates in the DULT support might be implemented in response to changes in the `DULT`_ specification.
   Not all optional features are supported.

Integration prerequisites
*************************

Before you start the |NCS| integration with DULT, make sure that the following prerequisites are fulfilled:

* :ref:`Install the nRF Connect SDK <installation>`.
* Set up a supported :term:`Development Kit (DK)`.
  See :ref:`device_guides` for more information on setting up the DK you are using.

Solution architecture
*********************

The |NCS| integrates the location-tracking accessory role, facilitating communication between the accessory-locating network (typically a smartphone) and the accessory (your device).
The integration requires completing the :ref:`ug_integrating_dult` section.

.. _ug_integrating_dult:

Integration steps
*****************

The DULT integration in the |NCS| consists of the following steps:

1. :ref:`Registering the device <ug_dult_registering>`
#. :ref:`Performing prerequisite operations <ug_dult_prerequisite_ops>`
#. :ref:`Handling multiple accessory-locating networks <ug_dult_multi_user>`
#. :ref:`Managing the near-owner state <ug_dult_near_owner_state>`
#. :ref:`Customizing the access policy to the accessory non-owner service <ug_dult_anos_access>`
#. :ref:`Building the location-enabled advertising payload <ug_dult_advertising>`
#. :ref:`Managing the identification process <ug_dult_identifier>`
#. :ref:`Using the sound callbacks and managing the sound state <ug_dult_sound>`
#. :ref:`Interacting with the motion detector <ug_dult_motion_detector>`
#. :ref:`Managing the battery information <ug_dult_battery>`

These steps are described in the following sections.

The DULT standard implementation in the |NCS| integrates the location-tracking accessory role.
For an integration example, see the Find Hub Network (FHN) extension of the :ref:`bt_fast_pair_readme`.
Also see the :ref:`fast_pair_locator_tag` sample that integrates the Fast Pair with the FHN extension, which integrates the :ref:`dult_readme` module.

.. rst-class:: numbered-step

.. _ug_dult_registering:

Registering the device
***********************

The location-tracking accessory must be registered with the accessory-locating network that it aims to join.
Upon registration, the network provider generates registration data for the accessory.
The location-tracking accessory uses the registration data for procedures defined by the DULT standard.

Check with the network provider for information on how to register the accessory and obtain the necessary data.
After registration, you should have the following data:

* Product data - A unique identifier for the accessory make and model.
* Manufacturer name - The name of the company that produces the accessory.
* Model name - The name of the specific model of the accessory.
* Accessory category - Choose the appropriate category value from the `DULT Accessory Category Values table`_.
* Network ID - Accessory-locating network ID.
  See the `DULT Manufacturer Network ID Registry`_ from the DULT specification for a list of network IDs.
* Network version (optional) - The version of the accessory-locating network implementation running on the accessory.
* Knowledge on how to construct the Identifier Payload - The accessory-locating network defines its own identifier that allows the network to identify the accessory in case of unwanted tracking, and to share obfuscated accessory owner information with a tracked individual.

.. rst-class:: numbered-step

.. _ug_dult_prerequisite_ops:

Performing prerequisite operations
**********************************

You must enable the :kconfig:option:`CONFIG_DULT` Kconfig option to support the DULT standard in your project.

Several Kconfig options are available to configure the DULT integration.
For more details, see the :ref:`Configuration <dult_configuration>` section of the :ref:`dult_readme`.

.. _ug_dult_api_variant:

DULT API variant
================

The DULT subsystem provides two API contracts that differ in the number of supported DULT users and in the user lifecycle.
Select one with the :kconfig:option:`CONFIG_DULT_API_VARIANT_V1` or the :kconfig:option:`CONFIG_DULT_API_VARIANT_V2` Kconfig option.

API variant v1 (:kconfig:option:`CONFIG_DULT_API_VARIANT_V1`):

* Only one DULT user can be registered at a time.
* The :c:func:`dult_reset` function is the terminal teardown.
  It releases the DULT subsystem, unregisters the DULT user together with its callbacks, and clears the preset configuration.
* The :c:func:`dult_user_unregister`, :c:func:`dult_multi_user_cb_register`, and :c:func:`dult_multi_user_conn_claim` functions are not supported.
* In a typical integration, the DULT association follows the stack lifetime.
  The accessory-locating network enables the DULT subsystem when it enables its own stack, and resets it when it disables the stack.

API variant v2 (:kconfig:option:`CONFIG_DULT_API_VARIANT_V2`):

* Up to :kconfig:option:`CONFIG_DULT_USER_MAX` DULT users can be registered at the same time, but just one of them is the associated user.
* The :c:func:`dult_reset` function is not terminal.
  It releases the association and keeps the DULT user registered, so the user can be enabled again with the :c:func:`dult_enable` function.
* The :c:func:`dult_user_unregister` function is the terminal teardown that frees the user.
* The registered callbacks, the battery level, and the near-owner state are preserved across the :c:func:`dult_reset` function call and cleared by the :c:func:`dult_user_unregister` function call.
* In a typical integration, the DULT association follows the association state with the accessory-locating network.
  The network claims the association when the accessory is associated with it, or when it enables its stack with the accessory already associated, and releases the association on disassociation or when it disables its stack.

For an example of both integrations, see the :ref:`ug_bt_fast_pair_prerequisite_ops_fhn_dult_integration` section of the Fast Pair integration guide.

.. note::
   The :kconfig:option:`CONFIG_DULT_API_VARIANT_V1` option is selected by default for backwards compatibility, but it is deprecated.
   Use the :kconfig:option:`CONFIG_DULT_API_VARIANT_V2` option for new integrations.

.. _ug_dult_accessory_type:

Accessory type
==============

Declare the size and the discoverability of your accessory with the :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_SMALL` or the :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_LARGE` Kconfig option.
The :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_SMALL` option is selected by default.
Check the size and discoverability requirements in the `DULT`_ specification to determine the accessory type for your product.

The DULT specification best practices are required for accessories that are small and not easily discoverable, and only recommended for accessories that are large and easily discoverable.
The requirements for large accessories are therefore more relaxed:

* With the :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_SMALL` option, the DULT subsystem validates the accessory capabilities during the DULT user registration, as described in the :ref:`ug_dult_user` section.
* With the :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_LARGE` option, all accessory capabilities are optional and the DULT subsystem does not validate them.

.. _ug_dult_user:

DULT user
=========

The DULT subsystem introduces the concept of a DULT user.
Each DULT user represents one accessory-locating network, identified by its network ID.
The number of users that can be registered at the same time depends on the selected :ref:`ug_dult_api_variant`.
To use the DULT subsystem, you must register a DULT user by calling the :c:func:`dult_user_register` function before you can call any other function from the DULT API.
Upon registration, you must provide the DULT user configuration to the DULT subsystem.
The DULT user configuration includes the following data:

* Registration data - The data obtained during :ref:`registering the device <ug_dult_registering>`.
* Accessory capabilities - Capabilities of your accessory.
  Set appropriate bits in the bitmask to indicate the supported capabilities.
  There are following capabilities available:

  * Play sound (:c:enum:`DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS`) - A feature that enables the accessory to emit sound signals.
  * Motion detector unwanted tracking (:c:enum:`DULT_ACCESSORY_CAPABILITY_MOTION_DETECTOR_UT_BIT_POS`) - A feature that improves security by preventing unwanted tracking.
  * Identifier lookup by NFC (:c:enum:`DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_NFC_BIT_POS`) - A feature supporting identifier lookup by NFC functionality.
  * Identifier lookup by Bluetooth® LE (:c:enum:`DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_BLE_BIT_POS`) - A feature supporting identifier lookup by Bluetooth LE functionality.

* Firmware version - The firmware version of your accessory.
* Network version - The version of the accessory-locating network implementation, provided through the :c:member:`dult_user.network_version` field.
  This field is optional.
  Leave it as ``NULL`` if the accessory-locating network does not define a network version.

The DULT user registration fails if a mandatory accessory capability is missing.
Which capabilities are mandatory depends on the declared :ref:`ug_dult_accessory_type`:

* With the :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_SMALL` Kconfig option, the play sound capability is mandatory, and at least one of the identifier lookup capabilities, by NFC or by Bluetooth LE, must be declared.
  The motion detector unwanted tracking capability is optional.
* With the :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_LARGE` Kconfig option, all accessory capabilities are optional and the DULT subsystem does not validate them.

How you release a registered DULT user depends on the selected :ref:`ug_dult_api_variant`:

* With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V1` Kconfig option, call the :c:func:`dult_reset` function.
  This function unregisters the registered DULT user information and callbacks, so you must register them again to change the DULT user.
* With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V2` Kconfig option, call the :c:func:`dult_user_unregister` function.
  The :c:func:`dult_reset` function only releases the association and keeps the user registered, so the associated user must call it before unregistering.

.. _ug_dult_callback_registration:

Callback registration
=====================

An application can communicate with the DULT subsystem using API calls and registered callbacks.
The DULT subsystem uses the registered callbacks to inform the application about the DULT-related events and to retrieve the necessary information from the application.

The application must register the required callbacks before it enables the DULT subsystem and starts to operate as the location-tracking accessory.
To identify the callback registration functions in the DULT API, look for the ``_cb_register`` suffix.
Set your application-specific callback functions in the callback structure, which serves as the input parameter for the ``..._cb_register`` API function.
Each callback structure is registered for each DULT user.
The callback structure must persist in the application memory (static declaration), as during the registration, the DULT subsystem stores only the memory pointer to it.
Use the following functions to register callbacks:

  * :c:func:`dult_id_read_state_cb_register` (mandatory if the identifier lookup by Bluetooth LE capability is declared)
  * :c:func:`dult_sound_cb_register` (mandatory if the play sound capability is declared)
  * :c:func:`dult_motion_detector_cb_register` (mandatory if the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR` Kconfig option is enabled)
  * :c:func:`dult_multi_user_cb_register` (mandatory if the :kconfig:option:`CONFIG_DULT_USER_MAX` Kconfig option is set to a value greater than ``1``)
  * :c:func:`dult_bt_anos_cb_register` (optional)

With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V1` Kconfig option, the callbacks are cleared by the :c:func:`dult_reset` function and must be registered again after each subsequent DULT user registration.
With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V2` Kconfig option, the callbacks are preserved across the :c:func:`dult_reset` function call and cleared only by the :c:func:`dult_user_unregister` function call.

Preset configuration
====================

Before enabling the DULT subsystem, you should preset the initial accessory configuration with dedicated APIs that depend on the chosen feature set or the accessory state.
The preset configuration is available for the following API functions:

* :c:func:`dult_near_owner_state_set` - Apply this configuration in case the accessory state is different than the default value (see the :ref:`Managing the near-owner state <ug_dult_near_owner_state>` section for more details).
* :c:func:`dult_battery_level_set` - Apply this configuration in case the battery feature is enabled with the :kconfig:option:`CONFIG_DULT_BATTERY` (see the :ref:`Managing the battery information <ug_dult_battery>` section for more details).

The preset configuration is stored for each DULT user, in the same way as the registered callbacks.
Only the preset configuration of the associated user is effective.
A user that wins the association arbitration always applies its own preset configuration, and the values preset by the other registered users are never used on its behalf.

Enabling the DULT subsystem
===========================

After the DULT user registration, callbacks registration and preset configuration, you must enable the DULT subsystem with the :c:func:`dult_enable` function.
The DULT user that successfully calls this function becomes the associated user.
In the DULT subsystem disabled state, most of the DULT APIs are not available.

The teardown depends on the selected :ref:`ug_dult_api_variant`:

* With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V1` Kconfig option, the :c:func:`dult_reset` function unregisters the current DULT user and callbacks, resets the preset configuration, and disables the DULT subsystem.
* With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V2` Kconfig option, the :c:func:`dult_reset` function releases the association and keeps the DULT user, its callbacks, and its preset configuration registered.
  Only the associated user can call it.
  Complete the teardown with the :c:func:`dult_user_unregister` function, which fails while the user still holds the association.

Use the :c:func:`dult_user_is_associated` and :c:func:`dult_is_any_associated` functions to query the current association state.

.. rst-class:: numbered-step

.. _ug_dult_multi_user:

Handling multiple accessory-locating networks
*********************************************

This step applies only to products that support multiple DULT-based accessory-locating networks.
Skip it if your product registers a single accessory-locating network.

To support this use case, enable the :kconfig:option:`CONFIG_DULT_API_VARIANT_V2` Kconfig option and set the value of the :kconfig:option:`CONFIG_DULT_USER_MAX` Kconfig option to the number of accessory-locating networks that your product registers.
The :kconfig:option:`CONFIG_DULT_API_VARIANT_V1` Kconfig option supports a single DULT user and emits no association notifications.
All registered users share the DULT subsystem during the pre-association window, but just one of them is the associated user.

The associated user is arbitrated by the :c:func:`dult_enable` function and processed in the order received.
The first registered user that calls it becomes the associated user, and every other registered user is evicted.
While the association is held, the :c:func:`dult_enable` function called by another registered user fails, so the associated user is never preempted.
The DULT subsystem notifies all registered users about the arbitration outcome through the :c:member:`dult_multi_user_cb.ownership_claimed` callback.
The callback's ``is_owner`` parameter is set to ``true`` for the user that won the arbitration, and to ``false`` for a user that was evicted.
Releasing the association with the :c:func:`dult_reset` function notifies all registered users through the :c:member:`dult_multi_user_cb.ownership_released` callback.
The callback's ``was_owner`` parameter is set to ``true`` for the user whose association just ended, and to ``false`` for a registered bystander that can claim the association again.

An evicted user stays registered until it calls the :c:func:`dult_user_unregister` function as part of its own teardown.
While another accessory-locating network holds the DULT association, every evicted user must block its own association flow, so that the accessory cannot be associated with two accessory-locating networks at the same time.
The way to achieve this depends on the capabilities of the evicted accessory-locating network stack.

If the eviction is related to a network stack that must be disabled to block the association flow, you have the following options to satisfy this requirement:

* Disable the evicted user stack directly by relying on the losing network API that is used to forward the association status as a listener of the :c:member:`dult_multi_user_cb.ownership_claimed` callback.
  For example, the FHN extension of the Fast Pair module - as the losing network - reports this outcome through the :c:member:`bt_fast_pair_fhn_info_cb.dult_ownership_state_changed` callback.
* Disable the evicted user stack by relying on the callback from the winning network API that indicates the successful association status.
  For example, the FHN extension of the Fast Pair module - as the winning network - reports this outcome through the :c:member:`bt_fast_pair_fhn_info_cb.provisioning_state_changed` callback.

Otherwise, you can keep the evicted user registered and block its association flow at the application code level until the association is released.
Consult the network-specific documentation to find the guidance on the recommended approach.

Claiming the association again is a decision of the user that owns the accessory state, not an automatic consequence of the :c:member:`dult_multi_user_cb.ownership_released` callback.

Both notifications are delivered from the system workqueue, so you can drive the DULT user lifecycle directly from the callback.
Transitions are delivered in order, but toggling the association repeatedly within a single execution context without yielding may collapse cancelling transitions and lose intermediate states.
The final association state is always reported correctly, and two consecutive notifications of the same kind are never delivered.

.. rst-class:: numbered-step

.. _ug_dult_near_owner_state:

Managing the near-owner state
*****************************

The location-tracking accessory can be in one of the two modes of the DULT near-owner state:

* Separated mode - The accessory is separated from the owner.
* Near-owner mode - The accessory is near the owner.

Check with your accessory-locating network provider for information on how to switch between the two modes.
Call the :c:func:`dult_near_owner_state_set` function to set the appropriate DULT near-owner state after registering the DULT user and whenever the state changes.
By default, the DULT near-owner state is set to the near-owner mode on boot.
The state is stored for each DULT user, and the accessory reports the value of the currently associated user.
With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V1` Kconfig option, the state is also reset to the near-owner mode when the :c:func:`dult_reset` function is called.
With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V2` Kconfig option, the state is preserved across the :c:func:`dult_reset` function call and cleared by the :c:func:`dult_user_unregister` function call.
While no user is associated, the effective near-owner state is the near-owner mode, regardless of the values preset by the registered users.
A value preset with the :c:func:`dult_near_owner_state_set` function becomes effective when the user becomes the associated user.

In the near-owner mode, most of the DULT functionalities are unavailable to protect the owner's privacy.

.. rst-class:: numbered-step

.. _ug_dult_anos_access:

Customizing the access policy to the accessory non-owner service
****************************************************************

By default, the accessory non-owner service (ANOS) accepts operations only in the separated near-owner state, as required by the DULT specification.
Your accessory-locating network can override this gate for the Accessory Information operations by registering the :c:struct:`dult_bt_anos_cb` structure with the :c:func:`dult_bt_anos_cb_register` function.
This step is optional and available with both :ref:`DULT API variants <ug_dult_api_variant>`.

The order in which the ANOS verifies each operation depends on whether the :c:struct:`dult_bt_anos_cb` callback structure is registered:

.. tabs::

   .. group-tab:: Without the access policy callback

      1. Operations that are not defined by the DULT specification are rejected.
      #. All operations are rejected outside the separated near-owner state.
      #. Operation-specific preconditions are verified, and a failed precondition is reported with the status defined for that operation.
         For example, operations that depend on an accessory capability that is not declared are rejected.

   .. group-tab:: With the access policy callback

      1. Operations that are not defined by the DULT specification are rejected.
      #. Operations from the Non-owner controls group are rejected outside the separated near-owner state.
         The Accessory Information operations are exempt from this gate.
      #. Operation-specific preconditions are verified, and a failed precondition is reported with the status defined for that operation.
         For example, operations that depend on an accessory capability that is not declared are rejected.
      #. The :c:member:`dult_bt_anos_cb.access_verify` callback is the last gate, and denying the access rejects the operation.

Register the callback structure as described in the :ref:`ug_dult_callback_registration` section.
With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V1` Kconfig option, the ANOS rejects all operations until the DULT subsystem is enabled, so the callback only relaxes the near-owner state gate for the DULT associated user.
With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V2` Kconfig option, the ANOS consults the access policy of the DULT associated user.
It also serves operations that arrive before the DULT association, see the :ref:`ug_dult_anos_pre_association_access` section for details.

.. note::
   Granting access to the accessory non-owner service operations outside the separated near-owner state is a deviation from the DULT specification.
   Make sure that the policy implemented by the :c:member:`dult_bt_anos_cb.access_verify` callback matches the specification of the accessory-locating network layered on top of DULT.

.. _ug_dult_anos_pre_association_access:

Accessing ANOS before associating with an accessory-locating network
====================================================================

This section applies to products that must serve the Accessory Information operations before the accessory is associated with its accessory-locating network, for example during the accessory association flow.

Two associations are involved in this flow:

* The association with the accessory-locating network, which that network defines, for example the provisioning operation of the FHN extension of the Fast Pair module.
* The DULT association, which is claimed with the :c:func:`dult_enable` function and is internal to the DULT subsystem.

The DULT association can be claimed at any point before the accessory is associated with the accessory-locating network.
How early the ANOS can serve the Accessory Information operations depends on the selected :ref:`ug_dult_api_variant`:

.. tabs::

   .. group-tab:: API variant v1

      The ANOS rejects every operation until the DULT association is claimed.
      Register the access policy callback while the DULT subsystem is still disabled, and call the :c:func:`dult_enable` function as soon as your accessory-locating network is ready to serve the Accessory Information operations.

   .. group-tab:: API variant v2

      The ANOS also serves operations that arrive before the DULT association is claimed.
      Use the :c:func:`dult_multi_user_conn_claim` function to specify the connections that are served by a given DULT user, and call it from the Bluetooth LE connected callback of the DULT user that owns the connection.
      The DULT subsystem uses the claim to resolve the DULT user for operations that arrive before the DULT association, and consults the access policy of that user.
      The connection claim is required also when your product registers a single accessory-locating network.

      The following rules apply to the claim:

      * The claim is released automatically when the connection is disconnected.
      * The claim is exclusive, so a connection claimed by one DULT user cannot be claimed by another one.
      * The claim is only needed by a DULT user that serves the accessory non-owner service operations before it claims the DULT association.

Before the accessory is associated with the accessory-locating network, it typically stays in the near-owner mode, so the access policy callback can grant access to the Accessory Information operations only.
The Non-owner controls operations stay unavailable until the accessory switches to the separated mode, see :ref:`ug_dult_near_owner_state`.

.. rst-class:: numbered-step

.. _ug_dult_advertising:

Building the location-enabled advertising payload
*************************************************

The DULT specification defines a location-enabled advertising payload with a fixed part that is common to every accessory-locating network.
Integrating this payload is up to the accessory-locating network, and not every network uses it.
Some networks might define their own advertising payload structure instead.
Check with your accessory-locating network provider whether the network relies on the DULT payload, and skip this step if it does not.

If your accessory-locating network does integrate the DULT location-enabled advertising payload, the network is responsible for advertising it.
Use the :c:func:`dult_bt_adv_data_fill` function to encode the DULT Advertising Data (AD) into the :c:struct:`bt_data` structure.
Pass the network-specific proprietary data through the :c:struct:`dult_bt_adv_data` structure as the function input parameter, and provide the memory for the :c:struct:`bt_data` structure and the associated buffer (``buf`` and ``buf_size``) as output parameters.
The buffer must remain valid for as long as the resulting :c:struct:`bt_data` structure is referenced by your advertising set.
Initialize the :c:struct:`dult_bt_adv_data` structure with one of the following macros, depending on the level of customization you require:

* :c:macro:`DULT_BT_ADV_DATA_INIT` - Takes the proprietary data buffer and its length explicitly.
  Use it when the length is not known at compile time or when only a part of the buffer is used.
* :c:macro:`DULT_BT_ADV_DATA_PROPRIETARY_INIT` - Derives the length from the proprietary data buffer, which must be an array.
* :c:macro:`DULT_BT_ADV_DATA_NO_PROPRIETARY_INIT` - Appends no proprietary data.

For the first two macros, set the :c:member:`dult_bt_adv_data.flags_present` field according to whether the surrounding advertising payload includes the optional Flags AD type.
The field selects the proprietary data length limit:

* :c:macro:`DULT_BT_ADV_PROPRIETARY_DATA_MAX_LEN_WITH_FLAGS` - The limit defined by the DULT specification, applicable when the Flags AD type is present.
* :c:macro:`DULT_BT_ADV_PROPRIETARY_DATA_MAX_LEN_NO_FLAGS` - An extension that reuses the three bytes reserved for the Flags AD type when it is not emitted.

.. note::
   The extended limit exceeds the proprietary data field size defined by the DULT specification.
   Use it only if you can guarantee that the surrounding advertising payload emits no Flags AD type.

Only the currently associated DULT user can call the :c:func:`dult_bt_adv_data_fill` function, because the location-enabled advertising payload is defined only for an associated accessory.
Build the payload after the :c:func:`dult_enable` function call succeeds and stop advertising it once the association is released.
The serialized payload carries the near-owner state as it was at the time of the call.
Serialize the payload again and update your advertising set whenever the DULT near-owner state changes, so that the advertised near-owner state stays accurate.

.. note::
   The DULT module is only responsible for serializing the DULT Advertising Data (AD) payload.
   The remaining requirements for the location-enabled Bluetooth LE advertising, such as the advertising interval and the Bluetooth LE address, are defined by the `DULT`_ specification.
   The code that manages the Bluetooth LE advertising with the DULT payload must comply with all of these requirements.

.. rst-class:: numbered-step

.. _ug_dult_identifier:

Managing the identification process
***********************************

The accessory is required to include a way to uniquely identify it.
One way to satisfy this requirement is to add a mechanism for retrieving the Identifier Payload defined by the accessory-locating network.
The identifier can be retrieved over Bluetooth LE.

Identifier retrieval over Bluetooth LE
======================================

When identifier retrieval over Bluetooth LE is supported, the DULT specification requires the accessory to provide a physical mechanism (for example, button press and hold) that can be utilized to enter the identifier read state for a limited amount of time.
In this state, the DULT subsystem allows the accessory-locating network to read the Identifier Payload.

Set the identifier lookup by Bluetooth LE accessory capability bit (:c:enum:`DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_BLE_BIT_POS`) in the accessory capabilities bitmask when registering the DULT user if you support this method of retrieving the identifier.

To register the identifier read state callbacks, use the :c:func:`dult_id_read_state_cb_register` function.
Call the :c:func:`dult_id_read_state_enter` function to enter the identifier read state.
The identifier read state is automatically exited after a timeout.
Calling the :c:func:`dult_id_read_state_enter` function while the accessory is already in the identifier read state resets the timeout.
When the identifier read state is exited, the DULT subsystem calls the :c:member:`dult_id_read_state_cb.exited` callback to inform the application about this event.
Upon receiving the identifier read request, when the accessory is in the identifier read state, the DULT subsystem calls the :c:member:`dult_id_read_state_cb.payload_get` callback to get the Identifier Payload from the application.
During the callback execution, you must provide the Identifier Payload using callback output parameters.
The Identifier Payload must be constructed according to the requirements defined by the chosen accessory-locating network.

The connected non-owner device requests the identification information using the accessory non-owner service (ANOS) through GATT write operation.
The accessory responds with the Identifier Payload using the ANOS through GATT indication operation.
Configure the :kconfig:option:`CONFIG_DULT_BT_ANOS_ID_PAYLOAD_LEN_MAX` Kconfig option to set the maximum length of your accessory-locating network Identifier Payload.

.. rst-class:: numbered-step

.. _ug_dult_sound:

Using the sound callbacks and managing the sound state
******************************************************

The DULT specification requires the accessory to support the play sound functionality.
For details about the sound maker, see the `DULT Sound maker`_ section of the DULT documentation.

To integrate the play sound functionality, set the play sound accessory capability bit (:c:enum:`DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS`) in the accessory capabilities bitmask.
You must do this when registering the DULT user to indicate support for this functionality.

There are following sound sources available:

* Bluetooth GATT (:c:enum:`DULT_SOUND_SRC_BT_GATT`) - Sound source type originating from the Bluetooth ANOS.
  The non-owner device can trigger the sound callbacks by sending the relevant request message over the DULT GATT service.
* Motion detector (:c:enum:`DULT_SOUND_SRC_MOTION_DETECTOR`) - Sound source type originating from the motion detector.
  The motion detector may trigger the sound callbacks if the accessory separated from the owner for an amount of time controlled by the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MIN` and :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MAX` Kconfig options is moving.
  Used only when the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR` Kconfig option is enabled.
* External (:c:enum:`DULT_SOUND_SRC_EXTERNAL`) - Sound source type originating from the location unknown to the DULT module.
  The accessory-locating network often provides a native mechanism for playing sounds.
  The :c:enum:`DULT_SOUND_SRC_EXTERNAL` sound source is used to notify the DULT module that externally defined sound action is in progress.

To register the sound callbacks, use the :c:func:`dult_sound_cb_register` function.
All sound callbacks defined in the :c:struct:`dult_sound_cb` structure are mandatory to register:

* The sound start request is indicated by the :c:member:`dult_sound_cb.sound_start` callback.
  The minimum duration for the DULT sound action originating from the Bluetooth ANOS is defined by the :c:macro:`DULT_SOUND_DURATION_BT_GATT_MIN_MS`.
  The upper layer determines the sound duration, and for the sound action originating from the Bluetooth ANOS, the duration must exceed the value set in the :c:macro:`DULT_SOUND_DURATION_BT_GATT_MIN_MS` macro.
  In case of the sound action originating from the motion detector, the minimum duration is not defined.
* The sound stop request is indicated by the :c:member:`dult_sound_cb.sound_stop` callback.

All callbacks pass the sound source as a first parameter and only report the internal sound sources (:c:enum:`DULT_SOUND_SRC_BT_GATT` or :c:enum:`DULT_SOUND_SRC_MOTION_DETECTOR`).
The :c:enum:`DULT_SOUND_SRC_EXTERNAL` never appears as the callback parameter as the external sound source cannot originate from the DULT module.
You must treat all callbacks from the :c:struct:`dult_sound_cb` structure as requests.
The internal sound state of the DULT subsystem is not automatically changed on any callback event.
The state is only changed when you acknowledge such a request in your application using the :c:func:`dult_sound_state_update` function.
The application is the ultimate owner of the sound state and only notifies the DULT subsystem about each change.
The :c:func:`dult_sound_state_update` function should be called by the application on each sound state change as defined by the :c:struct:`dult_sound_state_param` structure.
All fields defined in this structure compose the sound state.
You must configure the following fields in the :c:struct:`dult_sound_state_param` structure that is passed to the :c:func:`dult_sound_state_update` function:

* Sound state active flag - Determines whether the sound is currently playing.
* Source of the new sound state - The sound source that triggered the sound state change.

The :c:func:`dult_sound_state_update` function can be used to change the sound state asynchronously, as it is often impossible to execute sound playing action on the speaker device in the context of the requesting callbacks.
Asynchronous support is also necessary to report sound state changes that are triggered by an external source unknown to the DULT subsystem.

.. rst-class:: numbered-step

.. _ug_dult_motion_detector:

Interacting with the motion detector
************************************

DULT motion detector is an optional feature of the DULT subsystem.
For more details, see the `DULT motion detector`_ section of the DULT documentation.
To support the DULT motion detector feature in your project, enable the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR` Kconfig option.

To integrate the motion detector feature, set the motion detector unwanted tracking accessory capability bit (:c:enum:`DULT_ACCESSORY_CAPABILITY_MOTION_DETECTOR_UT_BIT_POS`) in the accessory capabilities bitmask.
You must do this when registering the DULT user to indicate support for this feature.

To register the motion detector callbacks, use the :c:func:`dult_motion_detector_cb_register` function.
You must register all motion detector callbacks defined in the :c:struct:`dult_motion_detector_cb` structure:

* The motion detector start request is indicated by the :c:member:`dult_motion_detector_cb.start` callback.
  After this callback is called, the motion detector events are polled periodically with the :c:member:`dult_motion_detector_cb.period_expired` callback.
  A typical action after the motion detector start request is to power up the accelerometer and start collecting motion data.
* The motion detector period expired event is indicated by the :c:member:`dult_motion_detector_cb.period_expired` callback.
  This callback is called at the end of each motion detector period.
  The :c:member:`dult_motion_detector_cb.start` callback indicates the beginning of the first motion detector period.
  The next period is started as soon as the previous period expires.
  The user should notify the DULT module if motion was detected in the previous period.
  The return value of this callback is used to pass this information.
  The motion must be considered as detected if it fulfills the requirements defined in the `DULT motion detector`_ section of the DULT documentation.
* The motion detector stop request is indicated by the :c:member:`dult_motion_detector_cb.stop` callback.
  It concludes the motion detector activity that was started by the :c:member:`dult_motion_detector_cb.start` callback.
  A typical action after the motion detector stop request is to power down the accelerometer.

The motion detector is started by the DULT subsystem when the accessory is in the separated state for an amount of time controlled by the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MIN` and :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MAX` Kconfig options.
When the motion is detected during the motion detector active period, the DULT subsystem calls the :c:member:`dult_sound_cb.sound_start` callback to request the sound action with the :c:enum:`DULT_SOUND_SRC_MOTION_DETECTOR` parameter as the sound source.
Emitted sounds help to alert the non-owner that they are carrying an accessory that does not belong to them and might be used by the original owner to track their location.

Test mode
=========

Enable the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_TEST_MODE` Kconfig option to shorten the separated unwanted tracking timings, so that the motion detector can be exercised without waiting for the production periods.
With this option enabled, the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_BACKOFF_PERIOD`, :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MIN`, and :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MAX` Kconfig options become configurable and default to short test values.

You can also override the same timings at runtime with the :c:func:`dult_test_motion_detector_separated_ut_period_set` function:

* The runtime values in the :c:struct:`dult_test_motion_detector_separated_ut_period` structure are expressed in seconds, while the Kconfig options are expressed in minutes.
* New values take effect on the next timer arm and do not restart a timer that is already running.

For the accepted value ranges, see the API documentation of the :ref:`dult_readme` library.

.. caution::
   The :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_TEST_MODE` option selects the :kconfig:option:`CONFIG_DULT_TEST` option, which enables the test-only layer of the DULT module.
   Use it for qualification and debugging only, and never enable it in a production build.

.. rst-class:: numbered-step

.. _ug_dult_battery:

Managing the battery information
********************************

DULT battery information is an optional feature of the DULT GATT service.
You can enable the :kconfig:option:`CONFIG_DULT_BATTERY` Kconfig option to support the DULT battery information in your project.
Select the battery type that your device uses (see the :kconfig:option:`CONFIG_DULT_BATTERY_TYPE` choice configuration).
You can also configure the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_CRITICAL_THR`, :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_LOW_THR`, and :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_MEDIUM_THR` Kconfig options to specify the mapping between a battery level expressed as a percentage value and battery levels defined in the `DULT`_ specification.
The battery level expressed as a percentage value is mapped to one of four battery levels defined in the DULT specification:

* Full battery level - The battery level is higher than the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_MEDIUM_THR` Kconfig option threshold and less than or equal to 100%.
* Medium battery level - The battery level is higher than the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_LOW_THR` Kconfig option threshold and less than or equal to the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_MEDIUM_THR` Kconfig option threshold.
* Low battery level - The battery level is higher than the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_CRITICAL_THR` Kconfig option threshold and less than or equal to the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_LOW_THR` Kconfig option threshold.
* Critically low battery level - The battery level is higher than or equal to 0% and less than or equal to the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_CRITICAL_THR` Kconfig option threshold.

When the battery information is enabled, use the :c:func:`dult_battery_level_set` function to set the current battery level.
The battery level is stored for each DULT user, so each registered user must set its own value after its registration.
With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V1` Kconfig option, you must set the battery level before enabling DULT, as the :c:func:`dult_enable` function fails otherwise.
With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V2` Kconfig option, the :c:func:`dult_enable` function succeeds with the battery level unset, but the accessory non-owner service rejects the battery read operations until the associated user sets its battery level.
Set the battery level of each registered user before that user can become the associated user.
To keep the battery level information accurate, you should set the battery level to the new value with the help of this function as soon as the device battery level changes.
If the :kconfig:option:`CONFIG_DULT_BATTERY` Kconfig option is disabled, the :c:func:`dult_battery_level_set` function must not be used.

Applications and samples
************************

The following sample use the DULT integration in the |NCS|:

* :ref:`fast_pair_locator_tag` sample (uses the FHN extension of the :ref:`bt_fast_pair_readme` that integrates the DULT specification)

Library support
***************

The following |NCS| library support the DULT integration:

* :ref:`dult_readme` library implements the DULT specification and provides the APIs required for :ref:`ug_dult` with the |NCS|.

Terms and licensing
*******************

The use of DULT may be subject to terms and licensing.
Refer to the official `DULT`_ documentation for development-related licensing information.

Dependencies
************

The following are the required dependencies for the DULT integration:

* :ref:`zephyr:bluetooth`
