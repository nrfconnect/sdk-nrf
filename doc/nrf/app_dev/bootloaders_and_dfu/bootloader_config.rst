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

You can also use custom project configuration files to set permanent options for the associated image, specifying them at build time using :ref:`ug_multi_image_variables`.

For example, you can assign custom project configurations for both the bootloaders and a sample application as follows:

.. code-block:: console

   west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- \
   -Db0_CONF_FILE=prj_immutable.conf \
   -Dmcuboot_CONF_FILE=prj_upgradable.conf \
   -DCONF_FILE=prj_app.conf

In the example above, :file:`prj_app.conf` includes :kconfig:option:`CONFIG_SECURE_BOOT` and ``CONFIG_BOOTLOADER_MCUBOOT`` to enable the immutable and upgradable bootloaders by default.

You can use custom project configuration files in combination with temporary configuration options associated with a single build, set using either the command line or Kconfig fragments.

Assigning Kconfig fragments
***************************

You can use Kconfig fragments specific to bootloaders to set temporary configuration options for the associated image, specifying them at build time using :ref:`ug_multi_image_variables`.

For example, you can assign the :file:`my-custom-fragment.conf` fragment to the |NSIB|, which uses the child name of ``b0``, as follows:

.. code-block:: console

   west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- \
   -DCONFIG_SECURE_BOOT=y \
   -DCONFIG_BOOTLOADER_MCUBOOT=y \
   -Db0_OVERLAY_CONFIG=my-custom-fragment.conf

In the same way, you can replace ``b0`` with ``mcuboot`` to apply the :file:`my-custom-fragment.conf` fragment to the MCUboot image:

.. code-block:: console

   west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- \
   -DCONFIG_SECURE_BOOT=y \
   -DCONFIG_BOOTLOADER_MCUBOOT=y \
   -Dmcuboot_OVERLAY_CONFIG=my-custom-fragment.conf

You can use this method to apply Kconfig fragments to any child image in the build, as well as to set any Kconfig option that can be set from the command line.

See :ref:`ug_multi_image_variables` for more information about customizing images using this method.
