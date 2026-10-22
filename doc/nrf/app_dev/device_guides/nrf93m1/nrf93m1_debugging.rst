.. _ug_nrf93m1_debugging:

Debugging nRF93M1
#################

.. contents::
   :local:
   :depth: 2

This page describes how to collect the information that you need to debug the nRF93M1 module, or to give to Nordic technical support: modem traces and modem coredumps.
For metrics that nRF Cloud collects continuously, see :ref:`ug_93m1_cloud_connecting_observability`.

.. _ug_nrf93m1_debugging_trace:

Modem trace
***********

The module produces a real-time binary modem trace that shows the AT commands, IP packets, and the LTE RRC and NAS-EPS signaling exchanged with the network.
Capture a trace when you debug registration, connectivity, or power saving problems, and when Nordic technical support asks for one.

The trace is not buffered or stored in the module, so you must capture it while the problem occurs.
You can send the trace to UART0, to the USB interface of the nRF93M1, or to both.
The default is both.
On the nRF93M1 DK, use the USB interface of the nRF93M1 (``vcom1``), which is available on the connector labeled **USB2**.

Requirements
============

* The `nRF Util`_ ``trace`` command.
  Install it with the following command:

  .. code-block:: console

     nrfutil install trace

* The `Wireshark`_ application, if you want to inspect the trace live or open it afterwards.
* The module trace output enabled.
  Trace output is on by default.
  The power saving configuration in :ref:`ug_nrf93m1_power_optimization` disables the trace output with ``AT%MODULECFG="logCtrl",0`` and the USB interface with ``AT%MODULECFG="usbCtrl",2``.
  If you used it, enable both again over UART1, and reset the module before you capture:

  .. code-block:: console

     AT%MODULECFG="logCtrl",2
     OK
     AT%MODULECFG="usbCtrl",0
     OK

Capturing a trace
=================

1. Connect USB2 on the DK to your computer and find the trace port:

   .. code-block:: console

      nrfutil device list

   The nRF93M1 is listed with three virtual COM ports.
   Note the port with ``vcom: 1``.
   In the following example, the trace port is :file:`/dev/ttyACM3`:

   .. code-block:: console

      2623CKLQ0001610
      Product         nRF93M1
      Ports           /dev/ttyACM2, vcom: 0
                      /dev/ttyACM3, vcom: 1
                      /dev/ttyACM4, vcom: 2
      Traits          nordicUsb, serialPorts, usb

#. Start the capture, and save the raw trace data and a PcapNG file:

   .. code-block:: console

      nrfutil trace lte --input-serialport /dev/ttyACM3 --output-raw trace.bin --output-pcapng trace.pcapng

   You can give more than one output.
   Replace the :file:`/dev/ttyACM3` port with the port from the previous step, or with ``COM`` followed by the port number on Windows.

#. Reproduce the problem.
   For example, send the AT commands over the AT port while the capture runs:

   .. code-block:: console

      AT+CFUN=0
      OK
      AT+CFUN=1
      OK

#. Press :kbd:`Ctrl+C` to stop the capture.
   Stop it this way so that the output files are finalized.

Viewing a trace
===============

Open the :file:`trace.pcapng` file in `Wireshark`_.
To see the packets live while you capture, run the command with ``--output-wireshark`` instead of ``--output-pcapng``.
To make Wireshark detect the ``nrfutil trace lte`` command, run ``nrfutil trace install-trace-plugin``.

You can also convert a raw trace that you captured earlier:

.. code-block:: console

   nrfutil trace lte --input-file trace.bin --output-pcapng trace.pcapng

By default, the PcapNG output includes the AT command, IP, LTE RRC, and NAS-EPS packets.
To change this, use the ``--modem-trace-filter`` option with a comma-separated list of packet types, or use ``all`` to include every type.
For a quick look without Wireshark, print the trace as text with ``--x-output-modem-txt-stdout``.
This option is experimental.

.. note::
   The tool decodes the trace using a database that matches the modem firmware version.
   If it cannot select the database, give the version explicitly with ``--mfw-revision-id``, for example ``--mfw-revision-id mfw_nrf93m1_1.5.0``.
   Read the version with ``AT+CGMR``.

The ``%MODULECFG`` command configures the trace:

.. list-table::
   :header-rows: 1

   * - Parameter
     - Values
   * - ``"logPortSel"``
     - Output interface: ``0`` for USB only, ``1`` for UART only, ``2`` for dynamic USB and UART (default).
   * - ``"logCtrl"``
     - Logging: ``0`` for disabled, ``1`` for software logging, ``2`` for all logging (default).
   * - ``"logLevel"``
     - Log print level, from ``0`` for debug (default) to ``5`` for error.

All three settings take effect after a reset.
UART0 requires a baud rate of at least 921,600 bps and supports up to 6 Mbps, so make sure that your capture hardware keeps up with the data rate.

.. _ug_nrf93m1_debugging_coredump:

Modem coredumps
***************

If the modem firmware crashes or hits an assertion, the module collects information about the state at the time of the crash in a coredump.
Coredumps are always collected, but they are never exported or uploaded unless you enable it.
You then decide when to send a coredump, for example when Nordic technical support asks for it.

A coredump has the following properties:

* It is stored in the OTA partition of the module and stays there until you export, upload, or delete it, or until an OTA update overwrites it.
* A new crash does not overwrite an existing coredump.
* It is encrypted, so you cannot read it yourself.
  Nordic can decode it with the modem firmware symbols that match your firmware version.

When the module starts and finds a valid coredump, it prints the following unsolicited result code:

.. code-block:: console

   %NRFCLOUDOBS: coredump available (43 KB)

Enabling coredump reporting
===========================

By default, ``AT%NRFCLOUDOBSUPLOAD`` and ``AT%NRFCLOUDOBSEXPORT`` do not include coredumps.
To include them, enable coredump reporting:

.. code-block:: console

   AT%NRFCLOUDOBSCOREDUMP=1
   OK

The setting is stored in the modem configuration, so it persists.
Read the current setting with ``AT%NRFCLOUDOBSCOREDUMP?``.

Uploading a coredump to nRF Cloud
=================================

Use this method when the device is registered to the network and provisioned to nRF Cloud.
See :ref:`ug_93m1_cloud_connecting_provisioning`.

1. Enable network registration status reports and go online:

   .. code-block:: console

      AT+CEREG=1
      OK
      AT+CFUN=1
      OK
      +CEREG: 2
      +CEREG: 5

   Status ``1`` or ``5`` means that the device is registered, and you can continue.

#. Enable coredump reporting, and add a heartbeat so that the report contains additional information about the device:

   .. code-block:: console

      AT%NRFCLOUDOBSCOREDUMP=1
      OK
      AT%NRFCLOUDOBSHEARTBEAT
      OK

#. Upload the data to nRF Cloud:

   .. code-block:: console

      AT%NRFCLOUDOBSUPLOAD
      %COAP: RESPONSE,2.01
      %COAP: DATA,8,Accepted
      ...
      %NRFCLOUDOBSUPLOAD: UPLOADED,18
      OK

   The coredump is removed from the module automatically after it is uploaded.

nRF Cloud stores the coredump as a custom data recording (CDR) of the device, with the reason ``mfw_coredump``.
To find it, open the page of the device in nRF Cloud and check the :guilabel:`Timeline`.
Download the file and provide it, together with a description of the situation in which the crash happened, to Nordic technical support.

Exporting a coredump over the serial port
=========================================

Use this method when the device cannot connect to the cloud.
Set up capturing of the serial terminal output before you start, because each exported line is very long.

.. code-block:: console

   AT%NRFCLOUDOBSCOREDUMP=1
   OK
   AT%NRFCLOUDOBSHEARTBEAT
   OK
   AT%NRFCLOUDOBSEXPORT
   MC:wOVH2q6i7t5m3t(...)ztQ==:
   MC:wOVH2q6i7t5m3t(...)ztQ==:
   OK

The command prints the data as base64-encoded chunk lines that start with ``MC:``.
The coredump is removed from the module automatically after it is exported.
Save the full output to a text file and provide it to Nordic technical support, together with a description of the situation in which the crash happened.

Capturing a RAM dump with nRF Util
==================================

When the modem crashes, the module can also send a full dump of its memory (a RAM dump) to nRF Util, in addition to storing the coredump.

The ``faultAction`` parameter of the ``%MODULECFG`` command selects what the module does when the modem firmware crashes:

.. list-table::
   :header-rows: 1

   * - Value
     - Action
   * - ``0``
     - Dump the exception information to flash and to the tool, then halt.
   * - ``1``
     - Print the exception information, then reset.
   * - ``2``
     - Dump the exception information to flash, then reset.
   * - ``3``
     - Dump the exception information to flash and to the tool, then reset.
   * - ``4``
     - Collect the observability chunks, then reset. This is the default. Export or upload the chunks as described in the previous sections.
   * - ``10``
     - Dump the exception information to flash, the tool, and UART, then halt.
   * - ``13``
     - Dump the exception information to flash, the tool, and UART, then reset.

To capture a RAM dump, complete the following steps:

1. Select an action that dumps to the tool and resets the module afterwards.
   The setting is stored in the module configuration:

   .. code-block:: console

      AT%MODULECFG="faultAction",3
      OK

#. Start a trace capture on the trace port, as described in :ref:`ug_nrf93m1_debugging_trace`, and give a directory for the dump with the ``--x-output-ramdump-dir`` option:

   .. code-block:: console

      nrfutil trace lte --input-serialport /dev/ttyACM3 --output-raw trace.bin --x-output-ramdump-dir ramdumps

#. Reproduce the crash.
   When the modem crashes, the trace shows that the module enters the dump handshake mode, and nRF Util writes the RAM dump to the directory as a :file:`ramdump_<date>_<time>.bin` file of a few megabytes.

#. Provide the RAM dump to Nordic technical support, together with a description of the situation in which the crash happened.

The module resets after the dump, and the USB ports are enumerated again, so the capture session ends.
Start a new capture if you want to continue tracing.
Set ``faultAction`` back to ``4`` when you have collected the dump, so that the module returns to the default behavior.

Deleting a coredump
===================

To delete a stored coredump without exporting it, erase the OTA partition:

.. code-block:: console

   AT%FWUPD=0
   OK

Updating the modem firmware over the air also overwrites a stored coredump.
