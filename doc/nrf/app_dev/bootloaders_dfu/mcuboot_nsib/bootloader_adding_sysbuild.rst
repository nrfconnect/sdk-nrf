.. _ug_bootloader_adding_sysbuild:

Enabling a bootloader chain using sysbuild
##########################################

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

To build |NSIB| with a Zephyr or |NCS| sample, enable the :kconfig:option:`SB_CONFIG_SECURE_BOOT_APPCORE` in the application's :file:`sysbuild.conf` file or using the command line:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- -DSB_CONFIG_SECURE_BOOT_APPCORE=y

|how_to_configure|

Like other images, you can assign image-specific configurations at build time to further customize the bootloader's functionality.
For details, see :ref:`zephyr:sysbuild` documentation in Zephyr.

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

To specify a signature key file for this bootloader, set the :kconfig:option:`SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE` option in the application's :file:`sysbuild.conf` file or using the command line:

.. tabs::

   .. group-tab:: Kconfig / sysbuild.conf

      .. code-block:: console

         SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE="<path_to>/priv.pem"

   .. group-tab:: Command line

      .. code-block:: console

         -DSB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE=\"<path_to>/priv.pem\"

      Escaped quotations avoid malformed-string warnings from Kconfig.

This option accepts the private key of an Ed25519 key pair for nRF54L SoCs and private key of an ECDSA key pair for the others.
The build system scripts automatically extract the public key at build time.

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

For SoCs using KMU for NSIB (nRF54L Series devices), the private key must be provisioned in the KMU before NSIB can be run.

Additionally, the |NSIB| supports a custom method for signing images with private keys:

* :ref:`Using a custom command <ug_bootloader_adding_sysbuild_immutable_b0_custom_signing>` - Uses the :kconfig:option:`SB_CONFIG_SECURE_BOOT_SIGNING_CUSTOM` Kconfig option.


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

See :kconfig:option:`SB_CONFIG_SECURE_BOOT_SIGNING_COMMAND` for specifics about what a usable signing command must do.
The command string can include its own arguments like a typical terminal command, including arguments specific to the build system:

.. parsed-literal::
   :class: highlight

   my_command *[options]* *<args ...>* *<build_system_args ..>*

See the description of :kconfig:option:`SB_CONFIG_SECURE_BOOT_SIGNING_COMMAND` for which arguments can be sent to the build system in this way.

.. note::

   Whitespace, hyphens, and other non-alphanumeric characters must be escaped appropriately when setting the string from the command line.
   If the custom signing command uses its own options or arguments, it is recommended to define the string in a :file:`sysbuild.conf` file to avoid tracking backslashes.
   Like public key paths, environment variables are not expanded when using them in a command string set from the file.

.. _ug_bootloader_adding_sysbuild_immutable_mcuboot:

Adding MCUboot as an immutable bootloader
=========================================

To build :doc:`MCUboot <mcuboot:index-ncs>` with a Zephyr or |NCS| sample, enable the :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` in the application's :file:`sysbuild.conf` file or using the command line:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

|how_to_configure|
Like other images, you can assign image-specific configurations at build time to further customize the bootloader's functionality.
For details, see :ref:`zephyr:sysbuild` documentation in Zephyr.

Configuring MCUboot as an immutable bootloader
----------------------------------------------

The following sections describe different configuration options available for MCUboot as an immutable bootloader.

.. _ug_bootloader_adding_sysbuild_immutable_mcuboot_keys:

Adding a custom signature key file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can specify the signature key file for this bootloader by setting the :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` option to the selected private key file.
You can set the option in :file:`sysbuild.conf` or using the command line:

.. tabs::

   .. group-tab:: Kconfig / sysbuild.conf

      .. code-block:: console

         SB_CONFIG_BOOT_SIGNATURE_KEY_FILE="priv.pem"

   .. group-tab:: Command line

      .. code-block:: console

         -DSB_CONFIG_BOOT_SIGNATURE_KEY_FILE=\"priv.pem\"

      Escaped quotations avoid malformed-string warnings from Kconfig.

The path of the key must be an absolute path.
To utilize keys relative to the application configuration directory, use the escaped CMake variable ``\${APPLICATION_CONFIG_DIR}`` in the path by replacing ``$`` with ``\$``.

See :ref:`ug_fw_update_keys` for information on how to generate custom keys for a project.

