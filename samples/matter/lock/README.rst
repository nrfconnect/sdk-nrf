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
The door lock sample can be configured to use only one transport protocol, for example Thread or Wi-Fi, or can use the :ref:`switchable Matter over Wi-Fi and Matter over Thread <matter_lock_sample_wifi_thread_switching>` architecture, where the application is able to load and run on boot an image with either the Thread or the Wi-Fi support.

Depending on the network you choose:

* In case of Thread, this device works as a Thread :ref:`Sleepy End Device <thread_ot_device_types>`.
* In case of Wi-Fi, this device works in the :ref:`Legacy Power Save mode <ug_nrf70_developing_powersave_dtim_unicast>`.
  This means that the device sleeps most of the time and wakes up on each Delivery Traffic Indication Message (DTIM) interval to poll for pending messages.

The same distinction applies in the :ref:`matter_lock_sample_wifi_thread_switching` scenario, depending on the network you have switched to.

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

* Matter over Thread is supported for ``nrf52840dk_nrf52840``, ``nrf5340dk_nrf5340_cpuapp``, and ``nrf21540dk_nrf52840``.
* Matter over Wi-Fi is supported for ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield attached or for ``nrf7002dk_nrf5340_cpuapp``.
* :ref:`Switching of Matter over Thread and Matter over Wi-Fi <matter_lock_sample_wifi_thread_switching>` is supported for ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield attached, using the ``thread_wifi_switched`` build type.

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

When built using the ``thread_wifi_switched`` build type and programmed to the nRF5340 DK with the nRF7002 EK shield attached, the door lock sample supports a feature that allows you to :ref:`dynamically switch between Matter over Thread and Matter over Wi-Fi <ug_matter_overview_architecture_integration_designs_switchable>` on the device boot.
Due to internal flash size limitations, only one transport protocol can be used at a time.

.. matter_door_lock_sample_thread_wifi_switch_desc_start

The application is built in two variants: the first one is the application working with Matter over Thread and the second one is the application working with Matter over Wi-Fi.
You can configure which transport is selected on the device boot as the default one.
Both application variants are programmed into separate partitions of the external flash.
The application runs from the internal flash memory, using one of the variants from the external flash.
You can trigger the switch from one variant to another using the **Button 3** on the nRF5340 DK.
The device is rebooted into the MCUboot bootloader, which replaces the current variant by swapping the application variant in the internal flash.

.. note::
   Because the external flash is used for both the DFU and the switching feature, this implementation has higher memory size requirements and you need an external flash with at least 6 MB of memory.

.. matter_door_lock_sample_thread_wifi_switch_desc_end

See `Matter door lock build types`_, `Selecting a build type`_, and :ref:`matter_lock_sample_switching_thread_wifi` for more information about how to configure and test this feature with this sample.
The Thread and Wi-Fi switching also supports :ref:`dedicated Device Firmware Upgrades <matter_lock_sample_switching_thread_wifi_dfu>`.

.. _matter_lock_sample_ble_nus:

Matter Bluetooth LE with Nordic UART Service
============================================

.. matter_door_lock_sample_lock_nus_desc_start

The Matter implementation in the |NCS| lets you extend Bluetooth LE an additional Bluetooth LE service and use it in any Matter application, even when the device is not connected to a Matter network.
The Matter door lock sample provides a basic implementation of this feature, which integrates Bluetooth LE with with :ref:`Nordic UART Service (NUS) <nus_service_readme>`.
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

The sample uses different configuration files depending on the supported features.
Configuration files are provided for different build types and they are located in the application root directory.

The :file:`prj.conf` file represents a ``debug`` build type.
Other build types are covered by dedicated files with the build type added as a suffix to the ``prj`` part, as per the following list.
For example, the ``release`` build type file name is :file:`prj_release.conf`.
If a board has other configuration files, for example associated with partition layout or child image configuration, these follow the same pattern.

.. include:: /getting_started/modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

Before you start testing the application, you can select one of the build types supported by the sample.
This sample supports the following build types, depending on the selected board:

* ``debug`` -- Debug version of the application - can be used to enable additional features for verifying the application behavior, such as logs or command-line shell.
* ``release`` -- Release version of the application - can be used to enable only the necessary application functionalities to optimize its performance.
* ``thread_wifi_switched`` -- Debug version of the application with the ability to :ref:`switch between Thread and Wi-Fi network support <matter_lock_sample_wifi_thread_switching>` in the field - can be used for the nRF5340 DK with the nRF7002 EK shield attached.
* ``no_dfu`` -- Debug version of the application without Device Firmware Upgrade feature support - can be used for the nRF52840 DK, nRF5340 DK, nRF7002 DK, and nRF21540 DK.

.. note::
    `Selecting a build type`_ is optional.
    The ``debug`` build type is used by default if no build type is explicitly selected.

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

.. _matter_lock_sample_configuration_fem:

FEM support
===========

.. include:: /includes/sample_fem_support.txt

.. _matter_lock_sample_configuration_nus:

Enabling Matter Bluetooth LE with Nordic UART Service
=====================================================

You can enable the :ref:`matter_lock_sample_ble_nus` feature by setting the :kconfig:option:`CONFIG_CHIP_NUS` Kconfig option to ``y``.

The door lock's Bluetooth LE service extension with NUS requires a secure connection with a smartphone, which is established using a security PIN code.
The PIN code is different depending on the build type:

* In the ``debug`` build type, the secure PIN code is generated randomly and printed in the log console in the following way:

  .. code-block:: console

     PROVIDE THE FOLLOWING CODE IN YOUR MOBILE APP: 165768

* In the ``release`` build type, the secure PIN is set to ``123456`` due to lack of a different way of showing it on nRF boards other than in the log console.

