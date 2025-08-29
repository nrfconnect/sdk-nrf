.. _mcuboot_ncs:

Using MCUboot in nRF Connect SDK
################################

The |NCS| includes MCUboot-specific integration files located in the :file:`modules/mcuboot` subfolder in the `sdk-nrf`_ repository.

You can use MCUboot in the |NCS| in one of the following roles:

* As an `Immutable first-stage bootloader`_ that can perform device firmware updates to the application.
  Enable :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` in sysbuild to use it in this role.
* As an `Second-stage upgradable bootloader`_ that can perform device firmware updates to both itself and the application.
  Enable both :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` and :kconfig:option:`SB_CONFIG_SECURE_BOOT_APPCORE` in sysbuild to use it in this role.

See the following user guides for more information on adding, configuring, and testing MCUboot for your application build in the |NCS|:

* `Secure bootloader chain`_
* `Enabling a bootloader chain using sysbuild`_
* `MCUboot and NSIB`_
* `Partitioning device memory`_
* `Customizing the bootloader`_
* `Signature keys`_

When you add MCUboot to your application build, the files that can be used for firmware over-the-air (FOTA) upgrades are automatically generated.
See the `MCUboot output build files`_ page for a list of all these files.

.. note::
   When you use MCUboot in the direct-xip mode, enable the :kconfig:option:`SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT` sysbuild Kconfig option to let the build system generate an additional set of files for the second application slot.
   These files are identical to the ones listed on the `MCUboot output build files`_ page, but they are placed in the :file:`mcuboot_secondary_app` folder for the main application, while the files for the remaining applications are located in the respective :file:`<application>_secondary_app` folder.
   For example, :file:`mcuboot_secondary_app/zephyr/zephyr.signed.bin` is created and placed in the second slot on the target device when the :file:`zephyr.signed.bin` file is placed in the first slot.
   Similarly, the :file:`ipc_radio_secondary_app/zephyr/zephyr.signed.bin` file is created and placed in the second slot on the target device when the :file:`ipc_radio/zephyr/zephyr.signed.bin` file is placed in the first slot.
   For more information about the direct-xip mode, see the *Equal slots (direct-xip)* section in the :doc:`Bootloader documentation <design>`.
