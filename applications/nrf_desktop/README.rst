.. _nrf_desktop:

nRF Desktop
###########

.. contents::
   :local:
   :depth: 2

The nRF Desktop is a reference design of a Human Interface Device (HID) that is connected to a host through Bluetooth® Low Energy or USB, or both.
Depending on the configuration, this application can work as desktop mouse, gaming mouse, keyboard, or connection dongle.
See `nRF Desktop reference design page`_ for an overview of supported features.

.. tip::
    To get started with hardware that has pre-configured software, go to the `User interface`_ section.

The nRF Desktop application supports common input hardware interfaces like motion sensors, rotation sensors, and buttons scanning module.
You can configure the firmware at runtime using a dedicated configuration channel established with the HID feature report.
The same channel is used to transmit DFU packets.

.. _nrf_desktop_architecture:

Overview: Firmware architecture
*******************************

The nRF Desktop application design aims at high performance, while still providing configurability and extensibility.

The application architecture is modular, event-driven and build around :ref:`lib_caf`.
This means that parts of the application functionality are separated into isolated modules that communicate with each other using application events, which are handled by the :ref:`app_event_manager`.
Modules register themselves as listeners of those events that they are configured to react to.
An application event can be submitted by multiple modules and it can have multiple listeners.

Module and component overview
=============================

The following figure shows the nRF Desktop modules and how they relate with other components and the :ref:`app_event_manager`.
The figure does not present all the available modules.
For example, the figure does not include the modules that are used as hotfixes or only for debug or profiling purposes.

.. figure:: /images/nrf_desktop_arch.svg
   :alt: nRF Desktop high-level design (overview)

   Application high-level design overview

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
   In such case, the table shows the :ref:`app_event_manager` events received and submitted by all implementations of a given application module.

Module usage per hardware type
==============================

Since the application architecture is uniform and the code is shared, the set of modules in use depends on the selected device role.
A different set of modules is enabled when the application is working as mouse, keyboard, or dongle.
In other words, not all of the :ref:`nrf_desktop_app_internal_modules` need to be enabled for a given reference design.

Gaming mouse module set
-----------------------

The following figure shows the modules that are enabled when the application is working as a gaming mouse:

.. figure:: /images/nrf_desktop_arch_gmouse.svg
   :alt: nRF Desktop high-level design (gaming mouse)

   Application configured as a gaming mouse

Desktop mouse module set
------------------------

The following figure shows the modules that are enabled when the application is working as a desktop mouse:

.. figure:: /images/nrf_desktop_arch_dmouse.svg
   :alt: nRF Desktop high-level design (desktop mouse)

   Application configured as a desktop mouse

Keyboard module set
-------------------

The following figure shows the modules that are enabled when the application is working as a keyboard:

.. figure:: /images/nrf_desktop_arch_kbd.svg
   :alt: nRF Desktop high-level design (keyboard)

   Application configured as a keyboard

Dongle module set
-----------------

The following figure shows the modules that are enabled when the application is working as a dongle:

.. figure:: /images/nrf_desktop_arch_dongle.svg
   :alt: nRF Desktop high-level design (dongle)

   Application configured as a dongle

Thread usage
============

The application limits the number of threads in use to the minimum and does not use the user-space threads.

The following threads are kept running in the application:

* System-related threads
    * Idle thread
    * System workqueue thread
    * Logger thread (when :ref:`zephyr:logging_api` is enabled)
    * Shell thread (when :ref:`zephyr:shell_api` is enabled)
    * Threads related to Bluetooth® LE (the exact number depends on the selected Link Layer)
* Application-related threads
    * Motion sensor thread (running only on mouse)
    * Settings loading thread (enabled by default only on keyboard)
    * QoS data sampling thread (running only if Bluetooth® LE QoS feature is enabled)

Most of the application activity takes place in the context of the system work queue thread, either through scheduled work objects or through the Application Event Manager callbacks (executed from the system workqueue thread).
Because of this, the application does not need to handle resource protection.
The only exception are places where the interaction with interrupts or multiple threads cannot be avoided.

Memory allocation
=================

Most of memory resources that are used by the application are allocated statically.

The application uses dynamic allocation to:

* Create the Application Event Manager events.
  For more information, see the :ref:`app_event_manager` page.
* Temporarily store the HID-related data in the :ref:`nrf_desktop_hid_state` and :ref:`nrf_desktop_hid_forward`.
  For more information, see the documentation pages of these modules.

When configuring HEAP, make sure that the values for the following options match the typical event size and the system needs:

* :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` - The size must be big enough to handle the worst possible use case for the given device.

.. important::
    The nRF Desktop uses ``k_heap`` as the backend for dynamic allocation.
    This backend is used by default in Zephyr.
    For more information, refer to Zephyr's documentation about :ref:`zephyr:heap_v2`.

HID data handling
=================

The nRF Desktop device and the host can exchange HID data using one of the following HID report types:

* HID input report
* HID output report
* HID feature report

The nRF Desktop application uses all of these report types.
See sections below for details about handling given HID report type.

HID input reports
-----------------

The nRF Desktop application uses HID input reports to transmit information about user input from the nRF Desktop device to a host.
The user input can be, for example, button press or mouse motion.

The nRF Desktop supports the following HID input reports:

* HID mouse report
* HID keyboard report
* HID consumer control report
* HID system control report

Every of these reports uses predefined report format and provides the given information.
For example, the mouse motion is forwarded as HID mouse report.

An nRF Desktop device supports the selected subset of the HID input reports.
For example, the nRF Desktop keyboard reference design (nrf52kbd_nrf52832) supports HID keyboard report, HID consumer control report and HID system control report.

As an example, the following section describes handling HID mouse report data.

HID mouse report handling
~~~~~~~~~~~~~~~~~~~~~~~~~

The nRF Desktop mouse sends HID input reports to the host after the host connects and subscribes for the HID reports.

The :ref:`nrf_desktop_motion` sensor sampling is synchronized with sending the HID mouse input reports to the host.

The :ref:`nrf_desktop_wheel` and :ref:`caf_buttons` provide data to the :ref:`nrf_desktop_hid_state` when the mouse wheel is used or a button is pressed, respectively.
These inputs are not synchronized with the HID report transmission to the host.

When the mouse is constantly in use, the motion module is kept in the fetching state.
In this state, the nRF Desktop mouse forwards the data from the motion sensor to the host in the following way:

1. USB state (or Bluetooth HIDS) sends a HID mouse report to the host and submits ``hid_report_sent_event``.
#. The event triggers sampling of the motion sensor.
#. A dedicated thread is used to fetch the sample from the sensor.
#. After the sample is fetched, the thread forwards it to the :ref:`nrf_desktop_hid_state` as ``motion_event``.
#. The |hid_state| updates the HID report data, generates new HID input report, and submits it as ``hid_report_event``.
#. The HID report data is forwarded to the host either by the :ref:`nrf_desktop_usb_state` or by the :ref:`nrf_desktop_hids`.
   The USB state has precedence if the USB is connected.
#. When the HID input report is sent to the host, ``hid_report_sent_event`` is submitted.
   The motion sensor sample is triggered and the sequence repeats.

If the device is connected through Bluetooth, the :ref:`nrf_desktop_hid_state` uses a pipeline that consists of two HID reports, which it creates upon receiving the first ``motion_event``.
The |hid_state| submits two ``hid_report_event`` events.
Sending the first event to the host triggers the motion sensor sample.

For the Bluetooth connections, submitting ``hid_report_sent_event`` is delayed by one Bluetooth connection interval.
Because of this delay, the :ref:`nrf_desktop_hids` requires pipeline of two HID reports to make sure that data is sent on every connection event.
Such solution is necessary to achieve high report rate.

If there is no motion data for the predefined number of samples, the :ref:`nrf_desktop_motion` goes to the idle state.
This is done to reduce the power consumption.
When a motion is detected, the module switches back to the fetching state.

The following diagram shows the data exchange between the application modules.
To keep it simple, the diagram only shows data related to HID input reports that are sent after the host is connected and the HID subscriptions are enabled.

.. figure:: /images/nrf_desktop_motion_sensing.svg
   :alt: nRF Desktop mouse HID data sensing and transmission

   nRF Desktop mouse HID data sensing and transmission

HID output reports
------------------

HID output reports are used to transmit data from host to an nRF Desktop device.
The nRF Desktop supports the HID keyboard LED report.
The report is used by the host to update the state of the keyboard LEDs, for example to indicate that the Caps Lock key is active.

.. note::
   Only the nrf52840dk_nrf52840 in ``keyboard`` configuration has hardware LEDs that can be used to display the Caps Lock and Num Lock state.

The following diagrams show the HID output report data exchange between the application modules.

* Scenario 1: Peripheral connected directly to the host

  .. figure:: /images/nrf_desktop_peripheral_host.svg
     :alt: HID output report: Data handling and transmission between host and peripheral

     HID output report: Data handling and transmission between host and peripheral

  In this scenario, the HID output report is sent from the host to the peripheral either through Bluetooth or the USB connection.
  Depending on the connection, the HID report is received by the :ref:`nrf_desktop_hids` or :ref:`nrf_desktop_usb_state`, respectively.
  The module then sends the HID output report as ``hid_report_event`` to the :ref:`nrf_desktop_hid_state`, which keeps track of the HID output report states and updates state of the hardware LEDs by sending ``led_event`` to :ref:`nrf_desktop_leds`.

* Scenario 2: Dongle intermediates between the host and the peripheral

  .. figure:: /images/nrf_desktop_peripheral_host_dongle.svg
     :alt: HID output report: Data handling and transmission between host and peripheral through dongle

     HID output report: Data handling and transmission between host and peripheral through dongle

  In this scenario, the HID output report is sent from the host to the dongle using the USB connection and is received by the :ref:`nrf_desktop_usb_state`.
  The destination module then sends the HID output report as ``hid_report_event`` to the :ref:`nrf_desktop_hid_forward`, which sends it to the peripheral using Bluetooth.

HID feature reports
-------------------

HID feature reports are used to transmit data between the host and an nRF Desktop device (in both directions).
The nRF Desktop uses only one HID feature report: the user config report.
The report is used by the :ref:`nrf_desktop_config_channel`.

.. note::
   The nRF Desktop also uses a dedicated HID output report to forward the :ref:`nrf_desktop_config_channel` data through the nRF Desktop dongle.
   This report is handled using the configuration channel's infrastructure and it can be enabled using :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_OUT_REPORT <config_desktop_app_options>`.
   See the Kconfig option's help for details about the report.

