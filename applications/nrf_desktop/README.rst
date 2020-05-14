.. _nrf_desktop:

nRF Desktop
###########

The nRF Desktop is a reference design of a HID device that is connected to a host through BLE or USB, or both.
Depending on the configuration, this application can function as a mouse, a gaming mouse, a keyboard, or a connection dongle.

.. note::
    The code is currently work-in-progress and as such is not fully functional, verified, or supported for product development.

Overview
********

The nRF Desktop project supports common input HW interfaces like motion sensors, rotation sensors, and buttons scanning module.
The firmware can be configured at runtime using a dedicated configuration channel established with the HID feature report.
The same channel is used to transmit DFU packets.

.. _nrf_desktop_architecture:

Firmware architecture
=====================

The nRF Desktop application design aims at configurability and extensibility while being responsive.

The application architecture is modular and event-driven.
This means that parts of application functionality were separated into isolated modules that communicate with each other using application events.
Application events are handled by the :ref:`event_manager`.
Modules register themselves as listeners of events they need to react on.
The application event can be submitted by multiple modules and it can have multiple listeners.

The event-based communication for each application module is presented as a table that is included on the :ref:`each module's documentation page <nrf_desktop_app_internal_modules>`.

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

The table contains the following columns:

Source Module
   The module that submits a given application event.

Input Event
   An application event that is received by the module described by the table.

This Module
   The module described by the table.
   This is the module that is the target of the Input Events and the source of Output Events directed to the Sink Modules.

Output Event
   An application event that is submitted by the module described by the table.

Sink Module
   The module that reacts on the application event.

.. note::
   Some application modules may have multiple implementations (for example, :ref:`nrf_desktop_motion`).
   In such case, the table presents the :ref:`event_manager` events received and submitted by all the implementations of a given application module.

The following figure shows the nRF Desktop modules and their relation with other components and the :ref:`event_manager`.

.. figure:: /images/nrf_desktop_arch.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (overview)

   Application high-level design overview (click to enlarge)

See the following subsections for more information about the firmware architecture:

    * `Module usage per hardware type`_
    * `Thread usage`_
    * `Memory allocation`_

Module usage per hardware type
------------------------------

Since the application architecture is uniform and the code is shared, the set of modules in use depends on the selected device role.
A different set of modules is enabled when the application is working as a mouse, a keyboard, or as a dongle:

    * `Gaming mouse module set`_
    * `Desktop mouse module set`_
    * `Keyboard module set`_
    * `Dongle module set`_

Gaming mouse module set
~~~~~~~~~~~~~~~~~~~~~~~

The following figure shows the modules enabled when the application is working as a gaming mouse.

.. figure:: /images/nrf_desktop_arch_gmouse.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (gaming mouse)

   Application configured as a gaming mouse (click to enlarge)

Desktop mouse module set
~~~~~~~~~~~~~~~~~~~~~~~~

The following figure shows the modules enabled when the application is working as a desktop mouse.

.. figure:: /images/nrf_desktop_arch_dmouse.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (desktop mouse)

   Application configured as a desktop mouse (click to enlarge)

Keyboard module set
~~~~~~~~~~~~~~~~~~~

The following figure shows the modules enabled when the application is working as a keyboard.

.. figure:: /images/nrf_desktop_arch_kbd.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (keyboard)

   Application configured as a keyboard (click to enlarge)

Dongle module set
~~~~~~~~~~~~~~~~~

The following figure shows the modules enabled when the application is working as a dongle.

.. figure:: /images/nrf_desktop_arch_dongle.svg
   :scale: 25 %
   :alt: nRF Desktop high-level design (dongle)

   Application configured as a dongle (click to enlarge)

Thread usage
------------

The application limits the number of threads that are used to minimum and does not use the user-space threads.

The following threads are kept running in the application:

* System-related threads
    * Idle thread
    * System workqueue thread
    * Logger thread (on debug :ref:`build types <nrf_desktop_requirements_build_types>`)
    * Shell thread (on :ref:`build types <nrf_desktop_requirements_build_types>` with shell enabled)
    * Threads related to Bluetooth LE (the exact number depends on the selected Link Layer)
* Application-related threads
    * Motion sensor thread (running only on a mouse)
    * Settings loading thread (enabled by default only on a keyboard)
    * QoS data sampling thread (running only on a dongle)

Most of the application activity takes place in the context of the system workqueue thread, either through scheduled work objects or through the event manager callbacks (executed from the system workqueue thread).
Because of this, the application does not need to handle resource protection.
The only exception are places where the interaction with interrupts or multiple threads cannot be avoided.

Memory allocation
-----------------

Most of the memory resources used by the application are allocated statically.

The application uses dynamic allocation for creating the event manager events.
For more information about events allocation, see :ref:`event_manager`.

.. note::
    When configuring HEAP, remember to set both :option:`CONFIG_HEAP_MEM_POOL_SIZE` and :option:`CONFIG_HEAP_MEM_POOL_MIN_SIZE` to match typical event size and the system needs.
    Zephyr's heap allocator works by dividing larger blocks into four smaller blocks.
    It is important to keep this in mind as this behavior impacts both the performance and the memory usage.
    For more information, refer to Zephyr's documentation at :ref:`heap_v2` and :ref:`memory_pools_v2`.

Forwarding the HID mouse data
-----------------------------

The nRF Desktop mouse sends HID input reports to host after the host connects and subscribes for the HID reports.

