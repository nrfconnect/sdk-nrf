.. _ug_bootloader_adding:

Adding a bootloader chain
#########################

.. contents::
   :local:
   :depth: 2

You can add a bootloader chain to an application in the following ways:

* Permanently:

  * Using Kconfig fragments.
  * Using :file:`prj.conf` Kconfig project configuration files.

* Temporarily (for a single build):

  * Using :ref:`one-time CMake arguments <zephyr:west-building-cmake-args>`.
  * Using :ref:`zephyr:menuconfig`.


While you can use temporary configurations for quickly experimenting with different configurations from build to build, the recommended method is to implement your bootloader chain with permanent configurations.

Both Kconfig fragments and Kconfig project configuration files use the same Kconfig syntax for configurations.
You can add bootloader chains to nearly any sample in |NCS| or Zephyr for rapid testing and experimentation.

Choose the bootloader type depending on your application needs.
For detailed information about the bootloader support in the |NCS| and the general architecture, see :ref:`ug_bootloader`.

.. _ug_bootloader_adding_immutable:

Adding an immutable bootloader
******************************

The following sections describe how to add either |NSIB| or MCUboot as an immutable bootloader.

.. _ug_bootloader_adding_immutable_b0:

Adding |NSIB| as an immutable bootloader
========================================

To build |NSIB| with a Zephyr or |NCS| sample, enable the :kconfig:option:`CONFIG_SECURE_BOOT` in the application's :file:`prj.conf` file, in an associated Kconfig fragment, or using the command line:

.. code-block:: console

   west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- -DCONFIG_SECURE_BOOT=y

|how_to_configure|

Like other child images, :ref:`image-specific configurations <ug_multi_image_variables>` can be assigned at build time to further customize the bootloader's functionality.

To ensure that the immutable bootloader occupies as little flash memory as possible, you can also apply the :file:`prj_minimal.conf` configuration:

.. code-block:: console

   west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- \
   -DCONFIG_SECURE_BOOT=y \
   -Db0_CONF_FILE=prj_minimal.conf

See :ref:`ug_bootloader_config` for more information about using Kconfig fragments.

Configuring |NSIB| as an immutable bootloader
---------------------------------------------

The following sections describe different configuration options available for |NSIB| as an immutable bootloader.

.. _ug_bootloader_adding_immutable_keys:

Adding a custom signature key file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To add a signature key file to this bootloader, set the :kconfig:option:`CONFIG_SB_SIGNING_KEY_FILE` option in the application's :file:`prj.conf` file, in an associated Kconfig fragment, or using the command line:

.. tabs::

   .. group-tab:: Kconfig / prj.conf

      .. code-block:: console

         CONFIG_SB_SIGNING_KEY_FILE="priv.pem"

   .. group-tab:: Command line

      .. code-block:: console

         -DCONFIG_SB_SIGNING_KEY_FILE=\"priv.pem\"

      Escaped quotations avoid malformed-string warnings from Kconfig.

This option only accepts the private key of an ECDSA key pair, as build system scripts automatically extract the public key at build time.

The file argument must be a string and is specified in one of the following ways:

* The relative path to the file from the application configuration directory (if this is not set, then it will be the same as the application source directory).

  * If the :file:`prj.conf` file is external to the directory, the key's location is determined relative to the application directory, not to the configuration file.

* The absolute path to the file.

For example, if a directory named :file:`_keys` located in :file:`/home/user/ncs` contains signing keys, you can provide the path in the following ways:

.. tabs::

   .. group-tab:: Kconfig / prj.conf

      .. code-block:: console

         CONFIG_SB_SIGNING_KEY_FILE="../../_keys/priv.pem"

      Or

      .. code-block:: console

         CONFIG_SB_SIGNING_KEY_FILE="/home/user/ncs/_keys/priv.pem"

   .. group-tab:: Command line

      .. code-block:: console

         -DCONFIG_SB_SIGNING_KEY_FILE=\"../../_keys/priv.pem\"

      Or

      .. code-block:: console

         -DCONFIG_SB_SIGNING_KEY_FILE=\"/home/user/ncs/_keys/priv.pem\"

      Or, if you set an environment variable named :envvar:`NCS` to :file:`/home/user/ncs`:

      .. code-block:: console

         -DCONFIG_SB_SIGNING_KEY_FILE=\"$NCS/_keys/priv.pem\"

.. note::

   The public key string must be an absolute path to the location of the public key file.
   Environment variables (like :envvar:`$HOME`, :envvar:`$PWD`, or :envvar:`$USER`) and the ``~`` character on Unix systems are not expanded when setting an absolute path from a :file:`prj.conf` file or Kconfig fragment, but are expanded correctly in key file paths from the command line that are not given as strings.

You can find specific configuration options for keys with this bootloader in :file:`nrf/subsys/bootloader/Kconfig`.

