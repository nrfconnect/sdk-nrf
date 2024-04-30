.. _matter_lock_sample:
.. _chip_lock_sample:

Matter: Door lock
#################

.. contents::
   :local:
   :depth: 2

This door lock sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a door lock device with one basic bolt.
You can use this sample as a reference for creating your application.

This device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power 802.15.4 Thread or Wi-Fi network.
Support for both Thread and Wi-Fi depends on the hardware platform.
The door lock sample can be built with support for one transport protocol, either Thread or Wi-Fi, or with support for :ref:`switching between Matter over Wi-Fi and Matter over Thread <matter_lock_sample_wifi_thread_switching>`, where the application activates either Thread or Wi-Fi on boot, depending on the runtime configuration.

Depending on the network you choose:

* In case of Thread, this device works as a Thread :ref:`Sleepy End Device <thread_ot_device_types>`.
* In case of Wi-Fi, this device works in the :ref:`Legacy Power Save mode <ug_nrf70_developing_powersave_dtim_unicast>`.
  This means that the device sleeps most of the time and wakes up on each Delivery Traffic Indication Message (DTIM) interval to poll for pending messages.

The same distinction applies in the :ref:`matter_lock_sample_wifi_thread_switching` scenario, depending on the active transport protocol.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

If you want to commission the lock device and :ref:`control it remotely <matter_lock_sample_network_mode>` through an IPv6 network, you also need a Matter controller device :ref:`configured on PC or mobile <ug_matter_configuring>`.
This requires additional hardware depending on the setup you choose.

.. note::
    |matter_gn_required_note|

If you want to enable and test :ref:`matter_lock_sample_ble_nus`, you also need a smartphone with either Android (Android 11 or newer) or iOS (iOS 16.1 or newer).

IPv6 network support
====================

The development kits for this sample offer the following IPv6 network support for Matter:

* Matter over Thread is supported for ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, and ``nrf21540dk/nrf52840``.
* Matter over Wi-Fi is supported for ``nrf5340dk/nrf5340/cpuapp`` with the ``nrf7002ek`` shield attached (2.4 GHz and 5 GHz), for ``nrf7002dk/nrf5340/cpuapp`` (2.4 GHz and 5 GHz), or for ``nrf7002dk/nrf5340/cpuapp/nrf7001`` (2.4 GHz only).
* :ref:`Switching between Matter over Thread and Matter over Wi-Fi <matter_lock_sample_wifi_thread_switching>` is supported for ``nrf5340dk/nrf5340/cpuapp`` with the ``nrf7002ek`` shield attached, using the ``thread_wifi_switched`` build type.

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
The controller must get the `Onboarding information`_ from the Matter accessory device and provision the device into the network.
For details, see the `Commissioning the device`_ section.

.. matter_door_lock_sample_remote_testing_end

.. _matter_lock_sample_wifi_thread_switching:

Thread and Wi-Fi switching
==========================

When built using the ``thread_wifi_switched`` build type and programmed to the nRF5340 DK with the nRF7002 EK shield attached, the door lock sample supports a feature that allows you to :ref:`switch between Matter over Thread and Matter over Wi-Fi <ug_matter_overview_architecture_integration_designs_switchable>` at runtime.
Due to Matter protocol limitations, a single Matter node can only use one transport protocol at a time.

.. matter_door_lock_sample_thread_wifi_switch_desc_start

The application is built with support for both Matter over Thread and Matter over Wi-Fi.
The device activates either Thread or Wi-Fi transport protocol on boot, based on a flag stored in the non-volatile memory on the device.
By default, Matter over Wi-Fi is activated.

You can trigger the switch from one transport protocol to the other using the **Button 3** on the nRF5340 DK.
This toggles the flag stored in the non-volatile memory, and then the device is factory reset and rebooted.
Because the flag is toggled, the factory reset does not switch the device back to the default transport protocol (Wi-Fi).
Instead, the factory reset and recommissioning to a Matter fabric allows the device to be provisioned with network credentials for the transport protocol that it was switched to, and to start operating in the selected network.

