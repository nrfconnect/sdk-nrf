.. |matter_name| replace:: Door lock
.. |matter_type| replace:: sample
.. |matter_dks_thread| replace:: ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp`` and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets
.. |matter_dks_wifi| replace:: ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target with the ``nrf7002eb2`` shield attached
.. |matter_dks_internal| replace:: nRF54LM20 DK
.. |matter_dks_tfm| replace:: ``nrf54l15dk/nrf54l15/cpuapp`` board target
.. |sample path| replace:: :file:`samples/matter/lock`
.. |matter_qr_code_payload| replace:: MT:8IXS142C00KA0648G00
.. |matter_pairing_code| replace:: 34970112332
.. |matter_qr_code_image| image:: /images/matter_qr_code_door_lock.png
                          :width: 200px
                          :alt: QR code for commissioning the door lock device

.. include:: /includes/matter/shortcuts.txt

.. _matter_lock_sample:
.. _chip_lock_sample:

Matter: Door lock
#################

.. contents::
   :local:
   :depth: 2

This door lock sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a door lock device with one basic bolt.
You can use this sample as a reference for creating your application.

.. include:: /includes/matter/introduction/sleep_thread_wifi.txt

The door lock sample can be built with support for one transport protocol, either Thread or Wi-Fi, or with support for :ref:`switching between Matter over Wi-Fi and Matter over Thread <matter_lock_sample_wifi_thread_switching>`, where the application activates either Thread or Wi-Fi on boot, depending on the runtime configuration.

The same distinction applies in the :ref:`matter_lock_sample_wifi_thread_switching` scenario, depending on the active transport protocol.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/matter/requirements/thread_wifi.txt

If you want to enable and test :ref:`Matter Bluetooth® LE with Nordic UART Service <matter_lock_sample_ble_nus>`, you also need a smartphone with either Android (Android 11 or newer) or iOS (iOS 16.1 or newer).

Overview
********

The sample uses buttons for changing the lock and device states, and LEDs to show the state of these changes.
You can test it in the following ways:

* Standalone, using a single DK that runs the door lock application.
* Remotely over the Thread or the Wi-Fi protocol, which in either case requires more devices, including a Matter controller that you can configure either on a PC or a mobile device.

.. include:: /includes/matter/overview/matter_quick_start.txt

Door lock features
==================

The door lock sample implements the following features:

* `Door lock credentials`_ - Support for door lock credentials and users.
* `Thread and Wi-Fi switching`_ - Support for switching between Matter over Thread and Matter over Wi-Fi.
* `Matter Bluetooth LE with Nordic UART Service`_ - Support for Matter Bluetooth LE with Nordic UART Service.
* :ref:`matter_lock_sample_schedules` - Support for scheduled timed access.

Use the ``click to show`` toggle to expand the content.

Door lock credentials
---------------------

.. toggle::

   By default, the application supports only PIN code credentials, but it is possible to implement support for other door lock credential types by using the ``AccessManager`` module.
   The credentials can be used to control remote access to the bolt lock.
   The PIN code assigned by the Matter controller is stored persistently, which means that it can survive a device reboot.
   Depending on the IPv6 network technology in use, the following storage implementations are enabled by default to store credentials and other lock configuration data:

   * Matter over Thread - secure storage (:kconfig:option:`CONFIG_LOCK_ACCESS_STORAGE_PROTECTED_STORAGE` Kconfig option enabled by default).
   * Matter over Wi-Fi - non-secure storage (:kconfig:option:`CONFIG_LOCK_ACCESS_STORAGE_PERSISTENT_STORAGE` and :option:`CONFIG_NCS_SAMPLE_MATTER_SETTINGS_STORAGE_BACKEND` Kconfig options enabled by default).
     For more details about the |NCS| Matter persistent storage module and its configuration, see the :ref:`ug_matter_persistent_storage` section of the :ref:`ug_matter_device_advanced_kconfigs` documentation.

   The application supports multiple door lock users and PIN code credentials.
   The following Kconfig options control the limits of the users and credentials that can be added to the door lock:

   * :kconfig:option:`CONFIG_LOCK_MAX_NUM_USERS` - Maximum number of users supported by the door lock.
   * :kconfig:option:`CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_USER` - Maximum number of credentials that can be assigned to one user.
   * :kconfig:option:`CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_TYPE` - Maximum number of credentials in total.
   * :kconfig:option:`CONFIG_LOCK_MAX_CREDENTIAL_LENGTH` - Maximum length of a single credential in bytes.

