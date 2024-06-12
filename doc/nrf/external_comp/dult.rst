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
   The optional features are not fully supported.

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
#. :ref:`Managing the near-owner state <ug_dult_near_owner_state>`
#. :ref:`Managing the identification process <ug_dult_identifier>`
#. :ref:`Using the sound callbacks and managing the sound state <ug_dult_sound>`
#. :ref:`Managing the battery information <ug_dult_battery>`

These steps are described in the following sections.

The DULT standard implementation in the |NCS| integrates the location-tracking accessory role.
For an integration example, see the Find My Device Network (FMDN) extension of the :ref:`bt_fast_pair_readme`.
Also see the :ref:`fast_pair_locator_tag` sample that integrates the Fast Pair with the FMDN extension, which integrates the :ref:`dult_readme` module.

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
* Knowledge on how to construct the Identifier Payload - The accessory-locating network defines its own identifier that allows the network to identify the accessory in case of unwanted tracking, and to share obfuscated accessory owner information with a tracked individual.

.. rst-class:: numbered-step

.. _ug_dult_prerequisite_ops:

Performing prerequisite operations
**********************************

You must enable the :kconfig:option:`CONFIG_DULT` Kconfig option to support the DULT standard in your project.

Several Kconfig options are available to configure the DULT integration.
For more details, see the :ref:`Configuration <dult_configuration>` section of the :ref:`dult_readme`.

DULT user
=========

The DULT subsystem introduces the concept of a DULT user.
The DULT subsystem can be used by only one DULT user at a time.
To use the DULT subsystem, you must register a DULT user by calling the :c:func:`dult_user_register` function before you can call any other function from the DULT API.
Upon registration, you must provide the DULT user configuration to the DULT subsystem.
The DULT user configuration includes the following data:

* Registration data - The data obtained during :ref:`registering the device <ug_dult_registering>`.
* Accessory capabilities - Capabilities of your accessory.
  Set appropriate bits in the bitmask to indicate the supported capabilities.
  There are following capabilities available:

  * Play sound (:c:enum:`DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS`) - A mandatory feature that enables the accessory to emit sound signals.
  * Motion detector unwanted tracking (:c:enum:`DULT_ACCESSORY_CAPABILITY_MOTION_DETECTOR_UT_BIT_POS`) - An optional feature that improves security by preventing unwanted tracking.
  * Identifier lookup by NFC (:c:enum:`DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_NFC_BIT_POS`) - A feature supporting identifier lookup by NFC functionality.
    It is optional, but becomes mandatory if the identifier lookup by Bluetooth® LE is not supported.
  * Identifier lookup by Bluetooth LE (:c:enum:`DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_BLE_BIT_POS`) - A feature supporting identifier lookup by Bluetooth LE functionality.
    It is optional, but becomes mandatory if identifier lookup by NFC is not supported.

* Firmware version - The firmware version of your accessory.

To change the DULT user, you must reset the DULT subsystem by calling the :c:func:`dult_reset` function.
This function unregisters the registered DULT user information and callbacks.

Callback registration
=====================

An application can communicate with the DULT subsystem using API calls and registered callbacks.
The DULT subsystem uses the registered callbacks to inform the application about the DULT-related events and to retrieve the necessary information from the application.

The application must register the required callbacks before it enables the DULT subsystem and starts to operate as the location-tracking accessory.
To identify the callback registration functions in the DULT API, look for the ``_cb_register`` suffix.
Set your application-specific callback functions in the callback structure, which serves as the input parameter for the ``..._cb_register`` API function.
The callback structure must persist in the application memory (static declaration), as during the registration, the DULT subsystem stores only the memory pointer to it.
Use the following functions to register callbacks:

  * :c:func:`dult_id_read_state_cb_register` (mandatory)
  * :c:func:`dult_sound_cb_register` (mandatory)

Preset configuration
====================

Before enabling the DULT subsystem, you should preset the initial accessory configuration with dedicated APIs that depend on the chosen feature set or the accessory state.
The preset configuration is available for the following API functions:

* :c:func:`dult_near_owner_state_set` - Apply this configuration in case the accessory state is different than the default value (see the :ref:`Managing the near-owner state <ug_dult_near_owner_state>` section for more details).
* :c:func:`dult_battery_level_set` - Apply this configuration in case the battery feature is enabled with the :kconfig:option:`CONFIG_DULT_BATTERY` (see the :ref:`Managing the battery information <ug_dult_battery>` section for more details).

Enabling the DULT subsystem
===========================

After the DULT user registration, callbacks registration and preset configuration, you must enable the DULT subsystem with the :c:func:`dult_enable` function.
To unregister the current DULT user and callbacks, reset the preset configuration, and disable the DULT subsystem, use the :c:func:`dult_reset` function.
No additional steps are required to integrate the DULT implementation.
In the DULT subsystem disabled state, most of the DULT APIs are not available.

.. rst-class:: numbered-step

.. _ug_dult_near_owner_state:

Managing the near-owner state
*****************************

The location-tracking accessory can be in one of the two modes of the DULT near-owner state:

* Separated mode - The accessory is separated from the owner.
* Near-owner mode - The accessory is near the owner.

Check with your accessory-locating network provider for information on how to switch between the two modes.
Call the :c:func:`dult_near_owner_state_set` function to set the appropriate DULT near-owner state after registering the DULT user and whenever the state changes.
By default, the DULT near-owner state is set to the near-owner mode on boot and when the :c:func:`dult_reset` function is called.

In the near-owner mode, most of the DULT functionalities are unavailable to protect the owner's privacy.

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
* External (:c:enum:`DULT_SOUND_SRC_EXTERNAL`) - Sound source type originating from the location unknown to the DULT module.
  The accessory-locating network often provides a native mechanism for playing sounds.
  The :c:enum:`DULT_SOUND_SRC_EXTERNAL` sound source is used to notify the DULT module that externally defined sound action is in progress.

To register the sound callbacks, use the :c:func:`dult_sound_cb_register` function.
All sound callbacks defined in the :c:struct:`dult_sound_cb` structure are mandatory to register:

* The sound start request is indicated by the :c:member:`dult_sound_cb.sound_start` callback.
  The minimum duration for the DULT sound action is defined by the :c:macro:`DULT_SOUND_DURATION_MIN_MS`.
  The upper layer determines the sound duration, and the duration must be greater than the value set in the :c:macro:`DULT_SOUND_DURATION_MIN_MS` macro.
* The sound stop request is indicated by the :c:member:`dult_sound_cb.sound_stop` callback.

All callbacks pass the sound source as a first parameter and only report the internal sound sources.
Currently, callbacks always use the :c:enum:`DULT_SOUND_SRC_BT_GATT` (internal) sound source.
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

When the battery information is enabled, you must call the :c:func:`dult_battery_level_set` function after registering the DULT user and before enabling DULT.
This function sets the current battery level.
To keep the battery level information accurate, you should set the battery level to the new value with the help of this function as soon as the device battery level changes.
If the :kconfig:option:`CONFIG_DULT_BATTERY` Kconfig option is disabled, the :c:func:`dult_battery_level_set` function must not be used.

Applications and samples
************************

The following sample use the DULT integration in the |NCS|:

* :ref:`fast_pair_locator_tag` sample (uses the FMDN extension of the :ref:`bt_fast_pair_readme` that integrates the DULT specification)

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