.. matter_door_lock_sample_thread_wifi_switch_desc_end

See `Matter door lock build types`_, `Selecting a build type`_, and :ref:`matter_lock_sample_switching_thread_wifi` for more information about how to configure and test this feature with this sample.

Wi-Fi firmware on external memory
---------------------------------

.. matter_door_lock_sample_nrf70_firmware_patch_start

You can program a portion of the application code related to the nRF70 Series' Wi-Fi firmware onto an external memory to free up space in the on-chip memory.
This option is available only when building for the nRF5340 DK with the nRF7002 EK shield attached.
To prepare an application to use this feature, you need to create additional MCUboot partitions.
To learn how to configure MCUboot partitions, see the :ref:`nrf70_fw_patch_update_adding_partitions` guide.
To enable this feature for Matter, set the :kconfig:option:`CONFIG_NRF_WIFI_FW_PATCH_DFU` Kconfig option to ``y`` for the application (in the application :file:`prj.conf`) and the :kconfig:option:`CONFIG_UPDATEABLE_IMAGE_NUMBER` Kconfig option to ``3`` for the MCUBoot child image (in its own :file:`prj.conf`).

.. matter_door_lock_sample_nrf70_firmware_patch_end

For example:

   .. code-block:: console

      west build -b nrf5340dk/nrf5340/cpuapp -p -- -DSHIELD=nrf7002ek -Dmultiprotocol_rpmsg_SHIELD=nrf7002ek_coex -DCONF_FILE=prj_thread_wifi_switched.conf -DCONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE=y -Dmcuboot_CONFIG_UPDATEABLE_IMAGE_NUMBER=3

.. _matter_lock_sample_ble_nus:

Matter Bluetooth LE with Nordic UART Service
============================================

.. matter_door_lock_sample_lock_nus_desc_start

The Matter implementation in the |NCS| lets you extend Bluetooth LE an additional Bluetooth LE service and use it in any Matter application, even when the device is not connected to a Matter network.
The Matter door lock sample provides a basic implementation of this feature, which integrates Bluetooth LE with the :ref:`Nordic UART Service (NUS) <nus_service_readme>`.
Using NUS, you can declare commands specific to a Matter sample and use them to control the device remotely through Bluetooth LE.
The connection between the device and the Bluetooth controller is secured and requires providing a passcode to pair the devices.

.. matter_door_lock_sample_lock_nus_desc_end

In the door lock sample, you can use the following commands with the Bluetooth LE with NUS:

* ``Lock`` - To lock the door of the connected device.
* ``Unlock`` - To unlock the door of the connected device.

If the device is already connected to the Matter network, the notification about changing the lock state will be send to the Bluetooth controller.

Currently, the door lock's Bluetooth LE service extension with NUS is only available for the nRF52840 and the nRF5340 DKs in the :ref:`Matter over Thread <ug_matter_gs_testing>` network variant.
However, you can use the Bluetooth LE service extension regardless of whether the device is connected to a Matter over Thread network or not.

See `Enabling Matter Bluetooth LE with Nordic UART Service`_ and `Testing door lock using Bluetooth LE with Nordic UART Service`_ for more information about how to configure and test this feature with this sample.

.. _matter_lock_sample_configuration:

Configuration
*************

|config|

.. _matter_lock_sample_configuration_build_types:

Matter door lock build types
============================

.. matter_door_lock_sample_configuration_file_types_start

The sample does not use a single :file:`prj.conf` file.
Configuration files are provided for different build types, and they are located in the sample root directory.
Before you start testing the application, you can select one of the build types supported by the application.

See :ref:`app_build_additions_build_types` and :ref:`cmake_options` for more information.

The sample supports the following build types:

