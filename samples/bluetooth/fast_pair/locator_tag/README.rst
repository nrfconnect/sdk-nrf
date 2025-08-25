.. _fast_pair_locator_tag:

Bluetooth Fast Pair: Locator tag
################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates :ref:`how to use Google Fast Pair with the nRF Connect SDK <ug_bt_fast_pair>` to create a locator tag device that is compatible with the Android `Find My Device app`_.
Locator tag is a small electronic device that can be attached to an object or a person, and is designed to help locate them in case they are missing.

Google Fast Pair Service (GFPS) is a standard for pairing Bluetooth® and Bluetooth LE devices with as little user interaction required as possible.
Google Fast Pair standard also supports the Find My Device Network (FMDN) extension that is the main focus of this sample demonstration.
For detailed information, see the official `Fast Pair Find My Device Network extension`_ documentation.

.. include:: /includes/fast_pair_fmdn_rename.txt

This sample follows the `Fast Pair Device Feature Requirements for Locator Tags`_ documentation and uses the Fast Pair configuration for the locator tag use case.
The software maturity level for the locator tag use case is outlined in the :ref:`Google Fast Pair use case support <software_maturity_fast_pair_use_case>` table.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. note::
   This sample does not build or run for the ``nrf54h20dk/nrf54h20/cpuapp`` board target due to the IronSide SE migration.
   See the ``NCSDK-34821`` in the :ref:`known_issues` page for more information.
   This documentation page may still refer to concepts that were valid before the IronSide SE migration (for example, to the SUIT solution).
   The codebase and documentation will be updated in the future releases to address this issue.

.. note::
   In case of the :zephyr:board:`nrf54h20dk` board target, the application still has high power consumption as the Bluetooth LE controller running on the radio core requires disabling MRAM latency (:kconfig:option:`CONFIG_MRAM_LATENCY_AUTO_REQ`).
   Enabling MRAM latency makes the Bluetooth LE controller unstable.

Overview
********

The sample works as a Fast Pair locator tag and is compatible with the Android `Find My Device app`_.

FMDN provisioning
=================

You can add the device that runs the sample code to the `Find My Device app`_ in a process called FMDN provisioning.
The process is triggered automatically as a subsequent step of the Fast Pair procedure.

To trigger the `Fast Pair Procedure`_, move your Android device close to the locator tag that is advertising in the Fast Pair discoverable mode.
Your smartphone detects the tag's advertising packets and displays the half-sheet notification which you can use to start the procedure.
The FMDN provisioning process automatically follows the Fast Pair procedure.
During the provisioning, the device is linked with the Google account of its new owner.

Using a provisioned device
==========================

Once provisioned, the accessory starts to advertise the FMDN payload.
This payload is used by nearby Android devices to report the accessory location to its owner.

You can manage your provisioned device in the `Find My Device app`_.
The application displays a map in your item view.
It helps you determine the approximate location of your device.
You can also play a sound on the nearby tagged accessory with the `Find My Device app`_ to make it easier to find.

Synchronization mode
--------------------

In the provisioned state, the accessory attempts to synchronize its beacon clock after the system reboot (for example, due to battery replacement).
In this case, it broadcasts the Fast Pair not discoverable advertising payload to indicate that the Android device needs to synchronize with the locator tag.
The smartphone detects the Fast Pair advertising, connects to the accessory, and exchanges the necessary data to synchronize its beacon clock with the current clock value of the connected device.
After the successful synchronization, the locator tag stops the Fast Pair not discoverable advertising.
From now on, the accessory uses only the FMDN payload for the advertising process.

.. note::
   The Android device uses a throttling mechanism to prevent the beacon clock synchronization from happening more than once every 24 hours.
   The provisioning operation is also considered a clock synchronization event and requires a 24-hour wait period before the next synchronization attempt.

Identification mode
-------------------

In the provisioned state, you can enter the identification mode for a limited time.
This mode allows the connected peer to read identifying information from the locator tag.
An example of such identifying information is reading the Bluetooth GAP Device Name over Bluetooth.
The identification mode also allows for reading the Identifier Payload defined in the Detecting Unwanted Location Trackers (DULT) specification.
The timeout for identification mode is handled by the DULT subsystem and was introduced to improve user privacy.

Recovery mode
-------------

In the provisioned state, you can enter the recovery mode for a limited time.
This mode allows the Android device to recover a lost provisioning key from the locator tag.

Motion detector mode
--------------------

In the provisioned state, the accessory can activate the motion detector mode defined in the DULT specification.
The mode is activated when the accessory is separated from the owner for a sufficient amount of time (see :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MIN` and :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MAX` Kconfig options).
In this state, if motion is detected, the accessory starts the ringing action.
Emitted sounds help to alert the non-owner that they are carrying an accessory that does not belong to them and might be used by the original owner to track their location.
On DKs, you can generate the simulated motion by button action.
On the Thingy:53, the built-in accelerometer is used to detect the motion.
The motion detector is deactivated for the period set in the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_BACKOFF_PERIOD` Kconfig option after 10 sounds has been played or after the motion has been detected and 20 seconds have passed.
The motion detector is also deactivated if the accessory reappears near its owner.

FMDN unprovisioning
===================

You can remove your device from the `Find My Device app`_ in a symmetrical operation, called unprovisioning.
Once unprovisioned, the accessory stops broadcasting the FMDN advertising payload.

During the unprovisioning, the device link with the owner's Google account is removed.
To bring the accessory back to its initial state and make provisioning available for the next owner, you need to perform a factory reset.

After the factory reset operation, the Fast Pair discoverable advertising gets automatically disabled to prevent a restart of the Fast Pair procedure.
You need to press a button to set your accessory in the Fast Pair discoverable advertising mode and make it available for the next FMDN provisioning.

.. _fast_pair_locator_tag_fp_adv_policy:

Fast Pair advertising policy
============================

The sample uses the :ref:`bt_fast_pair_adv_manager_readme` module and its trigger-based system to manage the advertising process.
The advertising is turned on if at least one trigger is active.

The sample code defines and manages the following triggers:

* Pairing mode trigger that activates after a button action.

  .. note::
     This trigger can also automatically change its state in the following cases:

     * When the application is booted and the device is unprovisioned, the trigger is activated.
     * When the FMDN provisioning is started, the trigger is deactivated.
       The button action used to control this trigger state is also disabled at the beginning of the FMDN provisioning and gets enabled again once the device becomes unprovisioned.

* DFU mode trigger that activates upon a DFU mode change.

The locator tag extension that is part of the :ref:`bt_fast_pair_adv_manager_readme` module additionally defines and manages the following triggers:

* FMDN provisioning trigger that activates on the Account Key write operation during the FMDN provisioning operation.
* Beacon clock synchronization trigger that activates after the system bootup if the device is provisioned.

If the Fast Pair advertising is enabled, the sample selects the correct advertising mode based on the state of the pairing mode trigger:

   * Fast Pair discoverable advertising - Selected when the trigger is active.
   * Fast Pair not discoverable advertising - Selected when the trigger is inactive.

To fully disable Fast Pair advertising, all trigger requests must be removed.
However, you cannot manually disable all triggers, as the FMDN provisioning and Beacon clock synchronization triggers are managed by the :ref:`bt_fast_pair_adv_manager_readme` module automatically and are required for the sample to work correctly.
This approach ensures that Fast Pair advertising remains enabled as long as any of the modules needs it, preventing premature disabling.

The sample automatically disables the advertising by removing the trigger requests after the factory reset operation.
To start Fast Pair discoverable advertising after the FMDN unprovisioning and factory reset operations, you need to activate the pairing mode trigger.

.. note::

   The Bluetooth advertising is active only until the Fast Pair Provider connects to a Bluetooth Central.
   Once connected, you can still request to turn on the advertising, but it will only activate after you disconnect.

.. _fast_pair_locator_tag_dfu:

Device Firmware Update (DFU)
============================

The locator tag sample supports over-the-air updates using MCUmgr's Simple Management Protocol (SMP) over Bluetooth.
The application configures the appropriate DFU solution in relation to the selected board target (see the following table for more details).
The following DFU solutions are supported in this sample:

* The :ref:`MCUboot <mcuboot:mcuboot_ncs>` bootloader solution.
  See the :ref:`app_dfu` user guide for more information.

To enable the DFU functionality use the ``SB_CONFIG_APP_DFU`` sysbuild Kconfig option.
This option is enabled by default if a supported DFU solution is configured (see the following table to learn about supported configurations).

To select a specific version of the application, change the :file:`VERSION` file in the sample root directory.
See the :ref:`zephyr:app-version-details` for details.

.. note::
   Ensure that the FMDN firmware version defined by the following Kconfig options matches the version in the :file:`VERSION` file:

   * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MAJOR`
   * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MINOR`
   * :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_REVISION`

The configuration of the DFU solution varies depending on the board target:

