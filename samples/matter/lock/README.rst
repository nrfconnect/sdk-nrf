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

* Matter over Thread is supported for ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf21540dk/nrf52840``, and ``nrf54h20dk/nrf54h20/cpuapp``.
* Matter over Wi-Fi is supported for ``nrf5340dk/nrf5340/cpuapp`` or ``nrf54h20dk/nrf54h20/cpuapp`` with the ``nrf7002ek`` shield attached, for ``nrf7002dk/nrf5340/cpuapp`` (2.4 GHz and 5 GHz), or for ``nrf7002dk/nrf5340/cpuapp/nrf7001`` (2.4 GHz only).
* :ref:`Switching between Matter over Thread and Matter over Wi-Fi <matter_lock_sample_wifi_thread_switching>` is supported for ``nrf5340dk/nrf5340/cpuapp`` with the ``nrf7002ek`` shield attached, using the :ref:`switched Thread and Wi-Fi configuration <matter_lock_sample_custom_configs>`.

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

Door lock credentials
=====================

By default, the application supports only PIN code credentials, but it is possible to implement support for other door lock credential types by using the ``AccessManager`` module.
The credentials can be used to control remote access to the bolt lock.
The PIN code assigned by the Matter controller is stored persistently, which means that it can survive a device reboot.
Depending on the IPv6 network technology in use, the following storage backends are supported by default to store the PIN code credential:

* Matter over Thread - secure storage backend (:kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND` Kconfig option enabled by default).
* Matter over Wi-Fi - non-secure storage backend (:kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_SETTINGS_STORAGE_BACKEND` Kconfig option enabled by default).

You can learn more about the |NCS| Matter persistent storage module and its configuration in the :ref:`ug_matter_persistent_storage` section of the :ref:`ug_matter_device_advanced_kconfigs` documentation.

The application supports multiple door lock users and PIN code credentials.
The following Kconfig options control the limits of the users and credentials that can be added to the door lock:

* :kconfig:option:`CONFIG_LOCK_MAX_NUM_USERS` - Maximum number of users supported by the door lock.
* :kconfig:option:`CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_USER` - Maximum number of credentials that can be assigned to one user.
* :kconfig:option:`CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_TYPE` - Maximum number of credentials in total.
* :kconfig:option:`CONFIG_LOCK_MAX_CREDENTIAL_LENGTH` - Maximum length of a single credential in bytes.

.. _matter_lock_sample_wifi_thread_switching:

Thread and Wi-Fi switching
==========================

When built using the :ref:`switched Thread and Wi-Fi configuration <matter_lock_sample_custom_configs>` and programmed to the nRF5340 DK with the nRF7002 EK shield attached, the door lock sample supports a feature that allows you to :ref:`switch between Matter over Thread and Matter over Wi-Fi <ug_matter_overview_architecture_integration_designs_switchable>` at runtime.
Due to Matter protocol limitations, a single Matter node can only use one transport protocol at a time.

.. matter_door_lock_sample_thread_wifi_switch_desc_start

The application is built with support for both Matter over Thread and Matter over Wi-Fi.
The device activates either Thread or Wi-Fi transport protocol on boot, based on a flag stored in the non-volatile memory on the device.
By default, Matter over Wi-Fi is activated.

You can trigger the switch from one transport protocol to the other using **Button 3** on the nRF5340 DK.
This toggles the flag stored in the non-volatile memory, and then the device is factory reset and rebooted.
Because the flag is toggled, the factory reset does not switch the device back to the default transport protocol (Wi-Fi).
Instead, the factory reset and recommissioning to a Matter fabric allows the device to be provisioned with network credentials for the transport protocol that it was switched to, and to start operating in the selected network.

.. matter_door_lock_sample_thread_wifi_switch_desc_end

See :ref:`matter_lock_sample_custom_configs` and :ref:`matter_lock_sample_switching_thread_wifi` for more information about how to configure and test this feature with this sample.

Wi-Fi firmware on external memory
---------------------------------

.. matter_door_lock_sample_nrf70_firmware_patch_start

