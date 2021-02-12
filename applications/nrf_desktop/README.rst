.. _nrf_desktop:

nRF Desktop
###########

.. contents::
   :local:
   :depth: 2

The nRF Desktop is a reference design of a Human Interface Device (HID) that is connected to a host through Bluetooth LE or USB, or both.
Depending on the configuration, this application can work as desktop mouse, gaming mouse, keyboard, or connection dongle.

.. tip::
    To get started with hardware that has pre-configured software, go to the `User interface`_ section.

The nRF Desktop application supports common input hardware interfaces like motion sensors, rotation sensors, and buttons scanning module.
You can configure the firmware at runtime using a dedicated configuration channel established with the HID feature report.
The same channel is used to transmit DFU packets.

.. _nrf_desktop_architecture:

Overview: Firmware architecture
*******************************

The nRF Desktop application design aims at high performance, while still providing configurability and extensibility.

The application architecture is modular and event-driven.
This means that parts of the application functionality are separated into isolated modules that communicate with each other using application events, which are handled by the :ref:`event_manager`.
Modules register themselves as listeners of those events that they are configured to react to.
An application event can be submitted by multiple modules and it can have multiple listeners.

Module and component overview
=============================

The following figure shows the nRF Desktop modules and how they relate with other components and the :ref:`event_manager`.
The figure does not present all the available modules.
For example, the figure does not include the modules that are used as hotfixes or only for debug or profiling purposes.

.. figure:: /images/nrf_desktop_arch.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (overview)

   Application high-level design overview (click to enlarge)

For more information about each of nRF Desktop modules, see the :ref:`nrf_desktop_app_internal_modules` section.

Module event tables
-------------------

The :ref:`documentation page of each application module <nrf_desktop_app_internal_modules>` includes a table that shows the event-based communication for the module.

+---------------------------------------+-------------+------------------------------------+--------------+-----------------------------------------+
| Source Module                         | Input Event | This Module                        | Output Event | Sink Module                             |
+=======================================+=============+====================================+==============+=========================================+
| The module that submits ``eventA``.   | ``eventA``  | The module described by the table. |              |                                         |
+---------------------------------------+             |                                    |              |                                         |
| Other module that submits ``eventA``. |             |                                    |              |                                         |
+---------------------------------------+-------------+                                    +--------------+-----------------------------------------+
| The module that submits ``eventB``.   | ``eventB``  |                                    |              |                                         |
+---------------------------------------+-------------+                                    +--------------+-----------------------------------------+
|                                       |             |                                    | ``eventC``   | The module that reacts on ``eventC``.   |
+---------------------------------------+-------------+------------------------------------+--------------+-----------------------------------------+

Each module event table contains the following columns:

Source Module
   The module that submits a given application event.
   Some of these events can have many listeners or sources.
   These are listed on the :ref:`nrf_desktop_event_rel_modules` page.

Input Event
   An application event that is received by the module described by the table.

This Module
   The module described by the table.
   This is the module that is the target of the Input Events and the source of Output Events directed to the Sink Modules.

Output Event
   An application event that is submitted by the module described by the table.

Sink Module
   The module that reacts on the application event.
   Some of these events can have many listeners or sources.
   These are listed on the :ref:`nrf_desktop_event_rel_modules` page.

.. note::
   Some application modules can have multiple implementations (for example, :ref:`nrf_desktop_motion`).
   In such case, the table shows the :ref:`event_manager` events received and submitted by all implementations of a given application module.

Module usage per hardware type
==============================

Since the application architecture is uniform and the code is shared, the set of modules in use depends on the selected device role.
A different set of modules is enabled when the application is working as mouse, keyboard, or dongle.
In other words, not all of the :ref:`nrf_desktop_app_internal_modules` need to be enabled for a given reference design.

Gaming mouse module set
-----------------------

The following figure shows the modules that are enabled when the application is working as a gaming mouse.

.. figure:: /images/nrf_desktop_arch_gmouse.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (gaming mouse)

   Application configured as a gaming mouse (click to enlarge)

Desktop mouse module set
------------------------

The following figure shows the modules that are enabled when the application is working as a desktop mouse.

.. figure:: /images/nrf_desktop_arch_dmouse.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (desktop mouse)

   Application configured as a desktop mouse (click to enlarge)

Keyboard module set
-------------------

The following figure shows the modules that are enabled when the application is working as a keyboard.

.. figure:: /images/nrf_desktop_arch_kbd.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (keyboard)

   Application configured as a keyboard (click to enlarge)

Dongle module set
-----------------

The following figure shows the modules that are enabled when the application is working as a dongle.

.. figure:: /images/nrf_desktop_arch_dongle.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (dongle)

   Application configured as a dongle (click to enlarge)

Thread usage
============

The application limits the number of threads in use to the minimum and does not use the user-space threads.

The following threads are kept running in the application:

* System-related threads
    * Idle thread
    * System workqueue thread
    * Logger thread (on debug :ref:`build types <nrf_desktop_requirements_build_types>`)
    * Shell thread (on :ref:`build types <nrf_desktop_requirements_build_types>` with shell enabled)
    * Threads related to Bluetooth LE (the exact number depends on the selected Link Layer)
* Application-related threads
    * Motion sensor thread (running only on mouse)
    * Settings loading thread (enabled by default only on keyboard)
    * QoS data sampling thread (running only if Bluetooth LE QoS feature is enabled)

Most of the application activity takes place in the context of the system work queue thread, either through scheduled work objects or through the event manager callbacks (executed from the system workqueue thread).
Because of this, the application does not need to handle resource protection.
The only exception are places where the interaction with interrupts or multiple threads cannot be avoided.

Memory allocation
=================

Most of memory resources that are used by the application are allocated statically.

The application uses dynamic allocation to:

* Create the event manager events.
  For more information, see the :ref:`event_manager` page.
* Temporarily store the HID-related data in the :ref:`nrf_desktop_hid_state` and :ref:`nrf_desktop_hid_forward`.
  For more information, see the documentation pages of these modules.

When configuring HEAP, make sure that the values for the following options match the typical event size and the system needs:

* :option:`CONFIG_HEAP_MEM_POOL_SIZE` - The size must be big enough to handle the worst possible use case for the given device.

.. important::
    The nRF Desktop uses ``k_heap`` as the backend for dynamic allocation.
    This backend is used by default in Zephyr.
    For more information, refer to Zephyr's documentation about :ref:`zephyr:heap_v2`.

HID mouse data forwarding
=========================

The nRF Desktop mouse sends HID input reports to host after the host connects and subscribes for the HID reports.

The :ref:`nrf_desktop_motion` sensor sampling is synchronized with sending the HID mouse input reports to the host.

The :ref:`nrf_desktop_wheel` and :ref:`nrf_desktop_buttons` provide data to the :ref:`nrf_desktop_hid_state` when the mouse wheel is used or a button is pressed, respectively.
These inputs are not synchronized with the HID report transmission to the host.

When the mouse is constantly in use, the motion module is kept in the fetching state.
In this state, the nRF Desktop mouse forwards the data from the motion sensor to the host in the following way:

a. USB state (or Bluetooth HIDS) sends a HID mouse report to the host and submits ``hid_report_sent_event``.
#. The event triggers sampling of the motion sensor.
#. A dedicated thread is used to fetch the sample from the sensor.
#. After the sample is fetched, the thread forwards it to the :ref:`nrf_desktop_hid_state` as ``motion_event``.
#. The |hid_state| updates the HID report data, generates new HID input report, and submits it as ``hid_report_event``.
#. The HID report data is forwarded to the host either by the :ref:`nrf_desktop_usb_state` or by the :ref:`nrf_desktop_hids`.
#. USB state has precedence if USB is connected.
#. When the HID input report is sent to the host, ``hid_report_sent_event`` is submitted.
   The motion sensor sample is triggered and the sequence repeats.

If the device is connected through Bluetooth, the :ref:`nrf_desktop_hid_state` uses a pipeline that consists of two HID reports.
The pipeline is created when the first ``motion_event`` is received.
The |hid_state| submits two ``hid_report_event`` events.
When the first one is sent to the host, the motion sensor sample is triggered.

For the Bluetooth connections, submitting ``hid_report_sent_event`` is delayed by one Bluetooth connection interval.
Because of this delay, the :ref:`nrf_desktop_hids` requires pipeline of two HID reports to make sure that data will be sent on every connection event.
Such solution is necessary to achieve high report rate.

If there is no motion data for the predefined number of samples, the :ref:`nrf_desktop_motion` goes to the idle state.
This is done to reduce the power consumption.
When a motion is detected, the module switches back to the fetching state.

