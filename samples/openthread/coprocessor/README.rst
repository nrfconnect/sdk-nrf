.. _ot_coprocessor_sample:

Thread: Co-processor
####################

.. contents::
   :local:
   :depth: 2

The :ref:`Thread <ug_thread>` Co-processor sample demonstrates how to implement OpenThread's :ref:`thread_architectures_designs_cp` inside the Zephyr environment.
The sample uses the :ref:`thread_architectures_designs_cp_rcp` architecture.

The sample is based on Zephyr's :zephyr:code-sample:`coprocessor` sample.
However, it customizes Zephyr's sample to fulfill the |NCS| requirements (for example, by increasing the stack size dedicated for the user application), and also extends it with features such as:

* Increased Mbed TLS heap size.
* Lowered main stack size to increase user application space.
* No obsolete configuration options.
* Vendor hooks for co-processor architecture allowing users to extend handled properties by their own, customized functionalities.
* Thread 1.2 features.

This sample supports optional :ref:`logging extension <ot_coprocessor_sample_logging>`, which can be turned on or off independently.
See :ref:`ot_coprocessor_sample_activating_variants` for details.

Requirements
************

The sample supports the following development kits for testing the network status:

.. table-from-sample-yaml::

To test the sample, you need at least one development kit.
You can use additional development kits programmed with the Co-processor sample for testing network joining.

Moreover, the sample requires a Userspace higher layer process running on your device to communicate with the MCU co-processor part.
This sample uses ``ot-cli`` as reference.


Overview
********

The sample demonstrates using a co-processor target on the MCU to communicate with `ot-cli` on Unix-like operating system.
According to the co-processor architecture, the MCU part must cooperate with user higher layer process to establish the complete full stack application.
The sample shows how to set up the connection between the co-processor and the host.

By default, this sample comes with the :ref:`RCP set of OpenThread functionalities <thread_ug_feature_sets>` enabled (:kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_RCP`).

.. _ot_coprocessor_sample_logging:

Logging extension
=================

By default, this sample uses :ref:`Spinel logging backend <ug_logging_backends_spinel>` for sending log messages to the host device using the Spinel protocol.
This is a useful feature, because it does not require separate interfaces to communicate with the co-processor through the Spinel protocol and collect log messages.
Moreover, using the Spinel logging backend (by setting :kconfig:option:`CONFIG_LOG_BACKEND_SPINEL`) does not exclude using another backend like UART or RTT at the same time.

By default, the log levels for all modules are set to critical to not engage the microprocessor in unnecessary activities.
To make the solution flexible, you can change independently the log levels for your modules, for the whole Zephyr system, and for OpenThread.
Use the :file:`logging.conf` configuration file located in the :file:`snippets/logging/` directory as reference for this purpose.

User interface
**************

All the interactions with the application are handled using serial communication.

You can interact with the sample through ``ot-daemon`` or ``ot-cli`` with commands listed in `OpenThread CLI Reference`_.
See :ref:`ug_thread_tools_ot_apps` for more information.

You can also use your own application, provided that it supports the Spinel communication protocol.

.. note::
    |thread_hwfc_enabled|
    In addition, the Co-processor sample reconfigures the baud rate to 1000000 bit/s by default.

Diagnostic module
=================

The Co-processor sample enables a diagnostic module in a similar way as described in the :ref:`ot_cli_sample_diag_module` section of the :ref:`ot_cli_sample` sample documentation.
However, the Co-processor and CLI samples use different commands for the module, as described in the :ref:`ot_coprocessor_testing` section.

Rebooting to bootloader
=======================

The Co-processor sample enables rebooting to bootloader for the ``nrf52840dongle/nrf52840`` board target, similar to what is described in the :ref:`ot_cli_sample_bootloader` section of the :ref:`ot_cli_sample` sample documentation.
However, the Co-processor and CLI samples use different commands, as described in the :ref:`ot_coprocessor_testing` section.
Additionally, the :ref:`ug_thread_tools_ot_apps` should be built with ``-DOT_PLATFORM_BOOTLOADER_MODE=ON`` option.

Configuration
*************

|config|

Check and configure the following library option that is used by the sample:

* :kconfig:option:`CONFIG_OPENTHREAD_COPROCESSOR_RCP` - Selects the RCP architecture for the sample.

.. _ot_coprocessor_sample_activating_variants:

Snippets
========

.. |snippet| replace:: :makevar:`coprocessor_SNIPPET`

.. include:: /includes/sample_snippets.txt

The following snippets are available:

* ``debug`` - Enables debugging the Thread sample by enabling :c:func:`__ASSERT()` statements globally.
* ``logging`` - Enables logging using RTT.
  For additional options, refer to :ref:`RTT logging <ug_logging_backends_rtt>`.
* ``usb`` - Enables emulating a serial port over USB for Spinel communication with the host.
* ``hci`` - Enables support for the Bluetooth HCI interface parallel to :ref:`Thread RCP <thread_architectures_designs_cp_rcp>`.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/openthread/coprocessor`

|enable_thread_before_testing|

.. include:: /includes/build_and_run.txt

.. _ot_coprocessor_testing:

HCI support
===========

Currently, HCI is only supported using the nRF USB interface.
The device will show two virtual UART ports.
Usually the first port will be associated with the HCI interface, and the second one with the Thread co-processor.

Testing
=======

After building the sample and programming it to your development kit, complete the following steps to test it:

1. Connect the development kit's SEGGER J-Link USB port to the PC USB port with a USB cable.
   If you are using HCI, connect the kit's nRF USB port to the PC USB port instead.
#. Get the kit's serial port name (for example, :file:`/dev/ttyACM0`).
#. Run and configure ot-cli as described in :ref:`ug_thread_tools_ot_apps`.
#. From this point, you can follow the :ref:`ot_cli_sample_testing` instructions in the CLI sample by removing the `ot` prefix for each command.
   If you are using HCI, follow the instructions for the :zephyr:code-sample:`bluetooth_hci_uart` sample in the Zephyr documentation.
   You can follow these instead of or in addition to the CLI sample instructions.

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:thread_protocol_interface`

* :ref:`zephyr:logging_api`:

  * ``include/logging/log.h``

* :ref:`zephyr:bluetooth-hci`
