.. _customer_uicr_sample:

CUSTOMER data in UICR
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

The sample will read CUSTOMER from UICR. During programming customer.bin is programmed to the nRF,
which will be generated as part of uicr.hex by the gen_uicr image and flashed to the device when
`west flash` is called.

Building and running
*********************

.. |sample path| replace:: :file:`samples/ironside_se/customer_uicr`

.. include:: /includes/build_and_run.txt

Testing
*******

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset the development kit.

The application writes out all the content of UICR->CUSTOMER. The written ``customer.bin`` file contains
0xdeadbeef as the first 4 bytes, which can be verified from the logs. The ``customer.bin`` file was
generated using the command:

   .. code-block:: console

   python3 -c "open('customer.bin','wb').write((0xdeadbeef).to_bytes(4, 'little'))"


Configuration
*************

The sample uses the following key configurations:

Kconfig Configuration
  The ``sysbuild/uicr/prj.conf`` file overwrites uicr image configuration. It enables CUSTOMER hex generation via CONFIG_GEN_UICR_CUSTOMER and points to the binary file
  with CONFIG_GEN_UICR_CUSTOMER_BIN_FILE.

Sysbuild Configuration
  The ``sysbuild/uicr`` folder overlays content in the uicr image folder. By storing ``customer.bin`` to this folder, CONFIG_GEN_UICR_CUSTOMER_BIN_FILE can use relative
  path to point to ``customer.bin``.

Dependencies
************

This sample uses the following |NCS| subsystems:

* UICR generation - Generates UICR, which in this sample contains data stored in UICR.CUSTOMER
* Sysbuild - Enables building the UICR image

In addition, it uses the following Zephyr subsystems:

* :ref:`Kernel <kernel>` - Provides basic system functionality and threading
* :ref:`Console <console>` - Enables UART console output for debugging and user interaction
