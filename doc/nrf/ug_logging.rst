﻿.. _ug_logging:

Logging in |NCS|
################

|NCS| provides a logging system that offers different logging possibilities for testing and debugging.
The system is multilevel, meaning that it can be enabled and configured independently for different modules.
This allows you to choose the logging information you want to receive and how it should be gathered.
Several logging backends allow for defining the interface and protocol used for gathering logging messages.

You can obtain logging information from the following sources:

* Custom applications
* Network protocol applications
* Samples, drivers, and libraries
* Zephyr system kernel

For example, you can gather logging information about network events from network protocol applications and logging information about threads from Zephyr system kernel at the same time.

This flexibility comes at a cost: the system's complexity requires different approach to configuration for single components and backends.
See the following sections for more information.

.. _ug_logging_zephyr:

Zephyr system logger
********************

Zephyr allows you to configure log levels independently for each module and also offers many general logger engine options, for example default levels, discarding messages, overflow behavior, and more.
You can use the Zephyr system logger in |NCS| for the following use cases:

* Internal system logs - Useful for analyzing detailed system performance, for example issues with threads stack sizes.
* Logs from external components - Useful for gathering messages from sample applications and custom modules.

For more information about configuring Zephyr's logger, see :ref:`zephyr:logging_api`.

.. _ug_logging_net_application:

Network protocol application logging
************************************

In |NCS|, network protocol applications are using separate threads of the Zephyr system.
Each one of these threads can use its own logging module.
For this reason, each must be configured individually.

Thread
======

As a result of being able to work as a bare metal application, OpenThread uses its own internal logging module for printing messages.
The OpenThread logger offers several configuration options that can be used to select log levels, regions, output methods, and more.
For detailed information about available options and API, visit `OpenThread logging`_.

In |NCS|, OpenThread runs as a Zephyr system application, meaning that it can be configured by setting options in the system configuration file, without modifying the source code.
For details about how to configure OpenThread logger in |NCS|, see :ref:`thread_ug_logging_options`.

Zigbee
======

Zigbee stack logs are independent from Zephyr system logger and must be configured by setting dedicated options in the system configuration file.
For detailed information, see :ref:`zigbee_ug_logging`.

.. _ug_logging_backends:

Logging backends
****************

Logging backends are low-level drivers that can be configured to work with hardware interface for sending log messages.
You can enable many backends at once and make them work simultaneously, if you want to gather the same kind of information from different sources.

.. tip::
    As a recommendation, avoid selecting many backends using the same physical interface instance to avoid mutual interference.
    For example, instead of ``UART0`` for both, choose ``UART0`` for one backend and ``UART1`` for another.

UART
====

The UART interface can be configured as a logging backend using the following Kconfig options:

* :option:`CONFIG_LOG_BACKEND_UART` - This option enables the UART logging backend.
* :option:`CONFIG_LOG_BACKEND_UART_SYST_ENABLE` - This option is used to output logs in system format.

For information about how to see UART output, see :ref:`putty`.

RTT
===

SEGGER's J-Link RTT backend logging can be handled with the following Kconfig options:

* :option:`CONFIG_LOG_BACKEND_RTT` - This option enables RTT logging backend.
* :option:`CONFIG_LOG_BACKEND_RTT_MODE_DROP` - This option enables the mode in which messages that do not fit the buffer are dropped.
* :option:`CONFIG_LOG_BACKEND_RTT_MODE_BLOCK` - This option enables the mode in which the device is blocked until a message is transferred.
* :option:`CONFIG_LOG_BACKEND_RTT_OUTPUT_BUFFER_SIZE` - This option defines the size of the buffer used for storing data prepared for sending.
* :option:`CONFIG_LOG_BACKEND_RTT_RETRY_CNT` - This option defines the number of retries before a message is dropped.
* :option:`CONFIG_LOG_BACKEND_RTT_RETRY_DELAY_MS` - This option defines the time interval between transmission retries.
* :option:`CONFIG_LOG_BACKEND_RTT_SYST_ENABLE` - This option is used to output logs in the system format.
* :option:`CONFIG_LOG_BACKEND_RTT_MESSAGE_SIZE` - This option defines the maximum message size.
* :option:`CONFIG_LOG_BACKEND_RTT_BUFFER` - This option selects the index of the buffer used for logger output.
* :option:`CONFIG_LOG_BACKEND_RTT_BUFFER_SIZE` - This option defines the size of the buffer used for logger output.
* :option:`CONFIG_LOG_BACKEND_RTT_FORCE_PRINTK` - This option enables processing of ``printk`` calls in the logger buffers instead of the RTT buffer.

For information about how to run SEGGER's J-Link RTT on your PC and see the logs, see :ref:`testing_rtt`.

Spinel
======

Using Spinel protocol as a logging backend is specific to OpenThread's :ref:`thread_architectures_designs_cp_ncp` architecture.
The Spinel protocol can be configured as a logging backend using the following Kconfig options:

* :option:`CONFIG_LOG_BACKEND_SPINEL` - This option enables the Spinel logging backend.
* :option:`CONFIG_LOG_BACKEND_SPINEL_BUFFER_SIZE` - This option defines the size of buffer used for logger output.

To communicate using the Spinel protocol and gather logs, you need one of the following tools:

* `PySpinel`_
* :ref:`ug_thread_tools_wpantund`

Each one of these tools accepts the ``-d <DEBUG_LEVEL>`` and ``--debug=<DEBUG_LEVEL>`` arguments, which can be used to display logging messages.
See `PySpinel arguments`_ for an example if you are using PySpinel.
Alternatively, see `wpantund Usage Overview`_ for information about how to change wpantund configuration file to avoid passing arguments manually every time.

Shell
=====

When you enable Zephyr's :ref:`zephyr:shell_api`, it by default becomes a logging backend.
You can disable this backend by using the following Kconfig option:

* :option:`CONFIG_SHELL_LOG_BACKEND` - This option enables and disables the shell logging backend.

.. note::
   The UART and RTT logging backends can also be configured as shell backends.
   For example, if the UART backend is disabled, but UART is selected as the shell backend and Zephyr's shell is enabled as the logging backend, the logging output will end up in UART.

Logging output examples
***********************

See the following examples of different sample logs available in |NCS|:

* Minimal Zephyr logs

  .. code-block:: console

     D: Debug message
     I: Info message
     W: Warning message
     E: Error message

* Full Zephyr logs

  .. code-block:: console

     [00013022] <dbg> sample_app: Debug message'
     [00013023] <inf> sample_app: Info message'
     [00013023] <wrn> sample_app: Warning message'
     [00013023] <err> sample_app: Error message'

* OpenThread logs

  .. code-block:: console

     -CORE----: Notifier: StateChanged (0x00000040) [Rloc-]
     -MLE-----: Send Parent Request to routers (ff02:0:0:0:0:0:0:2)
     -MAC-----: Sent IPv6 UDP msg, len:84, chksum:1e84, to:0xffff, sec:no, prio:net