The :ref:`nrf_desktop_wheel` and :ref:`nrf_desktop_buttons` provide the data to the :ref:`nrf_desktop_hid_state` when the mouse wheel is used or a button is pressed.
These inputs are not synchronized with the HID report transmission to the host.

The :ref:`nrf_desktop_motion` sensor sampling is synchronized with sending the HID mouse input reports to the host.

When mouse is constantly in use, the motion module is kept in the fetching state.
In this state, the nRF Desktop mouse forwards the data from the motion sensor to the host in the following way:

1. USB state (or Bluetooth HIDS) sends a HID mouse report to the host and submits ``hid_report_sent_event``.
#. The ``hid_report_sent_event`` triggers sampling the motion sensor.
   A dedicated thread is used to fetch the sample from the sensor.
#. After the sample is fetched, the thread forwards it to the :ref:`nrf_desktop_hid_state` as ``motion_event``.
#. The |hid_state| updates the HID report data, generates new HID input report, and submits it as ``hid_report_event``.
#. The HID report data is forwarded to the host either by the :ref:`nrf_desktop_usb_state` or by the :ref:`nrf_desktop_hids`.
#. When the HID input report is sent to the host, the ``hid_report_sent_event`` is submitted.
#. The motion sensor sample is triggered and the sequence repeats.

If the device is connected through Bluetooth, the :ref:`nrf_desktop_hid_state` uses a pipeline that consists of two HID reports.
The pipeline is created when the first ``motion_event`` is received.
The |hid_state| submits two ``hid_report_event`` events.
When the first one is sent to the host, the motion sensor sample is triggered.

For the Bluetooth connections, submitting ``hid_report_sent_event`` is delayed by one Bluetooth connection interval.
Because of that, the :ref:`nrf_desktop_hids` requires pipeline of two HID reports to make sure that data will be sent on every connection event.
This is necessary to achieve high report rate when the device is connected through Bluetooth.

When there is no motion data for the predefined number of samples, the :ref:`nrf_desktop_motion` goes to the idle state.
This is done to reduce the power consumption.
When motion is detected, the module switches back to the fetching state.

The following diagram shows the data exchange between the application modules.
To simplify, the diagram shows only data related to HID input reports that are sent after the host is connected and the HID subscriptions are enabled.

.. figure:: /images/nrf_desktop_motion_sensing.svg
   :scale: 50 %
   :alt: nRF Desktop mouse HID data sensing and transmission

   nRF Desktop mouse HID data sensing and transmission

Requirements
************

The project supports several boards related to the following hardware reference designs.
Depending on what board you use, you need to select a respective configuration file and a :ref:`build type <nrf_desktop_requirements_build_types>`.


.. tabs::

   .. tab:: Gaming mouse

      .. figure:: /images/nrf_desktop_gaming_mouse_top_no_captions.svg
         :alt: nRF Desktop gaming mouse (top view)

      * nRF52840 Gaming Mouse (nrf52840gmouse_nrf52840)

   .. tab:: Desktop mouse

      .. figure:: /images/nrf_desktop_desktop_mouse_side_no_captions.svg
         :alt: nRF Desktop desktop mouse (side view)

      * nRF52832 Desktop Mouse (nrf52dmouse_nrf52832)
      * nRF52810 Desktop Mouse (nrf52810dmouse_nrf52810)

   .. tab:: Keyboard

      .. figure:: /images/nrf_desktop_keyboard_top_no_captions.svg
         :alt: nRF Desktop keyboard (top view)

      * nRF52832 Desktop Keyboard (nrf52kbd_nrf52832)

   .. tab:: HID dongle

      .. figure:: /images/nrf_desktop_dongle_no_captions.svg
         :alt: nRF Desktop dongle (top view)

      * nRF52840 USB Dongle (nrf52840dongle_nrf52840)
      * nRF52833 USB Dongle (nrf52833dongle_nrf52833)

   .. tab:: DK

      .. figure:: /images/nrf_desktop_nrf52840_dk_no_captions.svg
         :alt: DK

      * nRF52840 DK (nrf52840dk_nrf52840) - the application is configured to work as a gaming mouse (motion emulated using DK buttons)
      * nRF52833 DK (nrf52833dk_nrf52833) - the application is configured to work as a HID dongle

..

The application was designed to allow easy porting to new hardware.
Check :ref:`nrf_desktop_porting_guide` for more information.

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

The following build types are available in nRF Desktop:

* ``ZRelease`` -- Release version of the application with no debugging features.
* ``ZDebug`` -- Debug version of the application; the same as the ``ZRelease`` build type, but with debug options enabled.
* ``ZDebugWithShell`` -- ``ZDebug`` build type with the shell enabled.
* ``ZReleaseB0`` -- ``ZRelease`` build type with the support for the bootloader enabled.
* ``ZDebugB0`` -- ``ZDebug`` build type with the support for the bootloader enabled.

In nRF Desktop, not every board can support every build type.
If the given build type is not supported on the selected board, an error message will appear when `Building and running`_.
For example, if the ``ZDebugWithShell`` build type is not supported on the selected board, the following notification appears:

.. code-block:: console

  Configuration file for build type ZDebugWithShell is missing.

See :ref:`nrf_desktop_porting_guide` for detailed information about the application configuration and how to create build type files for your hardware.

User interface
**************

The nRF Desktop configuration files have a set of preprogrammed options bound to different parts of the hardware.
These options are related to the following functionalities:

* `Turning devices on and off`_
* `Connectability`_
* `System state indication`_
* `Debugging`_
* `Power Management`_

Turning devices on and off
==========================

