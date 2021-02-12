.. _fmfu_smp_svr_sample:

nRF9160: Full modem firmware update using SMP Server
####################################################

.. contents::
   :local:
   :depth: 2

This sample application implements a Simple Management Protocol (SMP) Server, using the SMP transfer encoding with the MCU manager (mcumgr) management protocol, to provide an interface over UART which enables the device to do full modem firmware updates.

For more information about mcumgr and SMP, see :ref:`device_mgmt`.

Requirements
************

This sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

.. include:: /includes/spm.txt

Overview
********

The sample does the following:

1. It deinitializes the :ref:`nrfxlib:nrf_modem`.
#. It registers to mcumgr a ``stat`` command.
#. It then registers the commands to upload the firmware and to get the hash.
#. It finally enters an idle loop, waiting for any communication over the serial line.

The sample also provides a UART overlay that will allow your sample to transfer at a speed of 1M baud, and it enables support for the ``fmfu_mgmt`` command group.
See :ref:`lib_fmfu_mgmt` for more details.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/fmfu_smp_svr`

.. include:: /includes/build_and_run_nrf9160.txt

To use the UART overlay for increasing the transfer speed, add the ``-DOVERLAY_FILE=uart.overlay`` flag to your build.

Testing
=======

After programming the sample to your development kit, perform the following steps to test it:

1. Connect the USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator, observe that the sample starts, and then close the terminal emulator.
#. Call the provided :file:`update_modem.py` script specifying the COM port, the firmware ZIP file, and the UART baud rate shown in the following examples.

   * If you used the default baud rate:

     .. parsed-literal::
        :class: highlight

        python update_modem.py mfw_nrf9160_1.2.2.zip /dev/ttyACM0 *115200*

   * If you used the ``-DOVERLAY_FILE=uart.overlay`` flag:

     .. parsed-literal::
        :class: highlight

        python update_modem.py mfw_nrf9160_1.2.2.zip /dev/ttyACM0 *1000000*



Sample output
-------------

The python script should print the following output:

.. code-block:: console

   # nrf9160 modem firmware upgrade over serial port example started.
   {
   "duration": 406,
   "error_code": "Ok",
   "operation": "open_uart",
   "outcome": "success",
   "progress_percentage": 100
   }
   Programming modem bootloader.
   ...
   Finished with file.
   Verifying memory range 1 of 3
   Verifying memory range 2 of 3
   Verifying memory range 3 of 3
   Verification success.
   {
   "duration": 5,
   "error_code": "Ok",
   "operation": "close_uart",
   "outcome": "success",
   "progress_percentage": 100
   }
   ------------------------------------------------------------

Troubleshooting
===============

You can use the mcumgr CLI tool to test if the sample is running correctly, as follows:

.. parsed-literal::
  :class: highlight

   mcumgr --conntype serial --connstring="dev=*COM Port*,baud=*Baudrate*" stat smp_com
   stat group: smp_com
       512 frame_max
       504 pack_max

Dependencies
************

This sample uses the following nRF Connect SDK libraries:

* :ref:`lib_fmfu_mgmt`
* :ref:`modem_info_readme`

This sample uses the following nrfxlib libraries:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following nRF Connect SDK sample:

* :ref:`secure_partition_manager`