The following diagram shows the data exchange between the application modules.
To keep it simple, the diagram only shows data related to HID input reports that are sent after the host is connected and the HID subscriptions are enabled.

.. figure:: /images/nrf_desktop_motion_sensing.svg
   :scale: 50 %
   :alt: nRF Desktop mouse HID data sensing and transmission

   nRF Desktop mouse HID data sensing and transmission

Requirements
************

The nRF Desktop application supports several development kits related to the following hardware reference designs.
Depending on what development kit you use, you need to select the respective configuration file and :ref:`build type <nrf_desktop_requirements_build_types>`.


.. tabs::

   .. tab:: Gaming mouse

      .. figure:: /images/nrf_desktop_gaming_mouse_top_no_captions.svg
         :alt: nRF Desktop gaming mouse (top view)

      .. table-from-rows:: /includes/sample_board_rows.txt
         :header: heading
         :rows: nrf52840gmouse_nrf52840

   .. tab:: Desktop mouse

      .. figure:: /images/nrf_desktop_desktop_mouse_side_no_captions.svg
         :alt: nRF Desktop desktop mouse (side view)

      .. table-from-rows:: /includes/sample_board_rows.txt
         :header: heading
         :rows: nrf52dmouse_nrf52832, nrf52810dmouse_nrf52810

   .. tab:: Keyboard

      .. figure:: /images/nrf_desktop_keyboard_top_no_captions.svg
         :alt: nRF Desktop keyboard (top view)

      .. table-from-rows:: /includes/sample_board_rows.txt
         :header: heading
         :rows: nrf52kbd_nrf52832

   .. tab:: HID dongle

      .. figure:: /images/nrf_desktop_dongle_no_captions.svg
         :alt: nRF Desktop dongle (top view)

      .. table-from-rows:: /includes/sample_board_rows.txt
         :header: heading
         :rows: nrf52840dongle_nrf52840, nrf52833dongle_nrf52833, nrf52820dongle_nrf52820

   .. tab:: DK

      .. figure:: /images/nrf_desktop_nrf52840_dk_no_captions.svg
         :alt: DK

      .. table-from-rows:: /includes/sample_board_rows.txt
         :header: heading
         :rows: nrf52840dk_nrf52840, nrf52833dk_nrf52833

      In nRF52840 DK, the application is configured to work as gaming mouse (with motion emulated by using DK buttons) and in nRF52833 DK, the application is configured to work as HID dongle.

..

The application is designed to allow easy porting to new hardware.
Check :ref:`nrf_desktop_porting_guide` for details.

.. _nrf_desktop_requirements_build_types:

nRF Desktop build types
=======================

nRF Desktop does not use a single :file:`prj.conf` file.
Configuration files are provided for different build types for each supported board.

.. include:: /gs_modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

.. note::
    `Selecting a build type`_ is optional.
    The ``ZDebug`` build type is used by default in nRF Desktop if no build type is explicitly selected.

The following build types are available for various boards in nRF Desktop:

* ``ZRelease`` -- Release version of the application with no debugging features.
* ``ZReleaseB0`` -- ``ZRelease`` build type with the support for the B0 bootloader enabled (for :ref:`background DFU <nrf_desktop_bootloader_background_dfu>`).
* ``ZReleaseMCUBoot`` -- ``ZRelease`` build type with the support for the MCUboot bootloader enabled (for :ref:`serial recovery DFU <nrf_desktop_bootloader_serial_dfu>` or :ref:`background DFU <nrf_desktop_bootloader_background_dfu>`).
* ``ZDebug`` -- Debug version of the application; the same as the ``ZRelease`` build type, but with debug options enabled.
* ``ZDebugB0`` -- ``ZDebug`` build type with the support for the B0 bootloader enabled (for :ref:`background DFU <nrf_desktop_bootloader_background_dfu>`).
* ``ZDebugMCUBoot`` -- ``ZDebug`` build type with the support for the MCUboot bootloader enabled (for :ref:`serial recovery DFU <nrf_desktop_bootloader_serial_dfu>` or :ref:`background DFU <nrf_desktop_bootloader_background_dfu>`).
* ``ZDebugWithShell`` -- ``ZDebug`` build type with the shell enabled.

In nRF Desktop, not every development kit can support every build type mentioned above.
If the given build type is not supported on the selected DK, an error message will appear when `Building and running`_.
For example, if the ``ZDebugWithShell`` build type is not supported on the selected DK, the following notification appears:

.. code-block:: console

  Configuration file for build type ZDebugWithShell is missing.

In addition to the build types mentioned above, some boards can provide more build types, which can be used to generate an application in a specific variant.
For example, such additional configurations are used to allow generation of application with different role (such as mouse, keyboard, or dongle on a DK) or to select a different link layer (such as LLPM capable Nordic SoftDevice LL or standard Zephyr SW LL).

See :ref:`nrf_desktop_porting_guide` for detailed information about the application configuration and how to create build type files for your hardware.

User interface
**************

The nRF Desktop configuration files have a set of preprogrammed options bound to different parts of the hardware.
These options are related to the functionalities discussed in this section.

Turning devices on and off
==========================

The nRF Desktop hardware reference designs are equipped with hardware switches to turn the device on and off.
See the following figures for the exact location of these switches:

.. tabs::

   .. tab:: Gaming mouse

      .. figure:: /images/nrf_desktop_gaming_mouse_bottom.svg
         :alt: nRF Desktop gaming mouse (bottom view)

      The switch is located at the bottom of the gaming mouse, close to the optical sensor.
      The mouse uses this switch also for changing dongle and Bluetooth LE peers, as described in the `Bluetooth LE peer control`_ section.

   .. tab:: Desktop mouse

      .. figure:: /images/nrf_desktop_desktop_mouse_bottom.svg
         :alt: nRF Desktop desktop mouse (bottom view)

      The switch is located at the bottom of the desktop mouse, close to the optical sensor.

   .. tab:: Keyboard

      .. figure:: /images/nrf_desktop_keyboard_back_power.svg
         :alt: nRF Desktop keyboard (back view)

      The switch is located at the back of the keyboard.

..

Connectability
==============

The nRF Desktop devices provide user input to the host in the same way as other mice and keyboards, using connection through USB or Bluetooth LE.

The nRF Desktop devices support additional operations, like firmware upgrade or configuration change.
The support is implemented through the :ref:`nrf_desktop_config_channel`.
The host can use dedicated Python scripts to exchange the data with an nRF Desktop peripheral.
For detailed information, see :ref:`nrf_desktop_config_channel_script`.

To save power, the behavior of a device can change in time.
For more information, see the `Power management`_ section.

.. _nrf_desktop_usb:

Connection through USB
----------------------

The nRF Desktop devices use the USB HID class.
No additional software or drivers are required.

.. tabs::

   .. tab:: Gaming mouse

      .. figure:: /images/nrf_desktop_gaming_mouse_usb_slot.svg
         :alt: nRF Desktop gaming mouse (top view)

      The gaming mouse has the USB connector slot located below the scroll wheel.
      The connector should slide in the socket along the cut in the mouse base.

   .. tab:: HID dongle

      .. figure:: /images/nrf_desktop_dongle_usb.svg
         :alt: nRF Desktop dongle

      The dongle has an USB connector located at one end of the board.
      It should be inserted to the USB slot located on the host.

..

Gaming mouse, dongle, and DK support the HID data transmission through USB.

Gaming mouse USB
~~~~~~~~~~~~~~~~

The gaming mouse can send HID data when connected through USB.
When the device is connected both wirelessly and through USB at the same time, it provides input only through the USB connection.
If the device is disconnected from USB, it automatically switches to sending the data wirelessly using Bluetooth LE.

The gaming mouse is a battery-powered device.
When it is connected through USB, charging of the rechargeable batteries starts.

Dongle USB
~~~~~~~~~~

The nRF Desktop dongle works as a bridge between the devices connected through standard Bluetooth LE or Low Latency Packet Mode and the host connected through USB.
It receives data wirelessly from the connected peripherals and forwards the data to the host.

The nRF Desktop dongle is powered directly through USB.

DK USB
~~~~~~

The DK functionality depends on the application configuration.
Depending on the selected configuration options, it can work as a mouse, keyboard, or a dongle.

.. _nrf_desktop_ble:

Connection through Bluetooth LE
-------------------------------

When turned on, the nRF Desktop peripherals are advertising until they go to the suspended state or connect through Bluetooth.
The peripheral supports one wireless connection at a time, but it can be bonded with :ref:`multiple peers <nrf_desktop_ble_peers>`.

