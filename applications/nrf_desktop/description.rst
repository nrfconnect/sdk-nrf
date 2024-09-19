.. _nrf_desktop_description:

nRF Desktop: Application description
####################################

.. contents::
   :local:
   :depth: 2

The nRF Desktop application supports common input hardware interfaces like motion sensors, rotation sensors, and buttons matrixes.
You can configure the firmware at runtime using a dedicated configuration channel established with the HID feature report.
The same channel is used to transmit DFU packets.

The fwupd tools can communicate with devices running the nRF Desktop application with the :ref:`nrf_desktop_bootloader_background_dfu` feature enabled.
For more information on fwupd support in the nRF desktop application, see the :ref:`nrf_desktop_fwupd` page.

.. _nrf_desktop_architecture:

Application overview
********************

The nRF Desktop application design aims at high performance, while still providing configurability and extensibility.

The application architecture is modular, event-driven and build around :ref:`lib_caf`.
This means that parts of the application functionality are separated into isolated modules that communicate with each other using application events that are handled by the :ref:`app_event_manager`.
Modules register themselves as listeners of events that they are configured to react to.
An application event can be submitted by multiple modules and it can have multiple listeners.

.. _nrf_desktop_module_component:

Module and component overview
=============================

The following figure shows the nRF Desktop modules and how they relate to other components and the :ref:`app_event_manager`.
The figure does not present all the available modules.
For example, the figure does not include the modules that are used as hotfixes or only for debug or profiling purposes.

.. figure:: /images/nrf_desktop_arch.svg
   :alt: nRF Desktop high-level design (overview)

   Application high-level design overview

For more information about nRF Desktop modules, see the :ref:`nrf_desktop_app_internal_modules` section.

.. _nrf_desktop_module_table:

Module event tables
-------------------

The :ref:`documentation page of each application module <nrf_desktop_app_internal_modules>` includes a table that shows the event-based communication for the module.

+---------------------------------------+-------------+------------------------------------+--------------+-----------------------------------------+
| Source module                         | Input event | This module                        | Output event | Sink module                             |
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

Source module
   The module that submits a given application event.
   Some of these events can have many listeners or sources.
   These are listed on the :ref:`nrf_desktop_event_rel_modules` page.

Input event
   An application event that is received by the module described in the table.

This module
   The module described in the table.
   This is the module that is the target of the input events and the source of output events directed to the sink modules.

Output event
   An application event that is submitted by the module described in the table.

Sink module
   The module that reacts on an application event.
   Some of these events can have many listeners or sources.
   These are listed on the :ref:`nrf_desktop_event_rel_modules` page.

.. note::
   Some application modules can have multiple implementations (for example, :ref:`nrf_desktop_motion`).
   In such case, the table shows the :ref:`app_event_manager` events received and submitted by all implementations of a given application module.

Module usage per hardware type
==============================

Since the application architecture is uniform and the code is shared, the set of modules in use depends on the selected device role.
A different set of modules is enabled when the application is working as a mouse, keyboard, or dongle.
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
    * Threads related to *Bluetooth®* LE (the exact number depends on the selected Link Layer)
* Application-related threads
    * Motion sensor thread (running only on mouse)
    * Settings loading thread (enabled by default only on keyboard)
    * QoS data sampling thread (running only if Bluetooth LE QoS feature is enabled)

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

When configuring heap, make sure that the values for the following options match the typical event size and the system needs:

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

All of these reports use predefined report format and provide the given information.
For example, the mouse motion is forwarded as HID mouse report.

An nRF Desktop device supports the selected subset of the HID input reports.
For example, the nRF Desktop keyboard reference design (``nrf52kbd``) supports HID keyboard report, HID consumer control report and HID system control report.

As an example, the following section describes handling HID mouse report data.

.. _nrf_desktop_hid_mouse_report_handling:

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

If the device is connected through Bluetooth LE or the device is connected through USB and :ref:`nrf_desktop_usb_state_sof_synchronization` is enabled, the :ref:`nrf_desktop_hid_state` uses a pipeline that consists of two HID reports that it creates upon receiving the first ``motion_event``.
The |hid_state| submits two ``hid_report_event`` events.
Sending the first event to the host triggers the motion sensor sample.