.. _matter_lock_sample_wifi_thread_switching:

Thread and Wi-Fi switching
--------------------------

.. toggle::

   When built using the :ref:`switched Thread and Wi-Fi configuration <matter_lock_sample_custom_configs>` and programmed to the nRF54LM20 DK with the nRF7002-EB II shield attached, the sample supports a feature that allows you to :ref:`switch between Matter over Thread and Matter over Wi-Fi <ug_matter_overview_architecture_integration_designs_switchable>` at runtime.
   Due to Matter protocol limitations, a single Matter node can only use one transport protocol at a time.

   The application is built with support for both Matter over Thread and Matter over Wi-Fi.
   The device activates either Thread or Wi-Fi transport protocol on boot, based on a flag stored in the non-volatile memory on the device.
   By default, Matter over Wi-Fi is activated.

   You can trigger the switch from one transport protocol to the other using the **Button 2** on the nRF54LM20 DK.
   This toggles the flag stored in the non-volatile memory, and then the device is factory reset and rebooted.
   Because the flag is toggled, the factory reset does not switch the device back to the default transport protocol (Wi-Fi).
   Instead, the factory reset and recommissioning to a Matter fabric allows the device to be provisioned with network credentials for the transport protocol that it was switched to, and to start operating in the selected network.

   See :ref:`matter_lock_sample_custom_configs` and :ref:`matter_lock_sample_switching_thread_wifi` for more information about how to configure and test this feature with this sample.

.. _matter_lock_sample_ble_nus:

Matter Bluetooth LE with Nordic UART Service
--------------------------------------------

.. toggle::

   The Matter implementation in the |NCS| lets you extend Bluetooth LE an additional Bluetooth LE service and use it in any Matter application, even when the device is not connected to a Matter network.
   The Matter door lock sample provides a basic implementation of this feature, which integrates Bluetooth LE with the :ref:`Nordic UART Service (NUS) <nus_service_readme>`.
   Using NUS, you can declare commands specific to a Matter sample and use them to control the device remotely through Bluetooth LE.
   The connection between the device and the Bluetooth controller is secured and requires providing a passcode to pair the devices.

   In the door lock sample, you can use the following commands with the Bluetooth LE with NUS:

   * ``Lock`` - To lock the door of the connected device.
   * ``Unlock`` - To unlock the door of the connected device.

   If the device is already connected to the Matter network, the notification about changing the lock state will be sent to the Bluetooth controller.

   Currently, the door lock's Bluetooth LE service extension with NUS is only available for the sample in the :ref:`Matter over Thread <ug_matter_gs_testing>` network variant.
   However, you can use the Bluetooth LE service extension regardless of whether the device is connected to a Matter over Thread network or not.

   See `Enabling Matter Bluetooth LE with Nordic UART Service`_ and `Testing Bluetooth LE with Nordic UART Service`_ for more information about how to configure and test this feature with this sample.

.. _matter_lock_scheduled_timed_access:

Scheduled timed access
----------------------

.. toggle::

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

   To use the scheduled timed access feature, :ref:`enable the Schedules support <matter_lock_sample_schedules>`.

   See the :ref:`matter_lock_sample_schedule_testing` section of this sample for more information about testing the scheduled timed access feature.

.. _matter_lock_sample_configuration:
.. _matter_lock_sample_custom_configs:

Configuration
*************

.. include:: /includes/matter/configuration/intro.txt
The |matter_type| supports the following build configurations:

.. list-table:: |matter_name| build configurations
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
     - nRF54LM20 DK with the nRF7002-EB II shield attached
     - Debug version of the application with the ability to :ref:`switch between Thread and Wi-Fi network support <matter_lock_sample_wifi_thread_switching>` in the field.

Advanced configuration options
==============================

.. include:: /includes/matter/configuration/advanced/intro.txt
.. include:: /includes/matter/configuration/advanced/tfm.txt
.. include:: /includes/matter/configuration/advanced/dfu.txt
.. include:: /includes/matter/configuration/advanced/factory_data.txt
.. include:: /includes/matter/configuration/advanced/custom_board.txt
.. include:: /includes/matter/configuration/advanced/internal_memory.txt

