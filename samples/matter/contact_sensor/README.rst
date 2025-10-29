.. _matter_contact_sensor_sample:

Matter: Contact sensor
######################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the :ref:`Matter <ug_matter>` application layer to build a device capable of detecting a contact.
This device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power 802.15.4 Thread network.
You can use this sample as a reference for creating your own application.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

If you want to commission the device and :ref:`control it remotely <matter_contact_sensor_network_mode>`, you also need a Matter controller device :ref:`configured on PC or mobile <ug_matter_configuring>`.
This requires additional hardware depending on the setup you choose.

.. note::
    |matter_gn_required_note|

Overview
********

The sample does not use a real contact sensor due to a hardware limitation.
It simulates contact detection, which you can initiate by pressing and releasing a button.
The contact sensor state is represented by the BooleanState cluster on the endpoint ``1``.

You can test the device remotely over a Thread network, which requires more devices.

The remote control testing requires a Matter controller that you can configure either on a PC or a mobile device.
You can enable both methods after :ref:`building and running the sample <matter_contact_sensor_sample_remote_control>`.

Testing with the Matter Quick Start app
=======================================

.. |sample_type| replace:: sample

.. include:: /includes/matter_quick_start.txt

.. _matter_contact_sensor_sample_lit:

ICD LIT device type
===================

.. include:: ../smoke_co_alarm/README.rst
    :start-after: matter_smoke_co_alarm_sample_lit_start
    :end-before: matter_smoke_co_alarm_sample_lit_end

.. _matter_contact_sensor_network_mode:

Remote testing in a network
===========================

.. |Bluetoothsc| replace:: Bluetooth®
.. |WiFi| replace:: Wi-Fi®

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_remote_testing_start
    :end-before: matter_light_bulb_sample_remote_testing_end

Configuration
*************

|config|

.. _matter_contact_sensor_custom_configs:

Matter contact sensor custom configurations
===========================================

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_configuration_file_types_start
    :end-before: matter_light_bulb_sample_configuration_file_types_end

.. |Bluetooth| replace:: Bluetooth

.. include:: /includes/advanced_conf_matter.txt

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         .. include:: /includes/matter_sample_state_led.txt

      LED 2:
         Shows the state of the contact detection.
         The following states are possible:

         * Solid On - The contact is detected.
         * Off - The contact is not detected.

         Additionally, the LED starts blinking evenly (500 ms on/500 ms off) when the ``Identify`` command of the Identify cluster is received on the endpoint ``1``.
         You can specify the duration of the effect using the command arguments.

      Button 1:
         .. include:: /includes/matter_sample_button.txt

      Button 2:
         Simulates contact detection.
         Pressing the button sets the contact state to detected.
         Releasing the button sets the contact state to not detected.

      Button 3:
         Functions as the User Active Mode Trigger (UAT) button.
         For more information about Intermittently Connected Devices (ICD) and User Active Mode Trigger, see the :ref:`ug_matter_device_low_power_icd` documentation section.

      .. include:: /includes/matter_segger_usb.txt

   .. group-tab:: nRF54 DKs

      LED 0:
         .. include:: /includes/matter_sample_state_led.txt

      LED 1:
         Shows the state of the contact detection.
         The following states are possible:

         * Solid On - The contact is detected.
         * Off - The contact is not detected.

         Additionally, the LED starts blinking evenly (500 ms on/500 ms off) when the ``Identify`` command of the Identify cluster is received on the endpoint ``1``.
         You can specify the duration of the effect using the command arguments.

      Button 0:
         .. include:: /includes/matter_sample_button.txt

      Button 1:
         Simulates contact detection.
         Pressing the button sets the contact state to detected.
         Releasing the button sets the contact state to not detected.

      Button 2:
         Functions as the User Active Mode Trigger (UAT) button.
         For more information about Intermittently Connected Devices (ICD) and User Active Mode Trigger, see the :ref:`ug_matter_device_low_power_icd` documentation section.

      .. include:: /includes/matter_segger_usb.txt

      NFC port with antenna attached:
         Optionally used for obtaining the `Onboarding information`_ from the Matter accessory device to start the :ref:`commissioning procedure <matter_light_bulb_sample_remote_control_commissioning>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/contact_sensor`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

See `Configuration`_ for information on building the sample with the DFU support.

Selecting a custom configuration
================================

Before you start testing the application, you can select one of the :ref:`matter_contact_sensor_custom_configs`.
See :ref:`app_build_file_suffixes` and :ref:`cmake_options` for more information on how to select a configuration.

Testing
=======

After building the sample and programming it to your development kit, complete the following steps to test its basic features.