The nRF Desktop Bluetooth Central device scans for all bonded peripherals that are not connected.
The scanning is interrupted when any device connected to the dongle through Bluetooth comes in use.
Continuing the scanning in such scenario would cause report rate drop.

The scanning starts automatically when one of the bonded peers disconnects.
It also takes place periodically when a known peer is not connected.

The peripheral connection can be based on standard Bluetooth LE connection parameters or on Bluetooth LE with Low Latency Packet Mode (LLPM).

LLPM is a proprietary Bluetooth extension from Nordic Semiconductor.
It can be used only if it is supported by both connected devices (desktop mice do not support it).
LLPM enables sending data with high report rate (up to 1000 reports per second), which is not supported by the standard Bluetooth LE.

.. _nrf_desktop_ble_peers:

Bluetooth LE peer control
~~~~~~~~~~~~~~~~~~~~~~~~~

A connected Bluetooth LE peer device can be controlled using predefined buttons or button combinations.
There are several peer operations available.

The application distinguishes between the following button press types:

* Short - Button pressed for less than 0.5 seconds.
* Standard - Button pressed for more than 0.5 seconds, but less than 5 seconds.
* Long - Button pressed for more than 5 seconds.
* Double - Button pressed twice in quick succession.

The peer operation states provide visual feedback through LEDs (if the device has LEDs).
Each of the states is represented by separate LED color and effect.
The LED colors and effects are described in the :file:`led_state_def.h` file located in the board-specific directory in the application configuration directory.

The assignments of hardware interface elements depend on the device type.

.. tabs::

   .. tab:: Gaming mouse

      The following predefined hardware interface elements are assigned to peer control operations for the gaming mouse:

      Hardware switch
          * The switch is located next to the optical sensor.

          .. figure:: /images/nrf_desktop_gaming_mouse_bottom.svg
             :alt: nRF Desktop gaming mouse - bottom view

             nRF Desktop gaming mouse - bottom view

          * You can set the switch in the following positions:

            * Top position: Select the dongle peer.
            * Middle position: Select the Bluetooth LE peers.
            * Bottom position: Mouse turned off.

          When the dongle peer is selected, the peer control is disabled until the switch is set to another position.

      Peer control button
          * The button is located on the left side of the mouse, in the thumb area.

          .. figure:: /images/nrf_desktop_gaming_mouse_led1_peer_control_button.svg
             :alt: nRF Desktop gaming mouse - side view

             nRF Desktop gaming mouse - side view

          * Short-press to initialize the peer selection.
            (The **LED1** changes color and starts blinking.)
            During the peer selection:

            1. Short-press to toggle between available peers.
               The **LED1** changes color for each peer and keeps blinking.
            #. Double-press to confirm the peer selection.
               The peer is changed after the confirmation.
               **LED1** stops blinking.

               .. note::
                   |led_note|

          * Long-press to initialize the peer erase.
            When **LED1** starts blinking rapidly, double-press to confirm the operation.
            |nRF_Desktop_confirmation_effect|
          * |nRF_Desktop_cancel_operation|

   .. tab:: Desktop mouse

      The following predefined buttons are assigned to peer control operations for the desktop mouse:

      Scroll wheel button
        * Press the scroll wheel before the mouse is powered up with the on/off switch.
          Long-press to initialize and confirm the peer erase.

          .. figure:: /images/nrf_desktop_desktop_mouse_side_scroll.svg
             :alt: nRF Desktop desktop mouse - side view

             nRF Desktop desktop mouse - side view

          |nRF_Desktop_confirmation_effect|
        * |nRF_Desktop_cancel_operation|

   .. tab:: Keyboard

      The following predefined buttons or button combinations are assigned to peer control operations for the keyboard:

      Page Down key
        * Press the Page Down key while keeping the Fn modifier key pressed.

          .. figure:: /images/nrf_desktop_keyboard_top.svg
             :alt: nRF Desktop keyboard - top view

             nRF Desktop keyboard - top view

        * Short-press the Page Down key to initialize the peer selection.
          During the peer selection:

          1. Short-press to toggle between available peers.
             **LED1** blinks rapidly for each peer.
             The amount of blinks corresponds to the number assigned to a peer: one blink for peer 1, two blinks for peer 2, and so on.
          #. Double-press to confirm the peer selection.
             The peer is changed after the confirmation.
             **LED1** becomes solid for a short time and then turns itself off.

             .. note::
                  |led_note|

        * Long-press to initialize the peer erase.
          When **LED1** starts blinking rapidly, double-press to confirm the operation.
          |nRF_Desktop_confirmation_effect|
        * |nRF_Desktop_cancel_operation|

   .. tab:: HID dongle

      The following predefined buttons are assigned to peer control operations for the HID dongle:

      SW1 button
        * The **SW1** button is located on the top of the dongle, on the same side as **LED2**.

          .. figure:: /images/nrf_desktop_dongle_front_led2_sw1.svg
             :alt: nRF Desktop dongle - top view

             nRF Desktop dongle - top view

        * Long-press to initialize peer erase.
          When **LED2** starts blinking rapidly, double-press to confirm the operation.
          After the confirmation, all the Bluetooth bonds are removed for the dongle.
        * Short-press to start scanning for both bonded and non-bonded Bluetooth Peripherals.
          The scan is interrupted if another peripheral connected to the dongle is in use.

          .. note::
              |led_note|

        * |nRF_Desktop_cancel_operation|

..

System state indication
=======================

When available, one of the LEDs is used to indicate the state of the device.
This system state LED is kept on when the device is active.

.. tabs::

   .. tab:: Gaming mouse

      .. figure:: /images/nrf_desktop_gaming_mouse_led2.svg
         :alt: nRF Desktop gaming mouse (top view)

      The system state LED of the gaming mouse is located under the transparent section of the cover.
      The color of the LED changes when the device's battery is being charged.

   .. tab:: HID dongle

      .. figure:: /images/nrf_desktop_dongle_front_led1.svg
         :alt: nRF Desktop dongle

      **LED1** is used for the system state indication.
      It is located in the bottom right corner of the dongle, next to the USB connector.

..

In case of a system error, the system state LED will start to blink rapidly for some time before the device is reset.

.. _nrf_desktop_debugging:

Debugging
=========

Each of the nRF Desktop hardware reference designs has a slot for the dedicated debug board.
See the following figures for the exact location of these slots.

.. tabs::

   .. tab:: Gaming mouse

      .. figure:: /images/nrf_desktop_gaming_mouse_debug_board_slot.svg
         :alt: nRF Desktop gaming mouse (top view)

      The debug slot is located at the end of the gaming mouse, below the cover.

   .. tab:: Desktop mouse

      .. figure:: /images/nrf_desktop_desktop_mouse_side_debug.svg
         :alt: nRF Desktop desktop mouse (side view)

      The debug slot is located on the side of the desktop mouse.
      It is accesible through a hole in the casing.

   .. tab:: Keyboard

      .. figure:: /images/nrf_desktop_keyboard_back_debug.svg
         :alt: nRF Desktop keyboard (back view)

      The debug slot is located on the back of the keyboard.

..

The boards that you can plug into these slots are shown below.
The debug board can be used for programming the device (and powering it).
The bypass boards are needed to make the device work when the debug board is not used.
Their purpose is to close the circuits, which allows the device to be powered, for example during :ref:`nrf_desktop_testing_steps`.

.. tabs::

   .. tab:: Debug board

      .. figure:: /images/nrf_desktop_400391_jlink_debug.svg
         :alt: nRF Desktop debug board

      The device can be programmed using the J-Link.
      The J-Link connector slot is located on the top of the debug board.

   .. tab:: Short bypass board

      .. figure:: /images/nrf_desktop_400398_debug.svg
         :alt: nRF Desktop bypass board (short)

      The shorter nRF desktop bypass board can be used with the desktop mouse.

   .. tab:: Long bypass board

      .. figure:: /images/nrf_desktop_400398_long_debug.svg
         :alt: nRF Desktop bypass board (long)

      The longer nRF desktop bypass board can be used with the gaming mouse.

..

Power management
================

Reducing power consumption is important for every battery-powered device.

The nRF Desktop peripherals are either suspended or powered off when they are not in use for a defined amount of time:

* In the suspended state, the device maintains the active connection.
* In the powered off state, the CPU is switched to the off mode.

In both cases, most of the functionalities are disabled.
For example, LEDs are turned off and advertising is stopped.

Moving the mouse or pressing any button wakes up the device and turns on the disabled functionalities.

You can define the amount of time after which the peripherals are suspended or powered off in :option:`CONFIG_DESKTOP_POWER_MANAGER_TIMEOUT`.
By default, this period is set to 120 seconds.

