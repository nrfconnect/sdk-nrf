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

.. note::
   The software maturity level for the locator tag use case is listed in the :ref:`software_maturity_fast_pair_use_case` table.

This sample follows the `Fast Pair Device Feature Requirements for Locator Tags`_ documentation and uses the Fast Pair configuration for the locator tag use case.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

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

FMDN unprovisioning
===================

You can remove your device from the `Find My Device app`_ in a symmetrical operation, called unprovisioning.
Once unprovisioned, the accessory stops broadcasting the FMDN advertising payload.

During the unprovisioning, the device link with the owner's Google account is removed.
To bring the accessory back to its initial state and make provisioning available for the next owner, you need to perform a factory reset.

After the factory reset operation, the Fast Pair discoverable advertising gets automatically disabled to prevent a restart of the Fast Pair procedure.
You need to press a button to set your accessory in the Fast Pair discoverable advertising mode and make it available for the next FMDN provisioning.

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
   * Anti-Spoofing Private Key (base64, uncompressed): ``rie10A7ONqwd77VmkxGsblPUbMt384qjDgcEJ/ctT9Y=``
   * Device Type: Locator Tag
   * Notification Type: Fast Pair
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

Development kits
================

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Keeps blinking at constant intervals to indicate that firmware is running.

      LED 2:
         Indicates that the ringing action is in progress.
         Depending on the ringing state:

         * Lit if the device is ringing.
         * Off if the device is not ringing.

      LED 3:
         Depending on the FMDN provisioning state:

         * Lit if the device is provisioned.
         * Off if the device is not provisioned.

      LED 4:
         Depending on the states of the recovery mode and the identification mode:

         * Lit if both modes are active on the device.
         * Off if both modes are inactive on the device.
         * Blinks at a 0.5 second interval if the identification mode is active on the device and the recovery mode is inactive on the device.
         * Blinks at a 1 second interval if the recovery mode is active on the device and the identification mode is inactive on the device.

      Button 1:
         Toggles between different modes of the Fast Pair advertising set:

         * Without an Account Key stored on the device:

           * Fast Pair advertising disabled.
           * Fast Pair discoverable advertising (the default after the system bootup).

         * With an Account Key stored on the device:

           * Fast Pair advertising disabled.
           * Fast Pair not discoverable advertising (the default after the system bootup).

         .. note::

            The Bluetooth advertising is active only until the Fast Pair Provider connects to a Bluetooth Central.
            After the connection, you can still switch the advertising modes, but the switch will come into effect only after disconnection.

            The sample automatically switches to the following Fast Pair advertising modes under certain conditions:

            * Fast Pair not discoverable advertising - On the Account Key write operation.
            * Fast Pair advertising disabled:

               * Right before the factory reset operation.
                 Press **Button 1** to set the device to the Fast Pair discoverable advertising mode after the unprovisioning operation.

               * After the beacon clock synchronization.

      Button 2:
         Stops the ongoing ringing action.

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
         A short press requests the FMDN subsystem to enable the identification mode for five minutes.
         This timeout value is defined by the :kconfig:option:`CONFIG_DULT_ID_READ_STATE_TIMEOUT` Kconfig option according to the DULT specification requirements.

         A long press (>3 s) enables the recovery mode for one minute as defined by the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY_TIMEOUT` Kconfig option.

         When pressed during the application bootup, resets the accessory to default factory settings.


   .. group-tab:: nRF54 DKs

      LED 0:
         Keeps blinking at constant intervals to indicate that firmware is running.

      LED 1:
         Indicates that the ringing action is in progress.
         Depending on the ringing state:

         * Lit if the device is ringing.
         * Off if the device is not ringing.

      LED 2:
         Depending on the FMDN provisioning state:

         * Lit if the device is provisioned.
         * Off if the device is not provisioned.

      LED 3:
         Depending on the states of the recovery mode and the identification mode:

         * Lit if both modes are active on the device.
         * Off if both modes are inactive on the device.
         * Blinks at a 0.5 second interval if the identification mode is active on the device and the recovery mode is inactive on the device.
         * Blinks at a 1 second interval if the recovery mode is active on the device and the identification mode is inactive on the device.

      Button 0:
         Toggles between different modes of the Fast Pair advertising set:

         * Without an Account Key stored on the device:

           * Fast Pair advertising disabled.
           * Fast Pair discoverable advertising (the default after the system bootup).

         * With an Account Key stored on the device:

           * Fast Pair advertising disabled.
           * Fast Pair not discoverable advertising (the default after the system bootup).

         .. note::

            The Bluetooth advertising is active only until the Fast Pair Provider connects to a Bluetooth Central.
            After the connection, you can still switch the advertising modes, but the switch will come into effect only after disconnection.

            The sample automatically switches to the following Fast Pair advertising modes under certain conditions:

            * Fast Pair not discoverable advertising - On the Account Key write operation.
            * Fast Pair advertising disabled:

              * Right before the factory reset operation.
                Press **Button 0** to set the device to the Fast Pair discoverable advertising mode after the unprovisioning operation.

              * After the beacon clock synchronization.

      Button 1:
         Stops the ongoing ringing action.

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
         A short press requests the FMDN subsystem to enable the identification mode for five minutes.
         This timeout value is defined by the :kconfig:option:`CONFIG_DULT_ID_READ_STATE_TIMEOUT` Kconfig option according to the DULT specification requirements.

         A long press (>3 s) enables the recovery mode for one minute as defined by the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY_TIMEOUT` Kconfig option.

         When pressed during the application bootup, resets the accessory to default factory settings.