+--------------+--------------------------------+-------------------------------------------------------------------+
| DFU solution | Mode of operation              | Board targets                                                     |
+==============+================================+===================================================================+
| MCUboot      | direct-xip mode without revert | * ``nrf52dk/nrf52832`` (only ``release`` configuration)           |
|              |                                | * ``nrf52833dk/nrf52833`` (only ``release`` configuration)        |
|              |                                | * ``nrf52840dk/nrf52840``                                         |
|              |                                | * ``nrf54l15dk/nrf54l05/cpuapp`` (only ``release`` configuration) |
|              |                                | * ``nrf54l15dk/nrf54l10/cpuapp``                                  |
|              |                                | * ``nrf54l15dk/nrf54l15/cpuapp``                                  |
|              |                                | * ``nrf54lm20dk/nrf54lm20a/cpuapp``                               |
+--------------+--------------------------------+-------------------------------------------------------------------+
| MCUboot      | overwrite only mode            | * ``nrf5340dk/nrf5340/cpuapp``                                    |
|              |                                | * ``nrf5340dk/nrf5340/cpuapp/ns``                                 |
|              |                                | * ``thingy53/nrf5340/cpuapp``                                     |
|              |                                | * ``thingy53/nrf5340/cpuapp/ns``                                  |
+--------------+--------------------------------+-------------------------------------------------------------------+
| SUIT         | overwrite only mode            | * ``nrf54h20dk/nrf54h20/cpuapp``                                  |
+--------------+--------------------------------+-------------------------------------------------------------------+

Signature algorithm
-------------------

Each board target that is supported by this sample, sets a specific signature algorithm for its DFU functionality.
The signature algorithm determines the type of the key pair that is used to automatically sign the application image by the |NCS| build system (the private key) and to verify the application image by the bootloader (the public key).
The bootloader must have access to the public key for its image verification process.
The choice of the signature algorithm and the implementation of the public key storage solution have an impact on the security properties of the overall DFU solution.

The configuration of the signature algorithm and the public key storage solution in this sample varies depending on the board target:

+--------------------------------+-------------------------------------------------------------------+---------------------------+---------------------------+
| Signature algorithm            | Board targets                                                     | Public key storage        | Properties                |
+================================+===================================================================+===========================+===========================+
| RSA-2048                       | * ``nrf52dk/nrf52832`` (only ``release`` configuration)           | Bootloader partition      | SW calculation,           |
|                                | * ``nrf52833dk/nrf52833`` (only ``release`` configuration)        |                           | Signature derived from    |
|                                | * ``nrf5340dk/nrf5340/cpuapp``                                    |                           | image hash                |
|                                | * ``nrf5340dk/nrf5340/cpuapp/ns``                                 |                           |                           |
|                                | * ``thingy53/nrf5340/cpuapp``                                     |                           |                           |
|                                | * ``thingy53/nrf5340/cpuapp/ns``                                  |                           |                           |
+--------------------------------+-------------------------------------------------------------------+---------------------------+---------------------------+
| ECDSA-P256                     | * ``nrf52840dk/nrf52840``                                         | Bootloader partition      | HW-accelerated            |
|                                |                                                                   |                           | (Cryptocell 310),         |
|                                |                                                                   |                           | Signature derived from    |
|                                |                                                                   |                           | image hash                |
+--------------------------------+-------------------------------------------------------------------+---------------------------+---------------------------+
| ED25519                        | * ``nrf54l15dk/nrf54l05/cpuapp`` (only ``release`` configuration) | Key Management Unit (KMU) | HW-accelerated (CRACEN),  |
|                                | * ``nrf54l15dk/nrf54l10/cpuapp``                                  |                           | Signature derived from    |
|                                | * ``nrf54l15dk/nrf54l15/cpuapp``                                  |                           | image (pure)              |
+--------------------------------+-------------------------------------------------------------------+---------------------------+---------------------------+
| ED25519                        | * ``nrf54lm20dk/nrf54lm20a/cpuapp``                               | Bootloader partition      | SW calculation,           |
|                                |                                                                   |                           | Signature derived from    |
|                                |                                                                   |                           | image (pure)              |
+--------------------------------+-------------------------------------------------------------------+---------------------------+---------------------------+

.. note::
   The SUIT DFU integration in this sample does not support the secure boot feature and its requirement for the signature verification.
   The affected board targets are not listed in the table above.

Each supported board target has the signature key file (the ``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE`` Kconfig option) defined in the :file:`sysbuild/configuration` directory that is part of the sample directory.
The signature key file is unique for each board target and is located in the :file:`<board_target>` subdirectory.
For example, the signature key file for the ``nrf54l15dk/nrf54l15/cpuapp`` board target is located in the :file:`sysbuild/configuration/nrf54l15dk_nrf54l15_cpuapp` subdirectory.

.. important::
   The signature private keys defined by the sample are publicly available and intended for demonstration purposes only.
   For production purposes, you must create and use your own signature key file that must be stored in a secure location.

DFU mode
--------

You can perform the DFU procedure by entering the DFU mode for a limited time.
The DFU mode is accessible regardless of the current FMDN provisioning state.

This mode allows the connected peer to access the SMP GATT Service.
Access to this service is restricted only to the DFU mode as a security measure.
This restriction helps satisfy FMDN privacy requirements, which prohibit the locator tag device from sharing identifying information, such as firmware version, with connected peers during standard operation.
Moreover, after entering the DFU mode, the SMP GATT Service UUID is present in the Fast Pair advertising payload which helps to filter and find the devices that are in the DFU mode.
It is located in advertising data when Fast Pair advertising is in the discoverable mode, or in the scan response data when it is in the not discoverable mode.

.. _android_notifications_fastpair:

Android notifications about firmware updates
--------------------------------------------

You can receive Android notifications about the new firmware version for your accessory if you add it during the FMDN provisioning process to the `Find My Device app`_ of your Android device.
The subsequent sections highlight the most important details to have this feature work properly.

The default device model for this sample is configured to support the Android intent feature for firmware updates (the **Firmware Type** option) and to use the `nRF Connect Device Manager`_ application as the Android companion application for this accessory firmware.
See the :ref:`fast_pair_locator_tag_google_device_model` section for configuration details.

This sample also supports the firmware version read operation over Bluetooth and the GATT Device Information Service (DIS).
This mechanism is used by the Android device to read the local firmware version of the Fast Pair accessory in the following cases:

* During the Fast Pair procedure and the FMDN provisioning operation.
* Asynchronously every 24 hours in the FMDN provisioning state.

Whenever the Android device reads the local firmware version of this Fast Pair accessory, it compares the local version with the version registered in the **Firmware Version** field of the Google Nearby Console.
If the local version is different than the registered version, the Android device generates the firmware update intent that is sent to the `nRF Connect Device Manager`_ application.
Then, the `nRF Connect Device Manager`_ application processes the intent and displays the user notification about the new firmware version available for this accessory.

.. note::
   The example code for handling the Fast Pair firmware update intents in the Android companion application is available in the `nRF Connect Device Manager GitHub repository`_.
   The code has been added as part of the `nRF Connect Device Manager GitHub PR with support for Fast Pair firmware update intents`_.

To learn more about the Android intent feature for firmware updates, see the :ref:`ug_bt_fast_pair_provisioning_register_firmware_update_intent` section in the Fast Pair user guide.

.. _fast_pair_locator_tag_google_device_model:

Fast Pair device registration
=============================

Before you can use your device as a Fast Pair Provider, the device model must be registered with Google.
This is required to obtain the Model ID and Anti-Spoofing Private Key.
You can either register your own device or use the debug Model ID and Anti-Spoofing Public/Private Key pair obtained by Nordic Semiconductor for development purposes.

.. note::
   To support the locator tag use case, you must select :guilabel:`Locator Tag` option in the **Device Type** list when registering your device model.
   To use the FMDN extension, you must also set the **Find My Device** option to ``true`` during device model registration.

If the Model ID and Anti-Spoofing Private Key are not specified, the following default debug Fast Pair model is used:

* NCS locator tag:

   * Device Name: NCS locator tag
   * Model ID: ``0x4A436B``
   * Anti-Spoofing Private Key (Base64, uncompressed): ``rie10A7ONqwd77VmkxGsblPUbMt384qjDgcEJ/ctT9Y=``
   * Device Type: Locator Tag
   * Notification Type: Fast Pair
   * Firmware Version: ``99.99.99+0``
   * Firmware Type: Non-critical
   * Companion App Package Name: no.nordicsemi.android.nrfconnectdevicemanager
   * Data-Only connection: true
   * No Personalized Name: true
   * Find My Device: true

For details, see :ref:`ug_bt_fast_pair_provisioning` in the Fast Pair user guide.

.. include:: /applications/nrf_desktop/description.rst
   :start-after: nrf_desktop_fastpair_important_start
   :end-before: nrf_desktop_fastpair_important_end

.. tip::
   The sample encodes TX power in the advertising set data that includes the Fast Pair payload.
   There is no need to provide the TX power value during device model registration.

User interface
**************