You can program a portion of the application code related to the nRF70 Series' Wi-Fi firmware onto an external memory to free up space in the on-chip memory.
This option is available only when building for the nRF5340 DK with the nRF7002 EK shield attached.
To prepare an application to use this feature, you need to create additional MCUboot partitions.
To learn how to configure MCUboot partitions, see the :ref:`nrf70_fw_patch_update_adding_partitions` guide.
To enable this feature for Matter, set the ``SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE``, ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH`` Kconfig options to ``y``, and set the ``SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES`` Kconfig option to ``3``.

.. matter_door_lock_sample_nrf70_firmware_patch_end

For example:

   .. code-block:: console

      west build -b nrf5340dk/nrf5340/cpuapp -p -- -Dlock_SHIELD=nrf7002ek -Dipc_radio_SHIELD=nrf7002ek_coex -DFILE_SUFFIX=thread_wifi_switched -DSB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE=y -DSB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES=3 -DCONFIG_CHIP_DFU_OVER_BT_SMP=y -DSB_CONFIG_WIFI_NRF70=y -DSB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH=y

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

.. _matter_lock_scheduled_timed_access:

Scheduled timed access
======================

The scheduled timed access feature is an optional Matter lock feature that can be applicable to all available lock users.
You can use the scheduled timed access feature to allow guest users of the home to access the lock at the specific scheduled times.
To use this feature, you need to create at least one user on the lock device, and assign credentials.
For more information about setting user credentials, see the Saving users and credentials on door lock devices section of the :doc:`matter:chip_tool_guide` page in the :doc:`Matter documentation set <matter:index>`, and the :ref:`matter_lock_sample_remote_access_with_pin` section of this sample.

You can schedule the following types of timed access:

   - ``Week-day`` - Restricts access to a specified time window on certain days of the week for a specific user.
     This schedule grants repeated access each week.
     When the schedule is cleared, the user is granted unrestricted access.

   - ``Year-day`` - Restricts access to a specified time window on a specific date window.
     This schedule grants access only once, and does not repeat.
     When the schedule is cleared, the user is granted unrestricted access.

   - ``Holiday`` - Sets up a holiday operating mode in the lock device.
     You can choose one of the following operating modes:

     - ``Normal`` - The lock operates normally.
     - ``Vacation`` - Only remote operations are enabled.
     - ``Privacy`` - All external interactions with the lock are disabled.
       Can only be used if the lock is in the locked state.
       Manually unlocking the lock changes the mode to ``Normal``.
     - ``NoRemoteLockUnlock`` - All remote operations with the lock are disabled.
     - ``Passage`` - The lock can be operated without providing a PIN.
       This option can be used, for example, for employees during working hours.

To enable the scheduled timed access feature, use the ``schedules`` snippet.
See the :ref:`matter_lock_snippets` section of this sample to learn how to enable the snippet.

See the :ref:`matter_lock_sample_schedule_testing` section of this sample for more information about testing the scheduled timed access feature.

.. _matter_lock_sample_configuration:

Configuration
*************

|config|

.. _matter_lock_sample_custom_configs:

Matter door lock custom configurations
======================================

.. matter_door_lock_sample_configuration_file_types_start

.. include:: /includes/sample_custom_config_intro.txt

The sample supports the following configurations:

.. list-table:: Sample configurations
   :widths: auto
   :header-rows: 1

   * - Configuration
     - File name
     - :makevar:`FILE_SUFFIX`
     - Supported board
     - Description
   * - Debug (default)
     - :file:`prj.conf`
     - No suffix
     - All from `Requirements`_
     - Debug version of the application.

       Enables additional features for verifying the application behavior, such as logs.
   * - Release
     - :file:`prj_release.conf`
     - ``release``
     - All from `Requirements`_
     - Release version of the application.

       Enables only the necessary application functionalities to optimize its performance.
   * - Switched Thread and Wi-Fi
     - :file:`prj_thread_wifi_switched.conf`
     - ``thread_wifi_switched``
     - nRF5340 DK with the nRF7002 EK shield attached
     - Debug version of the application with the ability to :ref:`switch between Thread and Wi-Fi network support <matter_lock_sample_wifi_thread_switching>` in the field.

.. matter_door_lock_sample_configuration_file_types_end

Matter lock with Trusted Firmware-M
===================================

.. include:: ../template/README.rst
    :start-after: matter_template_build_with_tfm_start
    :end-before: matter_template_build_with_tfm_end

.. _matter_lock_snippets:

Snippets
========

.. |snippet| replace:: :makevar:`lock_SNIPPET`

