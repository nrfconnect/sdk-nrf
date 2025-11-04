.. _matter_manufacturer_specific_sample:

Matter: Manufacturer-specific
#############################

.. contents::
   :local:
   :depth: 2

This sample demonstrates an implementation of custom manufacturer-specific clusters used by the application layer.
This sample uses development kit's buttons and LEDs to demonstrate the functionality of the custom ``NordicDevkit`` cluster.
The device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power, 802.15.4 Thread network or on top of a Wi-Fi® network.
Support for both Thread and Wi-Fi is mutually exclusive and depends on the hardware platform, so only one protocol can be supported for a specific Matter device.
In case of Thread, this device works as a Thread :ref:`Minimal End Device <thread_ot_device_types>`.
You can use this sample as a reference for creating your own application.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

For testing purposes, that is to commission the device and :ref:`control it remotely <matter_manufacturer_specific_network_mode>` through a Thread network or a Wi-Fi network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>`.
This requires additional hardware depending on the setup you choose.

.. note::
    |matter_gn_required_note|

IPv6 network support
====================

The development kits for this sample offer the following IPv6 network support for Matter:

* Matter over Thread is supported for the ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf21540dk/nrf52840``, ``nrf54l15dk/nrf54l15/cpuapp``, ``nrf54l15dk/nrf54l10/cpuapp`` and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets.
* Matter over Wi-Fi is supported for the ``nrf5340dk/nrf5340/cpuapp`` board target with the ``nrf7002ek`` shield attached, ``nrf7002dk/nrf5340/cpuapp`` board target, or ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target with the ``nrf7002eb2`` shield attached.

Overview
********

The sample starts the Bluetooth® LE advertising automatically and prepares the Matter device for commissioning into a Matter-enabled IPv6 network.

.. tabs::

   .. group-tab:: nRF52, nRF53 and nRF70 DKs

      The sample uses the **LED 1** to show the state of the connection.
      You can press **Button 1** to start the factory reset when needed.
      **Button 2** is used to set the state of the ``NordicDevkit`` cluster's attribute, ``UserButton``.
      **LED 2** reflects the state of the ``UserLED``.

   .. group-tab:: nRF54 DKs

      The sample uses the **LED 0** to show the state of the connection.
      You can press **Button 0** to start the factory reset when needed.
      **Button 1** is used to set the state of the ``NordicDevkit`` cluster's attribute, ``UserButton``.
      **LED 1** reflects the state of the ``UserLED``.

The Matter command ``SetLED`` is used to control the state of ``UserLED``.
It takes one argument - the action to be performed (``0`` to turn the LED off, ``1`` to turn it on, ``2`` to toggle the state).
The ``UserLED`` attribute is persistent and stored across the reboots.
The ``UserButtonChanged`` event is generated when the ``UserButton`` attribute is changed.

The ``NordicDevkit`` cluster introduces a writable ``DevKitName`` attribute, of string type as well.
The ``DevKitName`` attribute is persistent and stored across the reboots.

The sample additionally extends the ``Basic Information`` cluster with a ``RandomNumber`` attribute and ``GenerateRandom`` command that updates the ``RandomNumber`` with a random value.
The ``RandomNumberChanged`` event is generated when the ``RandomNumber`` attribute is changed.
The ``RandomNumber`` attribute value is not persistent and it is generated on each application's boot.

Custom manufacturer-specific cluster
====================================

The sample provides a custom ``NordicDevkit`` cluster, configured in the :file:`src/default_zap/NordicDevKitCluster.xml` file.
To learn more about adding custom clusters to your Matter application, see the :ref:`ug_matter_creating_custom_cluster` section.

.. _matter_manufacturer_specific_network_mode:

Remote testing in a network
===========================

By default, the Matter accessory device has no IPv6 network configured.
You must pair it with the Matter controller over Bluetooth LE to get the configuration from the controller to use the device within a Thread network.
The controller must get the `Onboarding information`_ from the Matter accessory device and provision the device into the network.
For details, see the `Commissioning the device`_ section.

Configuration
*************

|config|

.. _matter_manufacturer_specific_custom_configs:

Matter manufacturer-specific sample with Trusted Firmware-M
===========================================================

.. include:: ../template/README.rst
    :start-after: matter_template_build_with_tfm_start
    :end-before: matter_template_build_with_tfm_end

Advanced configuration options
==============================

This section describes other configuration options for the sample.

Device Firmware Upgrade support
-------------------------------

.. |Bluetooth| replace:: Bluetooth

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_build_with_dfu_start
    :end-before: matter_door_lock_sample_build_with_dfu_end

Factory data support
--------------------

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_factory_data_start
    :end-before: matter_door_lock_sample_factory_data_end

User interface
**************

.. tabs::

   .. group-tab:: nRF52, nRF53 and nRF70 DKs

      LED 1:
         .. include:: /includes/matter_sample_state_led.txt

      LED 2:
         Reflects the state of ``UserLED`` attribute in the ``NordicDevkit`` cluster.

      Button 1:
         .. include:: /includes/matter_sample_button.txt

      Button 2:
         Sets the state of ``UserButton`` attribute in the ``NordicDevkit`` cluster to ``true`` on press and ``false`` on release.

      .. include:: /includes/matter_segger_usb.txt

   .. group-tab:: nRF54 DKs

      LED 0:
         .. include:: /includes/matter_sample_state_led.txt

      LED 1:
         Reflects the state of ``UserLED`` attribute in the ``NordicDevkit`` cluster.

      Button 0:
         .. include:: /includes/matter_sample_button.txt

      Button 1:
         Sets the state of ``UserButton`` attribute in the ``NordicDevkit`` cluster to ``true`` on press and ``false`` on release.

      .. include:: /includes/matter_segger_usb.txt


Building and running
********************

.. |sample path| replace:: :file:`samples/matter/manufacturer_specific`

.. include:: /includes/build_and_run.txt

See `Configuration`_ for information about building the sample with the DFU support.

Building the Matter over Wi-Fi sample variant on nRF5340 DK with nRF7002 EK shield
==================================================================================

.. include:: /includes/matter_building_nrf5340dk_70ek

Building the Matter over Wi-Fi sample variant on nRF54LM20 DK with nRF7002-EB II shield
=======================================================================================

.. include:: /includes/matter_building_nrf54lm20dk_7002eb2

Flashing the Matter over Wi-Fi sample variant
=============================================

.. include:: /includes/matter_sample_wifi_flash.txt

Enabling remote control
=======================

Remote control allows you to control the Matter manufacturer-specific device from an IPv6 network.

`Commissioning the device`_ allows you to set up a testing environment and remotely control the sample over a Matter-enabled Thread network.

.. _matter_manufacturer_specific_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_commissioning_start
    :end-before: matter_light_bulb_sample_commissioning_end

.. _matter_manufacturer_specific_network_mode_onboarding:

Onboarding information
++++++++++++++++++++++

When you start the commissioning procedure, the controller must get the onboarding information from the Matter accessory device.
The onboarding information representation depends on your commissioner setup.

For this sample, you can use one of the following :ref:`onboarding information formats <ug_matter_network_topologies_commissioning_onboarding_formats>` to provide the commissioner with the data payload that includes the device discriminator and the setup PIN code:

  .. list-table:: Manufacturer-specific sample onboarding information
     :header-rows: 1

     * - QR Code
       - QR Code Payload
       - Manual pairing code
     * - Scan the following QR code with the app for your ecosystem:

         .. figure:: ../../../doc/nrf/images/matter_qr_code_template_sample.png
            :width: 200px
            :alt: QR code for commissioning the manufactuter specific device

       - MT:Y.K9042C00KA0648G00
       - 34970112332

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_onboarding_start
    :end-before: matter_door_lock_sample_onboarding_end

|matter_cd_info_note_for_samples|

Testing
=======

After `Enabling remote control`_, you can test the ``NordicDevkit`` cluster's attributes and commands.

Testing custom cluster
----------------------

To test ``NordicDevkit`` cluster's attributes and commands, complete the following steps:

1. |connect_kit|
#. |connect_terminal_ANSI|
#. Commission an accessory with node ID equal to 1 to the Matter network by following the steps described in the `Commissioning the device`_ section.
#. Run the chip-tool in the interactive mode:

   .. parsed-literal::
      :class: highlight

      chip-tool interactive start

#. Read the ``NordicDevkit`` cluster's attributes by index:

   .. parsed-literal::
      :class: highlight

      any read-by-id 0xFFF1FC01 *attribute-id* 1 1

   * *attribute-id* is the attribute's ID, equal to ``0xFFF10000`` for ``DevKitName``, ``0xFFF10001`` for ``UserLED`` and ``0xFFF10002`` for ``UserButton`` attributes for the ``NordicDevkit`` cluster in this sample.
#. Verify that all attributes have been read correctly and are equal to the default values defined in cluster's configuration.
#. Write the ``DevkitName`` attribute:

   .. parsed-literal::
      :class: highlight

      any write-by-id 0xFFF1FC01 0xFFF10000 "NewName" 1 1

#. Read the ``DevkitName`` attribute again to check if it has changed.
#. Send the ``SetLED`` command to the device to control the LED state:

   .. parsed-literal::
      :class: highlight

      any command-by-id 0xFFF1FC01 0xFFF10000 '{ "0x0": "u:*action*" }' 1 1

   * *action* is the action that should be performed on LED attribute: ``0`` to turn the LED off, ``1`` to turn it on, ``2`` to toggle the state.

#. Verify that the LED state has changed and the attribute value is updated.
#. Subscribe to the ``UserButton`` attribute to monitor the button state:

   .. parsed-literal::
      :class: highlight

      any subscribe-by-id 0xFFF1FC01 0xFFF10002 0 120 1 1

#. Press the button assigned to the ``UserButton`` and check if the attribute state is updated in the chip-tool.
#. Read the ``UserButtonChanged`` event to check that events were generated on ``UserButton`` attribute changes.

   .. parsed-literal::
      :class: highlight

      any read-event-by-id 0xFFF1FC01 0xFFF10000 1 1

#. Read the ``Basic Information`` cluster's ``RandomNumber`` attribute:

   .. parsed-literal::
      :class: highlight

      any read-by-id 0x0028 0x17 1 0

#. Send the ``GenerateRandom`` command to the device to update the ``RandomNumber`` attribute:

   .. parsed-literal::
      :class: highlight

      any command-by-id 0x0028 0 '{}' 1 0

#. Verify that the random value has been generated and the attribute value is updated.
#. Read the ``Basic Information`` cluster's ``RandomNumberChanged`` event to check that events were generated on ``RandomNumber`` attribute changes.

   .. parsed-literal::
      :class: highlight

      any read-event-by-id 0x0028 0x4 1 0

#. Reboot the device, restart the chip-tool, and check if the attributes are persisting after joining the network.

Upgrading the device firmware
=============================

To upgrade the device firmware, complete the steps listed for the selected method in the :doc:`matter:nrfconnect_examples_software_update` tutorial of the Matter documentation.

Dependencies
************

This sample uses the Matter library that includes the |NCS| platform integration layer:

* `Matter`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