The user interface of the sample depends on the hardware platform you are using.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Indicates that the firmware is running and informs about the state of the DFU mode.
         Depending on the DFU mode state:

         * Blinks at a 1 second interval if the DFU mode is disabled.
         * Blinks at a 0.25 second interval if the DFU mode is enabled.

      LED 2:
         Depending on the motion detected event, the ringing state, and the motion detector state:

         * Blinks fast twice if the motion detected event appears.
         * Lit if the device is ringing and no motion detection event is being indicated.
         * Blinks at a 0.25 second interval if the motion detector is active and none of the above conditions are met.
         * Off if none of the above conditions are met.

      LED 3:
         Depending on the FMDN provisioning state and the Fast Pair advertising state:

         * Lit if the device is provisioned and Fast Pair advertising is disabled.
         * Blinks at a 0.25 second interval if the device is provisioned and Fast Pair advertising is enabled.
         * Blinks at a 1 second interval if the device is not provisioned and Fast Pair advertising is enabled.
         * Off if the device is not provisioned and Fast Pair advertising is disabled.

      LED 4:
         Depending on the states of the recovery mode and the identification mode:

         * Lit if both modes are active on the device.
         * Off if both modes are inactive on the device.
         * Blinks at a 0.5 second interval if the identification mode is active on the device and the recovery mode is inactive on the device.
         * Blinks at a 1 second interval if the recovery mode is active on the device and the identification mode is inactive on the device.

      Button 1:
         Sends a request to turn on Fast Pair discoverable advertising or removes such a request.
         This action controls the pairing mode trigger.
         It is enabled only in the FMDN unprovisioned state and is disabled immediately once the FMND provisioning is started.
         See the :ref:`fast_pair_locator_tag_fp_adv_policy` section for details.

      Button 2:
         Stops the ongoing ringing action on a single press.
         Generates simulated motion event on a double press.

      Button 3:
         Decrements the battery level by 10% (the default value in the :ref:`CONFIG_APP_BATTERY_LEVEL_DECREMENT <CONFIG_APP_BATTERY_LEVEL_DECREMENT>` Kconfig option), starting from the full battery level of 100%.
         In the default sample configuration, with the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` Kconfig option enabled, pressing the button when the battery level is lower than the decrement value transitions the device back to the starting point of 100%.
         You can disable the default :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` Kconfig setting to transition through the mode in which the battery level indication is not supported (refer to the last point in the list).
         The battery level is encoded in the FMDN advertising set according to the following rules:

         * Normal battery level (starting point, as derived from the 100% battery level) - The battery level is higher than 40% and less than or equal to 100%.
           The lower threshold, 40%, is controlled by the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_LOW_THR` Kconfig option.
         * Low battery level - The battery level is higher than 10% and less than or equal to 40%.
           The lower threshold, 10%, is controlled by the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_CRITICAL_THR` Kconfig option.
         * Critically low battery level (battery replacement needed soon) - The battery level is higher than or equal to 0% and less than or equal to 10%.
         * Battery level indication unsupported - Occurs when the special :c:macro:`BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE` value is used.
           This battery level is unavailable in the default sample configuration.
           You can disable the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` Kconfig option to reach this mode when transitioning from the critically low battery level to the full battery level of 100%.

         In the default sample configuration, with the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` Kconfig option enabled, the battery level is also encoded in the Battery Level response of the Accessory Non-owner Service (ANOS), which is set according to the following rules:

         * Full battery level (starting point, as derived from the 100% battery level) - The battery level is higher than 80% and less than or equal to 100%.
           The lower threshold, 80%, is controlled by the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_MEDIUM_THR` Kconfig option.
         * Medium battery level - The battery level is higher than 40% and less than or equal to 80%.
           The lower threshold, 40%, is controlled by the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_LOW_THR` Kconfig option.
         * Low battery level - The battery level is higher than 10% and less than or equal to 40%.
           The lower threshold, 10%, is controlled by the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_CRITICAL_THR` Kconfig option.
         * Critically low battery level (battery replacement needed soon) - The battery level is higher than or equal to 0% and less than or equal to 10%.

      Button 4:
         When pressed during the application bootup, resets the accessory to its default factory settings.

         The triggered action varies depending on how long the button was held:

            * From 0 to 3 seconds:

              Requests the FMDN subsystem to enable the identification mode for five minutes.
              This timeout value is defined by the :kconfig:option:`CONFIG_DULT_ID_READ_STATE_TIMEOUT` Kconfig option according to the DULT specification requirements.

            * From 3 to 7 seconds:

              Enables the recovery mode for one minute as defined by the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY_TIMEOUT` Kconfig option.

            * From 7 seconds or more:

              Enables the DFU mode for five minutes.

   .. group-tab:: nRF54 DKs

      LED 0:
         Indicates that the firmware is running and informs about the state of the DFU mode.
         Depending on the DFU mode state:

         * Blinks at a 1 second interval if the DFU mode is disabled.
         * Blinks at a 0.25 second interval if the DFU mode is enabled.

      LED 1:
         Depending on the motion detected event, the ringing state, and the motion detector state:

         * Blinks fast twice if the motion detected event appears.
         * Lit if the device is ringing and no motion detection event is being indicated.
         * Blinks at a 0.25 second interval if the motion detector is active and none of the above conditions are met.
         * Off if none of the above conditions are met.

      LED 2:
         Depending on the FMDN provisioning state and the Fast Pair advertising state:

         * Lit if the device is provisioned and Fast Pair advertising is disabled.
         * Blinks at a 0.25 second interval if the device is provisioned and Fast Pair advertising is enabled.
         * Blinks at a 1 second interval if the device is not provisioned and Fast Pair advertising is enabled.
         * Off if the device is not provisioned and Fast Pair advertising is disabled.

      LED 3:
         Depending on the states of the recovery mode and the identification mode:

         * Lit if both modes are active on the device.
         * Off if both modes are inactive on the device.
         * Blinks at a 0.5 second interval if the identification mode is active on the device and the recovery mode is inactive on the device.
         * Blinks at a 1 second interval if the recovery mode is active on the device and the identification mode is inactive on the device.

      Button 0:
         Sends a request to turn on Fast Pair discoverable advertising or removes such a request.
         This action controls the pairing mode trigger.
         It is enabled only in the FMDN unprovisioned state and is disabled immediately once the FMND provisioning is started.
         See the :ref:`fast_pair_locator_tag_fp_adv_policy` section for details.

      Button 1:
         Stops the ongoing ringing action on a single press.
         Generates simulated motion event on a double press.

      Button 2:
         Decrements the battery level by 10% (the default value in the :ref:`CONFIG_APP_BATTERY_LEVEL_DECREMENT <CONFIG_APP_BATTERY_LEVEL_DECREMENT>` Kconfig option), starting from the full battery level of 100%.
         In the default sample configuration, with the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` Kconfig option enabled, pressing the button when the battery level is lower than the decrement value transitions the device back to the starting point of 100%.
         You can disable the default :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` Kconfig setting to transition through the mode in which the battery level indication is not supported (refer to the last point in the list).
         The battery level is encoded in the FMDN advertising set according to the following rules:

         * Normal battery level (starting point, as derived from the 100% battery level) - The battery level is higher than 40% and less than or equal to 100%.
           The lower threshold, 40%, is controlled by the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_LOW_THR` Kconfig option.
         * Low battery level - The battery level is higher than 10% and less than or equal to 40%.
           The lower threshold, 10%, is controlled by the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_CRITICAL_THR` Kconfig option.
         * Critically low battery level (battery replacement needed soon) - The battery level is higher than or equal to 0% and less than or equal to 10%.
         * Battery level indication unsupported - Occurs when the special :c:macro:`BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE` value is used.
           This battery level is unavailable in the default sample configuration.
           You can disable the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` Kconfig option to reach this mode when transitioning from the critically low battery level to the full battery level of 100%.

         In the default sample configuration, with the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT` Kconfig option enabled, the battery level is also encoded in the Battery Level response of the Accessory Non-owner Service (ANOS), which is set according to the following rules:

         * Full battery level (starting point, as derived from the 100% battery level) - The battery level is higher than 80% and less than or equal to 100%.
           The lower threshold, 80%, is controlled by the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_MEDIUM_THR` Kconfig option.
         * Medium battery level - The battery level is higher than 40% and less than or equal to 80%.
           The lower threshold, 40%, is controlled by the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_LOW_THR` Kconfig option.
         * Low battery level - The battery level is higher than 10% and less than or equal to 40%.
           The lower threshold, 10%, is controlled by the :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_CRITICAL_THR` Kconfig option.
         * Critically low battery level (battery replacement needed soon) - The battery level is higher than or equal to 0% and less than or equal to 10%.

      Button 3:
         When pressed during the application bootup, resets the accessory to its default factory settings.

         The triggered action varies depending on how long the button was held:

            * From 0 to 3 seconds:

              Requests the FMDN subsystem to enable the identification mode for five minutes.
              This timeout value is defined by the :kconfig:option:`CONFIG_DULT_ID_READ_STATE_TIMEOUT` Kconfig option according to the DULT specification requirements.

            * From 3 to 7 seconds:

              Enables the recovery mode for one minute as defined by the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY_TIMEOUT` Kconfig option.

            * From 7 seconds or more:

              Enables the DFU mode for five minutes.

   .. group-tab:: Thingy:53

      RGB LED:
         Thingy:53 displays the application state in the RGB scale using **LED1**.
         The **LED1** displays a color sequence with each blink that indicates the overall application state.

         Each color of the LED indicates the different application state:

         * Green - Indicates that firmware is running.
         * Blue - Indicates that the device is provisioned.
         * Yellow - Indicates that the identification mode is active.
         * Red - Indicates that the recovery mode is active.
         * White - Indicates that the Fast Pair advertising is active.
         * Purple - Indicates that the DFU mode is active.
         * Cyan - Indicates that the motion detector is active.

      Speaker/Buzzer:
         Produces sound when the ringing action is in progress and to indicate a new button action.

      Button (SW3):
         When pressed during the application bootup, resets the accessory to its default factory settings.

         When pressed, stops the ongoing ringing action.

         When released, the triggered action varies depending on how long the button was held:

         * From 1 to 3 seconds (notified by one short beep):

           Sends a request to turn on Fast Pair discoverable advertising or removes such a request.
           This action controls the pairing mode trigger.
           It is enabled only in the FMDN unprovisioned state and is disabled immediately once the FMND provisioning is started.
           See the :ref:`fast_pair_locator_tag_fp_adv_policy` section for details.

         * From 3 to 5 seconds (notified by two short beeps):

           Requests the FMDN subsystem to enable the identification mode for five minutes.
           This timeout value is defined by the :kconfig:option:`CONFIG_DULT_ID_READ_STATE_TIMEOUT` Kconfig option according to the DULT specification requirements.

         * From 5 to 7 seconds (notified by three short beeps):

           Enables the recovery mode for one minute as defined by the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY_TIMEOUT` Kconfig option.

         * From 7 to 10 seconds (notified by four short beeps):

           Enables the DFU mode for five minutes.

         * From 10 seconds or more (notified by one longer beep):

           No action - allows to not perform any button operation.

Configuration
*************

|config|

Configuration options
=====================

The following Kconfig options are specific to the Fast Pair locator tag sample.

.. _SB_CONFIG_APP_DFU:

SB_CONFIG_APP_DFU
   The sample sysbuild configuration option enables the Device Firmware Update (DFU) functionality.
   The value of this option is propagated to the application configuration option :ref:`CONFIG_APP_DFU <CONFIG_APP_DFU>`.
   On multi-core devices, it adds the Kconfig fragment to the network core image configuration which speeds up the DFU process.
   This option is enabled by default if the MCUboot bootloader image is used (``SB_CONFIG_BOOTLOADER_MCUBOOT``) or if the chosen board target is based on an nRF54H Series SoC  (``SB_CONFIG_SOC_SERIES_NRF54HX``).

.. _CONFIG_APP_DFU:

CONFIG_APP_DFU
   The sample application configuration option enables the Device Firmware Update (DFU) functionality.
   The value of this option is set based on the sysbuild configuration option :ref:`SB_CONFIG_APP_DFU <SB_CONFIG_APP_DFU>`.

The following Kconfig options are specific to the chosen hardware platform.

.. tabs::

   .. group-tab:: Development kits

      .. _CONFIG_APP_BATTERY_LEVEL_DECREMENT:

      CONFIG_APP_BATTERY_LEVEL_DECREMENT
         The sample configuration defines the decrement value used to simulate the battery level on the development kit.
         The battery level is defined in percentages and ranges from 0% to 100%.
         By default, the decrement level is set to 10%.


   .. group-tab:: Thingy:53

      .. _CONFIG_APP_BATTERY_POLL_INTERVAL:

      CONFIG_APP_BATTERY_POLL_INTERVAL
         The sample configuration defines the polling interval in seconds for measuring battery level.
         By default, the interval is set to 60 seconds.

      .. _CONFIG_APP_UI_SPEAKER_FREQ:

      CONFIG_APP_UI_SPEAKER_FREQ
         The sample configuration defines the frequency in hertz units for sounds generated by the speaker.
         By default, the frequency is set to 4000 Hz.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/fast_pair/locator_tag`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

