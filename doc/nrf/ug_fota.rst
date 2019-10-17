.. _ug_fota:

Firmware Over The Air (FOTA) Upgrades
=====================================

A typical project for the nrf9160 will contain several individually compiled components.
What components can be upgraded depend on various configurations.
This chapter will describe what is required to make a given component upgradeable, and how to upgrade the various components.

All supported FOTA procedures can be handled by the :ref:`FOTA Downolad library<lib_fota_download>`.
The type of update is deduced by inspecting the header of the firmware.
Once the type of update has been found, the :ref:`DFU Target Library<lib_dfu_target>` is invoked to do what is needed to apply the firmware.

This document describes the process after it has been decided that the firmware pointed to by a given URL should be used to perform a DFU operation on the device.
The process of providing that URL to the device is non-trivial.
One solution used for this is :ref:`lib_aws_fota`.

.. note::
  Even though the SPM component and the application are two individually compiled components, they are treated as a single binary blob in the context of firmware upgrades.

Application (and SPM)
********************

To allow upgrades of the application (and SPM) it is required that MCUBoot (``CONFIG_BOOTLOADER_MCUBOOT=y``) and the :ref:`FOTA Download library<lib_fota_download>` is included in the build.
With this configuration set, a set of files used for FOTA will be automatically generated.
In summary :file:`app_update.bin` is what is uploaded to the server and downloaded using FOTA, while :file:`app_moved_test_update.hex` is what is used when flashing locally to verify the DFU procedure.

* :file:`app_update.bin` - Contains a signed variant of the firmware in binary format (as opposed to intelhex).
  This is the file that is uploaded to the server and downloaded as the actual FOTA.

* :file:`app_to_sign.bin` - Unsigned variant of the firmware in binary format.

* :file:`app_signed.hex` - Signed variant of the firmware in intelhex format.
  This hex file is linked against the same address as the normal application.
  Hence, flashing this to the device will result in a direct overwrite of the application, not a trigger of the DFU procedure.

* :file:`app_test_update.hex` - Same as :file:`app_signed.hex` except that it contains metadata which instructs MCUBoot to test this firmware upon boot.
  As with :file:`app_signed.hex`, this hex file is linked against the same address as the normal application, and will simply overwrite the application when flashed, as opposed to trigger a DFU procedure.

* :file:`app_moved_test_update.hex` - Same as :file:`app_test_update.hex` except that it has been moved to the address used to store the upgrade candidates.
  When this file is flashed to the device, MCUBoot will trigger the DFU procedure upon reboot.

Modem firmware
**************

The modem firmware can be upgraded over the air using the :ref:`FOTA Download library<lib_fota_download>` if the ``CONFIG_DFU_TARGET_MODEM`` option is set.
MCUBoot is not required to support these updates, as they are handled by the modem itself.

Modem upgrades can be done in two ways, through delta patching, and through a full upgrade.
A wired connection is required to perform a full upgrade.
These are typically performed using nRF Connect for Desktop.

Delta patches are upgrades where only the difference from the last version in contained in the binary.
Only delta patches are supported over the air.

A delta patch can only upgrade the modem firmware from one specific version to another version.
I.e, to upgrade devices with version 1, 2, 3, 4 and 5 to version 6 - five different delta patches are needed.

..
  TODO: Fix link to 'here'
More information on what delta patch can be used for what modem firmware version can be found here.


MCUBoot
*******

..
  TODO: Fix link to 'Bootloader sample'

For MCUBoot to be upgradeable, Bootloader sample  needs to be included in the build (``CONFIG_SECURE_BOOT=y``) in addition to MCUBoot.
With this configuration, MCUBoot will be stored in one of two possible partitions called S0 and S1.
The fist stage bootloader implemented by the Bootloader sample (hereby called B0)  will verify and start the MCUBoot variant with the highest version number.
The copy routine itself is performed as a regular application update, with the exception that the partition being written to is the non-active S0/S1 slot.

As MCUBoot can execute from two different flash locations, S0 or S1, it is necessary to provide two variants, one compiled against each location.
This is done automatically by the build system, and the zip file to be provided to the FOTA backend is also generated.
To ensure that the correct variant is downloaded, the application must check which of the slots (S0 or S1) is currently in use, and download the candidate linked against the other one.
This is handled in the ``bootloader`` target of DFU Target.

..
  TODO: Fix link to 'DFU Target'