.. important::
    When the gaming mouse is powered from USB, the power down time-out functionality is disabled.

    If a nRF Desktop device supports remote wakeup, the USB connected device goes to suspended state when USB is suspended.
    The device can then trigger remote wakeup of the connected host on user input.

Building and running
********************

.. |sample path| replace:: :file:`applications/nrf_desktop`

The nRF Desktop application is built the same way to any other |NCS| application or sample.

.. include:: /includes/build_and_run.txt

.. note::
    Information about the known issues in nRF Desktop can be found in |NCS|'s :ref:`release_notes` and on the :ref:`known_issues` page.

.. _nrf_desktop_selecting_build_types:

Selecting a build type
======================

Before you start testing the application, you can select one of the :ref:`nrf_desktop_requirements_build_types`, depending on your development kit and building method.

Selecting a build type in SES
-----------------------------

.. include:: /gs_modifying.rst
   :start-after: build_types_selection_ses_start
   :end-before: build_types_selection_ses_end

Selecting a build type from command line
----------------------------------------

.. include:: /gs_modifying.rst
   :start-after: build_types_selection_cmd_start
   :end-before: build_types_selection_cmd_end

.. _nrf_desktop_testing_steps:

Testing
=======

The application can be built and tested in various configurations.
The following procedure refers to the scenario where the gaming mouse (nRF52840 Gaming Mouse) and the keyboard (nRF52832 Desktop Keyboard) are connected simultaneously to the dongle (nRF52840 USB Dongle).

After building the application with or without :ref:`specifying the build type <nrf_desktop_selecting_build_types>`, test the nRF Desktop application by performing the following steps:

1. Program the required firmware to each device.
#. Insert the :ref:`debug board or bypass board <nrf_desktop_debugging>` into the mouse to make sure it is powered.
#. Turn on both mouse and keyboard.
   **LED1** on the keyboard and **LED1** on the mouse start breathing.
#. Plug the dongle to the USB port.
   The blue **LED2** on the dongle starts breathing.
   This indicates that the dongle is scanning for peripherals.
#. Wait for the establishment of the Bluetooth connection, which happens automatically.
   After the Bluetooth connection is established, the LEDs stop breathing and remain turned on.
   The devices can then be used simultaneously.

   .. note::
        You can manually start the scanning for new peripheral devices by pressing the **SW1** button on the dongle for a short time.
        This might be needed if the dongle does not connect with all the peripherals before time-out.
        The scanning is interrupted after the amount of time predefined in :option:`CONFIG_DESKTOP_BLE_SCAN_DURATION_S`, because it negatively affects the performance of already connected peripherals.

#. Move the mouse and press any key on the keyboard.
   The input is reflected on the host.

   .. note::
       When a :ref:`configuration with debug features <nrf_desktop_requirements_build_types>` is enabled, for example logger and assertions, the gaming mouse report rate can be significantly lower.

       Make sure that you use the release configurations before testing the mouse report rate.
       For the release configurations, you should observe a 500-Hz report rate when both the mouse and the keyboard are connected and a 1000-Hz rate when only the mouse is connected.

#. Switch the Bluetooth peer on the gaming mouse by pressing the Precise Aim button (see `User interface`_).
   The color of **LED1** changes from red to green and the LED starts blinking rapidly.
#. Press the Precise Aim button twice quickly to confirm the selection.
   After the confirmation, **LED1** starts breathing and the mouse starts the Bluetooth advertising.
#. Connect to the mouse with an Android phone, a laptop, or any other Bluetooth Central.

After the connection is established and the device is bonded, you can use the mouse with the connected device.

Windows Hardware Lab Kit tests
------------------------------

The nRF Desktop devices have passed the tests from official playlist required for compatibility with Windows 10 by Windows Hardware Compatibility Program (:file:`HLK Version 1903 CompatPlaylist x86 x64 ARM64.xml`).
The tests were conducted using `Windows Hardware Lab Kit`_.


.. _nrf_desktop_porting_guide:

Integrating your own hardware
*****************************

This section describes how to adapt the nRF Desktop application to different hardware.
It describes the configuration sources that are used for the default configuration, and lists steps required for adding a new board.

Configuration sources
=====================

The nRF Desktop application uses the following files as configuration sources:

* Devicetree Specification (DTS) files - These reflect the hardware configuration.
  See :ref:`zephyr:dt-guide` for more information about the DTS data structure.
* :file:`_def` files - These contain configuration arrays for the application modules and are specific to the nRF Desktop application.
* Kconfig files - These reflect the software configuration.
  See :ref:`kconfig_tips_and_tricks` for information about how to configure them.

You must modify these configuration sources when `Adding a new board`_.
For information about differences between DTS and Kconfig, see :ref:`zephyr:dt_vs_kconfig`.

.. _nrf_desktop_board_configuration:

Board configuration
===================

The nRF Desktop application is modular.
Depending on requested functions, it can provide mouse, keyboard, or dongle functionality.
The selection of modules depends on the chosen role and also on the selected reference design.
For more information about modules available for each configuration, see :ref:`nrf_desktop_architecture`.

For a board to be supported by the application, you must provide a set of configuration files at :file:`applications/nrf_desktop/configuration/your_board_name`.
The application configuration files define both a set of options with which the nRF Desktop application will be created for your board and the selected :ref:`nrf_desktop_requirements_build_types`.
Include the following files in this directory:

Mandatory configuration files
    * Application configuration file for the ``ZDebug`` :ref:`build type <nrf_desktop_requirements_build_types>`.
    * Configuration files for the selected modules.

Optional configuration files
    * Application configuration files for other build types.
    * Configuration file for the bootloader.
    * Memory layout configuration.
    * DTS overlay file.

See `Adding a new board`_ for information about how to add these files.

nRF Desktop board configuration files
-------------------------------------

The nRF Desktop application comes with configuration files for the following reference designs:

nRF52840 Gaming Mouse (nrf52840gmouse_nrf52840)
      * The reference design is defined in :file:`nrf/boards/arm/nrf52840gmouse_nrf52840` for the project-specific hardware.
      * To achieve gaming-grade performance:

        * The application is configured to act as a gaming mouse, with both Bluetooth LE and USB transports enabled.
        * Bluetooth is configured to use Nordic's SoftDevice link layer.

      * |preconfigured_build_types|

nRF52832 Desktop Mouse (nrf52dmouse_nrf52832) and nRF52810 Desktop Mouse (nrf52810dmouse_nrf52810)
      * Both reference designs are meant for the project-specific hardware and are defined in :file:`nrf/boards/arm/nrf52dmouse_nrf52832` and :file:`nrf/boards/arm/nrf52810dmouse_nrf52810`, respectively.
      * The application is configured to act as a mouse.
      * Only the Bluetooth LE transport is enabled.
        Bluetooth uses Zephyr's software link layer.
      * There is no configuration with bootloader available.

Sample mouse, keyboard or dongle (nrf52840dk_nrf52840)
      * The configuration uses the nRF52840 Development Kit.
      * The build types allow to build the application as mouse, keyboard or dongle.
      * Inputs are simulated based on the hardware button presses.
      * The configuration with bootloader is available.

Sample dongle (nrf52833dk_nrf52833)
      * The configuration uses the nRF52833 Development Kit.
      * The application is configured to act as both mouse and keyboard.
      * Bluetooth uses Nordic's SoftDevice link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.

nRF52832 Desktop Keyboard (nrf52kbd_nrf52832)
      * The reference design used is defined in :file:`nrf/boards/arm/nrf52kbd_nrf52832` for the project-specific hardware.
      * The application is configured to act as a keyboard, with the Bluetooth LE transport enabled.
      * Bluetooth is configured to use Nordic's SoftDevice link layer.
      * |preconfigured_build_types|

nRF52840 USB Dongle (nrf52840dongle_nrf52840) and nRF52833 USB Dongle (nrf52833dongle_nrf52833)
      * Since the nRF52840 Dongle is generic and defined in Zephyr, project-specific changes are applied in the DTS overlay file.
      * The application is configured to act as both mouse and keyboard.
      * Bluetooth uses Nordic's SoftDevice link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * |preconfigured_build_types|

nRF52820 USB Dongle (nrf52820dongle_nrf52820)
      * The application is configured to act as both mouse and keyboard.
      * Bluetooth uses Zephyr's software link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * |preconfigured_build_types|

.. _porting_guide_adding_board:

Adding a new board
==================

When adding a new board for the first time, focus on a single configuration.
Moreover, keep the default ``ZDebug`` build type that the application is built with, and do not add any additional build type parameters.

.. note::
    The following procedure uses the gaming mouse configuration as an example.

To use the nRF Desktop application with your custom board:

1. Define the reference design by copying the nRF Desktop reference design files that are the closest match for your hardware.
   For example, for gaming mouse use :file:`nrf/boards/arm/nrf52840gmouse_nrf52840`.
#. Edit the DTS files to make sure they match the hardware configuration.
   Pay attention to the following elements:

   * Pins that are used.
   * Bus configuration for optical sensor.
   * `Changing interrupt priority`_.

#. Edit the reference design's Kconfig files to make sure they match the required system configuration.
   For example, disable the drivers that will not be used by your device.
#. Copy the project files for the device that is the closest match for your hardware.
   For example, for gaming mouse these are located at :file:`applications/nrf_desktop/configure/nrf52840gmouse_nrf52840`.
#. Optionally, depending on the reference design, edit the DTS overlay file.
   This step is not required if you have created a new reference design and its DTS files fully describe your hardware.
   In such case, the overlay file can be left empty.
#. In Kconfig, ensure that the following modules that are specific for gaming mouse are enabled:

   * :ref:`nrf_desktop_motion`
   * :ref:`nrf_desktop_wheel`
   * :ref:`nrf_desktop_buttons`
   * :ref:`nrf_desktop_battery_meas`
   * :ref:`nrf_desktop_leds`

#. For each module enabled, change its :file:`_def` file to match your hardware.
   Apply the following changes, depending on the module:

   Motion module
     * The ``nrf52840gmouse_nrf52840`` uses the PMW3360 optical motion sensor.
       The sensor is configured in DTS, and the sensor type is selected in the application configuration.
       To add a new sensor, expand the application configuration.
   Wheel module
     * The wheel is based on the QDEC peripheral of the nRF52840 device and the hardware-related part is configured in DTS.
   Buttons module
     * To simplify the configuration of arrays, the nRF Desktop application uses :file:`_def` files.
     * The :file:`_def` file of the buttons module contains pins assigned to rows and columns.
   Battery measurement module
     * The :file:`_def` file of the battery measurement module contains the mapping needed to match the voltage that is read from ADC to the battery level.
   LEDs module
     * The application uses two logical LEDs - one for the peers state, and one for the system state indication.
     * Each of the logical LEDs can have either one (monochromatic) or three color channels (RGB).
       Such color channel is a physical LED.
     * The project uses Pulse-Width Modulation (PWM) channels to control the brightness of each physical LED.
       Configure the PWM peripheral in DTS files, and configure the :file:`_def` file of the LEDs module to indicate which PWM channel is assigned to each LED color.
       Ensure that PWM channels are correctly configured in DTS and PWM driver is enabled in the Kconfig file.

#. Review Bluetooth options in Kconfig:

   a. Ensure that the Bluetooth role is properly configured.
      For mouse, it should be configured as peripheral.
   #. Update the configuration related to peer control.
      You can also disable the peer control using the :option:`CONFIG_DESKTOP_BLE_PEER_CONTROL` option.
      Peer control details are described in the :ref:`nrf_desktop_ble_bond` documentation.

   Refer to the :ref:`nrf_desktop_bluetooth_guide` section and Zephyr's :ref:`zephyr:bluetooth` page for more detailed information about the Bluetooth configuration.
#. Edit Kconfig to disable options that you do not use.
   Some options have dependencies that might not be needed when these options are disabled.
   For example, when the LEDs module is disabled, the PWM driver is not needed.

.. _porting_guide_adding_sensor:

Adding a new motion sensor
==========================

This procedure describes how to add a new motion sensor into the project.
You can use it as a reference for adding other hardware components.

The nRF Desktop application comes with a :ref:`nrf_desktop_motion` that is able to read data from a motion sensor.
While |NCS| provides support for two motion sensor drivers (PMW3360 and PAW3212), you can add support for a different sensor, based on your development needs.

Complete the steps described in the following sections to add a new motion sensor.

1. Add a new sensor driver
--------------------------

First, create a new motion sensor driver that will provide code for communication with the sensor.
Use the two existing |NCS| sensor drivers as an example.

The communication between the application and the sensor is done through a sensor driver API (see :ref:`sensor_api`).
For motion module to work correctly, the driver must support a trigger (see ``sensor_trigger_set``) on a new data (see ``SENSOR_TRIG_DATA_READY`` trigger type).

When motion data is ready, the driver calls a registered callback.
The application starts a process of retrieving a motion data sample.
The motion module calls ``sensor_sample_fetch`` and then ``sensor_channel_get`` on two sensor channels, ``SENSOR_CHAN_POS_DX`` and ``SENSOR_CHAN_POS_DY``.
The driver must support these two channels.

2. Create a DTS binding
-----------------------

Zephyr recommends to use DTS for hardware configuration (see :ref:`zephyr:dt_vs_kconfig`).
For the new motion sensor configuration to be recognized by DTS, define a dedicated DTS binding.
See :ref:`dt-bindings` for more information, and refer to :file:`dts/bindings/sensor` for binding examples.

3. Configure sensor through DTS
-------------------------------

Once binding is defined, it is possible to set the sensor configuration.
This is done by editing the DTS file that describes the board.
For more information, see :ref:`devicetree-intro`.

As an example, take a look at the PMW3360 sensor that already exists in |NCS|.
The following code excerpt is taken from :file:`boards/arm/nrf52840gmouse_nrf52840/nrf52840gmouse_nrf52840.dts`.

.. code-block:: none

   &spi1 {
   	compatible = "nordic,nrf-spim";
   	status = "okay";
   	sck-pin = <16>;
   	mosi-pin = <17>;
   	miso-pin = <15>;
   	cs-gpios = <&gpio0 13 0>;

   	pmw3360@0 {
   		compatible = "pixart,pmw3360";
   		reg = <0>;
   		irq-gpios = <&gpio0 21 0>;
   		spi-max-frequency = <2000000>;
   		label = "PMW3360";
   	};
   };

The communication with PMW3360 is done through the SPI, which makes the sensor a subnode of the SPI bus node.
SPI pins are defined as part of the bus configuration, as these are common among all devices connected to this bus.
In this case, the PMW3360 sensor is the only device on this bus and so there is only one pin specified for selecting chip.

When the sensor's node is mentioned, you can read ``@0`` in ``pmw3360@0``.
For SPI devices, ``@0`` refers to the position of the chip select pin in the ``cs-gpios`` array for a corresponding device.

Note the string ``compatible = "pixart,pmw3360"`` in the subnode configuration.
This string indicates which DTS binding the node will use.
The binding should match with the DTS binding created earlier for the sensor.

The following options are inherited from the ``spi-device`` binding and are common to all SPI devices:

* ``reg`` - The slave ID number the device has on a bus.
* ``label`` - Used to generate a name of the device (for example, it will be added to generated macros).
* ``spi-max-frequency`` - Used for setting the bus clock frequency.

  .. note::
      To achieve the full speed, data must be propagated through the application and reach Bluetooth LE a few hundred microseconds before the subsequent connection event.
      If you aim for the lowest latency through the LLPM (a 1-ms interval), the sensor data readout should take no more then 250 us.
      The bus and the sensor configuration must ensure that communication speed is fast enough.

The remaining option ``irq-gpios`` is specific to ``pixart,pmw3360`` binding.
It refers to the PIN to which the motion sensor IRQ line is connected.

If a different kind of bus is used for the new sensor, the DTS layout will be different.

4. Include sensor in the application
------------------------------------

Once the new sensor is supported by |NCS| and board configuration is updated, you can include it in the nRF Desktop application.

The nRF Desktop application selects a sensor using the configuration options defined in :file:`src/hw_interface/Kconfig.motion`.
Add the new sensor as a new choice option.

The :ref:`nrf_desktop_motion` of the nRF Desktop application has access to several sensor attributes.
These attributes are used to modify the sensor behavior in runtime.
Since the names of the attributes differ for each sensor, the :ref:`nrf_desktop_motion` uses a generic abstraction of them.
You can translate the new sensor-specific attributes to a generic abstraction by modifying :file:`configuration/common/motion_sensor.h` .

.. tip::
    If an attribute is not supported by the sensor, it does not have to be defined.
    In such case, set the attribute to ``-ENOTSUP``.

5. Select the new sensor
------------------------

The application can now use the new sensor.
Edit the application configuration files for your board to enable it.
See :ref:`nrf_desktop_board_configuration` for details.

To start using the new sensor, complete the following steps:

1. Enable all dependencies required by the driver (for example, bus driver).
#. Enable the new sensor driver.
#. Select the new sensor driver in the application configuration options.

Changing interrupt priority
---------------------------