When building the sample, you can provide the Fast Pair Model ID (``SB_CONFIG_BT_FAST_PAIR_MODEL_ID``) and the Fast Pair Anti-Spoofing Key (``SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY``) as sysbuild Kconfig options.
If the data is not provided, the sample uses the default provisioning data obtained for the *NCS locator tag* (the locator tag debug Fast Pair Provider).
See :ref:`ug_bt_fast_pair_provisioning` for details.

.. note::
   You cannot use the sample without the Fast Pair provisioning data.
   Programming the device with the sample firmware without providing the proper Fast Pair provisioning data results in assertion failure during boot.

Release build
=============

To build the sample in a release variant, set the ``FILE_SUFFIX=release`` CMake option.
The build will use the :file:`prj_release.conf` configuration file instead of :file:`prj.conf`.
Check the contents of both files to learn which configuration changes you should apply when preparing the production build of your end product.

The release build reduces the code size and RAM usage of the sample by disabling logging functionality and performing other optimizations.

See :ref:`cmake_options` for detailed instructions on how to add the ``FILE_SUFFIX=release`` option to your build.
For example, when building from the command line, you can add it as follows:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DFILE_SUFFIX=release

DFU build with the key storage in KMU
=====================================

The MCUboot-based targets that enable the ``SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU`` Kconfig option use the Key Management Unit (KMU) hardware peripheral to store the public key that is used by the bootloader to verify the application image.

.. note::
   The board targets based on the nRF54L SoC Series are currently the only targets that support the KMU-based key storage.
   See the :ref:`fast_pair_locator_tag_dfu` section of this sample documentation for the details regarding the supported signature algorithms, public key storage location and the signature key file.

To use KMU, the public key must first be provisioned.
This provisioning step can be performed automatically by the west runner, provided that a :file:`keyfile.json` file is present in the build directory.
In this sample, the :file:`keyfile.json` file is automatically generated using the ``SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE`` Kconfig option.
This option uses the input key specified by the ``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE`` Kconfig option to generate the required file during the build process.

To trigger KMU provisioning during flashing, use the ``west flash`` command with either the ``--erase`` or ``--recover`` flag.
This ensures that both the firmware and the MCUboot public key are correctly programmed onto the target device using KMU-based key storage.
Use the following command to perform the operation:

.. parsed-literal::
   :class: highlight

   west flash --recover

Alternatively, you can perform the provisioning operation manually with the ``west ncs-provision upload`` command and then flash the device with the ``west flash`` command.
See :ref:`ug_nrf54l_developing_provision_kmu` for further details regarding the KMU provisioning process.

.. _fast_pair_locator_tag_motion_detector_test_build:

Motion detector test build
==========================