.. list-table:: Sample build types
   :widths: auto
   :header-rows: 1

   * - Build type
     - File name
     - Supported board
     - Description
   * - Debug (default)
     - :file:`prj.conf`
     - All from `Requirements`_
     - Debug version of the application; can be used to enable additional features for verifying the application behavior, such as logs or command-line shell.
   * - Release
     - :file:`prj_release.conf`
     - All from `Requirements`_
     - Release version of the application; can be used to enable only the necessary application functionalities to optimize its performance.
   * - Switched Thread and Wi-Fi
     - :file:`prj_thread_wifi_switched.conf`
     - nRF5340 DK with the nRF7002 EK shield attached
     - Debug version of the application with the ability to :ref:`switch between Thread and Wi-Fi network support <matter_lock_sample_wifi_thread_switching>` in the field.

.. matter_door_lock_sample_configuration_file_types_end

.. _matter_lock_sample_configuration_dfu:

Device Firmware Upgrade support
===============================

.. matter_door_lock_sample_build_with_dfu_start

.. note::
   You can enable over-the-air Device Firmware Upgrade only on hardware platforms that have external flash memory.
   Currently only nRF52840 DK, nRF5340 DK and nRF7002 DK support Device Firmware Upgrade feature.

The sample supports over-the-air (OTA) device firmware upgrade (DFU) using one of the two following protocols:

* Matter OTA update protocol that uses the Matter operational network for querying and downloading a new firmware image.
* Simple Management Protocol (SMP) over Bluetooth® LE.
  In this case, the DFU can be done either using a smartphone application or a PC command line tool.
  Note that this protocol is not part of the Matter specification.

In both cases, :ref:`MCUboot <mcuboot:mcuboot_wrapper>` secure bootloader is used to apply the new firmware image.

The DFU over Matter is enabled by default.
The following configuration arguments are available during the build process for configuring DFU:

* To configure the sample to support the DFU over Matter and SMP, use the ``-DCONFIG_CHIP_DFU_OVER_BT_SMP=y`` build flag.

See :ref:`cmake_options` for instructions on how to add these options to your build.

When building on the command line, run the following command with *build_target* replaced with the build target name of the hardware platform you are using (see `Requirements`_), and *dfu_build_flag* replaced with the desired DFU build flag:

.. parsed-literal::
   :class: highlight

   west build -b *build_target* -- *dfu_build_flag*

For example:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 -- -DCONFIG_CHIP_DFU_OVER_BT_SMP=y

.. matter_door_lock_sample_build_with_dfu_end

.. _matter_lock_sample_configuration_fem:

FEM support
===========

.. include:: /includes/sample_fem_support.txt

.. _matter_lock_sample_configuration_nus:

Enabling Matter Bluetooth LE with Nordic UART Service
=====================================================

You can enable the :ref:`matter_lock_sample_ble_nus` feature by setting the :kconfig:option:`CONFIG_CHIP_NUS` Kconfig option to ``y``.

.. note::
   This sample supports one Bluetooth LE connection at a time.
   Matter commissioning, DFU, and NUS over Bluetooth LE must be run separately.

The door lock's Bluetooth LE service extension with NUS requires a secure connection with a smartphone, which is established using a security PIN code.
The PIN code is different depending on the build type:

* In the ``debug`` build type, the secure PIN code is generated randomly and printed in the log console in the following way:

  .. code-block:: console

     PROVIDE THE FOLLOWING CODE IN YOUR MOBILE APP: 165768

* In the ``release`` build type, the secure PIN is set to ``123456`` due to lack of a different way of showing it on nRF boards other than in the log console.

See `Testing door lock using Bluetooth LE with Nordic UART Service`_ for more information about how to test this feature.

Factory data support
====================

.. matter_door_lock_sample_factory_data_start

In this sample, the factory data support is enabled by default for all build types except for the target board nRF21540 DK.
This means that a new factory data set will be automatically generated when building for the target board.