.. include:: /includes/sample_snippets.txt

The following snippet is available:

* ``schedules`` - Enables schedule timed access features.

  .. |snippet_zap_file| replace:: :file:`snippets/schedules/lock.zap`
  .. |snippet_dir| replace:: :file:`snippets/schedules`

  .. include:: /includes/matter_snippets_note.txt

.. _matter_lock_sample_configuration_dfu:

Device Firmware Upgrade support
===============================

.. matter_door_lock_sample_build_with_dfu_start

.. note::
   You can enable over-the-air Device Firmware Upgrade only on hardware platforms that have external flash memory.
   Currently only nRF52840 DK, nRF5340 DK, nRF7002 DK and nRF54L15 DK support Device Firmware Upgrade feature.

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

When building on the command line, run the following command with *board_target* replaced with the board target name of the hardware platform you are using (see `Requirements`_), and *dfu_build_flag* replaced with the desired DFU build flag:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- *dfu_build_flag*

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
The PIN code is different depending on the :ref:`configuration <matter_lock_sample_custom_configs>` the sample was built with:

* In the debug configuration, the secure PIN code is generated randomly and printed in the log console in the following way:

  .. code-block:: console

     PROVIDE THE FOLLOWING CODE IN YOUR MOBILE APP: 165768

* In the release configuration, the secure PIN is set to ``123456`` due to lack of a different way of showing it on nRF boards other than in the log console.

See `Testing door lock using Bluetooth LE with Nordic UART Service`_ for more information about how to test this feature.

Factory data support
====================

.. matter_door_lock_sample_factory_data_start

In this sample, the factory data support is enabled by default for all configurations except for the target board nRF21540 DK.
This means that a new factory data set will be automatically generated when building for the target board.

To disable factory data support, set the following Kconfig options to ``n``:

   * :kconfig:option:`CONFIG_CHIP_FACTORY_DATA`
   * ``SB_CONFIG_MATTER_FACTORY_DATA_GENERATE``

To learn more about factory data, read the :doc:`matter:nrfconnect_factory_data_configuration` page in the Matter documentation.

.. matter_door_lock_sample_factory_data_end

User interface
**************

.. tabs::

   .. group-tab:: nRF52, nRF53, nRF21 and nRF70 DKs

      LED 1:
         .. include:: /includes/matter_sample_state_led.txt

      LED 2:
         Shows the state of the lock.
         The following states are possible:

         * Solid On - The bolt is extended and the door is locked.
         * Off - The bolt is retracted and the door is unlocked.
         * Rapid Even Flashing (50 ms on/50 ms off during 2 s) - The simulated bolt is in motion from one position to another.

         Additionally, the LED starts blinking evenly (500 ms on/500 ms off) when the Identify command of the Identify cluster is received on the endpoint ``1``.
         The command's argument can be used to specify the duration of the effect.

      Button 1:
         .. include:: /includes/matter_sample_button.txt

      Button 2:
         * Changes the lock state to the opposite one.

      Button 3:
         * On the nRF5340 DK when using the :ref:`switched Thread and Wi-Fi configuration <matter_lock_sample_custom_configs>`: If pressed for more than ten seconds, it switches the Matter transport protocol from Thread or Wi-Fi to the other and factory resets the device.
         * On other platform or configuration: Not available.

      .. include:: /includes/matter_segger_usb.txt

      NFC port with antenna attached:
         Optionally used for obtaining the `Onboarding information`_ from the Matter accessory device to start the :ref:`commissioning procedure <matter_lock_sample_remote_control>`.

   .. group-tab:: nRF54 DKs

      LED 0:
         .. include:: /includes/matter_sample_state_led.txt

      LED 1:
         Shows the state of the lock.
         The following states are possible:

         * Solid On - The bolt is extended and the door is locked.
         * Off - The bolt is retracted and the door is unlocked.
         * Rapid Even Flashing (50 ms on/50 ms off during 2 s) - The simulated bolt is in motion from one position to another.

         Additionally, the LED starts blinking evenly (500 ms on/500 ms off) when the Identify command of the Identify cluster is received on the endpoint ``1``.
         The command's argument can be used to specify the duration of the effect.

      Button 0:
         .. include:: /includes/matter_sample_button.txt

      Button 1:
         * Changes the lock state to the opposite one.

      .. include:: /includes/matter_segger_usb.txt

      NFC port with antenna attached:
         Optionally used for obtaining the `Onboarding information`_ from the Matter accessory device to start the :ref:`commissioning procedure <matter_lock_sample_remote_control>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/lock`

