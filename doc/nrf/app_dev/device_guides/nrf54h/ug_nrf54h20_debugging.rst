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

      .. note::
         |nrfjprog_deprecation_note|

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

Debugging errors reported by SDFW
*********************************

The Secure Domain Firmware (SDFW) report errors through the ``CTRL-AP.BOOTSTATUS`` register.
You can read this value using the ``nrfutil device x-boot-status-get`` command:

.. parsed-literal::
   :class: highlight

    nrfutil device x-boot-status-get --help

SDFW errors
===========

A value of ``0`` indicates *no error*, while any other value signifies that an error has occurred.

.. note::
   ``0`` is the reset value of this register.
   Therefore, a device experiencing erratic behavior might still report ``0`` incorrectly.
   For example, this may occur if the device is in a boot loop.


Several components report errors through this register.
The first 4 bits of the first byte is reserved for future use and must be ``0``, the second 4 bits of the bootstatus indicate which component reported an error:

 * System Controller ROM -> ``0x01``
 * Secure Domain ROM -> ``0x02``
 * System Controller Firmware -> ``0x0A``
 * Secure Domain Firmware -> ``0x0B``

.. note::
      Each one of these values has a different handling of the remaining bits.
      This chapter only describes the semantics for Secure Domain Firmware errors (``0x0B******``).


The second byte describes the boot step within the SDFW booting process that reported the failure.
For more information, see `SDFW Boot Steps`_
The last two bytes contain the lower 16 bits of the error code.

For example, ``0x0BA1FF62`` indicates that the SDFW reported an error in the BICR validate step (``0xA1``) with error message ``0xFF62``, or ``-158``.

SDFW Boot Steps
---------------

The boot steps are defined as follows:

.. parsed-literal::
   :class: highlight

    #define BOOTSTATUS_STEP_START_GRTC 0x06
    #define BOOTSTATUS_STEP_SDFW_UPDATE 0x30
    #define BOOTSTATUS_STEP_BELLBOARD_CONFIG 0x4F
    #define BOOTSTATUS_STEP_SUIT_INIT 0x6F
    #define BOOTSTATUS_STEP_DOMAIN_ALLOCATE 0x8F
    #define BOOTSTATUS_STEP_MEMORY_FINALIZE 0x91
    #define BOOTSTATUS_STEP_TRACEHOST_INIT 0x93
    #define BOOTSTATUS_STEP_CURRENT_LIMITED 0xA0
    #define BOOTSTATUS_STEP_BICR_VALIDATE 0xA1
    #define BOOTSTATUS_STEP_DOMAIN_BOOT 0xAF
    #define BOOTSTATUS_STEP_ADAC 0xC0
    #define BOOTSTATUS_STEP_SERVICES 0xCF

Errors are not accumulated, as only one error is reported even if multiple boot steps fail.
The SDFW chooses which error to report if multiple errors occur.
The types of errors that can overwrite other errors are the following:

 * An update of SDFW has failed.
 * The SDFW is unable to initialize the ADAC over CTRL-AP communication.

The following is a short description of the failures related to the boot steps:

 * ``BOOTSTATUS_STEP_START_GRTC``  - SDFW was unable to initialize the driver used for the GRTC.
 * ``BOOTSTATUS_STEP_SDFW_UPDATE`` - SDROM was instructed to install an update before last reset, and is indicating that an error occurred while performing the update.
 * ``BOOTSTATUS_STEP_BELLBOARD_CONFIG`` - SDFW was unable to apply the static bellboard configuration.
 * ``BOOTSTATUS_STEP_SUIT_INIT`` - A SUIT related error occurred.
 * ``BOOTSTATUS_STEP_DOMAIN_ALLOCATE`` - An error occurred while allocating global resources.
 * ``BOOTSTATUS_STEP_MEMORY_FINALIZE`` - SDFW was unable to apply the required memory protection configuration.
 * ``BOOTSTATUS_STEP_TRACEHOST_INIT`` - An error occurred when initializing the trace host.
 * ``BOOTSTATUS_STEP_CURRENT_LIMITED`` - System Controller ROM booted the system in current limited mode due to an issue in the BICR.
 * ``BOOTSTATUS_STEP_BICR_VALIDATE`` - SDFW discovered an invalid BICR. Note that not seeing this issue does not imply that there are no issues in the BICR.
 * ``BOOTSTATUS_STEP_DOMAIN_BOOT`` - An error occurred while booting the local domains.
 * ``BOOTSTATUS_STEP_ADAC`` - An error occurred while initializing the ADAC transport.
 * ``BOOTSTATUS_STEP_SERVICES`` - An error occurred while initializing the SSF server.