You can edit the DTS files to change the priority of the peripheral's interrupt.
This can be useful when :ref:`adding a new custom board <porting_guide_adding_board>` or whenever you need to change the interrupt priority.

The ``interrupts`` property is an array, where meaning of each element is defined by the specification of the interrupt controller.
These specification files are located at :file:`zephyr/dts/bindings/interrupt-controller/` DTS binding file directory.

For example, for nRF52840 the file is :file:`arm,v7m-nvic.yaml`.
This file defines ``interrupts`` property in the ``interrupt-cells`` list.
In case of nRF52840, it contains two elements: ``irq`` and ``priority``.
The default values for these elements for the given peripheral can be found in the :file:`dtsi` file specific for the device.
In case of nRF52840, this is :file:`zephyr/dts/arm/nordic/nrf52840.dtsi`, which has the following ``interrupts`` property for nRF52840:

.. code-block::

   spi1: spi@40004000 {
           /*
            * This spi node can be SPI, SPIM, or SPIS,
            * for the user to pick:
            * compatible = "nordic,nrf-spi" or
            *              "nordic,nrf-spim" or
            *              "nordic,nrf-spis".
            */
           #address-cells = <1>;
           #size-cells = <0>;
           reg = <0x40004000 0x1000>;
           interrupts = <4 1>;
           status = "disabled";
           label = "SPI_1";
   };

To change the priority of the peripheral's interrupt, override the ``interrupts`` property of the peripheral node by including the following code snippet in the :file:`dts.overlay` or directly in the board DTS:

.. code-block:: none

   &spi1 {
       interrupts = <4 2>;
   };

This code snippet will change the **SPI1** interrupt priority from default ``1`` to ``2``.

.. _nrf_desktop_flash_memory_layout:

Flash memory layout
===================

Depending on whether the bootloader is enabled, the partition layout on the flash memory is set by defining one of the following options:

* `Memory layout in DTS`_ (bootloader is not enabled)
* `Memory layout in partition manager`_ (bootloader is enabled)

The set of required partitions differs depending on configuration:

* There must be at least one partition where the code is stored.
* There must be one partition for storing :ref:`zephyr:settings_api`.
* The bootloader, if enabled, will add additional partitions to the set.

.. important::
   Before updating the firmware, make sure that the data stored in the settings partition is compatible with the new firmware.
   If it is incompatible, erase the settings area before using the new firmware.

Memory layout in DTS
--------------------

When using the flash memory layout in the DTS files, define the ``partitions`` child node in the flash device node (``&flash0``).

Since the nRF Desktop application uses the partition manager when the bootloader is present, the partition definition from the DTS files is valid only for configurations without the bootloader.

.. note::
    If you wish to change the default flash memory layout of the board without editing board-specific files, edit the DTS overlay file.
    The nRF Desktop application automatically adds the overlay file if the :file:`dts.overlay` file is present in the project's board configuration directory.
    See more in the `Board configuration`_ section.

.. important::
    By default, Zephyr does not use the code partition defined in the DTS files.
    It is only used if :option:`CONFIG_USE_DT_CODE_PARTITION` is enabled.
    If this option is disabled, the code is loaded at the address defined by :option:`CONFIG_FLASH_LOAD_OFFSET` and can spawn for :option:`CONFIG_FLASH_LOAD_SIZE` (or for the whole flash if the load size is set to zero).

Because the nRF Desktop application depends on the DTS layout only for configurations without the bootloader, only the settings partition is relevant in such cases and other partitions are ignored.

For more information about how to configure the flash memory layout in the DTS files, see :ref:`zephyr:flash_map_api`.

Memory layout in partition manager
----------------------------------

When the bootloader is enabled, the nRF Desktop application uses the partition manager for the layout configuration of the flash memory.
The nRF Desktop configurations with bootloader use static configurations of partitions to ensure that the partition layout will not change between builds.

Add the :file:`pm_static_${CMAKE_BUILD_TYPE}.yml` file to the project's board configuration directory to define the static partition manager configuration for given board and build type.
Take into account the following points:

* For the :ref:`background firmware upgrade <nrf_desktop_bootloader_background_dfu>`, you must define the secondary image partition.
  This is because the update image is stored on the secondary image partition while the device is running firmware from the primary partition.
* When you use :ref:`USB serial recovery <nrf_desktop_bootloader_serial_dfu>`, you do not need the secondary image partition.
  The firmware image is overwritten by the bootloader.

For more information about how to configure the flash memory layout using the partition manager, see :ref:`partition_manager`.

External flash configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Partition Manager supports partitions in external flash.

Enabling external flash can be useful especially for memory-limited devices.
For example, the MCUboot can use it as a secondary image partition for the :ref:`background firmware upgrade <nrf_desktop_bootloader_background_dfu>`.
The MCUboot moves the image data from the secondary image partition to the primary image partition before booting the new firmware.

For an example of the nRF Desktop application configuration that uses an external flash, see the ``ZDebugMCUBootQSPI`` configuration of the nRF52840 Development Kit.
This configuration uses the ``MX25R64`` external flash that is part of the development kit.

For detailed information, see the :ref:`partition_manager` documentation.

.. _nrf_desktop_bluetooth_guide:

Bluetooth in nRF Desktop
========================

The nRF Desktop devices use :ref:`Zephyr's Bluetooth API <zephyr:bluetooth>` to handle the Bluetooth LE connections.

This API is used only by the application modules that handle such connections.
The information about peer and connection state is propagated to other application modules using :ref:`event_manager` events.

The nRF Desktop devices come in the following types:

* Peripheral devices (mouse or keyboard)

  * Support only the Bluetooth Peripheral role (:option:`CONFIG_BT_PERIPHERAL`).
  * Handle only one Bluetooth LE connection at a time.
  * Use more than one Bluetooth local identity.

* Central devices (dongle)

  * Support only the Bluetooth Central role (:option:`CONFIG_BT_CENTRAL`).
  * Handle multiple Bluetooth LE connections simultaneously.
  * Use only one Bluetooth local identity (the default one).

Both central and peripheral devices have dedicated configuration options and use dedicated modules.

.. note::
    There is no nRF Desktop device that supports both central and peripheral roles.

Common configuration and application modules
--------------------------------------------

Some Bluetooth-related :ref:`configuration options <nrf_desktop_bluetooth_guide_configuration>` (including :ref:`nrf_desktop_bluetooth_guide_configuration_ll` in a separate section) and :ref:`application modules <nrf_desktop_bluetooth_guide_modules>` are common for every nRF Desktop device.

.. _nrf_desktop_bluetooth_guide_configuration:

Configuration options
~~~~~~~~~~~~~~~~~~~~~

This section describes the most important Bluetooth Kconfig options common for all nRF Desktop devices.
For detailed information about every option, see the Kconfig help.

* :option:`CONFIG_BT_MAX_PAIRED`

  * nRF Desktop central: The maximum number of paired devices is greater than or equal to the maximum number of simultaneously connected peers.
  * nRF Desktop peripheral: The maximum number of paired devices is equal to the number of peers plus one, where the one additional paired device slot is used for erase advertising.

* :option:`CONFIG_BT_ID_MAX`

  * nRF Desktop central: The device uses only one Bluetooth local identity, that is the default one.
  * nRF Desktop peripheral: The number of Bluetooth local identities must be equal to the number of peers plus two.

    * One additional local identity is used for erase advertising.
    * The other additional local identity is the default local identity, which is unused, because it cannot be reset after removing the bond.
      Without the identity reset, the previously bonded central could still try to reconnect after being removed from Bluetooth bonds on the peripheral side.

* :option:`CONFIG_BT_MAX_CONN`

  * nRF Desktop central: Set the option to the maximum number of simultaneously connected devices.
  * nRF Desktop peripheral: The default value (one) is used.

.. note::
   After changing the number of Bluetooth peers for the nRF Desktop peripheral device, update the LED effects used to represent the Bluetooth connection state.
   For details, see :ref:`nrf_desktop_led_state`.

.. _nrf_desktop_bluetooth_guide_configuration_ll:

Link Layer configuration options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The nRF Desktop devices use one of the following Link Layers:

* :option:`CONFIG_BT_LL_SW_SPLIT`
    This Link Layer does not support the Low Latency Packet Mode (LLPM) and has a lower memory usage, so it can be used by memory-limited devices.