.. include:: /includes/build_and_run.txt

See `Configuration`_ for information about building the sample with the DFU support.

.. include:: ../template/README.rst
    :start-after: matter_template_build_wifi_nrf54h20_start
    :end-before: matter_template_build_wifi_nrf54h20_end

.. code-block:: console

    west build -b nrf54h20dk/nrf54h20/cpuapp -p -- -DSB_CONFIG_WIFI_NRF70=y -DCONFIG_CHIP_WIFI=y -Dlock_SHIELD=nrf700x_nrf54h20dk

Selecting a configuration
=========================

Before you start testing the application, you can select one of the :ref:`matter_lock_sample_custom_configs`.
See :ref:`app_build_file_suffixes` and :ref:`cmake_options` for more information how to select a configuration.

Testing
=======

After building the sample and programming it to your development kit, complete the following steps to test its basic features:

.. tabs::

   .. group-tab:: nRF52, nRF53, nRF21 and nRF70 DKs

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

   .. group-tab:: nRF54 DKs

      #. |connect_kit|
      #. |connect_terminal_ANSI|
      #. Observe that **LED 1** is lit, which means that the door lock is closed.
      #. Press **Button 1** to unlock the door.
         **LED 1** is blinking while the lock is opening.

         After approximately two seconds, **LED 1** turns off permanently.
         The following messages appear on the console:

         .. code-block:: console

            I: Unlock Action has been initiated
            I: Unlock Action has been completed

      #. Press **Button 1** one more time to lock the door again.
         **LED 1** starts blinking and remains turned on.
         The following messages appear on the console:

         .. code-block:: console

            I: Lock Action has been initiated
            I: Lock Action has been completed

      #. Keep the **Button 0** pressed for more than six seconds to initiate factory reset of the device.

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

.. _matter_lock_sample_remote_access_with_pin:

Testing remote access with PIN code credential
==============================================

.. note::
   You can test the PIN code credential support with any Matter compatible controller.
   The following steps use the CHIP Tool controller as an example.
   For more information about setting user credentials, see the Saving users and credentials on door lock devices section of the :doc:`matter:chip_tool_guide` page in the :doc:`Matter documentation set <matter:index>`.

After building the sample and programming it to your development kit, complete the following steps to test remote access with PIN code credential:

1. |connect_kit|
#. |connect_terminal_ANSI|
#. Wait until the device boots.
#. Commission an accessory with node ID equal to 10 to the Matter network by following the steps described in the `Commissioning the device`_ section.
#. Make the door lock require a PIN code for remote operations:

   .. code-block:: console

      ./chip-tool doorlock write require-pinfor-remote-operation 1 10 1 --timedInteractionTimeoutMs 5000

#. Add the example ``Home`` door lock user:

   .. code-block:: console

      ./chip-tool doorlock set-user 0 2 Home 123 1 0 0 10 1 --timedInteractionTimeoutMs 5000

   This command creates a ``Home`` user with a unique ID of ``123`` and an index of ``2``.
   The new user's status is set to ``1``, and both its type and credential rule to ``0``.
   The user is assigned to the door lock cluster residing on endpoint ``1`` of the node with ID ``10``.

#. Add the example ``12345678`` PIN code credential to the ``Home`` user:

   .. code-block:: console

      ./chip-tool doorlock set-credential 0 '{"credentialType": 1, "credentialIndex": 1}' 12345678 2 null null 10 1 --timedInteractionTimeoutMs 5000

#. Unlock the door lock with the given PIN code:

   .. code-block:: console

      ./chip-tool doorlock unlock-door 10 1 --PINCode 12345678 --timedInteractionTimeoutMs 5000

#. Reboot the device.
#. Wait until the device it rebooted and attached back to the Matter network.
#. Unlock the door lock with the PIN code provided before the reboot:

   .. code-block:: console

      ./chip-tool doorlock unlock-door 10 1 --PINCode 12345678 --timedInteractionTimeoutMs 5000

.. note::
   Accessing the door lock remotely without a valid PIN code credential will fail.

