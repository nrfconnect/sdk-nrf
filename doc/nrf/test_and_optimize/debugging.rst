.. _gs_debugging:
.. _debugging:

Debugging an application
########################

.. contents::
   :local:
   :depth: 2

The main recommended tool for debugging in the |NCS| is `nRF Debug <Debugging overview_>`_ of the |nRFVSC|.
The tool uses `Microsoft's debug adaptor`_ and integrates custom debugging features specific for the |NCS|.

.. tabs::

   .. group-tab:: nRF Connect for Visual Studio Code

      Use nRF Debug after adding the required Kconfig options to the :file:`prj.conf` file.
      For details, see the `How to debug an application`_ section in the |nRFVSC| documentation.

      Read also the `Debugging overview`_ and other guides in the debugging section of the extension documentation for more information about debugging in the |nRFVSC|, for example testing and debugging with custom options.

   .. group-tab:: Command line

      Use west with nRF Debug.
      For details, see the :ref:`Debugging with west debug <zephyr:west-debugging>` section on the :ref:`zephyr:west-build-flash-debug` page in the Zephyr documentation.

.. note::
    For hands-on tutorials on debugging and troubleshooting applications in the |NCS|, see `Lesson 2 - Debugging and troubleshooting`_ in the `nRF Connect SDK Intermediate course`_ in the Nordic Developer Academy.

Debug configuration
*******************

When you are following the `How to debug an application`_ process in the |nRFVSC| and select the :guilabel:`Enable debug options` checkbox in the **Add Build Configuration** page, the following Kconfig options are set to ``y`` when you add the configuration:

* :kconfig:option:`CONFIG_DEBUG_OPTIMIZATIONS` - This option limits the optimizations made by the compiler to only those that do not impact debugging.
* :kconfig:option:`CONFIG_DEBUG_THREAD_INFO` - This option adds additional information to the thread object, so that the debugger can discover the threads.
  This will work for any debugger.

You can also set these options to ``y`` manually.
There are many more Kconfig options for debugging that are specific to different modules.
For details, see the respective documentation pages of the modules.

Debug build types
=================

Some applications and samples provide a specific build type that enables additional debug functionalities.
You can select build types when you are :ref:`configuring the build settings <modifying_build_types>`.

Debug logging
=============

You can use logging system to get more information about the state of your application.
Logs are integrated to many various modules and subsystems in the |NCS| and Zephyr.
These logs are visible once you configure logger for your application.

You can also configure log level per logger module, for example to get more information about a given subsystem.
See :ref:`ug_logging` for details on how to enable and configure logs.

Debug libraries
===============

The |NCS| also provides several libraries and drivers for debugging different components.
For example:

* You can use :ref:`lib_debug` for any of your applications, for example to measure CPU load or trace hardware peripheral events on pins.

* You can use :ref:`nrf_profiler` to measure performance and debug applications without introducing big performance overhead.
  This option requires introducing additional code changes: your application must register profiler events and log their occurrences.

* When working with the :ref:`SEGGER J-Link with the RTT feature <testing_rtt_connect>`, you can use the :ref:`lib_eth_rtt`, which is useful for handling data transfer.

* The Thread protocol implementation offers :ref:`pre-built libraries with debug symbols <thread_ug_feature_updating_libs>`.

* The Zigbee protocol implementation offers :ref:`lib_zigbee_osif` and :ref:`lib_zigbee_shell` with custom Kconfig options that you can set for debugging.

Debugging stack overflows
=========================

One of the potential root causes of fatal errors in an application are stack overflows.
Read the Stack Overflows section on the :ref:`zephyr:fatal` page in the Zephyr documentation to learn about stack overflows and how to debug them.

You can also use a separate module to make sure that the stack sizes used by your application are big enough to avoid stack overflows.
One of such modules is for example Zephyr's :ref:`zephyr:thread_analyzer`.

Debugging multi-core SoCs
*************************

If you use a multi-core SoC, for example from the nRF53 Series, and you only wish to debug the application core firmware, a single debug session is sufficient.
To debug the firmware running on the network core, you need to set up two separate debug sessions: one for the network core and one for the application core.
When debugging the network core, the application core debug session runs in the background and you can debug both cores if needed.

Complete the following steps to start debugging the network core:

1. Set up sessions for the application core and network core as mentioned in the `How to debug applications for a multi-core System on Chip`_ section in the |nRFVSC| documentation.
#. Select the appropriate CPU for debugging in each session, corresponding to the application core and the network core of your SoC, respectively.
#. Once both sessions are established, execute the code on the application core.

   The startup code releases the ``NETWORK.FORCEOFF`` signal to start the network core and allocates the necessary GPIO pins for it.
#. Start code execution on the network core in the other debug session.

If you want to reset the network core while debugging, make sure to first reset the application core and execute the code.

.. _debugging_spe_nspe:

Debugging secure and non-secure firmware
****************************************

When using a build target with :ref:`CMSE enabled <app_boards_spe_nspe_cpuapp_ns>` (``_ns``), by default you can only debug firmware in the non-secure environment of the application core firmware.

To debug firmware running in the secure environment, you need to build Trusted Firmware-M with debug symbols enabled and load the symbols during the debugging session.
To build Trusted Firmware-M with debug symbols, set the :kconfig:option:`CONFIG_TFM_CMAKE_BUILD_TYPE_RELWITHDEBINFO` Kconfig option.

nRF Debug in the |nRFVSC| automatically loads the Trusted Firmware-M debug symbols.

Enabling non-halting debugging with Cortex-M Debug Monitor
**********************************************************

The debugging process can run in two modes.
The halt-mode debugging stops the CPU when a debug request occurs.
The monitor-mode debugging lets a CPU debug parts of an application while crucial functions continue.
Unlike halt-mode, the monitor-mode is useful for scenarios like PWM motor control or Bluetooth, where halting the entire application is risky.
The CPU takes debug interrupts, running a monitor code for J-Link communication and user-defined functions.

Use the following steps to enable monitor-mode debugging in the |NCS|:

1. In the application configuration file, set the Kconfig options :kconfig:option:`CONFIG_CORTEX_M_DEBUG_MONITOR_HOOK` and :kconfig:option:`CONFIG_SEGGER_DEBUGMON`.
2. Attach the debugger to the application.
3. Depending on debugger you are using, enable monitor-mode debugging:

  * For nRF Debug in the |nRFVSC|, enter ``-exec monitor exec SetMonModeDebug=1`` in the debug console.
  * For debugging using Ozone, enter ``Exec.Command("SetMonModeDebug = 1");`` in the console.

For more information about monitor-mode debugging, see Zephyr's :ref:`zephyr:debugmon` documentation and SEGGER's `Monitor-mode Debugging <Monitor-mode Debugging_>`_ documentation.

Remote Debugging with Memfault
******************************

To collect coredumps from a remote device that has been deployed to the field, enable :ref:`Memfault <ug_memfault>`.
Memfault collects device state at the time of a crash for debugging crashes remotely.
Additionally, you can use Memfault to collect metrics for monitoring device health, including battery life, memory usage, and CPU usage.
For more information on enabling Memfault for your project, see :ref:`ug_memfault`.

Other debugging tools
*********************

In addition to nRF Debug, a useful tool for debugging the communication over Bluetooth and mesh networking protocols, such as :ref:`ug_thread` or :ref:`ug_zigbee`, is the `nRF Sniffer for 802.15.4`_.
The nRF Sniffer allows you to look into data exchanged over-the-air between devices.

Check also the different `nRF Connect for Desktop`_ apps, which you can use to test and optimize your application for different use cases.
