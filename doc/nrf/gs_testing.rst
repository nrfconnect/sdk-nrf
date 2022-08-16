.. _gs_testing:

Testing and debugging an application
####################################

.. contents::
   :local:
   :depth: 2

|application_sample_definition|

Follow the instructions in the testing section of the application's documentation to make sure that the application runs as expected.

Information about the current state of the application is usually provided through the LEDs or through UART, or through both.
See the user interface section of the application's documentation for description of the LED states or available UART commands.

.. _testing_vscode:

How to connect with |nRFVSC|
****************************

The |nRFVSC| is a complete IDE for developing applications for nRF91, nRF53 and nRF52 Series devices.
The extension pack includes nRF Terminal, which is an integrated serial port and RTT terminal to connect to your board.
For detailed instructions, see the `nRF Terminal documentation`_ on the `nRF Connect for Visual Studio Code`_ documentation site.

.. note::

   .. include:: /gs_modifying.rst
      :start-after: modify_vsc_compiler_options_start
      :end-before: modify_vsc_compiler_options_end

.. _putty:

How to connect with PuTTY
*************************

To see the UART output, connect to the development kit with a terminal emulator, for example, PuTTY.

Connect with the following settings:

 * Baud rate: 115200
 * 8 data bits
 * 1 stop bit
 * No parity
 * HW flow control: None

If you want to send commands through UART, make sure to configure the required line endings and turn on local echo and local line editing:

.. figure:: /images/putty.svg
   :alt: PuTTY configuration for sending commands through UART

UART can also be used for logging purposes as one of the :ref:`logging backends <ug_logging_backends>`.

.. _testing_rtt:

How to use RTT
**************

To view the logging output using Real Time Transfer (RTT), modify the configuration settings of the application to override the default UART console:

 .. code-block:: none

    CONFIG_USE_SEGGER_RTT=y
    CONFIG_RTT_CONSOLE=y
    CONFIG_UART_CONSOLE=n

SEGGER's J-Link RTT can also be used for logging purposes as one of the :ref:`logging backends <ug_logging_backends>`.

.. note::

   SEGGER's J-Link RTT is part of the `J-Link Software and Documentation Pack`_.
   You must have this software installed on your platform to use RTT.

.. _testing_rtt_connect:

Connecting using RTT
====================

To run RTT on your platform, complete the following steps:

1. From the J-Link installation directory, open the J-Link RTT Viewer application:

   * On Windows, the executable is called :file:`JLinkRTTViewer.exe`.
   * On Linux, the executable is called :file:`JLinkRTTViewerExe`.

#. Select the following options to configure your connection:

   * Connection to J-Link: USB
   * Target Device: Select your IC from the list
   * Target Interface and Speed: SWD, 4000 KHz
   * RTT Control Block: Auto Detection

   .. figure:: /images/rtt_viewer_configuration.png
      :alt: Example of RTT Viewer configuration

#. Click :guilabel:`OK` to view the logging output from the device.

.. _lte_connect:

How to connect with LTE Link Monitor
************************************

To connect to nRF9160-based kits (for example, the nRF9160 DK or Nordic Thingy:91), you can also use `LTE Link Monitor`_, which is implemented in `nRF Connect for Desktop`_.
This application is used to establish LTE communication with the nRF9160 modem through AT commands, and it also displays the UART output.

To connect to the nRF9160-based kit with LTE Link Monitor, perform the following steps:

1. Launch the LTE Link Monitor app.

   .. note::

      Make sure that :guilabel:`Automatic requests` (in the **SETTINGS** section) is enabled in LTE Link Monitor.

#. Connect the nRF9160-based kit to the PC with a USB cable.
#. Power on the nRF9160-based kit.
#. Click :guilabel:`Select Device` and select the particular kit entry from the drop-down list in the LTE Link Monitor.
#. Observe that the LTE Link monitor app starts AT communication with the modem of the nRF9160-based kit and shows the status of the communication in the display terminal.
   The app also displays any information that is logged on UART.

   .. note::

      In the case of nRF9160 DK, the reset button must be pressed to restart the device and to start the application.

.. _gs_debugging:

Debugging an application
************************

To debug an application, set up the debug session as described in `Debugging an application`_ in the |nRFVSC| documentation.

If you use a multi-core SoC, for example from the nRF53 Series, and you only wish to debug the application core firmware, a single debug session is sufficient.
To debug the firmware running on the network core, you need to set up two separate debug sessions: one for the network core and one for the application core.
When debugging the network core, the application core debug session runs in the background and you can debug both cores if needed.