* :option:`CONFIG_BT_LL_SOFTDEVICE`
    This Link Layer does support the Low Latency Packet Mode (LLPM).
    If you opt for this Link Layer and enable this option, the :option:`CONFIG_DESKTOP_BLE_USE_LLPM` is also enabled by default and can be configured further:

    * When :option:`CONFIG_DESKTOP_BLE_USE_LLPM` is enabled, set the value for :option:`CONFIG_SDC_MAX_CONN_EVENT_LEN_DEFAULT` to ``3000``.

      This is required by the nRF Desktop central and helps avoid scheduling conflicts with Bluetooth Link Layer.
      Such conflicts could lead to a drop in HID input report rate or a disconnection.
      Setting the value to ``3000`` also enables the nRF Desktop central to exchange data with up to 2 standard Bluetooth LE peripherals during every connection interval (every 7.5 ms).

    * When :option:`CONFIG_DESKTOP_BLE_USE_LLPM` is disabled, the device will use only standard Bluetooth LE connection parameters with the lowest available connection interval of 7.5 ms.

      If the LLPM is disabled and more than 2 simultaneous Bluetooth connections are supported (:option:`CONFIG_BT_MAX_CONN`), you can set the value for :option:`CONFIG_SDC_MAX_CONN_EVENT_LEN_DEFAULT` to ``2500``.
      With this value, the nRF Desktop central is able to exchange the data with up to 3 Bluetooth LE peripherals during every 7.5-ms connection interval.
      Using the value of ``3000`` for more than 2 simultaneous Bluetooth LE connections will result in a lower HID input report rate.

.. _nrf_desktop_bluetooth_guide_modules:

Application modules
~~~~~~~~~~~~~~~~~~~

Every nRF Desktop device that enables Bluetooth must handle connections and manage bonds.
These features are implemented by the following modules:

* :ref:`nrf_desktop_ble_state` - Enables Bluetooth and LLPM (if supported), and handles Zephyr connection callbacks.
* :ref:`nrf_desktop_ble_bond` - Manages Bluetooth bonds and local identities.

You need to enable all these modules to enable both features.
For information about how to enable the modules, see their respective documentation pages.

Optionally, you can also enable the following module:

* :ref:`nrf_desktop_ble_qos` - Helps achieve better connection quality and higher report rate.
  The module can be used only with the SoftDevice Link Layer.

.. note::
   The nRF Destkop devices enable :option:`CONFIG_BT_SETTINGS`.
   When this option is enabled, the application is responsible for calling the :c:func:`settings_load` function - this is handled by the :ref:`nrf_desktop_settings_loader`.

.. _nrf_desktop_bluetooth_guide_peripheral:

Bluetooth Peripheral
--------------------

The nRF Desktop peripheral devices must include additional configuration options and additional application modules to comply with the HID over GATT specification.

The HID over GATT profile specification requires Bluetooth Peripherals to define the following GATT Services:

* HID Service - Handled in the :ref:`nrf_desktop_hids`.
* Battery Service - Handled in the :ref:`nrf_desktop_bas`.
* Device Information Service - Implemented in Zephyr and enabled with :option:`CONFIG_BT_DIS`.
  It can be configured using Kconfig options with the ``CONFIG_BT_DIS`` prefix.

The nRF Desktop peripherals must also define a dedicated GATT Service, which is used to provide the following information:

* Information whether the device can use the LLPM Bluetooth connection parameters.
* Hardware ID of the peripheral.

The GATT Service is implemented by the :ref:`nrf_desktop_dev_descr`.

Apart from the GATT Services, an nRF Desktop peripheral device must enable and configure the following application modules:

* :ref:`nrf_desktop_ble_adv` - Controls the Bluetooth advertising.
* :ref:`nrf_desktop_ble_latency` - Keeps the connection latency low when the :ref:`nrf_desktop_config_channel` is being used or when an update image is being received by the :ref:`nrf_desktop_smp`.
  This is done to ensure quick data transfer.

Optionally, you can also enable the following module:

* :ref:`nrf_desktop_qos` - Forwards the Bluetooth LE channel map generated by :ref:`nrf_desktop_ble_qos`.
  The Bluetooth LE channel map is forwarded using GATT characteristic.
  The Bluetooth Central can apply the channel map to avoid congested RF channels.
  This results in better connection quality and higher report rate.

Bluetooth Central
-----------------

The nRF Desktop central must implement Bluetooth scanning and handle the GATT operations.
The central must also control the Bluetooth connection parameters.
These features are implemented by the following application modules:

* :ref:`nrf_desktop_ble_scan` - Controls the Bluetooth scanning.
* :ref:`nrf_desktop_ble_conn_params` - Control the Bluetooth connection parameters and react on connection slave latency update requests received from the connected peripherals.
* :ref:`nrf_desktop_ble_discovery` - Handles discovering and reading the GATT Characteristics from the connected peripheral.
* :ref:`nrf_desktop_hid_forward` - Subscribes for HID reports from the Bluetooth Peripherals (HID over GATT) and forwards data using application events.

.. _nrf_desktop_bootloader:

Bootloader
==========

The nRF Desktop application can use one of the following bootloaders:

**Secure Bootloader**
  In this documentation, the Secure Bootloader is referred as *B0*.
  B0 is a small, simple, and secure bootloader that allows the application to boot directly from one of the application slots, thus increasing the speed of the direct firmware upgrade (DFU) process.

  This bootloader can be used only for the :ref:`background DFU <nrf_desktop_bootloader_background_dfu>` through the :ref:`nrf_desktop_config_channel` and :ref:`nrf_desktop_dfu`.
  More information about the B0 can be found at the :ref:`bootloader` page.

**MCUboot**
  The MCUboot bootloader can be used in the following scenarios:

  * :ref:`Background DFU <nrf_desktop_bootloader_background_dfu>`.
    In this scenario, the MCUboot swaps the application images located on the secondary and primary slot before booting the new image.
    Because of this, the image is not booted directly from the secondary image slot.
    The swap operation takes additional time, but an external FLASH can be used as the secondary image slot.

    You can use the MCUboot for the background DFU through the :ref:`nrf_desktop_config_channel` and :ref:`nrf_desktop_dfu`.
    The MCUboot can also be used for the background DFU over Simple Management Protocol (SMP).
    The SMP can be used to transfer the new firmware image in the background from an Android device.
    In that case, the :ref:`nrf_desktop_smp` is used to handle the image transfer.

  * :ref:`USB serial recovery <nrf_desktop_bootloader_serial_dfu>`.
    In this scenario, the MCUboot bootloader supports the USB serial recovery.
    The USB serial recovery can be used for memory-limited devices that support the USB connection.

  For more information about the MCUboot, see the :ref:`MCUboot <mcuboot:mcuboot_wrapper>` documentation.

.. note::
    The nRF Desktop application can use either B0 or MCUboot.
    The MCUboot is not used as the second stage bootloader.

.. important::
    Make sure that you use your own private key for the release version of the devices.
    Do not use the debug key for production.

Configuring the B0 bootloader
-----------------------------

To enable the B0 bootloader, select the :option:`CONFIG_SECURE_BOOT` Kconfig option.

The B0 bootloader requires the following options enabled:

* :option:`CONFIG_SB_SIGNING_KEY_FILE` - Required for providing the signature used for image signing and verification.
* :option:`CONFIG_FW_INFO` - Required for the application versioning information.
* :option:`CONFIG_FW_INFO_FIRMWARE_VERSION` - Enable this option to set the version of the application after you enabled :option:`CONFIG_FW_INFO`.
* :option:`CONFIG_BUILD_S1_VARIANT` - Required for the build system to be able to construct the application binaries for both application's slots in flash memory.

Configuring the MCUboot bootloader
----------------------------------

To enable the MCUboot bootloader, select the :option:`CONFIG_BOOTLOADER_MCUBOOT` Kconfig option.

Configure the MCUboot bootloader with the following options:

* ``CONFIG_BOOT_SIGNATURE_KEY_FILE`` - This option defines the path to the private key that is used to sign the application and that is used by the bootloader to verify the application signature.
  The key must be defined only in the MCUboot bootloader configuration file.
* :option:`CONFIG_IMG_MANAGER` and :option:`CONFIG_MCUBOOT_IMG_MANAGER` - These options allow the application to manage the DFU image.
  Enable both of them only for configurations that support :ref:`background DFU <nrf_desktop_bootloader_background_dfu>`.
  For these configurations, the :ref:`nrf_desktop_dfu` uses the provided API to request firmware upgrade and confirm the running image.

.. _nrf_desktop_bootloader_background_dfu:

Background Device Firmware Upgrade
==================================

The nRF Desktop application uses the :ref:`nrf_desktop_config_channel` and :ref:`nrf_desktop_dfu` for the background DFU process.
The firmware update process has three stages, discussed below.
At the end of these three stages, the nRF Desktop application will be rebooted with the new firmware package installed.

