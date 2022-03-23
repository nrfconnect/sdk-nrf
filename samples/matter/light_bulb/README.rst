.. _matter_light_bulb_sample:
.. _chip_light_bulb_sample:

Matter: Light bulb
##################

.. contents::
   :local:
   :depth: 2

This light bulb sample demonstrates the usage of the :ref:`Matter <ug_matter>` (formerly Project Connected Home over IP, Project CHIP) application layer to build a white dimmable light bulb device.
This device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power, 802.15.4 Thread network.
You can use this sample as a reference for creating your own application.

.. note::
    This sample is self-contained and can be tested on its own.
    However, it is required when testing the :ref:`Matter light switch <matter_light_switch_sample>` sample.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf5340dk_nrf5340_cpuapp, nrf21540dk_nrf52840

For remote testing scenarios, you also need the following:

* To commission the light bulb device and :ref:`control it remotely <matter_light_bulb_network_mode>` through a Thread network: a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>` (which requires additional hardware depending on which setup you choose).
* To use the :ref:`test mode <matter_light_bulb_sample_test_mode>` and control the light bulb using light switch: the :ref:`Matter light switch <matter_light_switch_sample>` sample programmed to another supported development kit.

.. note::
    |matter_gn_required_note|

Overview
********

The sample uses buttons to test changing the light bulb and device states, and LEDs to show the state of these changes.
You can test it in the following ways:

* Standalone, by using a single DK that runs the light bulb application.
* Remotely over the Thread protocol, which requires more devices.

The remote control testing requires a Matter controller that you can configure on a PC, mobile device (for remote testing in a network), or on an embedded device (for remote testing using test mode).
You can enable both methods after :ref:`building and running the sample <matter_light_bulb_sample_remote_control>`.

.. _matter_light_bulb_network_mode:

Remote testing in a network
===========================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_remote_testing_start
    :end-before: matter_door_lock_sample_remote_testing_end

.. _matter_light_bulb_sample_test_mode:

Remote testing using test mode
==============================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_test_mode_start
    :end-before: matter_door_lock_sample_test_mode_end

Using the test mode allows you to control the light bulb remotely without using a smartphone compatible with Android.
You can use the light bulb device programmed with this sample together with a light switch device programmed with the :ref:`Matter light switch <matter_light_switch_sample>` sample to create a simplified Thread network.

Configuration
*************

|config|

Matter light bulb build types
=============================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_configuration_file_types_start
    :end-before: matter_door_lock_sample_configuration_file_types_end

Device Firmware Upgrade support
===============================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_build_with_dfu_start
    :end-before: matter_door_lock_sample_build_with_dfu_end

FEM support
===========

.. include:: /includes/sample_fem_support.txt

User interface
**************

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_led1_start
    :end-before: matter_door_lock_sample_led1_end

LED 2:
    Shows the state of the light bulb.
    The following states are possible:

    * Solid On - The light bulb is on.
    * Off - The light bulb is off.

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_button1_start
    :end-before: matter_door_lock_sample_button1_end

Button 2:
    Changes the light bulb state to the opposite one.

Button 3:
    Starts the Thread networking in the :ref:`test mode <matter_light_bulb_sample_test_mode>` using the default configuration.

    When running the application in light switch test mode, it also starts publishing the light bulb service messages for a predefined period of time to advertise the light bulb device IP address to the light switch device (if used).
    If the light switch device cannot receive messages during this time, press the button again to restart the publishing service.

Button 4:
    Starts the NFC tag emulation, enables Bluetooth LE advertising for the predefined period of time (15 minutes by default), and makes the device discoverable over Bluetooth LE.
    This button is used during the :ref:`commissioning procedure <matter_light_bulb_sample_remote_control_commissioning>`.

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_jlink_start
    :end-before: matter_door_lock_sample_jlink_end

NFC port with antenna attached:
    Optionally used for obtaining the commissioning information from the Matter accessory device to start the :ref:`commissioning procedure <matter_light_bulb_sample_remote_control_commissioning>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/light_bulb`

.. include:: /includes/build_and_run.txt

See `Configuration`_ for information about building the sample with the DFU support.

Selecting a build type
======================

Before you start testing the application, you can select one of the `Matter light bulb build types`_, depending on your building method.

Selecting a build type in |VSC|
-------------------------------

.. include:: /gs_modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

Selecting a build type from command line
----------------------------------------

.. include:: /gs_modifying.rst
   :start-after: build_types_selection_cmd_start
   :end-before: For example, you can replace the

For example, you can replace the *selected_build_type* variable to build the ``release`` firmware for ``nrf52840dk_nrf52840`` by running the following command in the project directory:

.. parsed-literal::
   :class: highlight

   west build -b nrf52840dk_nrf52840 -d build_nrf52840dk_nrf52840 -- -DCONF_FILE=prj_release.conf

The ``build_nrf52840dk_nrf52840`` parameter specifies the output directory for the build files.

.. note::
   If the selected board does not support the selected build type, the build is interrupted.
   For example, if the ``shell`` build type is not supported by the selected board, the following notification appears:

   .. code-block:: console

      File not found: ./ncs/nrf/samples/matter/light_bulb/configuration/nrf52840dk_nrf52840/prj_shell.conf

Testing
=======

After building the sample and programming it to your development kit, complete the following steps to test its basic features.

You can either test the sample's basic features or use the light switch sample to test the light bulb's :ref:`communication with another device <matter_light_bulb_sample_light_switch_tests>`.

.. _matter_light_bulb_sample_basic_features_tests:

Testing basic features
----------------------

After building the sample and programming it to your development kit, complete the following steps to test its basic features:

#. |connect_kit|
#. |connect_terminal_ANSI|
#. Observe that **LED 2** is off.
#. Press **Button 2** on the light bulb device.
   The **LED 2** turns on and the following messages appear on the console:

   .. code-block:: console

      I: Turn On Action has been initiated
      I: Turn On Action has been completed

#. Press **Button 2** again.
   The **LED 2** turns off and the following messages appear on the console:

   .. code-block:: console

      I: Turn Off Action has been initiated
      I: Turn Off Action has been completed

#. Press **Button 1** to initiate the factory reset of the device.

.. _matter_light_bulb_sample_light_switch_tests:

Testing communication with another device
-----------------------------------------

After building this sample and the :ref:`Matter light switch <matter_light_switch_sample>` sample and programming them to the development kits, complete the steps in the following sections to test communication between both devices.

Bind both devices
+++++++++++++++++

.. include:: ../light_switch/README.rst
   :start-after: matter_light_switch_sample_prepare_to_testing_start
   :end-before: matter_light_switch_sample_prepare_to_testing_end

Test connection
+++++++++++++++

.. include:: ../light_switch/README.rst
   :start-after: matter_light_switch_sample_testing_start
   :end-before: matter_light_switch_sample_testing_end

.. _matter_light_bulb_sample_remote_control:

Enabling remote control
=======================

Remote control allows you to control the Matter light bulb device from a Thread network.

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_remote_control_start
    :end-before: matter_door_lock_sample_remote_control_end

.. _matter_light_bulb_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_commissioning_start
    :end-before: matter_door_lock_sample_commissioning_end

Before starting the commissioning procedure, the device must be made discoverable over Bluetooth LE.
The device becomes discoverable automatically upon the device startup, but only for a predefined period of time (15 minutes by default).
If the Bluetooth LE advertising times out, press **Button 4** to re-enable it.

When you start the commissioning procedure, the controller must get the commissioning information from the Matter accessory device.
The data payload includes the device discriminator and setup PIN code.
It is encoded within a QR code printed to the UART console and can be shared using an NFC tag.

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