For the Bluetooth connections, submitting ``hid_report_sent_event`` is delayed by one Bluetooth connection interval.
Because of this delay, the :ref:`nrf_desktop_hids` requires a pipeline of two sequential HID reports to make sure that data is sent on every connection event.
Such a solution is necessary to achieve a high report rate.
For :ref:`nrf_desktop_usb_state_sof_synchronization`, the pipeline of two sequential HID reports is necessary to ensure that a USB peripheral can provide HID data on every USB poll.

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
   Only the ``nrf52840dk/nrf52840`` in ``keyboard`` configuration has hardware LEDs that can be used to display the Caps Lock and Num Lock state.

The following diagrams show the HID output report data exchange between the application modules.

* Scenario 1: Peripheral connected directly to the host

  .. figure:: /images/nrf_desktop_peripheral_host.svg
     :alt: HID output report: Data handling and transmission between host and peripheral

     HID output report: Data handling and transmission between host and peripheral

  In this scenario, the HID output report is sent from the host to the peripheral either through Bluetooth or the USB connection.
  Depending on the connection, the HID report is received by the :ref:`nrf_desktop_hids` or :ref:`nrf_desktop_usb_state`, respectively.
  The module then sends the HID output report as ``hid_report_event`` to the :ref:`nrf_desktop_hid_state` that keeps track of the HID output report states and updates state of the hardware LEDs by sending ``led_event`` to :ref:`nrf_desktop_leds`.

* Scenario 2: Dongle intermediates between the host and the peripheral

  .. figure:: /images/nrf_desktop_peripheral_host_dongle.svg
     :alt: HID output report: Data handling and transmission between host and peripheral through dongle

     HID output report: Data handling and transmission between host and peripheral through dongle

  In this scenario, the HID output report is sent from the host to the dongle using the USB connection and is received by the :ref:`nrf_desktop_usb_state`.
  The destination module then sends the HID output report as ``hid_report_event`` to the :ref:`nrf_desktop_hid_forward` that sends it to the peripheral using Bluetooth.

HID feature reports
-------------------

HID feature reports are used to transmit data between the host and an nRF Desktop device (in both directions).
The nRF Desktop uses only one HID feature report: the user config report.
The report is used by the :ref:`nrf_desktop_config_channel`.

.. note::
   The nRF Desktop also uses a dedicated HID output report to forward the :ref:`nrf_desktop_config_channel` data through the nRF Desktop dongle.
   This report is handled using the configuration channel's infrastructure and you can enable it using the :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_OUT_REPORT <config_desktop_app_options>` Kconfig option.
   See the Kconfig option's help for details about the report.

HID protocols
-------------

The following HID protocols are supported by nRF Desktop for HID input and output reports:

* Report protocol - Most widely used in HID devices.
  When establishing a connection, the host reads a HID descriptor from the HID device.
  The HID descriptor describes the format of HID reports and is used by the host to interpret the data exchanged between the HID device and the host.
* Boot protocol - Only available for mice and keyboards data.
  No HID descriptor is used for this HID protocol.
  Instead, fixed data packet formats must be used to send data between the HID device and the host.

.. _nrf_desktop_requirements:

Requirements
************

The nRF Desktop application supports several development kits related to the following hardware reference designs.
Depending on the development kit you use, you need to select the respective configuration file and :ref:`build type <nrf_desktop_requirements_build_types>`.

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
         :rows: nrf52840dk_nrf52840, nrf52833dk_nrf52833, nrf52833dk_nrf52820, nrf5340dk_nrf5340_cpuapp, nrf54l15pdk_nrf54l15_cpuapp, nrf54h20dk_nrf54h20_cpuapp

      Depending on the configuration, a DK may act either as mouse, keyboard or dongle.
      For information about supported configurations for each board, see the :ref:`nrf_desktop_board_configuration_files` section.

..

The application is designed to allow easy porting to new hardware.
See :ref:`nrf_desktop_porting_guide` for details.

.. _nrf_desktop_requirements_build_types:

nRF Desktop build types
=======================

The nRF Desktop application uses multiple files to configure each specific build type.
Those files can be easily identified by their :ref:`zephyr:application-file-suffixes`.
Before you start testing the application, you can select one of the build types supported by the application.
Not every board supports all of the mentioned build types.

See :ref:`app_build_file_suffixes` and :ref:`cmake_options` for more information.

The application supports the following build types:

.. list-table:: nRF Desktop build types
   :widths: auto
   :header-rows: 1

   * - Build type
     - File suffix
     - Supported board target
     - Description
   * - Debug (default)
     - none
     - All from `Requirements`_
     - Debug version of the application; the same as the ``release`` build type, but with debug options enabled.
   * - Release
     - ``release``
     - All from `Requirements`_
     - Release version of the application with no debugging features.
   * - Debug Fast Pair
     - ``fast_pair``
     - ``nrf52840dk/nrf52840``, ``nrf52840gmouse/nrf52840``
     - Debug version of the application with `Fast Pair`_ support.
   * - Release Fast Pair
     - ``release_fast_pair``
     - ``nrf52kbd/nrf52832``, ``nrf52840gmouse/nrf52840``
     - Release version of the application with `Fast Pair`_ support.
   * - Dongle
     - ``dongle``
     - ``nrf52840dk/nrf52840``
     - Debug version of the application that lets you generate the application with the dongle role.
   * - Keyboard
     - ``keyboard``
     - ``nrf52840dk/nrf52840``
     - Debug version of the application that lets you generate the application with the keyboard role.
   * - MCUboot QSPI
     - ``mcuboot_qspi``
     - ``nrf52840dk/nrf52840``
     - Debug version of the application that uses MCUboot with the secondary slot in the external QSPI FLASH.
   * - MCUboot SMP
     - ``mcuboot_smp``
     - ``nrf52840dk/nrf52840``, ``nrf52840gmouse/nrf52840``
     - | Debug version of the application that enables MCUmgr with DFU support and offers support for the MCUboot DFU procedure over SMP.
       | See the :ref:`nrf_desktop_bootloader_background_dfu` section for more information.
   * - WWCB
     - ``wwcb``
     - ``nrf52840dk/nrf52840``
     - Debug version of the application with the support for the B0 bootloader enabled for `Works With ChromeBook (WWCB)`_.
   * - Triple Bluetooth LE connection
     - ``3bleconn``
     - ``nrf52840dongle/nrf52840``
     - Debug version of the application with the support for up to three simultaneous Bluetooth LE connections.
   * - Quadruple LLPM connection
     - ``4llpmconn``
     - ``nrf52840dongle/nrf52840``
     - Debug version of the application with the support for up to four simultaneous Bluetooth LE connections, in Low Latency Packet Mode.
   * - Release quadruple LLPM connection
     - ``release_4llpmconn``
     - ``nrf52840dongle/nrf52840``
     - Release version of the application with the support for up to four simultaneous Bluetooth LE connections, in Low Latency Packet Mode.

.. note::
    Bootloader-enabled configurations with support for :ref:`serial recovery DFU <nrf_desktop_bootloader_serial_dfu>` or :ref:`background DFU <nrf_desktop_bootloader_background_dfu>` are set as default if they fit in the non-volatile memory.
    See :ref:`nrf_desktop_board_configuration_files` for details about which boards have bootloader included in their default configuration.

See :ref:`nrf_desktop_porting_guide` for detailed information about the application configuration and how to create build type files for your hardware.

.. _nrf_desktop_user_interface:

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

The behavior of a device can change due to power saving measures.
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

.. note::
   To simplify pairing the nRF Desktop peripherals with Windows 10 hosts, the peripherals include `Swift Pair`_ payload in the Bluetooth LE advertising data.
   By default, the Swift Pair payload is included for all of the Bluetooth local identities, apart from the dedicated local identity used for connection with an nRF Desktop dongle.

   Some of the nRF Desktop configurations also include `Fast Pair`_ payload in the Bluetooth LE advertising data to simplify pairing the nRF Desktop peripherals with Android hosts.
   These configurations apply further modifications that are needed to improve the user experience.
   See the :ref:`nrf_desktop_bluetooth_guide_fast_pair` section for details.

The nRF Desktop Bluetooth Central device scans for all bonded peripherals that are not connected.
Right after entering the scanning state, the scanning operation is uninterruptible for a predefined time (:kconfig:option:`CONFIG_DESKTOP_BLE_FORCED_SCAN_DURATION_S`) to speed up connection establishment with Bluetooth Peripherals.
After the timeout, the scanning is interrupted when any device connected to the dongle through Bluetooth becomes active.
A connected peripheral is considered active when it provides HID input reports.
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
          After the forced scan timeout, the scan is interrupted if another peripheral connected to the dongle is active.

          .. note::
              |led_note|

        * |nRF_Desktop_cancel_operation|

..

System state indication
=======================

When available, one of the LEDs is used to indicate the state of the device.
This system state LED is kept lit when the device is active.

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

Each of the nRF Desktop hardware reference designs has a slot for a dedicated debug board.
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
You can use the debug board for programming the device (and powering it).
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

You can define the amount of time after which the peripherals are suspended or powered off using the :kconfig:option:`CONFIG_CAF_POWER_MANAGER_TIMEOUT` Kconfig option.
By default, this period is set to 120 seconds.