If you want to make testing the motion detector easier, enable the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_TEST_MODE` Kconfig option to shorten the period between the device entering the Unwanted Tracking Protection mode and the activation of the motion detector.
With this option enabled, the values of the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MIN` and :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MAX` Kconfig options that are responsible for this period are shortened to three minutes.
Otherwise, the period would be a random value between 8 and 24 hours.
The :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_TEST_MODE` Kconfig option also shortens the motion detector backoff period (:kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_BACKOFF_PERIOD`) from six hours to two minutes.

See :ref:`configuring_kconfig` for detailed instructions on how to enable the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_TEST_MODE` Kconfig option in your build.
For example, when building from the command line, you can enable it as follows:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DCONFIG_DULT_MOTION_DETECTOR_TEST_MODE=y

.. _fast_pair_locator_tag_testing:

Testing
=======

.. note::
   Images in the testing section are generated for the debug device model registered by Nordic Semiconductor in the Google Nearby Console (see :ref:`fast_pair_locator_tag_google_device_model`).
   The debug device model name is covered by asterisks and the default Fast Pair logo is displayed instead of the one specified during the device model registration.

   If the test Android device uses a primary email account that is not on Google's email allow list for the FMDN feature, testing steps will fail at the FMDN provisioning stage for the default debug (uncertified) device model.
   To be able to test with debug device models and use the "Inspect Spot device" tool in the `Find My Device app`_, register your development email account by completing Google's device proposal form.
   You can find the link to the device proposal form in the `Fast Pair Find My Device Network extension`_ specification.

|test_sample|

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_terminal_specific|
      #. Reset the development kit.
      #. Observe that **LED 1** is blinking to indicate that firmware is running.
      #. Observe that **LED 3** is blinking in 1 second intervals, which indicates that the device is not provisioned as an FMDN beacon and the Fast Pair discoverable advertising is enabled.
         The Fast Pair Provider is ready for the FMDN provisioning.
      #. Move the Android device close to your locator tag device.
      #. Wait for the notification from your Android device about the detected Fast Pair Provider.

         .. figure:: /images/bt_fast_pair_locator_tag_discoverable_notification.png
            :scale: 80 %
            :alt: Fast Pair discoverable notification with support for the `Find My Device app`_

      #. Initiate the connection and trigger the Fast Pair procedure by tapping the :guilabel:`Connect` button.
         After the procedure is complete, you will see a pop-up with the Acceptable Use Policy for the `Find My Device app`_.

         .. note::
            If you use the default debug device model and you have the `nRF Connect Device Manager`_ application installed on your test Android device, you may get the Android notification from this mobile application about the new firmware update.
            You can ignore this notification, as it is not related to the Fast Pair provisioning process.
            To test the notification feature, follow the :ref:`fast_pair_locator_tag_testing_fw_update_notifications` test section.

      #. If you want to start the FMDN provisioning, accept the Acceptable Use Policy by tapping the :guilabel:`Agree and continue` button.

         .. figure:: /images/bt_fast_pair_locator_tag_acceptable_use_policy.png
            :scale: 80 %
            :alt: The Find My Device Acceptable Use Policy

      #. Wait for the Android device's notification that indicates completion of the provisioning process.

         .. figure:: /images/bt_fast_pair_locator_tag_provisioning_complete.png
            :scale: 80 %
            :alt: Notification about provisioning completion for the `Find My Device app`_

      #. Observe that **LED 3** is lit, which indicates that the device is provisioned as an FMDN beacon and the Fast Pair advertising is disabled.
      #. Open the `Find My Device app`_ by tapping the :guilabel:`Open app` button.
      #. In your accessory view, tap the :guilabel:`Find nearby` button.
      #. Observe in the Find nearby view that the grey shape shrinks when you move your Android device further away from your locator tag device.
         The shape also grows when you move your Android device closer to the locator tag.
         The closest proximity level is indicated by the device logo displayed inside the grey shape.

         .. figure:: /images/bt_fast_pair_locator_tag_fmd_app_nearby_view.png
            :scale: 80 %
            :alt: Find nearby view of the `Find My Device app`_

      #. Start the ringing action on your device by tapping the :guilabel:`Play sound` button.
      #. Observe that **LED 2** is lit, which indicates that the ringing action is in progress.
      #. Stop the ringing action in one of the following ways:

         * Press **Button 2** on the locator tag device.
         * Tap the :guilabel:`Stop sound` button in the `Find My Device app`_.

      #. Observe that **LED 2** is off, which indicates that your accessory is no longer ringing.
      #. Press **Button 3** a few times to change the normal (default) battery level to low battery level.
      #. See that the Android notification with the **Device with low battery** label is displayed after a while.
      #. Exit the Find nearby view and return to the main accessory view.
      #. Open **Device details** by tapping the cog icon, located below the map view, next to the accessory name.
      #. Start the unprovisioning operation by tapping the :guilabel:`Remove from Find My Device` button at the bottom of the screen.
      #. Confirm the operation by tapping the :guilabel:`Remove` button in the Remove from Find My Device view.

         .. figure:: /images/bt_fast_pair_locator_tag_fmd_app_unprovision.png
            :scale: 80 %
            :alt: Remove from Find My Device view

      #. Reconfirm the procedure by tapping the :guilabel:`Remove` button in the Android pop-up window.
      #. Wait for the unprovisioning operation to complete.
      #. Observe that **LED 3** is off, which indicates that the device is no longer provisioned as an FMDN beacon and the Fast Pair advertising is disabled.
      #. Observe that the Android does not display a notification about the detected Fast Pair Provider, as the locator tag device disables advertising after the unprovisioning operation.
      #. Press **Button 1** to request turning on the Fast Pair advertising in discoverable mode and to restart the FMDN provisioning process.

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_specific|
      #. Reset the development kit.
      #. Observe that **LED 0** is blinking to indicate that firmware is running.
      #. Observe that **LED 2** is blinking in 1 second intervals, which indicates that the device is not provisioned as an FMDN beacon and the Fast Pair discoverable advertising is enabled.
         The Fast Pair Provider is ready for the FMDN provisioning.
      #. Move the Android device close to your locator tag device.
      #. Wait for the notification from your Android device about the detected Fast Pair Provider.

         .. figure:: /images/bt_fast_pair_locator_tag_discoverable_notification.png
            :scale: 80 %
            :alt: Fast Pair discoverable notification with support for the `Find My Device app`_

      #. Initiate the connection and trigger the Fast Pair procedure by tapping the :guilabel:`Connect` button.
         After the procedure is complete, you will see a pop-up with the Acceptable Use Policy for the `Find My Device app`_.

         .. note::
            If you use the default debug device model and you have the `nRF Connect Device Manager`_ application installed on your test Android device, you may get the Android notification from this mobile application about the new firmware update.
            You can ignore this notification, as it is not related to the Fast Pair provisioning process.
            To test the notification feature, follow the :ref:`fast_pair_locator_tag_testing_fw_update_notifications` test section.

      #. If you want to start the FMDN provisioning, accept the Acceptable Use Policy by tapping the :guilabel:`Agree and continue` button.

         .. figure:: /images/bt_fast_pair_locator_tag_acceptable_use_policy.png
            :scale: 80 %
            :alt: The Find My Device Acceptable Use Policy

      #. Wait for the Android device's notification that indicates completion of the provisioning process.

         .. figure:: /images/bt_fast_pair_locator_tag_provisioning_complete.png
            :scale: 80 %
            :alt: Notification about provisioning completion for the `Find My Device app`_

      #. Observe that **LED 2** is lit, which indicates that the device is provisioned as an FMDN beacon and the Fast Pair advertising is disabled.
      #. Open the `Find My Device app`_ by tapping the :guilabel:`Open app` button.
      #. In your accessory view, tap the :guilabel:`Find nearby` button.
      #. Observe in the Find nearby view that the grey shape shrinks when you move your Android device further away from your locator tag device.
         The shape also grows when you move your Android device closer to the locator tag.
         The closest proximity level is indicated by the device logo displayed inside the grey shape.

         .. figure:: /images/bt_fast_pair_locator_tag_fmd_app_nearby_view.png
            :scale: 80 %
            :alt: Find nearby view of the `Find My Device app`_

      #. Start the ringing action on your device by tapping the :guilabel:`Play sound` button.
      #. Observe that **LED 1** is lit, which indicates that the ringing action is in progress.
      #. Stop the ringing action in one of the following ways:

         * Press **Button 1** on the locator tag device.
         * Tap the :guilabel:`Stop sound` button in the `Find My Device app`_.

      #. Observe that **LED 1** is off, which indicates that your accessory is no longer ringing.
      #. Press **Button 2** a few times to change the normal (default) battery level to low battery level.
      #. See that the Android notification with the **Device with low battery** label is displayed after a while.
      #. Exit the Find nearby view and return to the main accessory view.
      #. Open **Device details** by tapping the cog icon, located below the map view, next to the accessory name.
      #. Start the unprovisioning operation by tapping the :guilabel:`Remove from Find My Device` button at the bottom of the screen.
      #. Confirm the operation by tapping the :guilabel:`Remove` button in the Remove from Find My Device view.

         .. figure:: /images/bt_fast_pair_locator_tag_fmd_app_unprovision.png
            :scale: 80 %
            :alt: Remove from Find My Device view

      #. Reconfirm the procedure by tapping the :guilabel:`Remove` button in the Android pop-up window.
      #. Wait for the unprovisioning operation to complete.
      #. Observe that **LED 2** is off, which indicates that the device is no longer provisioned as an FMDN beacon and the Fast Pair advertising is disabled.
      #. Observe that the Android does not display a notification about the detected Fast Pair Provider, as the locator tag device disables advertising after the unprovisioning operation.
      #. Press **Button 0** to request turning on the Fast Pair advertising in discoverable mode and to restart the FMDN provisioning process.

.. _fast_pair_locator_tag_testing_clock_sync:

Clock synchronization
---------------------

Testing steps for the clock synchronization feature require a second Android device.
The device must be registered to a **different** Google account from the first Android device.
Testing steps also require the `nRF Connect for Mobile`_ application on your second Android device.

.. note::
   You can execute these testing steps in combination with the :ref:`fast_pair_locator_tag_testing_fw_update_notifications` testing steps, as both test variants require you to wait for more than 24 hours.

To test this feature, complete the following steps:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. Go to the :ref:`fast_pair_locator_tag_testing` section and follow the instructions on performing the FMDN provisioning operation.
      #. Observe that **LED 3** is lit, which indicates that the device is provisioned as an FMDN beacon.
      #. Power off the development kit and wait for 24 hours.
      #. Power off the Android device that you used to perform the FMDN provisioning operation.
      #. Power on the development kit.
      #. |connect_terminal_specific|
      #. Use your second Android device configured for a **different** account, and start the `nRF Connect for Mobile`_ application.
      #. Wait for the **SCANNER** tab of the mobile application to populate the scanning list.
      #. Observe that the device advertising the Fast Pair Service UUID (*0xFE2C*) AD type with the Fast Pair Account Data payload is present in the scanning list.
         This entry indicates that the development kit is in the clock synchronization mode, and it is advertising the Fast Pair not discoverable payload.

         .. note::
            By default, the `nRF Connect for Mobile`_ application filters advertising packets used by ecosystems like Google or Apple.
            You need to take additional steps to see the Fast Pair advertising on the scanning list:

               1. In the **SCANNER** tab of the `nRF Connect for Mobile`_ application, tap the :guilabel:`No filter` white bar below the **SCANNER** label.
               #. In the drop-down menu for the filtering configuration, tap the three-dot icon in the **Exclude** option.
               #. In the exclude configuration menu, uncheck the **Google** option to remove the filtering for Google-related advertising like the Fast Pair advertising.
               #. Close the exclude configuration menu and the filtering configuration drop-down.
               #. Tap the :guilabel:`SCAN` button to restart the scanning activity.

      #. Power on the first Android device that you used to perform the FMDN provisioning operation.
      #. In the terminal, observe the message confirming that your first Android device connected to the development kit and synchronized the clock value::

            FMDN: clock information synchronized with the authenticated Bluetooth peer

      #. Go back to your second Android device and refresh the scanning list in the `nRF Connect for Mobile`_ application.
      #. Observe that the entry for the device advertising the Fast Pair Service UUID AD type is no longer present in the scanning list.
         The missing entry indicates that the development kit is no longer in the clock synchronization mode, and it is not advertising the Fast Pair not discoverable payload.
      #. In your first Android device, open the `Find My Device app`_.
         Navigate to your accessory view, and tap the :guilabel:`Find nearby` button.
      #. Start the ringing action on your device by tapping the :guilabel:`Play sound` button.
      #. Observe that **LED 2** is lit to confirm that the Android device is able to connect to your development kit after a clock synchronization.

   .. group-tab:: nRF54 DKs

      1. Go to the :ref:`fast_pair_locator_tag_testing` section and follow the instructions on performing the FMDN provisioning operation.
      #. Observe that **LED 2** is lit, which indicates that the device is provisioned as an FMDN beacon.
      #. Power off the development kit and wait for 24 hours.
      #. Power off the Android device that you used to perform the FMDN provisioning operation.
      #. Power on the development kit.
      #. |connect_terminal_specific|
      #. Use your second Android device configured for a **different** account, and start the `nRF Connect for Mobile`_ application.
      #. Wait for the **SCANNER** tab of the mobile application to populate the scanning list.
      #. Observe that the device advertising the Fast Pair Service UUID (*0xFE2C*) AD type with the Fast Pair Account Data payload is present in the scanning list.
         This entry indicates that the development kit is in the clock synchronization mode, and it is advertising the Fast Pair not discoverable payload.

         .. note::
            By default, the `nRF Connect for Mobile`_ application filters advertising packets used by ecosystems like Google or Apple.
            You need to take additional steps to see the Fast Pair advertising on the scanning list:

               1. In the **SCANNER** tab of the `nRF Connect for Mobile`_ application, tap the :guilabel:`No filter` white bar below the **SCANNER** label.
               #. In the drop-down menu for the filtering configuration, tap the three-dot icon in the **Exclude** option.
               #. In the exclude configuration menu, uncheck the **Google** option to remove the filtering for Google-related advertising like the Fast Pair advertising.
               #. Close the exclude configuration menu and the filtering configuration drop-down.
               #. Tap the :guilabel:`SCAN` button to restart the scanning activity.

      #. Power on the first Android device that you used to perform the FMDN provisioning operation.
      #. In the terminal, observe the message confirming that your first Android device connected to the development kit and synchronized the clock value::

            FMDN: clock information synchronized with the authenticated Bluetooth peer

      #. Go back to your second Android device and refresh the scanning list in the `nRF Connect for Mobile`_ application.
      #. Observe that the entry for the device advertising the Fast Pair Service UUID AD type is no longer present in the scanning list.
         The missing entry indicates that the development kit is no longer in the clock synchronization mode, and it is not advertising the Fast Pair not discoverable payload.
      #. In your first Android device, open the `Find My Device app`_.
         Navigate to your accessory view, and tap the :guilabel:`Find nearby` button.
      #. Start the ringing action on your device by tapping the :guilabel:`Play sound` button.
      #. Observe that **LED 1** is lit to confirm that the Android device is able to connect to your development kit after a clock synchronization.

Motion detector
---------------

To test this feature, complete the following steps:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. Enable the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_TEST_MODE` Kconfig option in the sample configuration to shorten the motion detector activation periods.
         Refer to the :ref:`fast_pair_locator_tag_motion_detector_test_build` section for details on how to build the sample in the motion detector test mode.
      #. Build and flash the application in the motion detector test mode on your target device.
      #. Go to the :ref:`fast_pair_locator_tag_testing` section and follow the instructions on performing the FMDN provisioning operation.
      #. Observe that **LED 3** is lit, which indicates that the device is provisioned as an FMDN beacon.
      #. Double-click **Button 2** to simulate the motion event.
      #. Observe that **LED 2** blinks fast twice, which indicates that the motion detected event appears.
      #. Observe that the ringing action does not start, because the motion detector is inactive.
      #. Put the device into the Unwanted Tracking Protection mode.
         You can do it in two ways.
         If you have access to the "Inspect Spot device" tool in the Find My Device Android app, you can:

         1. Open the Find My Device app.
         #. Tap your accessory in the list.
            The accessory should have "Nearby" written under its name.
         #. Tap the gear icon next to the device name
         #. Tap the 3-dot menu and then :guilabel:`Inspect Spot device (internal)`.
         #. Tap the 3-dot menu and then :guilabel:`Activate Unwanted Tracking Mode`.
         #. Observe the "Changed unwanted tracking mode successfully" message on the screen.
         #. Turn off your Android device to prevent it from automatically deactivating the mode.
            Do this as quickly as possible after activating the mode.

         Otherwise, you can:

         1. Turn off your Android device.
         #. Wait for minimum 30 minutes for other Android device to put the device into the Unwanted Tracking Protection mode.
            The other Android device can belong to anyone around you.
            It is recommended to be in a crowded area.

      #. Wait for three minutes (random value between the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MIN` and :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MAX` Kconfig options) for the motion detector to be activated.
      #. Observe that **LED 2** blinks at a 0.25 second interval, which indicates that the motion detector is active.
      #. Double-click **Button 2** to simulate the motion event.
      #. Observe that **LED 2** blinks fast twice, which indicates that the motion detected event appears.
      #. Observe that after up to 10 seconds, the ringing action starts for one second, which is indicated by **LED 2** being lit.
      #. Observe that **LED 2** goes back to blinking at a 0.25 second interval, which indicates that the motion detector is active.
      #. Double-click **Button 2** to simulate the motion event.
      #. Observe that **LED 2** blinks fast twice, which indicates that the motion detected event appears.
      #. Observe that after up to 0.5 second, the ringing action starts for one second, which is indicated by **LED 2** being lit.
      #. Observe that **LED 2** goes back to blinking at a 0.25 second interval, which indicates that the motion detector is active.
      #. Double-click **Button 2** to simulate the motion event.
      #. Observe that **LED 2** blinks fast twice, which indicates that the motion detected event appears.
      #. Observe that after up to 0.5 second, the ringing action starts, which is indicated by **LED 2** being lit.
      #. Double-click **Button 2** to simulate the motion event while the **LED 2** is still lit (ringing action is still in progress).
      #. Observe that **LED 2** blinks fast twice, which indicates that the motion detected event appears.
      #. Observe that after 0.5 second of not ringing, the ringing action starts again for one second, which is indicated by **LED 2** being lit.
      #. Observe that **LED 2** goes back to blinking at a 0.25 second interval, which indicates that the motion detector is active.
      #. Observe that after 20 seconds from the first motion event or after 10 ringing actions completes, the **LED 2** is off, which indicates that the motion detector is inactive.
      #. Double-click **Button 2** to simulate the motion event.
      #. Observe that **LED 2** blinks fast twice, which indicates that the motion detected event appears.
      #. Observe that the ringing action does not start, because the motion detector is inactive.
      #. Wait for two minutes (the backoff period defined by the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_BACKOFF_PERIOD` Kconfig option) for the motion detector to be activated again.
      #. Observe that **LED 2** blinks at a 0.25 second interval, which indicates that the motion detector is active.
      #. Double-click **Button 2** to simulate the motion event and observe the ringing action just as before.
      #. Wait for 20 seconds for the **LED 2** to go off, which indicates that the motion detector is inactive.
      #. Wait for two minutes for the motion detector to be activated again.
      #. Observe that **LED 2** blinks at a 0.25 second interval, which indicates that the motion detector is active.
      #. Turn on your Android device.
      #. Observe that after up to few hours the **LED 2** goes off, which means that the motion detector has been deactivated, because the device is no longer in the Unwanted Tracking Protection mode.

   .. group-tab:: nRF54 DKs

      1. Enable the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_TEST_MODE` Kconfig option in the sample configuration to shorten the motion detector activation periods.
         Refer to the :ref:`fast_pair_locator_tag_motion_detector_test_build` section for details on how to build the sample in the motion detector test mode.
      #. Build and flash the application in the motion detector test mode on your target device.
      #. Go to the :ref:`fast_pair_locator_tag_testing` section and follow the instructions on performing the FMDN provisioning operation.
      #. Observe that **LED 2** is lit, which indicates that the device is provisioned as an FMDN beacon.
      #. Double-click **Button 1** to simulate the motion event.
      #. Observe that **LED 1** blinks fast twice, which indicates that the motion detected event appears.
      #. Observe that the ringing action does not start, because the motion detector is inactive.
      #. Put the device into the Unwanted Tracking Protection mode.
         You can do it in two ways.
         If you have access to the "Inspect Spot device" tool in the Find My Device Android app, you can:

         1. Open the Find My Device app.
         #. Tap your accessory in the list.
            The accessory should have "Nearby" written under its name.
         #. Tap the gear icon next to the device name
         #. Tap the 3-dot menu and then :guilabel:`Inspect Spot device (internal)`.
         #. Tap the 3-dot menu and then :guilabel:`Activate Unwanted Tracking Mode`.
         #. Observe the "Changed unwanted tracking mode successfully" message on the screen.
         #. Turn off your Android device to prevent it from automatically deactivating the mode.
            Do this as quickly as possible after activating the mode.

         Otherwise, you can:

         1. Turn off your Android device.
         #. Wait for minimum 30 minutes for other Android device to put the device into the Unwanted Tracking Protection mode.
            The other Android device can belong to anyone around you.
            It is recommended to be in a crowded area.

      #. Wait for three minutes (random value between the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MIN` and :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MAX` Kconfig options) for the motion detector to be activated.
      #. Observe that **LED 1** blinks at a 0.25 second interval, which indicates that the motion detector is active.
      #. Double-click **Button 1** to simulate the motion event.
      #. Observe that **LED 1** blinks fast twice, which indicates that the motion detected event appears.
      #. Observe that after up to 10 seconds, the ringing action starts for one second, which is indicated by **LED 1** being lit.
      #. Observe that **LED 1** goes back to blinking at a 0.25 second interval, which indicates that the motion detector is active.
      #. Double-click **Button 1** to simulate the motion event.
      #. Observe that **LED 1** blinks fast twice, which indicates that the motion detected event appears.
      #. Observe that after up to 0.5 second, the ringing action starts for one second, which is indicated by **LED 1** being lit.
      #. Observe that **LED 1** goes back to blinking at a 0.25 second interval, which indicates that the motion detector is active.
      #. Double-click **Button 1** to simulate the motion event.
      #. Observe that **LED 1** blinks fast twice, which indicates that the motion detected event appears.
      #. Observe that after up to 0.5 second, the ringing action starts, which is indicated by **LED 1** being lit.
      #. Double-click **Button 1** to simulate the motion event while the **LED 1** is still lit (ringing action is still in progress).
      #. Observe that **LED 1** blinks fast twice, which indicates that the motion detected event appears.
      #. Observe that after 0.5 second, of not ringing, the ringing action starts again for one second, which is indicated by **LED 1** being lit.
      #. Observe that **LED 1** goes back to blinking at a 0.25 second interval, which indicates that the motion detector is active.
      #. Observe that after 20 seconds from the first motion event or after 10 ringing actions completes, the **LED 1** is off, which indicates that the motion detector is inactive.
      #. Double-click **Button 1** to simulate the motion event.
      #. Observe that **LED 1** blinks fast twice, which indicates that the motion detected event appears.
      #. Observe that the ringing action does not start, because the motion detector is inactive.
      #. Wait for two minutes (the backoff period defined by the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_BACKOFF_PERIOD` Kconfig option) for the motion detector to be activated again.
      #. Observe that **LED 1** blinks at a 0.25 second interval, which indicates that the motion detector is active.
      #. Double-click **Button 1** to simulate the motion event and observe the ringing action just as before.
      #. Wait for 20 seconds for the **LED 1** to go off, which indicates that the motion detector is inactive.
      #. Wait for two minutes for the motion detector to be activated again.
      #. Observe that **LED 1** blinks at a 0.25 second interval, which indicates that the motion detector is active.
      #. Turn on your Android device.
      #. Observe that after up to few hours the **LED 1** goes off, which means that the motion detector has been deactivated, because the device is no longer in the Unwanted Tracking Protection mode.