The nRF Desktop hardware reference designs are equipped with hardware switches to turn the device on and off.
See the following figures for the exact location of these switches:

.. tabs::

   .. tab:: Gaming mouse

      .. figure:: /images/nrf_desktop_gaming_mouse_bottom.svg
         :alt: nRF Desktop gaming mouse (bottom view)

      The switch is located at the bottom side of the gaming mouse, close to the optical sensor.
      The mouse uses this switch also for changing dongle and Bluetooth LE peers, as described in the `Bluetooth LE peer control`_ section.

   .. tab:: Desktop mouse

      .. figure:: /images/nrf_desktop_desktop_mouse_bottom.svg
         :alt: nRF Desktop desktop mouse (bottom view)

      The switch is located at the bottom side of the desktop mouse, close to the optical sensor.

   .. tab:: Keyboard

      .. figure:: /images/nrf_desktop_keyboard_back_power.svg
         :alt: nRF Desktop keyboard (back view)

      The switch is located at the back side of the keyboard.

..

Connectability
==============

The nRF Desktop devices provide user input to the host in the same way as other mice and keyboards, using the following connection options:

* :ref:`nrf_desktop_usb`
* :ref:`nrf_desktop_ble`

The nRF Desktop devices support additional operations, like firmware upgrade or configuration change.
The support is implemented through the :ref:`nrf_desktop_config_channel`.
The host can use dedicated Python scripts to exchange the data with an nRF Desktop peripheral.
For detailed information, see the ``hid_configurator`` documentation.

To save power, the behavior of a device can change in time.
For more information, see the `Power Management` section.

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

      The dongle has an USB connector located at one and of the board.
      It should be inserted to the USB slot located on the host.

..

The following devices support the HID data transmission over USB:


* `Gaming mouse USB`_
* `Dongle USB`_
* `DK USB`_

Gaming mouse USB
~~~~~~~~~~~~~~~~

The gaming mouse can send HID data when connected through USB.
When the device is connected both wirelessly and through USB at the same time, it provides input only through the USB connection.
If the device is disconnected from USB, it automatically switches to sending the data wirelessly using Bluetooth LE.

The gaming mouse is a battery-powered device.
When it gets connected through the USB the rechargeable batteries charging is started.

Dongle USB
~~~~~~~~~~

The nRF Desktop dongle works as a bridge between the devices connected through the Bluetooth LE or Low Latency Packet Mode and the host connected through USB.
It receives data wirelessly from the connected peripherals and forwards the data to the host.

The nRF Desktop dongle is powered directly through USB.

DK USB
~~~~~~

The DK functionality depends on the application configuration.
Depending on the selected configuration options, it can work as a mouse or a dongle.

.. _nrf_desktop_ble:

Connection through Bluetooth LE
-------------------------------

When turned on, the nRF Desktop peripherals are advertising until they go to the suspended state or connect through Bluetooth.
The peripheral supports one wireless connection at a time, but it can be bonded with multiple peers.

The nRF Desktop Bluetooth Central device scans for all bonded peripherals that are not connected.
The scanning is interrupted when any device connected to the dongle through Bluetooth is in use.
Continuing the scanning in such scenario would cause report rate drop.

The scanning starts automatically when one of the bonded peers disconnects.
It also takes place periodically when a known peer is not connected.

The peripheral connection can be based on standard Bluetooth LE connection parameters or on Bluetooth LE with Low Latency Packet Mode (LLPM).
LLPM is a proprietary Bluetooth extension from Nordic Semiconductor.
It can be used only if it is supported by both connected devices (desktop mice do not support it).
LLPM enables sending data with high report rate (up to 1000 reports per second), which is not supported by the standard Bluetooth LE.

Bluetooth LE peer control
~~~~~~~~~~~~~~~~~~~~~~~~~

A connected Bluetooth LE peer device can be controlled using predefined buttons or button combinations.
There are several peer operations available.

The application distinguishes between the following button press types:

* Short -- Button pressed for less than 0.5 seconds.
* Standard -- Button pressed for more than 0.5 seconds, but less than 5 seconds.
* Long -- Button pressed for more than 5 seconds.
* Double -- Button pressed twice in quick succession.