HID protocols
-------------

The following HID protocols are supported by nRF Desktop for HID input reports and HID output reports:

* Report protocol - Most widely used in HID devices.
  When establishing connection, host reads a HID descriptor from the HID device.
  HID descriptor describes format of HID reports and is used by the host to interpret data exchanged between HID device and host.
* Boot protocol - Only available for mice and keyboards data.
  No HID descriptor is used for this HID protocol.
  Instead, fixed data packet formats must be used to send data between the HID device and the host.

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
         :rows: nrf52840dk_nrf52840, nrf52833dk_nrf52833, nrf52833dk_nrf52820, nrf5340dk_nrf5340_cpuapp

      Depending on the configuration, a DK may act either as mouse, keyboard or dongle.
      You can check supported configurations for each board in the :ref:`nrf_desktop_board_configuration_files` section.

..

The application is designed to allow easy porting to new hardware.
Check :ref:`nrf_desktop_porting_guide` for details.

.. _nrf_desktop_requirements_build_types:

nRF Desktop build types
=======================

The nRF Desktop does not use a single :file:`prj.conf` file.
Configuration files are provided for different build types for each supported board.

Each board has its own :file:`prj.conf` file, which represents a ``debug`` build type.
Other build types are covered by dedicated files with the build type added as a suffix to the ``prj`` part, as per the following list.
For example, the ``release`` build type file name is :file:`prj_release.conf`.
If a board has other configuration files, for example associated with partition layout or child image configuration, these follow the same pattern.

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

.. note::
    `Selecting a build type`_ is optional.
    The ``debug`` build type is used by default in nRF Desktop if no build type is explicitly selected.

The following build types are available for various boards in the nRF Desktop:

* Bootloader-enabled configurations with support for :ref:`serial recovery DFU <nrf_desktop_bootloader_serial_dfu>` or :ref:`background DFU <nrf_desktop_bootloader_background_dfu>` are set as default if they fit in flash memory.
  See :ref:`nrf_desktop_board_configuration_files` for details about which boards have bootloader included in their default configuration.
* ``release`` - Release version of the application with no debugging features.
* ``debug`` - Debug version of the application; the same as the ``release`` build type, but with debug options enabled.
* ``wwcb`` - ``debug`` build type with the support for the B0 bootloader enabled for `Works With ChromeBook (WWCB)`_.

In nRF Desktop, not every development kit can support every build type mentioned above.
If the given build type is not supported on the selected DK, an error message will appear when `Building and running`_.
For example, if the ``wwcb`` build type is not supported on the selected DK, the following notification appears:

.. code-block:: console

   File not found: ./ncs/nrf/applications/nrf_desktop/configuration/nrf52dmouse_nrf52832/prj_wwcb.conf

|nrf_desktop_build_type_conf|
For example, the nRF52840 Development Kit supports the ``keyboard`` configuration, which is defined in the :file:`prj_keyboard.conf` file in the :file:`configuration/nrf52840dk_nrf52840` directory.
This configuration lets you generate the application with the keyboard role.

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

      The dongle has a USB connector located at one end of the board.
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

The nRF Desktop dongle works as a bridge between the devices connected through standard Bluetooth® Low Energy or Low Latency Packet Mode and the host connected through USB.
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

.. note::
   To simplify pairing the nRF Desktop peripherals with Windows 10 hosts, the peripherals include `Swift Pair`_ payload in the Bluetooth LE advertising data.
   By default, the Swift Pair payload is included for all of the Bluetooth local identities, apart from the dedicated local identity used for connection with an nRF Desktop dongle.

   Some of the nRF Desktop configurations also include `Fast Pair`_ payload in the Bluetooth LE advertising data to simplify pairing the nRF Desktop peripherals with Android hosts.
   These configurations apply further modifications that are needed to improve the user experience.
   See the :ref:`nrf_desktop_bluetooth_guide_fast_pair` documentation section for details.

The nRF Desktop Bluetooth Central device scans for all bonded peripherals that are not connected.
Right after entering the scanning state, the scanning operation is uninterruptable for a predefined time (:kconfig:option:`CONFIG_DESKTOP_BLE_FORCED_SCAN_DURATION_S`) to speed up connection establishment with Bluetooth Peripherals.
After the timeout, the scanning is interrupted when any device connected to the dongle through Bluetooth comes in use.
A connected peripheral is considered in use when it provides HID input reports.
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
          After the forced scan timeout, the scan is interrupted if another peripheral connected to the dongle is in use.

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

You can define the amount of time after which the peripherals are suspended or powered off in :kconfig:option:`CONFIG_CAF_POWER_MANAGER_TIMEOUT`.
By default, this period is set to 120 seconds.

.. important::
    When the gaming mouse is powered from USB, the power down timeout functionality is disabled.

    If a nRF Desktop device supports remote wakeup, the USB connected device goes to suspended state when USB is suspended.
    The device can then trigger remote wakeup of the connected host on user input.

Configuration
*************

|config|

Adding nRF21540 EK shield support
=================================

The nRF Desktop application can be used with the :ref:`ug_radio_fem_nrf21540ek` shield, an RF front-end module (FEM) for the 2.4 GHz range extension.
The shield can be used with any nRF Desktop HID application configured for a development kit that is fitted with Arduino-compatible connector (see the :guilabel:`DK` tab in `Requirements`_).
This means that the shield support is not available for nRF Desktop's dedicated boards, such as ``nrf52840gmouse_nrf52840``, ``nrf52kbd_nrf52832``, or ``nrf52840dongle_nrf52840``.

Low Latency Packet mode
-----------------------

The RF front-end module (FEM) cannot be used together with Low Latency Packet Mode (LLPM) due to timing requirements.
The LLPM support in the nRF Desktop application (:kconfig:option:`CONFIG_CAF_BLE_USE_LLPM`) must be disabled for builds with FEM.

Building with EK shield support
-------------------------------

To build the application with the shield support, pass the ``SHIELD`` parameter to the build command.
Make sure to also disable the LLPM support.
For example, you can build the application for ``nrf52840dk_nrf52840`` with ``nrf21540ek`` shield using the following command:

.. code-block:: console

   west build -b nrf52840dk_nrf52840 -- -DSHIELD=nrf21540ek -DCONFIG_CAF_BLE_USE_LLPM=n

For the multi-core build, you need to pass the ``SHIELD`` parameter to images built on both application and network core.
The network core controls the FEM, but the application core needs to forward the needed pins to the network core.
Use ``hci_rpmsg_`` as the *childImageName* parameter, because in the nRF Desktop application, network core runs using ``hci_rpmsg_``.
The command for ``nrf5340dk_nrf5340_cpuapp`` with ``nrf21540ek`` shield would look as follows:

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp -- -DSHIELD=nrf21540ek_fwd -Dhci_rpmsg_SHIELD=nrf21540ek -DCONFIG_CAF_BLE_USE_LLPM=n

For detailed information about building an application using the nRF21540 EK, see the :ref:`ug_radio_fem_nrf21540ek_programming` section in the Working with RF Front-end modules documentation.

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

Selecting a build type in |VSC|
-------------------------------

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

Selecting a build type from command line
----------------------------------------

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_selection_cmd_start
   :end-before: build_types_selection_cmd_end

.. note::
   If nRF Desktop is built with `Fast Pair`_ support, you must provide Fast Pair Model ID and Anti Spoofing private key as CMake options.
   You can use either your own provisioning data or the provisioning data obtained by Nordic Semiconductor for development purposes.
   The following debug devices are meant to be used with the nRF Desktop and have been registered:

   * NCS keyboard - The Fast Pair Provider meant to be used with keyboards:

      * Device Name: NCS keyboard
      * Model ID: ``0x52FF02``
      * Anti-Spoofing Private Key (base64, uncompressed): ``8E8ulwhSIp/skZeg27xmWv2SxRxTOagypHrf2OdrhGY=``
      * Device Type: Input Device
      * Notification Type: Fast Pair
      * Data-Only connection: true
      * No Personalized Name: false

   * NCS gaming mouse - Fast Pair Provider meant to be used with gaming mice:

      * Device Name: NCS gaming mouse
      * Model ID: ``0x8E717D``
      * Anti-Spoofing Private Key (base64, uncompressed): ``dZxFzP7X9CcfLPC0apyRkmgsh3n2EbWo9NFNXfVuxAM=``
      * Device Type: Input Device
      * Notification Type: Fast Pair
      * Data-Only connection: true
      * No Personalized Name: false

   See :ref:`ug_bt_fast_pair_provisioning` documentation for the following information:

   * Registering a Fast Pair Provider
   * Provisioning a Fast Pair Provider in |NCS|

.. nrf_desktop_fastpair_important_start

.. important::
   This is the debug Fast Pair provisioning data obtained by Nordic for the development purposes.
   It should not be used in production.

   To test with the debug mode Model ID, you must configure the Android device to include the debug results while displaying the nearby Fast Pair Providers.
   For details, see `Verifying Fast Pair`_ in the GFPS documentation.

