.. _matter_lock_sample:
.. _chip_lock_sample:

Matter: Door lock
#################

.. contents::
   :local:
   :depth: 2

This door lock sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a door lock device with one basic bolt.
This device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power 802.15.4 Thread or Wi-Fi network.
Support for both Thread and Wi-Fi is mutually exclusive and depends on the hardware platform, so only one protocol can be supported for a specific lock device.
In case of Thread, this device works as a Thread :ref:`Sleepy End Device <thread_ot_device_types>`.
You can use this sample as a reference for creating your application.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

If you want to commission the lock device and :ref:`control it remotely <matter_lock_sample_network_mode>` through an IPv6 network, you also need a Matter controller device :ref:`configured on PC or mobile <ug_matter_configuring>`.
This requires additional hardware depending on the setup you choose.

.. note::
    |matter_gn_required_note|

IPv6 network support
====================

The development kits for this sample offer the following IPv6 network support for Matter:

* Matter over Thread is supported for ``nrf52840dk_nrf52840``, ``nrf5340dk_nrf5340_cpuapp``, and ``nrf21540dk_nrf52840``.
* Matter over Wi-Fi is supported for ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield attached or for ``nrf7002dk_nrf5340_cpuapp``.

Overview
********

The sample uses buttons for changing the lock and device states, and LEDs to show the state of these changes.
You can test it in the following ways:

* Standalone, using a single DK that runs the door lock application.
* Remotely over the Thread or the Wi-Fi protocol, which in either case requires more devices, including a Matter controller that you can configure either on a PC or a mobile device.

You can enable both methods after :ref:`building and running the sample <matter_lock_sample_remote_control>`.

.. _matter_lock_sample_network_mode:

Remote testing in a network
===========================

.. matter_door_lock_sample_remote_testing_start

By default, the Matter accessory device has IPv6 networking disabled.
You must pair it with the Matter controller over Bluetooth® LE to get the configuration from the controller to use the device within a Thread or Wi-Fi network.
You have to make the device discoverable manually (for security reasons).
The controller must get the commissioning information from the Matter accessory device and provision the device into the network.
For details, see the `Commissioning the device`_ section.

.. matter_door_lock_sample_remote_testing_end

Configuration
*************

|config|

Matter door lock build types
============================

.. matter_door_lock_sample_configuration_file_types_start

The sample uses different configuration files depending on the supported features.
Configuration files are provided for different build types and they are located in the application root directory.

The :file:`prj.conf` file represents a ``debug`` build type.
Other build types are covered by dedicated files with the build type added as a suffix to the ``prj`` part, as per the following list.
For example, the ``release`` build type file name is :file:`prj_release.conf`.
If a board has other configuration files, for example associated with partition layout or child image configuration, these follow the same pattern.

.. include:: /gs_modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

Before you start testing the application, you can select one of the build types supported by the sample.
This sample supports the following build types, depending on the selected board:

* ``debug`` -- Debug version of the application - can be used to enable additional features for verifying the application behavior, such as logs or command-line shell.
* ``release`` -- Release version of the application - can be used to enable only the necessary application functionalities to optimize its performance.
* ``no_dfu`` -- Debug version of the application without Device Firmware Upgrade feature support - can be used for the nRF52840 DK, nRF5340 DK, nRF7002 DK, and nRF21540 DK.

.. note::
    `Selecting a build type`_ is optional.
    The ``debug`` build type is used by default if no build type is explicitly selected.

.. matter_door_lock_sample_configuration_file_types_end

Device Firmware Upgrade support
===============================

.. matter_door_lock_sample_build_with_dfu_start

.. note::
   You can enable over-the-air Device Firmware Upgrade only on hardware platforms that have external flash memory.
   Currently only nRF52840 DK and nRF5340 DK support Device Firmware Upgrade feature.

The sample supports over-the-air (OTA) device firmware upgrade (DFU) using one of the two following protocols:

* Matter OTA update protocol that uses the Matter operational network for querying and downloading a new firmware image.
* Simple Management Protocol (SMP) over Bluetooth® LE.
  In this case, the DFU can be done either using a smartphone application or a PC command line tool.
  Note that this protocol is not part of the Matter specification.

In both cases, :ref:`MCUboot <mcuboot:mcuboot_wrapper>` secure bootloader is used to apply the new firmware image.

The DFU over Matter is enabled by default.
The following configuration arguments are available during the build process for configuring DFU:

* To configure the sample to support the DFU over Matter and SMP, use the ``-DCONFIG_CHIP_DFU_OVER_BT_SMP=y`` build flag.
* To configure the sample to disable the DFU and the secure bootloader, use the ``-DCONF_FILE=prj_no_dfu.conf`` build flag.

See :ref:`cmake_options` for instructions on how to add these options to your build.

When building on the command line, run the following command with *build_target* replaced with the build target name of the hardware platform you are using (see `Requirements`_), and *dfu_build_flag* replaced with the desired DFU build flag:

.. parsed-literal::
   :class: highlight

   west build -b *build_target* -- *dfu_build_flag*

For example:

.. code-block:: console

   west build -b nrf52840dk_nrf52840 -- -DCONFIG_CHIP_DFU_OVER_BT_SMP=y

.. matter_door_lock_sample_build_with_dfu_end

FEM support
===========

.. include:: /includes/sample_fem_support.txt

User interface
**************