.. important::
   When the gaming mouse is powered from USB, the power down timeout functionality is disabled.

   If a nRF Desktop device supports remote wakeup, the USB connected device goes to suspended state when USB is suspended.
   The device can then trigger remote wakeup of the connected host on user input.

.. _nrf_desktop_configuration:

Configuration
*************

|config|

This section also describes the configuration sources that are used for the default configuration.

Configuration sources
=====================

The nRF Desktop application uses the following files as configuration sources:

* Devicetree Specification (DTS) files - These reflect the hardware configuration.
  See :ref:`zephyr:dt-guide` for more information about the DTS data structure.
* :file:`_def` files - These contain configuration arrays for the application modules and are specific to the nRF Desktop application.
* Kconfig files - These reflect the software configuration.
  See :ref:`kconfig_tips_and_tricks` for information about how to configure them.

For information about differences between DTS and Kconfig, see :ref:`zephyr:dt_vs_kconfig`.

The nRF Desktop introduces application-specific Kconfig options that can be used to simplify an application configuration.
For more information, see the :ref:`nrf_desktop_application_Kconfig` page.

The nRF Desktop application can be used with various hardware boards.
For more information about board support in the application, see :ref:`nrf_desktop_board_configuration`.

The nRF Desktop application can be used together with the `nRF21540 EK` shield to benefit from an RF front-end module (FEM) for the 2.4 GHz range extension.
For more information, see  :ref:`nrf_desktop_nrf21540ek`.

You can also configure the following feature in the nRF Desktop application:

* :ref:`Memory layout <nrf_desktop_memory_layout>`
* :ref:`Bluetooth <nrf_desktop_bluetooth_guide>`
* :ref:`Bootloader and Device Firmware Update <nrf_desktop_bootloader>`

Building and running
********************

.. |sample path| replace:: :file:`applications/nrf_desktop`

The nRF Desktop application is built the same way to any other |NCS| application or sample.

.. include:: /includes/build_and_run.txt

.. include:: /includes/nRF54H20_erase_UICR.txt

.. note::
   Information about the known issues in nRF Desktop can be found in |NCS|'s :ref:`release_notes` and on the :ref:`known_issues` page.

.. _nrf_desktop_selecting_build_types:

Selecting a build type
======================

Before you start testing the application, you can select one of the :ref:`nrf_desktop_requirements_build_types`, depending on your development kit.
See :ref:`app_build_file_suffixes` and :ref:`cmake_options` for information about how to select a build type.

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
      * Device Type: Mouse
      * Notification Type: Fast Pair
      * Data-Only connection: true
      * No Personalized Name: false

   See :ref:`ug_bt_fast_pair_provisioning` documentation for the following information:

   * Registering a Fast Pair Provider
   * Provisioning a Fast Pair Provider in |NCS|

.. nrf_desktop_fastpair_important_start

.. important::
   This is the debug Fast Pair provisioning data obtained by Nordic for the development purposes.
   Do not use it in production.

   To test with the debug mode Model ID, you must configure the Android device to include the debug results while displaying the nearby Fast Pair Providers.
   For details, see `Verifying Fast Pair`_ in the GFPS documentation.

.. nrf_desktop_fastpair_important_end

.. _nrf_desktop_testing_steps:

Testing
=======

You can build and test the application in various configurations.
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
   You can now use the devices simultaneously.

   .. note::
      You can manually start the scanning for new peripheral devices by pressing the **SW1** button on the dongle for a short time.
      This might be needed if the dongle does not connect with all the peripherals before scanning is interrupted by a timeout.

#. Move the mouse and press any key on the keyboard.
   The input is reflected on the host.

   .. note::
      When a :ref:`configuration with debug features <nrf_desktop_requirements_build_types>` is enabled, for example logger and assertions, the gaming mouse report rate can be significantly lower.

      Make sure that you use the ``release``configurations before testing the mouse report rate.
      For the ``release`` configurations, you should observe a 500-Hz report rate when both the mouse and the keyboard are connected and a 1000-Hz rate when only the mouse is connected.

#. Switch the Bluetooth peer on the gaming mouse by pressing the **Precise Aim** button (see `User interface`_).
   The color of **LED1** changes from red to green and the LED starts blinking rapidly.
#. Press the **Precise Aim** button twice quickly to confirm the selection.
   After the confirmation, **LED1** starts breathing and the mouse starts the Bluetooth advertising.
#. Connect to the mouse with an Android phone, a laptop, or any other Bluetooth Central.

