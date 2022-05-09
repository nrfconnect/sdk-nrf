.. _modem_fault:

nRF9160: Modem fault
####################

.. contents::
   :local:
   :depth: 2

The Modem fault sample demonstrates how two threads can synchronize with modem re-initialization upon a fault.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160_ns

.. include:: /includes/spm.txt

Overview
********

The sample consists of a main thread and a networking thread.
The main thread initializes the modem library and register the device to the network, after which it sleeps waiting for modem library re-initialization upon a modem fault.
The networking thread waits for the main thread to have registered the device to the network, before connecting to an HTTP server to download a test file.
Right before commencing the download, the networking thread schedules a workqueue task that will abruptly turn off the modem after 2 seconds while the download is in progress, triggering a modem fault.
The Modem library default fault handler in the Modem library integration layer will re-initialize the modem library using the system workqueue. See :ref:`nrf_modem_lib_readme` for more information about the different options for fault handling.

The application will be notified of the modem library re-initialization by its modem library initialization hook.
In the hook function, the semaphore is given to the main thread to register the device to the network once more, and in turn, give the semaphore to the networking thread to begin the download once again.
This time, the networking thread won't schedule the task to turn off the modem, allowing the download to complete.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/modem_fault`

.. include:: /includes/build_and_run_nrf9160.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the sample starts, connects to the LTE network and begins to download.
#. Observe the modem crashing and the ``recv()`` call exit with errno 110 (ESHUTDOWN), or errno 9 (EBADF).
#. Observe the fault handler re-initializing the modem library automatically.
#. Observe the main and networking threads synchronize to restart the download.

Sample output
=============

The sample shows the following output:

.. code-block:: console

	<inf> app: main: modem fault sample started
	<inf> app: main: initializing modem library
	<inf> app: net: waiting for network registration
	<inf> app: on_init: modem library initialized
	<inf> app: main: registering to network..
	<inf> app: main: registered
	<inf> app: net: connecting to speedtest.ftp.otenet.gr
	<inf> app: net: sent 85 bytes
	<inf> app: net: crashing in two seconds
	<inf> app: net: downloading..
	<inf> app: net: downloaded 2124 bytes
	<inf> app: net: downloaded 4248 bytes
	<inf> app: net: downloaded 6372 bytes
	<inf> app: net: downloaded 8496 bytes
	<inf> app: net: downloaded 10620 bytes
	<inf> app: net: downloaded 12744 bytes
	<inf> app: net: downloaded 14872 bytes
	<inf> app: net: downloaded 16920 bytes
	<err> nrf_modem: Modem error: 0x10, PC: 0x78162
	<wrn> app: net: recv() failed, err 110
	<inf> app: net: waiting for network registration
	<inf> app: on_init: modem library initialized
	<inf> app: main: registering to network..
	<inf> app: main: registered
	<inf> app: net: connecting to speedtest.ftp.otenet.gr
	<inf> app: net: sent 85 bytes
	<inf> app: net: downloading..

Observe the download complete.
Please note that based on whether the crash happened during a ``recv()`` call or in-between ``recv()`` calls will result in a different errno.
If the modem crashes while a ``recv()`` is pending, ``recv()`` will return -1 and set errno to ``ESHUTDOWN`` (110).
If the modem crashes in-between ``recv()`` calls, the ``recv()`` call will return -1 and set errno to ``EBADF`` (9).

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