.. nrf_desktop_fastpair_important_end

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
        This might be needed if the dongle does not connect with all the peripherals before scanning is interrupted by a timeout.

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

.. _nrf_desktop_measuring_hid_report_rate:

Measuring HID report rate
-------------------------

You can measure a HID report rate of your application to assess the performance of your HID device.
This measurement allows you to check how often the host computer can get user's input from the HID device.

Prerequsites
~~~~~~~~~~~~

The HID report rate can be measured by using either browser-based or platform-specific tools.
You can use any preferred HID report rate tool.

.. note::
   The host computer controls polling a HID peripheral for HID reports.
   The HID peripheral cannot trigger sending a HID report even if the report is prepared in time.
   Polling inaccuracies and missing polls on the host side can negatively affect the measured report rate.
   Make sure to close all unnecessary PC applications to mitigate negative impact of these applications on polling HID devices.
   If you are using a browser-based tool, leave open only the tab with HID report rate measurement tool to ensure that no other tab influences the measurement.

Building information
~~~~~~~~~~~~~~~~~~~~

Use the :file:`prj_release.conf` configuration for the HID report rate measurement.
Debug features, such as logging or assertions, decrease the application performance.

Use the nRF Desktop configuration that acts as a HID mouse reference design for the report rate measurement, as the motion data polling is synchronized with sending HID reports.

Make sure your chosen motion data source will generate movement in each poll interval.
Without a need for user's input, you can generate HID reports that contain mouse movement data.
To do this, use the :ref:`Motion simulated module <nrf_desktop_motion_report_rate>`.

To build an application for evaluating HID report rate, run the following command:

   .. parsed-literal::
      :class: highlight

      west build -p -b <target> -- \
      -DCONF_FILE=prj_release.conf \
      -DCONFIG_DESKTOP_MOTION_SIMULATED_ENABLE=y \

Report rate measuring tips
~~~~~~~~~~~~~~~~~~~~~~~~~~

See the following list of possible scenarios and best practices:

* If two or more peripherals are connected through the dongle, and all of the devices support LLPM, then the Bluetooth LE LLPM connection events split evenly among all of the peripherals connected through that dongle.
  It results in decreased HID report rate.
  For example, you should observe a 500 Hz HID report rate when both mouse and keyboard are connected through the dongle and a 1000 Hz rate when only the mouse is connected.
* If a HID peripheral is connected through a dongle, the dongle's performance must be taken into account when measuring the report rate.
  Delays related to data forwarding on the dongle also result in reduced report rate.
* If the device is connected through Bluetooth LE directly to the HID host, then the host sets the Bluetooth LE connection interval.
  A Bluetooth LE peripheral can suggest the preferred connection parameters.
  The suggested connection interval can be set using the :kconfig:option:`CONFIG_BT_PERIPHERAL_PREF_MIN_INT` and :kconfig:option:`CONFIG_BT_PERIPHERAL_PREF_MAX_INT` Kconfig options.
  Set parameters are not enforced, meaning that the HID host may still eventually use a value greater than the maximum connection interval requested by a peripheral.
* Radio frequency (RF) noise can negatively affect the HID report rate for wireless connections.
  If a HID report fails to be delivered in a given Bluetooth LE LLPM connection event, it is retransmitted in the subsequent connection event which effectively reduces the report rate.
  By avoiding congested RF channels, the :ref:`nrf_desktop_ble_qos` helps to achieve better connection quality and a higher report rate.
* For the USB device connected directly, you can configure your preferred USB HID poll interval using the :kconfig:option:`CONFIG_USB_HID_POLL_INTERVAL_MS` Kconfig option.
  By default, the :kconfig:option:`CONFIG_USB_HID_POLL_INTERVAL_MS` Kconfig option is set to ``1`` to request the lowest possible poll interval.
  Set parameters are not enforced, meaning that the HID host may still eventually use a value greater than the USB polling interval requested by a peripheral.

Testing steps
~~~~~~~~~~~~~

After building the application, test the nRF Desktop by performing the following steps:

1. Program the device with the built firmware.
#. Connect the device to the computer using a preferred transport (Bluetooth LE, USB, dongle).
#. Turn on the device.
   If you use the motion simulated module to generate the mouse movement, the device should automatically start to draw an octagon shape on the screen.
   Otherwise, you need to constantly keep generating motion manually, for example, by moving your mouse.
#. Turn off the device to finalize test preparations.
#. Launch selected HID report rate measurement tool.
#. Turn back on the device.
#. Run measurement.
#. Verify the average HID report rate reported by tool.

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

For information about differences between DTS and Kconfig, see :ref:`zephyr:dt_vs_kconfig`.

Application-specific Kconfig configuration
==========================================

The nRF Desktop introduces Kconfig options that can be used to simplify an application configuration.
These options can be used to select a device role and to automatically apply a default configuration suitable for the selected role.

.. note::
   Part of the default configuration is applied by modifying the default values of Kconfig options.
   Changing configuration in menuconfig does not automatically adjust user-configurable values to the new defaults.
   So, you must update those values manually.
   For more information, see the Stuck symbols in menuconfig and guiconfig section on the :ref:`kconfig_tips_and_tricks` in the Zephyr documentation.

   The default Kconfig option values are automatically updated if configuration changes are applied directly in the configuration files.

.. _nrf_desktop_hid_configuration:

HID configuration
-----------------

The nRF Desktop application introduces application-specific configuration options related to HID device configuration.
These options are defined in :file:`Kconfig.hid`.

The options define the nRF Desktop device role.
The device role can be either the HID dongle (:ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>`) or the HID peripheral (:ref:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL <config_desktop_app_options>`).
The HID peripheral role can also specify a peripheral type:

* HID mouse (:ref:`CONFIG_DESKTOP_PERIPHERAL_TYPE_MOUSE <config_desktop_app_options>`)
* HID keyboard (:ref:`CONFIG_DESKTOP_PERIPHERAL_TYPE_KEYBOARD <config_desktop_app_options>`)
* other HID device (:ref:`CONFIG_DESKTOP_PERIPHERAL_TYPE_OTHER <config_desktop_app_options>`)

Each role automatically implies nRF Desktop modules needed for the role.
For example, :ref:`nrf_desktop_hid_state` is automatically enabled for the HID peripheral role.

By default, the nRF Desktop devices use predefined format of HID reports.
The common HID report map is defined in the :file:`configuration/common/hid_report_desc.c` file.
The selected role implies a set of related HID reports.
For example, HID mouse automatically enables support for HID mouse report.
If ``other HID device`` peripheral type is chosen, the set of HID reports needs to be explicitly defined in the configuration.

Apart from this, the supported HID boot protocol interface can be specified as either:

* mouse (:ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE <config_desktop_app_options>`)
* keyboard (:ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD <config_desktop_app_options>`)
* none (:ref:`CONFIG_DESKTOP_HID_BOOT_INTERFACE_DISABLED <config_desktop_app_options>`)

.. _nrf_desktop_hid_device_identifiers:

HID device identifiers
~~~~~~~~~~~~~~~~~~~~~~

The nRF Desktop application defines the following common device identifiers:

* Manufacturer (:ref:`CONFIG_DESKTOP_DEVICE_MANUFACTURER <config_desktop_app_options>`)
* Vendor ID (:ref:`CONFIG_DESKTOP_DEVICE_VID <config_desktop_app_options>`)
* Product name (:ref:`CONFIG_DESKTOP_DEVICE_PRODUCT <config_desktop_app_options>`)
* Product ID (:ref:`CONFIG_DESKTOP_DEVICE_PID <config_desktop_app_options>`)

These Kconfig options determine the default values of device identifiers used for:

* :ref:`nrf_desktop_usb_state_identifiers`
* BLE GATT Device Information Service (:kconfig:option:`CONFIG_BT_DIS`) that is required for :ref:`nrf_desktop_bluetooth_guide_peripheral`

.. note::
   Apart from the mentioned common device identifiers, the nRF Desktop application defines an application-specific string representing device generation (:ref:`CONFIG_DESKTOP_DEVICE_GENERATION <config_desktop_app_options>`).
   The generation allows to distinguish configurations that use the same board and bootloader, but are not interoperable.
   The value can be read through the :ref:`nrf_desktop_config_channel`.

Debug configuration
-------------------

The nRF Desktop application introduces application-specific configuration options related to the debug configuration.
These options are defined in the :file:`Kconfig.debug` file.

The :ref:`CONFIG_DESKTOP_LOG <config_desktop_app_options>` Kconfig option enables support for logging in the nRF Desktop application.
This option overlays Kconfig option defaults from the Logging subsystem to align them with the nRF Desktop requirements.
The nRF Desktop configuration uses SEGGER J-Link RTT as the Logging subsystem backend.

The :ref:`CONFIG_DESKTOP_SHELL <config_desktop_app_options>` Kconfig option enables support for CLI in the nRF Desktop application.
This option overlays Kconfig option defaults from the Shell subsystem to align them with the nRF Desktop requirements.
The nRF Desktop configuration uses SEGGER J-Link RTT as the Shell subsystem backend.
If both shell and logging are enabled, logger uses shell as the logging backend.

See the :file:`Kconfig.debug` file content for details.

Default common configuration
----------------------------

The nRF Desktop application aligns the configuration with the nRF Desktop use case by overlaying Kconfig defaults and selecting or implying the required Kconfig options.
Among others, the Kconfig :ref:`app_event_manager` and :ref:`lib_caf` options are selected to ensure that they are enabled.
The :ref:`CONFIG_DESKTOP_SETTINGS_LOADER <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_POWER_MANAGER <config_desktop_app_options>` are implied to enable the :ref:`nrf_desktop_settings_loader` and :ref:`nrf_desktop_power_manager` modules, respectively.
See the :file:`Kconfig.defaults` file for details related to default common configuration.

