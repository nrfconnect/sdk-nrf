.. _customer_uicr_sample:

CUSTOMER register data in UICR
##########################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to store persistent data in UICR.CUSTOMER.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample reads the CUSTOMER register from the User Information Configuration Registers (UICRs).
During programming, the ``gen_uicr`` image generates :file:`uicr.hex`, which includes :file:`customer.bin`.
When you run ``west flash``, :file:`uicr.hex` is programmed to the device.

Building and running
*********************

.. |sample path| replace:: :file:`samples/ironside_se/customer_uicr`

.. include:: /includes/build_and_run.txt

Testing
*******

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset the development kit.

The application writes out all the content of UICR.CUSTOMER.
The written :file:`customer.bin` file contains ``0xdeadbeef`` as the first 4 bytes, which can be verified from the logs.
To generate :file:`customer.bin`, use this command:

   .. code-block:: console

   python3 -c "open('customer.bin','wb').write((0xdeadbeef).to_bytes(4, 'little'))"


Configuration
*************

The sample uses the following key configurations:

Kconfig Configuration
  The :file:`sysbuild/uicr/prj.conf` file overrides the UICR image configuration.
  It enables CUSTOMER HEX generation with :kconfig:option:`CONFIG_GEN_UICR_CUSTOMER` and specifies the binary file with :kconfig:option:`CONFIG_GEN_UICR_CUSTOMER_BIN_FILE`.

Sysbuild Configuration
  The :file:`sysbuild/uicr` folder overlays the content of the UICR image folder.
  By storing :file:`customer.bin` in this folder, :kconfig:option:`CONFIG_GEN_UICR_CUSTOMER_BIN_FILE` can use a relative path to point to :file:`customer.bin`.

Dependencies
************

This sample uses the following |NCS| subsystems:

* UICR generation - Generates UICR, which in this sample contains data stored in UICR.CUSTOMER
* Sysbuild - Enables building the UICR image

In addition, it uses the following Zephyr subsystems:

* :ref:`Kernel <kernel>` - Provides basic system functionality and threading
* :ref:`Console <console>` - Enables UART console output for debugging and user interaction
