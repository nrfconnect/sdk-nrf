.. _ug_bootloader_adding_sysbuild:

Adding a bootloader chain using sysbuild
########################################

.. contents::
   :local:
   :depth: 2

You can add a bootloader chain to an application in the following ways:

* Permanently:

  * Using :file:`sysbuild.conf` sysbuild Kconfig project configuration files.

* Temporarily (for a single build):

  * Using :ref:`one-time CMake arguments <zephyr:west-building-cmake-args>`.
  * Using ``sysbuild_menuconfig``.


While you can use temporary configurations for quickly experimenting with different configurations from build to build, the recommended method is to implement your bootloader chain with permanent configurations.

You can add bootloader chains to nearly any sample in |NCS| or Zephyr for rapid testing and experimentation.

Choose the bootloader type depending on your application needs.
For detailed information about the bootloader support in the |NCS| and the general architecture, see :ref:`ug_bootloader`.

.. _ug_bootloader_adding_sysbuild_immutable:

Adding an immutable bootloader
******************************

The following sections describe how to add either |NSIB| or MCUboot as an immutable bootloader.

.. _ug_bootloader_adding_sysbuild_immutable_b0:

Adding |NSIB| as an immutable bootloader
========================================

To build |NSIB| with a Zephyr or |NCS| sample, enable the ``SB_CONFIG_SECURE_BOOT_APPCORE`` in the application's :file:`sysbuild.conf` file or using the command line:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- -DSB_CONFIG_SECURE_BOOT_APPCORE=y

|how_to_configure|

Like other images, :ref:`image-specific configurations <ug_multi_image_variables>` can be assigned at build time to further customize the bootloader's functionality.

To ensure that the immutable bootloader occupies as little flash memory as possible, you can also apply the :file:`prj_minimal.conf` configuration:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- \
   -DSB_CONFIG_SECURE_BOOT_APPCORE=y \
   -Db0_FILE_SUFFIX=minimal

See :ref:`ug_bootloader_config` for more information about using Kconfig fragments.

Configuring |NSIB| as an immutable bootloader
---------------------------------------------

The following sections describe different configuration options available for |NSIB| as an immutable bootloader.

.. _ug_bootloader_adding_sysbuild_immutable_keys:

Adding a custom signature key file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To specify a signature key file for this bootloader, set the ``SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE`` option in the application's :file:`sysbuild.conf` file or using the command line:

.. tabs::

   .. group-tab:: Kconfig / sysbuild.conf

      .. code-block:: console

         SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE="<path_to>/priv.pem"

   .. group-tab:: Command line

      .. code-block:: console

         -DSB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE=\"<path_to>/priv.pem\"

      Escaped quotations avoid malformed-string warnings from Kconfig.

This option only accepts the private key of an ECDSA key pair, as the build system scripts automatically extract the public key at build time.

The file argument must be a string and is specified in one of the following ways:

* The relative path to the file from the application configuration directory (if this is not set, then it will be the same as the application source directory).

* The absolute path to the file.

For example, if a directory named :file:`_keys` located in :file:`/home/user/ncs` contains signing keys, you can provide the path in the following ways:

.. tabs::

   .. group-tab:: Kconfig / sysbuild.conf

      .. code-block:: console

         SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE="../../_keys/priv.pem"

      Or

      .. code-block:: console

         SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE="/home/user/ncs/_keys/priv.pem"

   .. group-tab:: Command line

      .. code-block:: console

         -DSB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE=\"../../_keys/priv.pem\"

      Or

      .. code-block:: console

         -DSB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE=\"/home/user/ncs/_keys/priv.pem\"

      Or, if you set an environment variable named :envvar:`NCS` to :file:`/home/user/ncs`:

      .. code-block:: console

         -DSB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE=\"$NCS/_keys/priv.pem\"

.. note::

   The public key string must contain a list of files where each item can be indicated as follows:

   * Using the relative path to a file from the application configuration directory.
     When not specified, it is assumed as the default application source directory.
   * Using the absolute path to a file.

   Environment variables (like :envvar:`$HOME`, :envvar:`$PWD`, or :envvar:`$USER`) and the ``~`` character on Unix systems are not expanded when setting an absolute path from a :file:`sysbuild.conf` file but are expanded correctly in key file paths from the command line that are not given as strings.

