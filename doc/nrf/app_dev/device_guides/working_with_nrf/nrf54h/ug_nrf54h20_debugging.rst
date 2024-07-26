.. _ug_nrf54h20_debugging:

nRF54H20 debugging
##################

.. contents::
   :local:
   :depth: 2

The main recommended tool for debugging in the |NCS| for the limited sampling of the nRF54H20 DK is the `GNU Project Debugger`_ (GDB tool).

When working from the command line, you can use west with the GDB tool.
For details, read the :ref:`Debugging with west debug <zephyr:west-debugging>` section on the :ref:`zephyr:west-build-flash-debug` page in the Zephyr documentation.

A useful tool for debugging the communication over Bluetooth® is the `nRF Sniffer for Bluetooth LE`_.
The nRF Sniffer allows you to look into data exchanged over-the-air between devices.

Debug configuration
*******************

Set the following Kconfig options to ``y`` for the images running on the cores you want to debug:

* :kconfig:option:`CONFIG_DEBUG_OPTIMIZATIONS` - This option limits the optimizations made by the compiler to only those that do not impact debugging.
* :kconfig:option:`CONFIG_DEBUG_THREAD_INFO` - This option adds additional information to the thread object so that the debugger can discover the threads.
  This will work for any debugger.

Debug configurations
********************

Some applications and samples provide a specific configuration that enables additional debug functionalities.
You can select custom configurations when you are :ref:`configuring the build settings <cmake_options>`.


Debugging single-core applications
**********************************

To debug single-core applications, you can use the ``west debug`` command to start a single debug session with GDB.

Debugging multi-core applications
*********************************

To debug the firmware running also on cores other than the application core, you need to set up a separate debug session for each one of the cores you want to debug.
When debugging another core, the application core debug session runs in the background and you can debug both cores if needed.

If you want to reset the other cores while debugging, make sure to first reset the application core and execute the code.

Using GDB as an external debugger
*********************************

An external debugger can access the device using the Debug Access Port (DAP).
The DAP is a standard Arm® CoreSight™ serial wire debug port (SWJ-DP) that implements the serial wire debug (SWD) protocol with a two-pin serial interface.

There are several access ports that connect to different parts of the system:

   * AHB-AP 0: application core access port ID
   * AHB-AP 1: radio core access port ID
   * AHB-AP 2: Secure Domain access port ID
   * AHB-AP 3: Auxiliary access port ID
   * CTRL-AP 4: Device level control access port ID
   * APB-AP 5: CoreSight™ subsystem access port ID

The following sections describe how to debug the nRF54H20 using GDB as the external debugger with J-link.

Selecting the core
==================

To debug a specific core using ``JLinkExe`` do the following:

1. Run J-Link on the application core::

      JLinkExe -USB <SEGGER-ID> -if SWD -Device Cortex-M33

   You can use this command to run J-Link also on other Arm cores.
   You can find the ``SEGGER-ID`` as follows:

   * Check the ``SEGGER ID`` printed on the label on the bottom side of the DK.
   * Run the ``nrfjprog --ids`` command.

   If just one DK is connected to the machine, defining ``SEGGER-ID`` is not necessary.
   If more than one DK is connected to the machine and ``SEGGER-ID`` is undefined, a pop up window will appear where you can manually select the ID of the DK you want to run J-Link on.

   .. note::
      PPR core debugging is not functional in the initial limited sampling.

#. Connect to the application core::

      exec CORESIGHT_SetIndexAHBAPToUse = <Domain AP index>
      connect

   ``<Domain AP index>`` is the ID of the access port.

J-Link script files
===================

You can also create J-Link script files in your local directory and add them to a GDB server call for a remote debugging session.

1. Create a script file with the following content::

      void ConfigTargetSettings(void) {
      J-Link_CORESIGHT_AddAP(<Domain AP index>, CORESIGHT_AHB_AP);
      CORESIGHT_IndexAHBAPToUse = <Domain AP index>;
      }

2. Add the script file to the GDB server call::

      -scriptfile [*full_path/to/script_file_name*]

Debug logging
*************

You can use the logging system to get more information about the state of your application.
Logs are integrated into various modules and subsystems in the |NCS| and Zephyr.
These logs are visible once you configure the logger for your application.

You can also configure log level per logger module to, for example, get more information about a given subsystem.
See :ref:`ug_nrf54h20_logging` for details on how to enable and configure logs on the nRF54H20 DK.

Debugging stack overflows
*************************

One of the potential root causes of fatal errors in an application are stack overflows.
Read the Stack Overflows section on the :ref:`zephyr:fatal` page in the Zephyr documentation to learn about stack overflows and how to debug them.

You can also use a separate module, such as Zephyr's :ref:`zephyr:thread_analyzer`, to make sure that the stack sizes used by your application are big enough to avoid stack overflows.