.. _fast_pair_locator_tag_testing_dfu:

Performing the DFU procedure
----------------------------

Testing steps for the DFU feature require the `nRF Connect Device Manager`_ application on your test Android device.

To perform the DFU procedure, complete the following steps:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. Observe that **LED 1** is blinking at a 1 second interval, which indicates that the DFU mode is disabled.
      #. Press the **Button 4** for 7 seconds or more to enter the DFU mode.
      #. Observe that **LED 1** is blinking at a 0.25 second interval, which indicates that the DFU mode is enabled.
      #. Observe that **LED 3** is blinking, which indicates that the Fast Pair advertising is enabled.
      #. Perform DFU using the `nRF Connect Device Manager`_ mobile app:

         .. include:: /app_dev/device_guides/nrf52/fota_update.rst
            :start-after: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start
            :end-before: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

   .. group-tab:: nRF54L DKs

      1. Observe that **LED 0** is blinking at a 1 second interval, which indicates that the DFU mode is disabled.
      #. Press the **Button 3** for 7 seconds or more to enter the DFU mode.
      #. Observe that **LED 0** is blinking at a 0.25 second interval, which indicates that the DFU mode is enabled.
      #. Observe that **LED 2** is blinking, which indicates that the Fast Pair advertising is enabled.
      #. Perform DFU using the `nRF Connect Device Manager`_ mobile app:

         .. include:: /app_dev/device_guides/nrf52/fota_update.rst
            :start-after: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start
            :end-before: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

   .. group-tab:: nRF54H DKs

      1. Observe that **LED 0** is blinking at a 1 second interval, which indicates that the DFU mode is disabled.
      #. Press the **Button 3** for 7 seconds or more to enter the DFU mode.
      #. Observe that **LED 0** is blinking at a 0.25 second interval, which indicates that the DFU mode is enabled.
      #. Observe that **LED 2** is blinking, which indicates that the Fast Pair advertising is enabled.
      #. Perform DFU using the `nRF Connect Device Manager`_ mobile app:

         .. include:: /includes/suit_fota_update_nrfcdm_test_steps.txt