You can find specific configuration options for keys with this bootloader in :file:`nrf/sysbuild/Kconfig.secureboot`.

See :ref:`ug_fw_update_keys` for information on how to generate custom keys for a project.

Additionally, the |NSIB| supports the following methods for signing images with private keys:

* Uses the ``SB_CONFIG_SECURE_BOOT_SIGNING_OPENSSL`` Kconfig option.
* :ref:`Using a custom command <ug_bootloader_adding_sysbuild_immutable_b0_custom_signing>` - Uses the ``SB_CONFIG_SECURE_BOOT_SIGNING_CUSTOM`` Kconfig option.

The OpenSSL method is handled internally by the build system, whereas using custom commands requires more configuration steps.

Checking the public key
^^^^^^^^^^^^^^^^^^^^^^^

You can check that the bootloader image is correctly compiled with the custom signing key by comparing its auto-generated public key against a manual public key dump using OpenSSL.
You can do this with ``diff``, running the following command from a terminal:

.. code-block:: console

   diff build/zephyr/nrf/subsys/bootloader/generated/public.pem <(openssl ec -in priv.pem -pubout)

If there is no file diff output, then the private key has been successfully included in the bootloader image.

.. _ug_bootloader_adding_sysbuild_immutable_b0_custom_signing:

Custom signing commands
~~~~~~~~~~~~~~~~~~~~~~~

If you want complete control over the key handling of a project, you can use a custom signing command with |NSIB|.
Using a custom signing command removes the need to use of a private key from the build system.
This is useful when the private keys are stored, managed, or otherwise processed through a *hardware security module* (`HSM`_) or an in-house tool.

To use a custom signing command with this bootloader, set the following options in the application's :file:`sysbuild.conf` file or using the command line:

.. tabs::

   .. group-tab:: Kconfig / sysbuild.conf

      .. code-block:: console

         SB_CONFIG_SECURE_BOOT_APPCORE=y
         SB_CONFIG_SECURE_BOOT_SIGNING_CUSTOM=y
         SB_CONFIG_SECURE_BOOT_SIGNING_PUBLIC_KEY="/path/to/pub.pem"
         SB_CONFIG_SECURE_BOOT_SIGNING_COMMAND="my_command"

   .. group-tab:: Command line

      .. code-block:: console

         west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- \
         -DSB_CONFIG_SECURE_BOOT_APPCORE=y \
         -DSB_CONFIG_SECURE_BOOT_SIGNING_CUSTOM=y \
         -DSB_CONFIG_SECURE_BOOT_SIGNING_PUBLIC_KEY=\"/path/to/pub.pem\" \
         -DSB_CONFIG_SECURE_BOOT_SIGNING_COMMAND=\"my_command\"

      Escaped quotations avoid malformed-string warnings from Kconfig.

.. note::
   The public key string must contain a list of files where each item can be indicated as follows:

   * Using the relative path to a file from the application configuration directory.
     When not specified, it is assumed as the default application source directory.
   * Using the absolute path to a file.

See ``SB_CONFIG_SECURE_BOOT_SIGNING_COMMAND`` for specifics about what a usable signing command must do.
The command string can include its own arguments like a typical terminal command, including arguments specific to the build system:

.. parsed-literal::
   :class: highlight

   my_command *[options]* *<args ...>* *<build_system_args ..>*

See the description of ``SB_CONFIG_SECURE_BOOT_SIGNING_COMMAND`` for which arguments can be sent to the build system in this way.

.. note::

   Whitespace, hyphens, and other non-alphanumeric characters must be escaped appropriately when setting the string from the command line.
   If the custom signing command uses its own options or arguments, it is recommended to define the string in a :file:`sysbuild.conf` file to avoid tracking backslashes.
   Like public key paths, environment variables are not expanded when using them in a command string set from the file.

.. _ug_bootloader_adding_sysbuild_immutable_mcuboot:

Adding MCUboot as an immutable bootloader
=========================================

To build :doc:`MCUboot <mcuboot:index-ncs>` with a Zephyr or |NCS| sample, enable the ``SB_CONFIG_BOOTLOADER_MCUBOOT`` in the application's :file:`sysbuild.conf` file or using the command line:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

