.. _bootloader:

Immutable bootloader
####################

.. contents::
   :local:
   :depth: 2

The bootloader sample implements an immutable first stage bootloader that can verify and boot either a second stage bootloader or an application.

If the second stage bootloader is upgradable, it can reside in one of two slots.
In this case, the sample chooses the second stage bootloader with the highest version number.

Overview
********

The bootloader sample provides both a simple *Root of Trust* (RoT) and support for an upgradable second stage bootloader.

This is accomplished as follows:

1. Locking the flash memory.
     To enable the RoT, the bootloader sample locks the flash memory that contains the sample bootloader and its configuration.
     Locking is done using the hardware that is available on the given architecture.

     For additional details on locking, see the :ref:`fprotect_readme` driver.

#. Selecting the next stage in the boot chain.
     The next stage in the boot chain can either be another bootloader or an application.

     When the bootloader sample is enabled and MCUboot is used as the second stage bootloader, there are two slots in which the second stage bootloader can reside.
     The second stage bootloader in each slot has a version number associated with it, and the bootloader sample selects the second stage bootloader that has the highest version number.

     For more information on the full bootloader chain, see :ref:`ug_bootloader`.
     For more information on creating a second stage bootloader, see :ref:`mcuboot:mcuboot_wrapper`.

#. Verifying the next stage in the boot chain.
     After selecting the image to be booted next, the bootloader sample verifies the validity of the image using one of the provisioned public keys hashes.

     The image for the next boot stage has a set of metadata associated with it.
     This metadata contains the full public key corresponding to the private key that was used to sign the firmware.
     The bootloader sample checks the public key against a set of provisioned keys.

     .. note::
        To save space, only the hashes of the provisioned keys are stored, and only the hashes of the keys are compared.

     If the public key in the metadata matches one of the valid provisioned public key hashes, the image is considered valid.

     All public key hashes at lower indices than the matching hash are permanently invalidated at this point, which means that images can no longer be validated with those public keys.
     For example, if an image is successfully validated with the public key at index 2, the public keys 0 and 1 are invalidated.
     This mechanism can be used to decommission broken keys.

     If the public key does not match any of the still valid provisioned hashes, validation fails.

#. Booting the next stage in the boot chain.
     After verifying the next boot stage, the bootloader sample uninitializes all the peripherals that it used and boots the next boot stage.

#. Sharing the cryptographic library over EXT_API.
     The bootloader sample shares some of its functionality through an external API (EXT_API).

     For more information on the process, see the :file:`bl_crypto.h` file.
     For more information on EXT_API, see :ref:`doc_fw_info_ext_api`.

Flash memory layout
===================

The flash memory layout is defined by the :file:`samples/bootloader/pm.yml` file.

The bootloader sample defines four main areas:

1. *B0* - Contains the bootloader sample.
#. *Provision* - Stores the provisioned data.
#. *S0* - Defines one of the two potential storage areas for the second stage bootloader.
#. *S1* - Defines the other one of the two potential storage areas for the second stage bootloader.

.. _bootloader_provisioning:

Provisioning
============

The public key hashes are not compiled with the source code of the bootloader sample.
Instead, they must be stored in a separate memory region through a process called *provisioning*.

By default, the bootloader sample will automatically generate and provision public key hashes directly into the bootloader HEX file, based on the specified private key and additional public keys.

Alternatively, to facilitate the manufacturing process of a device with the bootloader sample, it is possible to decouple this process and program the sample HEX file and the HEX file containing the public key hashes separately.
If you choose to do so, use the Python scripts located in the :file:`scripts/bootloader` folder to create and provision the keys manually.

.. note::
   On some SoCs/SiPs, like the nRF5340 or the nRF9160, the provisioned data is held in the *One-time programmable* (OTP) region in the *User Information Configuration Registers* (UICR).
   For this reason, please note the following:

   * You must erase the UICR before programming the bootloader.
     On the nRF9160, the UICR can only be erased by erasing the entire chip:

     * On the command line, call ``west flash`` with the ``--erase`` option to erase the chip, like in the following example:

     .. parsed-literal::
        :class: highlight

        west flash -d *build_directory* --erase

       This will erase the whole chip before programming the new image.

     * In |SES|, choose :guilabel:`Target` -> :guilabel:`Connect J-Link` and then :guilabel:`Target` -> :guilabel:`Erase All` to erase the whole chip.

   * The public key hash cannot contain half-words with the value ``0xFFFF``, because half-words are writeable when they are ``0xFFFF``, so such hashes cannot be guaranteed to be immutable.
     The bootloader will refuse to boot if any hash contains a half-word with the value ``0xFFFF``.
     If your public key hash is found to have ``0xFFFF``, please regenerate it or use another public key.