The peer operation states provide visual feedback through LEDs (if the device has LEDs).
Each of the states is represented by separate LED color and effect.
The LED colors and effects are described in the :file:`led_state_def.h` file located in the board-specific directory in the application configuration folder.

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
            During the peer selection:

            1. Short-press to select the next peer.
               The LED1 changes color and starts blinking.
            #. Short-press to toggle between available peers.
               The LED1 changes color for each peer and keeps blinking.
            #. Double-press to confirm the peer selection.
               The peer is changed after the confirmation.
               The LED1 stops blinking.

               .. note::
                   |led_note|

          * Long-press to initialize the peer erase.
            When the LED1 starts blinking rapidly, double-press to confirm the operation.
            |nRF_Desktop_confirmation_effect|
          * |nRF_Desktop_cancel_operation|

   .. tab:: Desktop mouse

      The following predefined buttons are assigned to peer control operations for the desktop mouse:

      Scroll wheel button
        * Long-press to initialize and confirm the peer erase.
          The scroll must be pressed before the mouse is powered up.

          .. figure:: /images/nrf_desktop_desktop_mouse_side_scroll.svg
             :alt: nRF Desktop desktop mouse - side view

             nRF Desktop desktop mouse - side view

          |nRF_Desktop_confirmation_effect|
        * |nRF_Desktop_cancel_operation|

   .. tab:: Keyboard

      The following predefined buttons or button combinations are assigned to peer control operations for the keyboard:

      Page Down key
        * The Page Down key must be pressed while keeping the Fn modifier key pressed.

          .. figure:: /images/nrf_desktop_keyboard_top.svg
             :alt: nRF Desktop keyboard - top view

             nRF Desktop keyboard - top view

        * Short-press the Page Down key to initialize the peer selection.
          During the peer selection:

          1. Short-press to select the next peer.
             The LED1 changes color to red and starts blinking.
          #. Short-press to toggle between available peers.
             The LED1 blinks rapidly for each peer.
             The amount of blinks corresponds to the number assigned to a peer: one blink for peer 1, two blinks for peer 2, and so on.
          #. Double-press to confirm the peer selection.
             The peer is changed after the confirmation.
             The LED1 becomes solid for a short time and then turns itself off.

             .. note::
                  |led_note|

        * Long-press to initialize the peer erase.
          When the LED1 starts blinking rapidly, double-press to confirm the operation.
          |nRF_Desktop_confirmation_effect|
        * |nRF_Desktop_cancel_operation|

   .. tab:: HID dongle

      The following predefined buttons are assigned to peer control operations for the HID dongle:

      SW1 button
        * The SW1 button is located on the top of the dongle, on the same side as LED2.

          .. figure:: /images/nrf_desktop_dongle_front_led2_sw1.svg
             :alt: nRF Desktop dongle - top view

             nRF Desktop dongle - top view

        * Long-press to initialize peer erase.
          When the LED2 starts blinking rapidly, double-press to confirm the operation.
          After the confirmation, all the Bluetooth bonds are removed for the dongle.
        * Short-press to start scanning for both bonded and non-bonded Bluetooth peripherals.
          The scan is interrupted if another peripheral connected to the dongle is in use.
        * |nRF_Desktop_cancel_operation|

..

System state indication
=======================

When available, an LED can be used to indicated the state of the device.
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

      The system state LED1 is located in the bottom right corner of the dongle, next to the USB connector.

..

In case of a system error, the system state LED will start to blink rapidly for some time before the device is reset.

Debugging
=========

Each of the nRF Desktop hardware reference designs has a slot for a dedicated debug board.
See the following figures for the exact location of these slots.

.. tabs::

   .. tab:: Gaming mouse

      .. figure:: /images/nrf_desktop_gaming_mouse_debug_board_slot.svg
         :alt: nRF Desktop gaming mouse (top view)

      The debug slot is located at the back of the gaming mouse, below the cover.

   .. tab:: Desktop mouse

      .. figure:: /images/nrf_desktop_desktop_mouse_side_debug.svg
         :alt: nRF Desktop desktop mouse (side view)

      The debug slot is located on the side of the desktop mouse.
      It is accesible through the whole in the casing.

   .. tab:: Keyboard

      .. figure:: /images/nrf_desktop_keyboard_back_debug.svg
         :alt: nRF Desktop keyboard (back view)

      The debug slot is located on the back of the keyboard.

..

The boards that you can plug into these slots are shown below.

The debug board can be used for programming the device (and powering it).
The bypass boards are meant for testing.
Their purpose is to close the circuits, which allows the device to be powered.


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

You can define the amount of time in ``CONFIG_DESKTOP_POWER_MANAGER_TIMEOUT``.
By default, it is set to 120 seconds.

.. note::
    When the gaming mouse is powered from USB, the power down time-out functionality is disabled.

Building and running
********************

.. |sample path| replace:: :file:`applications/nrf_desktop`

The nRF Desktop application is built in the same way to any other NCS application or sample.

.. include:: /includes/build_and_run.txt

.. _nrf_desktop_selecting_build_types:

Selecting a build type
======================

Before you start testing the application, you can select one of the :ref:`nrf_desktop_requirements_build_types`, depending on your board and building method.

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

Testing
=======

The application can be built and tested in various configurations.
The following procedure refers to the scenario where the gaming mouse (nRF52840 Gaming Mouse) and the keyboard (nRF52832 Desktop Keyboard) are connected simultaneously to the dongle (nRF52840 USB Dongle).

After building the application with or without :ref:`specifying the build types <nrf_desktop_selecting_build_types>`, test the nRF Desktop application by performing the following steps:

1. Program the required firmware to each device.
#. Turn on both the mouse and the keyboard.
   The LED on the keyboard and the LED on the mouse (located under the logo) both start breathing.
#. Plug the dongle to the USB port.
   The blue LED on the dongle starts breathing.
   This indicates that the dongle is scanning for peripherals.
#. Wait for the establishment of the Bluetooth connection, which happens automatically.
   After the Bluetooth connection is established, the LEDs stop breathing and remain turned on.
   The devices can then be used simultaneously.

   .. note::
        You can manually start the scanning for new peripheral devices by pressing the `SW1` button on the dongle for a short time.
        This might be needed if the dongle does not connect with all the peripherals before time-out.
        The scanning is interrupted after a predefined amount of time, because it negatively affects the performance of already connected peripherals.

#. Move the mouse and press any key on the keyboard.
   The input is reflected on the host.

    .. note::
        When a configuration with debug features is enabled, for example the logger and assertions, the gaming mouse report rate can be significantly lower.

        Make sure that you use the release configurations before testing the mouse report rate.
        For the release configurations, you should observe a 500-Hz report rate when both mouse and keyboard are connected and a 1000-Hz rate when only mouse is connected.