.. matter_door_lock_sample_led1_start

LED 1:
    Shows the overall state of the device and its connectivity.
    The following states are possible:

    * Short Flash On (50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state and is waiting for a commissioning application to connect.
    * Rapid Even Flashing (100 ms on/100 ms off) - The device is in the unprovisioned state and a commissioning application is connected over Bluetooth LE.
    * Solid On - The device is fully provisioned.

.. matter_door_lock_sample_led1_end

LED 2:
    Shows the state of the lock.
    The following states are possible:

    * Solid On - The bolt is extended and the door is locked.
    * Off - The bolt is retracted and the door is unlocked.
    * Rapid Even Flashing (50 ms on/50 ms off during 2 s) - The simulated bolt is in motion from one position to another.

    Additionally, the LED starts blinking evenly (500 ms on/500 ms off) when the Identify command of the Identify cluster is received on the endpoint ``1``.
    The command's argument can be used to specify the duration of the effect.

.. matter_door_lock_sample_button1_start

Button 1:
    Depending on how long you press the button:

    * If pressed for less than three seconds, it initiates the SMP server (Security Manager Protocol).
      After that, the Direct Firmware Update (DFU) over Bluetooth Low Energy can be started.
      (See `Upgrading the device firmware`_.)
    * If pressed for more than three seconds, it initiates the factory reset of the device.
      Releasing the button within the 3-second window cancels the factory reset procedure.

.. matter_door_lock_sample_button1_end

Button 2:
    * On nRF52840 DK, nRF5340 DK, and nRF21540 DK: Changes the lock state to the opposite one.
    * On nRF7002 DK:

      * If pressed for less than three seconds, it changes the lock state to the opposite one.
      * If pressed for more than three seconds, it starts the NFC tag emulation, enables Bluetooth LE advertising for the predefined period of time (15 minutes by default), and makes the device discoverable over Bluetooth LE.

.. matter_door_lock_sample_button4_start

Button 4:
    * On nRF52840 DK, nRF5340 DK, and nRF21540 DK: Starts the NFC tag emulation, enables Bluetooth LE advertising for the predefined period of time (15 minutes by default), and makes the device discoverable over Bluetooth LE.
      This button is used during the :ref:`commissioning procedure <matter_lock_sample_remote_control_commissioning>`.

    * On nRF7002 DK: Not available.

.. matter_door_lock_sample_button4_end

.. matter_door_lock_sample_jlink_start

SEGGER J-Link USB port:
    Used for getting logs from the device or for communicating with it through the command-line interface.

.. matter_door_lock_sample_jlink_end

NFC port with antenna attached:
    Optionally used for obtaining the commissioning information from the Matter accessory device to start the :ref:`commissioning procedure <matter_lock_sample_remote_control>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/lock`

.. include:: /includes/build_and_run.txt

See `Configuration`_ for information about building the sample with the DFU support.

Selecting a build type
======================

Before you start testing the application, you can select one of the `Matter door lock build types`_, depending on your building method.

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

      File not found: ./ncs/nrf/samples/matter/lock/configuration/nrf52840dk_nrf52840/prj_shell.conf

Testing
=======

After building the sample and programming it to your development kit, complete the following steps to test its basic features:

#. |connect_kit|
#. |connect_terminal_ANSI|
#. Observe that **LED 2** is lit, which means that the door lock is closed.
#. Press **Button 2** to unlock the door.
   **LED 2** is blinking while the lock is opening.
   After approximately two seconds, **LED 2** turns off permanently.
   The following messages appear on the console:

   .. code-block:: console

      I: Unlock Action has been initiated
      I: Unlock Action has been completed

#. Press **Button 2** one more time to lock the door again.
   **LED 2** starts blinking and remains turned on.
   The following messages appear on the console:

   .. code-block:: console

      I: Lock Action has been initiated
      I: Lock Action has been completed

#. Press **Button 1** to initiate factory reset of the device.

The device reboots after all its settings are erased.

.. _matter_lock_sample_remote_control:

Enabling remote control
=======================

Remote control allows you to control the Matter door lock device from a Thread or a Wi-Fi network.

.. matter_door_lock_sample_remote_control_start

`Commissioning the device`_ allows you to set up a testing environment and remotely control the sample over a Matter-enabled Thread or Wi-Fi network.

.. matter_door_lock_sample_remote_control_end

.. _matter_lock_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. matter_door_lock_sample_commissioning_start

To commission the device, go to the :ref:`ug_matter_gs_testing` page and complete the steps for the Matter network environment and the Matter controller you want to use.
After choosing the configuration, the guide walks you through the following steps:

* Only if you are configuring Matter over Thread: Configure the Thread Border Router.
* Build and install the Matter controller.
* Commission the device.
* Send Matter commands that cover scenarios described in the `Testing`_ section.

If you are new to Matter, the recommended approach is to use :ref:`CHIP Tool for Linux or macOS <ug_matter_configuring_controller>`.

.. matter_door_lock_sample_commissioning_end

Upgrading the device firmware
=============================

To upgrade the device firmware, complete the steps listed for the selected method in the :doc:`matter:nrfconnect_examples_software_update` tutorial of the Matter documentation.

Dependencies
************

This sample uses the Matter library that includes the |NCS| platform integration layer:

* `Matter`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`nfc_uri`
* :ref:`lib_nfc_t2t`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