.. note::
    The background DFU mode requires two application slots in the flash memory.
    For this reason, the feature is not available for devices with smaller flash size, because the size of the flash memory required is essentially doubled.
    The devices with smaller flash size can use either :ref:`nrf_desktop_bootloader_serial_dfu` or MCUboot bootloader with the secondary image partition located on an external flash.

The background firmware upgrade can also be performed over the Simple Management Protocol (SMP).
For more detailed information about the DFU over SMP, read the :ref:`nrf_desktop_smp` documentation.

Update image generation
-----------------------

The update image is generated in the build directory when building the firmware if the bootloader is enabled in the configuration:

* The :file:`zephyr/dfu_application.zip` is used by both B0 and MCUboot bootloader for the background DFU through the :ref:`nrf_desktop_config_channel` and :ref:`nrf_desktop_dfu`.
  This package contains firmware images along with additional metadata.

  .. note::
      By default, the build process for the B0 bootloader will construct an image for the first slot (slot 0 or S0).
      To ensure that application is built for both slots, select the :option:`CONFIG_BUILD_S1_VARIANT` Kconfig option.

      When this option is selected, the :file:`zephyr/dfu_application.zip` contains both images.
      The update tool checks if the currently running image runs from either slot 0 or slot 1.
      It then transfers the update image that can be run from the unused slot.

* The :file:`zephyr/app_update.bin` is used for the background DFU through the :ref:`nrf_desktop_smp`.

Update image transfer
---------------------

The update image is transmitted in the background through the :ref:`nrf_desktop_config_channel`.
The configuration channel data is transmitted either through USB or over Bluetooth, using HID feature reports.
This allows the device to be used normally during the whole process (that is, the device does not need to enter any special state in which it becomes non-responsive to the user).

Depending on the side on which the process is handled:

* On the application side, the process is handled by :ref:`nrf_desktop_dfu`.
  See the module documentation for how to enable and configure it.
* On the host side, the process is handled by the :ref:`nrf_desktop_config_channel_script`.
  See the tool documentation for more information about how to execute the background DFU process on the host.

If the MCUboot bootloader is selected, the update image can also be transfered in the background through the :ref:`nrf_desktop_smp`.

Update image verification and swap
----------------------------------

Once the update image transfer is completed, the background DFU process will continue after the device reboot.
If :ref:`nrf_desktop_config_channel_script` is used, the reboot is triggered by the script right after the image transfer completes.

After the reboot, the bootloader locates the update image on the update partition of the device.
The image verification process ensures the integrity of the image and checks if its signature is valid.
If verification is successful, the bootloader boots the new version of the application.
Otherwise, the old version is used.

.. _nrf_desktop_bootloader_serial_dfu:

Serial recovery DFU
===================

The nRF Desktop application also supports the serial recovery DFU mode through USB.
In this mode, unlike in :ref:`background DFU mode <nrf_desktop_bootloader_background_dfu>`, the application is overwritten and only one application slot is used.
This mode can so be used on devices with a limited amount of flash memory available.

.. note::
   The serial recovery DFU and the background DFU cannot be enabled at the same time on the same device.

The serial recovery DFU is a feature of the bootloader.
For the serial recovery DFU to be performed, the bootloader must be able to access the USB subsystem.
This is not possible for the B0, and you have to use :ref:`MCUboot <mcuboot:mcuboot_wrapper>` instead.

As only one application slot is available, the transfer of the new version of the application cannot be done while the application is running.
To start the serial recovery DFU, the device should boot into recovery mode, in which the bootloader will be waiting for a new image upload to start.
In the serial recovery DFU mode, the new image is transferred through USB.
If the transfer is interrupted, the device will not be able to boot the application and automatically start in the serial recovery DFU mode.

Configuring serial recovery DFU
-------------------------------

Configure :ref:`MCUboot <mcuboot:mcuboot_wrapper>` to enable the serial recovery DFU through USB.
The MCUboot configuration for a given board and :ref:`build type <nrf_desktop_requirements_build_types>` should be written to :file:`applications/nrf_desktop/configuration/your_board_name/mcuboot_buildtype.conf`.
For an example of the configuration, see the ``ZReleaseMCUBoot`` build type of the nRF52820 or the nRF52833 dongle.

Not every configuration with MCUboot in the nRF Desktop supports the USB serial recovery.
For example, the ``ZDebugMCUBootSMP`` configuration for the nRF52840 Development Kit supports the MCUboot bootloader with background firmware upgrade.

Select the following Kconfig options to enable the serial recovery DFU:

* ``CONFIG_MCUBOOT_SERIAL`` - This option enables the serial recovery DFU.
* ``CONFIG_BOOT_SERIAL_CDC_ACM`` - This option enables the serial interface through USB.

  .. note::
        The USB subsystem must be enabled and properly configured.
        See :ref:`usb_api` for more information.

* ``CONFIG_BOOT_SERIAL_DETECT_PORT`` and ``CONFIG_BOOT_SERIAL_DETECT_PIN`` - These options select the pin used for triggering the serial recovery mode.
  To enter the serial recovery mode, set the pin to a logic value defined by ``CONFIG_BOOT_SERIAL_DETECT_PIN_VAL`` when the device boots.
  By default, the selected GPIO pin should be set to low.

Once the device enters the serial recovery mode, you can use the :ref:`mcumgr <zephyr:device_mgmt>` to:

* Query information about the present image.
* Upload the new image.
  The :ref:`mcumgr <zephyr:device_mgmt>` uses the :file:`zephyr/app_update.bin` update image file.
  It is generated by the build system when building the firmware.

For example, the following line will start the upload of the new image to the device:

.. code-block:: console

  mcumgr -t 60 --conntype serial --connstring=/dev/ttyACM0 image upload build-nrf52833dongle_nrf52833/zephyr/app_update.bin

Dependencies
************

This application uses the following |NCS| libraries and drivers:

* :ref:`event_manager`
* :ref:`profiler`
* :ref:`hids_readme`
* :ref:`hogp_readme`
* :ref:`nrf_bt_scan_readme`
* :ref:`gatt_dm_readme`
* :file:`drivers/sensor/paw3212`
* :file:`drivers/sensor/pmw3360`

.. _nrf_desktop_app_internal_modules:

Application internal modules
****************************

The nRF Desktop application uses its own set of internal modules.
See `Module and component overview`_ for more information.
More information about each application module and its configuration details is available on the subpages.

Each module documentation page has a table that shows the relations between module events.
`Module event tables`_ for some modules include extensive lists of source and sink modules.
These are valid for events that have many listeners or sources, and are gathered on the :ref:`nrf_desktop_event_rel_modules` subpage.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   doc/main.rst
   doc/battery_charger.rst
   doc/battery_meas.rst
   doc/ble_adv.rst
   doc/ble_bond.rst
   doc/ble_conn_params.rst
   doc/ble_discovery.rst
   doc/ble_latency.rst
   doc/ble_qos.rst
   doc/ble_scan.rst
   doc/ble_state.rst
   doc/board.rst
   doc/buttons.rst
   doc/buttons_sim.rst
   doc/click_detector.rst
   doc/config_channel.rst
   doc/cpu_meas.rst
   doc/dev_descr.rst
   doc/dfu.rst
   doc/failsafe.rst
   doc/fn_keys.rst
   doc/bas.rst
   doc/hid_forward.rst
   doc/hid_state.rst
   doc/hids.rst
   doc/info.rst
   doc/led_state.rst
   doc/led_stream.rst
   doc/leds.rst
   doc/motion.rst
   doc/passkey.rst
   doc/power_manager.rst
   doc/profiler_sync.rst
   doc/qos.rst
   doc/selector.rst
   doc/smp.rst
   doc/settings_loader.rst
   doc/usb_state.rst
   doc/watchdog.rst
   doc/wheel.rst
   doc/constlat.rst
   doc/hfclk_lock.rst
   doc/event_rel_modules.rst

Application configuration options
*********************************

.. options-from-kconfig::
   :show-type:

.. |nRF_Desktop_confirmation_effect| replace:: After the confirmation, Bluetooth advertising using a new local identity is started.
   When a new Bluetooth Central device successfully connects and bonds, the old bond is removed and the new bond is used instead.
   If the new peer does not connect in the predefined period of time, the advertising ends and the application switches back to the old peer.

.. |nRF_Desktop_cancel_operation| replace:: You can cancel the ongoing peer operation with a standard button press.

.. |preconfigured_build_types| replace:: The preconfigured build types configure the device with or without the bootloader and in debug or release mode.

.. |hid_state| replace:: HID state module

.. |led_note| replace:: A breathing LED indicates that the device has entered the advertising mode.
   This happens when the device is looking for a peer to connect to.