#. Switch the Bluetooth peer on the gaming mouse by pressing the Precise Aim button.
   The gaming mouse LED color changes from red to green and the LED starts blinking rapidly.
#. Press the Precise Aim button twice quickly to confirm the selection.
   After the confirmation, the LED starts breathing and the mouse starts the Bluetooth advertising.
#. Connect to the mouse with an Android phone, a laptop, or any other Bluetooth Central.
   After the connection is established, you can use the mouse with the connected device.

.. _nrf_desktop_porting_guide:

Integrating your own hardware
*****************************

This section describes how to adapt the nRF Desktop application to different hardware.
It describes the configuration sources that are used for the default configuration, and lists steps required for adding a new board:

    * `Configuration sources`_
    * `Board configuration`_

        * `nRF Desktop board configuration files`_

    * `Adding a new board`_

Configuration sources
=====================

The nRF Desktop project uses the following files as sources of its configuration:

    * DTS files -- reflect the hardware configuration (see :ref:`zephyr:dt-guide`).
    * ``_def`` files -- contain configuration arrays for the application modules and are specific to the nRF Desktop project.
    * Kconfig files -- reflect the software configuration (see :ref:`kconfig_tips_and_tricks`).

You must modify these configuration sources when `Adding a new board`_.
For information about differences between DTS files and Kconfig, see :ref:`zephyr:dt_vs_kconfig`.

Board configuration
===================

The nRF Desktop application is modular.
Depending on requested functions, it can provide mouse, keyboard, or dongle functionality.
The selection of modules depends on the chosen role and also on the selected board.
For more information about modules available for each configuration, see :ref:`nrf_desktop_architecture`.

For a board to be supported by the application, you must provide a set of configuration files at ``applications/nrf_desktop/configuration/your_board_name``.
The application configuration files define both a set of options with which the nRF Desktop project will be created for your board and the selected :ref:`nrf_desktop_requirements_build_types`.
Include the following files in this directory:

Mandatory configuration files
    * Application configuration file for the ``ZDebug`` build type.
    * Configuration files for the selected modules.

Optional configuration files
    * Application configuration files for other build types.
    * Configuration file for the bootloader.
    * DTS overlay file.

See `Adding a new board`_ for information about how to add these files.

nRF Desktop board configuration files
-------------------------------------

The nRF Desktop project comes with configuration files for the following boards:

nRF52840 Gaming Mouse (nrf52840gmouse_nrf52840)
      * The board is defined in ``nrf/boards/arm/nrf52840gmouse_nrf52840`` for the project-specific hardware.
      * To achieve gaming-grade performance:

        * The application is configured to act as a gaming mouse, with both Bluetooth LE and USB transports enabled.
        * Bluetooth is configured to use Nordic's proprietary link layer.

      * |preconfigured_build_types|

nRF52832 Desktop Mouse (nrf52dmouse_nrf52832) and nRF52810 Desktop Mouse (nrf52810dmouse_nrf52810)
      * Both boards are meant for the project-specific hardware and are defined in ``nrf/boards/arm/nrf52dmouse_nrf52832`` and ``nrf/boards/arm/nrf52810dmouse_nrf52810``, respectively.
      * The application is configured to act as a mouse.
      * Only the Bluetooth Low Energy transport is enabled.
        Bluetooth uses Zephyr's software link layer.
      * There is no configuration with bootloader available.

Sample mouse or keyboard (nrf52840dk_nrf52840)
      * The configuration uses the nRF52840 Development Kit.
      * The build types allow to build the application both as mouse or as keyboard.
      * Inputs are simulated based on the hardware button presses.
      * The configuration with bootloader is available.

nRF52832 Desktop Keyboard (nrf52kbd_nrf52832)
      * The board used is defined in ``nrf/boards/arm/nrf52kbd_nrf52832`` for the project-specific hardware.
      * The application is configured to act as a keyboard, with the Bluetooth LE transport enabled.
      * Bluetooth is configured to use Nordic's proprietary link layer.
      * |preconfigured_build_types|

nRF52840 USB Dongle (nrf52840dongle_nrf52840)
      * This configuration uses Nordic's nRF52840 dongle defined in Zephyr.
      * Since the board is generic, project-specific changes are applied in the DTS overlay file.
      * The application is configured to act as both mouse and keyboard.
      * Bluetooth uses Nordic's proprietary link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * |preconfigured_build_types|

.. _porting_guide_adding_board:

Adding a new board
==================

When adding a new board for the first time, focus on a single configuration.
Moreover, keep the default ``ZDebug`` build type that the application is built with, and do not add any additional build type parameters.

.. note::
    The following procedure uses the gaming mouse configuration as an example.

To use the nRF Desktop project with your custom board:

1. Define the board by copying the nRF Desktop board files that are the closest match for your hardware.
   For example, for gaming mouse use ``nrf/boards/arm/nrf52840gmouse_nrf52840``.
#. Edit the DTS files to make sure they match the hardware configuration.
   Pay attention to pins that are used and to the bus configuration for optical sensor.
#. Edit the board's Kconfig files to make sure they match the required system configuration.
   For example, disable the drivers that will not be used by your device.
#. Copy the project files for the device that is the closest match for your hardware.
   For example, for gaming mouse these are located at ``applications/nrf_desktop/configure/nrf52840gmouse_nrf52840``.
#. Optionally, depending on the board, edit the DTS overlay file.
   This step is not required if you have created a new board and its DTS files fully describe your hardware.
   In such case, the overlay file can be left empty.