To disable factory data support, set the following Kconfig options to ``n``:

   * :kconfig:option:`CONFIG_CHIP_FACTORY_DATA`
   * :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_BUILD`

To learn more about factory data, read the :doc:`matter:nrfconnect_factory_data_configuration` page in the Matter documentation.

.. matter_door_lock_sample_factory_data_end

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

    * If pressed for less than three seconds:

      * If the device is not provisioned to the Matter network, it initiates the SMP server (Simple Management Protocol) and Bluetooth LE advertising for Matter commissioning.
        After that, the Device Firmware Update (DFU) over Bluetooth Low Energy can be started.
        (See `Upgrading the device firmware`_.)
        Bluetooth LE advertising makes the device discoverable over Bluetooth LE for the predefined period of time (15 minutes by default).

      * If the device is already provisioned to the Matter network it re-enables the SMP server.
        After that, the DFU over Bluetooth Low Energy can be started.
        (See `Upgrading the device firmware`_.)

    * If pressed for more than three seconds, it initiates the factory reset of the device.
      Releasing the button within a 3-second window of the initiation cancels the factory reset procedure.

.. matter_door_lock_sample_button1_end

Button 2:
    * Changes the lock state to the opposite one.

Button 3:
    * On the nRF5340 DK when using the ``thread_wifi_switched`` build type: If pressed for more than ten seconds, it switches the Matter transport protocol from Thread or Wi-Fi to the other and factory resets the device.
    * On other platform or build type: Not available.

.. matter_door_lock_sample_jlink_start

SEGGER J-Link USB port:
    Used for getting logs from the device or for communicating with it through the command-line interface.

.. matter_door_lock_sample_jlink_end

NFC port with antenna attached:
    Optionally used for obtaining the `Onboarding information`_ from the Matter accessory device to start the :ref:`commissioning procedure <matter_lock_sample_remote_control>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/lock`

.. include:: /includes/build_and_run.txt

See `Configuration`_ for information about building the sample with the DFU support.

Selecting a build type
======================

Before you start testing the application, you can select one of the :ref:`matter_lock_sample_configuration_build_types`.
See :ref:`cmake_options` for information about how to select a build type.

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

#. Keep the **Button 1** pressed for more than six seconds to initiate factory reset of the device.

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

.. note::
   Before starting the commissioning to Matter procedure, ensure that there is no other Bluetooth LE connection established with the device.

.. matter_door_lock_sample_commissioning_start

To commission the device, go to the :ref:`ug_matter_gs_testing` page and complete the steps for the Matter network environment and the Matter controller you want to use.
After choosing the configuration, the guide walks you through the following steps:

* Only if you are configuring Matter over Thread: Configure the Thread Border Router.
* Build and install the Matter controller.
* Commission the device.
* Send Matter commands that cover scenarios described in the `Testing`_ section.

If you are new to Matter, the recommended approach is to use :ref:`CHIP Tool for Linux or macOS <ug_matter_configuring_controller>`.

.. matter_door_lock_sample_commissioning_end

Onboarding information
++++++++++++++++++++++

When you start the commissioning procedure, the controller must get the onboarding information from the Matter accessory device.
The onboarding information representation depends on your commissioner setup.

For this sample, you can use one of the following :ref:`onboarding information formats <ug_matter_network_topologies_commissioning_onboarding_formats>` to provide the commissioner with the data payload that includes the device discriminator and the setup PIN code:

  .. list-table:: Door lock sample onboarding information
     :header-rows: 1

     * - QR Code
       - QR Code Payload
       - Manual pairing code
     * - Scan the following QR code with the app for your ecosystem:

         .. figure:: ../../../doc/nrf/images/matter_qr_code_door_lock.png
            :width: 200px
            :alt: QR code for commissioning the light bulb device

       - MT:8IXS142C00KA0648G00
       - 34970112332

.. matter_door_lock_sample_onboarding_start

When the factory data support is enabled, the onboarding information will be stored in the build directory in the following files:

   * The :file:`factory_data.png` file includes the generated QR code.
   * The :file:`factory_data.txt` file includes the QR Code Payload and the manual pairing code.

.. matter_door_lock_sample_onboarding_end

|matter_cd_info_note_for_samples|

Upgrading the device firmware
=============================

To upgrade the device firmware, complete the steps listed for the selected method in the :doc:`matter:nrfconnect_examples_software_update` tutorial of the Matter documentation.

.. _matter_lock_sample_switching_thread_wifi:

Testing switching between Thread and Wi-Fi
==========================================

.. note::
   You can only test :ref:`matter_lock_sample_wifi_thread_switching` on the nRF5340 DK with the nRF7002 EK shield attached, using the ``thread_wifi_switched`` build type.

To test this feature, complete the following steps:

#. Build the door lock application using the ``thread_wifi_switched`` build type:

   .. code-block:: console

      west build -b nrf5340dk/nrf5340/cpuapp -- -DCONF_FILE=prj_thread_wifi_switched.conf -DSHIELD=nrf7002ek -Dmultiprotocol_rpmsg_SHIELD=nrf7002ek_coex

#. |connect_terminal_ANSI|
#. Program the application to the kit using the following command:

   .. code-block:: console

      west flash --erase

#. Wait until the device boots.
#. Find the following log message which indicates that Wi-Fi transport protocol is activated:

   .. code-block:: console

      D: 423 [DL]Starting Matter over Wi-Fi

#. Press **Button 3** for more than ten seconds to trigger switching from one transport protocol to the other.
#. Wait until the operation is finished and the device boots again.
#. Find the following log message which indicates that Thread transport protocol is activated:

   .. code-block:: console

      D: 423 [DL]Starting Matter over Thread

Testing door lock using Bluetooth LE with Nordic UART Service
=============================================================

To test the :ref:`matter_lock_sample_ble_nus` feature, complete the following steps:

#. Install `nRF Toolbox`_ on your Android (Android 11 or newer) or iOS smartphone (iOS 16.1 or newer).
#. Build the door lock application for Matter over Thread with the :kconfig:option:`CONFIG_CHIP_NUS` set to ``y``.
   For example, if you build from command line for the ``nrf52840dk/nrf52840``, use the following command:

   .. code-block:: console

      west build -b nrf52840dk/nrf52840 -- -DCONFIG_CHIP_NUS=y

#. Program the application to the kit using the following command:

   .. code-block:: console

      west flash --erase

#. If you built the sample with the ``debug`` build type, connect the board to an UART console to see the log entries from the device.
#. Open the nRF Toolbox application on your smartphone.
#. Select :guilabel:`Universal Asynchronous Receiver/Transmitter UART` from the list in the nRF Toolbox application.
#. Tap on :guilabel:`Connect`.
   The application connects to the devices connected through UART.
#. Select :guilabel:`MatterLock_NUS` from the list of available devices.
   The Bluetooth Pairing Request with an input field for passkey appears as a notification (Android) or on the screen (iOS).
#. Depending on the build type you are using:

   * For the ``release`` build type: Enter the passkey ``123456``.
   * For the ``debug`` build type, complete the following steps:

     a. Search the device's logs to find ``PROVIDE THE FOLLOWING CODE IN YOUR MOBILE APP:`` phrase.
     #. Read the randomly generated passkey from the console logs.
     #. Enter the passcode on your smartphone.

#. Wait for the Bluetooth LE connection to be established between the smartphone and the DK.
#. In the nRF Toolbox application, add the following macros:

   * ``Lock`` as the Command value type ``Text`` and any image.
   * ``Unlock`` as the Command value type ``Text`` and any image.

#. Tap on the generated macros and observe the **LED 2** on the DK.

The Bluetooth LE connection between a phone and the DK will be suspended when the commissioning to the Matter network is in progress or there is an active session of SMP DFU.

To read the current door lock state from the device, read the Bluetooth LE RX characteristic.
The new lock state is updated after changing the state from any of the following sources: NUS, buttons, Matter stack.

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
