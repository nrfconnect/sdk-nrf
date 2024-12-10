.. _ug_bootloader_config:

Customizing the bootloader
##########################

.. contents::
   :local:
   :depth: 2

By default, to set Kconfig options, the |NSIB| and MCUboot bootloaders use Kconfig project configuration files, usually called :file:`prj.conf`, located in their source directories:

* |NSIB| - :file:`nrf/samples/bootloader`
* MCUboot - :file:`bootloader/mcuboot/boot/zephyr`

However, there are other ways to customize your application using Kconfig options:

* Using custom project configurations - For permanent options.
* Using Kconfig fragments - For temporary options.

Using custom project configurations
***********************************

You can use custom project configuration options for the associated image, specifying them at build time.
These settings are temporary and will remain enabled until you clean the build pristinely.

For example, you can temporarily assign custom project configurations for both the bootloaders and a sample application as follows:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- \
   -Db0_FILE_SUFFIX=immutable \
   -Dmcuboot_FILE_SUFFIX=upgradable \
   -Dapp_FILE_SUFFIX=app

You can use custom project configuration files in combination with temporary configuration options associated with a single build, set using either the command line or Kconfig fragments.

Assigning Kconfig fragments
***************************

You can use Kconfig fragments specific to bootloaders to set temporary configuration options for the associated image, specifying them at build time.

For example, you can assign the :file:`my-custom-fragment.conf` fragment to the |NSIB|, which uses the image name of ``b0``, as follows:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- \
   -DSB_CONFIG_SECURE_BOOT_APPCORE=y \
   -DSB_CONFIG_BOOTLOADER_MCUBOOT=y \
   -Db0_EXTRA_CONF_FILE=my-custom-fragment.conf

In the same way, you can replace ``b0`` with ``mcuboot`` to apply the :file:`my-custom-fragment.conf` fragment to the MCUboot image:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- \
   -DSB_CONFIG_SECURE_BOOT_APPCORE=y \
   -DSB_CONFIG_BOOTLOADER_MCUBOOT=y \
   -Dmcuboot_EXTRA_CONF_FILE=my-custom-fragment.conf

You can use this method to apply Kconfig fragments to any image in the build, and set any Kconfig option that is available from the command line.

See :ref:`sysbuild` for more information about customizing images using this method.

Customizing partitions
**********************

With the Partition Manager, you can further customize it if a dynamic partition map has been set.
For more information, see the :ref:`Configuration <pm_configuration>` section of the :ref:`partition_manager` page.
