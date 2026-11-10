.. _mcuboot_serial_recovery_sample:

.. ncs-sample::
   :title: MCUboot serial recovery

   The MCUboot serial recovery sample demonstrates recovery through MCUboot's raw Simple Management Protocol (SMP) server over UART.
   It supports upload to a selected image slot.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You need one of the following SMP clients:

* The `nRF Util development tool`_ with the ``mcu-manager`` command.
  To install the command, run ``nrfutil install mcu-manager``.
* The ``mcumgr`` command-line tool.

Connect the development kit to the computer with its USB cable.
The nRF54L15 DK exposes the console UART through its onboard debug probe, so no additional wiring is required.

Overview
********

The sysbuild configuration adds MCUboot and enables the following features in its image:

* UART serial recovery using raw SMP framing.
* Direct upload to a selected image slot.
* Image state reporting.
* Progressive slot erase.

The application included after MCUboot is the Zephyr SMP server sample.
It provides visible output and a shell that the automated recovery tests use to verify that the recovered application boots.

The default recovery trigger is the MCUboot recovery button.
The Twister scenarios also cover entry when no bootable application exists, entry after a pin reset, leaving recovery after a timeout, and entry requested by the application.

Building and running
********************

Build the sample for the nRF54L15 DK:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp samples/dfu/mcuboot_serial_recovery

Flash the complete sysbuild image:

.. code-block:: console

   west flash

The initial boot output identifies that the CPU is running the application:

.. code-block:: console

   <inf> execution_context: Now running: application
   smp_sample: build time: <BUILD TIME>

Entering recovery mode
**********************

Hold the MCUboot recovery button while resetting the development kit, then release the button.
MCUboot remains in serial recovery until it receives a reset command.
While recovery is active, MCUboot uses the UART as a raw SMP transport, so it cannot print text logs on the same UART without corrupting SMP frames.
When the automated tests detect recovery, either from the configured reset trigger or from a successful SMP command, they log the following message:

.. code-block:: console

   Now running: MCUboot serial recovery

The test log and the application runtime log show which image the CPU is running without mixing text with raw SMP traffic.

Disconnect any serial terminal before using an SMP client because the terminal and SMP client use the same UART.

When using nRF Util, pass the serial port of the development kit to each command with the ``--serial-port`` option.

When using ``mcumgr``, configure a connection for the development kit's serial port:

.. tabs::

   .. group-tab:: Linux

      .. code-block:: console

         mcumgr conn add recovery type="serial" connstring="dev=/dev/ttyACM0,baud=115200"

   .. group-tab:: Windows

      .. code-block:: console

         mcumgr conn add recovery type="serial" connstring="COM1,baud=115200"

Replace the example port with the port assigned to your development kit.

Requesting recovery from the application
****************************************

The application can request serial recovery for the next boot instead of relying on the recovery button.
MCUboot consumes the request when it enters serial recovery, so the following reset boots the application again.
The sample provides the following build variants for this purpose:

* ``boot_mode`` - The application sets the retained boot mode (:kconfig:option:`CONFIG_RETENTION_BOOT_MODE`), and MCUboot checks it with the :kconfig:option:`CONFIG_BOOT_SERIAL_BOOT_MODE` Kconfig option.
* ``boot_req`` - The application uses the bootloader request subsystem (:kconfig:option:`CONFIG_NRF_MCUBOOT_BOOT_REQUEST`), and MCUboot checks it with the :kconfig:option:`CONFIG_NRF_BOOT_SERIAL_BOOT_REQ` Kconfig option.

Build the selected variant:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp samples/dfu/mcuboot_serial_recovery -- -DFILE_SUFFIX=boot_mode
   west build -b nrf54l15dk/nrf54l15/cpuapp samples/dfu/mcuboot_serial_recovery -- -DFILE_SUFFIX=boot_req

After the application boots, run the matching shell command to request recovery and reboot:

.. code-block:: console

   uart:~$ mcuboot serial_recovery
   uart:~$ boot_request serial_recovery

The first command is available in the ``boot_mode`` variant, and the second in the ``boot_req`` variant.

Uploading directly to a selected slot
*************************************

Build or locate a signed application binary.
Upload it directly to the secondary slot of image pair 0 by selecting image number 2:

.. tabs::

   .. group-tab:: nRF Util

      .. nrfutil-command:: console
         :values: serial_port=/dev/ttyACM0

         nrfutil mcu-manager serial image-upload --serial-port <serial_port> --image-number 2 --firmware build/mcuboot_serial_recovery/zephyr/zephyr.signed.bin
         nrfutil mcu-manager serial image-list --serial-port <serial_port>

   .. group-tab:: MCUmgr

      .. code-block:: console

         mcumgr -c recovery image upload -e -n 2 build/mcuboot_serial_recovery/zephyr/zephyr.signed.bin
         mcumgr -c recovery image list

The image list reports image number 0, slot 1 after the upload.
Image numbers 0 and 1 select the primary slot, and image number 2 selects the secondary slot.

Resetting and recovering
************************

Reset the development kit after uploading an image:

.. tabs::

   .. group-tab:: nRF Util

      .. nrfutil-command:: console
         :values: serial_port=/dev/ttyACM0

         nrfutil mcu-manager serial reset --serial-port <serial_port>

   .. group-tab:: MCUmgr

      .. code-block:: console

         mcumgr -c recovery reset

Reconnect the terminal and verify that the application prints its build time.
If an upload is interrupted, reset the development kit while holding the recovery button, and then repeat the upload.
If the primary image is damaged, MCUboot enters recovery automatically in the ``sample.dfu.mcuboot_serial_recovery.no_application`` configuration.

Limitations
***********

* This sample supports UART on the nRF54L15 DK only.
  The sample does not support USB CDC ACM serial recovery.
* The sample has one updateable image pair.
  Multicore and network-core recovery are not supported.
* Image number 2 is the only non-default direct-upload target provided by this partition layout.

Testing
*******

The :file:`tests.yaml` file defines stable build and hardware scenarios for the nRF54L15 DK.
The hardware tests verify recovery of a damaged application, pin-reset entry, recovery timeout, and direct upload to the secondary slot.
The ``boot_mode`` and ``boot_req`` scenarios verify that a recovery request from the application is consumed after one reset, and that the application can be updated after entering recovery through the request.

Dependencies
************

This sample uses the following components:

* :ref:`MCUboot <mcuboot_index_ncs>`
* :ref:`mcuboot_serial_recovery`