|how_to_configure|
Like other images, you can assign :ref:`image-specific configurations <ug_multi_image_variables>` at build time to further customize the bootloader's functionality.

Configuring MCUboot as an immutable bootloader
----------------------------------------------

The following sections describe different configuration options available for MCUboot as an immutable bootloader.

.. _ug_bootloader_adding_sysbuild_immutable_mcuboot_keys:

Adding a custom signature key file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can specify the signature key file for this bootloader by setting the ``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE`` option to the selected private key file.
You can set the option in :file:`sysbuild.conf` or using the command line:

.. tabs::

   .. group-tab:: Kconfig / sysbuild.conf

      .. code-block:: console

         SB_CONFIG_BOOT_SIGNATURE_KEY_FILE="priv.pem"

   .. group-tab:: Command line

      .. code-block:: console

         -DSB_CONFIG_BOOT_SIGNATURE_KEY_FILE=\"priv.pem\"

      Escaped quotations avoid malformed-string warnings from Kconfig.

The path of the key must be an absolute path, though ``${APPLICATION_CONFIG_DIR}`` can be used to get the path of the application configuration directory to use keys relative to this directory.

See :ref:`ug_fw_update_keys` for information on how to generate custom keys for a project.

The key type must also be set correctly:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- \
   -DSB_CONFIG_BOOTLOADER_MCUBOOT=y \
   -DSB_CONFIG_BOOT_SIGNATURE_KEY_FILE=\"${APPLICATION_CONFIG_DIR}/../../priv-ecdsa256.pem\" \
   -DSB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y

You can find specific configuration options for keys with this bootloader in :file:`zephyr/share/sysbuild/images/bootloader/Kconfig`.

Checking the public key
^^^^^^^^^^^^^^^^^^^^^^^

You can extract the public key locally and compare it against MCUboot's auto-generated file to verify that it is using the custom key:

.. code-block:: console

   diff build/mcuboot/zephyr/autogen-pubkey.c <(python3 bootloader/mcuboot/scripts/imgtool.py getpub -k priv.pem)

If there is no file diff output, then the private key was successfully included with the bootloader image.

.. _ug_bootloader_adding_sysbuild_upgradable:

Adding an upgradable bootloader
*******************************

MCUboot is the only upgradable bootloader currently available for the |NCS|.
The following section describes how to add it to your secure bootloader chain.

.. _ug_bootloader_adding_sysbuild_upgradable_mcuboot:

Adding MCUboot as an upgradable bootloader
==========================================

To use MCUboot as an upgradable bootloader to your application, complete the following steps:

1. :ref:`Add nRF Secure Immutable Bootloader as the immutable bootloader <ug_bootloader_adding_sysbuild_immutable_b0>`.
#. Add MCUboot to the boot chain by including the ``SB_CONFIG_BOOTLOADER_MCUBOOT`` Kconfig option with either the build command or in the application's :file:`sysbuild.conf` file:

   .. code-block::

      west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- \
      -DSB_CONFIG_SECURE_BOOT_APPCORE=y \
      -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

   |how_to_configure|

#. Optionally, you can configure MCUboot to use the cryptographic functionality exposed by the immutable bootloader and reduce the flash memory usage for MCUboot to less than 16 kB.
   To enable this configuration, apply both the :file:`prj_minimal.conf` Kconfig project file and the :file:`external_crypto.conf` Kconfig fragment for the MCUboot image:

   .. code-block::

      west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- \
      -DSB_CONFIG_SECURE_BOOT_APPCORE=y \
      -DSB_CONFIG_BOOTLOADER_MCUBOOT=y \
      -Dmcuboot_FILE_SUFFIX=minimal \
      -Dmcuboot_EXTRA_CONF_FILE=external_crypto.conf

   See :ref:`ug_bootloader_config` for more information about using Kconfig fragments with bootloaders.

The build process generates several :ref:`app_build_output_files`, including :ref:`app_build_mcuboot_output`.

Configuring MCUboot as an upgradable bootloader
-----------------------------------------------

The following sections describe different configuration options available for MCUboot as an upgradable bootloader.