.. _fast_pair_locator_tag_testing_fw_update_notifications:

Android notifications about firmware updates
--------------------------------------------

Testing steps for the firmware update notification feature require the `nRF Connect Device Manager`_ application on your test Android device.

.. note::
   The support for the Android notifications about the firmware updates in the context of the FMDN extension is a new feature.
   Before you start the testing, ensure that versions for the following applications and services are equal to or greater than the specified minimum versions:

   * `Google Play Services`_ - ``v25.12.33``
   * `nRF Connect Device Manager`_ - ``v2.5.0``

   Also, ensure that Android notifications are enabled for the `nRF Connect Device Manager`_ application.

.. note::
   You can execute these testing steps in combination with the :ref:`fast_pair_locator_tag_testing_clock_sync` testing steps, as both test variants require you to wait for more than 24 hours.

To test this feature, complete the following steps:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. Go to the :ref:`fast_pair_locator_tag_testing` section and follow the instructions on performing the FMDN provisioning operation.
      #. Observe that **LED 3** is lit, which indicates that the device is provisioned as an FMDN beacon.
      #. Power off the development kit and wait for 24 hours.
      #. Power off your test Android device.
      #. Power on the development kit.
      #. |connect_terminal_specific|
      #. Power on your test Android device and unlock it after the smartphone screen is turned on.
      #. Wait for the device to automatically connect to the development kit and read the local firmware version in the background.
      #. In the terminal, verify that a message confirming that the firmware version is being read appears::

            DIS Firmware Revision characteristic is being read

      #. Observe that you get the Android notification about the new firmware update on your test Android device.

         .. note::
            Notification delivery on Android may be delayed due to system conditions or background processing.
            It is recommended to wait up to five minutes before concluding that this test step has failed.

         .. figure:: /images/bt_fast_pair_locator_tag_android_fw_update_notification.png
            :scale: 80 %
            :alt: Firmware update notification in the Android notification center

      #. Upgrade the sample firmware to the latest version:

         a. Set the application firmware version to ``v99.99.99`` by modifying the following files:

            * The :file:`VERSION` file.
            * The :file:`configuration/prj.conf` file (the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MAJOR`, :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MINOR`, and :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_REVISION` Kconfig option group).

            The new firmware version should match the version specified in the **Firmware Version** field from the :ref:`fast_pair_locator_tag_google_device_model` section.

         #. Rebuild the sample application to generate the DFU package.
         #. Follow the instructions from the :ref:`fast_pair_locator_tag_testing_dfu` section to perform the DFU procedure.

      #. Validate the new firmware version is correct by reading the application boot banner.
      #. Power off the development kit and wait again for 24 hours.
      #. Power off your test Android device.
      #. Power on the development kit.
      #. |connect_terminal_specific|
      #. Power on your test Android device and unlock it after the smartphone screen is turned on.
      #. Wait for the device to automatically connect to the development kit and read the local firmware version in the background.
      #. In the terminal, verify that a message confirming that the firmware version is being read appears.
      #. Observe that you no longer get the Android notification about the new firmware update on your test Android device.

   .. group-tab:: nRF54 DKs

      1. Go to the :ref:`fast_pair_locator_tag_testing` section and follow the instructions on performing the FMDN provisioning operation.
      #. Observe that **LED 2** is lit, which indicates that the device is provisioned as an FMDN beacon.
      #. Power off the development kit and wait for 24 hours.
      #. Power off your test Android device.
      #. Power on the development kit.
      #. |connect_terminal_specific|
      #. Power on your test Android device and unlock it after the smartphone screen is turned on.
      #. Wait for the device to automatically connect to the development kit and read the local firmware version in the background.
      #. In the terminal, verify that a message confirming that the firmware version is being read appears::

            DIS Firmware Revision characteristic is being read

      #. Observe that you get the Android notification about the new firmware update on your test Android device.

         .. note::
            Notification delivery on Android may be delayed due to system conditions or background processing.
            It is recommended to wait up to five minutes before concluding that this test step has failed.

         .. figure:: /images/bt_fast_pair_locator_tag_android_fw_update_notification.png
            :scale: 80 %
            :alt: Firmware update notification in the Android notification center

      #. Upgrade the sample firmware to the latest version:

         a. Set the application firmware version to ``v99.99.99`` by modifying the following files:

            * The :file:`VERSION` file.
            * The :file:`configuration/prj.conf` file (the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MAJOR`, :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MINOR`, and :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_REVISION` Kconfig option group).

            The new firmware version should match the version specified in the **Firmware Version** field from the :ref:`fast_pair_locator_tag_google_device_model` section.

         #. Rebuild the sample application to generate the DFU package.
         #. Follow the instructions from the :ref:`fast_pair_locator_tag_testing_dfu` section to perform the DFU procedure.

      #. Validate the new firmware version is correct by reading the application boot banner.
      #. Power off the development kit and wait again for 24 hours.
      #. Power off your test Android device.
      #. Power on the development kit.
      #. |connect_terminal_specific|
      #. Power on your test Android device and unlock it after the smartphone screen is turned on.
      #. Wait for the device to automatically connect to the development kit and read the local firmware version in the background.
      #. In the terminal, verify that a message confirming that the firmware version is being read appears.
      #. Observe that you no longer get the Android notification about the new firmware update on your test Android device.