.. _matter_lock_sample_configuration_nus:

Enabling Matter Bluetooth LE with Nordic UART Service
-----------------------------------------------------

.. toggle::

    To enable the :ref:`matter_lock_sample_ble_nus` feature, set the :option:`CONFIG_CHIP_NUS` Kconfig option to ``y``.

    .. note::

       This |matter_type| supports one Bluetooth LE connection at a time.
       Matter commissioning, DFU, and NUS over Bluetooth LE must be run separately.

    The door lock's Bluetooth LE service extension with NUS requires a secure connection with a smartphone, which is established using a security PIN code.
    The PIN code is different depending on the `Configuration`_ the |matter_type| was built with:

    * In the ``debug`` configuration, the secure PIN code is generated randomly and printed in the log console in the following way:

    .. code-block:: console

       PROVIDE THE FOLLOWING CODE IN YOUR MOBILE APP: 165768

    * In the ``release`` configuration, the secure PIN is set to ``123456``.

    See the `Testing Bluetooth LE with Nordic UART Service`_ section for more information about how to test this feature.

.. _matter_lock_sample_schedules:

Scheduled timed access
----------------------

.. toggle::

   To enable the scheduled timed access feature, run the following command with *board_target* replaced with the board target name:

   .. parsed-literal::
      :class: highlight

      west build -b *board_target* -p -- -DEXTRA_CONF_FILE=schedules.conf

User interface
**************

.. include:: /includes/matter/interface/intro.txt

First LED:
   .. include:: /includes/matter/interface/state_led.txt

Second LED:
   Shows the state of the lock.
   The following states are possible:

   * Solid On - The bolt is extended and the door is locked.
   * Off - The bolt is retracted and the door is unlocked.
   * Rapid Even Flashing (50 ms on/50 ms off during 2 s) - The simulated bolt is in motion from one position to another.

   Additionally, the LED starts blinking evenly (500 ms on/500 ms off) when the Identify command of the Identify cluster is received on the endpoint ``1``.
   The command's argument can be used to specify the duration of the effect.

First Button:
   .. include:: /includes/matter/interface/main_button.txt

Second Button:
   * Changes the lock state to the opposite one.

Third Button:
   * On the nRF54LM20 DK with the nRF7002-EB II shield attached when using the :ref:`switched Thread and Wi-Fi configuration <matter_lock_sample_custom_configs>`: If pressed for more than ten seconds, it switches the Matter transport protocol from Thread or Wi-Fi to the other and factory resets the device.
   * On other platform or configuration: Not available.

.. include:: /includes/matter/interface/segger_usb.txt
.. include:: /includes/matter/interface/nfc.txt

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt

|matter_ble_advertising_manual|

Advanced building options
=========================

.. include:: /includes/matter/building_and_running/advanced/intro.txt
.. include:: /includes/matter/building_and_running/advanced/building_nrf54lm20dk_7002eb2.txt
.. include:: /includes/matter/building_and_running/advanced/wifi_flash.txt

Testing
*******

.. include:: /includes/matter/testing/intro.txt

.. _matter_lock_sample_testing_start:

Testing with CHIP Tool
======================

Complete the following steps to test the |matter_name| device using CHIP Tool:

.. |node_id| replace:: 1

.. include:: /includes/matter/testing/1_prepare_matter_network_thread_wifi.txt
.. include:: /includes/matter/testing/2_prepare_dk.txt
.. include:: /includes/matter/testing/3_commission_thread_wifi.txt

.. rst-class:: numbered-step

Unlock the door lock
--------------------

Send the Unlock command to the door lock via Matter controller.
Use the following command:

.. parsed-literal::
   :class: highlight

   chip-tool doorlock unlock-door |node_id| 1 --timedInteractionTimeoutMs 5000

Observe that the door lock is unlocked.

.. rst-class:: numbered-step

Lock the door lock
------------------

Send the Lock command to the door lock via Matter controller.
Use the following command:

.. parsed-literal::
   :class: highlight

   chip-tool doorlock lock-door |node_id| 1 --timedInteractionTimeoutMs 5000

Observe that the door lock is locked.

