.. _untitled_sample_project:

Untitled sample project
#######################

.. contents::
   :local:
   :depth: 2

This sample template demonstrates how to structure an application for development in |NCS|.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

This sample hosts a MCUmgr SMP server over Bluetooth which allows devices to connect, query it and upload new firmware which the included MCUboot bootloader will swap to (and allow reverting if there is a problem with the image).
An optional Kconfig fragment file is provided which will enable security manager support in Bluetooth for pairing/bonding support, and require an authenticated Bluetooth link to use MCUmgr functionality on the device.
In addition, the Memfault SDK is used as a module in the |NCS| to collect core dumps and reboot reasons from devices and send them through a Bluetooth gateway to the Memfault cloud.
See `Memfault terminology`_ for more details on the various Memfault concepts.

Core dumps
==========

You can trigger core dumps in this sample using the following methods:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

         * **Button 4** - Triggers a hardfault exception by division by zero.
         * ``mfl crash`` shell command - Triggers an assertion fail.

      When a fault occurs, it results in crashes that are captured by Memfault.
      After your development kit reboots and reconnects with the Bluetooth gateway, it sends core dump data to the Memfault cloud for further inspection and analysis.

   .. group-tab:: nRF54 DKs

      You can trigger core dumps in this sample using the following methods:

         * **Button 3** - Triggers a hardfault exception by division by zero.
         * ``mfl crash`` shell command - Triggers an assertion fail.

      When a fault occurs, it results in crashes that are captured by Memfault.
      After your development kit reboots and reconnects with the Bluetooth gateway, it sends core dump data to the Memfault cloud for further inspection and analysis.

Configuration
*************

|config|

The Memfault SDK allows configuring some of its options using Kconfig.
For the options not configurable using Kconfig, use the :file:`samples/bluetooth/peripheral_mds/memfault_config/memfault_platform_config.h` file.
See `Memfault SDK`_ for more information.

