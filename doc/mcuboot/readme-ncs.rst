.. _mcuboot_ncs:

Using MCUboot in nRF Connect SDK
################################

The |NCS| includes MCUboot-specific integration files located in the :file:`modules/mcuboot` subfolder in the `sdk-nrf`_ repository.

You can use MCUboot in the |NCS| in one of the following roles:

* As an `Immutable first-stage bootloader`_ that can perform device firmware updates to the application.
  Enable :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` to use it in this role.
* As an `Second-stage upgradable bootloader`_ that can perform device firmware updates to both itself and the application.
  Enable both :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` and :kconfig:option:`CONFIG_SECURE_BOOT` to use it in this role.

See the following user guides for more information on adding, configuring, and testing MCUboot for your application build in the |NCS|:

* `Secure bootloader chain`_
* `Adding a bootloader chain`_
* `Testing the bootloader chain`_
* `Using external memory partitions`_
* `Customizing the bootloader`_
* `Signature keys`_

When you add MCUboot to your application build, the files that can be used for firmware over-the-air (FOTA) upgrades are automatically generated.
See the `MCUboot output build files`_ page for a list of all these files.

See the `Multi-image builds`_ page for more information on image files in multi-image builds.

.. note::
   When you use MCUboot in the direct-xip mode, enable the :kconfig:option:`CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT` Kconfig option to let the build system generate an additional set of files for the second application slot.
   These ``.hex`` files are identical to the ones listed above, but their names also use the ``mcuboot_secondary_`` prefix.
   For example, :file:`mcuboot_secondary_app_signed.hex` is created and placed in the second slot on the target device when the :file:`app_signed.hex` file is placed in the first slot.
   For more information about the direct-xip mode, see the *Equal slots (direct-xip)* section in the :doc:`Bootloader documentation <design>`.