Thingy:53
=========

RGB LED:
   Thingy:53 displays the application state in the RGB scale using **LED1**.
   The **LED1** displays a color sequence with each blink that indicates the overall application state.

   Each color of the LED indicates the different application state:

   * Green - Indicates that firmware is running.
   * Blue - Indicates that the device is provisioned.
   * Yellow - Indicates that the identification mode is active.
   * Red - Indicates that the recovery mode is active.
   * White - Indicates that the Fast Pair advertising is active.

Speaker/Buzzer:
   Produces sound when the ringing action is in progress and to indicate a new button action.

Button (SW3):
   When pressed during the application bootup, the accessory is reset to its default factory settings.

   When pressed, stops the ongoing ringing action.

   When released, the action depends on how long the button was held:

   * From 1 to 3 seconds (notified by one short beep):

     Toggles between different modes of the Fast Pair advertising set:

     * Without an Account Key stored on the device:

       * Fast Pair advertising disabled.
       * Fast Pair discoverable advertising (the default after the system bootup).

     * With an Account Key stored on the device:

       * Fast Pair advertising disabled.
       * Fast Pair not discoverable advertising (the default after the system bootup).

     .. note::
        The Bluetooth advertising is active only until the Fast Pair Provider connects to a Bluetooth Central.
        After the connection, you can still switch the advertising modes, but the effect is only after disconnection.

        The sample automatically switches to the following Fast Pair advertising modes under certain conditions:

        * Fast Pair not discoverable advertising - On the Account Key write operation.
        * Fast Pair advertising disabled:

          * Right before the factory reset operation.
          * After the beacon clock synchronization.

   * From 3 to 5 seconds (notified by two short beeps):

     Requests the FMDN subsystem to enable the identification mode for five minutes.
     This timeout value is defined by the :kconfig:option:`CONFIG_DULT_ID_READ_STATE_TIMEOUT` Kconfig option according to the DULT specification requirements.

   * From 5 to 7 seconds (notified by three short beeps):

     Enables the recovery mode for one minute as defined by the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY_TIMEOUT` Kconfig option.

   * From 7 seconds or more (notified by one longer beep):

     No action - allows to not perform any button operation.

Configuration
*************

|config|

Configuration options
=====================

All Kconfig options are specific to the chosen platform type and are listed in their dedicated subsections.

Development kits
----------------

Check and configure the following Kconfig options for your development kit board target:

.. _CONFIG_APP_BATTERY_LEVEL_DECREMENT:

CONFIG_APP_BATTERY_LEVEL_DECREMENT
   The sample configuration defines the decrement value used to simulate the battery level on the development kit.
   The battery level is defined in percentages and ranges from 0% to 100%.
   By default, the decrement level is set to 10%.

Thingy:53
---------

Check and configure the following Kconfig options for your Thingy:53 board target:

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

When building the sample, you can provide the Fast Pair Model ID (``FP_MODEL_ID``) and the Fast Pair Anti-Spoofing Key (``FP_ANTI_SPOOFING_KEY``) as CMake options.
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

.. note::
   The sample does not support the DFU functionality.

The release build reduces the code size and RAM usage of the sample by disabling logging functionality and performing other optimizations.
Additionally, it enables the Link Time Optimization (LTO) configuration through the :kconfig:option:`CONFIG_LTO` Kconfig option, which further reduces the code size.
LTO is an advanced compilation technique that optimizes across all compiled units of an application at the link stage, rather than optimizing each unit separately.

.. note::
   Support for the LTO is experimental.

See :ref:`cmake_options` for detailed instructions on how to add the ``FILE_SUFFIX=release`` option to your build.
For example, when building from the command line, you can add it as follows:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DFILE_SUFFIX=release

.. _fast_pair_locator_tag_testing:

Testing
=======

.. note::
   Images in the testing section are generated for the debug device model registered by Nordic Semiconductor in the Google Nearby Console (see :ref:`fast_pair_locator_tag_google_device_model`).
   The debug device model name is covered by asterisks and the default Fast Pair logo is displayed instead of the one specified during the device model registration.

   If the test Android device uses a primary email account that is not on Google's email allow list for the FMDN feature, testing steps will fail at the FMDN provisioning stage for the default debug (uncertified) device model.
   To be able to test with debug device models, register your development email account by completing Google's device proposal form.
   You can find the link to the device proposal form in the `Fast Pair Find My Device Network extension`_ specification.

|test_sample|

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_terminal_specific|
      #. Reset the development kit.
      #. Observe that **LED 1** is blinking to indicate that firmware is running.
      #. Observe that **LED 3** is off, which indicates that the device is not provisioned as an FMDN beacon.
         The Fast Pair Provider is advertising in the Fast Pair discoverable mode and is ready for the FMDN provisioning.
      #. Move the Android device close to your locator tag device.
      #. Wait for the notification from your Android device about the detected Fast Pair Provider.

         .. figure:: /images/bt_fast_pair_locator_tag_discoverable_notification.png
            :scale: 80 %
            :alt: Fast Pair discoverable notification with support for the `Find My Device app`_

      #. Initiate the connection and trigger the Fast Pair procedure by tapping the :guilabel:`Connect` button.
         After the procedure is complete, you will see a pop-up with the Acceptable Use Policy for the `Find My Device app`_.
      #. If you want to start the FMDN provisioning, accept the Acceptable Use Policy by tapping the :guilabel:`Agree and continue` button.

         .. figure:: /images/bt_fast_pair_locator_tag_acceptable_use_policy.png
            :scale: 80 %
            :alt: The Find My Device Acceptable Use Policy

      #. Wait for the Android device's notification that indicates completion of the provisioning process.

         .. figure:: /images/bt_fast_pair_locator_tag_provisioning_complete.png
            :scale: 80 %
            :alt: Notification about provisioning completion for the `Find My Device app`_

      #. Observe that **LED 3** is lit, which indicates that the device is provisioned as an FMDN beacon.
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
      #. Observe that **LED 3** is off, which indicates that the device is no longer provisioned as an FMDN beacon.
      #. Observe that the Android does not display a notification about the detected Fast Pair Provider, as the locator tag device disables advertising after the unprovisioning operation.
      #. Press **Button 1** if you want to start the Fast Pair discoverable advertising and restart the FMDN provisioning process.

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_specific|
      #. Reset the development kit.
      #. Observe that **LED 0** is blinking to indicate that firmware is running.
      #. Observe that **LED 2** is off, which indicates that the device is not provisioned as an FMDN beacon.
         The Fast Pair Provider is advertising in the Fast Pair discoverable mode and is ready for the FMDN provisioning.
      #. Move the Android device close to your locator tag device.
      #. Wait for the notification from your Android device about the detected Fast Pair Provider.

         .. figure:: /images/bt_fast_pair_locator_tag_discoverable_notification.png
            :scale: 80 %
            :alt: Fast Pair discoverable notification with support for the `Find My Device app`_

      #. Initiate the connection and trigger the Fast Pair procedure by tapping the :guilabel:`Connect` button.
         After the procedure is complete, you will see a pop-up with the Acceptable Use Policy for the `Find My Device app`_.
      #. If you want to start the FMDN provisioning, accept the Acceptable Use Policy by tapping the :guilabel:`Agree and continue` button.

         .. figure:: /images/bt_fast_pair_locator_tag_acceptable_use_policy.png
            :scale: 80 %
            :alt: The Find My Device Acceptable Use Policy

      #. Wait for the Android device's notification that indicates completion of the provisioning process.

         .. figure:: /images/bt_fast_pair_locator_tag_provisioning_complete.png
            :scale: 80 %
            :alt: Notification about provisioning completion for the `Find My Device app`_

      #. Observe that **LED 2** is lit, which indicates that the device is provisioned as an FMDN beacon.
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
      #. Observe that **LED 2** is off, which indicates that the device is no longer provisioned as an FMDN beacon.
      #. Observe that the Android does not display a notification about the detected Fast Pair Provider, as the locator tag device disables advertising after the unprovisioning operation.
      #. Press **Button 0** if you want to start the Fast Pair discoverable advertising and restart the FMDN provisioning process.


Clock synchronization
---------------------

Testing steps for the clock synchronization feature require a second Android device.
The device must be registered to a **different** Google account from the first Android device.

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

This sample enables the ``SB_CONFIG_BT_FAST_PAIR`` Kconfig option.
With this option enabled, the build system calls the :ref:`bt_fast_pair_provision_script`, which automatically generates a hexadecimal file containing Fast Pair Model ID and Anti Spoofing Private Key.
For more details about enabling Fast Pair for your application, see the :ref:`ug_bt_fast_pair_prerequisite_ops_kconfig` section in the Fast Pair integration guide.

Bluetooth LE advertising data providers
=======================================

The :ref:`bt_le_adv_prov_readme` are used to generate Bluetooth advertising and scan response data for the Fast Pair advertising set.
The sample uses the following providers to generate the advertising packet payload:

* Advertising flags provider (:kconfig:option:`CONFIG_BT_ADV_PROV_FLAGS`)
* TX power provider (:kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER`)
* Google Fast Pair provider (:kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR`)

The sample uses the Bluetooth device name provider (:kconfig:option:`CONFIG_BT_ADV_PROV_DEVICE_NAME`) provider to generate the scan response data.