To send data to the Memfault cloud through a Bluetooth gateway, you must configure a project key using the :kconfig:option:`CONFIG_MEMFAULT_NCS_PROJECT_KEY` Kconfig option.
You can find your project key in the project settings at `Memfault Dashboards`_.
Use the :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID` Kconfig option to set a static device ID.
For this sample, the device ID is ``ncs-ble-testdevice`` by default.

See :ref:`configuring_kconfig` for information about the different ways you can set Kconfig options in the |NCS|.

Configuration options
=====================

.. _CONFIG_APP_SHOW_VERSION:

CONFIG_APP_SHOW_VERSION
    When enabled, this will show the version of the application at boot time, from the :file:`VERSION` file located in the application directory.

.. _CONFIG_APP_SHOW_BUILD_DATE:

CONFIG_APP_SHOW_BUILD_DATE
    When enabled, this will show the build date of the application at boot time.

Building and running
********************

.. |sample path| replace:: :file:`samples/untitled_sample_demo`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample

To upload MCUboot and the bundle of images to the device, program the sample by using the :ref:`flash command without erase <programming_params_no_erase>`.

Testing
=======

Before testing, ensure that your device is configured with the project key of your Memfault project.

|test_sample|

Testing with nRF Memfault mobile applications
---------------------------------------------

You can use the `nRF Memfault for Android`_ or the `nRF Memfault for iOS`_ applications for testing the :ref:`mds_readme`.
You can also use them for your custom applications using the Memfault Diagnostic Service.

1. |connect_terminal_ANSI|
#. Reset your development kit.
#. Observe that the sample starts.
#. On your mobile phone with access to the Internet, open the nRF Memfault application.
#. In the mobile application, tap the :guilabel:`Start` button.
#. Look for your device running the Memfault Diagnostic Service in the :guilabel:`Discovered devices` list.
#. Select your device from the list.
#. Tap :guilabel:`Connect` button to connect with your development kit.
#. Wait for the application to establish connection with your development kit.

   In the mobile application, observe that Memfault chunks are forwarded from your device to the Memfault Cloud.
#. Upload the symbol file generated from your build to your Memfault account so that the information from your application can be parsed.
   The symbol file is located in the build folder: :file:`peripheral_mds/build/peripheral_mds/zephyr/zephyr.elf`:

   a. Open `Memfault`_ in a web browser.
   #. Log in to your account and select the project you created earlier.
   #. Navigate to :guilabel:`Fleet` > :guilabel:`Devices` in the left side menu.
      You can see your newly connected device and the software version in the list.
   #. Select the software version number for your device and click :guilabel:`Upload` to upload the symbol file.

#. Return to the terminal and press Tab to confirm that the Memfault shell is working.
   The shell commands available are displayed.

   To learn about the Memfault shell commands, issue command ``mflt help``.
#. Use the buttons to trigger Memfault crashes, traces and metrics collection.

   See :ref:`peripheral_mds_user_interface` for details about button functions.
#. Explore the Memfault user interface to see the errors and metrics sent from your device.
#. When you have finished collecting diagnostic data, tap :guilabel:`Disconnect` to the close connection with your development kit.

   As the bond information is preserved, you can tap :guilabel:`Connect` again to immediately reconnect to the device.

Testing with MDS BLE gateway script
-----------------------------------

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_terminal_ANSI|
      #. Reset your development kit.
      #. Observe that the sample starts.
      #. Run the following command in the |NCS| root directory to install the MDS BLE gateway script dependencies:

         .. code-block:: console

            pip install --user -r scripts/memfault/requirements.txt

      #. Connect the nRF52 development kit to your PC that uses the :ref:`mds_ble_gateway_script`.
      #. Start the :file:`mds_ble_gateway.py` script with the correct parameters, for example:

         .. code-block:: console

            python3 mds_ble_gateway.py --snr 682900407 --com COM0

      #. Wait for the script to establish a connection with your development kit.
      #. Upon connection, data already collected by the `Memfault SDK`_ is forwarded to the cloud for further analysis.
         When connected, the new data is periodically transferred to the cloud with the interval configured in the :kconfig:option:`CONFIG_BT_MDS_DATA_POLL_INTERVAL` Kconfig option.
      #. On the terminal running the script, you can observe the Memfault chunk counter:

         .. code-block:: console

            Sending..
            Forwarded 2 Memfault Chunks

      #. Upload the symbol file generated from your build to your Memfault account so that the information from your application can be parsed.
         The symbol file is located in the build folder: :file:`peripheral_mds/build/peripheral_mds/zephyr/zephyr.elf`:

         a. Open `Memfault`_ in a web browser.
         #. Log in to your account and select the project you created earlier.
         #. Navigate to :guilabel:`Fleet` > :guilabel:`Devices` in the left side menu.
            You can see your newly connected device and the software version in the list.
         #. Select the software version number for your device and click :guilabel:`Upload` to upload the symbol file.

      #. Return to the terminal and press the Tab button on your keyboard to confirm that the Memfault shell is working.
         The shell commands available are displayed.

         To learn about the Memfault shell commands, issue command ``mflt help``
      #. Use the buttons to trigger Memfault crashes, traces and metrics collection.

         See :ref:`peripheral_mds_user_interface` for details about button functions.
      #. Explore the Memfault user interface to see the errors and metrics sent from your device.

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_ANSI|
      #. Reset your development kit.
      #. Observe that the sample starts.
      #. Run the following command in the |NCS| root directory to install the MDS BLE gateway script dependencies:

         .. code-block:: console

            pip install --user -r scripts/memfault/requirements.txt

      #. Connect the nRF52 development kit to your PC that uses the :ref:`mds_ble_gateway_script`.
      #. Start the :file:`mds_ble_gateway.py` script with the correct parameters, for example:

         .. code-block:: console

            python3 mds_ble_gateway.py --snr 682900407 --com COM0

      #. Wait for the script to establish a connection with your development kit.
      #. Upon connection, data already collected by the `Memfault SDK`_ is forwarded to the cloud for further analysis.
         When connected, the new data is periodically transferred to the cloud with the interval configured in the :kconfig:option:`CONFIG_BT_MDS_DATA_POLL_INTERVAL` Kconfig option.
      #. On the terminal running the script, you can observe the Memfault chunk counter:

         .. code-block:: console

            Sending..
            Forwarded 2 Memfault Chunks

      #. Upload the symbol file generated from your build to your Memfault account so that the information from your application can be parsed.
         The symbol file is located in the build folder: :file:`peripheral_mds/build/peripheral_mds/zephyr/zephyr.elf`:

         a. Open `Memfault`_ in a web browser.
         #. Log in to your account and select the project you created earlier.
         #. Navigate to :guilabel:`Fleet` > :guilabel:`Devices` in the left side menu.
            You can see your newly connected device and the software version in the list.
         #. Select the software version number for your device and click :guilabel:`Upload` to upload the symbol file.

      #. Return to the terminal and press the Tab button on your keyboard to confirm that the Memfault shell is working.
         The shell commands available are displayed.

         To learn about the Memfault shell commands, issue command ``mflt help``
      #. Use the buttons to trigger Memfault crashes, traces and metrics collection.

         See :ref:`peripheral_mds_user_interface` for details about button functions.
      #. Explore the Memfault user interface to see the errors and metrics sent from your device.

Testing with Memfault WebBluetooth Client
-----------------------------------------

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      .. note::
         The Web Bluetooth API used by the `Memfault WebBluetooth Client`_ is an experimental feature.
         The functionality depends on your browser and computer OS compatibility.

      1. |connect_terminal_ANSI|
      #. Reset your development kit.
      #. Observe that the sample starts.
      #. Open a recent version of the `Google Chrome browser`_.
      #. Run the `Memfault WebBluetooth Client`_ script to forward Memfault diagnostic data to the cloud.
         For more details, see the `Memfault WebBluetooth Client source code`_.
      #. Make sure that your development kit is advertising.
      #. In the browser, click the :guilabel:`Connect` button and select your device from the list.
      #. Upon connection, data already collected by the `Memfault SDK`_ is forwarded to the cloud for further the analysis.
         When connected, the new data is periodically flushed to the cloud with the interval configured by the Kconfig option :kconfig:option:`CONFIG_BT_MDS_DATA_POLL_INTERVAL`.
      #. Upload the symbol file generated from your build to your Memfault account so that the information from your application can be parsed.
         The :file:`zephyr.elf` symbol file is located in the build folder :file:`peripheral_mds/build/peripheral_mds/zephyr`.

         a. In a web browser, navigate to `Memfault`_.
         #. Log in to your account and select the project you created earlier.
         #. Navigate to :guilabel:`Fleet` > :guilabel:`Devices` in the left side menu.
            You can see your newly connected device and the software version in the list.
         #. Select the software version number for your device and click :guilabel:`Upload` to upload the symbol file.

      #. Return to the terminal and press the Tab button on your keyboard to confirm that the Memfault shell is working.
         The shell commands available are displayed.

         To learn about the Memfault shell commands, issue command ``mflt help``.
      #. Use the buttons to trigger Memfault crashes, traces and metrics collection.
         See :ref:`peripheral_mds_user_interface` for details about button functions.
      #. Explore the Memfault user interface to see the errors and metrics sent from your device.

   .. group-tab:: nRF54 DKs

      .. note::
         The Web Bluetooth API used by the `Memfault WebBluetooth Client`_ is an experimental feature.
         The functionality depends on your browser and computer OS compatibility.

      1. |connect_terminal_ANSI|
      #. Reset your development kit.
      #. Observe that the sample starts.
      #. Open a recent version of the `Google Chrome browser`_.
      #. Run the `Memfault WebBluetooth Client`_ script to forward Memfault diagnostic data to the cloud.
         For more details, see the `Memfault WebBluetooth Client source code`_.
      #. Make sure that your development kit is advertising.
      #. In the browser, click the :guilabel:`Connect` button and select your device from the list.
      #. Upon connection, data already collected by the `Memfault SDK`_ is forwarded to the cloud for further the analysis.
         When connected, the new data is periodically flushed to the cloud with the interval configured by the Kconfig option :kconfig:option:`CONFIG_BT_MDS_DATA_POLL_INTERVAL`.
      #. Upload the symbol file generated from your build to your Memfault account so that the information from your application can be parsed.
         The :file:`zephyr.elf` symbol file is located in the build folder :file:`peripheral_mds/build/peripheral_mds/zephyr`.

         a. In a web browser, navigate to `Memfault`_.
         #. Log in to your account and select the project you created earlier.
         #. Navigate to :guilabel:`Fleet` > :guilabel:`Devices` in the left side menu.
            You can see your newly connected device and the software version in the list.
         #. Select the software version number for your device and click :guilabel:`Upload` to upload the symbol file.

      #. Return to the terminal and press the Tab button on your keyboard to confirm that the Memfault shell is working.
         The shell commands available are displayed.

         To learn about the Memfault shell commands, issue command ``mflt help``.
      #. Use the buttons to trigger Memfault crashes, traces and metrics collection.
         See :ref:`peripheral_mds_user_interface` for details about button functions.
      #. Explore the Memfault user interface to see the errors and metrics sent from your device.

Testing over Bluetooth LE
=========================

The testing scenario uses the FOTA over Bluetooth LE method to load the firmware update to the device.
For the testing scenario to work, make sure you meet the following requirements:

* Your update image differs from the base image so that the update can be applied.
* You changed the firmware version by setting :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` to ``"2.0.0"``.
* You are familiar with the FOTA over Bluetooth LE method (see :ref:`the guide for the nRF54L15 DK <ug_nrf54l_developing_ble_fota_steps>`).
* You have the `nRF Connect Device Manager`_ mobile app installed on your smartphone to update your  device with the new firmware over Bluetooth LE.

Meeting these requirements requires rebuilding the sample.

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Start the `nRF Connect Device Manager`_ mobile app.
#. Follow the testing steps for the FOTA over Bluetooth LE.
   See the following sections of the documentation:

   * :ref:`Testing steps for FOTA over Bluetooth LE with nRF54L15 <ug_nrf54l_developing_ble_fota_steps_testing>`

#. Once the firmware update has been loaded, check the UART output.
   See the following Sample output section.
#. Reboot the device.
   The firmware update will be applied to the device and it will boot into it.


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`mds_readme`
* :ref:`mod_memfault`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:mcu_mgr`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
