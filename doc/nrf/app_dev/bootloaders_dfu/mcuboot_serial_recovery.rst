.. _mcuboot_serial_recovery:

MCUboot serial recovery
#######################

.. contents::
   :local:
   :depth: 2

MCUboot implements a Simple Management Protocol (SMP) server.
SMP is a basic transfer encoding mechanism for use with the MCUmgr management protocol.
See Zephyr's :ref:`Device Management <zephyr:device_mgmt>` documentation for more information about MCUmgr and SMP.

MCUboot supports the following subset of the MCUmgr commands:

* Echo (OS group)
* Reset (OS group)
* Image list (IMG group)
* Image upload (IMG group)

If the ``CONFIG_ENABLE_MGMT_PERUSER`` Kconfig option is enabled, it can also support system-specific MCUmgr commands depending on the given MCUboot port.
The serial recovery feature can use any serial interface provided by the system.

Uploading image
***************

By default, uploading an image is targeted to the primary slot.
You can load an image to other slots only if you have enabled the ``CONFIG_MCUBOOT_SERIAL_DIRECT_IMAGE_UPLOAD`` Kconfig option.
To use progressive slot erasing during image upload, enable the ``CONFIG_BOOT_ERASE_PROGRESSIVELY`` Kconfig option.
As a result, a device can receive images smoothly and can erase the required part of a flash automatically.

Implementing serial recovery
****************************

See the following subsections for information on how to configure and use the serial recovery feature in MCUboot, enabling firmware updates through serial interfaces.

Selecting interface
===================

A serial recovery protocol is available over a hardware serial port or a USB CDC ACM virtual serial port.
You can enable the :zephyr:code-sample:`smp-svr` implementation by setting the ``CONFIG_MCUBOOT_SERIAL`` Kconfig option.
To set a type of an interface, use the ``BOOT_SERIAL_DEVICE_CHOICE`` Kconfig, and select either the ``CONFIG_BOOT_SERIAL_UART`` or the ``CONFIG_BOOT_SERIAL_CDC_ACM`` value.
Specify the appropriate interface for the serial recovery protocol using the devicetree-chosen node:

* ``Zephyr,console`` - If you are using a hardware serial port.
* ``Zephyr,cdc-acm-uart`` - If you are using a virtual serial port.

Entering the serial recovery mode
=================================

To enter the serial recovery mode, the device must first initiate rebooting, and a triggering event must occur (for example, pressing a button).

By default, the serial recovery GPIO pin active state enters the serial recovery mode.
Use the ``mcuboot_button0`` devicetree button alias to assign the GPIO pin to the MCUboot.

Alternatively, MCUboot can be configured to wait for a limited time at startup to check if DFU is invoked through an MCUmgr command.
To enable this mode, set the ``CONFIG_BOOT_SERIAL_WAIT_FOR_DFU`` Kconfig option.
The duration of this wait period in milliseconds is defined by the ``CONFIG_BOOT_SERIAL_WAIT_FOR_DFU_TIMEOUT`` Kconfig option.

Additionally, you can enable the following options:

* Entering recovery mode (``CONFIG_BOOT_SERIAL_BOOT_MODE``) - This option uses Zephyr's boot mode retention system to enter the serial recovery mode.
  An application must set the :ref:`boot mode<zephyr:retention_api>` to ensure MCUboot stays in the serial recovery mode after reboot.
  To activate this feature, use the boot mode API call ``bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);``.
* Entering recovery mode by resetting the serial pin (``CONFIG_BOOT_SERIAL_PIN_RESET``) - This option verifies if the SoC reset was triggered by a pin reset.
  If confirmed, the device automatically enters the recovery mode.

For details on other available configuration options for the serial recovery protocol, refer to the Kconfig options in the :file:`menuconfig` file.

Using serial recovery mode
**************************

Complete the following steps to use the serial recovery mode in MCUboot, enabling communication with the device's SMP server for firmware management and updates.

Installing MCUmgr CLI
=====================

MCUmgr command-line tool can be used as an SMP client for evaluation purposes.
To start using it, complete the :ref:`installation instructions <dfu_tools_mcumgr_cli>`.

Configuring connection
======================

Run the following command to configure the connection:

.. tabs::

  .. group-tab:: Windows

    .. parsed-literal::
      :class: highlight

      mcumgr conn add serial_1 type="serial" connstring="COM1,baud=115200"

  .. group-tab:: Linux

    .. parsed-literal::
      :class: highlight

      mcumgr conn add serial_1 type="serial" connstring="dev=/dev/ttyACM0,baud=115200"

Managing direct image upload
============================

By default, the SMP server uses the first slot.
To upload to a different slot, use the image upload MCUmgr command with a selected image number, ensuring the ``CONFIG_MCUBOOT_SERIAL_DIRECT_IMAGE_UPLOAD`` Kconfig option is enabled.
The ``CONFIG_UPDATEABLE_IMAGE_NUMBER`` Kconfig option adjusts the number of image pairs supported.

See the image number to partition mapping:

* 0 and 1 - image-0, primary slot of the first image
* 2 - image-1, the secondary slot of the first image
* 3 - image-2
* 4 - image-3
* 0 - the default upload target when no selection is made

System-specific commands
========================

By setting the ``CONFIG_ENABLE_MGMT_PERUSER`` Kconfig option, you can enable additional commands.
If you want to enable erasing the storage partition, set the ``CONFIG_BOOT_MGMT_CUSTOM_STORAGE_ERASE`` Kconfig option.

Image management examples
=========================

The SMP protocol, when implemented through MCUboot, operates identically to its implementation through an application, using the same set of commands.
Ensure you have established the connection configuration to manage your images.

* To upload an image, run the following command:

  .. parsed-literal::
    :class: highlight

    mcumgr image upload *path_to_signed_image_bin_file* -c serial_1

  You should see the output:

  .. parsed-literal::
    :class: highlight

    20.25 KiB / 20.25 KiB [=================================] 100.00% 3.08 KiB/s 6s
    Done

* To list all images, run the following command:

  .. parsed-literal::
    :class: highlight

    mcumgr image list -c serial_1

  The terminal will return the output:

  .. parsed-literal::
    :class: highlight

    Images:
     image=0 slot=0
        version: 0.0.0.0
        bootable: false
        flags:
        hash: Unavailable
    Split status: N/A (0)

Resetting the device
====================

The device must be reset after updating or uploading a new firmware image through the serial recovery mode by running the following command:

.. parsed-literal::
  :class: highlight

  mcumgr reset -c serial_1