See `Testing door lock using Bluetooth LE with Nordic UART Service`_ for more information about how to test this feature.

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

Button 3:
    * On the nRF5340 DK when using the ``thread_wifi_switched`` build type: If pressed for more than ten seconds, it switches the running application from Thread or Wi-Fi to the other and factory resets the device.
    * On other platform or build type: Not available.

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
    Optionally used for obtaining the `Onboarding information`_ from the Matter accessory device to start the :ref:`commissioning procedure <matter_lock_sample_remote_control>`.

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

.. include:: /getting_started/modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

Selecting a build type from command line
----------------------------------------

.. include:: /getting_started/modifying.rst
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

Upgrading the device firmware
=============================

To upgrade the device firmware, complete the steps listed for the selected method in the :doc:`matter:nrfconnect_examples_software_update` tutorial of the Matter documentation.

.. _matter_lock_sample_switching_thread_wifi:

Testing switching between Thread and Wi-Fi
==========================================

.. note::
   You can only test :ref:`matter_lock_sample_wifi_thread_switching` on the nRF5340 DK with the nRF7002 EK shield attached, using the ``thread_wifi_switched`` build type.

To test this feature, complete the following steps:

#. Build the door lock application for Matter over Thread:

   .. code-block:: console

      west build -b nrf5340dk_nrf5340_cpuapp -- -DCONF_FILE=prj_thread_wifi_switched.conf -DSHIELD=nrf7002_ek -Dhci_rpmsg_SHIELD=nrf7002_ek_coex -DCONFIG_CHIP_WIFI=n

#. Program the application to the kit using the following command:

   .. code-block:: console

      west flash --erase

#. Erase the entire content of the external flash using the following command:

   .. code-block:: console

      nrfjprog --qspieraseall

#. Program the application to a partition of the external flash using the following command:

   .. code-block:: console

      west build -t flash-external

#. Build the door lock application for Matter over Wi-Fi:

   .. code-block:: console

      west build -b nrf5340dk_nrf5340_cpuapp -p always -- -DCONF_FILE=prj_thread_wifi_switched.conf -DSHIELD=nrf7002_ek -Dhci_rpmsg_SHIELD=nrf7002_ek_coex

#. Program the application to another partition of the external flash using the following command:

   .. code-block:: console

      west build -t flash-external

#. |connect_terminal_ANSI|
#. Press **Button 3** for more than ten seconds to trigger switching to the Matter over Wi-Fi variant of the application.
#. Observe logs showing the progress of the operation until the device shuts down.
#. Wait until the device boots again.
#. Find the following log message which indicates that the expected variant of the application is running:

   .. code-block:: console

      D: 823 [DL]WiFiManager has been initialized

.. _matter_lock_sample_switching_thread_wifi_dfu:

Device Firmware Upgrade over Matter for Thread/Wi-Fi switchable application
---------------------------------------------------------------------------

To upgrade the device firmware when using the ``thread_wifi_switched`` build type, complete the steps from the *Device Firmware Upgrade over Matter* section in the :doc:`matter:nrfconnect_examples_software_update` tutorial of the Matter documentation.

Because the application supports switching between Thread and Wi-Fi, you need to make sure that the Matter OTA image file served by the OTA Provider includes two application variants: for Matter over Thread and for Matter over Wi-Fi, respectively.

To make sure that both application variants are included in the OTA image file, use the dedicated the :file:`combine_ota_images.py` script.
The script takes the Matter OTA image files generated for both variants and combines them in one file.
It also assumes that both input images were created with the same *vendor_id*, *product_id*, *version*, *version_string*, *min_version*, *max_version*, and *release_notes*.

Complete the following steps to generate the Matter OTA combined image file:

#. Build the door lock application for Matter over Thread by running the following command:

   .. code-block:: console

      west build -b nrf5340dk_nrf5340_cpuapp -d build_thread -- -DCONF_FILE=prj_thread_wifi_switched.conf -DSHIELD=nrf7002_ek -Dhci_rpmsg_SHIELD=nrf7002_ek_coex -DCONFIG_CHIP_WIFI=n

   This command creates the *build_thread* directory, where the Matter over Thread application is stored.
#. Build the door lock application for Matter over Wi-Fi by running the following command:

   .. code-block:: console

      west build -b nrf5340dk_nrf5340_cpuapp -d build_wifi -- -DCONF_FILE=prj_thread_wifi_switched.conf -DSHIELD=nrf7002_ek -Dhci_rpmsg_SHIELD=nrf7002_ek_coex

   This command creates the *build_wifi* directory, where the Matter over Wi-Fi application is stored.
#. Combine Matter OTA image files generated for both variants by running the ``combine_ota_images.py`` script in the sample directory by running the following command (with ``<output_directory>`` changed to the directory name of your choice):

   .. code-block:: console

      src/combine_ota_images.py build_thread/zephyr/matter.ota build_wifi/zephyr/matter.ota <output_directory>/combined_matter.ota

.. note::
    Keep the order in which the files are passed to the script, given that the Thread variant image file must be passed in front of the Wi-Fi variant image.

Testing door lock using Bluetooth LE with Nordic UART Service
=============================================================

To test the :ref:`matter_lock_sample_ble_nus` feature, complete the following steps:

#. Install `nRF Toolbox`_ on your Android (Android 11 or newer) or iOS smartphone (iOS 16.1 or newer).
#. Build the door lock application for Matter over Thread with the :kconfig:option:`CONFIG_CHIP_NUS` set to ``y``.
   For example, if you build from command line for the ``nrf52840dk_nrf52840``, use the following command:

   .. code-block:: console

      west build -b nrf52840dk_nrf52840 -- -DCONFIG_CHIP_NUS=y

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