.. _matter_lock_sample_schedule_testing:

Testing scheduled timed access
==============================

.. note::
   You can test :ref:`matter_lock_scheduled_timed_access` using any Matter compatible controller.
   The following steps use the CHIP Tool controller as an example.

   All scheduled timed access entries are saved to non-volatile memory and loaded automatically after device reboot.
   Adding a single schedule for a user contributes to the settings partition memory occupancy increase.

After building the sample with the feature enabled and programming it to your development kit, complete the following steps to test scheduled timed access:

1. |connect_kit|
#. |connect_terminal_ANSI|
#. Wait until the device boots.
#. Commission an accessory with node ID equal to 10 to the Matter network by following the steps described in the `Commissioning the device`_ section.
#. Add the example ``Home`` door lock user:

   .. code-block:: console

      ./chip-tool doorlock set-user 0 2 Home 123 1 0 0 10 1 --timedInteractionTimeoutMs 5000

   This command creates a ``Home`` user with a unique ID of ``123`` and an index of ``2``.
   The new user's status is set to ``1``, and both its type and credential rule to ``0``.
   The user is assigned to the door lock cluster residing on endpoint ``1`` of the node with ID ``10``.

#. Set the example ``Week-day`` schedule using the following command:

   .. parsed-literal::
      :class: highlight

      ./chip-tool doorlock set-week-day-schedule *weekday-index* *user-index* *days-mask* *start-hour* *start-minute* *end-hour* *end-minute* *destination-id* *endpoint-id*

   * *weekday-index* is the index of the new schedule, starting from ``1``.
     The maximum value is defined by the :kconfig:option:`CONFIG_LOCK_MAX_WEEKDAY_SCHEDULES_PER_USER` Kconfig option.
   * *user-index* is the user index defined for the user created in the previous step.
   * *days-mask* is a bitmap of numbers of the week days starting from ``0`` as a Sunday and finishing at ``6`` as a Saturday.
     For example, to assign this schedule to Tuesday, Thursday and Saturday you need to provide ``84`` because it is equivalent to the ``01010100`` bitmap.
   * *start-hour* is the starting hour for the week day schedule.
   * *start-minute* is the starting minute for the week day schedule.
   * *end-hour* is the ending hour for the week day schedule.
   * *end-minute* is the ending hour for the week day schedule.
   * *destination-id* is the device node ID.
   * *endpoint-id* is the Matter door lock endpoint, in this sample assigned to ``1``.

   For example, use the following command to set a ``Week-day`` schedule with index ``1`` for Tuesday, Thursday and Saturday to start at 7:30 AM and finish at 10:30 AM, dedicated for user with ID ``2``:

   .. code-block:: console

      ./chip-tool doorlock set-week-day-schedule 1 2 84 7 30 10 30 1 1


#. Set the example ``Year-day`` schedule using the following command:

   .. parsed-literal::
      :class: highlight

      ./chip-tool doorlock set-year-day-schedule *yearday-index* *user-index* *localtime-start* *localtime-end* *destination-id* *endpoint-id*


   * *yearday-index* is the index of the new schedule, starting from ``1``.
     The maximum value is defined by the :kconfig:option:`CONFIG_LOCK_MAX_YEARDAY_SCHEDULES_PER_USER` Kconfig option.
   * *user-index* is the user index defined for the user created in the previous step.
   * *localtime-start* is the starting time in Epoch Time.
   * *localtime-end* is the ending time in Epoch Time.
   * *destination-id* is the device node ID.
   * *endpoint-id* is the Matter door lock endpoint, in this sample assigned to ``1``.

   Both ``localtime-start`` and ``localtime-end`` are in seconds with the local time offset based on the local timezone and DST offset on the day represented by the value.

   For example, use the following command to set a ``Year-day`` schedule with index ``1`` to start on Monday, May 27, 2024, at 7:00:00 AM GMT+02:00 DST and finish on Thursday, May 30, 2024, at 7:00:00 AM GMT+02:00 DST dedicated for user with ID ``2``::

   .. code-block:: console

      ./chip-tool doorlock set-year-day-schedule 1 2 1716786000 1717045200 1 1