See :ref:`ug_fw_update_keys` for information on how to generate custom keys for a project.

Additionally, the |NSIB| supports the following methods for signing images with private keys:

* :ref:`ug_fw_update_keys_python` - The default method, using the :kconfig:option:`CONFIG_SB_SIGNING_PYTHON`.
* :ref:`ug_fw_update_keys_openssl` - Uses the :kconfig:option:`CONFIG_SB_SIGNING_OPENSSL`.
* :ref:`Using a custom command <ug_bootloader_adding_immutable_b0_custom_signing>` - Uses the :kconfig:option:`CONFIG_SB_SIGNING_CUSTOM`.

Both Python and OpenSSL methods are handled internally by the build system, whereas using custom commands requires more configuration steps.

Checking the public key
^^^^^^^^^^^^^^^^^^^^^^^

You can check that the bootloader image is correctly compiled with the custom signing key by comparing its auto-generated public key against a manual public key dump using OpenSSL.
You can do this with ``diff``, running the following command from a terminal:

.. code-block:: console

   diff build/zephyr/nrf/subsys/bootloader/generated/public.pem <(openssl ec -in priv.pem -pubout)

If there is no file diff output, then the private key has been successfully included in the bootloader image.

.. _ug_bootloader_adding_immutable_b0_custom_signing:

Custom signing commands
~~~~~~~~~~~~~~~~~~~~~~~

If you want complete control over the key handling of a project, you can use a custom signing command with |NSIB|.
Using a custom signing command removes the need to use of a private key from the build system.
This is useful when the private keys are stored, managed, or otherwise processed through a *hardware security module* (`HSM`_) or an in-house tool.

To use a custom signing command with this bootloader, set the following options in the application's :file:`prj.conf` file, in an associated Kconfig fragment, or using the command line:

.. tabs::

   .. group-tab:: Kconfig / prj.conf

      .. code-block:: console

         CONFIG_SECURE_BOOT=y
         CONFIG_SB_SIGNING_CUSTOM=y
         CONFIG_SB_SIGNING_PUBLIC_KEY="/path/to/pub.pem"
         CONFIG_SB_SIGNING_COMMAND="my_command"

   .. group-tab:: Command line

      .. code-block:: console

         west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- \
         -DCONFIG_SECURE_BOOT=y \
         -DCONFIG_SB_SIGNING_CUSTOM=y \
         -DCONFIG_SB_SIGNING_PUBLIC_KEY=\"/path/to/pub.pem\" \
         -DCONFIG_SB_SIGNING_COMMAND=\"my_command\"

      Escaped quotations avoid malformed-string warnings from Kconfig.

.. note::

   The public key string must be an absolute path to the location of the public key file, as mentioned previously in :ref:`ug_bootloader_adding_immutable_keys`.

See :kconfig:option:`CONFIG_SB_SIGNING_COMMAND` for specifics about what a usable signing command must do.
The command string can include its own arguments like a typical terminal command, including arguments specific to the build system:

.. parsed-literal::
   :class: highlight

   my_command *[options]* *<args ...>* *<build_system_args ..>*

See the description of :kconfig:option:`CONFIG_SB_SIGNING_COMMAND` for which arguments can be sent to the build system in this way.

.. note::

   Whitespace, hyphens, and other non-alphanumeric characters must be escaped appropriately when setting the string from the command line.
   If the custom signing command uses its own options or arguments, it is recommended to define the string in a :file:`prj.conf` file or Kconfig fragment to avoid tracking backslashes.
   Like public key paths, environment variables are not expanded when using them in a command string set from one of these files.

.. _ug_bootloader_adding_immutable_mcuboot:

Adding MCUboot as an immutable bootloader
=========================================