The bootloader uses the :ref:`doc_bl_storage` library to access provisioned data.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns, nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52dk_nrf52832

.. _bootloader_build_and_run:

Building and running
********************

.. |sample path| replace:: :file:`samples/bootloader`

.. include:: /includes/build_and_run_bootloader.txt

Add it to any other sample, then build and program that sample as described in the following sections.

Building with |SES|
===================

The most common use case for the bootloader sample is to be included as a child image in a multi-image build, rather than being built stand-alone.
Complete the following steps to add the bootloader sample as a child image to your application:

1. Create a private key in PEM format by running the following command:

   .. code-block:: console

      openssl ecparam -name prime256v1 -genkey -noout -out priv.pem

   It will store your private key in a file named :file:`priv.pem` in the current folder.
   As OpenSSL is installed with GIT, it should be available in your GIT bash.
   See `OpenSSL`_ for more information.

   .. note::
      This step is optional for testing the bootloader chain.
      If you do not provide your own keys, debug keys are created automatically.
      However, you should never go into production with an application that is not protected by secure keys.

#. Enable Secure Boot by running ``menuconfig`` on your application:

   a. Select :guilabel:`Project` -> :guilabel:`Configure nRF Connect SDK project`.
   #. Go to :guilabel:`Modules` -> :guilabel:`Nordic nRF Connect` -> :guilabel:`Bootloader` and set :guilabel:`Use Secure Bootloader` to enable :option:`CONFIG_SECURE_BOOT`.
   #. Under :guilabel:`Private key PEM file` (:option:`CONFIG_SB_SIGNING_KEY_FILE`), enter the path to the private key that you created.
      If you choose to run the sample with default debug keys, you can skip this step.

      There are additional configuration options that you can modify, but it is not recommended to do so.
      The default settings are suitable for most use cases.

      .. note::
         If you need more flexibility with signing, or if you do not want the build system to handle your private key, choose :option:`CONFIG_SB_SIGNING_CUSTOM`.
         This option allows you to define the signing command.
         In this case, you must also specify :option:`CONFIG_SB_SIGNING_COMMAND` and :option:`CONFIG_SB_SIGNING_PUBLIC_KEY`.

   #. Click :guilabel:`Configure`.

#. Select :guilabel:`Build` -> :guilabel:`Build Solution` to compile your application.
   The build process creates two images, one for the bootloader and one for the application, and merges them together.
#. Select :guilabel:`Build` -> :guilabel:`Build and Run` to program the resulting image to your device.

Building on the command line
============================

You can easily add an immutable bootloader to most Zephyr or |NCS| samples by enabling :option:`CONFIG_SECURE_BOOT` in the sample's ``prj.conf`` file, or directly to the build command as follows:

.. code-block:: console

   west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- -DCONFIG_SECURE_BOOT=y

For more information, see :ref:`gs_programming_cmd`.

Bootloader overlays
-------------------

Overlays specific to bootloaders can be used to further modify the bootloader child image when compiling with an application:

.. code-block:: console

   west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- -DCONFIG_SECURE_BOOT=y -Db0_OVERLAY_CONFIG=overlay-minimal-size.conf

The image-specific overlay will be grabbed from the child image's respective source directory, such as :file:`samples/bootloader` in the |NCS| folder structure for ``b0``.

For more information, see :ref:`ug_multi_image_variables`.

Testing
=======

To test the bootloader sample, perform the following steps:

1. |connect_terminal|
#. Reset the development kit.
#. Observe that the development kit prints the following information (hash and boot address may vary):

.. code-block:: console

   Attempting to boot slot 0.
   Attempting to boot from address 0x9000.
   Verifying signature against key 0.
   Hash: 0x18...3f
   Firmware signature verified.
   Firmware version 1
   Setting monotonic counter (version: 1, slot: 0)

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`partition_manager`
* :ref:`doc_fw_info`
* :ref:`fprotect_readme`
* :ref:`doc_bl_crypto`
* :ref:`doc_bl_validation`
* :ref:`doc_bl_storage`

The sample also uses drivers from nrfx.