.. _nrf_desktop_bluetooth_configuration:

Bluetooth configuration
-----------------------

The nRF Desktop application introduces application-specific configuration options related to Bluetooth connectivity configuration.
These options are defined in :file:`Kconfig.ble` file.

The :ref:`CONFIG_DESKTOP_BT <config_desktop_app_options>` Kconfig option enables support for Bluetooth connectivity in the nRF Desktop application.
The option is enabled by default.

The nRF Desktop Bluetooth peripheral configuration (:ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>`) is automatically enabled for the nRF Desktop HID peripheral role (:ref:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL <config_desktop_app_options>`).
The nRF Desktop Bluetooth central configuration (:ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>`) is automatically enabled for the nRF Desktop HID dongle role (:ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>`)

The nRF Desktop Bluetooth configuration options perform the following:

* Implies application modules related to Bluetooth that are required for the selected device role
* Selects required functionalities in Zephyr's Bluetooth stack
* Overlays Bluetooth Kconfig option defaults to align them with the nRF Desktop use-case

See :file:`Kconfig.ble` file content for details.
See the :ref:`nrf_desktop_bluetooth_guide` for more information about Bluetooth support in nRF Desktop application.

CAF configuration
-----------------

The nRF Desktop application overlays defaults of the :ref:`lib_caf` related Kconfig options to align them with the nRF Desktop use-case.
The files that apply the overlays are located in the :file:`src/modules` directory and are named :file:`Kconfig.caf_module_name.default`.
For example, the Kconfig defaults of :ref:`caf_settings_loader` are overlayed in the :file:`src/modules/Kconfig.caf_settings_loader.default`.

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
    * Application configuration file for the ``debug`` (:file:`prj.conf`) :ref:`build type <nrf_desktop_requirements_build_types>`.
    * Configuration files for the selected modules.

Optional configuration files
    * Application configuration files for other build types.
    * Configuration file for the bootloader.
    * Memory layout configuration.
    * DTS overlay file.

See `Adding a new board`_ for information about how to add these files.

.. _nrf_desktop_board_configuration_files:

nRF Desktop board configuration files
-------------------------------------

The nRF Desktop application comes with configuration files for the following reference designs:

nRF52840 Gaming Mouse (nrf52840gmouse_nrf52840)
      * The reference design is defined in :file:`nrf/boards/arm/nrf52840gmouse_nrf52840` for the project-specific hardware.
      * To achieve gaming-grade performance:

        * The application is configured to act as a gaming mouse, with both Bluetooth LE and USB transports enabled.
        * Bluetooth is configured to use Nordic's SoftDevice link layer.

      * The configuration with the B0 bootloader is set as default.
      * The board supports debug (:file:`prj_fast_pair.conf`) and release (:file:`prj_release_fast_pair.conf`) :ref:`nrf_desktop_bluetooth_guide_fast_pair` configurations.
        Both configurations use the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``), and they support the firmware updates using the :ref:`nrf_desktop_dfu` and the :ref:`nrf_desktop_dfu_mcumgr`.

nRF52832 Desktop Mouse (nrf52dmouse_nrf52832) and nRF52810 Desktop Mouse (nrf52810dmouse_nrf52810)
      * Both reference designs are meant for the project-specific hardware and are defined in :file:`nrf/boards/arm/nrf52dmouse_nrf52832` and :file:`nrf/boards/arm/nrf52810dmouse_nrf52810`, respectively.
      * The application is configured to act as a mouse.
      * Only the Bluetooth LE transport is enabled.
        Bluetooth uses either Zephyr's software link layer (nrf52810dmouse_nrf52810) or Nordic's SoftDevice link layer (nrf52dmouse_nrf52832).
      * The preconfigured build types for both nrf52dmouse_nrf52832 and nrf52810dmouse_nrf52810 boards are without the bootloader due to memory size limits on nrf52810dmouse_nrf52810 board.

Sample mouse, keyboard or dongle (nrf52840dk_nrf52840)
      * The configuration uses the nRF52840 Development Kit.
      * The build types allow to build the application as mouse, keyboard or dongle.
      * Inputs are simulated based on the hardware button presses.
      * The configuration with the B0 bootloader is set as default.
      * The board supports debug :ref:`nrf_desktop_bluetooth_guide_fast_pair` configuration that acts as a mouse (:file:`prj_fast_pair.conf`).
        The configuration uses the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``), and supports firmware updates using the :ref:`nrf_desktop_dfu` and the :ref:`nrf_desktop_dfu_mcumgr`.

Sample dongle (nrf52833dk_nrf52833)
      * The configuration uses the nRF52833 Development Kit.
      * The application is configured to act as a dongle that forwards data from both mouse and keyboard.
      * Bluetooth uses Nordic's SoftDevice link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * The configuration with the MCUboot bootloader is set as default.

Sample dongle (nrf52833dk_nrf52820)
      * The configuration uses the nRF52820 emulation on the nRF52833 Development Kit.
      * The application is configured to act as a dongle that forwards data from both mouse and keyboard.
      * Bluetooth uses Zephyr's software link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * |preconfigured_build_types|

nRF52832 Desktop Keyboard (nrf52kbd_nrf52832)
      * The reference design used is defined in :file:`nrf/boards/arm/nrf52kbd_nrf52832` for the project-specific hardware.
      * The application is configured to act as a keyboard, with the Bluetooth LE transport enabled.
      * Bluetooth is configured to use Nordic's SoftDevice link layer.
      * The preconfigured build types configure the device without the bootloader in debug mode and with B0 bootloader in release mode due to memory size limits.
      * The board supports release :ref:`nrf_desktop_bluetooth_guide_fast_pair` configuration (:file:`prj_release_fast_pair.conf`).
        The configuration uses the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``), and supports firmware updates using the :ref:`nrf_desktop_dfu` and the :ref:`nrf_desktop_dfu_mcumgr`.

nRF52840 USB Dongle (nrf52840dongle_nrf52840) and nRF52833 USB Dongle (nrf52833dongle_nrf52833)
      * Since the nRF52840 Dongle is generic and defined in Zephyr, project-specific changes are applied in the DTS overlay file.
      * The application is configured to act as a dongle that forwards data from both mouse and keyboard.
      * Bluetooth uses Nordic's SoftDevice link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * The configuration with the B0 bootloader is set as default for nrf52840dongle_nrf52840 board and with the MCUboot bootloader is set as default for nrf52833dongle_nrf52833 board.

nRF52820 USB Dongle (nrf52820dongle_nrf52820)
      * The application is configured to act as a dongle that forwards data from both mouse and keyboard.
      * Bluetooth uses Zephyr's software link layer and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * |preconfigured_build_types|

Sample dongle (nrf5340dk_nrf5340)
      * The application is configured to act as a dongle that forwards data from both mouse and keyboard.
      * Bluetooth uses Nordic's SoftDevice link layer without LLPM and is configured to act as a central.
        Input data comes from Bluetooth and is retransmitted to USB.
      * The configuration with the B0 bootloader is set as default.

.. _porting_guide_adding_board:

Adding a new board
==================

When adding a new board for the first time, focus on a single configuration.
Moreover, keep the default ``debug`` build type that the application is built with, and do not add any additional build type parameters.
The following procedure uses the gaming mouse configuration as an example.

Zephyr support for a board
--------------------------

Before introducing nRF Desktop application configuration for a given board, you need to ensure that the board is supported in Zephyr.

.. note::
   You can skip this step if your selected board is already supported in Zephyr.

Follow the Zephyr's :ref:`zephyr:board_porting_guide` for detailed instructions related to introducing Zephyr support for a new board.
Make sure that the following conditions are met:

#. Edit the DTS files to make sure they match the hardware configuration.
   Pay attention to the following elements:

   * Pins that are used.
   * Bus configuration for optical sensor.
   * `Changing interrupt priority`_.

#. Edit the board's Kconfig files to make sure they match the required system configuration.
   For example, disable the drivers that will not be used by your device.

.. tip::
   You can define the new board by copying the nRF Desktop reference design files that are the closest match for your hardware and then aligning the configuration to your hardware.
   For example, for gaming mouse use :file:`nrf/boards/arm/nrf52840gmouse_nrf52840`.

nRF Desktop configuration
-------------------------

Perform the following steps to add nRF Desktop application configuration for a board that is already supported in Zephyr.

1. Copy the project files for the device that is the closest match for your hardware.
   For example, for gaming mouse these are located at :file:`applications/nrf_desktop/configuration/nrf52840gmouse_nrf52840`.
#. Optionally, depending on the reference design, edit the DTS overlay file.
   This step is not required if you have created a new reference design and its DTS files fully describe your hardware.
   In such case, the overlay file can be left empty.
#. In Kconfig, ensure that the following hardware interface modules that are specific for gaming mouse are enabled:

   * :ref:`caf_buttons`
   * :ref:`caf_leds`
   * :ref:`nrf_desktop_motion`
   * :ref:`nrf_desktop_wheel`
   * :ref:`nrf_desktop_battery_meas`

#. For each module enabled, change its configuration to match your hardware.
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
     * The module uses Zephyr's :ref:`zephyr:led_api` driver for setting the LED color.
       Zephyr's LED driver can use the implementation based on either GPIO or PWM (Pulse-Width Modulation).
       The hardware configuration is described through DTS.
       See the :ref:`caf_leds` configuration section for details.