Complete the following steps to start debugging the network core:

1. Set up sessions for the application core and network core as mentioned in `Debugging an application`_.
#. Select the appropriate CPU for debugging in each session, corresponding to the application core and the network core of your SoC, respectively.
#. Once both sessions are established, execute the code on the application core.

   The startup code releases the ``NETWORK.FORCEOFF`` signal to start the network core and allocates the necessary GPIO pins for it.
#. Start code execution on the network core in the other debug session.

If you want to reset the network core while debugging, make sure to first reset the application core and execute the code.

Debugging secure and non-secure firmware
========================================

When using an ``_ns`` build target, by default you can only debug firmware in the non-secure part of the application core firmware.

To debug firmware running in the secure part, you need to build Trusted Firmware-M with debug symbols enabled and load the symbols during the debugging session.
To build Trusted Firmware-M with debug symbols, set the :kconfig:option:`CONFIG_TFM_CMAKE_BUILD_TYPE_RELWITHDEBINFO` Kconfig option.
To load the Trusted Firmware-M debug symbols in the GDB tool, run the following command and answer ``y`` if prompted by the tool:

.. code-block:: console

   (gdb) add-symbol-file build/tfm/bin/tfm_s.elf

For more information about the GDB tool, see `Debug tools`_.

Debug configuration
===================

When you are following the `Debugging an application`_ process in the |nRFVSC| and select the :guilabel:`Enable debug options` checkbox in the **Add Build Configuration** page, the following Kconfig options are set to ``y`` when you add the configuration:

* :kconfig:option:`CONFIG_DEBUG_OPTIMIZATIONS` - This option limits the optimizations made by the compiler to only those that do not impact debugging.
* :kconfig:option:`CONFIG_DEBUG_THREAD_INFO` - This option adds additional information to the thread object, so that the debugger can discover the threads.
  This will work for any debugger.

You can also set these options to ``y`` manually.
There are many more Kconfig options for debugging that are specific to different modules.
For details, see the respective documentation pages of the modules.

Debug build types
-----------------

Some applications and samples provide a specific build type that enables additional debug functionalities.
You can select build types when you are :ref:`configuring the build settings <gs_modifying_build_types>`.

Debug logging
-------------

You can use logging system to get more information about the state of your application.
Logs are integrated to many various modules and subsystems in the |NCS| and Zephyr.
These logs are visible once you configure logger for your application.

You can also configure log level per logger module, for example to get more information about a given subsystem.
See :ref:`ug_logging` for details on how to enable and configure logs.

Debug libraries
---------------

The |NCS| also provides several libraries and drivers for debugging different components.
For example:

* You can use :ref:`lib_debug` for any of your applications, for example to measure CPU load or trace hardware peripheral events on pins.

* You can use :ref:`nrf_profiler` to measure performance and debug applications without introducing big performance overhead.
  This option requires introducing additional code changes: your application must register profiler events and log their occurrences.

* When working with the :ref:`SEGGER J-Link with the RTT feature <testing_rtt_connect>`, you can use the :ref:`lib_eth_rtt`, which is useful for handling data transfer.

* The Thread protocol implementation offers :ref:`pre-built libraries with debug symbols <thread_ug_feature_updating_libs>`.

* The Zigbee protocol implementation offers :ref:`lib_zigbee_osif` and :ref:`lib_zigbee_shell` with custom Kconfig options that you can set for debugging.

Debugging stack overflows
-------------------------

One of the potential root causes of fatal errors in an application are stack overflows.
Read the Stack Overflows section on the :ref:`zephyr:fatal` page in the Zephyr documentation to learn about stack overflows and how to debug them.

You can also use a separate module to make sure that the stack sizes used by your application are big enough to avoid stack overflows.
One of such modules is for example Zephyr's :ref:`zephyr:thread_analyzer`.

Debug tools
===========

The main recommended tool for debugging in the |NCS| is the `GNU Project Debugger`_ (GDB tool).

* When working with the |nRFVSC|, use the GDB tool after adding the required Kconfig options to the :file:`prj.conf` file.
* If you are working from the command line, you can use west with the GDB tool.
  For details, read the :ref:`Debugging with west debug <zephyr:west-debugging>` section on the :ref:`zephyr:west-build-flash-debug` page in the Zephyr documentation.

A useful tool for debugging the communication over Bluetooth and mesh networking protocols, such as :ref:`ug_thread` or :ref:`ug_zigbee`, is the `nRF Sniffer for 802.15.4`_.
The nRF Sniffer allows you to look into data exchanged over-the-air between devices.