After the connection is established and the device is bonded, you can use the mouse with the connected device.

.. _nrf_desktop_measuring_hid_report_rate:

Measuring HID report rate
-------------------------

You can measure a HID report rate of your application to assess the performance of your HID device.
This measurement allows you to check how often the host computer can get user's input from the HID device.

Prerequisites
~~~~~~~~~~~~~

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

Use the configuration with the ``release`` file suffix for the HID report rate measurement.
Debug features, such as logging or assertions, decrease the application performance.

Use the nRF Desktop configuration that acts as a HID mouse reference design for the report rate measurement, as the motion data polling is synchronized with sending HID reports.

Make sure your chosen motion data source will generate movement in each poll interval.
Without a need for user's input, you can generate HID reports that contain mouse movement data.
Use the :ref:`Motion simulated module <nrf_desktop_motion_report_rate>` for this.

To build an application for evaluating HID report rate, run the following command:

   .. parsed-literal::
      :class: highlight

      west build -p -b *board_target* -- \
      -DFILE_SUFFIX=release \
      -DCONFIG_DESKTOP_MOTION_SIMULATED_ENABLE=y \

Report rate measuring tips
~~~~~~~~~~~~~~~~~~~~~~~~~~

See the following list of possible scenarios and best practices:

* If two or more peripherals are connected through the dongle, and all of the devices support LLPM, the Bluetooth LE LLPM connection events split evenly among all of the peripherals connected through that dongle.
  It results in decreased HID report rate.
  For example, you should observe a 500 Hz HID report rate when both mouse and keyboard are connected through the dongle and a 1000 Hz rate when only the mouse is connected.
* If a HID peripheral is connected through a dongle, the dongle's performance must be taken into account when measuring the report rate.
  Delays related to data forwarding on the dongle also result in reduced report rate.
* If the device is connected through Bluetooth LE directly to the HID host, the host sets the Bluetooth LE connection interval.
  A Bluetooth LE peripheral can suggest the preferred connection parameters.
  You can set the suggested connection interval using the :kconfig:option:`CONFIG_BT_PERIPHERAL_PREF_MIN_INT` and :kconfig:option:`CONFIG_BT_PERIPHERAL_PREF_MAX_INT` Kconfig options.
  Set parameters are not enforced, meaning that the HID host may still eventually use a value greater than the maximum connection interval requested by a peripheral.
* Radio frequency (RF) noise can negatively affect the HID report rate for wireless connections.
  If a HID report fails to be delivered in a given Bluetooth LE LLPM connection event, it is retransmitted in the subsequent connection event, which effectively reduces the report rate.
  By avoiding congested RF channels, the :ref:`nrf_desktop_ble_qos` helps to achieve better connection quality and a higher report rate.
* For the USB device connected directly, the applicable options will vary depending on the used USB stack:

  * If you use the USB legacy stack, you can configure your preferred USB HID poll interval using the :kconfig:option:`CONFIG_USB_HID_POLL_INTERVAL_MS` Kconfig option.
    By default, the :kconfig:option:`CONFIG_USB_HID_POLL_INTERVAL_MS` Kconfig option is set to ``1`` to request the lowest possible poll interval.
  * If you use the USB next stack, you can configure your preferred USB HID polling rate using the ``in-polling-period-us`` property of a DTS node compatible with ``zephyr,hid-device``.
    The lowest polling rate that is supported by the USB High-Speed is 125 µs, which corresponds to 8 kHz report rate.
    The lowest polling rate supported by devices that do not support USB High-Speed is 1000 µs, which corresponds to 1 kHz report rate.

  Polling parameters are not enforced, meaning that the HID host may still eventually use a value greater than the USB polling parameter requested by a peripheral.

USB High-Speed
~~~~~~~~~~~~~~

You can use the nRF54H20 DK to evaluate USBHS.
Use the ``release`` configuration and slightly modify the simulated motion module's configuration to ensure that non-zero motion values are reported in every HID report.
See an example of the build command:

   .. parsed-literal::
      :class: highlight

      west build -p -b nrf54h20dk/nrf54h20/cpuapp -- \
      -DFILE_SUFFIX=release \
      -DCONFIG_DESKTOP_MOTION_SIMULATED_ENABLE=y \
      -DCONFIG_DESKTOP_MOTION_SIMULATED_EDGE_TIME=8192 \
      -DCONFIG_DESKTOP_MOTION_SIMULATED_SCALE_FACTOR=5

For information about generating motion data, see the :ref:`nrf_desktop_motion_report_rate` documentation section.

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