#. In Kconfig, ensure that the following modules that are specific for gaming mouse are enabled:

    * :ref:`nrf_desktop_motion`
    * :ref:`nrf_desktop_wheel`
    * :ref:`nrf_desktop_buttons`
    * :ref:`nrf_desktop_battery_meas`
    * :ref:`nrf_desktop_leds`

#. For each module enabled, change its ``_def`` file to match your hardware:

   Motion module
     * The ``nrf52840gmouse_nrf52840`` uses the PMW3360 optical motion sensor.
       The sensor is configured in DTS, and the sensor type is selected in the application configuration.
       To add a new sensor, expand the application configuration.
   Wheel module
     * The wheel is based on the QDEC peripheral of the nRF52840 device and the hardware-related part is configured in DTS.
   Buttons module
     * To simplify the configuration of arrays, the nRF Desktop project uses ``_def`` files.
     * The ``_def`` file of the buttons module contains pins assigned to rows and columns.
   Battery measurement module
     * The ``_def`` file of the battery measurement module contains the mapping needed to match the voltage read from ADC to the battery level.
   LEDs module
     * The application uses two logical LEDs -- one for the peers state, and one for the system state indication.
     * Each of the logical LEDs can have either one (monochromatic) or three color channels (RGB).
       Such color channel is a physical LED.
     * The project uses PWM channels to control the brightness of each physical LED.
       The PWM peripheral must be configured in DTS files, and the ``_def`` file of the LEDs module must be configured to indicate which PWM channel is assigned to each LED color.
       Ensure that PWM channels are correctly configured in DTS and PWM driver is enabled in Kconfig file.

#. Review Bluetooth options in Kconfig:

   * Ensure that the Bluetooth role is configured appropriately.
     For mouse, it should be configured as peripheral.
   * Update the configuration related to peer control.
     You can also disable the peer control using the ``CONFIG_DESKTOP_BLE_PEER_CONTROL`` option.
     Peer control details are described in the :ref:`nrf_desktop_ble_bond` module documentation.

   Refer to the :ref:`nrf_desktop_bluetooth_guide` and :ref:`zephyr:bluetooth` sections for more detailed information about the Bluetooth configuration.
#. Edit Kconfig to disable options that you do not use.
   Some options have dependencies that might not be needed when these options are disabled.
   For example, when the LEDs module is disabled, the PWM driver is not needed.

.. _nrf_desktop_flash_memory_layout:

Flash memory layout
===================

Depending on whether the bootloader is enabled, the partition layout on the flash memory is set by defining either:

* `Memory layout in DTS`_ (bootloader is not enabled)
* `Memory layout in partition manager`_ (bootloader is enabled)

The set of required partitions differs depending on the configuration:

* There must be at least one partition where the code is stored.
* There must be one partition to store settings.
* The bootloader, if enabled, will add additional partitions to the set.


Memory layout in DTS
--------------------

When using the flash memory layout in the DTS files, define the ``partitions`` child node in the flash device node (``&flash0``).

Since the nRF Desktop project uses the partition manager when the bootloader is present, the partition definition from the DTS files is valid only for configurations without the bootloader.

.. note::
    If you wish to change the default flash memory layout of the board without editing board-specific files, edit the DTS overlay file.
    The nRF Desktop project automatically adds the overlay file if the ``dts.overlay`` file is present in the project's board configuration directory.
    See more in the `Board configuration`_ section.

.. warning::
    By default, Zephyr does not use the code partition defined in the DTS files.
    It is only used if :option:`CONFIG_USE_DT_CODE_PARTITION` is enabled.
    If this option is disabled, the code is loaded at the address defined by :option:`CONFIG_FLASH_LOAD_OFFSET` and can spawn for :option:`CONFIG_FLASH_LOAD_SIZE` (or for the whole flash if the load size is set to zero).

Since the nRF Desktop project depends on the DTS layout only when bootloader is not used, only the settings partition is relevant and other partitions are ignored.

For more information about how to configure the flash memory layout in the DTS files, see :ref:`zephyr:legacy_flash_partitions`.

Memory layout in partition manager
----------------------------------

When the bootloader is enabled, the nRF Desktop project uses the partition manager for the layout configuration of the flash memory.
The project uses the static configuration of partitions.
Add the ``pm_static_${CMAKE_BUILD_TYPE}.yml`` partition manager configuration file to the project's board configuration directory to use this configuration.

For more information about how to configure the flash memory layout using the partition manager, see :ref:`partition_manager`.

.. _nrf_desktop_bluetooth_guide:

Bluetooth in nRF Desktop
========================

The nRF Desktop devices use the Zephyr Bluetooth API (:ref:`zephyr:bluetooth`) to handle the Bluetooth LE connections.

The Zephyr Bluetooth API is used only by the application modules that handle Bluetooth LE connections.
The information about peer and connection state is propagated to other application modules using :ref:`event_manager` events.

There are two types of nRF Desktop devices:

* Peripheral devices (mouse or keyboard)

  * Support only the Bluetooth peripheral role (:option:`CONFIG_BT_PERIPHERAL`).
  * Handle only one Bluetooth LE connection at a time.
  * Use more than one Bluetooth local identity.

* Central devices (dongle)

  * Support only the Bluetooth central role (:option:`CONFIG_BT_CENTRAL`).
  * Handle multiple Bluetooth LE connections simultaneously.
  * Use only one Bluetooth local identity (the default one).

Both central and peripheral devices have dedicated configuration options and use dedicated modules.
There is no nRF Desktop device that supports both central and peripheral roles.