The key type must also be set correctly:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- \
   -DSB_CONFIG_BOOTLOADER_MCUBOOT=y \
   -DSB_CONFIG_BOOT_SIGNATURE_KEY_FILE=\"\${APPLICATION_CONFIG_DIR}/../../priv-ecdsa256.pem\" \
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
#. Add MCUboot to the boot chain by including the :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` Kconfig option with either the build command or in the application's :file:`sysbuild.conf` file:

   .. code-block::

      west build -b nrf52840dk/nrf52840 zephyr/samples/hello_world -- \
      -DSB_CONFIG_SECURE_BOOT_APPCORE=y \
      -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

   |how_to_configure|

#. In order to reduce the flash memory usage for MCUboot, see :ref:`mcuboot_minimal_configuration`.

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

.. _ug_bootloader_using_firmware_loader_mode:

Using MCUboot in firmware loader mode
**************************************

MCUboot includes a firmware loader mode supported in sysbuild.
This mode enables a project configuration that includes MCUboot instance (optionally with serial recovery), a main application not intended for firmware updates, and a secondary application which is dedicated to loading firmware updates.
The benefit of this configuration is having a dedicated application for loading firmware updates, for example, over Bluetooth®.
This allows the main application to be larger in comparison to any symmetric size dual-bank mode update, which helps on devices with limited flash or RAM.

If your application uses a custom memory layout (a very common scenario), you must include it in the overlay file for the firmware loader image.
For reference, see :file:`nrf/samples/dfu/single_slot/sysbuild/ble_mcumgr/boards/nrf54l15dk_nrf54l15_cpuapp.overlay`.
The firmware loader will be automatically placed in the ``slot1_partition`` partition by the build system.
For devices with a separate radio core, the firmware loader solution has a different architecture.
For details, see :ref:`ug_bootloader_firmware_loader_mode_nrf54h20`.

The project must also configure MCUboot to operate in firmware loader mode and specify a firmware loader image in the :file:`sysbuild.conf` file.
For example, to select the :ref:`fw_loader_ble_mcumgr` firmware loader image, set the following options:

.. code-block:: cfg

    SB_CONFIG_BOOTLOADER_MCUBOOT=y
    SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER=y
    SB_CONFIG_FIRMWARE_LOADER_IMAGE_BLE_MCUMGR=y

At least one mode must be set in MCUboot for entering the firmware loader application, supported entrance methods include:

* GPIO
* Boot mode using retention subsystem
* No valid main application
* Device reset using dedicated reset pin

For this example, the use of a GPIO when booting will be used. Create a ``sysbuild`` folder and add a ``sysbuild/mcuboot.conf`` Kconfig fragment file to use when building MCUboot with the following:

.. code-block:: cfg

    CONFIG_BOOT_FIRMWARE_LOADER_ENTRANCE_GPIO=y

The project can now be built and flashed and will boot the firmware loader application when the button is held upon device reboot, or the main application will be booted when the device is reset and the button is not held down.
See :ref:`sysbuild_images_adding_custom_firmware_loader_images` for details on how to add custom firmware loader images using sysbuild.

.. _ug_bootloader_firmware_loader_mode_nrf54h20:

Firmware loader mode on the nRF54H20 SoC
========================================

On the  nRF54H20 SoC, the firmware loader mode uses the merged slot update strategy.
This means that the application and radio images are merged into a single image, for both the main application and the firmware loader application.
MCUboot treats this merged package as a single image.
For details on the merged slot update strategy, see :ref:`ug_nrf54h20_partitioning_merged`.

The usage of the merged slot update strategy requires the chosen ``zephyr,code-partition`` devicetree node for the firmware loader image to be explicitly set to the ``cpuapp_slot1_partition`` node.
For reference, see :file:`nrf/samples/dfu/single_slot/sysbuild/ble_mcumgr/boards/nrf54h20dk_nrf54h20_cpuapp.overlay`.

.. _ug_bootloader_firmware_loader_update:

Update of the firmware loader image
===================================

.. caution::
   Failures in firmware loader updates make the device unrecoverable without physical access.
   Make sure to test the firmware loader update process thoroughly before deploying the firmware to a production environment.

