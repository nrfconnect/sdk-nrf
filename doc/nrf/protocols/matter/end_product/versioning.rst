.. _ug_versioning_in_matter:

Maintaining firmware version
############################

.. contents::
   :local:
   :depth: 2

To effectively deploy a Matter product, it is essential to implement application version management.
This versioning is crucial for managing firmware upgrades on devices.
It is also displayed within ecosystem applications, as provided by the Basic Information cluster.

There are two primary approaches for maintaining versioning:

* Utilizing a :file:`VERSION` file as detailed on the :ref:`app-version-details` page of the Zephyr Project documentation.
  This method involves defining the version information in a specific file format.

* Utilizing dedicated Kconfig configurations.
  This approach uses Kconfig options to set and manage the versioning details.

Choose the approach that best aligns with your project requirements and infrastructure.

.. note::
  These approaches cannot be used simultaneously.
  The Kconfig options for configuring the versioning details are unavailable if the :file:`VERSION` file is present.

Using :file:`VERSION` file
**************************

To implement versioning based on a :file:`VERSION` file, you must create a file named :file:`VERSION` within the application directory and ensure it contains the appropriate content:

.. code-block:: console

    VERSION_MAJOR =
    VERSION_MINOR =
    PATCHLEVEL =
    VERSION_TWEAK =
    EXTRAVERSION =

.. note::
   You must assign a value to at least one of the variables.
   If not, the :file:`VERSION` file will cause error.

For example:

.. code-block:: console

   VERSION_MAJOR = 2
   VERSION_MINOR = 5
   PATCHLEVEL = 99
   VERSION_TWEAK = 0
   EXTRAVERSION = dev

The :file:`VERSION` file is used to derive the firmware version in the following format for:

* MCUboot image: ``MAJOR.MINOR.PATCHLEVEL+TWEAK``.
  The above example would be formatted as ``2.5.99+0``.
* Matter OTA image: 32-bit integer where each numeric variable is represented by 8 bits within the integer.
  The above example would be formatted as ``0x02056300``.
* Informational purposes, for example displayed in the ecosystem applications:

  * If ``EXTRAVERSION`` is non-empty: ``MAJOR.MINOR.PATCHLEVEL-EXTRA+TWEAK``.
    The above example would be formatted as ``2.5.99-dev+0``.
  * Otherwise: ``MAJOR.MINOR.PATCHLEVEL+TWEAK``.
    The above example would be formatted as ``2.5.99+0``.

Using Kconfig options
*********************

Depending on how you transfer the updated images to the device, you need to use different Kconfig options.

.. _ug_matter_dfu_ota:

Device Firmware Upgrade over Matter versioning
==============================================

When using DFU over Matter, you must edit the :file:`prj.conf` file in the application directory.
Add the following Kconfig options to change the version:

* :kconfig:option:`CONFIG_CHIP_DEVICE_SOFTWARE_VERSION` to set to the version number.
* :kconfig:option:`CONFIG_CHIP_DEVICE_SOFTWARE_VERSION_STRING` to set the version string.

Additionally, since Nordic chips use MCUboot Image Tool, you need to also edit the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option, with a value in the following format: ``"MAJOR . MINOR . PATCHLEVEL + TWEAK"``.

For example:

.. code-block:: console

   CONFIG_CHIP_DEVICE_SOFTWARE_VERSION=33907456
   CONFIG_CHIP_DEVICE_SOFTWARE_VERSION_STRING="2.5.99+0"
   CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION="2.5.99+0"

Where ``33907456`` is 0x02056300, the hexadecimal versioning of 2.5.99+0.

.. _ug_matter_dfu_smp:

Device Firmware Upgrade over Bluetooth LE versioning
====================================================

For DFU over Bluetooth LE, you need to edit the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option in the :file:`prj.conf` file in the application directory.
Set its value in the following format: ``"MAJOR . MINOR . PATCHLEVEL + TWEAK"``.

.. _ug_matter_dfu_performing:

Performing Device Firmware Upgrade
**********************************

After properly configuring the application version, you can perform device firmware upgrade as explained in :doc:`matter:nrfconnect_examples_software_update`.