.. note::
   The following steps use the CHIP Tool controller as an example.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      #. |connect_kit|
      #. |connect_terminal_ANSI|
      #. If the device was not erased during the programming, press and hold **Button 1** until the factory reset takes place.
      #. Commission the device to the Matter network.
         See `Commissioning the device`_ for more information.
      #. Read the contact detection state by invoking the following command with the *<node_id>* and *<endpoint_id>* replaced with your values (for example, ``1`` and ``1``):

         .. code-block:: console

            ./chip-tool booleanstate read state-value <node_id> <endpoint_id>

         The received output will look similar to the following:

         .. code-block:: console

            [1756375237.321] [61118:61120] [TOO] Endpoint: 1 Cluster: 0x0000_0045 Attribute 0x0000_0000 DataVersion: 2772696482
            [1756375237.321] [61118:61120] [TOO]   StateValue: FALSE


      #. Press the **Button 2** to simulate contact detection.
         Read the contact detection state again using the same command as before (do not release the button):

         .. code-block:: console

            ./chip-tool booleanstate read state-value <node_id> <endpoint_id>

         The received value will be different, for example:

         .. code-block:: console

            [1756375237.321] [61118:61120] [TOO] Endpoint: 1 Cluster: 0x0000_0045 Attribute 0x0000_0000 DataVersion: 2772696482
            [1756375237.321] [61118:61120] [TOO]   StateValue: TRUE

      #. Subscribe to the contact detection state changes to see the state change in real time:

         .. code-block:: console

            ./chip-tool booleanstate subscribe state-value 0 300 <node_id> <endpoint_id>

      #. Press and release **Button 2** a few times and observe the state changes reported automatically in the CHIP Tool terminal.

   .. group-tab:: nRF54 DKs

      #. |connect_kit|
      #. |connect_terminal_ANSI|
      #. If the device was not erased during the programming, press and hold **Button 0** until the factory reset takes place.
      #. Commission the device to the Matter network.
         See `Commissioning the device`_ for more information.
      #. Read the contact detection state by invoking the following command with the *<node_id>* and *<endpoint_id>* replaced with your values (for example, ``1`` and ``1``):

         .. code-block:: console

            ./chip-tool booleanstate read state-value <node_id> <endpoint_id>

         The received output will look similar to the following:

         .. code-block:: console

            [1756375237.321] [61118:61120] [TOO] Endpoint: 1 Cluster: 0x0000_0045 Attribute 0x0000_0000 DataVersion: 2772696482
            [1756375237.321] [61118:61120] [TOO]   StateValue: FALSE


      #. Press the **Button 1** to simulate contact detection.
         Read the contact detection state again using the same command as before (do not release the button):

         .. code-block:: console

            ./chip-tool booleanstate read state-value <node_id> <endpoint_id>

         The received value will be different, for example:

         .. code-block:: console

            [1756375237.321] [61118:61120] [TOO] Endpoint: 1 Cluster: 0x0000_0045 Attribute 0x0000_0000 DataVersion: 2772696482
            [1756375237.321] [61118:61120] [TOO]   StateValue: TRUE

      #. Subscribe to the contact detection state changes to see the state change in real time:

         .. code-block:: console

            ./chip-tool booleanstate subscribe state-value 0 300 <node_id> <endpoint_id>

      #. Press and release **Button 1** a few times and observe the state changes reported automatically in the CHIP Tool terminal.

.. _matter_contact_sensor_sample_remote_control:

Enabling remote control
=======================

Remote control allows you to control the Matter contact sensor device from an IPv6 network.

`Commissioning the device`_ allows you to set up a testing environment and remotely control the sample over a Matter-enabled Thread network.

.. _matter_contact_sensor_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_commissioning_start
    :end-before: matter_light_bulb_sample_commissioning_end

Before starting the commissioning procedure, make sure that the device is discoverable over Bluetooth LE.
The device is automatically discoverable upon the device startup, but only for a predefined period of time (one hour by default).
If the Bluetooth LE advertising times out, enable it again.

Onboarding information
++++++++++++++++++++++

When you start the commissioning procedure, the controller must get the onboarding information from the Matter accessory device.
The onboarding information representation depends on your commissioner setup.

For this sample, you can use one of the following :ref:`onboarding information formats <ug_matter_network_topologies_commissioning_onboarding_formats>` to provide the commissioner with the data payload that includes the device discriminator and the setup PIN code:

  .. list-table:: Contact Sensor sample onboarding information
     :header-rows: 1

     * - QR Code
       - QR Code Payload
       - Manual pairing code
     * - Scan the following QR code with the app for your ecosystem:

         .. figure:: ../../../doc/nrf/images/matter_qr_code_contact_sensor.png
            :width: 200px
            :alt: QR code for commissioning the contact sensor device

       - MT:Y.K9042C00KA0648G00
       - 34970112332

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_onboarding_start
    :end-before: matter_door_lock_sample_onboarding_end

|matter_cd_info_note_for_samples|

Upgrading the device firmware
=============================

To upgrade the device firmware, complete the steps listed for the selected method in the :doc:`matter:nrfconnect_examples_software_update` tutorial in the Matter documentation.

Dependencies
************

This sample uses the Matter library that includes the |NCS| platform integration layer:

* `Matter`_

In addition, it uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`nfc_uri`
* :ref:`lib_nfc_t2t`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