To build a firmware loader image prepared for update, the ``SB_CONFIG_FIRMWARE_LOADER_UPDATE`` Kconfig option must be set to ``y``.
This causes the build system to generate a special installer application, linked to the same slot as the main application.
The signed firmware loader image is appended at the end of the installer image, creating a single binary blob :file:`fw_loader_installer.merged.bin`.
This blob is then signed with the same key as the main application, creating the :file:`fw_loader_installer.signed.bin` file that can be uploaded to the device.
In the following chapters, the MCUboot image contained in the :file:`fw_loader_installer.signed.bin` is referred to as the installer package.
When building, a :file:`dfu_fw_loader_installer.zip` file is generated, which can be used to update the firmware loader image using DFU like a standard application update.

The following are the DFU steps for updating the firmware loader image.

#. Replacement of existing application: The firmware update process begins with replacing the existing application with the installer package via DFU.
#. Validation: Once the new DFU image is transferred, mcuboot validates the installer package.
#. Installation: After successful validation, the installer is activated.
   The installer carries out the necessary actions to copy the firmware loader to its designated location in the device's memory.
   Parameters such as the number of bytes to copy are determined based on the MCUboot header and TLV information of the new firmware loader image (included in the installer package).
#. Request firmware loader entrance: After the copy is completed, the installer requests firmware loader entrance by the configured method.
#. Entry into DFU mode: MCUBoot validates the newly installed firmware loader image and starts it.
   The device enters DFU mode, ready to receive additional images.

After a successful firmware loader update, the main application must be updated again, as it was previously replaced by the installer image.

The following firmware loader entrance methods are currently supported:

* Installer self-invalidation by erasing the start of the installer package, enabled by :kconfig:option:`CONFIG_INSTALLER_FW_LOADER_ENTRANCE_SELF_INVALIDATE_BY_ERASE`.

  .. note::
     This strategy cannot be used if MCUboot locks the flash area containing the installer.

* Using a boot mode, enabled by :kconfig:option:`CONFIG_INSTALLER_FW_LOADER_ENTRANCE_BOOT_MODE`.
* Using a boot request, enabled by :kconfig:option:`CONFIG_INSTALLER_FW_LOADER_ENTRANCE_BOOT_REQ`.
* Installer does not perform any actions to request firmware loader entrance, enabled by :kconfig:option:`CONFIG_INSTALLER_FW_LOADER_ENTRANCE_METHOD_NONE`.

  This strategy can be used if firmware loader entrance is requested by other means, for example, by a GPIO or a reset pin.

Depending on the board and other configuration, additional steps, such as adding DTS overlays and MCUboot configuration, may be required.

.. caution::
   Do not ignore build warnings about inconsistent configuration.
   When using the :kconfig:option:`CONFIG_INSTALLER_FW_LOADER_ENTRANCE_BOOT_REQ` method, the :kconfig:option:`CONFIG_NRF_BOOT_FIRMWARE_LOADER_BOOT_REQ` Kconfig option must be set to ``y`` in the MCUboot image.
   Similarly, when using the :kconfig:option:`CONFIG_INSTALLER_FW_LOADER_ENTRANCE_BOOT_MODE` method, the :kconfig:option:`CONFIG_BOOT_FIRMWARE_LOADER_BOOT_MODE` Kconfig option must be set to ``y`` in the MCUboot image.
   Failure to ensure this results in a boot loop.

Additionally, the installer can be configured to reboot the device after requesting firmware loader entrance by setting the :kconfig:option:`CONFIG_INSTALLER_REBOOT` Kconfig option to ``y``.
This is the default behavior for the :kconfig:option:`CONFIG_INSTALLER_FW_LOADER_ENTRANCE_SELF_INVALIDATE_BY_ERASE`, :kconfig:option:`CONFIG_INSTALLER_FW_LOADER_ENTRANCE_BOOT_REQ`, and :kconfig:option:`CONFIG_INSTALLER_FW_LOADER_ENTRANCE_BOOT_MODE` methods.
This behavior is not supported for the :kconfig:option:`CONFIG_INSTALLER_FW_LOADER_ENTRANCE_METHOD_NONE` strategy, as it may cause the installer to enter a boot loop.

For examples of how to configure the firmware loader update, refer to the :ref:`single_slot_sample` sample.

The basic installer is provided in the |NCS| as :ref:`installer`, enabled by :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_INSTALLER`.
This is the default selection for the ``FIRMWARE_LOADER_INSTALLER_APPLICATION`` Kconfig choice.
You can use a custom installer by extending the ``FIRMWARE_LOADER_INSTALLER_APPLICATION`` Kconfig choice, as well as the :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_INSTALLER_APPLICATION_IMAGE_NAME` and :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_INSTALLER_APPLICATION_IMAGE_PATH` options.