To build :doc:`MCUboot <mcuboot:index-ncs>` with a Zephyr or |NCS| sample, enable the :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` in the application's :file:`prj.conf` file, an associated Kconfig fragment, or using the command line:

.. code-block:: console

   west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- -DCONFIG_BOOTLOADER_MCUBOOT=y

|how_to_configure|
Like other child images, you can assign :ref:`image-specific configurations <ug_multi_image_variables>` at build time to further customize the bootloader's functionality.

Configuring MCUboot as an immutable bootloader
----------------------------------------------

The following sections describe different configuration options available for MCUboot as an immutable bootloader.

.. _ug_bootloader_adding_immutable_mcuboot_keys:

Adding a custom signature key file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To pass the signature key file into the MCUboot image, set its :kconfig:option:`CONFIG_BOOT_SIGNATURE_KEY_FILE` option to the selected private key file.
You can set the option in :file:`bootloader/mcuboot/boot/zephyr/prj.conf`, an associated Kconfig fragment, or using the command line:

.. tabs::

   .. group-tab:: Kconfig / prj.conf

      .. code-block:: console

         CONFIG_BOOT_SIGNATURE_KEY_FILE="priv.pem"

   .. group-tab:: Command line

      .. code-block:: console

         -Dmcuboot_CONFIG_BOOT_SIGNATURE_KEY_FILE=\"priv.pem\"

      Escaped quotations avoid malformed-string warnings from Kconfig.

The path of the key works as :ref:`described above <ug_bootloader_adding_immutable_keys>` for |NSIB|, except the application directory for relative pathing is considered to be :file:`bootloader/mcuboot`.

See :ref:`ug_fw_update_keys` for information on how to generate custom keys for a project.

We recommend you also set the associated configuration for a key type to ensure MCUboot compiles the public key into its image correctly.

.. code-block:: console

   west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- \
   -DCONFIG_BOOTLOADER_MCUBOOT=y \
   -Dmcuboot_CONFIG_BOOT_SIGNATURE_KEY_FILE=\"../../priv-ecdsa256.pem\" \
   -Dmcuboot_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y

You can find specific configuration options for keys with this bootloader in :file:`bootloader/mcuboot/boot/zephyr/Kconfig`.

Checking the public key
^^^^^^^^^^^^^^^^^^^^^^^

You can extract the public key locally and compare it against MCUboot's auto-generated file to verify that it is using the custom key:

.. code-block:: console

   diff build/mcuboot/zephyr/autogen-pubkey.c <(python3 bootloader/mcuboot/scripts/imgtool.py getpub -k priv.pem)

If there is no file diff output, then the private key was successfully included with the bootloader image.

.. _ug_bootloader_adding_upgradable:

Adding an upgradable bootloader
*******************************

MCUboot is the only upgradable bootloader currently available for the |NCS|.
The following section describes how to add it to your secure bootloader chain.

.. _ug_bootloader_adding_upgradable_mcuboot:

Adding MCUboot as an upgradable bootloader
==========================================

To use MCUboot as an upgradable bootloader to your application, complete the following steps:

1. :ref:`Add |NSIB| as the immutable bootloader <ug_bootloader_adding_immutable_b0>`.
#. Add MCUboot to the boot chain by including the :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` Kconfig option with either the build command or in the application's :file:`prj.conf` file:

   .. code-block::

      west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- \
      -DCONFIG_SECURE_BOOT=y \
      -DCONFIG_BOOTLOADER_MCUBOOT=y

   |how_to_configure|

#. Optionally, you can configure MCUboot to use the cryptographic functionality exposed by the immutable bootloader and reduce the flash memory usage for MCUboot to less than 16 kB.
   To enable this configuration, apply both the :file:`prj_minimal.conf` Kconfig project file and the :file:`external_crypto.conf` Kconfig fragment for the MCUboot image:

   .. code-block::

      west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- \
      -DCONFIG_BOOTLOADER_MCUBOOT=y \
      -DCONFIG_SECURE_BOOT=y \
      -Dmcuboot_CONF_FILE=prj_minimal.conf \
      -Dmcuboot_OVERLAY_CONFIG=external_crypto.conf

   See :ref:`ug_bootloader_config` for more information about using Kconfig fragments with bootloaders.

Configuring MCUboot as an upgradable bootloader
-----------------------------------------------

The following sections describe different configuration options available for MCUboot as an upgradable bootloader.

Adding a custom signature key file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The process to use specific signature keys with MCUboot used as the upgradable bootloader is the same as when it is used :ref:`as the immutable one <ug_bootloader_adding_immutable_mcuboot_keys>`.

.. note::

   Since each bootloader is built with its own signature key, using a different private key with an upgradable bootloader will not cause problems with the secure boot chain.
   You can also use the same private key for both the immutable and upgradable bootloaders, as long as the key type is supported by both of them.

.. _ug_bootloader_adding_presigned_variants:

Generating pre-signed variants
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Enable the :kconfig:option:`CONFIG_BUILD_S1_VARIANT` Kconfig option when building the upgradable bootloader to automatically generate :ref:`pre-signed variants <upgradable_bootloader_presigned_variants>` of the image for both slots:

.. code-block::

   west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- \
   -DCONFIG_SECURE_BOOT=y \
   -DCONFIG_BOOTLOADER_MCUBOOT=y \
   -DCONFIG_BUILD_S1_VARIANT=y

This is a necessary step for creating application update images for use with :ref:`ug_fw_update`.

The S1 variant is built as a separate child image called ``s1_image``.
For this reason, any modifications to the configuration of the S1 variant must be done to the ``s1_image`` child image.
By default, this child image is an exact duplicate of the original image, with the exception of its placement in memory.
You only have to modify the version set in the :kconfig:option:`CONFIG_FW_INFO_FIRMWARE_VERSION` Kconfig option.
To make ``s1_image`` bootable with |NSIB|, the value of :kconfig:option:`CONFIG_FW_INFO_FIRMWARE_VERSION` for ``s1_image`` must be bigger than the one for original image.