Disabling the locator tag
-------------------------

The following instructions on disabling the locator tag apply to the default debug device model registered for this sample (see the :ref:`fast_pair_locator_tag_google_device_model` section), and are used for demonstration purposes.

To disable the locator tag device, complete the following steps:

1. Find the power switch on the device.
#. Turn off the locator tag by sliding the switch to the off position.
#. Observe that the on-device LEDs are turned off.

.. note::
   Vendor-specific instructions on disabling the unknown trackers are registered on mobile platforms, like Android and iOS, for each device model.

Fast Pair Validator app
-----------------------

You can test the sample against the Eddystone test suite from the `Fast Pair Validator app`_.

.. note::
   To start testing the FMDN solution with the `Fast Pair Validator app`_, use your project in the Google Nearby Console and the Fast Pair device model that is defined in the scope of your project.
   The default debug device model from Nordic Semiconductor cannot be used for this purpose.
   Additionally, you must sign into the `Fast Pair Validator app`_ using an email address associated with your project in the Google Nearby Console.

|test_sample|

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. Open the Fast Pair Validator app on your Android device.
      #. In the app list, tap on the target device model that matches your flashed firmware.
      #. Select the correct test category by tapping the :guilabel:`EDDYSTONE` button.

         .. figure:: /images/bt_fast_pair_fpv_app_test_category.png
            :scale: 80 %
            :alt: Test category selection in the Fast Pair Validator app

      #. In the Eddystone test view, choose a test case by tapping the :guilabel:`START` button next to it.

         .. figure:: /images/bt_fast_pair_fpv_app_eddystone_category.png
            :scale: 80 %
            :alt: Eddystone test suite in the Fast Pair Validator app

      #. In the chosen test view, run the test case by tapping the :guilabel:`TEST` button.
      #. Follow the pop-up instructions during the test execution.

         .. important::
            Use your locator tag device button interface to comply with the instructions during the test execution:

            * *Please put the headset into pairing mode* - Ensure that your device advertises in the Fast Pair discoverable mode or switch to this mode by pressing **Button 1**.
            * *After you close this dialog, as soon as you hear device ringing, press a button on the device to stop ringing* - Stop the ringing action by pressing **Button 2** once the **LED 2** is lit.
            * *Before you close this dialog, press the button on the device to allow reading its identifier* - Enter the identification mode by shortly pressing **Button 4**.
              The **LED 4** should start blinking at half-second intervals after the button press.

      #. Wait until the test case execution completes.

         .. figure:: /images/bt_fast_pair_fpv_app_eddystone_provisioning_test.png
            :scale: 80 %
            :alt: Successful execution of the provisioning test case in the Fast Pair Validator app

      #. Exit the test case view by tapping on the cross icon in the upper left corner.
      #. Start another test case or exit the Fast Pair Validator app.

         .. note::
            Each test case is concluded with the unprovisioning operation that disables the Fast Pair advertising in the discoverable mode.
            To set your locator tag device back into the pairing mode, press **Button 1**.

   .. group-tab:: nRF54 DKs

      1. Open the Fast Pair Validator app on your Android device.
      #. In the app list, tap on the target device model that matches your flashed firmware.
      #. Select the correct test category by tapping the :guilabel:`EDDYSTONE` button.

         .. figure:: /images/bt_fast_pair_fpv_app_test_category.png
            :scale: 80 %
            :alt: Test category selection in the Fast Pair Validator app

      #. In the Eddystone test view, choose a test case by tapping the :guilabel:`START` button next to it.

         .. figure:: /images/bt_fast_pair_fpv_app_eddystone_category.png
            :scale: 80 %
            :alt: Eddystone test suite in the Fast Pair Validator app

      #. In the chosen test view, run the test case by tapping the :guilabel:`TEST` button.
      #. Follow the pop-up instructions during the test execution.

         .. important::
            Use your locator tag device button interface to comply with the instructions during the test execution:

            * *Please put the headset into pairing mode* - Ensure that your device advertises in the Fast Pair discoverable mode or switch to this mode by pressing **Button 0**.
            * *After you close this dialog, as soon as you hear device ringing, press a button on the device to stop ringing* - Stop the ringing action by pressing **Button 1** once the **LED 1** is lit.
            * *Before you close this dialog, press the button on the device to allow reading its identifier* - Enter the identification mode by shortly pressing **Button 3**.
              The **LED 3** should start blinking at half-second intervals after the button press.

      #. Wait until the test case execution completes.

         .. figure:: /images/bt_fast_pair_fpv_app_eddystone_provisioning_test.png
            :scale: 80 %
            :alt: Successful execution of the provisioning test case in the Fast Pair Validator app

      #. Exit the test case view by tapping on the cross icon in the upper left corner.
      #. Start another test case or exit the Fast Pair Validator app.

         .. note::
            Each test case is concluded with the unprovisioning operation that disables the Fast Pair advertising in the discoverable mode.
            To set your locator tag device back into the pairing mode, press **Button 0**.


Dependencies
************

The sample uses subsystems and firmware components available in the |NCS|.
For details, see the following sections.

Fast Pair GATT Service
======================

This sample uses the :ref:`bt_fast_pair_readme` and its dependencies and is configured to meet the requirements of the Fast Pair standard together with its FMDN extension.
For details about integrating Fast Pair in the |NCS|, see :ref:`ug_bt_fast_pair`.

By default, this sample sets the ``SB_CONFIG_BT_FAST_PAIR_MODEL_ID`` and ``SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY`` Kconfig options to use the Nordic device model that is intended for demonstration purposes.
With these options set, the build system calls the :ref:`bt_fast_pair_provision_script` that automatically generates a hexadecimal file containing Fast Pair Model ID and the Anti-Spoofing Private Key.
For more details about enabling Fast Pair for your application, see the :ref:`ug_bt_fast_pair_prerequisite_ops_kconfig` section in the Fast Pair integration guide.

Bluetooth LE advertising data providers
=======================================

The :ref:`bt_le_adv_prov_readme` are used to generate Bluetooth advertising and scan response data for the Fast Pair advertising set.
The sample uses the following providers to generate the advertising packet payload:

* Advertising flags provider (:kconfig:option:`CONFIG_BT_ADV_PROV_FLAGS`)
* TX power provider (:kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER`)
* Google Fast Pair provider (:kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR`)

The sample uses the Bluetooth device name provider (:kconfig:option:`CONFIG_BT_ADV_PROV_DEVICE_NAME`) provider to generate the scan response data.

When the DFU functionality is enabled (:ref:`CONFIG_APP_DFU <CONFIG_APP_DFU>`), the sample uses the SMP GATT Service UUID provider.
The SMP UUID is placed in the advertising data when Fast Pair advertising is in the discoverable mode, or in the scan response data when it is in the not discoverable mode.

Device Firmware Update (DFU)
============================

This sample uses the following components for the DFU functionality:

* :ref:`MCUboot bootloader <mcuboot:mcuboot_ncs>` (for supported board targets)
* :ref:`zephyr:app-version-details`
* :ref:`zephyr:mcu_mgr`