#. Review the :ref:`nrf_desktop_hid_configuration`.
#. By default, the nRF Desktop device enables Bluetooth connectivity support.
   Review the :ref:`nrf_desktop_bluetooth_configuration`.

   a. Ensure that the Bluetooth role is properly configured.
      For mouse, it should be configured as peripheral.
   #. Update the configuration related to peer control.
      You can also disable the peer control using the :ref:`CONFIG_DESKTOP_BLE_PEER_CONTROL <config_desktop_app_options>` option.
      Peer control details are described in the :ref:`nrf_desktop_ble_bond` documentation.

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

.. rst-class:: numbered-step

Add a new sensor driver
-----------------------

First, create a new motion sensor driver that will provide code for communication with the sensor.
Use the two existing |NCS| sensor drivers as an example.

The communication between the application and the sensor is done through a sensor driver API (see :ref:`sensor_api`).
For the motion module to work correctly, the driver must support a trigger (see ``sensor_trigger_set``) on a new data (see ``SENSOR_TRIG_DATA_READY`` trigger type).

When the motion data is ready, the driver calls a registered callback.
The application starts a process of retrieving a motion data sample.
The motion module calls ``sensor_sample_fetch`` and then ``sensor_channel_get`` on two sensor channels, ``SENSOR_CHAN_POS_DX`` and ``SENSOR_CHAN_POS_DY``.
The driver must support these two channels.

.. rst-class:: numbered-step

Create a DTS binding
--------------------

Zephyr recommends to use DTS for hardware configuration (see :ref:`zephyr:dt_vs_kconfig`).
For the new motion sensor configuration to be recognized by DTS, define a dedicated DTS binding.
See :ref:`dt-bindings` for more information, and refer to :file:`dts/bindings/sensor` for binding examples.

.. rst-class:: numbered-step

Configure sensor through DTS
----------------------------

Once binding is defined, it is possible to set the sensor configuration.
This is done by editing the DTS file that describes the board.
For more information, see :ref:`devicetree-intro`.

As an example, take a look at the PMW3360 sensor that already exists in |NCS|.
The following code excerpt is taken from :file:`boards/arm/nrf52840gmouse_nrf52840/nrf52840gmouse_nrf52840.dts`:

.. code-block:: none

   &spi1 {
   	compatible = "nordic,nrf-spim";
   	status = "okay";
   	cs-gpios = <&gpio0 13 0>;

	pinctrl-0 = <&spi1_default_alt>;
	pinctrl-1 = <&spi1_sleep_alt>;
	pinctrl-names = "default", "sleep";
   	pmw3360@0 {
   		compatible = "pixart,pmw3360";
   		reg = <0>;
   		irq-gpios = <&gpio0 21 0>;
   		spi-max-frequency = <2000000>;
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
      If you aim for the lowest latency through the LLPM (an interval of 1 ms), the sensor data readout should take no more than 250 us.
      The bus and the sensor configuration must ensure that communication speed is fast enough.

The remaining option ``irq-gpios`` is specific to ``pixart,pmw3360`` binding.
It refers to the PIN to which the motion sensor IRQ line is connected.

If a different kind of bus is used for the new sensor, the DTS layout will be different.

.. rst-class:: numbered-step

Include sensor in the application
---------------------------------

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

.. rst-class:: numbered-step

Select the new sensor
---------------------

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

The ``interrupts`` property is an array, where the meaning of each element is defined by the specification of the interrupt controller.
These specification files are located at :file:`zephyr/dts/bindings/interrupt-controller/` DTS binding file directory.

For example, for nRF52840 the file is :file:`arm,v7m-nvic.yaml`.
This file defines ``interrupts`` property in the ``interrupt-cells`` list.
For nRF52840, it contains two elements: ``irq`` and ``priority``.
The default values for these elements for the given peripheral can be found in the :file:`dtsi` file specific for the device.
In the case of nRF52840, this is :file:`zephyr/dts/arm/nordic/nrf52840.dtsi`, which has the following ``interrupts``:

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

The flash memory is set by defining one of the following options:

* `Memory layout in DTS`_ (bootloader is not enabled)
* `Memory layout in partition manager`_ (either bootloader or :ref:`nrf_desktop_bluetooth_guide_fast_pair` is enabled)

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
    It is only used if :kconfig:option:`CONFIG_USE_DT_CODE_PARTITION` is enabled.
    If this option is disabled, the code is loaded at the address defined by :kconfig:option:`CONFIG_FLASH_LOAD_OFFSET` and can spawn for :kconfig:option:`CONFIG_FLASH_LOAD_SIZE` (or for the whole flash if the load size is set to zero).

Because the nRF Desktop application depends on the DTS layout only for configurations without the bootloader, only the settings partition is relevant in such cases and other partitions are ignored.

For more information about how to configure the flash memory layout in the DTS files, see :ref:`zephyr:flash_map_api`.

Memory layout in partition manager
----------------------------------

When the bootloader is enabled, the nRF Desktop application uses the partition manager for the layout configuration of the flash memory.
The nRF Desktop configurations with bootloader use static configurations of partitions to ensure that the partition layout will not change between builds.

Add the :file:`pm_static_${BUILD_TYPE}.yml` file to the project's board configuration directory to define the static partition manager configuration for given board and build type.
For example, to define the static partition layout for the nrf52840dk_nrf52840 board and ``release`` build type, you would need to add the :file:`pm_static_release.yml` file into the :file:`applicatons/nrf_desktop/configuration/nrf52840dk_nrf52840` directory.

Take into account the following points:

* For the :ref:`background firmware upgrade <nrf_desktop_bootloader_background_dfu>`, you must define the secondary image partition.
  This is because the update image is stored on the secondary image partition while the device is running firmware from the primary partition.
  For this reason, the feature is not available for devices with smaller flash size, because the size of the required flash memory is essentially doubled.
  The devices with smaller flash size can use either USB serial recovery or the MCUboot bootloader with the secondary image partition located on an external flash.
* When you use :ref:`USB serial recovery <nrf_desktop_bootloader_serial_dfu>`, you do not need the secondary image partition.
  The firmware image is overwritten by the bootloader.

For an example of configuration, see the static partition maps defined for the existing configuration that uses a given DFU method.
For more information about how to configure the flash memory layout using the partition manager, see :ref:`partition_manager`.

.. _nrf_desktop_pm_external_flash:

External flash configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Partition Manager supports partitions in external flash.

Enabling external flash can be useful especially for memory-limited devices.
For example, the MCUboot can use it as a secondary image partition for the :ref:`background firmware upgrade <nrf_desktop_bootloader_background_dfu>`.
The MCUboot moves the image data from the secondary image partition to the primary image partition before booting the new firmware.

For an example of the nRF Desktop application configuration that uses an external flash, see the ``mcuboot_qspi`` configuration of the nRF52840 Development Kit.
This configuration uses the ``MX25R64`` external flash that is part of the development kit.

For detailed information, see the :ref:`partition_manager` documentation.

.. _nrf_desktop_bluetooth_guide:

Bluetooth in nRF Desktop
========================

The nRF Desktop devices use :ref:`Zephyr's Bluetooth API <zephyr:bluetooth>` to handle the Bluetooth LE connections.

This API is used only by the application modules that handle such connections.
The information about peer and connection state is propagated to other application modules using :ref:`app_event_manager` events.

The :ref:`CONFIG_DESKTOP_BT <config_desktop_app_options>` Kconfig option enables support for Bluetooth connectivity in the nRF Desktop.
Specific Bluetooth configurations and application modules are selected or implied according to the HID device role.
Apart from that, the defaults of Bluetooth-related Kconfigs are aligned with the nRF Desktop use case.

The nRF Desktop devices come in the following roles:

* HID peripheral (:ref:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL <config_desktop_app_options>`) that works as Bluetooth Peripheral (:ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>`)

  * Support only the Bluetooth Peripheral role (:kconfig:option:`CONFIG_BT_PERIPHERAL`).
  * Handle only one Bluetooth LE connection at a time.
  * Use more than one Bluetooth local identity.

* HID dongle (:ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>`) that works as Bluetooth Central (:ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>`)

  * Support only the Bluetooth Central role (:kconfig:option:`CONFIG_BT_CENTRAL`).
  * Handle multiple Bluetooth LE connections simultaneously.
  * Use only one Bluetooth local identity (the default one).

Both central and peripheral devices have dedicated configuration options and use dedicated modules.

The nRF Desktop peripheral configurations that enable `Fast Pair`_ support use slightly different Bluetooth configuration.
This is needed to improve the user experience.
See :ref:`nrf_desktop_bluetooth_guide_fast_pair` for more details.

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

* :kconfig:option:`CONFIG_BT_MAX_PAIRED`

  * nRF Desktop central: The maximum number of paired devices is greater than or equal to the maximum number of simultaneously connected peers.
    The :kconfig:option:`CONFIG_BT_MAX_PAIRED` is by default set to :ref:`CONFIG_DESKTOP_HID_DONGLE_BOND_COUNT <config_desktop_app_options>`.
  * nRF Desktop peripheral: The maximum number of paired devices is equal to the number of peers plus one, where the one additional paired device slot is used for erase advertising.

* :kconfig:option:`CONFIG_BT_ID_MAX`

  * nRF Desktop central: The device uses only one Bluetooth local identity, that is the default one.
  * nRF Desktop peripheral: The number of Bluetooth local identities must be equal to the number of peers plus two.

    * One additional local identity is used for erase advertising.
    * The other additional local identity is the default local identity, which is unused, because it cannot be reset after removing the bond.
      Without the identity reset, the previously bonded central could still try to reconnect after being removed from Bluetooth bonds on the peripheral side.

* :kconfig:option:`CONFIG_BT_MAX_CONN`

  * nRF Desktop central: This option is set to the maximum number of simultaneously connected devices.
    The :kconfig:option:`CONFIG_BT_MAX_CONN` is by default set to :ref:`CONFIG_DESKTOP_HID_DONGLE_CONN_COUNT <config_desktop_app_options>`.
  * nRF Desktop peripheral: The default value (one) is used.

.. note::
   After changing the number of Bluetooth peers for the nRF Desktop peripheral device, update the LED effects used to represent the Bluetooth connection state.
   For details, see :ref:`nrf_desktop_led_state`.

.. _nrf_desktop_bluetooth_guide_configuration_ll:

Link Layer configuration options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The nRF Desktop devices use one of the following Link Layers:

* :kconfig:option:`CONFIG_BT_LL_SW_SPLIT`
    This Link Layer does not support the Low Latency Packet Mode (LLPM) and has a lower memory usage, so it can be used by memory-limited devices.

* :kconfig:option:`CONFIG_BT_LL_SOFTDEVICE`
    This Link Layer does support the Low Latency Packet Mode (LLPM).
    If you opt for this Link Layer and enable the :kconfig:option:`CONFIG_BT_CTLR_SDC_LLPM`, the :kconfig:option:`CONFIG_CAF_BLE_USE_LLPM` is also enabled by default and can be configured further:

    * When :kconfig:option:`CONFIG_CAF_BLE_USE_LLPM` is enabled, set the value for :kconfig:option:`CONFIG_BT_CTLR_SDC_MAX_CONN_EVENT_LEN_DEFAULT` to ``3000``.

      This is required by the nRF Desktop central and helps avoid scheduling conflicts with Bluetooth Link Layer.
      Such conflicts could lead to a drop in HID input report rate or a disconnection.
      Because of this, if the nRF Desktop central supports LLPM and more than one simultaneous Bluetooth connection, it also uses 10 ms connection interval instead of 7.5 ms.
      Setting the value of :kconfig:option:`CONFIG_BT_CTLR_SDC_MAX_CONN_EVENT_LEN_DEFAULT` to ``3000`` also enables the nRF Desktop central to exchange data with up to 3 standard Bluetooth LE peripherals during every connection interval (every 10 ms).

    * When :kconfig:option:`CONFIG_CAF_BLE_USE_LLPM` is disabled, the device will use only standard Bluetooth LE connection parameters with the lowest available connection interval of 7.5 ms.

      If the LLPM is disabled and more than 2 simultaneous Bluetooth connections are supported (:kconfig:option:`CONFIG_BT_MAX_CONN`), you can set the value for :kconfig:option:`CONFIG_BT_CTLR_SDC_MAX_CONN_EVENT_LEN_DEFAULT` to ``2500``.
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
   The nRF Desktop devices enable :kconfig:option:`CONFIG_BT_SETTINGS`.
   When this option is enabled, the application is responsible for calling the :c:func:`settings_load` function - this is handled by the :ref:`nrf_desktop_settings_loader`.

.. _nrf_desktop_bluetooth_guide_peripheral:

Bluetooth Peripheral
--------------------

The nRF Desktop peripheral devices must include additional configuration options and additional application modules to comply with the HID over GATT specification.

The HID over GATT profile specification requires Bluetooth Peripherals to define the following GATT Services:

* HID Service - Handled in the :ref:`nrf_desktop_hids`.
* Battery Service - Handled in the :ref:`nrf_desktop_bas`.
* Device Information Service - Implemented in Zephyr and enabled with :kconfig:option:`CONFIG_BT_DIS`.
  The device identifiers are configured according to the common :ref:`nrf_desktop_hid_device_identifiers` by default.
  It can be configured using Kconfig options with the ``CONFIG_BT_DIS`` prefix.

The nRF Desktop peripherals must also define a dedicated GATT Service, which is used to provide the following information:

* Information whether the device can use the LLPM Bluetooth connection parameters.
* Hardware ID of the peripheral.

The GATT Service is implemented by the :ref:`nrf_desktop_dev_descr`.

Apart from the GATT Services, an nRF Desktop peripheral device must enable and configure the following application modules:

* :ref:`nrf_desktop_ble_adv` - Controls the Bluetooth advertising.
* :ref:`nrf_desktop_ble_latency` - Keeps the connection latency low when the :ref:`nrf_desktop_config_channel` is used or when either the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr` receive an update image.
  This is done to ensure quick data transfer.

Optionally, you can also enable the following module:

* :ref:`nrf_desktop_qos` - Forwards the Bluetooth LE channel map generated by :ref:`nrf_desktop_ble_qos`.
  The Bluetooth LE channel map is forwarded using GATT characteristic.
  The Bluetooth Central can apply the channel map to avoid congested RF channels.
  This results in better connection quality and higher report rate.

.. _nrf_desktop_bluetooth_guide_fast_pair:

Fast Pair
~~~~~~~~~

The nRF Desktop peripheral can be built with Google `Fast Pair`_ support.
The configurations that enable Fast Pair are set in the :file:`prj_fast_pair.conf` and :file:`prj_release_fast_pair.conf` files.

.. note::
   The Fast Pair integration in the nRF Desktop is :ref:`experimental <software_maturity>`.
   The factory reset of the Fast Pair non-volatile data is not yet supported.

   The Fast Pair support in the |NCS| is :ref:`experimental <software_maturity>`.
   See :ref:`ug_bt_fast_pair` for details.

These configurations support multiple bonds per Bluetooth local identity (:kconfig:option:`CONFIG_CAF_BLE_STATE_MAX_LOCAL_ID_BONDS` is set to ``3``) and erase advertising (:ref:`CONFIG_DESKTOP_BLE_PEER_ERASE <config_desktop_app_options>`), but Bluetooth peer selection (:ref:`CONFIG_DESKTOP_BLE_PEER_SELECT <config_desktop_app_options>`) is disabled.
You can now pair with your other hosts without putting peripheral back in pairing mode (without triggering the erase advertising).
The nRF Desktop peripheral that integrates Fast Pair behaves as follows:

  * The dongle peer does not use the Fast Pair advertising payload.
  * The bond erase operation is enabled for the dongle peer.
    This will let you change the bonded Bluetooth Central.
  * If the dongle peer (:ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE <config_desktop_app_options>`) is enabled, the `Swift Pair`_ payload is, by default, included only for the mentioned peer.
    In the Fast Pair configurations, the dongle peer is intended to be used for all of the peers that are not Fast Pair Seekers.
  * If the used Bluetooth local identity has no bonds, the device advertises in pairing mode, and the Fast Pair discoverable advertising is used.
    This allows to pair with the nRF Desktop device using both Fast Pair and normal Bluetooth pairing flows.
    This advertising payload is also used during the erase advertising.
  * If the used Bluetooth local identity already has a bond, the device is no longer in the pairing mode and the Fast Pair not discoverable advertising is used.
    This allows to pair only with the Fast Pair Seekers linked to Google Accounts that are already associated with the nRF Desktop device.
    In this mode the device by default rejects normal Bluetooth pairing (:ref:`CONFIG_DESKTOP_FAST_PAIR_LIMIT_NORMAL_PAIRING <config_desktop_app_options>` option is enabled).
    The Fast Pair UI indication is hidden after the Provider reaches :kconfig:option:`CONFIG_CAF_BLE_STATE_MAX_LOCAL_ID_BONDS` bonded peers on the used local identity.
  * The :ref:`nrf_desktop_factory_reset` is enabled by default if the :ref:`nrf_desktop_config_channel` is supported by the device.
    The factory reset operation removes both Fast Pair and Bluetooth non-volatile data.
    The factory reset operation is triggered using the configuration channel.

After successful erase advertising procedure, the peripheral removes all of the bonds of a given Bluetooth local identity.

Apart from that, the following changes are applied in configurations that support Fast Pair:

* The static :ref:`partition_manager` configuration is modified to introduce a dedicated flash partition used to store the Fast Pair provisioning data.
* Bluetooth privacy feature (:kconfig:option:`CONFIG_BT_PRIVACY`) is enabled.
* The fast and slow advertising intervals defined in the :ref:`nrf_desktop_ble_adv` are aligned with Fast Pair expectations.
* The Bluetooth advertising filter accept list (:kconfig:option:`CONFIG_CAF_BLE_ADV_FILTER_ACCEPT_LIST`) is disabled to allow Fast Pair Seekers other than the bonded one to connect outside of the pairing mode.
* The security failure timeout (:ref:`CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S <config_desktop_app_options>`) is longer to prevent disconnections during the Fast Pair procedure.
* Passkey authentication (:ref:`CONFIG_DESKTOP_BLE_ENABLE_PASSKEY <config_desktop_app_options>`) is disabled on keyboard.
  Fast Pair currently does not support devices that use screen or keyboard for Bluetooth authentication.
* TX power correction value (:kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL`) is configured to align the TX power included in the advertising data with the Fast Pair expectations.

See :ref:`ug_bt_fast_pair` for detailed information about Fast Pair support in the |NCS|.

Bluetooth Central
-----------------

The nRF Desktop central must implement Bluetooth scanning and handle the GATT operations.
The central must also control the Bluetooth connection parameters.
These features are implemented by the following application modules:

* :ref:`nrf_desktop_ble_scan` - Controls the Bluetooth scanning.
* :ref:`nrf_desktop_ble_conn_params` - Controls the Bluetooth connection parameters and reacts on latency update requests received from the connected peripherals.
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
    In this scenario, the MCUboot either swaps the application images located on the secondary and primary slot before booting a new image (``swap mode``) or boots a new application image directly from the secondary image slot (``direct-xip mode``).
    The swap operation significantly increases boot time after a successful image transfer, but an external flash can be used as the secondary image slot.

    You can use the MCUboot for the background DFU through the :ref:`nrf_desktop_config_channel` and :ref:`nrf_desktop_dfu`.
    The MCUboot can also be used for the background DFU over Simple Management Protocol (SMP).
    The SMP can be used to transfer the new firmware image in the background, for example, from an Android device.
    In that case, either the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr` is used to handle the image transfer.
    The :ref:`nrf_desktop_ble_smp` relies on a generic module implemented in the Common Application Framework (CAF).
    The :ref:`nrf_desktop_dfu_mcumgr` uses an application-specific implementation that allows to synchronize a secondary image slot access with the :ref:`nrf_desktop_dfu` using the :ref:`nrf_desktop_dfu_lock`.

  * :ref:`USB serial recovery <nrf_desktop_bootloader_serial_dfu>`.
    In this scenario, the MCUboot bootloader supports the USB serial recovery.
    The USB serial recovery can be used for memory-limited devices that support the USB connection.
    In this mode, unlike in the background DFU mode, the DFU image transfer is handled by the bootloader.
    The application is not running and it can be overwritten.
    Because of that, only one application slot may be used.

  For more information about the MCUboot, see the :ref:`MCUboot <mcuboot:mcuboot_wrapper>` documentation.

.. note::
    The nRF Desktop application can use either B0 or MCUboot.
    The MCUboot is not used as the second stage bootloader.

.. important::
    Make sure that you use your own private key for the release version of the devices.
    Do not use the debug key for production.

If your configuration enables the bootloader, make sure to define a static flash memory layout in the Partition Manager.
See :ref:`nrf_desktop_flash_memory_layout` documentation section for details.

Configuring the B0 bootloader
-----------------------------

To enable the B0 bootloader, select the :kconfig:option:`CONFIG_SECURE_BOOT` Kconfig option in the application configuration.

The B0 bootloader additionally requires enabling the following options in the application configuration:

* :kconfig:option:`CONFIG_SB_SIGNING_KEY_FILE` - Required for providing the signature used for image signing and verification.
* :kconfig:option:`CONFIG_FW_INFO` - Required for the application versioning information.
* :kconfig:option:`CONFIG_FW_INFO_FIRMWARE_VERSION` - Enable this option to set the version of the application after you enabled :kconfig:option:`CONFIG_FW_INFO`.
  The nRF Desktop application with the B0 bootloader configuration builds two application images: one for the S0 slot and the other for the S1 slot.
  To generate the DFU package, you need to update this configuration only in the main application image as the ``s1_image`` child image mirrors it.
  You can do that by rebuilding the application from scratch or by changing the configuration of the main image through menuconfig.
* :kconfig:option:`CONFIG_BUILD_S1_VARIANT` - Required for the build system to be able to construct the application binaries for both application's slots in flash memory.

.. _nrf_desktop_configuring_mcuboot_bootloader:

Configuring the MCUboot bootloader
----------------------------------

To enable the MCUboot bootloader, select the :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` Kconfig option in the application configuration.

The MCUboot private key path (:kconfig:option:`CONFIG_BOOT_SIGNATURE_KEY_FILE`) must be set only in the MCUboot bootloader configuration file.
The key is used both by the build system (to sign the application update images) and by the bootloader (to verify the application signature using public key derived from the selected private key).

The MCUboot bootloader configuration depends on the selected way of performing image upgrade.
For detailed information about the available MCUboot bootloader modes, see the following sections.

Swap mode
~~~~~~~~~

In the swap mode, the MCUboot bootloader moves the image to the primary slot before booting it.
The swap mode is the image upgrade mode used by default for the :ref:`background DFU <nrf_desktop_bootloader_background_dfu>`.

If the swap mode is used, the application must request firmware upgrade and confirm the running image.
For this purpose, make sure to enable :kconfig:option:`CONFIG_IMG_MANAGER` and :kconfig:option:`CONFIG_MCUBOOT_IMG_MANAGER` Kconfig options in the application configuration.
These options allow the :ref:`nrf_desktop_dfu`, :ref:`nrf_desktop_ble_smp`, and :ref:`nrf_desktop_dfu_mcumgr` to manage the DFU image.

.. note::
  When the MCUboot bootloader is in the swap mode, it can use a secondary image slot located on the external flash.
  For details on using an external flash for the secondary image slot, see the :ref:`nrf_desktop_pm_external_flash` section.

Direct-xip mode
~~~~~~~~~~~~~~~

The direct-xip mode is used for the :ref:`background DFU <nrf_desktop_bootloader_background_dfu>`.
In this mode, the MCUboot bootloader boots an image directly from a given slot, so the swap operation is not needed.
To set the MCUboot mode of operations to the direct-xip mode, make sure to enable the :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP` Kconfig option in the application configuration.
This option automatically enables :kconfig:option:`CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT` to build application update images for both slots.

Enable the ``CONFIG_BOOT_DIRECT_XIP`` Kconfig option in the bootloader configuration to make the MCUboot run the image directly from both image slots.
The nRF Desktop's bootloader configurations do not enable the revert mechanism (``CONFIG_BOOT_DIRECT_XIP_REVERT``).
When the direct-xip mode is enabled (the :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP` Kconfig option is set in the application configuration), the application modules that control the DFU transport do not request firmware upgrades and do not confirm the running image.
In that scenario, the MCUboot bootloader simply boots the image with the higher image version.

By default, the MCUboot bootloader ignores the build number while comparing image versions.
Enable the ``CONFIG_BOOT_VERSION_CMP_USE_BUILD_NUMBER`` Kconfig option in the bootloader configuration to use the build number while comparing image versions.
To apply the same option for the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr`, enable the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_VERSION_CMP_USE_BUILD_NUMBER` Kconfig option in the application configuration.

It is recommended to also enable the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_REJECT_DIRECT_XIP_MISMATCHED_SLOT` Kconfig option in the application configuration to make sure that MCUmgr rejects application image updates with invalid start address.
This prevents uploading an update image build for improper slot through the MCUmgr's Simple Management Protocol (SMP).

Serial recovery mode
~~~~~~~~~~~~~~~~~~~~

In the :ref:`USB serial recovery <nrf_desktop_bootloader_serial_dfu>` mode, the MCUboot bootloader uses a built-in foreground DFU transport over serial interface through USB.
The application is not involved in the foreground DFU transport, therefore it can be directly overwritten by the bootloader.
Because of that, the configuration with the serial recovery mode requires only a single application slot.

Enable the USB serial recovery DFU using the following configuration options:

* ``CONFIG_MCUBOOT_SERIAL`` - This option enables the serial recovery DFU.
* ``CONFIG_BOOT_SERIAL_CDC_ACM`` - This option enables the serial interface through USB.

  .. note::
    Make sure to enable and properly configure the USB subsystem in the bootloader configuration.
    See :ref:`usb_api` for more information.

If the predefined button is pressed during the boot, the MCUboot bootloader enters the serial recovery mode instead of booting the application.
The GPIO pin used to trigger the serial recovery mode is configured using Devicetree Specification (DTS).
The pin is configured with the ``mcuboot-button0`` alias.
The ``mcuboot-led0`` alias can be used to define the LED activated in the serial recovery mode.
The ``CONFIG_MCUBOOT_INDICATION_LED`` Kconfig option must be selected to enable the LED.
By default, both the GPIO pin and the LED are defined in the board's DTS file.
See :file:`boards/arm/nrf52833dongle_nrf52833/nrf52833dongle_nrf52833.dts` for an example of board's DTS file used by the nRF Desktop application.

For an example of bootloader Kconfig configuration file defined by the application, see the MCUboot bootloader debug configuration defined for nRF52833 dongle (:file:`applications/nrf_desktop/configuration/nrf52833dongle_nrf52833/child_image/mcuboot/prj.conf`).

.. note::
  The nRF Desktop devices use either the serial recovery DFU with a single application slot or the background DFU.
  Both mentioned firmware upgrade methods are not used simultaneously by any of the configurations.
  For example, the ``nrf52840dk_nrf52840`` board in ``prj_mcuboot_smp.conf`` uses only the background DFU and does not enable the serial recovery feature.

.. _nrf_desktop_bootloader_background_dfu:

Background Device Firmware Upgrade
==================================

The nRF Desktop application uses the :ref:`nrf_desktop_config_channel` and :ref:`nrf_desktop_dfu` for the background DFU process.
From the application perspective, the update image transfer during the background DFU process is very similar for both MCUboot and B0 bootloader.
The firmware update process has three stages, discussed below.
At the end of these three stages, the nRF Desktop application will be rebooted with the new firmware package installed.

.. note::
  The background firmware upgrade can also be performed over the Simple Management Protocol (SMP).
  For more details about the DFU over SMP, read the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr` documentation.

Update image generation
-----------------------

The update image is generated in the build directory when building the firmware with the enabled bootloader.

The :file:`zephyr/dfu_application.zip` file is used by both B0 and MCUboot bootloader for the background DFU through the :ref:`nrf_desktop_config_channel` and :ref:`nrf_desktop_dfu`.
The package contains firmware images along with additional metadata.
If the used bootloader boots the application directly from either slot 0 or slot 1, the host script transfers the update image that can be run from the unused slot.
Otherwise, the image is always uploaded to the slot 1 and then moved to slot 0 by the bootloader before boot.

.. note::
  Make sure to properly configure the bootloader to ensure that the build system generates the :file:`zephyr/dfu_application.zip` archive containing all of the required update images.

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

.. _nrf_desktop_image_transfer_over_smp:

Image transfer over SMP
~~~~~~~~~~~~~~~~~~~~~~~

If the MCUboot bootloader is selected, the update image can also be transferred in the background either through the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr`.
The `nRF Connect Device Manager`_ application uses binary files for the image transfer over the Simple Management Protocol (SMP).

After building your application with either :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr` enabled, the :file:`dfu_application.zip` archive is generated in the build directory.
It contains all the firmware update files that are necessary to perform DFU.
For more information about the contents of update archive, see :ref:`app_build_fota`.

To perform DFU using the `nRF Connect Device Manager`_ mobile app, complete the following steps:

.. include:: /device_guides/working_with_nrf/nrf52/developing.rst
   :start-after: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start
   :end-before: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

.. include:: /device_guides/working_with_nrf/nrf52/developing.rst
   :start-after: fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_start
   :end-before: fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_end

.. note::
  If the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_REJECT_DIRECT_XIP_MISMATCHED_SLOT` Kconfig option is enabled in the application configuration, the device rejects update image upload for the invalid slot.
  It is recommended to enable the option if the application uses MCUboot in the direct-xip mode.

Update image verification and application image update
------------------------------------------------------

Once the update image transfer is completed, the background DFU process will continue after the device reboot.
If :ref:`nrf_desktop_config_channel_script` is used, the reboot is triggered by the script right after the image transfer completes.

After the reboot, the bootloader locates the update image on the update partition of the device.
The image verification process ensures the integrity of the image and checks if its signature is valid.
If verification is successful, the bootloader boots the new version of the application.
Otherwise, the old version is used.

.. _nrf_desktop_bootloader_serial_dfu:

Serial recovery DFU
===================

The serial recovery DFU is a feature of MCUboot and it needs to be enabled in the bootloader's configuration.
For the configuration details, see the :ref:`nrf_desktop_configuring_mcuboot_bootloader` section.

To start the serial recovery DFU, the device should boot into recovery mode, in which the bootloader will be waiting for a new image upload to start.
In the serial recovery DFU mode, the new image is transferred through an USB CDC ACM class instance.
The bootloader overwrites the existing application located on the primary slot with the new application image.
If the transfer is interrupted, the device will not be able to boot the incomplete application, and the image upload must be performed again.

Once the device enters the serial recovery mode, you can use the :ref:`mcumgr <zephyr:device_mgmt>` to:

* Query information about the present image.
* Upload the new image.
  The :ref:`mcumgr <zephyr:device_mgmt>` uses the :file:`zephyr/app_update.bin` update image file.
  It is generated by the build system when building the firmware.

For example, the following line will start the upload of the new image to the device:

.. code-block:: console

  mcumgr -t 60 --conntype serial --connstring=/dev/ttyACM0 image upload build-nrf52833dongle_nrf52833/zephyr/app_update.bin

The command assumes that ``/dev/ttyACM0`` serial device is used by the MCUboot bootloader for the serial recovery.

fwupd support
=============

fwupd is an open-source project providing tools and daemon for managing the installation of firmware updates on Linux-based systems.
Together with the `LVFS (Linux Vendor Firmware Service) <LVFS_>`_, it provides a solution for vendors to easily distribute firmware for compatible devices.

The fwupd tools can communicate with the devices running the nRF Desktop application with the :ref:`nrf_desktop_bootloader_background_dfu` feature enabled.

Nordic HID plugin
-----------------

The fwupd project allows communication with multiple types of devices through various communication protocols.
The communication protocols are implemented using plugins.
The plugin associated with the DFU protocol realized through the :ref:`nrf_desktop_config_channel` is branded as ``nordic_hid``.

Adding a new device
-------------------

The device specifies which protocol is used for the communication by adding the specific device information to the plugin metadata.
For the ``nordic_hid`` plugin, this file is located at `nordic-hid.quirk`_.
The following example shows the information passed to the plugin metadata:

.. code-block:: console

   [HIDRAW\VEN_1915&DEV_52DE]
   Plugin = nordic_hid
   GType = FuNordicHidCfgChannel
   NordicHidBootloader = B0

In this console snippet:

* ``[HIDRAW\VEN_1915&DEV_52DE]`` - This line describes the device instance ID provided by the OS, which identifies the device.
* ``Plugin = nordic_hid`` and ``GType = FuNordicHidCfgChannel`` - These lines set the plugin that the device uses.
* ``NordicHidBootloader`` - This optional line selects the bootloader that the device is running.
  If the device does not have the information about the underlying bootloader, the ``NordicHidBootloader`` option is used to select a proper bootloader type.
  If there is no information about the bootloader, both in metadata and from the device, the update procedure will fail.
  The possible values are either ``B0``, ``MCUBOOT`` or ``MCUBOOT+XIP``.

.. note::
   As the ``nordic_hid`` plugin communicates with the device using the Configuration channel, the device update is not allowed through the Serial recovery DFU.

To add a new device, a pull request must be opened to the `fwupd`_ repository with a new entry to the :file:`nordic-hid.quirk` file.

LVFS and update image preparation
---------------------------------

The `LVFS (Linux Vendor Firmware Service) <LVFS_>`_ hosts firmware images that can be downloaded by Linux machines and used by the fwupd tool for the firmware update of compatible devices.
A vendor account is needed to upload a new firmware archive to the site.
Information on how to apply for an account is found at the `LVFS Getting an account`_ website.

The nRF Desktop application DFU image is delivered as a zip package, containing a manifest and one or more binary files used for the update.
To prepare the image file compatible with the LVFS, the CAB file needs to be prepared.
The CAB package must contain the DFU package generated by the |NCS|, that is :file:`dfu_application.zip`, plus metadata file with information used by the LVFS.
For more information, see the `LVFS metadata`_ site.

When the CAB archive has been built, it can be uploaded to the LVFS where the CAB archive is verified and signed.
For more information about creating CAB files, signing, and uploading the update package, see the `LVFS Uploading Firmware`_ site.

Upgrading firmware using fwupd
------------------------------

Once the update image was uploaded onto the LVFS, the firmware update procedure can be tested on end machines.

Complete the following steps:

1. Make sure that the host machine to which the updatable device running the nRF Desktop application is connected has the fwupd tool installed.
#. Fetch the information about available update images from the LVFS by using the following commands:

   .. code-block:: console

      fwupdmgr refresh
      fwupdmgr get-updates

   In this console snippet:

   * ``fwupdmgr refresh`` - This command downloads the latest metadata from the LVFS.
   * ``fwupdmgr get-updates`` - This command displays the updates available for the devices on the host system.

#. Test the update image on a limited number of devices before it goes public.
   For more information about limiting the visibility of updated images uploaded to the LVFS, see the `LVFS testing`_ site.
#. Run the following command to update the devices:

   .. code-block:: console

      fwupdmgr update

When connecting to the device, the application verifies the bootloader type.
This is done to ensure a compatible firmware is uploaded to the nRF Desktop device, that is software that can support multiple bootloaders.
The device is queried for information about bootloader using the :ref:`nrf_desktop_config_channel`.
If the device does not provide information about the bootloader type, such information can optionally be provided inside the :file:`nordic-hid.quirk` file (see the ``NordicHidBootloader`` option under `Adding a new device`_).

fwupd can fail the image update in the following cases:

* When the bootloader information stored in an updated image does not match the type reported by the device.
* When there is no information about the bootloader used on the device.

For more information about building the fwupd tool locally, see the `LVFS building fwupd`_ site.

Dependencies
************

This application uses the following |NCS| libraries and drivers:

* :ref:`lib_caf`
* :ref:`app_event_manager`
* :ref:`nrf_profiler`
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
   doc/ble_passkey.rst
   doc/ble_qos.rst
   doc/ble_scan.rst
   doc/ble_state_pm.rst
   doc/ble_state.rst
   doc/board.rst
   doc/buttons.rst
   doc/buttons_sim.rst
   doc/click_detector.rst
   doc/config_channel.rst
   doc/cpu_meas.rst
   doc/dev_descr.rst
   doc/dfu.rst
   doc/dfu_mcumgr.rst
   doc/dfu_lock.rst
   doc/factory_reset.rst
   doc/failsafe.rst
   doc/fast_pair_app.rst
   doc/fn_keys.rst
   doc/bas.rst
   doc/hid_forward.rst
   doc/hid_state.rst
   doc/hid_state_pm.rst
   doc/hids.rst
   doc/info.rst
   doc/led_state.rst
   doc/led_stream.rst
   doc/leds.rst
   doc/motion.rst
   doc/passkey.rst
   doc/power_manager.rst
   doc/nrf_profiler_sync.rst
   doc/qos.rst
   doc/selector.rst
   doc/smp.rst
   doc/settings_loader.rst
   doc/swift_pair_app.rst
   doc/usb_state_pm.rst
   doc/usb_state.rst
   doc/watchdog.rst
   doc/wheel.rst
   doc/constlat.rst
   doc/hfclk_lock.rst
   doc/event_rel_modules.rst

.. _config_desktop_app_options:

Application configuration options
*********************************

Following are the application specific configuration options that can be configured for the nRF desktop and the internal modules:

.. options-from-kconfig::
   :show-type:

API documentation
*****************

Following are the API elements used by the application.

HID reports
===========

| Header file: :file:`applications/nrf_desktop/configuration/common/hid_report_desc.h`
| Source file: :file:`applications/nrf_desktop/configuration/common/hid_report_desc.c`

.. doxygengroup:: nrf_desktop_hid_reports
   :project: nrf
   :members:

LED states
==========

| Header file: :file:`applications/nrf_desktop/configuration/common/led_state.h`
| Source file: :file:`applications/nrf_desktop/src/modules/led_state.c`

.. doxygengroup:: nrf_desktop_led_state
   :project: nrf
   :members:

USB events
==========

| Header file: :file:`applications/nrf_desktop/src/events/usb_event.h`
| Source file: :file:`applications/nrf_desktop/src/modules/usb_state.c`

.. doxygengroup:: nrf_desktop_usb_event
   :project: nrf
   :members:
