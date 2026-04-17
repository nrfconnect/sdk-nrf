Bluetooth Tester application
############################

.. contents::
   :local:
   :depth: 2

The Tester application uses binary protocol to control Zephyr stack and is aimed at automated testing.
It requires two serial ports to operate.
The first serial is used by Bluetooth Testing Protocol (BTP) to drive Bluetooth stack.
BTP commands and events are received and buffered for further processing over the same serial.

BTP specification can be found in auto-pts project repository: https://github.com/auto-pts/auto-pts
The auto-pts is an automation framework for PTS Bluetooth testing tool provided by Bluetooth SIG.

See https://docs.zephyrproject.org/latest/guides/bluetooth/index.html for full documentation about how to use this test.


Supported Profiles and Services
*******************************

Host/Core
=========

* GAP
* GATT
* IAS
* L2CAP
* OTS
* SM

LE Audio
========

* AICS
* ASCS
* BAP
* CAP
* CAS
* CCP
* CSIP
* CSIS
* HAP
* HAS
* MCP
* MCS
* MCIP
* MICS
* PACS
* PBP
* TBS
* TMAP
* VCP
* VCS
* VOCS

Mesh
====

* Mesh Node
* Mesh Model

Building and running on QEMU
****************************

QEMU should have connection with the external host Bluetooth hardware.
You can use the ``btproxy`` tool from BlueZ to give access to a Bluetooth controller attached to the Linux host OS:

.. code-block::

   $ sudo tools/btproxy -u
   Listening on /tmp/bt-server-bredr

The ``/tmp/bt-server-bredr`` option is already set in Makefile through ``QEMU_EXTRA_FLAGS``.

To build tester application for QEMU use ``BOARD=qemu_cortex_m3`` and ``CONF_FILE=qemu.conf``.
After this, you can start qemu through the "run" build target.

.. note::
   The target board must support enough UARTs for BTP and controller.
   It is recommended to use ``qemu_cortex_m3``.

You can use the ``bt-stack-tester`` UNIX socket (previously set in Makefile) for now to control the tester application.

Next, build and flash tester application using ``west build`` and ``west flash``.

Use a serial client, for example ``PuTTY`` to communicate over the serial port (typically ``/dev/ttyUSBx``) with the tester using BTP.

Building and running on ``native_sim`` on Linux
***********************************************

You can build and run the tester application using the ``native_sim`` board, and any compatible HCI controller.
This has the advantage of allowing the use of Linux debugging tools like ``valgrind`` and ``gdb``, as well as tools like ``btmon``.
It is also faster to apply changes to the code, as building for ``native_sim`` is usually faster, and there is no flashing step involved.

Building for ``native_sim``:

.. code-block::

    west build -b native_sim

This generates a :file:`zephyr.exe` file that can be executed as follows:

.. code-block::

    zephyr.exe --bt-dev=hciX

Where ``hciX`` is the HCI controller (like ``hci0``).
However, for the purpose of running the tester application with auto-pts, running the application is left to the auto-pts client.


Building the Zephyr controller for a ``native_sim`` host on Linux
=================================================================

To build and flash the Zephyr controller as an HCI controller usable by Linux,
either the `bluetooth_hci_uart <https://docs.zephyrproject.org/latest/samples/bluetooth/hci_uart/README.html>`_
or `bluetooth_hci_usb <https://docs.zephyrproject.org/latest/samples/bluetooth/hci_usb/README.html>`__
samples can be used.
See also `bluetooth-tools <https://docs.zephyrproject.org/latest/connectivity/bluetooth/bluetooth-tools.html>`_.
When building these samples, the tester application controller overlay should be supplied.
For example:

.. code-block::

    west build -b nrf5340_audio_dk/nrf5340/cpunet -d ${ZEPHYR_BASE}/build/nrf5340_audio_dk_nrf5340_cpunet ${ZEPHYR_BASE}/samples/bluetooth/hci_ipc/ -- -DEXTRA_CONF_FILE=${ZEPHYR_BASE}/tests/bluetooth/tester/overlay-bt_ll_sw_split.conf
    west flash -d ${ZEPHYR_BASE}/build/nrf5340_audio_dk_nrf5340_cpunet
    west build -b nrf5340_audio_dk/nrf5340/cpuapp -d ${ZEPHYR_BASE}/build/nrf5340_audio_dk_nrf5340_cpuapp ${ZEPHYR_BASE}/samples/bluetooth/hci_uart/
    west flash -d ${ZEPHYR_BASE}/build/nrf5340_audio_dk_nrf5340_cpuapp

This builds and prepares an nRF5340 Audio DK to be an HCI controller over UART.

For single core boards like the nRF52840 DK, you can do it as follows:

.. code-block::

    west build -b nrf52840dk/nrf52840 ${ZEPHYR_BASE}/samples/bluetooth/hci_uart/ -- -DEXTRA_CONF_FILE=${ZEPHYR_BASE}/tests/bluetooth/tester/overlay-bt_ll_sw_split.conf
    west flash

You can also use the `bluetooth_hci_usb <https://docs.zephyrproject.org/latest/samples/bluetooth/hci_usb/README.html>`_ sample, but support for Bluetooth Isochronous channels is not yet fully supported.

Building for LE Audio
*********************

You can build the tester application with support for BT LE Audio by applying the :file:`overlay-le-audio.conf` and :file`hci_ipc.conf` files with ``--sysbuild`` for the supported boards,
for example:

.. code-block::

   west build -b nrf5340dk/nrf5340/cpuapp --sysbuild -- -DEXTRA_CONF_FILE=overlay-le-audio.conf;hci_ipc.conf

Building with support for BTSnoop and RTT logs
**********************************************

Add the following Kconfig options in the desired configuration file:

.. code-block::

   CONFIG_LOG=n
   CONFIG_LOG_BACKEND_RTT=y
   CONFIG_LOG_BACKEND_RTT_BUFFER=1
   CONFIG_LOG_BACKEND_RTT_MODE_DROP=n

   CONFIG_USE_SEGGER_RTT=y
   CONFIG_SEGGER_RTT_SECTION_CUSTOM=y

   CONFIG_BT_DEBUG_MONITOR_RTT=y
   CONFIG_BT_DEBUG_MONITOR_RTT_BUFFER=2
