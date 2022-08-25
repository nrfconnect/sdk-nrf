.. _mcuboot_ncs:

Using MCUboot in nRF Connect SDK
################################

The |NCS| includes MCUboot-specific integration files located in the :file:`modules/mcuboot` subfolder in the `sdk-nrf`_ repository.

You can use MCUboot in the |NCS| in one of the following roles:

* As an `Immutable first-stage bootloader`_ that can perform device firmware updates to the application.
  Enable :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` to use it in this role.
* As an `Upgradable second-stage bootloader`_ that can perform device firmware updates to both itself and the application.
  Enable both :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` and :kconfig:option:`CONFIG_SECURE_BOOT` to use it in this role.

See the following user guides for more information on adding, configuring, and testing MCUboot for your application build in the |NCS|:

* `Secure bootloader chain`_
* `Adding a bootloader chain`_
* `Testing the bootloader chain`_
* `Using external memory partitions`_
* `Customizing the bootloader`_
* `Firmware updates`_

When you add MCUboot to your application build, the following files that can be used for firmware over-the-air (FOTA) upgrades are automatically generated:

* :file:`app_update.bin` - Signed variant of the firmware in binary format (as opposed to intelhex).
  This file can be uploaded to a server as FOTA image.

* :file:`app_to_sign.bin` - Unsigned variant of the firmware in binary format.

* :file:`app_signed.hex` - Signed variant of the firmware in intelhex format.
  This HEX file is linked to the same address as the application.
  Programming this file to the device will overwrite the existing application.
  It will not trigger a DFU procedure.

* :file:`app_test_update.hex` - Same as :file:`app_signed.hex` except that it contains metadata that instructs MCUboot to test this firmware upon boot.
  As :file:`app_signed.hex`, this HEX file is linked against the same address as the application.
  Programming this file to the device will overwrite the existing application.
  It will not trigger a DFU procedure.

* :file:`app_moved_test_update.hex` - Same as :file:`app_test_update.hex` except that it is linked to the address used to store the upgrade candidates.
  When this file is programmed to the device, MCUboot will trigger the DFU procedure upon reboot.

See the `Multi-image builds`_ user guide for more information on image files in multi-image builds.

.. note::
   When you use MCUboot in the direct-xip mode, enable the :kconfig:option:`CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT` Kconfig option to let the build system generate an additional set of files for the second application slot.
   These ``.hex`` files are identical to the ones listed above, but their names also use the ``mcuboot_secondary_`` prefix.
   For example, :file:`mcuboot_secondary_app_signed.hex` is created and placed in the second slot on the target device when the :file:`app_signed.hex` file is placed in the first slot.
   For more information about the direct-xip mode, see the *Equal slots (direct-xip)* section in the :doc:`Bootloader documentation <design>`.