Common configuration and application modules
--------------------------------------------

Some Bluetooth-related :ref:`configuration options <nrf_desktop_bluetooth_guide_configuration>` and application modules are common for every nRF Desktop device.

.. _nrf_desktop_bluetooth_guide_configuration:

Configuration
~~~~~~~~~~~~~

This section describes the most important Bluetooth Kconfig options common for all nRF Desktop devices.
For detailed information about every option, see the Kconfig help.

* :option:`CONFIG_BT_MAX_PAIRED`

  * nRF Desktop central - The maximum number of paired devices is equal to the maximum number of simultaneously connected peers.
  * nRF Desktop peripheral - The maximum number of paired devices is equal to the number of peers plus one, where the one additional paired device slot is used for erase advertising.

* :option:`CONFIG_BT_ID_MAX`

  * nRF Desktop central - The device uses only one Bluetooth local identity, that is the default one.
  * nRF Desktop peripheral - The number of Bluetooth local identities must be equal to the number of peers plus by two.

    * One additional local identity is used for erase advertising.
    * The other additional local identity is the default local identity, which is unused, because it cannot be reset after removing the bond.
      Without the identity reset, the previously bonded central could still try to reconnect after being removed from Bluetooth bonds on the peripheral side.

* :option:`CONFIG_BT_MAX_CONN`

  * nRF Desktop central - The option must be set to the maximum number of simultaneously connected devices.
  * nRF Desktop peripheral - The default value (one) is used.

.. note::
   After changing the number of Bluetooth peers for the nRF Desktop peripheral device, you must update the LED effects used to represent the Bluetooth connection state.
   For details, see :ref:`nrf_desktop_led_state`.

The nRF Desktop devices use one of the following Link Layers:

* :option:`CONFIG_BT_LL_NRFXLIB` that supports the Low Latency Packet Mode (LLPM).
* :option:`CONFIG_BT_LL_SW_SPLIT` that does not support the LLPM and has a lower memory usage, so it can be used by memory-limited devices.


Application modules
~~~~~~~~~~~~~~~~~~~

Every nRF Desktop device that enables Bluetooth must handle connections and manage bonds.
These features are implemented by the following modules:

* :ref:`nrf_desktop_ble_state` - Enables Bluetooth and LLPM (if supported), and handles Zephyr connection callbacks.
* :ref:`nrf_desktop_ble_bond` - Manages Bluetooth bonds and local identities.

|enable_modules|

.. note::
   The nRF Destkop devices enable :option:`CONFIG_BT_SETTINGS`.
   When this option is enabled, the application is responsible for calling the :cpp:func:`settings_load` function - this is handled by the :ref:`nrf_desktop_settings_loader`.

Bluetooth Peripheral
--------------------

The nRF Desktop peripheral devices must include additional configuration options and additional application modules to comply with the HID over GATT specification.

The HID over GATT profile specification requires Bluetooth peripherals to define the following GATT Services:

* HID Service - Handled in the :ref:`nrf_desktop_hids`.
* Battery Service - Handled in the :ref:`nrf_desktop_bas`.
* Device Information Service - Implemented in Zephyr and enabled with :option:`CONFIG_BT_GATT_DIS`.
  Can be configured using Kconfig options with the ``CONFIG_BT_GATT_DIS`` prefix.

The nRF Desktop peripherals must also define a dedicated GATT Service, which is used to inform whether the device supports the LLPM Bluetooth extension.
The GATT Service is implemented by the :ref:`nrf_desktop_dev_descr`.

Apart from the GATT Services, an nRF Desktop peripheral device must enable and configure the following application modules:

* :ref:`nrf_desktop_ble_adv` - Controls the Bluetooth advertising.
* :ref:`nrf_desktop_ble_latency` - Keeps the connection latency low when the :ref:`nrf_desktop_config_channel` is being used, in order to ensure quick transfer of configuration data.

Bluetooth Central
-----------------

The nRF Desktop central must implement Bluetooth scanning and handle the GATT operations.
Both these features are implemented by the following application modules:

* :ref:`nrf_desktop_ble_scan` - Controls the Bluetooth scanning.
* :ref:`nrf_desktop_ble_discovery` - Handles discovering and reading the GATT Characteristics from the connected peripheral.
* :ref:`nrf_desktop_hid_forward` - Subscribes for HID reports from the Bluetooth peripherals (HID over GATT) and forwards the data using application events.

|enable_modules|

Optionally, you can also enable the following module:

* :ref:`nrf_desktop_ble_qos` - Helps achieve better connection quality and higher report rate.
  The module can be used only with the nrfxlib Link Layer.

.. _nrf_desktop_bootloader:

Bootloader and DFU
==================

The nRF Desktop application uses Secure Bootloader, referred in this documentation as the B0.

The B0 is a small, simple and secure bootloader that allows the application to boot directly from one of the application slots, thus increasing the speed of the direct firmware upgrade (DFU) process.
More information about the B0 can be found at :ref:`bootloader`.

Enabling the bootloader
    To enable the B0 bootloader, select the :option:`CONFIG_SECURE_BOOT` Kconfig option.
    If this option is not selected, the application will be built without the bootloader, and the DFU will not be available.

Required options
    The B0 bootloader requires the following options enabled:

    * :option:`CONFIG_SB_SIGNING_KEY_FILE` - required for providing the signature used for image signing and verification.
    * :option:`CONFIG_FW_INFO` - required for the application versioning information.
    * :option:`CONFIG_FW_INFO_FIRMWARE_VERSION` -  enable this option to set the version of the application after you enabled :option:`CONFIG_FW_INFO`.
    * :option:`CONFIG_BUILD_S1_VARIANT` - required for the build system to be able to construct the application binaries for both application's slots in flash memory.