Testing with commercial ecosystem
=================================

.. include:: /includes/matter/testing/ecosystem.txt

Testing door lock features
==========================

Besides testing the basic functionality of the door lock, you can test the following features.
Some of them requires a different command to build the sample.
Expand the following toggles to see testing steps for each feature.

.. _matter_lock_sample_remote_access_with_pin:

Testing remote access with PIN code credential
----------------------------------------------

.. toggle::

   .. note::
      You can test the PIN code credential support with any Matter compatible controller.
      The following steps use the CHIP Tool controller as an example.
      For more information about setting user credentials, see the Saving users and credentials on door lock devices section of the :doc:`matter:chip_tool_guide` page in the :doc:`Matter documentation set <matter:index>`.

   #. Prepare the development kit for testing.
      Refer to the :ref:`matter_lock_sample_testing_start` section for more information.

   #. Make the door lock require a PIN code for remote operations:

      .. code-block:: console

         chip-tool doorlock write require-pinfor-remote-operation 1 |node_id| 1 --timedInteractionTimeoutMs 5000

   #. Add the example ``Home`` door lock user:

      .. code-block:: console

         chip-tool doorlock set-user 0 2 Home 123 1 0 0 |node_id| 1 --timedInteractionTimeoutMs 5000

      This command creates a ``Home`` user with a unique ID of ``123`` and an index of ``2``.
      The new user's status is set to ``1``, and both its type and credential rule to ``0``.
      The user is assigned to the door lock cluster residing on endpoint ``1`` of the node with ID ``10``.

   #. Add the example ``12345678`` PIN code credential to the ``Home`` user:

      .. code-block:: console

         chip-tool doorlock set-credential 0 '{"credentialType": 1, "credentialIndex": 1}' 12345678 2 null null |node_id| 1 --timedInteractionTimeoutMs 5000

   #. Unlock the door lock with the given PIN code:

      .. code-block:: console

         chip-tool doorlock unlock-door |node_id| 1 --PINCode 12345678 --timedInteractionTimeoutMs 5000

   #. Reboot the device.
   #. Wait until the device it rebooted and attached back to the Matter network.
   #. Unlock the door lock with the PIN code provided before the reboot:

      .. code-block:: console

         chip-tool doorlock unlock-door |node_id| 1 --PINCode 12345678 --timedInteractionTimeoutMs 5000

   .. note::
      Accessing the door lock remotely without a valid PIN code credential will fail.

.. _matter_lock_sample_schedule_testing:

Testing scheduled timed access
------------------------------

.. toggle::

   .. note::
      You can test :ref:`matter_lock_scheduled_timed_access` using any Matter compatible controller.
      The following steps use the CHIP Tool controller as an example.

      All scheduled timed access entries are saved to non-volatile memory and loaded automatically after device reboot.
      Adding a single schedule for a user contributes to the settings partition memory occupancy increase.

   #. Prepare the development kit for testing.
      Refer to the :ref:`matter_lock_sample_testing_start` section for more information.

   #. Add the example ``Home`` door lock user:

      .. code-block:: console

         chip-tool doorlock set-user 0 2 Home 123 1 0 0 |node_id| 1 --timedInteractionTimeoutMs 5000

      This command creates a ``Home`` user with a unique ID of ``123`` and an index of ``2``.
      The new user's status is set to ``1``, and both its type and credential rule to ``0``.
      The user is assigned to the door lock cluster residing on endpoint ``1`` of the node with ID |node_id|.

   #. Set the example ``Week-day`` schedule using the following command:

      .. parsed-literal::
         :class: highlight

         chip-tool doorlock set-week-day-schedule *weekday-index* *user-index* *days-mask* *start-hour* *start-minute* *end-hour* *end-minute* *destination-id* *endpoint-id*

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

         chip-tool doorlock set-week-day-schedule 1 2 84 7 30 10 30 |node_id| 1


   #. Set the example ``Year-day`` schedule using the following command:

      .. parsed-literal::
         :class: highlight

         chip-tool doorlock set-year-day-schedule *yearday-index* *user-index* *localtime-start* *localtime-end* *destination-id* *endpoint-id*


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

         chip-tool doorlock set-year-day-schedule 1 2 1716786000 1717045200 |node_id| 1

   #. Set the example ``Holiday`` schedule using the following command:

      .. parsed-literal::
         :class: highlight

         chip-tool doorlock set-holiday-schedule *holiday-index* *localtime-start* *localtime-end* *operating-mode* *destination-id* *endpoint-id*

      * *holiday-index* is the index of the new schedule, starting from ``1``.
      * *localtime-start* is the starting time in Epoch Time.
      * *localtime-end* is the ending time in Epoch Time.
      * *operating-mode* is the operating mode described in the :ref:`matter_lock_scheduled_timed_access` section of this guide.
      * *destination-id* is the device node ID.
      * *endpoint-id* is the Matter door lock endpoint, in this sample assigned to ``1``.

      For example, use the following command to setup a ``Holiday`` schedule with the operating mode ``Vacation`` to start on Monday, May 27, 2024, at 7:00:00 AM GMT+02:00 DST and finish on Thursday, May 30, 2024, at 7:00:00 AM GMT+02:00 DST:

      .. parsed-literal::
         :class: highlight

         chip-tool doorlock set-holiday-schedule 1 1716786000 1717045200 1 |node_id| 1

   #. Read saved schedules using the following commands and providing the same arguments you used in the earlier steps:

      .. parsed-literal::
         :class: highlight

         chip-tool doorlock get-week-day-schedule *weekday-index* *user-index* *destination-id* *endpoint-id*

      .. parsed-literal::
         :class: highlight

         chip-tool doorlock get-year-day-schedule *yearday-index* *user-index* *destination-id* *endpoint-id*

      .. parsed-literal::
         :class: highlight

         chip-tool doorlock get-holiday-schedule *holiday-index* *destination-id* *endpoint-id*