Adding a custom signature key file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The process to use specific signature keys with MCUboot used as the upgradable bootloader is the same as when it is used :ref:`as the immutable one <ug_bootloader_adding_sysbuild_immutable_mcuboot_keys>`.

.. note::

   Since each bootloader is built with its own signature key, using a different private key with an upgradable bootloader will not cause problems with the secure boot chain.
   You can also use the same private key for both the immutable and upgradable bootloaders, as long as the key type is supported by both of them.

.. _ug_bootloader_adding_sysbuild_presigned_variants:

Generating pre-signed variants
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The S1 variant is built as a separate image called ``s1_image`` automatically.
This variant image will use the same application configuration as the base image, with the exception of its placement in memory.
You only have to modify the version set in the :kconfig:option:`CONFIG_FW_INFO_FIRMWARE_VERSION` Kconfig option.
To make ``s1_image`` bootable with |NSIB|, the value of :kconfig:option:`CONFIG_FW_INFO_FIRMWARE_VERSION` for the default image (or MCUboot if using MCUboot as a second-stage bootloader) must be bigger than the one for original image.

Using MCUboot in firmware loader mode
**************************************

MCUboot supports a firmware loader mode which is supported in sysbuild. This mode allows for a project to consist of an MCUboot image (optionally with serial recovery), a main application which does not support firmware updates, and a secondary application which is dedicated to loading firmware updates. The benefit of this is for having a dedicated application purely for loading firmware updates e.g. over Bluetooth and allowing the size of tha main application to be smaller, helping on devices with limited flash or RAM.
In order to use this mode, a static partition file must be created for the application to designate the addresses and sizes of the main image and firmware loader applications, the firmware loader partition **must** be named ``firmware_loader``. The following is an example static partition manager file for the nRF53:

.. code-block:: yaml

    app:
      address: 0x10200
      region: flash_primary
      size: 0xdfe00
    mcuboot:
      address: 0x0
      region: flash_primary
      size: 0x10000
    mcuboot_pad:
      address: 0x10000
      region: flash_primary
      size: 0x200
    mcuboot_primary:
      address: 0x10000
      orig_span: &id001
      - mcuboot_pad
      - app
      region: flash_primary
      size: 0xc0000
      span: *id001
    mcuboot_primary_app:
      address: 0x10200
      orig_span: &id002
      - app
      region: flash_primary
      size: 0xbfe00
      span: *id002
    firmware_loader:
      address: 0xd0200
      region: flash_primary
      size: 0x1fe00
    mcuboot_secondary:
      address: 0xd0000
      orig_span: &id003
      - mcuboot_pad
      - firmware_loader
      region: flash_primary
      size: 0x20000
      span: *id003
    mcuboot_secondary_app:
      address: 0xd0200
      orig_span: &id004
      - firmware_loader
      region: flash_primary
      size: 0x1fe00
      span: *id004
    settings_storage:
      address: 0xf0000
      region: flash_primary
      size: 0x10000
    pcd_sram:
      address: 0x20000000
      size: 0x2000
      region: sram_primary

The project must also have a ``sysbuild.cmake`` file which includes the firmware loader application in the build, this **must** be named ``firmware_loader``:

.. code-block:: cmake

      ExternalZephyrProject_Add(
        APPLICATION firmware_loader
        SOURCE_DIR <path_to_firmware_loader_application>
      )

There must also be a ``sysbuild.conf`` file which selects the required sysbuild options for enabling MCUboot and selecting the firmware loader mode:

.. code-block:: cfg

    SB_CONFIG_BOOTLOADER_MCUBOOT=y
    SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER=y

At least one mode must be set in MCUboot for entering the firmware loader application, supported entrance methods include:

* GPIO
* Boot mode using retention subsystem
* No valid main application
* Device reset using dedicated reset pin

For this example, the use of a GPIO when booting will be used. Create a ``sysbuild`` folder and add a ``sysbuild/mcuboot.conf`` Kconfig fragment file to use when building MCUboot with the following:

.. code-block:: cfg

    CONFIG_BOOT_FIRMWARE_LOADER_ENTRANCE_GPIO=y

The project can now be built and flashed and will boot the firmware loader application when the button is held upon device reboot, or the main application will be booted when the device is reset and the button is not held down.