.. note::
    The nRF Desktop application does not support MCUboot as a second stage bootloader.

    However, the MCUboot can be enabled instead of the B0.
    Although the nRF Desktop application does not use MCUboot for background DFU, it can be a preferred choice for devices where the USB serial recovery is to be used instead (for example, due to flash memory size limitations).

Device Firmware Upgrade
-----------------------

The nRF Desktop application uses the background image transfer for the DFU process.
The firmware update process has the following stages:

* `Update image generation`_
* `Update image transfer`_
* `Update image verification and swap`_

At the end of these three stages, the nRF Desktop application will be rebooted with the new firmware package installed.

Update image generation
~~~~~~~~~~~~~~~~~~~~~~~

The update image is automatically generated if the bootloader is enabled.
By default, the build process will construct an image for the first slot (slot 0 or S0).
To ensure that application is built for both slots, select the :option:`CONFIG_BUILD_S1_VARIANT` Kconfig option.

After the build process completes, both images can be found in the build directory, in the :file:`zephyr` subdirectory:

* :file:`signed_by_b0_s0_image.bin` is meant to be flashed at slot 0.
* :file:`signed_by_b0_s1_image.bin` is meant to be flashed at slot 1.

For the DFU process, use :file:`dfu_application.zip`.
This package contains both images along with additional metadata.
It allows the update tool to select the right image depending on the image that is currently running on the device.

Update image transfer
~~~~~~~~~~~~~~~~~~~~~

The update image is transmitted in the background through the :ref:`nrf_desktop_config_channel`.
The configuration channel data is transmitted either through USB or Bluetooth, using HID feature reports.
This allows the device to be used normally during the whole process (that is, the device does not need to enter any special state in which it becomes non-responsive to the user).

Depending on the side on which the process is handled:

* On the application side, the process is handled by :ref:`nrf_desktop_dfu`.
  See the module documentation for how to enable and configure it.
* On the host side, the process is handled by the :ref:`nrf_desktop_config_channel_script`.
  See the tool documentation for more information about how to execute the DFU process on the host.

Update image verification and swap
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Once the update image transfer is completed, the DFU process will continue after the device reboot.
If ``hid_configurator`` is used, the reboot is performed right after the image transfer completes.

After the reboot, the bootloader locates the update image on the update partition of the device.
The image verification process ensures the integrity of the image and checks if its signature is valid.
If verification is successful, the bootloader boots the new version of the application.
Otherwise, the old version is used.

Dependencies
************

This application uses the following |NCS| libraries and drivers:

    * :ref:`event_manager`
    * :ref:`profiler`
    * :ref:`hids_readme`
    * :ref:`hids_c_readme`
    * :ref:`nrf_bt_scan_readme`
    * :ref:`gatt_dm_readme`
    * ``drivers/sensor/paw3212``
    * ``drivers/sensor/pmw3360``


.. _nrf_desktop_app_internal_modules:

Application internal modules
****************************

This application uses its own set of internal modules.
For more information about each application module and its configuration details, see the following pages:

.. toctree::
   :maxdepth: 1

   doc/main.rst
   doc/battery_charger.rst
   doc/battery_meas.rst
   doc/board.rst
   doc/buttons.rst
   doc/buttons_sim.rst
   doc/click_detector.rst
   doc/dev_descr.rst
   doc/motion.rst
   doc/passkey.rst
   doc/selector.rst
   doc/wheel.rst
   doc/leds.rst
   doc/bas.rst
   doc/ble_adv.rst
   doc/ble_bond.rst
   doc/ble_discovery.rst
   doc/ble_latency.rst
   doc/ble_qos.rst
   doc/ble_scan.rst
   doc/ble_state.rst
   doc/config_channel.rst
   doc/fn_keys.rst
   doc/hid_forward.rst
   doc/hid_state.rst
   doc/hids.rst
   doc/info.rst
   doc/led_state.rst
   doc/led_stream.rst
   doc/power_manager.rst
   doc/settings_loader.rst
   doc/usb_state.rst
   doc/watchdog.rst
   doc/dfu.rst
   doc/constlat.rst
   doc/hfclk_lock.rst

Source and sink modules
=======================

The following page includes lists of source and sink modules for events that have many listeners or sources.
These were gathered on a single page to simplify the event propagation tables.

.. toctree::
   :maxdepth: 1

   doc/event_rel_modules.rst

.. |nRF_Desktop_confirmation_effect| replace:: After the confirmation, Bluetooth advertising using a new local identity is started.
   When a new Bluetooth Central device successfully connects and bonds, the old bond is removed and the new bond is used instead.
   If the new peer does not connect in the predefined period of time, the advertising ends and the application switches back to the old peer.

.. |nRF_Desktop_cancel_operation| replace:: You can cancel the ongoing peer operation with a standard button press.

.. |preconfigured_build_types| replace:: The preconfigured build types configure the device with or without the bootloader and in debug or release mode.

.. |enable_modules| replace:: You need to enable all these modules to enable both features.
   For information about how to enable the modules, see their respective documentation pages.

.. |hid_state| replace:: HID state module

.. |led_note| replace:: A breathing LED1 indicates that the device has entered the advertising mode.
   This happens when the device looks for a peer to connect to.