.. _matter_lock_sample_switching_thread_wifi:

Testing switching between Thread and Wi-Fi
--------------------------------------------

.. toggle::

   .. matter_door_lock_sample_thread_wifi_switch_desc_start

   .. note::
      You can only test :ref:`matter_lock_sample_wifi_thread_switching` on the nRF54LM20 DK with the nRF7002-EB II shield attached, using the :ref:`switched Thread and Wi-Fi configuration <matter_lock_sample_custom_configs>`.

   To test this feature, complete the following steps:

   #. Build the door lock application using the switched Thread and Wi-Fi configuration:

      .. code-block:: console

         west build -b nrf54lm20dk/nrf54lm20a/cpuapp -- -DFILE_SUFFIX=thread_wifi_switched -Dlock_SHIELD=nrf7002eb2  -DSB_CONFIG_WIFI_NRF70=y

   #. Prepare the development kit for testing.
      Refer to the :ref:`matter_lock_sample_testing_start` section for more information.

   #. Find the following log message which indicates that Wi-Fi transport protocol is activated:

      .. code-block:: console

         D: 423 [DL]Starting Matter over Wi-Fi

   #. Press the |Third Button| for more than ten seconds to trigger switching from one transport protocol to the other.
   #. Wait until the operation is finished and the device boots again.
   #. Find the following log message which indicates that Thread transport protocol is activated:

      .. code-block:: console

         D: 423 [DL]Starting Matter over Thread

   .. matter_door_lock_sample_thread_wifi_switch_desc_end

Testing Bluetooth LE with Nordic UART Service
---------------------------------------------

.. toggle::

   To test the :ref:`matter_lock_sample_ble_nus` feature, complete the following steps:

   .. note::
      Some of the steps depend on which :ref:`configuration <matter_lock_sample_custom_configs>` the sample was built with.

   #. Install `nRF Toolbox`_ on your Android (Android 11 or newer) or iOS (iOS 16.1 or newer) smartphone.
   #. Build the door lock application for Matter over Thread with the :option:`CONFIG_CHIP_NUS` set to ``y``.
      For example, if you build from command line for the ``nrf52840dk/nrf52840``, use the following command:

      .. code-block:: console

         west build -b nrf52840dk/nrf52840 -- -DCONFIG_CHIP_NUS=y

   #. Prepare the development kit for testing.
      Refer to the :ref:`matter_lock_sample_testing_start` section for more information.

   #. If you built the sample with the debug configuration, connect the board to a UART console to see the log entries from the device.
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

.. include:: /includes/matter/dependencies.txt