#. Set the example ``Holiday`` schedule using the following command:

   .. parsed-literal::
      :class: highlight

      ./chip-tool doorlock set-holiday-schedule *holiday-index* *localtime-start* *localtime-end* *operating-mode* *destination-id* *endpoint-id*

   * *holiday-index* is the index of the new schedule, starting from ``1``.
   * *localtime-start* is the starting time in Epoch Time.
   * *localtime-end* is the ending time in Epoch Time.
   * *operating-mode* is the operating mode described in the :ref:`matter_lock_scheduled_timed_access` section of this guide.
   * *destination-id* is the device node ID.
   * *endpoint-id* is the Matter door lock endpoint, in this sample assigned to ``1``.

   For example, use the following command to setup a ``Holiday`` schedule with the operating mode ``Vacation`` to start on Monday, May 27, 2024, at 7:00:00 AM GMT+02:00 DST and finish on Thursday, May 30, 2024, at 7:00:00 AM GMT+02:00 DST:

   .. parsed-literal::
      :class: highlight

      ./chip-tool doorlock set-holiday-schedule 1 1716786000 1717045200 1 1 1

#. Read saved schedules using the following commands and providing the same arguments you used in the earlier steps:

   .. parsed-literal::
      :class: highlight

      ./chip-tool doorlock get-week-day-schedule *weekday-index* *user-index* *destination-id* *endpoint-id*

   .. parsed-literal::
      :class: highlight

      ./chip-tool doorlock get-year-day-schedule *yearday-index* *user-index* *destination-id* *endpoint-id*

   .. parsed-literal::
      :class: highlight

      ./chip-tool doorlock get-holiday-schedule *holiday-index* *destination-id* *endpoint-id*

.. _matter_lock_sample_switching_thread_wifi:

Testing switching between Thread and Wi-Fi
==========================================

.. note::
   You can only test :ref:`matter_lock_sample_wifi_thread_switching` on the nRF5340 DK with the nRF7002 EK shield attached, using the :ref:`switched Thread and Wi-Fi configuration <matter_lock_sample_custom_configs>`.

To test this feature, complete the following steps:

#. Build the door lock application using the switched Thread and Wi-Fi configuration:

   .. code-block:: console

      west build -b nrf5340dk/nrf5340/cpuapp -- -DFILE_SUFFIX=thread_wifi_switched -Dlock_SHIELD=nrf7002ek -Dipc_radio_SHIELD=nrf7002ek_coex -DSB_CONFIG_WIFI_NRF70=y

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

.. note::
   Some of the steps depend on which :ref:`configuration <matter_lock_sample_custom_configs>` the sample was built with.

#. Install `nRF Toolbox`_ on your Android (Android 11 or newer) or iOS (iOS 16.1 or newer) smartphone.
#. Build the door lock application for Matter over Thread with the :kconfig:option:`CONFIG_CHIP_NUS` set to ``y``.
   For example, if you build from command line for the ``nrf52840dk/nrf52840``, use the following command:

   .. code-block:: console

      west build -b nrf52840dk/nrf52840 -- -DCONFIG_CHIP_NUS=y

#. Program the application to the kit using the following command:

   .. code-block:: console

      west flash --erase

#. If you built the sample with the debug configuration, connect the board to an UART console to see the log entries from the device.
#. Open the nRF Toolbox application on your smartphone.
#. Select :guilabel:`Universal Asynchronous Receiver/Transmitter UART` from the list in the nRF Toolbox application.
#. Tap on :guilabel:`Connect`.
   The application connects to the devices connected through UART.
#. Select :guilabel:`MatterLock_NUS` from the list of available devices.
   The Bluetooth Pairing Request with an input field for passkey appears as a notification (Android) or on the screen (iOS).
#. Depending on the configuration you are using:

   * For the release configuration: Enter the passkey ``123456``.
   * For the debug configuration, complete the following steps:

     a. Search the device's logs to find ``PROVIDE THE FOLLOWING CODE IN YOUR MOBILE APP:`` phrase.
     #. Read the randomly generated passkey from the console logs.
     #. Enter the passcode on your smartphone.

#. Wait for the Bluetooth LE connection to be established between the smartphone and the DK.
#. In the nRF Toolbox application, add the following macros:

   * ``Lock`` as the Command value type ``Text`` and any image.
   * ``Unlock`` as the Command value type ``Text`` and any image.

#. Tap on the generated macros and to change the lock state.

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
