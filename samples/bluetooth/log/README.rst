.. _bluetooth_log:

.. ncs-sample::
   :title: Bluetooth: SoftDevice Controller and MPSL logging

   The SoftDevice Controller and MPSL logging sample demonstrates how to enable log messages from the :ref:`nrfxlib:softdevice_controller` (SDC) and the :ref:`nrfxlib:mpsl` (MPSL).
   These log messages can help you debug HCI errors and other controller failures.

   .. note::
      The SoftDevice Controller and MPSL logging feature is :ref:`experimental <software_maturity>`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The SDC and MPSL libraries use tokenized logging.
Instead of full text strings, the libraries send short message IDs with optional numeric arguments.
A log handler shared by both libraries converts the message IDs back into readable messages and prints them using the Zephyr :ref:`zephyr:logging_api` API.
This keeps the library size small and the logging overhead low.

The sample contains no application code.
All the setup is done in the :ncs-file:`prj.conf` file.

Configuration
*************

|config|

The sample uses the following Kconfig options:

* :kconfig:option:`CONFIG_MPSL_LOG` - Enables MPSL log messages.
* :kconfig:option:`CONFIG_BT_CTLR_SDC_LOG` - Enables SoftDevice Controller log messages.
* :kconfig:option:`CONFIG_MPSL_LOG_HANDLER` - Enables the shared log handler.
  Both options above select it automatically.

For more options, such as the print level or how to use a custom log handler, see the :ncs-file:`/subsys/mpsl/log/Kconfig` file.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/log`

.. include:: /includes/build_and_run.txt

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Reset the kit.
#. Observe the SDC and MPSL log messages in the terminal.

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:softdevice_controller`
* :ref:`nrfxlib:mpsl`
